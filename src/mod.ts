import { getDirname } from "cross_dirname";
import { resolve } from "std/path/mod.ts";
import { join } from "std/path/mod.ts";
import { readJson } from "@kinobi-so/renderers-core";
import { copySync } from "std/fs/copy.ts";
import { visit } from "@kinobi-so/visitors-core";
import { CppFlavour } from "./visitor/types.ts";
import { renderVisitor } from "./visitor/renderVisitor.ts";
import { rootNodeFromAnchor } from "@kinobi-so/nodes-from-anchor";

const rootDir = join(getDirname(), "..");
const clientDir = join(rootDir, "generated");
const idlDir = join(rootDir, "idls");
const path = join(clientDir, "SolanaProgram");
const staticPath = resolve(
    rootDir,
    "src",
    "visitor",
    "templates",
    "static",
    "Resources",
);

const anchorIdl = readJson(join(idlDir, "tiny_adventure.json"));
const node = rootNodeFromAnchor(anchorIdl);

visit(
    node,
    renderVisitor(path, {
        formatCode: false,
        cppFlavour: CppFlavour.Unreal5,
    }),
);

copySync(staticPath, resolve(path, "Resources"));
