import {
  t as node,
  prefix as node_prefix,
  dependencies as node_dependencies,
} from './node';
import { map } from './lib';
import { UNSET_VARS, unresolve_string } from './env';

// TODO: validate if things were installed in install/prefix and not only install/

type command = string[];

const unresolve_commands = (
  nodes: map<node>,
  node: node,
  commands: command[]
): command[] =>
  commands.map((command) =>
    command.map((part) => unresolve_string(nodes, node, part))
  );

const to_script = (commands: command[]): command[] => {
  // TODO: escape this properly
  const script = commands.map((command) => command.join(' ')).join('\n');

  return [['sh', '-c', `set -e\n${script}`]];
};

// TODO: check that against a huge base of packages
// TODO: search for more installers like make install
const is_installer = (command: command) => command[0] === 'esy-installer';
const needs_gen_findlib = (nodes: map<node>, node: node): boolean => {
  /* anything that uses ocaml will be translated to @_toolchain/ocaml
     and @_toolchain/ocaml SHOULD depends on @opam/ocamlfind */
  if (node.name === 'ocaml') {
    return true;
  }

  return node_dependencies(nodes, node).some((node) =>
    needs_gen_findlib(nodes, node)
  );
};

// TODO: should it be created during the generate step?
const gen_findlib = (_nodes: map<node>, node: node) => [
  ['not-esy-gen-findlib'],
];
const clean_env = (_nodes: map<node>, _node: node): command[] =>
  UNSET_VARS.map((key) => ['unset', key]);
const copy_source = (_nodes: map<node>, node: node): command[] => {
  if (node.patch && node.patch.source) {
    return node.patch.source;
  }
  const from = `${node.build_plan.sourcePath}/.`;
  const to = `#{self.root}/${node_prefix(node)}`;
  return [
    ['rm', '-rf', to],
    ['cp', '-r', '-p', from, to],
  ];
};
const copy_patch_files = (_nodes: map<node>, node: node): command[] => {
  if (!node.patch || !node.patch.files_folder) {
    return [];
  }
  const to = `#{self.root}/${node_prefix(node)}`;
  return [['cp', '-r', `${node.patch.files_folder}/.`, to]];
};
const setup_install = (_nodes: map<node>, node: node): command[] => [
  ['mv', '#{self.install}', 'install'],
  ['mkdir', '-p', '#{self.install}/_esy'],
  ['mv', 'install', `#{self.install}/${node_prefix(node)}`],
];

const patch_dune = (node: node, command: command) => {
  // TODO: test
  // dune build -p pkg
  // refmterr dune build -p pkg
  const is_dune_build = () =>
    (command.slice(0, 2).includes('dune') ||
      command.slice(0, 2).includes('jbuilder')) &&
    command.includes('build');
  return is_dune_build()
    ? command.concat(['-x', node.target, '@install'])
    : command;
};

export const build = (nodes: map<node>, node: node): command[] => {
  const source_build = node.build_plan.build || [];
  const commands = unresolve_commands(nodes, node, source_build)
    .map((command) => patch_dune(node, command))
    .filter((command) => !is_installer(command));

  return [
    ...copy_source(nodes, node),
    ...copy_patch_files(nodes, node),
    ...setup_install(nodes, node),
    ...(needs_gen_findlib(nodes, node) ? gen_findlib(nodes, node) : []),
    ...to_script([
      ...clean_env(nodes, node),
      ['cd', node_prefix(node)],
      ...commands,
    ]),
  ];
};
export const install = (nodes: map<node>, node: node): command[] => {
  const source_install = node.build_plan.install || [];
  const commands = unresolve_commands(nodes, node, source_install)
    .filter((command) => !is_installer(command))
    .concat([['not-esy-installer', node.target]]);
  return to_script([
    ...clean_env(nodes, node),
    ['cd', node_prefix(node)],
    ...commands,
  ]);
};
