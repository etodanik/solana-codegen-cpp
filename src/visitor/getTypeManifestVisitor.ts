import {
    arrayTypeNode,
    CountNode,
    fixedCountNode,
    isNode,
    isScalarEnum,
    NumberTypeNode,
    numberTypeNode,
    pascalCase,
    prefixedCountNode,
    REGISTERED_TYPE_NODE_KINDS,
    remainderCountNode,
    resolveNestedTypeNode,
    snakeCase,
} from "@kinobi-so/nodes";
import { extendVisitor, mergeVisitor, pipe, visit } from "@kinobi-so/visitors-core";

import { IncludeMap } from "./IncludeMap.ts";
import { cppDocblock } from "./utils/render.ts";
import { numberFormatToCppType } from "./utils/types.ts";

export type TypeManifest = {
    includes: IncludeMap;
    nestedStructs: string[];
    type: string;
};

export function getTypeManifestVisitor(
    options: { nestedStruct?: boolean; parentName?: string | null; pluginName?: string } = {},
) {
    const pluginName: string = options.pluginName ?? "SolanaProgram";
    let parentName: string | null = options.parentName ?? null;
    let nestedStruct: boolean = options.nestedStruct ?? false;
    let inlineStruct: boolean = false;
    let parentSize: NumberTypeNode | number | null = null;

    return pipe(
        mergeVisitor(
            (): TypeManifest => ({
                includes: new IncludeMap(),
                nestedStructs: [],
                type: "",
            }),
            (_, values) => ({
                ...mergeManifests(values),
                type: values.map((v) => v.type).join("\n"),
            }),
            [
                ...REGISTERED_TYPE_NODE_KINDS,
                "definedTypeLinkNode",
                "definedTypeNode",
                "accountNode",
            ],
        ),
        (v) =>
            extendVisitor(v, {
                visitAccount(account, { self }) {
                    parentName = pascalCase(account.name);
                    const manifest = visit(account.data, self);
                    // manifest.includes.add([
                    //     "BorshSerialize.h",
                    // ]);
                    parentName = null;
                    return {
                        ...manifest,
                        type:
                            //"#[derive(BorshSerialize, BorshDeserialize, Clone, Debug, Eq, PartialEq)]\n" +
                            //'#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]\n' +
                            `${manifest.type}`,
                    };
                },

                visitArrayType(arrayType, { self }) {
                    const childManifest = visit(arrayType.item, self);

                    if (isNode(arrayType.count, "fixedCountNode")) {
                        return {
                            ...childManifest,
                            type: `${childManifest.type}`,
                            typeSuffix: `[${arrayType.count.value}]`
                        };
                    }

                    if (isNode(arrayType.count, "remainderCountNode")) {
                        childManifest.includes.add(
                            "Containers/Array.h",
                        );
                        return {
                            ...childManifest,
                            type: `TArray<${childManifest.type}>`,
                        };
                    }

                    const prefix = resolveNestedTypeNode(
                        arrayType.count.prefix,
                    );
                    if (prefix.endian === "le") {
                        switch (prefix.format) {
                            case "u32":
                                return {
                                    ...childManifest,
                                    type: `TArray<${childManifest.type}>`,
                                };
                            case "u8":
                            case "u16":
                            case "u64": {
                                const prefixFormat = prefix.format
                                    .toUpperCase();
                                childManifest.includes.add(
                                    "Containers/Array.h",
                                );
                                return {
                                    ...childManifest,
                                    type: `TArray<${childManifest.type}>`,
                                };
                            }
                            default:
                                throw new Error(
                                    `Array prefix not supported: ${prefix.format}`,
                                );
                        }
                    }

                    // TODO: Add to the Rust validator.
                    throw new Error("Array size not supported by Borsh");
                },

                visitBooleanType(booleanType) {
                    const resolvedSize = resolveNestedTypeNode(
                        booleanType.size,
                    );
                    if (
                        resolvedSize.format === "u8" &&
                        resolvedSize.endian === "le"
                    ) {
                        return {
                            includes: new IncludeMap(),
                            nestedStructs: [],
                            type: "bool",
                        };
                    }

                    // TODO: Add to the Rust validator.
                    throw new Error("Bool size not supported by Borsh");
                },

                visitBytesType(_bytesType, { self }) {
                    let arraySize: CountNode = remainderCountNode();
                    if (typeof parentSize === "number") {
                        arraySize = fixedCountNode(parentSize);
                    } else if (parentSize && typeof parentSize === "object") {
                        arraySize = prefixedCountNode(parentSize);
                    }
                    const arrayType = arrayTypeNode(
                        numberTypeNode("u8"),
                        arraySize,
                    );
                    return visit(arrayType, self);
                },

                visitDefinedType(definedType, { self }) {
                    parentName = pascalCase(definedType.name);
                    const manifest = visit(definedType.type, self);

                    manifest.includes.add([
                        `${parentName}.generated.h`,
                    ]);

                    parentName = null;

                    if (
                        isNode(definedType.type, "enumTypeNode") &&
                        isScalarEnum(definedType.type)
                    ) {
                        // manifest.includes.add(["num_derive::FromPrimitive"]);
                    }
                    return {
                        ...manifest,
                        nestedStructs: manifest.nestedStructs.map(
                            (struct) =>
                                // `#[derive(${traits.join(", ")})]\n` +
                                // '#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]\n' +
                                `${struct}`,
                        ),
                        type:
                            // `#[derive(${traits.join(", ")})]\n` +
                            // '#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]\n' +
                            `${manifest.type}`,
                    };
                },

                visitDefinedTypeLink(node) {
                    const pascalCaseDefinedType = pascalCase(node.name);
                    return {
                        includes: new IncludeMap().add(
                            `${pluginName}/Types/${pascalCaseDefinedType}.h`,
                        ),
                        nestedStructs: [],
                        type: `F${pascalCaseDefinedType}`,
                    };
                },

                visitEnumEmptyVariantType(enumEmptyVariantType) {
                    const name = pascalCase(enumEmptyVariantType.name);
                    return {
                        includes: new IncludeMap(),
                        nestedStructs: [],
                        type: `${name},`,
                    };
                },

                visitEnumStructVariantType(enumStructVariantType, { self }) {
                    const name = pascalCase(enumStructVariantType.name);
                    const originalParentName = parentName;

                    if (!originalParentName) {
                        throw new Error(
                            "Enum struct variant type must have a parent name.",
                        );
                    }

                    inlineStruct = true;
                    parentName = pascalCase(originalParentName) + name;
                    const typeManifest = visit(
                        enumStructVariantType.struct,
                        self,
                    );
                    inlineStruct = false;
                    parentName = originalParentName;

                    return {
                        ...typeManifest,
                        type: `${name} ${typeManifest.type},`,
                    };
                },

                visitEnumTupleVariantType(enumTupleVariantType, { self }) {
                    const name = pascalCase(enumTupleVariantType.name);
                    const originalParentName = parentName;

                    if (!originalParentName) {
                        throw new Error(
                            "Enum struct variant type must have a parent name.",
                        );
                    }

                    parentName = pascalCase(originalParentName) + name;
                    const childManifest = visit(
                        enumTupleVariantType.tuple,
                        self,
                    );
                    parentName = originalParentName;

                    return {
                        ...childManifest,
                        type: `${name}${childManifest.type},`,
                    };
                },

                visitEnumType(enumType, { self }) {
                    throw new Error("enum not implemented");
                    // const originalParentName = parentName;
                    // if (!originalParentName) {
                    //     // TODO: Add to the Rust validator.
                    //     throw new Error("Enum type must have a parent name.");
                    // }

                    // const variants = enumType.variants.map((variant) => visit(variant, self));
                    // const variantNames = variants.map((variant) => variant.type)
                    //     .join("\n");
                    // const mergedManifest = mergeManifests(variants);

                    // return {
                    //     ...mergedManifest,
                    //     type: `pub enum ${pascalCase(originalParentName)} {\n${variantNames}\n}`,
                    // };
                },

                visitFixedSizeType(fixedSizeType, { self }) {
                    parentSize = fixedSizeType.size;
                    const manifest = visit(fixedSizeType.type, self);
                    parentSize = null;
                    return manifest;
                },

                visitMapType(mapType, { self }) {
                    throw new Error("Map not implemented");
                    // const key = visit(mapType.key, self);
                    // const value = visit(mapType.value, self);
                    // const mergedManifest = mergeManifests([key, value]);
                    // mergedManifest.includes.add("std::collections::HashMap");
                    // return {
                    //     ...mergedManifest,
                    //     type: `HashMap<${key.type}, ${value.type}>`,
                    // };
                },

                visitNumberType(numberType) {
                    if (numberType.endian === "le") {
                        return {
                            includes: new IncludeMap(),
                            nestedStructs: [],
                            type: numberFormatToCppType(numberType.format),
                        };
                    }

                    // TODO: Add to the Rust validator.
                    throw new Error("Number endianness not supported by Borsh");
                },

                visitOptionType(optionType, { self }) {
                    const childManifest = visit(optionType.item, self);

                    const optionPrefix = resolveNestedTypeNode(
                        optionType.prefix,
                    );
                    if (
                        optionPrefix.format === "u8" &&
                        optionPrefix.endian === "le"
                    ) {
                        return {
                            ...childManifest,
                            type: `TOptional<${childManifest.type}>`,
                        };
                    }

                    // TODO: Add to the Rust validator.
                    throw new Error("Option size not supported by Borsh");
                },

                visitPublicKeyType() {
                    return {
                        includes: new IncludeMap().add(
                            "Solana/PublicKey.h",
                        ),
                        nestedStructs: [],
                        type: "FPublicKey",
                    };
                },

                visitSetType(setType, { self }) {
                    throw new Error("Set is not implemented");
                    // const childManifest = visit(setType.item, self);
                    // childManifest.includes.add("std::collections::HashSet");
                    // return {
                    //     ...childManifest,
                    //     type: `HashSet<${childManifest.type}>`,
                    // };
                },

                visitSizePrefixType(sizePrefixType, { self }) {
                    parentSize = resolveNestedTypeNode(sizePrefixType.prefix);
                    const manifest = visit(sizePrefixType.type, self);
                    parentSize = null;
                    return manifest;
                },

                visitStringType(aaa) {
                    if (!parentSize) {
                        throw new Error("Remainder string not implemented");
                        // return {
                        //     includes: new IncludeMap().add(
                        //         `kaigan::types::RemainderStr`,
                        //     ),
                        //     nestedStructs: [],
                        //     type: `RemainderStr`,
                        // };
                    }

                    if (typeof parentSize === "number") {
                        throw new Error("Fixed string not implemented");
                        // return {
                        //     includes: new IncludeMap(),
                        //     nestedStructs: [],
                        //     type: `[u8; ${parentSize}]`,
                        // };
                    }

                    if (
                        isNode(parentSize, "numberTypeNode") &&
                        parentSize.endian === "le"
                    ) {
                        switch (parentSize.format) {
                            case "u32":
                                return {
                                    includes: new IncludeMap().add(
                                        "CoreMinimal.h",
                                    ),
                                    nestedStructs: [],
                                    type: "FString",
                                };
                            case "u8":
                            case "u16":
                            case "u64": {
                                throw new Error(
                                    "Prefix string not implemented",
                                );
                                // const prefix = parentSize.format.toUpperCase();
                                // return {
                                //     includes: new IncludeMap().add(
                                //         `kaigan::types::${prefix}PrefixString`,
                                //     ),
                                //     nestedStructs: [],
                                //     type: `${prefix}PrefixString`,
                                // };
                            }
                            default:
                                throw new Error(
                                    `'String size not supported: ${parentSize.format}`,
                                );
                        }
                    }

                    // TODO: Add to the Rust validator.
                    throw new Error("String size not supported by Borsh");
                },

                visitStructFieldType(structFieldType, { self }) {
                    const originalParentName = parentName;
                    const originalInlineStruct = inlineStruct;
                    const originalNestedStruct = nestedStruct;

                    if (!originalParentName) {
                        throw new Error(
                            "Struct field type must have a parent name.",
                        );
                    }

                    parentName = pascalCase(originalParentName) +
                        pascalCase(structFieldType.name);
                    nestedStruct = true;
                    inlineStruct = false;

                    const fieldManifest = visit(structFieldType.type, self);

                    parentName = originalParentName;
                    inlineStruct = originalInlineStruct;
                    nestedStruct = originalNestedStruct;

                    const fieldName = pascalCase(structFieldType.name);
                    const docblock = cppDocblock(structFieldType.docs);
                    const macro = "";
                    const resolvedNestedType = resolveNestedTypeNode(
                        structFieldType.type,
                    );

                    if (inlineStruct) {
                        throw new Error("inlineStruct not implemented");
                    } 
                    
                    return {
                        ...fieldManifest,
                        type: inlineStruct
                            ? `${docblock}${macro}${fieldManifest.type} ${fieldName}${fieldManifest.typeSuffix};`
                            : `${docblock}${macro}${fieldManifest.type} ${fieldName}${fieldManifest.typeSuffix};`,
                    };
                },

                visitStructType(structType, { self }) {
                    const originalParentName = parentName;

                    if (!originalParentName) {
                        // TODO: Add to the Rust validator.
                        throw new Error("Struct type must have a parent name.");
                    }

                    const fields = structType.fields.map((field) => visit(field, self));
                    const fieldTypes = fields.map((field) => field.type).join(
                        "\n",
                    );
                    const mergedManifest = mergeManifests(fields);

                    if (nestedStruct) {
                        return {
                            ...mergedManifest,
                            nestedStructs: [
                                ...mergedManifest.nestedStructs,
                                `\nstruct F${pascalCase(originalParentName)} {\n${fieldTypes}\n};`,
                            ],
                            type: pascalCase(originalParentName),
                        };
                    }

                    if (inlineStruct) {
                        return {
                            ...mergedManifest,
                            type: `{\n${fieldTypes}\n}`,
                        };
                    }

                    return {
                        ...mergedManifest,
                        type: `\nstruct F${pascalCase(originalParentName)} {\n${fieldTypes}\n};`,
                    };
                },

                visitTupleType(tupleType, { self }) {
                    const items = tupleType.items.map((item) => visit(item, self));
                    const mergedManifest = mergeManifests(items);

                    return {
                        ...mergedManifest,
                        type: `(${items.map((item) => item.type).join(", ")})`,
                    };
                },
            }),
    );
}

function mergeManifests(
    manifests: TypeManifest[],
): Pick<TypeManifest, "includes" | "nestedStructs"> {
    return {
        includes: new IncludeMap().mergeWith(
            ...manifests.map((td) => td.includes),
        ),
        nestedStructs: manifests.flatMap((m) => m.nestedStructs),
    };
}
