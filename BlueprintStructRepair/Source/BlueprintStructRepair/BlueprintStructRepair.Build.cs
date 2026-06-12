using UnrealBuildTool;

public class BlueprintStructRepair : ModuleRules
{
	public BlueprintStructRepair(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"AssetRegistry",
			"Kismet",
			"LevelEditor",
			"Projects",
			"ToolMenus",
			"UnrealEd"
		});
	}
}
