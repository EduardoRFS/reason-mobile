"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const fs_1 = __importDefault(require("fs"));
const node_1 = require("./node");
const env_1 = require("./env");
const unresolve_commands = (nodes, node, commands) => commands.map((command) => command.map((part) => env_1.unresolve_string(nodes, node, part)));
const to_script = (commands) => {
    const escape_arg = (arg) => {
        const escaped = arg.replace(/"/g, '\\\\"');
        return `"${escaped}"`;
    };
    const script = commands
        .map((command) => command.map(escape_arg).join(' '))
        .join('\n');
    return [['sh', '-c', `set -e\n${script}`]];
};
// TODO: check that against a huge base of packages
// TODO: search for more installers like make install
const is_installer = (command) => command[0] === 'esy-installer';
const needs_gen_findlib = (nodes, node) => {
    /* anything that uses ocaml will be translated to @_toolchain/ocaml
       and @_toolchain/ocaml SHOULD depends on @opam/ocamlfind */
    if (node.name === 'ocaml') {
        return true;
    }
    return node_1.dependencies(nodes, node).some((node) => needs_gen_findlib(nodes, node));
};
// TODO: should it be created during the generate step?
const gen_findlib = (_nodes, node) => [
    ['not-esy-gen-findlib'],
];
const clean_env = (_nodes, _node) => env_1.UNSET_VARS.map((key) => ['unset', key]);
const copy_source = (_nodes, node) => {
    if (node.patch && node.patch.source) {
        return node.patch.source;
    }
    const from = `${node.build_plan.sourcePath}/.`;
    const to = `#{self.root}/${node_1.prefix(node)}`;
    return [
        ['rm', '-rf', to],
        ['cp', '-r', '-p', from, to],
    ];
};
const copy_patch_files = (_nodes, node) => {
    if (!node.patch || !node.patch.files_folder) {
        return [];
    }
    const to = `#{self.root}/${node_1.prefix(node)}`;
    return [['cp', '-r', `${node.patch.files_folder}/.`, to]];
};
const setup_install = (_nodes, node) => [
    ['mv', '#{self.install}', 'install'],
    ['mkdir', '-p', '#{self.install}/_esy'],
    ['mv', 'install', `#{self.install}/${node_1.prefix(node)}`],
];
const patch_dune = (node, command) => {
    // TODO: test
    // dune build -p pkg
    // refmterr dune build -p pkg
    const is_dune_build = () => (command.slice(0, 2).includes('dune') ||
        command.slice(0, 2).includes('jbuilder')) &&
        command.includes('build');
    return is_dune_build()
        ? command.concat(['-x', node.target, '@install'])
        : command;
};
const _patch_autoconf = (node, command) => {
    // TODO: what if
    // sh configure
    // ../configure
    if (command[0] !== './configure') {
        return command;
    }
    const configure_path = `${node.build_plan.sourcePath}/configure`;
    const configure = fs_1.default.readFileSync(configure_path).toString();
    if (!configure.includes('GNU Autoconf')) {
        return command;
    }
    const args_template = {
        '--build': '$ESY_TOOLCHAIN_BUILD',
        '--host': '$ESY_TOOLCHAIN_HOST',
        CC: '$ESY_TOOLCHAIN_CC',
        CXX: '$ESY_TOOLCHAIN_CXX',
        OCAMLFIND: 'ocamlfind -toolchain $ESY_TOOLCHAIN',
        OCAMLBUILD: 'ocamlbuild -use-ocamlfind -toolchain $ESY_TOOLCHAIN',
        OCAMLMKLIB: '$ESY_TOOLCHAIN_OCAML/bin/ocamlmklib',
        OCAMLMKTOP: '$ESY_TOOLCHAIN_OCAML/bin/ocamlmktop',
        OCAMLDEPDOTOPT: '$ESY_TOOLCHAIN_OCAML/bin/ocamldep',
        OCAMLDEP: '$ESY_TOOLCHAIN_OCAML/bin/ocamldep',
        OCAMLOPTDOTOPT: '$ESY_TOOLCHAIN_OCAML/bin/ocamlopt',
        OCAMLCDOTOPT: '$ESY_TOOLCHAIN_OCAML/bin/ocamlc',
        OCAMLOPT: '$ESY_TOOLCHAIN_OCAML/bin/ocamlc',
        OCAMLLIB: '$ESY_TOOLCHAIN_OCAML/lib/ocaml',
        OCAMLC: '$ESY_TOOLCHAIN_OCAML/bin/ocamlc.opt',
        OCAMLOBJINFO: '$ESY_TOOLCHAIN_OCAML/bin/ocamlobjinfo',
    };
    const additional_args = Object.entries(args_template)
        .filter(([key]) => configure.includes(`${key}=`))
        .map(([key, value]) => `${key}="${value}"`)
        .sort();
    return [...command, ...additional_args];
};
exports.build = (nodes, node) => {
    const source_build = node.build_plan.build || [];
    const commands = unresolve_commands(nodes, node, source_build)
        .map((command) => patch_dune(node, command))
        // .map((command) => patch_autoconf(node, command))
        .filter((command) => !is_installer(command));
    return [
        ...copy_source(nodes, node),
        ...copy_patch_files(nodes, node),
        ...setup_install(nodes, node),
        ...(needs_gen_findlib(nodes, node) ? gen_findlib(nodes, node) : []),
        ...to_script([
            ...clean_env(nodes, node),
            ['cd', node_1.prefix(node)],
            ...commands,
        ]),
    ];
};
exports.install = (nodes, node) => {
    const source_install = node.build_plan.install || [];
    const commands = unresolve_commands(nodes, node, source_install)
        .filter((command) => !is_installer(command))
        .concat([['not-esy-installer', node.target]]);
    return to_script([
        ...clean_env(nodes, node),
        ['cd', node_1.prefix(node)],
        ...commands,
    ]);
};
