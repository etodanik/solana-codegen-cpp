import { logWarn } from "@kinobi-so/errors";
import {
    getAllAccounts,
    getAllDefinedTypes,
    getAllInstructionsWithSubs,
    getAllPrograms,
    ImportFrom,
    InstructionNode,
    isNode,
    isNodeFilter,
    pascalCase,
    ProgramNode,
    resolveNestedTypeNode,
    snakeCase,
    structTypeNodeFromInstructionArgumentNodes,
    VALUE_NODES,
} from "@kinobi-so/nodes";
import { RenderMap } from "@kinobi-so/renderers-core";
import { extendVisitor, LinkableDictionary, pipe, recordLinkablesVisitor, staticVisitor, visit } from "@kinobi-so/visitors-core";

import { getTypeManifestVisitor } from "./getTypeManifestVisitor.ts";
import { IncludeMap } from "./IncludeMap.ts";
import { renderValueNode } from "./renderValueNodeVisitor.ts";
import { render } from "./utils/render.ts";

export type GetRenderMapOptions = {
    dependencyMap?: Record<ImportFrom, string>;
    renderParentInstructions?: boolean;
    pluginName: string;
};

export function getRenderMapVisitor(options: GetRenderMapOptions = {}) {
    const linkables = new LinkableDictionary();
    let program: ProgramNode | null = null;

    const renderParentInstructions = options.renderParentInstructions ?? false;
    const dependencyMap = options.dependencyMap ?? {};
    const pluginName = pascalCase(options.pluginName ?? "SolanaProgram");
    const typeManifestVisitor = getTypeManifestVisitor({ pluginName });

    return pipe(
        staticVisitor(
            () => new RenderMap(),
            [
                "rootNode",
                "programNode",
                "instructionNode",
                "accountNode",
                "definedTypeNode",
            ],
        ),
        (v) =>
            extendVisitor(v, {
                visitAccount(node) {
                    const typeManifest = visit(node, typeManifestVisitor);

                    // Seeds.
                    const seedsIncludes = new IncludeMap();
                    const pda = node.pda ? linkables.get(node.pda) : undefined;
                    const pdaSeeds = pda?.seeds ?? [];
                    const seeds = pdaSeeds.map((seed) => {
                        if (isNode(seed, "variablePdaSeedNode")) {
                            const seedManifest = visit(
                                seed.type,
                                typeManifestVisitor,
                            );
                            seedsIncludes.mergeWith(seedManifest.includes);
                            const resolvedType = resolveNestedTypeNode(
                                seed.type,
                            );
                            return {
                                ...seed,
                                resolvedType,
                                typeManifest: seedManifest,
                            };
                        }
                        if (isNode(seed.value, "programIdValueNode")) {
                            return seed;
                        }
                        const seedManifest = visit(
                            seed.type,
                            typeManifestVisitor,
                        );
                        const valueManifest = renderValueNode(seed.value, true);
                        seedsIncludes.mergeWith(valueManifest.includes);
                        const resolvedType = resolveNestedTypeNode(seed.type);
                        return {
                            ...seed,
                            resolvedType,
                            typeManifest: seedManifest,
                            valueManifest,
                        };
                    });
                    const hasVariableSeeds = pdaSeeds.filter(isNodeFilter("variablePdaSeedNode"))
                        .length > 0;
                    const constantSeeds = seeds
                        .filter(isNodeFilter("constantPdaSeedNode"))
                        .filter((seed) => !isNode(seed.value, "programIdValueNode"));

                    const { includes } = typeManifest;

                    if (hasVariableSeeds) {
                        includes.mergeWith(seedsIncludes);
                    }

                    return new RenderMap().add(
                        `Source/${pascalCase(pluginName)}/Public/${pascalCase(pluginName)}/Accounts/${pascalCase(node.name)}.h`,
                        render("accountsH.njk", {
                            account: node,
                            constantSeeds,
                            hasVariableSeeds,
                            includes: includes
                                .remove(
                                    `generatedAccounts::${pascalCase(node.name)}`,
                                )
                                .toString(dependencyMap),
                            pda,
                            program,
                            seeds,
                            typeManifest,
                        }),
                    );
                },

                visitDefinedType(node) {
                    const typeManifest = visit(node, typeManifestVisitor);
                    const includes = new IncludeMap().mergeWithManifest(
                        typeManifest,
                    );

                    return new RenderMap().add(
                        `Source/${pascalCase(pluginName)}/Public/${pascalCase(pluginName)}/Types/${pascalCase(node.name)}.h`,
                        render("definedTypesH.njk", {
                            definedType: node,
                            includes: includes.remove(
                                `generatedTypes::${pascalCase(node.name)}`,
                            ).toString(dependencyMap),
                            typeManifest,
                        }),
                    );
                },

                visitInstruction(node) {
                    // Includes.
                    const includes = new IncludeMap();
                    // includes.add([
                    //     "BorshSerialize.h",
                    // ]);

                    // canMergeAccountsAndArgs
                    const accountsAndArgsConflicts = getConflictsForInstructionAccountsAndArgs(node);
                    if (accountsAndArgsConflicts.length > 0) {
                        logWarn(
                            `[Rust] Accounts and args of instruction [${node.name}] have the following ` +
                                `conflicting attributes [${accountsAndArgsConflicts.join(", ")}]. ` +
                                `Thus, the conflicting arguments will be suffixed with "_arg". ` +
                                "You may want to rename the conflicting attributes.",
                        );
                    }

                    // Instruction args.
                    const instructionArgs: {
                        default: boolean;
                        innerOptionType: string | null;
                        name: string;
                        optional: boolean;
                        type: string;
                        value: string | null;
                    }[] = [];
                    let hasArgs = false;
                    let hasOptional = false;

                    node.arguments.forEach((argument) => {
                        const argumentVisitor = getTypeManifestVisitor({
                            pluginName,
                            nestedStruct: true,
                            parentName: `${pascalCase(node.name)}InstructionData`,
                        });
                        const manifest = visit(argument.type, argumentVisitor);
                        includes.mergeWith(manifest.includes);
                        const innerOptionType = isNode(argument.type, "optionTypeNode") ? manifest.type.slice("Option<".length, -1) : null;

                        const hasDefaultValue = !!argument.defaultValue &&
                            isNode(argument.defaultValue, VALUE_NODES);
                        let renderValue: string | null = null;
                        if (hasDefaultValue) {
                            const { includes: argIncludes, render: value } = renderValueNode(argument.defaultValue);
                            includes.mergeWith(argIncludes);
                            renderValue = value;
                        }

                        hasArgs = hasArgs ||
                            argument.defaultValueStrategy !== "omitted";
                        hasOptional = hasOptional ||
                            (hasDefaultValue &&
                                argument.defaultValueStrategy !== "omitted");

                        const name = accountsAndArgsConflicts.includes(argument.name) ? `${argument.name}_arg` : argument.name;

                        instructionArgs.push({
                            default: hasDefaultValue &&
                                argument.defaultValueStrategy === "omitted",
                            innerOptionType,
                            name,
                            optional: hasDefaultValue &&
                                argument.defaultValueStrategy !== "omitted",
                            type: manifest.type,
                            value: renderValue,
                        });
                    });

                    const struct = structTypeNodeFromInstructionArgumentNodes(
                        node.arguments,
                    );
                    const structVisitor = getTypeManifestVisitor({
                        pluginName,
                        parentName: `${pascalCase(node.name)}InstructionData`,
                    });
                    const typeManifest = visit(struct, structVisitor);
                    // console.log("node", node);
                    return new RenderMap().add(
                        `Source/${pascalCase(pluginName)}/Public/${pascalCase(pluginName)}/Instructions/${pascalCase(node.name)}.h`,
                        render("instructionsH.njk", {
                            hasArgs,
                            hasOptional,
                            includes: includes
                                .add([
                                    "Solana/AccountMeta.h",
                                    "Solana/Instruction.h",
                                    "Solana/PublicKey.h",
                                    "SolanaProgram/Programs.h",
                                    "Borsh/Borsh.h",
                                ])
                                .remove(
                                    `${pascalCase(node.name)}.h`,
                                )
                                .toString(dependencyMap),
                            instruction: node,
                            instructionArgs,
                            program,
                            typeManifest,
                        }),
                    );
                },

                visitProgram(node, { self }) {
                    program = node;
                    const renderMap = new RenderMap()
                        .mergeWith(
                            ...node.accounts.map((account) => visit(account, self)),
                        )
                        .mergeWith(
                            ...node.definedTypes.map((type) => visit(type, self)),
                        )
                        .mergeWith(
                            ...getAllInstructionsWithSubs(node, {
                                leavesOnly: !renderParentInstructions,
                            }).map((ix) => visit(ix, self)),
                        );

                    // Errors.
                    if (node.errors.length > 0) {
                        renderMap.add(
                            `Source/${pascalCase(pluginName)}/Public/${pascalCase(pluginName)}/Errors/${pascalCase(node.name)}.h`,
                            render("errorsH.njk", {
                                errors: node.errors,
                                includes: new IncludeMap().toString(dependencyMap),
                                program: node,
                            }),
                        );

                        renderMap.add(
                            `Source/${pascalCase(pluginName)}/Private/${pascalCase(pluginName)}/Errors/${pascalCase(node.name)}.cpp`,
                            render("errorsCpp.njk", {
                                errors: node.errors,
                                includes: new IncludeMap().add([
                                    `${pascalCase(pluginName)}/Errors/${pascalCase(node.name)}.h`,
                                ]).toString(
                                    dependencyMap,
                                ),
                                program: node,
                            }),
                        );
                    }

                    program = null;
                    return renderMap;
                },

                visitRoot(node, { self }) {
                    const programsToExport = getAllPrograms(node);
                    const accountsToExport = getAllAccounts(node);
                    const instructionsToExport = getAllInstructionsWithSubs(
                        node,
                        {
                            leavesOnly: !renderParentInstructions,
                        },
                    );
                    const definedTypesToExport = getAllDefinedTypes(node);
                    const hasAnythingToExport = programsToExport.length > 0 ||
                        accountsToExport.length > 0 ||
                        instructionsToExport.length > 0 ||
                        definedTypesToExport.length > 0;

                    const ctx = {
                        accountsToExport,
                        definedTypesToExport,
                        hasAnythingToExport,
                        instructionsToExport,
                        programsToExport,
                        root: node,
                        // TODO: Do we ever want to customize the name of the plugin?
                        pluginName,
                    };

                    const map = new RenderMap();
                    if (programsToExport.length > 0) {
                        map.add(
                            `Source/${pascalCase(pluginName)}/Public/${pascalCase(pluginName)}/Programs.h`,
                            render("programsH.njk", {
                                ...ctx,
                                includes: new IncludeMap().add("Solana/PublicKey.h"),
                            }),
                        );
                        // .add(
                        //     "errors/mod.rs",
                        //     render("errorsMod.njk", ctx),
                        // );
                    }
                    // if (accountsToExport.length > 0) {
                    //     map.add(
                    //         "accounts/mod.rs",
                    //         render("accountsMod.njk", ctx),
                    //     );
                    // }
                    // if (instructionsToExport.length > 0) {
                    //     map.add(
                    //         "instructions/mod.rs",
                    //         render("instructionsMod.njk", ctx),
                    //     );
                    // }
                    // if (definedTypesToExport.length > 0) {
                    //     map.add(
                    //         "types/mod.rs",
                    //         render("definedTypesMod.njk", ctx),
                    //     );
                    // }

                    map.add(
                        `${pascalCase(pluginName)}.uplugin`,
                        render("plugin.njk", ctx),
                    );

                    map.add(
                        `Source/${pascalCase(pluginName)}/${pascalCase(pluginName)}.Build.cs`,
                        render("build.njk", ctx),
                    );

                    map.add(
                        `Source/${pascalCase(pluginName)}/Public/${pascalCase(pluginName)}.h`,
                        render("moduleH.njk", ctx),
                    );

                    map.add(
                        `Source/${pascalCase(pluginName)}/Private/${pascalCase(pluginName)}.cpp`,
                        render("moduleCpp.njk", ctx),
                    );

                    return map
                        // .add("mod.rs", render("rootMod.njk", ctx))
                        .mergeWith(
                            ...getAllPrograms(node).map((p) => visit(p, self)),
                        );
                },
            }),
        (v) => recordLinkablesVisitor(v, linkables),
    );
}

function getConflictsForInstructionAccountsAndArgs(
    instruction: InstructionNode,
): string[] {
    const allNames = [
        ...instruction.accounts.map((account) => account.name),
        ...instruction.arguments.map((argument) => argument.name),
    ];
    const duplicates = allNames.filter((e, i, a) => a.indexOf(e) !== i);
    return [...new Set(duplicates)];
}
