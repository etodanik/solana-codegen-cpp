import {
  BaseThrowVisitor,
  RootNode,
  visit,
  WriteRenderMapVisitor,
} from "kinobi";
import { deleteFolder } from "../utils.ts";
import { logError, logInfo, logWarn } from "../logs.ts";
import {
  GetCppRenderMapOptions,
  GetCppRenderMapVisitor,
} from "./GetCppRenderMapVisitor.ts";
import { CppFlavour } from "./types.ts";
import { expandGlobSync } from "std/fs/mod.ts";

export type RenderCppOptions = GetCppRenderMapOptions & {
  deleteFolderBeforeRendering?: boolean;
  formatCode?: boolean;
  cppFlavour: CppFlavour;
};

export class RenderCppVisitor extends BaseThrowVisitor<void> {
  constructor(
    readonly path: string,
    readonly options: RenderCppOptions = {
      cppFlavour: CppFlavour.STL20,
    },
  ) {
    super();
  }

  visitRoot(root: RootNode): void {
    if (this.options.deleteFolderBeforeRendering ?? true) {
      deleteFolder(this.path);
    }

    visit(
      root,
      new WriteRenderMapVisitor(
        new GetCppRenderMapVisitor(this.options),
        this.path,
      ),
    );

    if (this.options.formatCode ?? true) {
      const filesList = [
        ...Array.from(expandGlobSync("**/*.h")),
        ...Array.from(expandGlobSync("**/*.cpp")),
      ].filter((item) => item.isFile)
        .map((
          item,
        ) => item.path);

      runFormatter("clang-format", [
        "--verbose",
        "-i",
        ...filesList,
      ]);
    }
  }
}

function runFormatter(cmd: string, args: string[]) {
  const { code, stdout, stderr, success } = new Deno.Command(cmd, { args })
    .outputSync();

  // ENOENT
  if (code == 34) {
    logWarn(`Could not find ${cmd}, skipping formatting.`);
    return;
  }
  if (stdout.length > 0) {
    logInfo(`(clang-format) ${new TextDecoder().decode(stdout)}`);
  }
  if (stderr.length > 0) {
    const logLevel = success ? logInfo : logError;
    logLevel(`(clang-format) ${new TextDecoder().decode(stderr)}`);
  }
}
