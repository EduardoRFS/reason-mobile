open Helper;

type findlib = {
  target: [ | `Host(string) | `Target(string)],
  path: list(string),
  destdir: string,
  ocaml_install: string,
};
let serialize_findlib = findlib => {
  let append_prefix_to_ocamlpath = path => {
    switch (findlib.target) {
    | `Host(_) => path
    | `Target(toolchain) =>
      Filename.dirname(path)
      ++ "/"
      ++ toolchain
      ++ "-sysroot/"
      ++ Filename.basename(path)
    };
  };
  let path =
    findlib.path
    |> List.map(append_prefix_to_ocamlpath)
    |> String.concat(":");
  [
    ("path", path),
    ("destdir", findlib.destdir),
    ("stdlib", findlib.ocaml_install ++ "/lib/ocaml"),
    ("ocamlc", findlib.ocaml_install ++ "/bin/ocamlc"),
    ("ocamlopt", findlib.ocaml_install ++ "/bin/ocamlopt"),
    ("ocamlcp", findlib.ocaml_install ++ "/bin/ocamlcp"),
    ("ocamlmklib", findlib.ocaml_install ++ "/bin/ocamlmklib"),
    ("ocamlmktop", findlib.ocaml_install ++ "/bin/ocamlmktop"),
    // TODO: probably ocamldoc isn't available
    ("ocamldoc", findlib.ocaml_install ++ "/bin/ocamldoc"),
    ("ocamldep", findlib.ocaml_install ++ "/bin/ocamldep"),
  ]
  |> List.map(((key, value)) => {
       let key =
         switch (findlib.target) {
         | `Host(_) => key
         | `Target(name) => key ++ "(" ++ name ++ ")"
         };
       key ++ " = " ++ "\"" ++ value ++ "\"";
     })
  |> String.concat("\n");
};

let run_command_at_env = (env, cmd) => {
  let env =
    env
    |> Esy.Env.operations
    |> List.filter_map(
         fun
         | Esy.Env.Set(key, value) =>
           Some("export " ++ key ++ "=\"" ++ value ++ "\"")
         | Esy.Env.Unset(key) => Some("unset " ++ key),
       );
  let command = String.concat("\n", env @ [cmd]);
  Lib.exec(command);
};
let get_ocamlpath = (target, build_env) => {
  // TODO: filter only the last one
  let env =
    build_env
    |> List.filter(env =>
         switch (target, env.Esy.Env.kind) {
         | (_, `Built) => true
         // TODO: move that to Lib
         | (`Host(toolchain), `Id(id)) =>
           !Lib.starts_with(~pattern="@_" ++ toolchain, id)
         | (`Target(toolchain), `Id(id)) =>
           Lib.starts_with(~pattern="@_" ++ toolchain, id)
         }
       )
    |> List.map(env =>
         Esy.Env.{
           ...env,
           operations:
             env.operations |> List.filter((!=)(Esy.Env.Unset("OCAMLPATH"))),
         }
       );
  let ocamlpath = Esy.Env.find_variable(env, "OCAMLPATH");
  ocamlpath |> String.split_on_char(':');
};

let find_ocaml_install = (build_env, target) => {
  switch (target) {
  | `Host(_) =>
    let+await ocamlrun_path =
      run_command_at_env(build_env, "dirname $(dirname $(which ocamlrun))");
    ocamlrun_path |> String.trim;
  | `Target(_) =>
    Esy.Env.find_variable(~optional=true, build_env, "ESY_TOOLCHAIN_OCAML")
    |> await
  };
};

let emit = (esy, target, name) => {
  let.await build_env = esy.Esy.build_env(name);
  let destdir = Esy.Env.find_variable(build_env, "OCAMLFIND_DESTDIR");

  let.await host_findlib = {
    let target = `Host(target);
    let path = get_ocamlpath(target, build_env);
    let+await ocaml_install = find_ocaml_install(build_env, target);
    serialize_findlib({target, path, destdir, ocaml_install});
  };
  let.await target_findlib = {
    let target = `Target(target);
    let path = get_ocamlpath(target, build_env);
    let+await ocaml_install = find_ocaml_install(build_env, target);
    serialize_findlib({target, path, destdir, ocaml_install});
  };
  await((host_findlib, target_findlib));
};
