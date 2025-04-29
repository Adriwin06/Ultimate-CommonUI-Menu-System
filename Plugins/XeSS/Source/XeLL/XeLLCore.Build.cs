/*******************************************************************************
 * Copyright 2024 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

using System.IO;
using UnrealBuildTool;

/*
 * NOTE: We cannot use 'XeLL' as the module name because `XELL_API` may conflict with the definition in XeLL SDK.
 * However, all other rules will remain the same as for the module name 'XeLL', including module folder name, file name prefixes, etc.
 */
public class XeLLCore : ModuleRules
{
	public XeLLCore(ReadOnlyTargetRules Target) : base(Target)
	{
		int EngineMajorVersion = ReadOnlyBuildVersion.Current.MajorVersion;
		int EngineMinorVersion = ReadOnlyBuildVersion.Current.MinorVersion;

		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"CoreUObject",
				"D3D12RHI",
				"Projects",
				"RenderCore",
				"RHI",

				"XeSSSDK"
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"Engine",

				"XeSSCommon",
				"XeSSUnreal"
			}
		);

		if (EngineMajorVersion < 5 || EngineMajorVersion == 5 && EngineMinorVersion < 1)
		{
			// For D3D12RHIPrivate.h
			PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/D3D12RHI/Private"));
			if (EngineMajorVersion == 5)
			{
				PrivateDependencyModuleNames.Add("RHICore");
			}
		}

		// Required by headers of D3D12RHI
		AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
	}
}
