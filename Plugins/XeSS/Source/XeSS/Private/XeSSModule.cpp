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

#include "XeSSMacros.h"

#include "Engine/Engine.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Windows/WindowsHWrapper.h"
#include "XeSSRHI.h"
#include "XeSSUpscaler.h"
#include "XeSSUtil.h"

#define LOCTEXT_NAMESPACE "FXeSSPlugin"
DEFINE_LOG_CATEGORY(LogXeSS);

TAutoConsoleVariable<FString> GCVarXeSSVersion(
	TEXT("r.XeSS.Version"),
	TEXT("Unknown"),
	TEXT("Show XeSS SDK's version"),
	ECVF_ReadOnly);

static TUniquePtr<FXeSSUpscaler> XeSSUpscaler;
static TUniquePtr<FXeSSRHI> XeSSRHI;
#if XESS_ENGINE_VERSION_GEQ(5, 1)
static TSharedPtr<FXeSSUpscalerViewExtension, ESPMode::ThreadSafe> XeSSUpscalerViewExtension;
#endif // XESS_ENGINE_VERSION_GEQ(5, 1)

void FXeSSPlugin::StartupModule()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("XeSS");

	// Log plugin version info
	UE_LOG(LogXeSS, Log, TEXT("XeSS plugin version: %u, version name: %s"), 
		Plugin->GetDescriptor().Version,
		*Plugin->GetDescriptor().VersionName
	);

	// Do not load the library if XeSS is explicitly disabled
	if (FParse::Param(FCommandLine::Get(), TEXT("xessdisabled")))
	{
		UE_LOG(LogXeSS, Log, TEXT("XeSS disabled by command line option"));
		return;
	}

	// XeSS is only currently supported for DX12
	const FString RHIName = GDynamicRHI->GetName();
	if (RHIName != TEXT("D3D12"))
	{
		const auto CVarXeSSEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Enabled"));

		CVarXeSSEnabled->AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateLambda([RHIName](IConsoleVariable* InVariable)
		{
			if (InVariable->GetBool())
			{
				XeSSUtil::AddErrorMessageToScreen(
					FString::Printf(TEXT("Current RHI %s doesn't support XeSS, please switch to D3D12 to use it"), *RHIName),
					XeSSUtil::ON_SCREEN_MESSAGE_KEY_NOT_SUPPORT_RHI
				);
			}
			else
			{
				XeSSUtil::RemoveMessageFromScreen(XeSSUtil::ON_SCREEN_MESSAGE_KEY_NOT_SUPPORT_RHI);
			}
		}));
		UE_LOG(LogXeSS, Log, TEXT("Current RHI %s doesn't support XeSS, please switch to D3D12 to use it"), *RHIName);
		return;
	}

	// Add DLL search path for XeFX.dll and XeFX_Loader.dll
	SetDllDirectory(*FPaths::Combine(Plugin->GetBaseDir(), TEXT("/Binaries/ThirdParty/Win64")));

	// Get XeSS SDK version
	xess_version_t XeSSLibVersion;
	if (xessGetVersion(&XeSSLibVersion) != XESS_RESULT_SUCCESS)
	{
		UE_LOG(LogXeSS, Warning, TEXT("Error when calling XeSS function: xessGetVersion"));
		return;
	}

	TStringBuilder<32> VersionStringBuilder;
	VersionStringBuilder << "XeSS version: " << XeSSLibVersion.major << "." << XeSSLibVersion.minor << "." << XeSSLibVersion.patch;
	GCVarXeSSVersion->Set(VersionStringBuilder.GetData());

	UE_LOG(LogXeSS, Log, TEXT("Loading XeSS library %d.%d.%d on %s RHI %s"),
		XeSSLibVersion.major, XeSSLibVersion.minor, XeSSLibVersion.patch,
		RHIVendorIdToString(), *RHIName);

	XeSSRHI.Reset(new FXeSSRHI(GDynamicRHI));
	check(XeSSRHI);

	bool bXeSSInitialized = XeSSRHI->IsXeSSInitialized();

	if (bXeSSInitialized)
	{
		XeSSUpscaler.Reset(new FXeSSUpscaler(XeSSRHI.Get()));
		check(XeSSUpscaler);
#if XESS_ENGINE_VERSION_GEQ(5, 1)
		XeSSUpscalerViewExtension = FSceneViewExtensions::NewExtension<FXeSSUpscalerViewExtension>(XeSSUpscaler.Get());
#endif // XESS_ENGINE_VERSION_GEQ(5, 1)
	}
	else
	{
		XeSSRHI.Reset();
		return;
	}

	UE_LOG(LogXeSS, Log, TEXT("XeSS successfully initialized"));
}

void FXeSSPlugin::ShutdownModule()
{
	UE_LOG(LogXeSS, Log, TEXT("XeSS plugin shut down"));

#if XESS_ENGINE_VERSION_GEQ(5, 1)
	XeSSUpscalerViewExtension = nullptr;
#else // XESS_ENGINE_VERSION_GEQ(5, 1)
	// restore default screen percentage driver and upscaler
	GCustomStaticScreenPercentage = nullptr;

#if ENGINE_MAJOR_VERSION < 5
	GTemporalUpscaler = ITemporalUpscaler::GetDefaultTemporalUpscaler();
#endif // ENGINE_MAJOR_VERSION < 5

#endif // XESS_ENGINE_VERSION_GEQ(5, 1)

	XeSSRHI.Reset();
	XeSSUpscaler.Reset();
}

FXeSSRHI* FXeSSPlugin::GetXeSSRHI() const
{
	return XeSSRHI.Get();
}

FXeSSUpscaler* FXeSSPlugin::GetXeSSUpscaler() const
{
	return XeSSUpscaler.Get();
}

bool FXeSSPlugin::IsXeSSSupported() const
{
	// XeSSRHI will be reset if XeSS is not supported(fail to initialize XeSS)
	return XeSSRHI.IsValid();
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FXeSSPlugin, XeSSPlugin)

