/*******************************************************************************
 * Copyright 2023 Intel Corporation
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

#include "XeFGRHIModule.h"

#include "XeSSCommonMacros.h"

#include "Misc/CommandLine.h"
#include "Modules/ModuleManager.h"
#include "Templates/UniquePtr.h"
#include "XeFGRHI.h"
#include "XeLLModule.h"
#include "XeSSCommonUtil.h"
#include "XeSSUnrealD3D12RHI.h"
#include "XeSSUnrealD3D12RHIIncludes.h"
#include "XeSSUnrealRHI.h"

#if XESS_ENGINE_WITH_XEFG_API
#include "Windows/AllowWindowsPlatformTypes.h"
#include "xess_fg/xefg_swapchain.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogXeFGRHIModule, Log, All);

static TAutoConsoleVariable<bool> CVarXeFGActive(
	TEXT("r.XeFG.Active"),
	true,
	TEXT("If XeFG is active, it should help prevent conflicts with other similar plugins."),
	ECVF_ReadOnly);

static TAutoConsoleVariable<FString> CVarXeFGVersion(
	TEXT("r.XeFG.Version"),
	TEXT("Unknown"),
	TEXT("Show XeFG SDK's version."),
	ECVF_ReadOnly);

#if XESS_ENGINE_WITH_XEFG_API
static TUniquePtr<FXeFGRHI> XeFGRHI;
static FXeLLModule* XeLLModule = nullptr;
#endif

void FXeFGRHIModule::StartupModule()
{
	// Do not init module if XeFG is explicitly disabled
	if (FParse::Param(FCommandLine::Get(), TEXT("XeFGDisabled")))
	{
		UE_LOG(LogXeFGRHIModule, Log, TEXT("XeFG disabled by command line option"));
		return;
	}

	int32 XeFGUIMode = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("XeFGUIMode="), XeFGUIMode))
	{
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.UIMode"))->Set(XeFGUIMode, ECVF_SetByCommandline);
		UE_LOG(LogXeFGRHIModule, Log, TEXT("XeFG UI mode set by command line option, value: %d"), XeFGUIMode);
	}

	int32 XeFGResourceValidity = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("XeFGResourceValidity="), XeFGResourceValidity))
	{
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.ResourceValidity"))->Set(XeFGResourceValidity, ECVF_SetByCommandline);
		UE_LOG(LogXeFGRHIModule, Log, TEXT("XeFG resource validity set by command line option, value: %d"), XeFGResourceValidity);
	}

	int32 XeFGLogLevel = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("XeFGLogLevel="), XeFGLogLevel))
	{
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.LogLevel"))->Set(XeFGLogLevel, ECVF_SetByCommandline);
		UE_LOG(LogXeFGRHIModule, Log, TEXT("XeFG SDK log level set by command line option, value: %d"), XeFGLogLevel);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("XeFGOverrideSwapChainDisabled")))
	{
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.OverrideSwapChain"))->Set(false, ECVF_SetByCommandline);
		UE_LOG(LogXeFGRHIModule, Log, TEXT("XeFG override swap chain disabled by command line option"));
	}

	if (!CVarXeFGActive->GetBool())
	{
		UE_LOG(LogXeFGRHIModule, Log, TEXT("XeFGRHI module initialization skipped due to 'r.XeFG.Active' set to false."));
		return;
	}
#if XESS_ENGINE_WITH_XEFG_API
	xefg_swapchain_version_t XeFGVersion;
	XeSSCommon::PushThirdPartyDllDirectory();
	xefg_swapchain_result_t Result = xefgSwapChainGetVersion(&XeFGVersion);
	XeSSCommon::PopThirdPartyDllDirectory();
	if (XEFG_SWAPCHAIN_RESULT_SUCCESS != Result)
	{
		UE_LOG(LogXeFGRHIModule, Error, TEXT("Failed to get XeFG SDK version, result: %d"), Result);
		return;
	}
	XeSSCommon::SetSDKVersionConsoleVariable(CVarXeFGVersion->AsVariable(), TEXT("XeFG"), &XeFGVersion, sizeof(XeFGVersion));
	UE_LOG(LogXeFGRHIModule, Log, TEXT("Loaded XeFG SDK %d.%d.%d on %s RHI %s"),
		XeFGVersion.major, XeFGVersion.minor, XeFGVersion.patch, RHIVendorIdToString(), GDynamicRHI->GetName());

	XeLLModule = &FModuleManager::LoadModuleChecked<FXeLLModule>(TEXT("XeLLCore"));
	if (XeSSUnreal::IsRHIInterfaceD3D12())
	{
		XeFGRHI = MakeUnique<FXeFGRHI>(static_cast<XeSSUnreal::XD3D12DynamicRHI*>(GDynamicRHI), *XeLLModule);
	}
	else
	{
		UE_LOG(LogXeFGRHIModule, Log, TEXT("XeFG not supported for current RHI: %s"), GDynamicRHI->GetName());
	}
#endif
}

void FXeFGRHIModule::ShutdownModule()
{
#if XESS_ENGINE_WITH_XEFG_API
	XeFGRHI.Reset();
	XeLLModule = nullptr;
	// Workaround for some editor exit crash, it should be OK to unload module with shutdown
	FModuleManager::Get().UnloadModule(TEXT("XeLLCore"), true);
#endif
}

FXeFGRHI* FXeFGRHIModule::GetXeFGRHI()
{
#if XESS_ENGINE_WITH_XEFG_API
	return XeFGRHI.Get();
#else
	return nullptr;
#endif
}

IMPLEMENT_MODULE(FXeFGRHIModule, XeFGRHI)
