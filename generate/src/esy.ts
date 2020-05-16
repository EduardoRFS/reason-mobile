import path from 'path';
import { exec, map } from './lib';

export type build_plan = {
  version: string;
  build: string[][] | null;
  install: string[][] | null;
  sourcePath: string;
  rootPath: string;
  buildPath: string;
  stagePath: string;
  installPath: string;
  env: {
    cur__target_dir: string;
    cur__install: string;
    cur__root: string;
    OCAMLPATH: string;
    [key: string]: string;
  };
};

// TODO: overrides and consume the original manifest
export type node = {
  id: string;
  name: string;
  overrides: unknown;
  dependencies: string[];
  devDependencies: string[];
};

export type lock = {
  root: string;
  node: { [id: string]: node };
};

type command = string[];
type override = {
  build: command[];
  install: command[];
  dependencies: dependencies;
};

type dependencies = map<string>;
type source = string;
type resolutions = map<
  | source
  | {
      source: source;
      override: override;
    }
>;
export type manifest = {
  dependencies?: dependencies;
  resolutions?: map<unknown>;
  targets?: {
    // TODO: validate that at runtime
    [key: string]: ('arm' | 'arm64' | 'x86' | 'x86_64')[]
  },
  target?: {
    dependencies?: map<string>;
    resolutions?: resolutions;
    overrides?: map<{
      build: string[][];
      install: string[][];
      dependencies: map<string>;
    }>;
  };
};
type config = {
  globalStorePrefix: string;
  project: string;
  store: string;
  localStore: string;
};

export type t = {
  name: string;
  lock: lock;
  manifest: manifest;
  build_plan: (name: string) => Promise<build_plan>;
  exec_env: (name: string) => Promise<map<string>>;
  get_config: () => Promise<config>;
};
const run = (name: string, args: string[]) => {
  const cmd = ['esy', !['package', 'esy'].includes(name) && `@${name}`, ...args]
    .filter(Boolean)
    .join(' ');
  return exec(cmd).then(({ stdout }) => stdout.toString().trim());
};

export const make = (manifest_path: string): t => {
  manifest_path = path.resolve(manifest_path);
  const { dir, name } = path.parse(manifest_path);
  const lock = require(name === 'package' || name === 'esy'
    ? `${dir}/esy.lock/index.json`
    : `${dir}/${name}.esy.lock/index.json`);
  const manifest = require(manifest_path);
  const build_plan = (pkg: string) =>
    run(name, ['build-plan', '-p', pkg]).then(JSON.parse);
  const exec_env = (pkg: string) =>
    run(name, ['exec-env', '-p', pkg, '--json']).then(JSON.parse);
  const get_config = async () => {
    // TODO: what if two of them have the same value?
    // TODO: this order matters
    const variables = ['project', 'store', 'localStore', 'globalStorePrefix'];
    const values = (
      await run(name, [
        'echo',
        variables.map((variable) => `%{${variable}}%`).join(':'),
      ])
    ).split(':');
    return Object.fromEntries(
      variables.map((variable, i) => [variable, values[i]])
    ) as config;
  };
  return { name, lock, manifest, build_plan, exec_env, get_config };
};
