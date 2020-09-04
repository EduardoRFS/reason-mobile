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

[@deriving yojson({strict: false})]
type node = {
  id: string,
  name: string,
  dependencies: list(string),
  devDependencies: list(string),
};

[@deriving yojson({strict: false})]
type lock = {
  root: string,
  node: StringMap.t(node),
};

[@deriving yojson({strict: false})]
type manifest = {
  dependencies: [@default None] option(StringMap.t(string)),
  // TODO: ensure we're copying devDependencies
  devDependencies: [@default None] option(StringMap.t(string)),
};

type config = {
  globalStorePrefix: string,
  project: string,
  store: string,
  localStore: string,
};

type t = {
  name: string,
  lock,
  manifest,
  build_plan: string => Lwt.t(build_plan),
  exec_env: string => Lwt.t(StringMap.t(string)),
  get_config: config,
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
let run_build_plan = (name, pkg) => {
  let+await output = run(name, ["build-plan", "-p", pkg]);
  output |> Yojson.Safe.from_string |> build_plan_of_yojson |> Result.get_ok;
};
let run_exec_env = (name, pkg) => {
  let+await output = run(name, ["exec-env", "-p", pkg, "--json"]);
  output
  |> Yojson.Safe.from_string
  |> StringMap.of_yojson([%of_yojson: string])
  |> Result.get_ok;
};
// TODO: use exec_env without json for env recreation

let make = manifest_path => {
  let.await manifest = {
    let.await manifest_data = manifest_path |> Lib.read_file;
    manifest_data
    |> Yojson.Safe.from_string
    |> manifest_of_yojson
    |> Result.get_ok
    |> await;
  };
  let (name, dir) =
    switch (manifest_path |> String.split_on_char('/') |> List.rev) {
    | [name, ...dir] => (name, List.rev(dir) |> String.concat("/"))
    | [] => assert(false)
    };
  let.await lock = {
    let lock_path =
      switch (name) {
      | "package"
      | "esy" => dir ++ "/esy.lock/index.json"
      | name => dir ++ "/" ++ name ++ ".esy.lock/index.json"
      };
    let.await lock_data = Lib.read_file(lock_path);
    lock_data
    |> Yojson.Safe.from_string
    |> lock_of_yojson
    |> Result.get_ok
    |> await;
  };
  let build_plan = run_build_plan(name);
  let exec_env = run_exec_env(name);

  let.await get_config = {
    let variables = ["project", "store", "localStore", "globalStorePrefix"];
    let.await values =
      run(
        name,
        [
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

  {name, lock, manifest, build_plan, exec_env, get_config} |> await;
};
