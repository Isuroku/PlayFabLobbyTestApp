// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PlayFabLobbyTestApp : ModuleRules
{
	public PlayFabLobbyTestApp(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "Json",
			"JsonUtilities", "HTTP", "UMG" });
		PrivateDependencyModuleNames.AddRange(new string[] { "PlayFabCpp", "PlayFabCommon", "PlayFab", "SignalR", "WebSockets" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
