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

#include "XeSSModule.h"

#include "XeSSCommonMacros.h"

#include "Engine/Engine.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Windows/WindowsHWrapper.h"
#include "XeSSCommonUtil.h"
#include "XeSSRHI.h"
#include "XeSSSDKLoader.h"
#include "XeSSUpscaler.h"
#include "XeSSUtil.h"

DEFINE_LOG_CATEGORY_STATIC(LogXeSSModule, Log, All);

#define XESS_UNSUPPORTED_RHI_MESSAGE TEXT("RHI not supported by XeSS, RHI: %s")

static TAutoConsoleVariable<bool> CVarXeSSSupported(
	TEXT("r.XeSS.Supported"),
	false,
	TEXT("If XeSS is supported."),
	ECVF_ReadOnly);

static TUniquePtr<FXeSSUpscaler> XeSSUpscaler;
static TUniquePtr<FXeSSRHI> XeSSRHI;
#if XESS_ENGINE_VERSION_GEQ(5, 1)
static TSharedPtr<FXeSSUpscalerViewExtension, ESPMode::ThreadSafe> XeSSUpscalerViewExtension;
#endif

void FXeSSModule::StartupModule()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("XeSS");

	// Log plugin version info
	UE_LOG(LogXeSSModule, Log, TEXT("XeSS plugin version: %u, version name: %s"),
		Plugin->GetDescriptor().Version,
		*Plugin->GetDescriptor().VersionName
	);

	// Do not init module if XeSS is explicitly disabled
	if (FParse::Param(FCommandLine::Get(), TEXT("XeSSDisabled")))
	{
		UE_LOG(LogXeSSModule, Log, TEXT("XeSS disabled by command line option"));
		return;
	}

	int32 XeSSLogLevel = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("XeSSLogLevel="), XeSSLogLevel))
	{
		auto CVarXeSSLogLevel = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.LogLevel"));
		CVarXeSSLogLevel->Set(XeSSLogLevel);
		UE_LOG(LogXeSSModule, Log, TEXT("XeSS SDK log level set by command line option, value: %d"), XeSSLogLevel);
	}

	check(GDynamicRHI);
	const FString RHIName = FString(GDynamicRHI->GetName());
	if (RHIName != TEXT("D3D12") && RHIName != TEXT("D3D11") && RHIName != TEXT("Vulkan"))
	{
		const auto CVarXeSSEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Enabled"));

		CVarXeSSEnabled->AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateLambda([RHIName](IConsoleVariable* InVariable)
		{
			if (InVariable->GetBool())
			{
				XeSSUtil::AddErrorMessageToScreen(FString::Printf(XESS_UNSUPPORTED_RHI_MESSAGE, *RHIName), XeSSUtil::ON_SCREEN_MESSAGE_KEY_UNSUPPORTED_RHI);
			}
			else
			{
				XeSSUtil::RemoveMessageFromScreen(XeSSUtil::ON_SCREEN_MESSAGE_KEY_UNSUPPORTED_RHI);
			}
		}));
		UE_LOG(LogXeSSModule, Log, XESS_UNSUPPORTED_RHI_MESSAGE, *RHIName);
		return;
	}

	IXeSSRHIModule& XeSSRHIModule = FModuleManager::LoadModuleChecked<IXeSSRHIModule>(*FString::Format(TEXT("XeSS{0}RHI"), { RHIName }));
	XeSSRHI.Reset(XeSSRHIModule.CreateXeSSRHI());
	check(XeSSRHI);

	if (XeSSRHI->Initialize())
	{
		XeSSUpscaler.Reset(new FXeSSUpscaler(XeSSRHI.Get()));
	}
	else
	{
		XeSSRHI.Reset();
		return;
	}

	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FXeSSModule::OnPostEngineInit);
	CVarXeSSSupported->Set(true);
	UE_LOG(LogXeSSModule, Log, TEXT("XeSS successfully initialized"));
}

void FXeSSModule::ShutdownModule()
{
	UE_LOG(LogXeSSModule, Log, TEXT("XeSS shut down"));

	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	XeSSUpscalerViewExtension = nullptr;
#else
	// Restore default screen percentage driver and upscaler
	GCustomStaticScreenPercentage = nullptr;

	#if ENGINE_MAJOR_VERSION < 5
	GTemporalUpscaler = ITemporalUpscaler::GetDefaultTemporalUpscaler();
	#endif

#endif

	XeSSRHI.Reset();
	XeSSUpscaler.Reset();
}

void FXeSSModule::OnPostEngineInit()
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	check(XeSSUpscaler);
	XeSSUpscalerViewExtension = FSceneViewExtensions::NewExtension<FXeSSUpscalerViewExtension>(XeSSUpscaler.Get());
#endif
}

FXeSSRHI* FXeSSModule::GetXeSSRHI() const
{
	return XeSSRHI.Get();
}

FXeSSUpscaler* FXeSSModule::GetXeSSUpscaler() const
{
	return XeSSUpscaler.Get();
}

bool FXeSSModule::IsXeSSSupported() const
{
	// XeSSRHI will be reset if XeSS is not supported(fail to initialize XeSS)
	return XeSSRHI.IsValid();
}

IMPLEMENT_MODULE(FXeSSModule, XeSSCore)
