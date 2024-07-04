using UnrealBuildTool;

public class SolanaProgram : ModuleRules
{
	public SolanaProgram(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.AddRange(new string[] { });
		PrivateIncludePaths.AddRange(new string[] { });

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				// "Json",
				// "JsonRpc",
				"JsonUtilities",
				"Solana",
				"Borsh"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(new string[] { });
	}
}