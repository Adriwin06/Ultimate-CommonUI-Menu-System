// This file is part of the FidelityFX Super Resolution 3.1 Unreal Engine Plugin.
//
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

using UnrealBuildTool;
using System.IO;

public class FFXD3D12Backend : ModuleRules
{
	public FFXD3D12Backend(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
				"Projects",
				"RenderCore",
				"Renderer",
				"RHI",
				"FFXFrameInterpolationApi",
				"FFXD3D12",
				"FFXFSR3Settings"
			}
		);
		
		BuildVersion Version;
		if (BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version))
		{
			if (Version.MajorVersion == 5 && Version.MinorVersion >= 0)
			{
				PrivateDependencyModuleNames.AddRange(
					new string[]
					{
						"RHICore",
						// ... add private dependencies that you statically link with here ...	
					}
					);
			}
		}

		if (Target.Platform.IsInGroup(UnrealPlatformGroup.Microsoft))
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"D3D12RHI",
					"FFXD3D12"
				});

			AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");

			// Required for some private headers needed for the rendering support.
			var EngineDir = Path.GetFullPath(Target.RelativeEnginePath);
			PrivateIncludePaths.AddRange(
				new string[] {
					Path.Combine(EngineDir, @"Source\Runtime\D3D12RHI\Private")
			});

			if (Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
			{
				PrivateIncludePaths.AddRange(
					new string[] {
					Path.Combine(EngineDir, @"Source\Runtime\D3D12RHI\Private\Windows")
				});
			}
			else
			{
				PrivateIncludePaths.AddRange(
					new string[] {
					Path.Combine(EngineDir, @"Source\Runtime\D3D12RHI\Private\" + Target.Platform)
				});
			}

			PrecompileForTargets = PrecompileTargetsType.Any;
		}
		else
		{
			PrecompileForTargets = PrecompileTargetsType.None;
		}
	}
}
