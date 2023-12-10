import { CppTypeManifest } from "./GetCppTypeManifestVisitor.ts";
import { Include } from "./types.ts";
import { Set } from "immutable";

const DEFAULT_INCLUDE_SET: Set<Include> = Set([]);

export class CppIncludeMap {
  protected _includes: Set<Include> = Set();

  get includes(): Set<Include> {
    return this._includes;
  }

  resolveIncludes(
    includes: string | string[] | Set<string> | Set<Include>,
  ): Include[] | Set<Include> {
    const normalizedIncludes = typeof includes === "string"
      ? [includes]
      : includes;

    return normalizedIncludes.map((item) => {
      if (typeof item === "object") {
        return item;
      }

      return {
        path: item,
        local: !!item.match(/\/\\\./ig),
      };
    });
  }

  add(includes: string | string[] | Set<string> | Set<Include>): CppIncludeMap {
    const resolvedIncludes = this.resolveIncludes(includes);
    this._includes = this._includes.merge(resolvedIncludes);
    return this;
  }

  remove(includes: string | string[] | Set<string>): CppIncludeMap {
    const includesToRemove = typeof includes === "string"
      ? [includes]
      : includes;
    includesToRemove.forEach((i) => this._includes.delete(i));
    return this;
  }

  mergeWith(...others: CppIncludeMap[]): CppIncludeMap {
    others.forEach((other) => {
      this.add(other._includes);
    });
    return this;
  }

  mergeWithManifest(manifest: CppTypeManifest): CppIncludeMap {
    return this.mergeWith(manifest.includes);
  }

  isEmpty(): boolean {
    return this._includes.size === 0;
  }

  toString(): string {
    const includeStatements = this._includes.map((i) => {
      return i.local ? `#include "${i.path}"` : `#include <${i.path}>`;
    });
    return includeStatements.join("\n");
  }
}
