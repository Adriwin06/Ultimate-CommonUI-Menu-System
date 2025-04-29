/*
* Copyright (c) 2022 - 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
*
* NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
* property and proprietary rights in and to this material, related
* documentation and any modifications thereto. Any use, reproduction,
* disclosure or distribution of this material and related documentation
* without an express license agreement from NVIDIA CORPORATION or
* its affiliates is strictly prohibited.
*/
#if UE_5_0_OR_LATER
using EpicGames.Core;
#else
using Tools.DotNETCommon;
#endif

using System.Collections.Generic;
using UnrealBuildTool;
using System.IO;

public class Streamline : ModuleRules
{

	protected virtual bool IsSupportedWindowsPlatform(ReadOnlyTargetRules Target)
	{
		return Target.Platform.IsInGroup(UnrealPlatformGroup.Windows);
	}

	public Streamline (ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		if (IsSupportedWindowsPlatform(Target))
		{
			string StreamlinePath = ModuleDirectory + "/";
			string StreamlineIncludePath = StreamlinePath + "include/";
			PublicIncludePaths.Add(StreamlineIncludePath);

			string SLProductionBinariesPath = PluginDirectory + @"\Binaries\ThirdParty\Win64\";
			string SLDevelopmentBinariesPath = SLProductionBinariesPath + @"Development\";
			string SLDebugBinariesPath = SLProductionBinariesPath + @"Debug\";

			// those are loaded at runtime by the SL & NGX plugin loader which accepts our path
			List<string> StreamlineDlls = new List<string>
			{
				 "sl.interposer.dll",
				 "sl.common.dll",

				 "sl.reflex.dll",
				 "sl.pcl.dll",

				 "nvngx_dlssg.dll",
				 "sl.dlss_g.dll",

				 "nvngx_deepdvc.dll",
				 "sl.deepdvc.dll",
			 };

			List<string> StreamlinePdbs = new List<string>(StreamlineDlls);
			StreamlinePdbs.ForEach(DLLFile =>  Path.ChangeExtension(DLLFile, ".pdb"));

			PublicDefinitions.Add("STREAMLINE_INTERPOSER_BINARY_NAME=TEXT(\"" + StreamlineDlls[0] + "\")");
			PublicDefinitions.Add("SL_BUILD_DEEPDVC=1");

			// 			bool bWithLatewarp = File.Exists(StreamlineIncludePath+"/sl_latewarp.h") && 
			// 								 File.Exists(SLProductionBinariesPath+ "/sl.latewarp.dll") &&
			// 								 File.Exists(SLProductionBinariesPath + "/nvngx_latewarp.dll");

			bool bWithLatewarp = false;

			if (bWithLatewarp)
			{
				
				StreamlineDlls.AddRange(new string[]
				{
					"sl.latewarp.dll",
					"nvngx_latewarp.dll",
				});

				PublicDefinitions.Add("WITH_LATEWARP=1");

			}
			else
			{
				PublicDefinitions.Add("WITH_LATEWARP=0");
			}

			bool bHasProductionBinaries = Directory.Exists(SLProductionBinariesPath);
			bool bHasDevelopmentBinaries = Directory.Exists(SLDevelopmentBinariesPath);
			bool bHasDebugBinaries = Directory.Exists(SLDebugBinariesPath);

			foreach (string StreamlineDll in StreamlineDlls)
			{
				RuntimeDependencies.Add(SLProductionBinariesPath + StreamlineDll, StagedFileType.NonUFS);

				if (Target.Configuration != UnrealTargetConfiguration.Shipping)
				{
					if (bHasDevelopmentBinaries)
					{
						RuntimeDependencies.Add(SLDevelopmentBinariesPath + StreamlineDll, StagedFileType.NonUFS);
					}

					if (bHasDebugBinaries)
					{
						RuntimeDependencies.Add(SLDebugBinariesPath + StreamlineDll, StagedFileType.NonUFS);
					}
				}
			}

			if (Target.Configuration != UnrealTargetConfiguration.Shipping)
			{
				// include symbols in non-shipping builds
				foreach (string StreamlinePdb in StreamlinePdbs)
				{
					RuntimeDependencies.Add(SLProductionBinariesPath + StreamlinePdb, StagedFileType.DebugNonUFS);

					if (bHasDevelopmentBinaries)
					{
						RuntimeDependencies.Add(SLDevelopmentBinariesPath + StreamlinePdb, StagedFileType.DebugNonUFS);
					}

					if (bHasDebugBinaries)
					{
						RuntimeDependencies.Add(SLDebugBinariesPath + StreamlinePdb, StagedFileType.DebugNonUFS);
					}
				}

				// useful to have debug overlay during testing, but we don't want to ship with that
				List<string> StreamlineOverlayBinaries = new List<string>
				{
					"sl.imgui.dll",
					"sl.imgui.pdb",
				};

				foreach (string StreamlineOverlayBinary in StreamlineOverlayBinaries)
				{
					StagedFileType FileType = StreamlineOverlayBinary.EndsWith("pdb") ? StagedFileType.DebugNonUFS : StagedFileType.NonUFS;
					if (bHasDevelopmentBinaries)
					{
						RuntimeDependencies.Add(SLDevelopmentBinariesPath + StreamlineOverlayBinary, FileType);
					}
					if (bHasDebugBinaries)
					{
						RuntimeDependencies.Add(SLDebugBinariesPath + StreamlineOverlayBinary, FileType);
					}
				}
			}
		}
	}
}

