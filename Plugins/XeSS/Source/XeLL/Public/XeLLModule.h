/*******************************************************************************
 * Copyright 2024 Intel Corporation
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

#pragma once

#include "XeSSCommonMacros.h"

#include "XeLLLatencyMarkers.h"
#include "XeLLMaxTickRateHandler.h"

#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

#include "XeSSUnrealCore.h"

typedef struct _xell_context_handle_t* xell_context_handle_t;

class FXeLLModule : public IModuleInterface, public XeSSUnreal::XTSTickerObjectBase
{
#if XESS_ENGINE_WITH_XEFG_API
public:
	FXeLLLatencyMarkers* GetLatencyMarker() { return XeLLLatencyMarker.Get(); }
	FXeLLMaxTickRateHandler* GetMaxTickRateHandler() { return XeLLMaxTickRateHandler.Get(); }
private:
	TUniquePtr<FXeLLLatencyMarkers> XeLLLatencyMarker;
	TUniquePtr<FXeLLMaxTickRateHandler> XeLLMaxTickRateHandler;
	int32 XeLLMode = 0;
#endif
public:
	/** IModuleInterface implementation */
	void StartupModule() override;
	void ShutdownModule() override;

	void DelayInit();

	// XeSSUnreal::XTSTickerObjectBase
	bool Tick(float DeltaTime) override;

	// Get the ptr of the XeLL context
	XELLCORE_API xell_context_handle_t GetXeLLContext();

	// XeFG needs XeLL to be enabled
	XELLCORE_API bool OnPreSetXeFGEnabled(bool bEnabled);

	// Check XeLL supported
	XELLCORE_API bool IsXeLLSupported();

	// Set XeLL mode, 0: off, 1: latency reduce enable
	XELLCORE_API void SetXeLLMode(const uint32 flags);
};
