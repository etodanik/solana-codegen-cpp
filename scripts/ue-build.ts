import * as dotenv from "std/dotenv/mod.ts";
import { copySync } from "std/fs/copy.ts";
import { resolve } from "std/path/mod.ts";

dotenv.loadSync({ export: true });

function getUnrealPlatform(denoPlatform: string) {
  switch (denoPlatform) {
    case "windows":
      return "Win64";
    case "darwin":
      return "Mac";
    default:
      throw new Error("Host platform not supported");
  }
}

function getBuildScript(denoPlatform: string) {
  switch (denoPlatform) {
    case "windows":
      return "Build.bat";
    default:
      return "Build.sh";
  }
}

const unrealEngineRoot = Deno.env.get("UNREAL_ENGINE_ROOT");
if (!unrealEngineRoot) {
  console.log("UNREAL_ENGINE_ROOT not found in environment variables.");
  Deno.exit(1);
}

const dirname = import.meta.dirname || ".";
const buildScript = getBuildScript(Deno.build.os);
const buildCommand = Deno.build.os === "windows"
  ? resolve(unrealEngineRoot, "Engine", "Build", "BatchFiles", buildScript)
  : resolve(unrealEngineRoot, "Engine", "Build", "BatchFiles", getUnrealPlatform(Deno.build.os), buildScript);
const generatedPath = resolve(dirname, "..", "generated", "Solana");
const pluginPath = resolve(dirname, "..", "integration", "SolanaTester", "Plugins", "Solana");
const projectPath = resolve(dirname, "..", "integration", "SolanaTester", "SolanaTester.uproject");

try {
  await Deno.remove(pluginPath, { recursive: true });
} catch (error) {
  if (!(error instanceof Deno.errors.NotFound)) {
    throw error;
  }
}

copySync(generatedPath, pluginPath);

const command = new Deno.Command(
  buildCommand,
  {
    args: [
      "SolanaTester",
      getUnrealPlatform(Deno.build.os),
      "Development",
      projectPath,
    ],
    stdin: "inherit",
    stderr: "inherit",
    stdout: "inherit",
  },
);

const process = command.spawn();

// const { code } = await process.status();
// const rawOutput = await process.output();
// const rawError = await process.stderrOutput();

// if (code === 0) {
//   const output = new TextDecoder().decode(rawOutput);
//   console.log(output);
// } else {
//   const error = new TextDecoder().decode(rawError);
//   console.log("Error:", error);
// }

// Deno.exit(code);
