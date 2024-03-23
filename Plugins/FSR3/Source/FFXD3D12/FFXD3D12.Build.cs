// This file is part of the FidelityFX Super Resolution 3.0 Unreal Engine Plugin.
//
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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
using System;
using System.IO;

public class FFXD3D12 : ModuleRules
{
	public FFXD3D12(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "../fidelityfx-sdk/include"),
			}
			);

		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "../fidelityfx-sdk/src"),
				Path.Combine(ModuleDirectory, "../fidelityfx-sdk/src/shared"),
				Path.Combine(ModuleDirectory, "../fidelityfx-sdk/src/components"),
				Path.Combine(ModuleDirectory, "../fidelityfx-sdk/src/backends/shared"),
				Path.Combine(ModuleDirectory, "../fidelityfx-sdk/shaders/dx12"),
			}
			);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Engine",
				"FFXShared",
				"FFXFrameInterpolationApi"
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",	
			}
			);

		if (Target.Platform.IsInGroup(UnrealPlatformGroup.Microsoft))
		{
			AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
		}

		string BuildWithDLL = System.Environment.GetEnvironmentVariable("FFX_BUILD_AS_DLL");
		bool bBuildWithDLL = (BuildWithDLL != null) && (BuildWithDLL == "1");
		if (bBuildWithDLL && Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
		{
			string AmdApiLibPath = Path.Combine(ModuleDirectory, "../fidelityfx-sdk/bin/ffx_sdk");

			string LibraryName = "ffx_backend_dx12_x64.lib";
			PublicAdditionalLibraries.Add(Path.Combine(AmdApiLibPath, LibraryName));
			RuntimeDependencies.Add("$(TargetOutputDir)/ffx_backend_dx12_x64.dll", Path.Combine(AmdApiLibPath, "ffx_backend_dx12_x64.dll"));
			PublicDelayLoadDLLs.Add("ffx_backend_dx12_x64.dll");
			PrivateDefinitions.Add("FFX_BUILD_AS_DLL=1");
		}

		PrecompileForTargets = PrecompileTargetsType.Any;
	}
}
