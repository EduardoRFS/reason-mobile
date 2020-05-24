#! node
"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const esy_1 = require("./esy");
const lib_1 = require("./lib");
const commands_1 = require("./commands");
const fs_1 = __importDefault(require("fs"));
const env_1 = __importDefault(require("./env"));
const patch_1 = __importDefault(require("./patch"));
const esy = fs_1.default.existsSync('./esy.json')
    ? esy_1.make('esy.json')
    : esy_1.make('package.json');
const targets = (() => {
    const manifest_targets = esy.manifest.targets;
    if (!manifest_targets) {
        if (!process.argv[2]) {
            console.error('you should specify a target: generate <system>.<arch>');
            process.exit(1);
        }
        const parts = process.argv[2].split('.');
        const [arch] = parts.slice(-1);
        const system = parts.slice(0, -1).join('.');
        return [{ system, arch, name: `${system}.${arch}` }];
    }
    const systems = Object.keys(manifest_targets);
    return systems.flatMap((system) => manifest_targets[system].map((arch) => ({
        system,
        arch,
        name: `${system}.${arch}`,
    })));
})();
const create_nodes = async () => {
    const nodes = Object.values(esy.lock.node);
    const config = await esy.get_config();
    const dependencies_map = (() => Object.fromEntries(nodes.map((node) => {
        const dependencies = node.dependencies
            .concat(node.devDependencies)
            .map((id) => esy.lock.node[id].name);
        return [node.name, dependencies];
    })))();
    const env_plan_map = await Promise.all(nodes.map(async ({ name }) => {
        const [build_map, exec_env] = await Promise.all([
            esy.build_plan(name),
            esy.exec_env(name),
        ]);
        return [name, { build_map, exec_env }];
    })).then((entries) => Object.fromEntries(entries));
    return nodes.flatMap((node) => ['native', ...targets.map((target) => target.name)].map((target) => {
        const name = lib_1.target_name(target, node.name);
        const native = node.name;
        const env_plan = env_plan_map[node.name];
        const exec_env = (() => {
            const fix_variable = (value) => Object.entries(config).reduce((exec_env, [key, value]) => lib_1.replace_all(value, `%{${key}}%`, exec_env), value);
            return Object.fromEntries(Object.entries(env_plan.exec_env).map(([key, value]) => {
                const new_value = fix_variable(value);
                return [key, new_value];
            }));
        })();
        const override_build_plan = patch_1.default(lib_1.escape_name(node.name), exec_env.cur__version) || {};
        const build_plan = {
            ...env_plan.build_map,
            ...override_build_plan,
        };
        const override_dependencies = override_build_plan.dependencies || {};
        const source_dependencies = Object.fromEntries(dependencies_map[node.name].map((name) => [name, '*']));
        const dependencies = Object.keys({
            ...source_dependencies,
            ...override_dependencies,
        });
        return {
            name,
            native,
            target,
            build_plan,
            exec_env,
            dependencies,
            patch: override_build_plan,
        };
    }));
};
(async () => {
    const nodes = await create_nodes();
    const nodes_map = Object.fromEntries(nodes.map((node) => [node.name, node]));
    const dependencies_map = Object.fromEntries(nodes.map((node) => {
        const dependencies = [
            node.native,
            lib_1.target_name(node.target, 'sysroot'),
            ...node.dependencies.map((name) => lib_1.target_name(node.target, name)),
        ];
        return [node.name, dependencies];
    }));
    const build_map = Object.fromEntries(nodes.map((node) => [node.name, commands_1.build(nodes_map, node)]));
    const install_map = Object.fromEntries(nodes.map((node) => [node.name, commands_1.install(nodes_map, node)]));
    const env_map = Object.fromEntries(nodes.map((node) => [node.name, env_1.default(nodes_map, node)]));
    const versions_map = Object.fromEntries(nodes
        .filter((node) => node.target === 'native')
        .map((node) => [node.name, node.build_plan.version]));
    const nodes_to_patch = nodes.filter((node) => node.target !== 'native');
    const mocks = nodes_to_patch.map((node) => {
        const exportedEnv = env_map[node.name].exported_env;
        if (node.name === lib_1.target_name(node.target, 'ocaml')) {
            exportedEnv.ESY_TOOLCHAIN_OCAML = {
                scope: 'global',
                val: `#{self.install}/${node.target}-sysroot`,
            };
        }
        const raw_dependencies = Object.keys((node.patch && node.patch.raw_dependencies) || {});
        const mock = {
            name: node.name,
            dependencies: Object.fromEntries(dependencies_map[node.name].concat(raw_dependencies).map((name) => {
                const version = (versions_map[name] || '*').split(':');
                return [
                    name,
                    (version.length === 1 ? version : version.slice(1)).join(':'),
                ];
            })),
            esy: {
                buildsInSource: true,
                buildEnv: env_map[node.name].build_env,
                exportedEnv,
                build: build_map[node.name],
                install: install_map[node.name],
            },
        };
        // TODO: checksum patch files
        const checksum = lib_1.sha1(JSON.stringify(mock)).slice(0, 8);
        const path = `.mocks/${checksum}`;
        const file = `${path}/esy.json`;
        return { name: node.name, mock, path, file };
    });
    if (!fs_1.default.existsSync('.mocks')) {
        fs_1.default.mkdirSync('.mocks');
    }
    mocks.forEach(({ path, file, mock }) => {
        if (!fs_1.default.existsSync(path)) {
            fs_1.default.mkdirSync(path);
        }
        fs_1.default.writeFileSync(file, JSON.stringify(mock, null, 2));
    });
    const resolutions_to_patch = mocks.map(({ file, mock }) => [mock.name, file]);
    const source_name = esy.lock.node[esy.lock.root].name;
    targets.forEach((target) => {
        const root_name = lib_1.target_name(target.name, source_name);
        const root = mocks.find((mock) => mock.name == root_name);
        const additional_resolutions = Object.fromEntries([
            ['generate', 'link:../generate/bin/package.json'],
            [`sysroot.tools`, `link:../sysroot/tools/package.json`],
            [
                `@_${target.name}/sysroot`,
                `link:../sysroot/${target.name}/package.json`,
            ],
            target.system === 'android' &&
                [
                    '@_android/ndk',
                    'link:../sysroot/android.ndk/package.json',
                ],
        ].filter(Boolean));
        const host_root_name = esy.lock.node[esy.lock.root].name;
        const wrapper = {
            dependencies: {
                ...esy.manifest.dependencies,
                ...root.mock.dependencies,
                source_name: undefined,
                [host_root_name]: undefined,
                ...(esy.manifest.target && esy.manifest.target.dependencies),
            },
            resolutions: {
                ...Object.fromEntries(resolutions_to_patch),
                ...esy.manifest.resolutions,
                ...additional_resolutions,
                ...(esy.manifest.target && esy.manifest.target.resolutions),
            },
        };
        fs_1.default.writeFileSync(`${target.name}.json`, JSON.stringify(wrapper, null, 2));
    });
})();
/* TODO: IMPORTANT, write a script to check if the files generated are right
  comparing the filesystem to the native one and checking with objdump things */
