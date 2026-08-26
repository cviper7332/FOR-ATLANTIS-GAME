// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectAtlantis : ModuleRules
{
	public ProjectAtlantis(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"RTAC"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ProjectAtlantis",
			"ProjectAtlantis/Variant_Platforming",
			"ProjectAtlantis/Variant_Platforming/Animation",
			"ProjectAtlantis/Variant_Combat",
			"ProjectAtlantis/Variant_Combat/AI",
			"ProjectAtlantis/Variant_Combat/Animation",
			"ProjectAtlantis/Variant_Combat/Gameplay",
			"ProjectAtlantis/Variant_Combat/Interfaces",
			"ProjectAtlantis/Variant_Combat/UI",
			"ProjectAtlantis/Variant_SideScrolling",
			"ProjectAtlantis/Variant_SideScrolling/AI",
			"ProjectAtlantis/Variant_SideScrolling/Gameplay",
			"ProjectAtlantis/Variant_SideScrolling/Interfaces",
			"ProjectAtlantis/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
