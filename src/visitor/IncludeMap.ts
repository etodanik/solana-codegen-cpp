import { TypeManifest } from "./getTypeManifestVisitor.ts";
import { Include } from "./types.ts";
import { Set } from "immutable";

const DEFAULT_INCLUDE_SET: Set<Include> = Set([]);

export class IncludeMap {
  protected includes: Set<Include> = Set();

  get includes(): Set<Include> {
    return this.includes;
  }

  resolveIncludes(
    includes: string | string[] | Set<string> | Set<Include>,
  ): Include[] | Set<Include> {
    const normalizedIncludes = typeof includes === "string" ? [includes] : includes;

    return normalizedIncludes.map((item) => {
      if (typeof item === "object") {
        return item;
      }

      return {
        path: item,
        local: !!item.match(/\/\\\./ig) || !!item.match(/\./ig),
      };
    });
  }

  add(includes: string | string[] | Set<string> | Set<Include>): IncludeMap {
    const resolvedIncludes = this.resolveIncludes(includes);
    this.includes = this.includes.merge(resolvedIncludes);
    return this;
  }

  remove(includes: string | string[] | Set<string>): IncludeMap {
    const includesToRemove = typeof includes === "string" ? [includes] : includes;
    includesToRemove.forEach((i) => this.includes.delete(i));
    return this;
  }

  mergeWith(...others: IncludeMap[]): IncludeMap {
    others.forEach((other) => {
      if (other?.includes) {
        this.add(other.includes);
      }
    });
    return this;
  }

  mergeWithManifest(manifest: CppTypeManifest): IncludeMap {
    return this.mergeWith(manifest.includes);
  }

  isEmpty(): boolean {
    return this.includes.size === 0;
  }

  toString(): string {
    const includeStatements = this.includes.map((i) => {
      return i.local ? `#include "${i.path}"` : `#include <${i.path}>`;
    });
    return includeStatements.join("\n");
  }
}
