type t = {
  name: string,
  native: string,
  target: string,
  build_plan: Esy.build_plan,
  exec_env: StringMap.t(string),
  dependencies: list(string),
  patch: option(Patch.t),
};

let prefix = t => t.target ++ "-sysroot";
let children = (nodes, t) =>
  nodes
  |> StringMap.bindings
  |> List.filter(((_key, parent)) =>
       parent.dependencies |> List.find_opt((==)(t.name)) |> Option.is_some
     );
let dependencies = (nodes, t) =>
  t.dependencies |> List.map(name => nodes |> StringMap.find(name));
