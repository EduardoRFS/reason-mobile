"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const fs_1 = __importDefault(require("fs"));
const crypto_1 = __importDefault(require("crypto"));
const util_1 = require("util");
const child_process_1 = __importDefault(require("child_process"));
const stat = util_1.promisify(fs_1.default.stat);
const read_file = util_1.promisify(fs_1.default.readFile);
exports.sort_uniq = (list) => list
    .slice()
    .sort()
    .reduce((new_list, el) => (el === new_list[0] ? new_list : [el, ...new_list]), []);
exports.exec = util_1.promisify(child_process_1.default.exec);
exports.sha1 = (str) => crypto_1.default.createHash('sha1').update(str).digest('hex');
exports.folder_sha1 = async (path) => {
    const find = await exports.exec(`find ${path}`);
    const hashes = (await Promise.all(find.stdout
        .split('\n')
        .map((str) => str.trim())
        .filter(Boolean)
        .map(async (file) => {
        const stats = await stat(file);
        if (!stats.isFile()) {
            return null;
        }
        const buffer = await read_file(file);
        return exports.sha1(buffer);
    }))).filter(Boolean);
    return exports.sha1(hashes.join(' '));
};
exports.exec(`find . -type f \\( -exec sha1sum "$PWD"/{} \\; \\) | sha1sum`);
exports.replace_all = (pattern, to, str) => {
    const index = str.indexOf(pattern);
    if (index === -1) {
        return str;
    }
    return (str.slice(0, index) +
        to +
        exports.replace_all(pattern, to, str.slice(index + pattern.length)));
};
exports.escape_name = (name) => name
    .replace(/@/g, '_')
    .replace(/\//g, '_')
    .replace(/:/g, '_')
    .replace(/-/g, '_');
exports.target_name = (target, name) => target === 'native'
    ? name
    : // TODO: @_ is needed to have priority on the env variables
        `@_${target}/${exports.escape_name(name)}`;
