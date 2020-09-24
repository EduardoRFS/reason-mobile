open Helper;

// TODO: validate if things were installed in install/prefix and not only install/

type command = list(string);

let unresolve_commands = (nodes, node, commands) =>
  commands
  |> List.map(command =>
       command |> List.map(part => Env.unresolve_string(nodes, node, part))
     );

let to_script = commands => {
  let escape_arg = arg => {
    let escaped = arg |> Lib.replace_all(~pattern="\"", ~by="\\\\\"");
    "\"" ++ escaped ++ "\"";
  };
  let script =
    commands
    |> List.map(command =>
         command |> List.map(escape_arg) |> String.concat(" ")
       )
    |> String.concat("\n");
  [["sh", "-c", "set -e\n" ++ script]];
};

// TODO: check that against a huge base of packages
// TODO: search for more installers like make install
let is_installer =
  fun
  | [] => false
  | [cmd, ..._] => cmd == "esy-installer";

let gen_findlib = (nodes, node) => {
  let rec needs_gen_findlib = (nodes, node) => {
    let is_ocaml = () => node.Node.name == "ocaml";
    let dep_needs_gen_findlib = () =>
      Node.dependencies(nodes, node)
      |> List.exists(needs_gen_findlib(nodes));
    is_ocaml() || dep_needs_gen_findlib();
  };
  needs_gen_findlib(nodes, node) ? [["not-esy-gen-findlib"]] : [];
};
let copy_source = node => {
  let patchSource = {
    let.some patch = node.Node.patch;
    patch.Patch.manifest.source;
  };

  let from = node.Node.build_plan.sourcePath ++ "/.";
  let to_ = "#{self.root}/" ++ Node.prefix(node);
  let default = [["rm", "-rf", to_], ["cp", "-r", "-p", from, to_]];
  patchSource |> Option.value(~default);
};
let copy_patch_files = node => {
  let.apply () = Option.value(~default=[]);
  let+some _ = {
    let.some patch = node.Node.patch;
    patch.Patch.files_folder;
  };
  let to_ = "#{self.root}/" ++ Node.prefix(node);
  [["cp", "-r", "#{self.original_root}/files/.", to_]];
};
let setup_install = node => [
  ["mv", "#{self.install}", "install"],
  ["mkdir", "-p", "#{self.install}/_esy"],
  ["mv", "install", "#{self.install}" ++ Node.prefix(node)],
];
let patch_dune = (node, command) => {
  // TODO: test
  // dune build -p pkg
  // refmterr dune build -p pkg
  let is_dune_build = () =>
    switch (command) {
    | ["dune" | "jbuilder", "build", ..._]
    | [_, "dune" | "jbuilder", "build", ..._] => true
    | _ => false
    };
  is_dune_build() ? command @ ["-x", node.Node.target, "@install"] : command;
};
// TODO: autoconf

let build = (nodes, node) => {
  let source_build = node.Node.build_plan.build |> Option.value(~default=[]);

  let commands =
    unresolve_commands(nodes, node, source_build)
    |> List.map(patch_dune(node))
    |> List.filter(command => !is_installer(command));

  copy_source(node)
  @ copy_patch_files(node)
  @ setup_install(node)
  @ gen_findlib(nodes, node)
  @ to_script([["cd", node |> Node.prefix]] @ commands);
};
let install = (nodes, node) => {
  let source_install =
    node.Node.build_plan.install |> Option.value(~default=[]);
  let commands =
    unresolve_commands(nodes, node, source_install)
    |> List.filter(command => !is_installer(command));
  let commands = commands @ [["not-esy-installer", node.Node.target]];
  to_script([["cd", node |> Node.prefix]] @ commands);
};
