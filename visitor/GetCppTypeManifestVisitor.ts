import {
  AccountDataNode,
  AccountNode,
  ArrayTypeNode,
  BoolTypeNode,
  BytesTypeNode,
  DefinedTypeNode,
  EnumEmptyVariantTypeNode,
  EnumStructVariantTypeNode,
  EnumTupleVariantTypeNode,
  EnumTypeNode,
  InstructionDataArgsNode,
  InstructionExtraArgsNode,
  InstructionNode,
  LinkTypeNode,
  MapTypeNode,
  NumberTypeNode,
  NumberWrapperTypeNode,
  OptionTypeNode,
  SetTypeNode,
  StringTypeNode,
  StructFieldTypeNode,
  StructTypeNode,
  TupleTypeNode,
  visit,
  Visitor,
} from "kinobi";
import { pascalCase } from "../utils.ts";
import { CppFlavour } from "./types.ts";
import { CppIncludeMap } from "./CppIncludeMap.ts";

export type CppTypeManifest = {
  type: string;
  includes: CppIncludeMap;
  nestedStructs: string[];
};

export type GetCppRenderMapOptions = {
  cppFlavour: CppFlavour;
};

export class GetCppTypeManifestVisitor implements Visitor<CppTypeManifest> {
  public parentName: string | null = null;
  public nestedStruct = false;
  private inlineStruct = false;

  constructor(readonly options: GetCppRenderMapOptions) {}

  visitRoot(): CppTypeManifest {
    throw new Error(
      "Cannot get type manifest for root node. Please select a child node.",
    );
  }

  visitProgram(): CppTypeManifest {
    throw new Error(
      "Cannot get type manifest for program node. Please select a child node.",
    );
  }

  visitAccount(account: AccountNode): CppTypeManifest {
    return visit(account.data, this);
  }

  visitAccountData(accountData: AccountDataNode): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitInstruction(instruction: InstructionNode): CppTypeManifest {
    return visit(instruction.dataArgs, this);
  }

  visitInstructionAccount(): CppTypeManifest {
    throw new Error(
      "Cannot get type manifest for instruction account node. Please select a another node.",
    );
  }

  visitInstructionDataArgs(
    instructionDataArgs: InstructionDataArgsNode,
  ): CppTypeManifest {
    this.parentName = pascalCase(instructionDataArgs.name);
    const manifest = visit(instructionDataArgs.struct, this);
    this.parentName = null;
    return manifest;
  }

  visitInstructionExtraArgs(
    instructionExtraArgs: InstructionExtraArgsNode,
  ): CppTypeManifest {
    this.parentName = pascalCase(instructionExtraArgs.name);
    const manifest = visit(instructionExtraArgs.struct, this);
    this.parentName = null;
    return manifest;
  }

  visitDefinedType(definedType: DefinedTypeNode): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitError(): CppTypeManifest {
    throw new Error("Cannot get type manifest for error node.");
  }

  visitArrayType(arrayType: ArrayTypeNode): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitLinkType(linkType: LinkTypeNode): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitEnumType(enumType: EnumTypeNode): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitEnumEmptyVariantType(
    enumEmptyVariantType: EnumEmptyVariantTypeNode,
  ): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitEnumStructVariantType(
    enumStructVariantType: EnumStructVariantTypeNode,
  ): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitEnumTupleVariantType(
    enumTupleVariantType: EnumTupleVariantTypeNode,
  ): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitMapType(mapType: MapTypeNode): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitOptionType(optionType: OptionTypeNode): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitSetType(setType: SetTypeNode): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitStructType(structType: StructTypeNode): CppTypeManifest {
    const { parentName } = this;

    if (!parentName) {
      // TODO: Add to the Cpp validator.
      throw new Error("Struct type must have a parent name.");
    }

    const fields = structType.fields.map((field) => visit(field, this));
    const fieldTypes = fields.map((field) => field.type).join("\n");
    const mergedManifest = this.mergeManifests(fields);

    if (this.nestedStruct) {
      throw new Error("Not Implemented (nestedStruct)");
    }

    if (this.inlineStruct) {
      throw new Error("Not Implemented (inlineStruct)");
    }

    return {
      ...mergedManifest,
      type: `pub struct ${pascalCase(parentName)} {\n${fieldTypes}\n}`,
    };
  }

  visitStructFieldType(
    structFieldType: StructFieldTypeNode,
  ): CppTypeManifest {
    const originalParentName = this.parentName;
    const originalInlineStruct = this.inlineStruct;
    const originalNestedStruct = this.nestedStruct;

    if (!originalParentName) {
      throw new Error("Struct field type must have a parent name.");
    }

    this.parentName = pascalCase(originalParentName) +
      pascalCase(structFieldType.name);
    this.nestedStruct = true;
    this.inlineStruct = false;

    const fieldManifest = visit(structFieldType.child, this);

    this.parentName = originalParentName;
    this.inlineStruct = originalInlineStruct;
    this.nestedStruct = originalNestedStruct;

    const fieldName = pascalCase(structFieldType.name);
    const docblock = this.createDocblock(structFieldType.docs);

    return {
      ...fieldManifest,
      type: this.inlineStruct
        ? `${docblock}${fieldName}: ${fieldManifest.type},`
        : `${docblock}${fieldName}: ${fieldManifest.type},`,
    };
  }

  visitTupleType(tupleType: TupleTypeNode): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitBoolType(boolType: BoolTypeNode): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitBytesType(bytesType: BytesTypeNode): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitNumberType(numberType: NumberTypeNode): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitNumberWrapperType(
    numberWrapperType: NumberWrapperTypeNode,
  ): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitPublicKeyType(): CppTypeManifest {
    throw new Error("Not Implemented.");
  }

  visitStringType(stringType: StringTypeNode): CppTypeManifest {
    if (
      stringType.size.kind === "prefixed" &&
      stringType.size.prefix.format === "u32" &&
      stringType.size.prefix.endian === "le"
    ) {
      switch (this.options.cppFlavour) {
        case CppFlavour.Unreal5:
          return {
            type: "FString",
            includes: new CppIncludeMap().add("Containers/UnrealString.h"),
            nestedStructs: [],
          };
        case CppFlavour.STL20:
        default:
          return {
            type: "std::string",
            includes: new CppIncludeMap().add("string"),
            nestedStructs: [],
          };
      }
    }

    if (stringType.size.kind === "fixed") {
      return {
        type: `char[${stringType.size.value}]`,
        includes: new CppIncludeMap(),
        nestedStructs: [],
      };
    }

    if (stringType.size.kind === "remainder") {
      throw new Error("Not implemented (remainder string type)");
    }

    throw new Error("String size not supported by Borsh");
  }

  protected mergeManifests(
    manifests: CppTypeManifest[],
  ): Pick<CppTypeManifest, "includes" | "nestedStructs"> {
    return {
      includes: new CppIncludeMap().mergeWith(
        ...manifests.map((td) => td.includes),
      ),
      nestedStructs: manifests.flatMap((m) => m.nestedStructs),
    };
  }

  protected createDocblock(docs: string[]): string {
    if (docs.length <= 0) return "";
    if (docs.length === 1) return `\n/** ${docs[0]} */\n`;
    const lines = docs.map((doc) => ` * ${doc}`);
    return `\n/**\n${lines.join("\n")}\n */\n`;
  }

  protected getArrayLikeSizeOption(
    // size: ArrayTypeNode["size"],
    // manifest: Pick<
    //   CppTypeManifest,
    //   "includes"
    // >,
  ): string | null {
    throw new Error("Not Implemented.");
  }
}
