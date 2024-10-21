/*
* Copyright (c) 2022 - 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
*
* NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
* property and proprietary rights in and to this material, related
* documentation and any modifications thereto. Any use, reproduction,
* disclosure or distribution of this material and related documentation
* without an express license agreement from NVIDIA CORPORATION or
* its affiliates is strictly prohibited.
*/
using System.IO;
using UnrealBuildTool;

public class StreamlineCore : ModuleRules
{
	public StreamlineCore(ReadOnlyTargetRules Target) : base(Target)
	{
		// For UE 4.2x compat
#if !UE_5_0_OR_LATER
		if (CppStandard < CppStandardVersion.Cpp17)
		{
			CppStandard = CppStandardVersion.Cpp17;
		}
#endif



		// that now gets defined in StreamlineViewExtension.h based on build config and r.Streamline.LogTrackedViews and -sllogviewtracking
		//PrivateDefinitions.Add("DEBUG_STREAMLINE_VIEW_TRACKING=1");




		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				EngineDirectory + "/Source/Runtime/Renderer/Private",
				Path.Combine(ModuleDirectory, "ThirdParty"),
			}
			);
			
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
					"Core",
					"CoreUObject",
					"EngineSettings",
					"Engine",
					"RenderCore",
					"Renderer",
					"RHI",
					"Projects",
					"SlateCore",
					"Slate",


					"Streamline",
					"StreamlineRHI",
					"StreamlineShaders",

					"ApplicationCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		



		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.Add("Settings");
		}

	}
}
