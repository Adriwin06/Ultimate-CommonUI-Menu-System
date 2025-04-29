/*******************************************************************************
 * Copyright 2023 Intel Corporation
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

using UnrealBuildTool;
using System.IO;
public class XeFGRHI : ModuleRules
{
	public XeFGRHI(ReadOnlyTargetRules Target) : base(Target)
	{
		int EngineMajorVersion = ReadOnlyBuildVersion.Current.MajorVersion;
		int EngineMinorVersion = ReadOnlyBuildVersion.Current.MinorVersion;

		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",

				"XeSSCommon",
				"XeSSUnreal",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"CoreUObject",
				"Engine",
				"Projects",
				"RHI",
				"D3D12RHI",
				"Renderer",

				"XeLLCore",
				"XeSSSDK",
			}
		);

		// For XeLLDelegates.h
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "../XeLL/Private"));

		if (EngineMajorVersion < 5 || EngineMajorVersion == 5 && EngineMinorVersion < 1)
		{
			// For D3D12RHIPrivate.h
			PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/D3D12RHI/Private"));
			if (EngineMajorVersion == 5)
			{
				PrivateDependencyModuleNames.Add("RHICore");
			}
		}
		if (EngineMajorVersion < 5 || EngineMajorVersion == 5 && EngineMinorVersion < 3)
		{
			// For SystemTextures.h
			PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private"));
		}

		// Required by headers of D3D12RHI
		AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
	}
}
