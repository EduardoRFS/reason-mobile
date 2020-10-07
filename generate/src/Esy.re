open Helper;

[@deriving yojson]
type command = list(string);

[@deriving yojson({strict: false})]
type build_plan = {
  name: string,
  version: string,
  build: [@default None] option(list(command)),
  install: [@default None] option(list(command)),
  sourcePath: string,
  rootPath: string,
  buildPath: string,
  stagePath: string,
  installPath: string,
  env: StringMap.t(string),
};

module Node = {
  [@deriving yojson({strict: false})]
  type t = {
    id: string,
    name: string,
    dependencies: list(string),
    devDependencies: list(string),
  };
};

[@deriving yojson({strict: false})]
type lock = {
  root: string,
  node: StringMap.t(Node.t),
};

[@deriving yojson({strict: false})]
type manifest = {
  dependencies: [@default StringMap.empty] StringMap.t(string),
  // TODO: ensure we're copying devDependencies
  devDependencies: [@default StringMap.empty] StringMap.t(string),
  resolutions: [@default StringMap.empty] StringMap.t(Yojson.Safe.t),
};

type config = {
  globalStorePrefix: string,
  project: string,
  store: string,
  localStore: string,
};

[@deriving yojson]
type env =
  | Set(string, string)
  | Unset(string)
  | Label(string);

type t = {
  name: string,
  lock,
  manifest,
  build_plan: string => Lwt.t(build_plan),
  build_env: string => Lwt.t(list(env)),
  exec_env: string => Lwt.t(list(env)),
  config,
};

let run = (name, args) => {
  let cmd =
    List.concat([
      ["esy"],
      switch (name) {
      | "package"
      | "esy" => []
      | _ => ["@" ++ name]
      },
      args,
    ])
    |> String.concat(" ");
  let+await stdout = Lib.exec(cmd);
  stdout |> String.trim;
};

let apply_config = (config, value) => {
  let dict =
    [
      ("globalStorePrefix", config.globalStorePrefix),
      ("project", config.project),
      ("store", config.store),
      ("localStore", config.localStore),
    ]
    |> List.concat_map(((pattern, by)) =>
         [("%{" ++ pattern ++ "}%", by), (pattern, by)]
       );
  dict
  |> List.fold_left(
       (value, (pattern, by)) => value |> Lib.replace_all(~pattern, ~by),
       value,
     );
};

let run_build_plan = (name, config, pkg) => {
  let+await output = run(name, ["build-plan", "-p", pkg]);
  let build_plan =
    output |> Yojson.Safe.from_string |> build_plan_of_yojson |> Result.get_ok;
  {
    ...build_plan,
    sourcePath: apply_config(config, build_plan.sourcePath),
    rootPath: apply_config(config, build_plan.rootPath),
    buildPath: apply_config(config, build_plan.buildPath),
    stagePath: apply_config(config, build_plan.stagePath),
    installPath: apply_config(config, build_plan.installPath),
  };
};

let get_esy_env = (kind, name, pkg) => {
  let command =
    switch (kind) {
    | `Exec => "exec-env"
    | `Build => "build-env"
    };
  let+await output = run(name, [command, "-p", pkg]);
  output
  |> String.split_on_char('\n')
  |> List.map(String.trim)
  |> List.filter(line => line != "" && line != "#")
  |> List.filter_map(line =>
       switch (line |> String.split_on_char(' ')) {
       | ["export", ...assignment] =>
         let (key, value) =
           switch (
             assignment |> String.concat(" ") |> String.split_on_char('=')
           ) {
           | [key, ...value] => (key, value |> String.concat("="))
           | _ => failwith("invalid export")
           };
         Some(Set(key, value));
       | ["unset", name] => Some(Unset(name))
       | ["#", label] => Some(Label(label))
       | ["#", ..._] => None
       | _ => failwith("unknown declaration on env")
       }
     );
};

let make = manifest_path => {
  let.await manifest = {
    let.await manifest_data = manifest_path |> Lib.read_file;
    manifest_data
    |> Yojson.Safe.from_string
    |> manifest_of_yojson
    |> Result.get_ok
    |> await;
  };
  let name =
    switch (manifest_path |> String.split_on_char('/') |> List.rev) {
    | [name, ..._] => Filename.remove_extension(name)
    | [] => assert(false)
    };
  let.await lock = {
    let lock_path =
      switch (name) {
      | "package"
      | "esy" => "esy.lock/index.json"
      | name => name ++ ".esy.lock/index.json"
      };
    let.await lock_data = Lib.read_file(lock_path);
    lock_data
    |> Yojson.Safe.from_string
    |> lock_of_yojson
    |> Result.get_ok
    |> await;
  };

  let.await config = {
    let variables = ["project", "store", "localStore", "globalStorePrefix"];
    let.await values =
      run(
        name,
        [
          "exec-command",
          "echo",
          variables |> List.map(v => "%{" ++ v ++ "}%") |> String.concat(":"),
        ],
      );
    switch (values |> String.split_on_char(':')) {
    | [project, store, localStore, globalStorePrefix] =>
      {project, store, localStore, globalStorePrefix} |> await
    | _ => assert(false)
    };
  };
  let build_plan = run_build_plan(name, config);
  let build_env = get_esy_env(`Build, name);
  let exec_env = get_esy_env(`Exec, name);

  {name, lock, manifest, build_plan, build_env, exec_env, config} |> await;
};
