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

		// Disable Unity build, due to inline function conflicts in modules D3D11RHI and D3D12RHI
		bUseUnity = false;

		if (EngineMajorVersion < 5 || EngineMajorVersion == 5 && EngineMinorVersion < 1)
		{
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				// For D3D11RHIPrivate.h
				PrivateIncludePaths.AddRange(
					new string[] {
						Path.Combine(EngineDirectory, "Source/Runtime/Windows/D3D11RHI/Private"),
						Path.Combine(EngineDirectory, "Source/Runtime/Windows/D3D11RHI/Private/Windows"),
					}
				);
				// Required by D3D11RHIPrivate.h
				AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAPI");
				AddEngineThirdPartyPrivateStaticDependencies(Target, "AMD_AGS");
				AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");
				AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelMetricsDiscovery");
				AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelExtensionsFramework");
			}
			// For D3D12RHIPrivate.h
			PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/D3D12RHI/Private"));
			if (EngineMajorVersion == 5)
			{
				PrivateDependencyModuleNames.Add("RHICore");
			}
		}
		// For SceneTextures.h, PostProcess/SceneRenderTargets.h
		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private"),
			}
		);

		// Required by headers of D3D12RHI
		AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");

		// For VulkanRHIPrivate.h
		PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/VulkanRHI/Private"));
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/VulkanRHI/Private/Windows"));
		}
		// Required by VulkanRHIPrivate.h
		AddEngineThirdPartyPrivateStaticDependencies(Target, "Vulkan");

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"XeSSCommon",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"D3D11RHI",
				"D3D12RHI",
				"Engine",
				"RenderCore",
				"Renderer",
				"RHI",
				"VulkanRHI",
			}
		);
	}
}
