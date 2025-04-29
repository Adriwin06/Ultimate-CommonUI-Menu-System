/*
* Copyright (c) 2020 - 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
*
* NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
* property and proprietary rights in and to this material, related
* documentation and any modifications thereto. Any use, reproduction,
* disclosure or distribution of this material and related documentation
* without an express license agreement from NVIDIA CORPORATION or
* its affiliates is strictly prohibited.
*/

using UnrealBuildTool;

public class DLSSMoviePipelineSupport : ModuleRules
{
	public DLSSMoviePipelineSupport(ReadOnlyTargetRules Target) : base(Target)
	{
		ShortName = "DLSSMPS";

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"DLSS",
				"DLSSBlueprint",
				"Engine",
				"MovieRenderPipelineCore",
				"MovieRenderPipelineRenderPasses",
			}
		);
	}
}
