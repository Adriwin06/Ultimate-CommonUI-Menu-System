// This file is part of the FidelityFX Super Resolution 3.1 Unreal Engine Plugin.
//
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "FFXFSR3TemporalUpscaling.h"
#include "FFXFSR3TemporalUpscaler.h"
#include "FFXFSR3ViewExtension.h"
#include "LogFFXFSR3.h"

#include "CoreMinimal.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/ConfigCacheIni.h"
#if UE_VERSION_AT_LEAST(5, 1, 0)
#include "Misc/ConfigUtilities.h"
#endif

#if UE_VERSION_AT_LEAST(5, 2, 0)
#include "DataDrivenShaderPlatformInfo.h"
#else
#include "RHIDefinitions.h"
#endif

IMPLEMENT_MODULE(FFXFSR3TemporalUpscalingModule, FFXFSR3TemporalUpscaling)

#define LOCTEXT_NAMESPACE "FSR3"

DEFINE_LOG_CATEGORY(LogFSR3);

static bool GFFXFSR3TemporalUpscalingModuleInit = false;

void FFXFSR3TemporalUpscalingModule::StartupModule()
{
	FString PluginFSR3ShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("FSR3"))->GetBaseDir(), TEXT("Source/fidelityfx-sdk/sdk/include/FidelityFX/gpu"));
	AddShaderSourceDirectoryMapping(TEXT("/ThirdParty/FSR3"), PluginFSR3ShaderDir);
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("FSR3"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/FSR3"), PluginShaderDir);
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FFXFSR3TemporalUpscalingModule::OnPostEngineInit);
	GFFXFSR3TemporalUpscalingModuleInit = true;
	UE_LOG(LogFSR3, Log, TEXT("FSR3 Temporal Upscaling Module Started"));
}

void FFXFSR3TemporalUpscalingModule::ShutdownModule()
{
	GFFXFSR3TemporalUpscalingModuleInit = false;
	UE_LOG(LogFSR3, Log, TEXT("FSR3 Temporal Upscaling Module Shutdown"));
}

bool FFXFSR3TemporalUpscalingModule::IsInitialized()
{
	return GFFXFSR3TemporalUpscalingModuleInit;
}

void FFXFSR3TemporalUpscalingModule::SetTemporalUpscaler(TSharedPtr<FFXFSR3TemporalUpscaler, ESPMode::ThreadSafe> Upscaler)
{
	TemporalUpscaler = Upscaler;
}

void FFXFSR3TemporalUpscalingModule::OnPostEngineInit()
{
	ViewExtension = FSceneViewExtensions::NewExtension<FFXFSR3ViewExtension>();
}

FFXFSR3TemporalUpscaler* FFXFSR3TemporalUpscalingModule::GetFSR3Upscaler() const
{
	return TemporalUpscaler.Get();
}

IFFXFSR3TemporalUpscaler* FFXFSR3TemporalUpscalingModule::GetTemporalUpscaler() const
{
	return TemporalUpscaler.Get();
}

float FFXFSR3TemporalUpscalingModule::GetResolutionFraction(uint32 Mode) const
{
	return TemporalUpscaler->GetResolutionFraction(Mode);
}

bool FFXFSR3TemporalUpscalingModule::IsPlatformSupported(EShaderPlatform Platform) const
{
	FStaticShaderPlatform ShaderPlatform(Platform);
	
	// All we need is SM5, which can run the RHI backend. Specific backends are handled elsewhere.
	bool bIsSupported = IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	
	return bIsSupported;
}

void FFXFSR3TemporalUpscalingModule::SetEnabledInEditor(bool bEnabled)
{
#if WITH_EDITOR
	return TemporalUpscaler->SetEnabledInEditor(bEnabled);
#endif
}

#undef LOCTEXT_NAMESPACE