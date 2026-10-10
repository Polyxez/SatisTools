using UnrealBuildTool;

public class SatisTools : ModuleRules
{
	public SatisTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Slate",
			"SlateCore",
			"UMG",
			"FactoryGame",
			"SML"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Projects",
			"RenderCore",
			"RHI"
		});
	}
}
