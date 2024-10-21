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
public class XeSSBlueprint : ModuleRules
{
	public XeSSBlueprint(ReadOnlyTargetRules Target) : base(Target)
	{
		int EngineMajorVersion = ReadOnlyBuildVersion.Current.MajorVersion;
		int EngineMinorVersion = ReadOnlyBuildVersion.Current.MinorVersion;

		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		if (EngineMajorVersion < 5 || EngineMajorVersion == 5 && EngineMinorVersion < 3)
		{
			// For PostProcess/TemporalAA.h
			PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private"));
		}

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Renderer",
				"RenderCore",
				"Projects",
				"RHI",
			}
		);

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicDefinitions.Add("WITH_XESS=1");

			PrivateIncludePaths.AddRange(
				new string[]
				{
					Path.Combine(ModuleDirectory, "../XeSS/Private"),
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"IntelXeSS",
					"XeSSCommon",
					"XeSSCore",
					"XeSSUnreal",
				}
			);
		}
		else
		{
			PublicDefinitions.Add("WITH_XESS=0");
			System.Console.WriteLine("XeSS not supported on this platform");
		}
	}
}
