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

#include "StreamlineShaders.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FStreamlineShadersModule"
DEFINE_LOG_CATEGORY_STATIC(LogStreamlineShaders, Log, All);

void FStreamlineShadersModule::StartupModule()
{
	// write the plugin version to the log
	// we use the StreamlineShaders module to write this information because it is the first plugin module loaded on supported platforms
	TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin(TEXT("StreamlineCore"));
	UE_LOG(LogStreamlineShaders, Log, TEXT("Loaded Streamline plugin version %s"), *ThisPlugin->GetDescriptor().VersionName);

	FString PluginShaderDir = FPaths::Combine(ThisPlugin->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/StreamlineCore"), PluginShaderDir);

}

void FStreamlineShadersModule::ShutdownModule()
{
	
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FStreamlineShadersModule, StreamlineShaders)
