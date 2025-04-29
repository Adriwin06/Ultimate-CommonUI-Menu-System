/*
* Copyright (c) 2022 - 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
*
* NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
* property and proprietary rights in and to this material, related
* documentation and any modifications thereto. Any use, reproduction,
* disclosure or distribution of this material and related documentation
* without an express license agreement from NVIDIA CORPORATION or
* its affiliates is strictly prohibited.
*/

using UnrealBuildTool;
using System.IO;

public class StreamlineReflexBlueprint : ModuleRules
{
	// TODO that probably should come from the StreamlineCore plugin
	protected virtual bool IsSupportedPlatform(ReadOnlyTargetRules Target)
	{
		return Target.Platform.IsInGroup(UnrealPlatformGroup.Windows);
	}

	public StreamlineReflexBlueprint(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"RenderCore",
				"Renderer",
				"Projects",
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
			}
		);


		// TODO that probably should come from the StreamlineCore plugin
		bool bPlatformSupportsStreamline = IsSupportedPlatform(Target);

		PublicDefinitions.Add("WITH_STREAMLINE=" + (bPlatformSupportsStreamline ? '1' : '0'));

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"StreamlineBlueprint"
			}
		);

		if (bPlatformSupportsStreamline)
		{
			PublicIncludePaths.AddRange(
				new string[]
				{
				}
			);

			PrivateIncludePaths.AddRange(
				new string[]
				{
				}
			);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"StreamlineCore",
					"StreamlineRHI",
					"Streamline",
				}
			);
		}
	}


}
