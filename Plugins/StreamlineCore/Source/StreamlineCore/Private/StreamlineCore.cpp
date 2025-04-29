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

#include "StreamlineCore.h"
#include "StreamlineCorePrivate.h"
#include "CoreMinimal.h"

#include "StreamlineSettings.h"
#include "StreamlineViewExtension.h"
#include "StreamlineReflex.h"
#include "StreamlineDLSSG.h"
#include "StreamlineLatewarp.h"
#include "StreamlineDeepDVC.h"

#include "StreamlineRHI.h"
#include "sl_helpers.h"

#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

#include "GeneralProjectSettings.h"
#if WITH_EDITOR
#include "ISettingsModule.h"
#endif
#include "SceneViewExtension.h"
#include "SceneView.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "FStreamlineModule"
DEFINE_LOG_CATEGORY(LogStreamline);


// Epic requested a CVar to control whether the plugin will perform initialization or not.
// This allows the plugin to be included in a project and active but allows for it to not do anything
// at runtime.
static TAutoConsoleVariable<bool> CVarStreamlineInitializePlugin(
	TEXT("r.Streamline.InitializePlugin"),
	true,
	TEXT("Enable/disable initializing the Streamline plugin (default = true)"),
	ECVF_ReadOnly);



Streamline::EStreamlineFeatureSupport TranslateStreamlineResult(sl::Result Result)
{
	switch (Result)
	{

	case sl::Result::eOk:					return Streamline::EStreamlineFeatureSupport::Supported;
	case sl::Result::eErrorOSDisabledHWS:   return Streamline::EStreamlineFeatureSupport::NotSupportedHardwareSchedulingDisabled;
	case sl::Result::eErrorOSOutOfDate: return Streamline::EStreamlineFeatureSupport::NotSupportedOperatingSystemOutOfDate;
	case sl::Result::eErrorDriverOutOfDate: return Streamline::EStreamlineFeatureSupport::NotSupportedDriverOutOfDate;
	case sl::Result::eErrorNoSupportedAdapterFound: return Streamline::EStreamlineFeatureSupport::NotSupportedIncompatibleHardware;
	case sl::Result::eErrorAdapterNotSupported: return Streamline::EStreamlineFeatureSupport::NotSupportedIncompatibleHardware;
	case sl::Result::eErrorMissingOrInvalidAPI: return Streamline::EStreamlineFeatureSupport::NotSupportedIncompatibleRHI;

	default:
		/* Intentionally falls through*/
		return Streamline::EStreamlineFeatureSupport::NotSupported;
	}
}
 
void FStreamlineCoreModule::StartupModule()
{
	if (!CVarStreamlineInitializePlugin.GetValueOnAnyThread())
	{
		UE_LOG(LogStreamline, Log, TEXT("Initialization of StreamlineCore is disabled."));
		return;
	}

	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UE_LOG(LogStreamline, Log, TEXT("%s Enter"), ANSI_TO_TCHAR(__FUNCTION__));

	if (GetPlatformStreamlineSupport() == EStreamlineSupport::Supported)
	{
		// set the view family extension that's gonna call into SL in the postprocessing pass
		bool bShouldCreateViewExtension = IsStreamlineDLSSGSupported() || IsStreamlineLatewarpSupported() || IsStreamlineDeepDVCSupported();
		if (FParse::Param(FCommandLine::Get(), TEXT("slviewextension")))
		{
			bShouldCreateViewExtension = true;
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("slnoviewextension")))
		{
			bShouldCreateViewExtension = false;
		}
		if (bShouldCreateViewExtension)
		{
			StreamlineViewExtension = FSceneViewExtensions::NewExtension<FStreamlineViewExtension>(GetStreamlineRHI());
		}
		else
		{
			StreamlineViewExtension = nullptr;
		}
		
		RegisterStreamlineReflexHooks();

		if (ForceTagStreamlineBuffers() || IsStreamlineDLSSGSupported())
		{
			RegisterStreamlineDLSSGHooks(GetStreamlineRHI());
		}
		if (ForceTagStreamlineBuffers() || IsStreamlineLatewarpSupported())
		{
			RegisterStreamlineLatewarpHooks(GetStreamlineRHI());
		}

		LogStreamlineFeatureSupport(sl::kFeatureImGUI, *GetStreamlineRHI()->GetAdapterInfo());
	}

	UE_LOG(LogStreamline, Log, TEXT("NVIDIA Streamline supported %u"), QueryStreamlineSupport() == EStreamlineSupport::Supported);

#if WITH_EDITOR
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule != nullptr)
	{
		UStreamlineSettings* Settings = GetMutableDefault<UStreamlineSettings>();
		SettingsModule->RegisterSettings("Project", "Plugins", "Streamline",
			LOCTEXT("StreamlineSettingsName", "NVIDIA Streamline"),
			LOCTEXT("StreamlineSettingsDecription", "Configure the NVIDIA Streamline plugins"),
			Settings
		);
		UStreamlineOverrideSettings* OverrideSettings = GetMutableDefault<UStreamlineOverrideSettings>();
		SettingsModule->RegisterSettings("Project", "Plugins", "StreamlineOverride",
			LOCTEXT("StreamlineOverrideSettingsName", "NVIDIA Streamline Overrides (Local)"),
			LOCTEXT("StreamlineOverrideSettingsDescription", "Configure the local settings for the NVIDIA Streamline plugins"),
			OverrideSettings);
	}
#endif

	UE_LOG(LogStreamline, Log, TEXT("%s Leave"), ANSI_TO_TCHAR(__FUNCTION__));
}

void FStreamlineCoreModule::ShutdownModule()
{
	if (!CVarStreamlineInitializePlugin.GetValueOnAnyThread())
	{
		return;
	}

	UE_LOG(LogStreamline, Log, TEXT("%s Enter"), ANSI_TO_TCHAR(__FUNCTION__));

	{
		StreamlineViewExtension = nullptr;
	}

	if (GetPlatformStreamlineSupport() == EStreamlineSupport::Supported)
	{
		if (IsStreamlineLatewarpSupported())
		{
			UnregisterStreamlineLatewarpHooks();
		}

		if (IsStreamlineDLSSGSupported())
		{
			UnregisterStreamlineDLSSGHooks();
		}
		
		UnregisterStreamlineReflexHooks();
	}

#if WITH_EDITOR
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule != nullptr)
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "Streamline");
		SettingsModule->UnregisterSettings("Project", "Plugins", "StreamlineOverride");
	}
#endif

	UE_LOG(LogStreamline, Log, TEXT("%s Leave"), ANSI_TO_TCHAR(__FUNCTION__));
}

EStreamlineSupport FStreamlineCoreModule::QueryStreamlineSupport() const
{
	return GetPlatformStreamlineSupport();
}

Streamline::EStreamlineFeatureSupport FStreamlineCoreModule::QueryDLSSGSupport() const
{
	return QueryStreamlineDLSSGSupport();
}

Streamline::EStreamlineFeatureSupport FStreamlineCoreModule::QueryLatewarpSupport() const
{
	return QueryStreamlineLatewarpSupport();
}

Streamline::EStreamlineFeatureSupport FStreamlineCoreModule::QueryDeepDVCSupport() const
{
	return QueryStreamlineDeepDVCSupport();
}

Streamline::EStreamlineFeatureSupport FStreamlineCoreModule::QueryReflexSupport() const
{
	return QueryStreamlineReflexSupport();
}

FStreamlineRHI* FStreamlineCoreModule::GetStreamlineRHI()
{
	return ::GetPlatformStreamlineRHI();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FStreamlineCoreModule, StreamlineCore)
	

