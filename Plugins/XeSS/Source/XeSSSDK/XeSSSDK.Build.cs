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

using System;
using System.IO;
using UnrealBuildTool;

public class XeSSSDK : ModuleRules
{
	public XeSSSDK(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		Type = ModuleType.CPlusPlus;

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"RHI",
				"XeSSCommon",
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
			}
		);

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string[] XeSSDLLs = {
				"libxell.dll",
				"libxess.dll",
				"libxess_dx11.dll",
				"libxess_fg.dll",
			};

			string[] XeSSLibs = {
				"libxell.lib",
				"libxess_fg.lib",
			};

			string XeSSIncPath = "$(PluginDir)/Source/XeSSSDK/inc";
			string XeSSLibPath = "$(PluginDir)/Source/XeSSSDK/lib";
			string XeSSBinariesPath = "$(PluginDir)/Binaries/ThirdParty/Win64";

			PublicIncludePaths.Add(XeSSIncPath);

			foreach (string LibFile in XeSSLibs)
			{
				PublicAdditionalLibraries.Add(Path.Combine(XeSSLibPath, LibFile));
			}
			foreach (string DllFile in XeSSDLLs)
			{
				PublicDelayLoadDLLs.Add(DllFile);
				RuntimeDependencies.Add(Path.Combine(XeSSBinariesPath, DllFile), StagedFileType.NonUFS);
			}
		}
	}
}
