import {
  t as node,
  prefix as node_prefix,
  dependencies as node_dependencies,
  children as node_children,
} from './node';
import { map, replace_all, target_name } from './lib';

// TODO: there is a bunch of -lpthread in the wild
// TODO: what I was trying to say? /\
// TODO: HAHAHA

export const ESY_VARS = [
  'SHELL',
  'PATH',
  'MAN_PATH',
  'OCAMLPATH',
  'OCAMLFIND_DESTDIR',
  'CAML_LD_LIBRARY_PATH',
  'ESY__ROOT_PACKAGE_CONFIG_PATH',
];
export const IGNORE_VARS = [
  ...ESY_VARS,
  'PWD',
  'cur__version',
  'cur__name',
  'LD_LIBRARY_PATH',
  'OCAML_SECONDARY_COMPILER_PREFIX'
];
export const UNSET_VARS = ['OCAMLLIB', 'OCAMLPATH'];
// TODO: esy already emits a compatible env because the native package is also added

// TODO: OCAML_TOPLEVEL_PATH is a patch to @toolchain/ocamlfind
// TODO: as we depends on the native package that isn't needed
// export const NATIVE_VARS = [
//   'PATH',
//   'CAML_LD_LIBRARY_PATH',
// ];

const var_is_missing_in_deps = (nodes: map<node>, node: node) => {
  const dependencies = node_dependencies(nodes, node);
  const deps_variables = dependencies.flatMap((dep) =>
    Object.entries(dep.exec_env)
  );

  return (variable: [string, string]) =>
    deps_variables.length === 0 ||
    deps_variables.every(
      (dep_variable) =>
        dep_variable[0] !== variable[0] && dep_variable[1] !== variable[1]
    );
};
export const unresolve_string = (
  nodes: map<node>,
  node: node,
  string: string,
  additional_ignore?: string[]
) => {
  // TODO: seriously this one really needs tests
  const replace = (
    node: node,
    name: string,
    use_cur: boolean,
    value: string
  ) => {
    const {
      sourcePath,
      rootPath,
      buildPath,
      stagePath,
      installPath,
      env,
    } = nodes[node.native].build_plan;
    const prefix = node_prefix(node);
    const to_replace = Object.entries({
      [sourcePath]: `#{${name}.original_root}/${prefix}`,
      [rootPath]: `#{${name}.root}/${prefix}`,
      [buildPath]: `#{${name}.target_dir}/${prefix}`,
      /* if is building and buildsInSource; then
      #{self.install} points to stagePath; else
      #{self.install} points to installPath */
      [stagePath]: `#{${name}.install}/${prefix}`,
      [installPath]: `#{${name}.install}/${prefix}`,
    }).map(([key, value]) => [value, key]);

    const variables = Object.entries(env)
      .filter(
        ([key, value]) =>
          value.length >= 3 &&
          !IGNORE_VARS.includes(key) &&
          !additional_ignore?.includes(key)
      )
      .map(([key, value]) => [`$${key}`, value]);
    const cur_variables = variables.filter(([key]) => key.startsWith('cur__'));
    const non_cur_variables = variables.filter(
      ([key]) => !key.startsWith('cur__')
    );

    return (
      to_replace
        .concat(non_cur_variables)
        // you can only point to cur__ if it's the current package
        .concat(use_cur ? cur_variables : [])
        .reduce(
          (value, [to, pattern]) => replace_all(pattern, to, value),
          value
        )
    );
  };

  string = replace(node, 'self', true, string);

  return node_dependencies(nodes, node).reduce(
    (string, dep) =>
      replace(dep, target_name(node.target, dep.name), false, string),
    string
  );
};
// TODO: can lead to self recursion
const unresolve_env = (nodes: map<node>, node: node, env: [string, string][]) =>
  env
    .filter(([key]) => !IGNORE_VARS.includes(key))
    .map(([key, value]) => {
      const new_value = unresolve_string(nodes, node, value, [key]);
      return new_value !== value ? ([key, new_value] as const) : null;
    })
    .filter(Boolean) as [string, string][];
const to_exported_env = (env: [string, string][]) =>
  Object.fromEntries(env.map(([key, val]) => [key, { scope: 'global', val }]));

const find_node_manifest_env = (nodes: map<node>, node: node) => {
  const node_variables = Object.entries(node.build_plan.env).filter(
    ([key]) => !IGNORE_VARS.includes(key) && !key.startsWith('cur__')
  );

  const exported_env = Object.entries({
    ...node.exec_env,
  }).filter(([key, value]) => node.build_plan.env[key] !== value);

  // TODO: if the variable is defined on some children as buildEnv this will fail
  const build_env = node_variables.filter(var_is_missing_in_deps(nodes, node));

  return { exported_env, build_env };
};
const build_env_ocamlfind = (_nodes: map<node>, node: node) => [
  ['OCAMLFIND_CONF', '#{self.root}/findlib/findlib.conf'],
  ['OCAMLFIND_DESTDIR', `#{self.install}/${node_prefix(node)}/lib`],
  ['TOPKG_CONF_TOOLCHAIN', node.target],
];
const env = (nodes: map<node>, node: node) => {
  const native = nodes[node.native];
  // TODO: it should be checked against the native one
  const manifest = find_node_manifest_env(nodes, native);
  const cur_env = Object.entries(native.build_plan.env).filter(([key]) =>
    key.startsWith('cur__')
  );
  const build_env = Object.fromEntries([
    ...unresolve_env(nodes, node, manifest.build_env),
    ...unresolve_env(nodes, node, cur_env),
    ...build_env_ocamlfind(nodes, node),
    ...Object.entries(node.patch?.build_env || {}),
  ]);

  // TODO: test exported variables like XXX_INCLUDE_PATH XXX_LIB_PATH
  const exported_env = to_exported_env([
    ...unresolve_env(nodes, node, manifest.exported_env),
    ...Object.entries(node.patch?.exported_env || {}),
  ]);
  return { build_env, exported_env };
};
export default env;
