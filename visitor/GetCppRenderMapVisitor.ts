import { getDirname } from "cross_dirname";
import {
  BaseThrowVisitor,
  getAllAccounts,
  getAllDefinedTypes,
  getAllInstructionsWithSubs,
  GetResolvedInstructionInputsVisitor,
  InstructionNode,
  isOptionTypeNode,
  logWarn,
  ProgramNode,
  RenderMap,
  ResolvedInstructionInput,
  RootNode,
  visit,
  Visitor,
} from "kinobi";
import type { ConfigureOptions } from "nunjucks";
import { join } from "std/path/mod.ts";
import { pascalCase, resolveTemplate } from "../utils.ts";
import {
  CppTypeManifest,
  GetCppTypeManifestVisitor,
} from "./GetCppTypeManifestVisitor.ts";
import { renderCppValueNode } from "./RenderCppValueNode.ts";
import { CppFlavour } from "./types.ts";
import { CppIncludeMap } from "./CppIncludeMap.ts";

export type GetCppRenderMapOptions = {
  renderParentInstructions?: boolean;
  typeManifestVisitor?: Visitor<CppTypeManifest> & {
    parentName: string | null;
    nestedStruct: boolean;
  };
  /* byteSizeVisitor?: Visitor<number | null> & {
    registerDefinedTypes?: (definedTypes: DefinedTypeNode[]) => void;
  }; */
  resolvedInstructionInputVisitor?: Visitor<ResolvedInstructionInput[]>;
  cppFlavour: CppFlavour;
};

export class GetCppRenderMapVisitor extends BaseThrowVisitor<RenderMap> {
  readonly options: Required<GetCppRenderMapOptions>;

  private program: ProgramNode | null = null;

  constructor(
    options: GetCppRenderMapOptions = { cppFlavour: CppFlavour.STL20 },
  ) {
    super();
    this.options = {
      renderParentInstructions: options.renderParentInstructions ?? false,
      resolvedInstructionInputVisitor:
        options.resolvedInstructionInputVisitor ??
          new GetResolvedInstructionInputsVisitor(),
      typeManifestVisitor: options.typeManifestVisitor ??
        new GetCppTypeManifestVisitor(options),
      cppFlavour: options.cppFlavour,
    };
  }

  visitRoot(root: RootNode): RenderMap {
    const programsToExport = root.programs.filter((p) => !p.internal);
    const accountsToExport = getAllAccounts(root)
      .filter((a) => !a.internal);
    const instructionsToExport = getAllInstructionsWithSubs(
      root,
      !this.options.renderParentInstructions,
    )
      .filter((i) => !i.internal);
    const definedTypesToExport = getAllDefinedTypes(root)
      .filter((t) => !t.internal);
    const hasAnythingToExport = programsToExport.length > 0 ||
      accountsToExport.length > 0 ||
      instructionsToExport.length > 0 ||
      definedTypesToExport.length > 0;

    const ctx = {
      root,
      programsToExport,
      accountsToExport,
      instructionsToExport,
      definedTypesToExport,
      hasAnythingToExport,
    };

    const map = new RenderMap();
    // if (programsToExport.length > 0) {
    //   map
    //     .add("programs.rs", this.render("programsMod.njk", ctx))
    //     .add("errors/mod.rs", this.render("errorsMod.njk", ctx));
    // }
    // if (accountsToExport.length > 0) {
    //   map.add("accounts/mod.rs", this.render("accountsMod.njk", ctx));
    // }
    // if (instructionsToExport.length > 0) {
    //   map.add("instructions/mod.rs", this.render("instructionsMod.njk", ctx));
    // }
    // if (definedTypesToExport.length > 0) {
    //   map.add("types/mod.rs", this.render("definedTypesMod.njk", ctx));
    // }

    return map
      .mergeWith(...root.programs.map((program) => visit(program, this)));
  }

  visitProgram(program: ProgramNode): RenderMap {
    this.program = program;
    const { name } = program;
    const pascalCaseName = pascalCase(name);
    const renderMap = new RenderMap();

    // Internal programs are support programs that
    // were added to fill missing types or accounts.
    // They don't need to render anything else.
    if (program.internal) {
      this.program = null;
      return renderMap;
    }

    renderMap
      .mergeWith(
        ...getAllInstructionsWithSubs(
          program,
          !this.options.renderParentInstructions,
        )
          .map((ix) => visit(ix, this)),
      )
      .add(
        `Programs/${pascalCaseName}Program.cpp`,
        this.render("programCpp.njk", {
          program,
        }),
      )
      .add(
        `Programs/${pascalCaseName}Program.h`,
        this.render("programH.njk", {
          program,
        }),
      );
    this.program = null;
    return renderMap;
  }

  visitInstruction(instruction: InstructionNode): RenderMap {
    // Includes.
    const includes = new CppIncludeMap();
    // includes.add(["borsh::BorshDeserialize", "borsh::BorshSerialize"]);

    // canMergeAccountsAndArgs
    const accountsAndArgsConflicts = this
      .getConflictsForInstructionAccountsAndArgs(instruction);
    if (accountsAndArgsConflicts.length > 0) {
      logWarn(
        `[C++] Accounts and args of instruction [${instruction.name}] have the following ` +
          `conflicting attributes [${accountsAndArgsConflicts.join(", ")}]. ` +
          `Thus, the conflicting arguments will be suffixed with "_arg". ` +
          "You may want to rename the conflicting attributes.",
      );
    }

    // Instruction args.
    const instructionArgs: {
      name: string;
      type: string;
      default: boolean;
      optional: boolean;
      innerOptionType: string | null;
      value: string | null;
    }[] = [];
    let hasArgs = false;
    let hasOptional = false;

    instruction.dataArgs.struct.fields.forEach((field) => {
      this.typeManifestVisitor.parentName =
        pascalCase(instruction.dataArgs.name) + pascalCase(field.name);
      this.typeManifestVisitor.nestedStruct = true;
      const manifest = visit(field.child, this.typeManifestVisitor);
      includes.mergeWith(manifest.includes);
      const innerOptionType = isOptionTypeNode(field.child)
        ? manifest.type.slice("Option<".length, -1)
        : null;
      this.typeManifestVisitor.parentName = null;
      this.typeManifestVisitor.nestedStruct = false;

      let renderValue: string | null = null;
      if (field.defaultsTo) {
        const { includes: argIncludes, render: value } = renderCppValueNode(
          field.defaultsTo.value,
        );
        includes.mergeWith(argIncludes);
        renderValue = value;
      }

      hasArgs = hasArgs || field.defaultsTo?.strategy !== "omitted";
      hasOptional = hasOptional || field.defaultsTo?.strategy === "optional";

      const name = accountsAndArgsConflicts.includes(field.name)
        ? `${field.name}_arg`
        : field.name;

      instructionArgs.push({
        name,
        type: manifest.type,
        default: field.defaultsTo?.strategy === "omitted",
        optional: field.defaultsTo?.strategy === "optional",
        innerOptionType,
        value: renderValue,
      });
    });

    const typeManifest = visit(instruction, this.typeManifestVisitor);
    const ctx = {
      instruction,
      instructionArgs,
      hasArgs,
      hasOptional,
      program: this.program,
      typeManifest,
    };

    const renderMap = new RenderMap();
    const instructionHeaderFilename = `${
      pascalCase(instruction.name)
    }Instruction`;

    renderMap.add(
      `Instructions/${instructionHeaderFilename}.h`,
      this.render("instructionsH.njk", {
        ...ctx,
        includes: includes.toString(),
      }),
    );

    includes.add(`${instructionHeaderFilename}.h`);
    renderMap.add(
      `Instructions/${instructionHeaderFilename}.cpp`,
      this.render("instructionsCpp.njk", {
        ...ctx,
        includes: includes.toString(),
      }),
    );

    return renderMap;
  }

  protected getMergeConflictsForInstructionAccountsAndArgs(
    instruction: InstructionNode,
  ): string[] {
    const allNames = [
      ...instruction.accounts.map((account) => account.name),
      ...instruction.dataArgs.struct.fields.map((field) => field.name),
      ...instruction.extraArgs.struct.fields.map((field) => field.name),
    ];
    const duplicates = allNames.filter((e, i, a) => a.indexOf(e) !== i);
    return [...new Set(duplicates)];
  }

  get typeManifestVisitor() {
    return this.options.typeManifestVisitor;
  }

  get resolvedInstructionInputVisitor() {
    return this.options.resolvedInstructionInputVisitor;
  }

  protected getConflictsForInstructionAccountsAndArgs(
    instruction: InstructionNode,
  ): string[] {
    const allNames = [
      ...instruction.accounts.map((account) => account.name),
      ...instruction.dataArgs.struct.fields.map((field) => field.name),
    ];
    const duplicates = allNames.filter((e, i, a) => a.indexOf(e) !== i);
    return [...new Set(duplicates)];
  }

  protected render(
    template: string,
    context?: object,
    options?: ConfigureOptions,
  ): string {
    const code = resolveTemplate(
      join(getDirname(), "templates"),
      template,
      context,
      options,
    );
    return code;
  }
}
