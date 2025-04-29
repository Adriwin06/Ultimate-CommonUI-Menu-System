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
using System.IO;
public class NGXD3D12RHI : ModuleRules
{
	public NGXD3D12RHI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
			}
			);

		PrivateIncludePaths.AddRange(
		new string[] {
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
					"Core",
					"Engine",
					"RenderCore",
					"RHI",
					"D3D12RHI",
				
					"NGX",
					"NGXRHI",
			}
			);

		if (ReadOnlyBuildVersion.Current.MajorVersion == 5 && ReadOnlyBuildVersion.Current.MinorVersion >= 1)
		{
			PrivateDefinitions.Add("ENGINE_PROVIDES_ID3D12DYNAMICRHI=1");

			if (ReadOnlyBuildVersion.Current.MajorVersion == 5 && ReadOnlyBuildVersion.Current.MinorVersion >= 5)
			{
				PrivateDefinitions.Add("ENGINE_ID3D12DYNAMICRHI_NEEDS_CMDLIST=1");
			}
			else
			{
				PrivateDefinitions.Add("ENGINE_ID3D12DYNAMICRHI_NEEDS_CMDLIST=0");
			}
		}
		else
		{
			PrivateDefinitions.Add("ENGINE_PROVIDES_ID3D12DYNAMICRHI=0");
			PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/D3D12RHI/Private"));
		}
		// those come from the D3D12RHI
		AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
	}
}
