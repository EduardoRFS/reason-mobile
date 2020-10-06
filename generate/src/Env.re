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
      "LD_LIBRARY_PATH",
      "OCAML_SECONDARY_COMPILER_PREFIX",
    ]
    @ esy
    @ unset;
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

let find_node_manifest_env = node => {
  let rec filter_all_after_label = (label, env) => {
    let pattern = node.Node.name ++ "@";
    let rec all_until_label = (acc, env) =>
      switch (env) {
      | [Esy.Label(_), ..._] => acc
      | [entry, ...env] => all_until_label([entry, ...acc], env)
      | [] => []
      };
    switch (env) {
    | [Esy.Label(name), ...env] when Lib.starts_with(~pattern, name) =>
      all_until_label([], env)
    | [_, ...env] => filter_all_after_label(label, env)
    | [] => []
    };
  };
  let extract_env = (label, env) =>
    env
    |> filter_all_after_label(label)
    |> List.filter_map(entry =>
         switch (entry) {
         | Esy.Set(key, value) => Some((key, `String(value)))
         | Esy.Unset(key) => Some((key, `Null))
         | _ => None
         }
       )
    |> List.filter(((key, _)) => !includes(key, Known_vars.ignore));

  let exported_env = node.Node.exec_env |> extract_env(node.Node.name ++ "@");
  let build_env = node.Node.build_env |> extract_env(node.Node.name ++ "@");
  // TODO: Probably I can remove cur_env by buildPlan patch only
  let cur_env =
    node.Node.build_env
    |> extract_env("Built-in")
    |> List.filter(((key, _)) => is_cur(key));

  (`Cur(cur_env), `Exported(exported_env), `Build(build_env));
};

let build_env_ocamlfind = (_nodes, node) => [
  ("OCAMLFIND_CONF", `String("#{self.original_root}/findlib/findlib.conf")),
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
    |> to_exported_env;
  (`Exported(exported_env), `Build(build_env));
};
