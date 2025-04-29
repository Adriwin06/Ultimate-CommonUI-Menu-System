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
#include "Tickable.h"
#include "Performance/LatencyMarkerModule.h"

typedef struct _xell_context_handle_t* xell_context_handle_t;

class FXeLLLatencyMarkers : public ILatencyMarkerModule, public FTickableGameObject, public FSelfRegisteringExec
{
public:
	explicit FXeLLLatencyMarkers(xell_context_handle_t InXeLLContext);
	virtual ~FXeLLLatencyMarkers() {}

	void Tick(float DeltaTime) override;
	bool IsTickable() const override { return true; }
	bool IsTickableInEditor() const override { return true; }
	bool IsTickableWhenPaused() const override { return true; }
	TStatId GetStatId(void) const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FLatencyMarkers, STATGROUP_Tickables); }

	void Initialize() override {};

	void SetEnabled(bool bInEnabled) override;
	bool GetEnabled() override;
	bool GetAvailable() override;
	void SetFlashIndicatorEnabled(bool bInEnabled) override;
	bool GetFlashIndicatorEnabled() override;

	bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

	void SetInputSampleLatencyMarker(uint64 FrameNumber) override;
	void SetSimulationLatencyMarkerStart(uint64 FrameNumber) override;
	void SetSimulationLatencyMarkerEnd(uint64 FrameNumber) override;
	void SetRenderSubmitLatencyMarkerStart(uint64 FrameNumber) override;
	void SetRenderSubmitLatencyMarkerEnd(uint64 FrameNumber) override;
	void SetPresentLatencyMarkerStart(uint64 FrameNumber) override;
	void SetPresentLatencyMarkerEnd(uint64 FrameNumber) override;
	void SetFlashIndicatorLatencyMarker(uint64 FrameNumber) override;
	void SetCustomLatencyMarker(uint32 MarkerId, uint64 FrameNumber) override;

	float GetTotalLatencyInMs() override { return AverageTotalLatencyMs; }
	float GetGameLatencyInMs() override { return AverageGameLatencyMs; }  // This is defined as "Game simulation start to driver submission end"
	float GetRenderLatencyInMs() override { return AverageRenderLatencyMs; }  // This is defined as "OS render queue start to GPU render end"

	float GetInputLatencyInMs() { return AverageInputLatencyMs; }

	float GetSimulationLatencyInMs() override { return AverageSimulationLatencyMs; }
	float GetRenderSubmitLatencyInMs() override { return AverageRenderSubmitLatencyMs; }
	float GetPresentLatencyInMs() override { return AveragePresentLatencyMs; }
	float GetDriverLatencyInMs() override { return AverageDriverLatencyMs; }
	float GetOSRenderQueueLatencyInMs() override { return AverageOSRenderQueueLatencyMs; }
	float GetGPURenderLatencyInMs() override { return AverageGPURenderLatencyMs; }

	float GetRenderSubmitOffsetFromFrameStartInMs() override { return RenderSubmitOffsetMs; }
	float GetPresentOffsetFromFrameStartInMs() override { return PresentOffsetMs; }
	float GetDriverOffsetFromFrameStartInMs() override { return DriverOffsetMs; }
	float GetOSRenderQueueOffsetFromFrameStartInMs() override { return OSRenderQueueOffsetMs; }
	float GetGPURenderOffsetFromFrameStartInMs() override { return GPURenderOffsetMs; }

private:
	void ClearLatencyResults();
	xell_context_handle_t XeLLContext = nullptr;

	TArray<uint64> SentMarkersFrameNumbers;
	bool CountMarkerFrameNumbers(uint32 markerID, uint64 frameNumber);

	bool bProperDriverVersion = false;

	float AverageTotalLatencyMs = 0.0f;
	float AverageGameLatencyMs = 0.0f;
	float AverageRenderLatencyMs = 0.0f;
	float AverageInputLatencyMs = 0.0f;

	float AverageSimulationLatencyMs = 0.0f;
	float AverageRenderSubmitLatencyMs = 0.0f;
	float AveragePresentLatencyMs = 0.0f;
	float AverageDriverLatencyMs = 0.0f;
	float AverageOSRenderQueueLatencyMs = 0.0f;
	float AverageGPURenderLatencyMs = 0.0f;

	float RenderSubmitOffsetMs = 0.0f;
	float PresentOffsetMs = 0.0f;
	float DriverOffsetMs = 0.0f;
	float OSRenderQueueOffsetMs = 0.0f;
	float GPURenderOffsetMs = 0.0f;

	bool bEnabled = true;
	bool bFlashIndicatorEnabled = true;
	bool bFlashIndicatorDriverControlled = false;
};
#endif
