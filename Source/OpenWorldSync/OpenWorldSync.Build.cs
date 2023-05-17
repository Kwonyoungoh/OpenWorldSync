// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OpenWorldSync : ModuleRules
{
	public OpenWorldSync(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "EnhancedInput", "UMG", "HTTP", "Json", "JsonUtilities", "Slate", "SlateCore", "Sockets", "Networking" , "GameplayTasks" });
	}
}
