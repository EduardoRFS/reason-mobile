"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.prefix = (t) => `${t.target}-sysroot`;
exports.children = (nodes, node) => Object.values(nodes).filter((parent) => parent.dependencies.includes(node.name));
exports.dependencies = (nodes, node) => node.dependencies.map((name) => nodes[name]);
