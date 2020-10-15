open Helper;
open Lib;

Printexc.record_backtrace(true);
// TODO: check if is inside of a esy shell

[@deriving yojson]
type mock_esy_manifest = {
  buildsInSource: bool,
  buildEnv: Yojson.Safe.t,
  exportedEnv: Yojson.Safe.t,
  build: list(Commands.command),
  install: list(Commands.command),
};

[@deriving yojson]
type mock_manifest = {
  name: string,
  dependencies: StringMap.t(string),
  esy: mock_esy_manifest,
};

type mock = {
  manifest: mock_manifest,
  path: string,
  file: string,
  patch_files_folder: option(string),
};

type target = {
  system: string,
  arch: string,
  name: string,
};

module type Installer = {let main: unit => Lwt.t(unit);};
if (Sys.argv[1] == "--install") {
  module Installer =
    Installer.Make({
      let install_folder = Sys.getenv("cur__install");
      let target = Sys.argv[2];
    });
  Installer.main() |> Lwt_main.run;
  exit(0);
};

let sysroot_folder = {
  let default =
    Filename.concat(
      Filename.current_dir_name,
      Filename.concat("..", "sysroot"),
    );
  let.apply () = Option.value(~default);
  Sys.getenv_opt("ESY__GENERATE_SYSROOT");
};

let esy = {
  let.apply () = Lwt_main.run;
  let.await manifest =
    switch (Sys.getenv_opt("ESY__ROOT_PACKAGE_CONFIG_PATH")) {
    | Some(path) => Filename.basename(path) |> await
    | None =>
      let+await exists = Lwt_unix.file_exists("./esy.json");
      exists ? "esy.json" : "package.json";
    };
  Esy.make(manifest);
};

// TODO: multiple targets
let target = {
  let parts = Sys.argv[1] |> String.split_on_char('.');
  switch (parts |> List.rev) {
  | [arch, ...system] =>
    let system = system |> List.rev |> String.concat(".");
    {arch, system, name: system ++ "." ++ arch};
  | _ => failwith("invalid system name")
  };
};

let create_nodes = () => {
  let nodes = esy.Esy.lock.node |> StringMap.bindings |> List.map(snd);

  let dependencies_map =
    nodes
    |> List.map(node => {
         let dependencies =
           node.Esy.Node.dependencies @ node.Esy.Node.devDependencies;
         let dependencies =
           dependencies
           |> List.map(id => {
                let node = esy.Esy.lock.node |> StringMap.find(id);
                node.Esy.Node.name;
              });
         (node.Esy.Node.name, dependencies);
       })
    |> List.to_seq
    |> StringMap.of_seq;

  let.await env_plan_map =
    nodes
    |> List.map(node => {
         let name = node.Esy.Node.name;
         let.await build_map = esy.Esy.build_plan(name)
         and.await build_env = esy.Esy.build_env(name)
         and.await exec_env = esy.Esy.exec_env(name);

         Lwt.return((
           name,
           (build_map, `Build(build_env), `Exec(exec_env)),
         ));
       })
    |> Lwt.all;
  let env_plan_map = env_plan_map |> List.to_seq |> StringMap.of_seq;

  nodes
  |> List.concat_map(node =>
       [`Host, `Target(target.name)] |> List.map(target => (node, target))
     )
  |> List.map(((node, target)) => {
       let name = Lib.target_name(target, node.Esy.Node.name);
       let native = node.Esy.Node.name;
       let env_plan = env_plan_map |> StringMap.find(node.Esy.Node.name);
       let (build_plan, `Build(build_env), `Exec(exec_env)) = env_plan;
       let.await patch =
         Patch.get_path(
           Lib.escape_name(node.name),
           switch (build_plan.Esy.version |> String.split_on_char(':')) {
           | [version] => version
           | [_, ...version] => String.concat(":", version)
           | [] => ""
           },
         );

       let build_plan =
         switch (patch) {
         | Some({Patch.manifest: {build, install, _}, _}) =>
           let build = build |> Option.is_some ? build : build_plan.Esy.build;
           let install =
             install |> Option.is_some ? install : build_plan.Esy.install;
           Esy.{...build_plan, build, install};
         | None => build_plan
         };

       let dependencies = dependencies_map |> StringMap.find(node.name);
       let dependencies =
         switch (patch) {
         | Some({
             Patch.manifest: {dependencies: Some(patch_dependencies), _},
             _,
           }) =>
           dependencies
           @ (patch_dependencies |> StringMap.bindings |> List.map(fst))
         | _ => dependencies
         };
       Lwt.return({
         Node.name,
         native_id: node.id,
         native,
         target,
         build_plan,
         build_env,
         exec_env,
         dependencies,
         patch,
       });
     })
  |> Lwt.all;
};
let add_as_mock = (name, folder) => {
  let.await hash = Lib.folder_sha1(folder);
  let hash = String.sub(hash, 0, 8);
  let mock_folder = ".mocks/" ++ hash;
  let.await _ = Lib.exec("cp -a " ++ folder ++ " " ++ mock_folder);
  Lwt.return((name, mock_folder ++ "/package.json"));
};

let main = () => {
  let.await nodes = create_nodes();

  let root_host = esy.Esy.lock.node |> StringMap.find(esy.Esy.lock.root);
  let source_name = root_host.Esy.Node.name;

  let nodes_map =
    nodes |> List.map(node => (node.Node.name, node)) |> StringMap.of_list;

  let dependencies_map =
    nodes_map
    |> StringMap.map(node => {
         let dependencies =
           node.Node.dependencies
           |> List.map(Lib.target_name(node.Node.target));
         [
           node.Node.native,
           Lib.target_name(node.target, "sysroot"),
           // TODO: split installer from generate to avoid cache invalidation
           ...dependencies,
         ]
         @ ["generate"];
       });

  let build_map =
    nodes_map
    |> StringMap.map(node =>
         Commands.build(
           ~is_root=node.Node.native == source_name,
           nodes_map,
           node,
         )
       );
  let install_map =
    nodes_map |> StringMap.map(node => Commands.install(nodes_map, node));
  let env_map = nodes_map |> StringMap.map(node => Env.env(nodes_map, node));
  let versions_map =
    nodes_map
    |> StringMap.filter((_, node) => node.Node.target == `Host)
    |> StringMap.map(node => node.Node.build_plan.version);

  // TODO: ensure the order is kept
  let mocks =
    nodes
    |> List.filter(node => node.Node.target != `Host)
    |> List.map(node => {
         let env = env_map |> StringMap.find(node.Node.name);
         let (`Exported(exportedEnv), `Build(buildEnv)) = env;
         let build = build_map |> StringMap.find(node.Node.name);
         let install = install_map |> StringMap.find(node.Node.name);

         let esy = {
           buildsInSource: true,
           buildEnv,
           exportedEnv,
           build,
           install,
         };

         let raw_dependencies =
           {
             let.some patch = node.Node.patch;
             patch.Patch.manifest.raw_dependencies;
           }
           |> Option.value(~default=StringMap.empty)
           |> StringMap.bindings
           |> List.map(fst);
         let dependencies =
           List.append(
             dependencies_map |> StringMap.find(node.Node.name),
             raw_dependencies,
           )
           |> List.map(name => {
                let version =
                  versions_map
                  |> StringMap.find_opt(node.Node.name)
                  |> Option.value(~default="*")
                  |> String.split_on_char(':');
                let version =
                  switch (version) {
                  | [version] => version
                  | [_, ...version] => version |> String.concat(":")
                  | [] => ""
                  };
                (name, version);
              })
           |> StringMap.of_list;

         let manifest = {name: node.Node.name, dependencies, esy};
         let checksum = {
           let manifest_json =
             manifest |> mock_manifest_to_yojson |> Yojson.Safe.to_string;
           let patch_checksum = {
             let.apply () = Option.value(~default="");
             let.some patch = node.Node.patch;
             patch.Patch.checksum;
           };
           let hash = Lib.sha1(manifest_json ++ patch_checksum);
           String.sub(hash, 0, 8);
         };
         let path = ".mocks/" ++ checksum;
         let file = path ++ "/esy.json";
         let patch_files_folder = {
           let.some patch = node.Node.patch;
           patch.Patch.files_folder;
         };
         {manifest, path, file, patch_files_folder};
       });

  let.await () = mkdirp(".mocks");
  let.await _ =
    mocks
    |> List.map(({path, file, manifest, patch_files_folder}) => {
         let.await () = mkdirp(path);
         let copy_files_folder = files_folder => {
           let new_files_folder = path ++ "/files";
           let.await () = mkdirp(new_files_folder);
           let.await _ =
             exec("cp -r " ++ files_folder ++ "/. " ++ new_files_folder);
           await();
         };
         let copy_files_folder =
           switch (patch_files_folder) {
           | Some(files_folder) => copy_files_folder(files_folder)
           | None => await()
           };

         Lwt.both(
           copy_files_folder,
           manifest
           |> mock_manifest_to_yojson
           |> Yojson.Safe.pretty_to_string
           |> Lib.write_file(~file),
         );
       })
    |> Lwt.all;

  let root_name = target_name(`Target(target.name), source_name);
  let root = mocks |> List.find(mock => mock.manifest.name == root_name);

  let.await additional_resolutions = {
    let.await tools =
      add_as_mock("sysroot.tools", Filename.concat(sysroot_folder, "tools"))
    // TODO: avoid copying ndk and musl_cc when not needed
    and.await sysroot =
      add_as_mock(
        "@_" ++ target.name ++ "/sysroot",
        Filename.concat(sysroot_folder, target.name),
      )
    and.await ndk =
      add_as_mock(
        "@_android/ndk",
        Filename.concat(sysroot_folder, "android.ndk"),
      )
    and.await musl_cc =
      add_as_mock(
        "@_linux.musl/cc",
        Filename.concat(sysroot_folder, "linux.musl.cc"),
      );
    await(
      [tools, sysroot]
      @ (target.system == "android" ? [ndk] : [])
      @ (target.system == "linux.musl" ? [musl_cc] : []),
    );
  };
  let resolutions =
    [
      {
        esy.Esy.lock.node
        |> StringMap.bindings
        |> List.filter_map(((_, node)) => {
             let version = node.Esy.Node.version;
             switch (version |> String.split_on_char(':')) {
             | ["github", ..._]
             | ["path", ..._] =>
               Some((node.Esy.Node.name, `String(version)))
             // TODO: only known case is esy-gmp
             | ["archive", ..._] =>
               switch (node.Esy.Node.overrides) {
               | [`String(manifest)] =>
                 Some((node.Esy.Node.name, `String(manifest)))
               | _ => None
               }
             | _ => None
             };
           });
      },
      mocks
      |> List.map(({file, manifest, _}) => (manifest.name, `String(file))),
      esy.Esy.manifest.resolutions |> StringMap.bindings,
      additional_resolutions
      |> List.map(((manifest, file)) => (manifest, `String(file))),
      // TODO: support target on the manifest
    ]
    |> List.concat;

  let dependencies: list((string, Yojson.Safe.t)) =
    [
      esy.Esy.manifest.dependencies |> StringMap.bindings,
      esy.Esy.manifest.devDependencies |> StringMap.bindings,
      root.manifest.dependencies |> StringMap.bindings,
    ]
    |> List.concat
    |> List.filter(((key, _)) => key != source_name)
    |> List.map(((key, value)) => (key, `String(value)));
  // TODO: support target on the manifest

  let root_mock = mocks |> List.find(mock => mock.manifest.name == root_name);
  let wrapper =
    `Assoc([
      ("dependencies", `Assoc(dependencies)),
      ("resolutions", `Assoc(resolutions)),
      (
        "esy",
        {...root_mock.manifest.esy, buildsInSource: false, install: []}
        |> mock_esy_manifest_to_yojson,
      ),
    ]);

  let json_file = target.name ++ ".json";
  let.await () =
    Lib.write_file(
      ~file=target.name ++ ".json",
      wrapper |> Yojson.Safe.pretty_to_string,
    );

  let.await _ = Lib.exec("esy @" ++ target.name ++ " install");
  let.await new_lock = Esy.make(json_file);
  let.await _ =
    mocks
    |> List.map(mock =>
         mock.manifest.name == root_name
           ? (
               new_lock.Esy.lock.node |> StringMap.find(new_lock.Esy.lock.root)
             ).Esy.Node.name
           : mock.manifest.name
       )
    |> List.map(name => {
         let.await build_plan = new_lock.Esy.build_plan(name);
         let root =
           new_lock.Esy.config.store ++ "/findlib/" ++ build_plan.Esy.id;
         let conf_d = root ++ "/findlib.conf.d";
         let.await () = Lib.mkdirp(conf_d);
         let.await (host_findlib, target_findlib) =
           Findlib.emit(new_lock, target.name, name);
         let.await () =
           Lib.write_file(~file=root ++ "/findlib.conf", host_findlib)
         and.await () =
           Lib.write_file(
             ~file=conf_d ++ "/" ++ target.name ++ ".conf",
             target_findlib,
           );
         await();
       })
    |> Lwt.all;
  await();
};

let () = main() |> Lwt_main.run;
