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

#include "XeFGModule.h"

#include "XeSSCommonMacros.h"

#include "Misc/CommandLine.h"
#include "Modules/ModuleManager.h"

#if XESS_ENGINE_WITH_XEFG_API
#include "Engine/GameViewportClient.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CoreDelegates.h"
#include "XeFGRHI.h"
#include "XeFGRHIModule.h"
#include "XeFGUIHandler.h"
#include "XeFGViewExtension.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "xess_fg/xefg_swapchain.h"
#include "Windows/HideWindowsPlatformTypes.h"

static FXeFGRHI* XeFGRHI = nullptr;
static TUniquePtr<FXeFGUIHandler> XeFGUIHandler;
#endif

DEFINE_LOG_CATEGORY_STATIC(LogXeFGModule, Log, All);

void FXeFGModule::StartupModule()
{
	// Do not init module if XeFG is explicitly disabled
	if (FParse::Param(FCommandLine::Get(), TEXT("XeFGDisabled")))
	{
		UE_LOG(LogXeFGModule, Log, TEXT("XeFG disabled by command line option"));
		return;
	}

#if XESS_ENGINE_WITH_XEFG_API
	XeFGRHI = FModuleManager::LoadModuleChecked<FXeFGRHIModule>(TEXT("XeFGRHI")).GetXeFGRHI();
	// NOTE: it will be nullptr when cooking, why?
	if (XeFGRHI)
	{
		if (XeFGRHI->IsXeFGSupported())
		{
			XeFGUIHandler.Reset(new FXeFGUIHandler(XeFGRHI));
			FCoreDelegates::OnPostEngineInit.AddRaw(this, &FXeFGModule::OnPostEngineInit);
		}
	}
#endif
}

void FXeFGModule::ShutdownModule()
{
#if XESS_ENGINE_WITH_XEFG_API
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
	XeFGUIHandler.Reset();
	XeFGViewExtension = nullptr;
	XeFGRHI = nullptr;
#endif
}

void FXeFGModule::OnPostEngineInit()
{
#if XESS_ENGINE_WITH_XEFG_API
	XeFGViewExtension = FSceneViewExtensions::NewExtension<FXeFGViewExtension>(XeFGRHI);
#endif
}

FXeFGRHI* FXeFGModule::GetXeFGRHI() const
{
#if XESS_ENGINE_WITH_XEFG_API
	return XeFGRHI;
#else
	return nullptr;
#endif
}

bool FXeFGModule::IsXeFGSupported() const
{
#if XESS_ENGINE_WITH_XEFG_API
	return XeFGRHI && XeFGRHI->IsXeFGSupported();
#else
	return false;
#endif
}

IMPLEMENT_MODULE(FXeFGModule, XeFGCore)
