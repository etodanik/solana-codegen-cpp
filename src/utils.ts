import nunjucks, { ConfigureOptions } from "nunjucks";

export type PickPartial<T, K extends keyof T> =
  & Omit<T, K>
  & Partial<Pick<T, K>>;

export type PartialExcept<T, K extends keyof T> =
  & Pick<T, K>
  & Partial<Omit<T, K>>;

export function readJson<T extends object>(value: string): T {
  return JSON.parse(Deno.readTextFileSync(value)) as T;
}

export function capitalize(str: string): string {
  if (str.length === 0) return str;
  return str.charAt(0).toUpperCase() + str.slice(1).toLowerCase();
}

export function titleCase(str: string): string {
  return str
    .replace(/([A-Z])/g, " $1")
    .split(/[-_\s+.]/)
    .filter((word) => word.length > 0)
    .map(capitalize)
    .join(" ");
}

export function constantCase(str: string): string {
  return titleCase(str).toUpperCase().split(" ").join("_");
}

export function pascalCase(str: string): string {
  return titleCase(str).split(" ").join("");
}

export function camelCase(str: string): string {
  if (str.length === 0) return str;
  const pascalStr = pascalCase(str);
  return pascalStr.charAt(0).toLowerCase() + pascalStr.slice(1);
}

export function kebabCase(str: string): string {
  return titleCase(str).split(" ").join("-").toLowerCase();
}

export function snakeCase(str: string): string {
  return titleCase(str).split(" ").join("_").toLowerCase();
}

export function mainCase(str: string): string {
  return camelCase(str);
}

export const resolveTemplate = (
  directory: string,
  file: string,
  context?: object,
  options?: ConfigureOptions,
): string => {
  // console.log("context", JSON.stringify(context));
  const env = nunjucks.configure(directory, {
    trimBlocks: true,
    autoescape: false,
    ...options,
  });
  env.addFilter("pascalCase", pascalCase);
  env.addFilter("camelCase", camelCase);
  env.addFilter("snakeCase", snakeCase);
  env.addFilter("kebabCase", kebabCase);
  env.addFilter("titleCase", titleCase);
  env.addFilter("constantCase", constantCase);
  return env.render(file, context);
};

export const deleteFolder = (path: string): void => {
  try {
    Deno.removeSync(path, { recursive: true });
  } catch (error) {
    if (!(error instanceof Deno.errors.NotFound)) {
      throw error;
    }
    // Do nothing...
  }
};
