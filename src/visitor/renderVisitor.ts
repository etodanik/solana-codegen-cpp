import { expandGlobSync } from "std/fs/mod.ts";
import { logError, logInfo, logWarn } from "@kinobi-so/errors";
import { deleteDirectory, writeRenderMapVisitor } from "@kinobi-so/renderers-core";
import { rootNodeVisitor, visit } from "@kinobi-so/visitors-core";

import { GetRenderMapOptions, getRenderMapVisitor } from "./getRenderMapVisitor.ts";
import { CppFlavour } from "./types.ts";

export type RenderOptions = GetRenderMapOptions & {
    deleteFolderBeforeRendering?: boolean;
    formatCode?: boolean;
    cppFlavour: CppFlavour;
    pluginName: string;
};

export function renderVisitor(path: string, options: RenderOptions = {}) {
    return rootNodeVisitor((root) => {
        // Delete existing generated folder.
        if (options.deleteFolderBeforeRendering ?? true) {
            deleteDirectory(path);
        }

        // Render the new files.
        visit(root, writeRenderMapVisitor(getRenderMapVisitor(options), path));
        // console.log('filesList', )
        // format the code
        if (options.formatCode) {
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
    });
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
