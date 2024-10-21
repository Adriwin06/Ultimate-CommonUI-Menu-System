/*******************************************************************************
 * Copyright 2021 Intel Corporation
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

/*
 * NOTE: We cannot use 'XeSS' as the module name because `XESS_API` will conflict with the definition in `xess.h`. 
 * However, all other rules will remain the same as for the module name 'XeSS', including module folder name, file name prefixes, etc.
 */
public class XeSSCore : ModuleRules
{
	public XeSSCore(ReadOnlyTargetRules Target) : base(Target)
	{
		int EngineMajorVersion = ReadOnlyBuildVersion.Current.MajorVersion;
		int EngineMinorVersion = ReadOnlyBuildVersion.Current.MinorVersion;

		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(
			new string[]
			{
				 Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private"),
			}
		);

		// No longer needed since Unreal 5.1
		if (EngineMajorVersion < 5 || EngineMajorVersion == 5 && EngineMinorVersion < 1) 
		{
			// For D3D12RHIPrivate.h
			PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/D3D12RHI/Private"));
		}

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"EngineSettings",
				"Engine",
				"Renderer",
				"RenderCore",
				"RHI",
				"D3D12RHI",
				"Projects",
				"DeveloperSettings",

				"IntelXeSS",
				"XeSSCommon",
				"XeSSPrePass",
				"XeSSUnreal",
			}
		);

		if (EngineMajorVersion >= 5)
		{
			PrivateDependencyModuleNames.Add("RHICore");
		}

		// Needed for D3D12RHI
		AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
		AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAPI");
		AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");
		// IntelMetricsDiscovery no longer used since Unreal 5.4
		if (EngineMajorVersion < 5 || EngineMajorVersion == 5 && EngineMinorVersion < 4)
		{
			AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelMetricsDiscovery");
		}
		AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelExtensionsFramework");
	}
}
