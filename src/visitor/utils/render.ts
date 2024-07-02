import { dirname as pathDirname, join } from "node:path";
import { fileURLToPath } from "node:url";

import {
    camelCase,
    kebabCase,
    pascalCase,
    snakeCase,
    titleCase,
} from "@kinobi-so/nodes";
import nunjucks, { ConfigureOptions as NunJucksOptions } from "nunjucks";

export function cppDocblock(docs: string[]): string {
    if (docs.length <= 0) return "";
    const lines = docs.map((doc) => `// ${doc}`);
    return `${lines.join("\n")}\n`;
}

export function constantCase(str: string): string {
    return titleCase(str).toUpperCase().split(" ").join("_");
}

export const render = (
    template: string,
    context?: object,
    options?: NunJucksOptions,
): string => {
    const dirname = pathDirname(fileURLToPath(import.meta.url));
    const templates = join(dirname, "..", "templates");
    const env = nunjucks.configure(templates, {
        autoescape: false,
        trimBlocks: true,
        ...options,
    });
    env.addFilter("pascalCase", pascalCase);
    env.addFilter("camelCase", camelCase);
    env.addFilter("snakeCase", snakeCase);
    env.addFilter("kebabCase", kebabCase);
    env.addFilter("titleCase", titleCase);
    env.addFilter("constantCase", constantCase);
    env.addFilter("rustDocblock", cppDocblock);
    return env.render(template, context);
};
