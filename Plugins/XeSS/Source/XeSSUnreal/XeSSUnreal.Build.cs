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

/*
* XeSSUnreal module is designed to offer a unified Unreal API across versions, thus to simplify plugin code.
* Design rules:
*   1. Prefix 'X' is used for using/typedef types.
*   2. Header files are organized via Unreal modules.
*   3. Try best to use forward declaration.
*   4. Use a *Includes.h to unify includes.
*/

using UnrealBuildTool;
using System.IO;

public class XeSSUnreal : ModuleRules
{
	public XeSSUnreal(ReadOnlyTargetRules Target) : base(Target)
	{
		int EngineMajorVersion = ReadOnlyBuildVersion.Current.MajorVersion;
		int EngineMinorVersion = ReadOnlyBuildVersion.Current.MinorVersion;

		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// No longer needed since Unreal 5.1
		if (EngineMajorVersion < 5 || EngineMajorVersion == 5 && EngineMinorVersion < 1)
		{
			// For D3D12RHIPrivate.h
			PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/D3D12RHI/Private"));
		}

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"XeSSCommon",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
				"Renderer",
				"RenderCore",
				"RHI",
				"D3D12RHI",
			}
		);
		if (EngineMajorVersion >= 5)
		{
			// Required by D3D12RHI
			PrivateDependencyModuleNames.Add("RHICore");
		}
		// Required by D3D12RHI
		AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
	}
}
