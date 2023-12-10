import { ValueNode } from "kinobi";
import { CppIncludeMap } from "./CppIncludeMap.ts";

export function renderCppValueNode(value: ValueNode): {
  includes: CppIncludeMap;
  render: string;
} {
  switch (value.kind) {
    case "string":
      return {
        includes: new CppIncludeMap().add("string"),
        render: `std::string(${JSON.stringify(value.value)})`,
      };
    case "number":
    case "boolean":
      return {
        includes: new CppIncludeMap(),
        render: JSON.stringify(value.value),
      };
    default: {
      const neverDefault: never = value;
      throw new Error(`Unexpected value type ${(neverDefault as any).kind}`);
    }
  }
}
