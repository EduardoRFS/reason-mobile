import path from 'path';
import fs from 'fs';
import { folder_sha1 } from './lib';

const root_folder = path.resolve('../patches');
const folders = fs.readdirSync(root_folder);

const get_patch_folder = (name: string, version: string) => {
  if (folders.includes(name)) {
    return path.resolve(root_folder, name);
  }

  // TODO: read the version from the original manifest
  const name_with_version = `${name}.${version}`;
  if (folders.includes(name_with_version)) {
    return path.resolve(root_folder, name_with_version);
  }

  return null;
};
const get_patch = async (name: string, version: string) => {
  const folder = get_patch_folder(name, version);
  if (!folder) {
    return null;
  }

  const files_folder = path.resolve(folder, 'files');
  const manifest = path.join(folder, 'package.json');
  const data = fs.readFileSync(manifest);

  const checksum_files_folder = fs.existsSync(files_folder)
    ? await folder_sha1(files_folder)
    : '';

  return {
    ...JSON.parse(data.toString()),
    checksum_files_folder: checksum_files_folder,
    files_folder: fs.existsSync(files_folder) && files_folder,
  };
};
export default get_patch;
