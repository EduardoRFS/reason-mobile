let includes = (t, list) =>
  list |> List.find_opt((==)(t)) |> Option.is_some;
let is_cur = s => String.length(s) >= 5 && String.sub(s, 0, 5) == "cur__";

module Known_vars = {
  let esy = [
    "SHELL",
    "PATH",
    "MAN_PATH",
    "OCAMLPATH",
    "OCAMLFIND_DESTDIR",
    "CAML_LD_LIBRARY_PATH",
    "ESY__ROOT_PACKAGE_CONFIG_PATH",
  ];

  let unset = ["OCAMLLIB", "OCAMLPATH"];
  let ignore =
    [
      "PWD",
      "cur__version",
      "cur__name",
      "cur__root",
      "cur__target_dir",
      "cur__dev",
      "LD_LIBRARY_PATH",
      "OCAML_SECONDARY_COMPILER_PREFIX",
      "OCAMLFIND_LDCONF",
      "DUNE_BUILD_DIR",
      "DUNE_STORE_ORIG_SOURCE_DIR",
    ]
    @ esy
    @ unset;
};

let find_node_manifest_env = node => {
  let extract_manifest = (kind, env) =>
    env
    |> List.filter_map(env =>
         env.Esy.Env.kind == kind ? Some(env.Esy.Env.operations) : None
       )
    |> List.concat
    |> List.map(
         fun
         | Esy.Env.Set(key, value) => (key, `String(value))
         | Esy.Env.Unset(key) => (key, `Null),
       )
    |> List.filter(((key, _)) => !includes(key, Known_vars.ignore));
  let exported_env =
    node.Node.exec_env
    |> List.rev
    |> extract_manifest(`Id(node.Node.native_id))
    |> List.filter(((key, _)) => !is_cur(key));
  // TODO: esy build-env should return self as a distinct label
  let build_env =
    node.Node.build_env
    |> List.rev
    |> extract_manifest(`Built)
    |> List.filter(((key, _)) => !is_cur(key));
  // TODO: Probably I can remove cur_env by buildPlan patch only
  let cur_env =
    node.Node.build_env
    |> extract_manifest(`Built)
    |> List.filter(((key, _)) => is_cur(key));

  (`Cur(cur_env), `Exported(exported_env), `Build(build_env));
};

let unresolve_string = (~additional_ignore=[], nodes, node, string) => {
  let replace = (node, name, use_cur, value) => {
    let {Esy.sourcePath, rootPath, buildPath, stagePath, installPath, env, _} =
      (nodes |> StringMap.find(node.Node.native)).Node.build_plan;
    let prefix = Node.prefix(node);
    let to_replace =
      [
        (name ++ ".original_root", sourcePath),
        (name ++ ".root", rootPath),
        (name ++ ".target_dir", buildPath),
        (name ++ ".install", stagePath),
        (name ++ ".install", installPath),
      ]
      |> List.map(((key, value)) => ("#{" ++ key ++ "}/" ++ prefix, value));

    let variables = env |> StringMap.bindings;

    let (`Cur(cur_env), _, _) = find_node_manifest_env(node);
    let cur_variables =
      cur_env
      |> List.filter_map(((key, value)) =>
           switch (value) {
           | `Null => None
           | `String(value) => Some((key, value))
           }
         );
    let variables =
      (use_cur ? variables @ cur_variables : variables)
      |> List.filter(((key, value)) => {
           let has_minimum_size = String.length(value) >= 3;
           let is_ignored = Known_vars.ignore |> includes(key);
           let is_additionally_ignored = additional_ignore |> includes(key);
           has_minimum_size && !is_ignored && !is_additionally_ignored;
         })
      |> List.map(((key, value)) => ("${" ++ key ++ "}", value));
    // TODO: this may be a problem for some characters
    to_replace
    @ variables
    |> List.fold_left(
         (value, (by, pattern)) => Lib.replace_all(~pattern, ~by, value),
         value,
       );
  };

  let string = replace(node, "self", true, string);
  Node.dependencies(nodes, node)
  |> List.fold_left(
       (string, dep) =>
         replace(
           dep,
           Lib.target_name(node.Node.target, dep.Node.name),
           false,
           string,
         ),
       string,
     );
};

let unresolve_env = (nodes, node, env) =>
  env
  |> List.filter(((key, _)) => !includes(key, Known_vars.ignore))
  |> List.map(((key, value)) => {
       let value =
         switch (value) {
         | `String(value) =>
           `String(
             unresolve_string(nodes, node, value, ~additional_ignore=[key]),
           )
         | `Null => `Null
         };
       (key, value);
     });
let to_exported_env = (env): Yojson.Safe.t =>
  `Assoc(
    env
    |> List.map(((key, value)) =>
         (key, `Assoc([("scope", `String("global")), ("val", value)]))
       ),
  );

let build_env_ocamlfind = (_nodes, node) => [
  ("OCAMLFIND_CONF", `String("%{store}%/findlib/#{self.id}/findlib.conf")),
  (
    "OCAMLFIND_DESTDIR",
    `String("#{self.install}/" ++ Node.prefix(node) ++ "/lib"),
  ),
  // TODO: add this only if depends on @opam/topkg
  (
    "TOPKG_CONF_TOOLCHAIN",
    `String(node.Node.target |> Node.target_to_string),
  ),
];

let patch_env_to_json = env =>
  env
  |> StringMap.bindings
  |> List.map(((key, value)) =>
       (
         key,
         switch (value) {
         | Some(value) => `String(value)
         | None => `Null
         },
       )
     );
let env = (nodes, node) => {
  let native = nodes |> StringMap.find(node.Node.native);
  let (`Cur(cur_env), `Exported(exported_env), `Build(build_env)) =
    find_node_manifest_env(native);
  let build_env =
    List.concat([
      unresolve_env(nodes, node, build_env),
      unresolve_env(nodes, node, cur_env),
      build_env_ocamlfind(nodes, node),
      switch (node.Node.patch) {
      | Some({Patch.manifest: {build_env: Some(build_env), _}, _}) =>
        patch_env_to_json(build_env)
      | _ => []
      },
      Known_vars.unset |> List.map(key => (key, `Null)),
    ]);
  let build_env: Yojson.Safe.t = `Assoc(build_env);

  let exported_env: Yojson.Safe.t =
    List.concat([
      unresolve_env(nodes, node, exported_env),
      switch (node.Node.patch) {
      | Some({Patch.manifest: {exported_env: Some(exported_env), _}, _}) =>
        patch_env_to_json(exported_env)
      | _ => []
      },
    ])
    // TODO: this is a workaround for a esy bug
    /* it did show on @opam/dune_configurator */
    |> List.map(((key, value)) => {
         let value =
           switch (node.Node.target, value) {
           | (`Target(target), `String(value)) =>
             `String(
               Lib.replace_all(~pattern="$ESY_TOOLCHAIN", ~by=target, value),
             )
           | _ => value
           };
         (key, value);
       })
    |> to_exported_env;
  (`Exported(exported_env), `Build(build_env));
};
