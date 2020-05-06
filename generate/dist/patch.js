"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const path_1 = __importDefault(require("path"));
const fs_1 = __importDefault(require("fs"));
const root_folder = path_1.default.resolve('../patches');
const folders = fs_1.default.readdirSync(root_folder);
const get_patch_folder = (name, version) => {
    if (folders.includes(name)) {
        return path_1.default.resolve(root_folder, name);
    }
    // TODO: read the version from the original manifest
    const name_with_version = `${name}.${version}`;
    if (folders.includes(name_with_version)) {
        return path_1.default.resolve(root_folder, name_with_version);
    }
    return null;
};
const get_patch = (name, version) => {
    const folder = get_patch_folder(name, version);
    if (!folder) {
        return null;
    }
    const files_folder = path_1.default.resolve(folder, 'files');
    const manifest = path_1.default.join(folder, 'package.json');
    const data = fs_1.default.readFileSync(manifest);
    return {
        ...JSON.parse(data.toString()),
        files_folder: fs_1.default.existsSync(files_folder) && files_folder,
    };
};
exports.default = get_patch;
