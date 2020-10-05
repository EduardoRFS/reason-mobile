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

  let ignore = [
    "PWD",
    "cur__version",
    "cur__name",
    "LD_LIBRARY_PATH",
    "OCAML_SECONDARY_COMPILER_PREFIX",
    ...esy,
  ];

  let unset = ["OCAMLLIB", "OCAMLPATH"];
};

// TODO: probably this could be removed by using exec-env without --json
let var_is_missing_in_deps = (nodes, node) => {
  let deps_variable =
    Node.dependencies(nodes, node)
    |> List.concat_map(dep => dep.Node.exec_env |> StringMap.bindings);
  // TODO: that may cause a bug if we have A: B -> A: C -> A: B
  variable => !List.exists((==)(variable), deps_variable);
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

    let (cur_variables, non_cur_variables) =
      env
      |> StringMap.bindings
      |> List.filter(((key, value)) => {
           let has_minimum_size = String.length(value) >= 3;
           let is_ignored = Known_vars.ignore |> includes(key);
           let is_additionally_ignored = additional_ignore |> includes(key);
           has_minimum_size && !is_ignored && !is_additionally_ignored;
         })
      |> List.partition(((key, _)) => is_cur(key));

    // TODO: this may be a problem for some characters
    to_replace
    @ non_cur_variables
    @ (use_cur ? cur_variables : [])
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
  |> List.filter_map(((key, value)) => {
       let new_value =
         switch (value) {
         | `String(value) =>
           `String(
             unresolve_string(nodes, node, value, ~additional_ignore=[key]),
           )
         | `Null => `Null
         };
       new_value != value ? Some((key, new_value)) : None;
     });
let to_exported_env = (env): Yojson.Safe.t =>
  `Assoc(
    env
    |> List.map(((key, value)) =>
         (key, `Assoc([("scope", `String("global")), ("val", value)]))
       ),
  );

let find_node_manifest_env = (nodes, node) => {
  let node_variables =
    node.Node.build_plan.env
    |> StringMap.bindings
    |> List.filter(((key, _)) =>
         !includes(key, Known_vars.ignore) && !is_cur(key)
       );
  let exported_env =
    node.Node.exec_env
    |> StringMap.bindings
    |> List.filter(((key, value)) => {
         switch (node.Node.build_plan.env |> StringMap.find_opt(key)) {
         | Some(found_value) => found_value != value
         | None => false
         }
       })
    |> List.map(((key, value)) => (key, `String(value)));

  let build_env =
    node_variables
    |> List.filter(var_is_missing_in_deps(nodes, node))
    |> List.map(((key, value)) => (key, `String(value)));
  (`Exported(exported_env), `Build(build_env));
};

let build_env_ocamlfind = (_nodes, node) => [
  ("OCAMLFIND_CONF", `String("#{self.root}/findlib/findlib.conf")),
  (
    "OCAMLFIND_DESTDIR",
    `String("#{self.install}/" ++ Node.prefix(node) ++ "/lib"),
  ),
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
  let (`Exported(exported_env), `Build(build_env)) =
    find_node_manifest_env(nodes, native);
  let cur_env =
    native.Node.build_plan.env
    |> StringMap.bindings
    |> List.filter(((key, _)) => is_cur(key))
    |> List.map(((key, value)) => (key, `String(value)));
  let build_env =
    List.concat([
      [("NOT_OCAMLPATH", `String("$OCAMLPATH"))],
      Known_vars.unset |> List.map(key => (key, `Null)),
      unresolve_env(nodes, node, build_env),
      unresolve_env(nodes, node, cur_env),
      build_env_ocamlfind(nodes, node),
      switch (node.Node.patch) {
      | Some({Patch.manifest: {build_env: Some(build_env), _}, _}) =>
        patch_env_to_json(build_env)
      | _ => []
      },
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
    |> to_exported_env;
  (`Exported(exported_env), `Build(build_env));
};
