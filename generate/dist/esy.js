"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const path_1 = __importDefault(require("path"));
const lib_1 = require("./lib");
const run = (name, args) => {
    const cmd = ['esy', !['package', 'esy'].includes(name) && `@${name}`, ...args]
        .filter(Boolean)
        .join(' ');
    return lib_1.exec(cmd).then(({ stdout }) => stdout.toString().trim());
};
exports.make = (manifest_path) => {
    manifest_path = path_1.default.resolve(manifest_path);
    const { dir, name } = path_1.default.parse(manifest_path);
    const lock = require(name === 'package' || name === 'esy'
        ? `${dir}/esy.lock/index.json`
        : `${dir}/${name}.esy.lock/index.json`);
    const manifest = require(manifest_path);
    const build_plan = (pkg) => run(name, ['build-plan', '-p', pkg]).then(JSON.parse);
    const exec_env = (pkg) => run(name, ['exec-env', '-p', pkg, '--json']).then(JSON.parse);
    const get_config = async () => {
        // TODO: what if two of them have the same value?
        // TODO: this order matters
        const variables = ['project', 'store', 'localStore', 'globalStorePrefix'];
        const values = (await run(name, [
            'echo',
            variables.map((variable) => `%{${variable}}%`).join(':'),
        ])).split(':');
        return Object.fromEntries(variables.map((variable, i) => [variable, values[i]]));
    };
    return { name, lock, manifest, build_plan, exec_env, get_config };
};
