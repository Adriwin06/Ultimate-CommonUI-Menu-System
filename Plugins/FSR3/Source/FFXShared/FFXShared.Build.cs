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
using System;
using System.IO;

public class FFXShared : ModuleRules
{
	public FFXShared(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "../fidelityfx-sdk/sdk/include"),
				Path.Combine(ModuleDirectory, "../fidelityfx-sdk/ffx-api/include/"),
				Path.Combine(ModuleDirectory, "../fidelityfx-sdk/ffx-api/include/ffx_api")
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "../fidelityfx-sdk/sdk/src"),
				Path.Combine(ModuleDirectory, "../fidelityfx-sdk/sdk/src/shared"),
			}
			);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Engine",
				"RHI",
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
			}
			);

		string TestsPath = Path.Combine(EngineDirectory, "Plugins/FFXRenderTest/FFXRenderTest/FFXRenderTest.uplugin");
		if (File.Exists(TestsPath))
		{
			PublicIncludePaths.AddRange(
						new string[] {
							Path.Combine(EngineDirectory, "Plugins/FFXRenderTest/FFXRenderTest/Source/FFXRenderTest/Public"),
						}
					);
			PublicDefinitions.Add("FFX_RENDER_TESTS=1");
		}

		PrecompileForTargets = PrecompileTargetsType.Any;
	}
}
