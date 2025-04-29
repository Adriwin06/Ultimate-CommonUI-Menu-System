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
#pragma once

#include "CoreMinimal.h"
#include "Misc/CoreMisc.h"
#include "Tickable.h"

#include "StreamlineCore.h"

#include "Windows/WindowsApplication.h"
#include "Performance/MaxTickRateHandlerModule.h"
#include "Performance/LatencyMarkerModule.h"

class FStreamlineRHI;

class FStreamlineLatencyBase
{
protected:

	static bool bStreamlineReflexSupported;
	static bool bStreamlinePCLSupported;
	static bool IsStreamlineReflexSupported();
	static bool IsStreamlinePCLSupported();
};

class FStreamlineMaxTickRateHandler : public IMaxTickRateHandlerModule, public FStreamlineLatencyBase
{
public:

	// The native Reflex plugin and engine interfaces established bit flags as the way
	// to express various Reflex modes so we replicate those to maintain compatibility to some degree
	// note that t.Streamline.Reflex.Mode uses 0,1,2 as the enums, matching what the underlying SL API does
	// having those magic bit numbers here makes translating between them a bit easier hopefully
	enum class ReflexFlags
	{
		// flags for the mode components
		EnabledBit = 0x1,
		BoostBit   = 0x2,
		AllBits    = EnabledBit | BoostBit,
		
		// actual modes
		ReflexOff   =  0,
		ReflexOn    = EnabledBit,
		ReflexBoost = EnabledBit | BoostBit,
		
		ReflexInvalidFlags = BoostBit
		
	};

	virtual ~FStreamlineMaxTickRateHandler() {}

	virtual void Initialize() override;
	virtual void SetEnabled(bool bInEnabled) override;
	virtual bool GetEnabled() override;
	virtual bool GetAvailable() override;

	virtual void SetFlags(uint32 Flags) override;
	

	virtual bool HandleMaxTickRate(float DesiredMaxTickRate) override;

	// Inherited via IMaxTickRateHandlerModule
	virtual uint32 GetFlags() override;

	STREAMLINECORE_API static FStreamlineMaxTickRateHandler* Get();
	STREAMLINECORE_API static void Reset();

private:

	static TUniquePtr<FStreamlineMaxTickRateHandler> StreamlineMaxTickRateHandler;
};

class FStreamlineLatencyMarkers : public ILatencyMarkerModule, public IWindowsMessageHandler, public FStreamlineLatencyBase, public FTickableGameObject
{
	float AverageTotalLatencyMs = 0.0f;
	float AverageGameLatencyMs = 0.0f;
	float AverageRenderLatencyMs = 0.0f;

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


	bool bFlashIndicatorDriverControlled = false;
public:

	virtual ~FStreamlineLatencyMarkers() {}

	virtual void Initialize() override;


	virtual void SetEnabled(bool bInEnabled) override;
	virtual bool GetEnabled() override;
	virtual bool GetAvailable() override;

	virtual void SetFlashIndicatorEnabled(bool bInEnabled) override;
	virtual bool GetFlashIndicatorEnabled() override;

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableInEditor() const override { return true; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual TStatId GetStatId(void) const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FLatencyMarkers, STATGROUP_Tickables); }



	virtual void SetInputSampleLatencyMarker(uint64 FrameNumber) override;
	virtual void SetSimulationLatencyMarkerStart(uint64 FrameNumber) override;
	virtual void SetSimulationLatencyMarkerEnd(uint64 FrameNumber) override;
	virtual void SetRenderSubmitLatencyMarkerStart(uint64 FrameNumber) override;
	virtual void SetRenderSubmitLatencyMarkerEnd(uint64 FrameNumber) override;
	virtual void SetPresentLatencyMarkerStart(uint64 FrameNumber) override;
	virtual void SetPresentLatencyMarkerEnd(uint64 FrameNumber) override;
	virtual void SetFlashIndicatorLatencyMarker(uint64 FrameNumber) override;
	virtual void SetCustomLatencyMarker(uint32 MarkerId, uint64 FrameNumber) override;


	virtual float GetTotalLatencyInMs() override { return AverageTotalLatencyMs; }
	virtual float GetGameLatencyInMs() override { return AverageGameLatencyMs; } // This is defined as "Game simulation start to driver submission end"
	virtual float GetRenderLatencyInMs() override { return AverageRenderLatencyMs; } // This is defined as "OS render queue start to GPU render end"

	virtual float GetSimulationLatencyInMs() override { return AverageSimulationLatencyMs; }
	virtual float GetRenderSubmitLatencyInMs() override { return AverageRenderSubmitLatencyMs; }
	virtual float GetPresentLatencyInMs() override { return AveragePresentLatencyMs; }
	virtual float GetDriverLatencyInMs() override { return AverageDriverLatencyMs; }
	virtual float GetOSRenderQueueLatencyInMs() override { return AverageOSRenderQueueLatencyMs; }
	virtual float GetGPURenderLatencyInMs() override { return AverageGPURenderLatencyMs; }

	virtual float GetRenderSubmitOffsetFromFrameStartInMs() override { return RenderSubmitOffsetMs; }
	virtual float GetPresentOffsetFromFrameStartInMs() override { return PresentOffsetMs; }
	virtual float GetDriverOffsetFromFrameStartInMs() override { return DriverOffsetMs; }
	virtual float GetOSRenderQueueOffsetFromFrameStartInMs() override { return OSRenderQueueOffsetMs; }
	virtual float GetGPURenderOffsetFromFrameStartInMs() override { return GPURenderOffsetMs; }

	// Inherited via IWindowsMessageHandler
	virtual bool ProcessMessage(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam, int32& OutResult) override;

	STREAMLINECORE_API static FStreamlineLatencyMarkers* Get();
	STREAMLINECORE_API static void Reset();

private:

	static TUniquePtr<FStreamlineLatencyMarkers> StreamlineLatencyMarkers;

};

STREAMLINECORE_API FStreamlineMaxTickRateHandler* GetStreamlineReflexMaxTickRateHandler();
STREAMLINECORE_API FStreamlineLatencyMarkers* GetStreamlineReflexLatencyMarkerModule();

void RegisterStreamlineReflexHooks();
void UnregisterStreamlineReflexHooks();

enum class Streamline::EStreamlineFeatureSupport;
extern STREAMLINECORE_API Streamline::EStreamlineFeatureSupport QueryStreamlineReflexSupport();
extern STREAMLINECORE_API bool IsStreamlineReflexSupported();