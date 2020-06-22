import fs from 'fs';
import crypto from 'crypto';
import { promisify } from 'util';
import child_process from 'child_process';

const stat = promisify(fs.stat);
const read_file = promisify(fs.readFile);

export type map<T> = { [key: string]: T };
export const sort_uniq = <T>(list: T[]) =>
  list
    .slice()
    .sort()
    .reduce(
      (new_list, el) => (el === new_list[0] ? new_list : [el, ...new_list]),
      [] as T[]
    );
export const exec = promisify(child_process.exec);
export const sha1 = (str: string | Buffer) =>
  crypto.createHash('sha1').update(str).digest('hex');
export const folder_sha1 = async (path: string) => {
  const find = await exec(`find ${path}`);
  const hashes = (
    await Promise.all(
      find.stdout
        .split('\n')
        .map((str) => str.trim())
        .filter(Boolean)
        .map(async (file) => {
          const stats = await stat(file);
          if (!stats.isFile()) {
            return null;
          }

          const buffer = await read_file(file);
          return sha1(buffer);
        })
    )
  ).filter(Boolean) as string[];
  return sha1(hashes.join(' '));
};
exec(`find . -type f \\( -exec sha1sum "$PWD"/{} \\; \\) | sha1sum`);
export const replace_all = (
  pattern: string,
  to: string,
  str: string
): string => {
  const index = str.indexOf(pattern);
  if (index === -1) {
    return str;
  }
  return (
    str.slice(0, index) +
    to +
    replace_all(pattern, to, str.slice(index + pattern.length))
  );
};

export const escape_name = (name: string) =>
  name
    .replace(/@/g, '_')
    .replace(/\//g, '_')
    .replace(/:/g, '_')
    .replace(/-/g, '_');
export const target_name = (target: string, name: string) =>
  target === 'native'
    ? name
    : // TODO: @_ is needed to have priority on the env variables
      `@_${target}/${escape_name(name)}`;
