open Helper;

[@deriving yojson]
type command = list(string);

[@deriving yojson({strict: false})]
type build_plan = {
  id: string,
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

// TODO: replace the esy build-env default by a structure representation of the shell script version
module Env = {
  [@deriving yojson]
  type operation =
    | Set(string, string)
    | Unset(string);

  type entry = {
    kind: [ | `Built | `Id(string)],
    operations: list(operation),
  };
  type t = list(entry);

  let operations = List.concat_map(t => t.operations);

  let find_variable = (~optional=false, t: t, var) => {
    // TODO: find a proper solution for this
    let apply_env = (env, value) =>
      env
      |> StringMap.bindings
      |> List.sort_uniq(((left, _), (right, _)) =>
           String.length(right) - String.length(left)
         )
      |> List.concat_map(((key, value)) =>
           [("${" ++ key ++ "}", value), ("$" ++ key, value)]
         )
      |> List.fold_left(
           (value, (pattern, by)) =>
             value |> Lib.replace_all(~pattern, ~by),
           value,
         );
    let value =
      t
      |> operations
      |> List.fold_left(
           (env, op) =>
             switch (op) {
             | Set(key, value) =>
               env |> StringMap.add(key, apply_env(env, value))
             | Unset(key) => env |> StringMap.remove(key)
             },
           StringMap.empty,
         )
      |> StringMap.find_opt(var);
    switch (value, optional) {
    | (Some(value), _) => value
    | (None, true) => ""
    | (None, false) => failwith("couldn't found $" ++ var)
    };
  };

  let parse_from_shell = commands => {
    let remove_header = commands => {
      let rec remove_all_until_whitespace = commands =>
        switch (commands) {
        | ["", ...commands] => commands
        | [_, ...commands] => remove_all_until_whitespace(commands)
        | [] => []
        };
      switch (commands) {
      | ["# Build environment" | "# Exec environment", ...commands] =>
        remove_all_until_whitespace(commands)
      | commands => commands
      };
    };
    let commands_without_header =
      commands
      |> String.split_on_char('\n')
      |> List.map(String.trim)
      |> remove_header
      |> List.filter(line => line != "" && line != "#");
    let (t, last_entry) =
      commands_without_header
      |> List.fold_left(
           ((entries, entry), line) =>
             switch (line |> String.split_on_char(' ')) {
             | ["export", ...assignment] =>
               let (key, value) =
                 switch (
                   assignment
                   |> String.concat(" ")
                   |> String.split_on_char('=')
                 ) {
                 | [key, ...value] => (key, value |> String.concat("="))
                 | _ => failwith("invalid export")
                 };
               let value =
                 value |> String.length >= 2
                   ? switch (value.[0], value.[String.length(value) - 1]) {
                     | ('"', '"') =>
                       String.sub(value, 1, String.length(value) - 2)
                     | _ => value
                     }
                   : value;
               (
                 entries,
                 {
                   ...entry,
                   operations: entry.operations @ [Set(key, value)],
                 },
               );
             | ["unset", name] => (
                 entries,
                 {...entry, operations: entry.operations @ [Unset(name)]},
               )
             | ["#", "Built-in"] => (
                 entries @ [entry],
                 {kind: `Built, operations: []},
               )
             // TODO: assert this is an id
             | ["#", id] => (
                 entries @ [entry],
                 {kind: `Id(id), operations: []},
               )
             | line =>
               failwith(
                 "unknown declaration on env: \""
                 ++ String.concat(" ", line)
                 ++ "\"",
               )
             },
           ([], {kind: `Built, operations: []}),
         );
    t @ [last_entry] |> List.filter(env => env.operations != []);
  };
};

type t = {
  name: string,
  lock,
  manifest,
  build_plan: string => Lwt.t(build_plan),
  build_env: string => Lwt.t(Env.t),
  exec_env: string => Lwt.t(Env.t),
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
  output |> Env.parse_from_shell;
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
