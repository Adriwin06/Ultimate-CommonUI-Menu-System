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

#if XESS_ENGINE_WITH_XEFG_API
#include "CoreMinimal.h"
#include "Misc/CoreMisc.h"
#include "Performance/MaxTickRateHandlerModule.h"

typedef struct _xell_context_handle_t* xell_context_handle_t;

class FXeLLMaxTickRateHandler : public IMaxTickRateHandlerModule, public FSelfRegisteringExec
{
public:
	explicit FXeLLMaxTickRateHandler(xell_context_handle_t InXeLLContext);
	virtual ~FXeLLMaxTickRateHandler() {}

	void Initialize() override {};
	void SetEnabled(bool bInEnabled) override;
	bool GetEnabled() override;
	bool GetAvailable() override;

	bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

	// Used to provide a generic customization interface for custom tick rate handlers
	void SetFlags(uint32 Flags) override;
	uint32 GetFlags() override;

	bool HandleMaxTickRate(float DesiredMaxTickRate) override;

private:
	bool bEnabled = false;
	bool bWasEnabled = false;
	bool bProperDriverVersion = false;
	float MinimumInterval = -1.0f;
	bool bLowLatencyMode = true;
	bool bBoost = false;

	uint32 CustomFlags = 0;
	uint32 LastCustomFlags = 0;

	xell_context_handle_t XeLLContext = nullptr;
};
#endif
