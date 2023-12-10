import { getDirname } from "cross_dirname";
import k from "kinobi";
import { join } from "std/path/mod.ts";
import { RenderCppVisitor } from "./visitor/RenderCppVisitor.ts";
import { CppFlavour } from "./visitor/types.ts";

const clientDir = join(getDirname(), "clients");
const idlDir = join(getDirname(), "idls");

const kinobi = k.createFromIdls([
  join(idlDir, "spl_memo.json"),
]);

const cppDir = join(clientDir, "cpp", "src", "generated");

kinobi.accept(
  new RenderCppVisitor(cppDir, {
    formatCode: true,
    cppFlavour: CppFlavour.STL20,
  }),
);
