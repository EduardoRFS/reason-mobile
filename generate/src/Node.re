type t = {
  name: string,
  native: string,
  target: [ | `Host | `Target(string)],
  build_plan: Esy.build_plan,
  exec_env: StringMap.t(string),
  dependencies: list(string),
  patch: option(Patch.t),
};

let target_to_string =
  fun
  | `Host => "native"
  | `Target(target) => target;
let prefix = t => target_to_string(t.target) ++ "-sysroot";

let children = (nodes, t) =>
  nodes
  |> StringMap.bindings
  |> List.filter(((_key, parent)) =>
       parent.dependencies |> List.find_opt((==)(t.name)) |> Option.is_some
     );
let dependencies = (nodes, t) =>
  t.dependencies |> List.map(name => nodes |> StringMap.find(name));
