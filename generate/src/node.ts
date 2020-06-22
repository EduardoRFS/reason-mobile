import { build_plan } from './esy';
import { map } from './lib';

export type t = {
  name: string;
  native: string;
  target: string;
  build_plan: build_plan;
  exec_env: map<string>;
  dependencies: string[];
  patch?: {
    checksum_files_folder: string;
    files_folder?: string;
    source: string[][];
    build: string[][];
    install: string[][];
    exported_env?: map<string>;
    dependencies: map<string>;
    raw_dependencies?: map<string>;
  };
};

export const prefix = (t: t) => `${t.target}-sysroot`;
export const children = (nodes: map<t>, node: t) =>
  Object.values(nodes).filter((parent) =>
    parent.dependencies.includes(node.name)
  );
export const dependencies = (nodes: map<t>, node: t) =>
  node.dependencies.map((name) => nodes[name]);
