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

#include "XeLLLatencyMarkers.h"

#if XESS_ENGINE_WITH_XEFG_API
#include "XeLLModule.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "xell/xell.h"
#include "xell/xell_d3d12.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "Engine/GameViewportClient.h"
#include "HAL/IConsoleManager.h"

#include "RHI.h"
#include "Engine/Console.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "XeLLDelegates.h"
#include "XeSSUnrealD3D12RHIIncludes.h"
#include "XeSSUnrealRHI.h"

DEFINE_LOG_CATEGORY_STATIC(LogLatencyMarkers, Log, All);

int32 DisableXeLLLatencyMarkers = 0;
static FAutoConsoleVariableRef CVarDisableXeLLLatencyMarkers(
	TEXT("r.XeLL.DisableLatencyMarkers"),
	DisableXeLLLatencyMarkers,
	TEXT("Disable XeLL Latency Markers")
);

int32 EnableXeLLLatencyResult = 0;
static FAutoConsoleVariableRef CVarEnableXeLLLatencyResult(
	TEXT("r.XeLL.EnableLatencyResult"),
	EnableXeLLLatencyResult,
	TEXT("Enable XeLL Latency Result.(Not available)")
);

int32 AllowRepeatedMarkers = 0;
static FAutoConsoleVariableRef CVarAllowRepeatedMarkers(
	TEXT("r.XeLL.AllowRepeatedMarkers"),
	AllowRepeatedMarkers,
	TEXT("Allow repeated markers sent to SDK.")
);

FXeLLLatencyMarkers::FXeLLLatencyMarkers(xell_context_handle_t InXeLLContext) : XeLLContext(InXeLLContext)
{
	SentMarkersFrameNumbers.Init(MAX_uint64, XELL_MARKER_COUNT);
	// clear latency results when switch on and off
	CVarEnableXeLLLatencyResult->AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateLambda([this](IConsoleVariable* InVariable)
	{
		ClearLatencyResults();
	}));
}

void FXeLLLatencyMarkers::Tick(float DeltaTime)
{
	if (DisableXeLLLatencyMarkers == 0 && bEnabled)
	{
		// Generate latency result
		if (EnableXeLLLatencyResult != 0)
		{
			// no frame report for now
		}
	}
}

void FXeLLLatencyMarkers::ClearLatencyResults()
{
	AverageTotalLatencyMs = 0.0f;
	AverageGameLatencyMs = 0.0f;
	AverageRenderLatencyMs = 0.0f;

	AverageSimulationLatencyMs = 0.0f;
	AverageRenderSubmitLatencyMs = 0.0f;
	AveragePresentLatencyMs = 0.0f;
	AverageDriverLatencyMs = 0.0f;
	AverageOSRenderQueueLatencyMs = 0.0f;
	AverageGPURenderLatencyMs = 0.0f;

	RenderSubmitOffsetMs = 0.0f;
	PresentOffsetMs = 0.0f;
	DriverOffsetMs = 0.0f;
	OSRenderQueueOffsetMs = 0.0f;
	GPURenderOffsetMs = 0.0f;

	AverageInputLatencyMs = 0.0f;
}

void FXeLLLatencyMarkers::SetEnabled(bool bInEnabled)
{
	if (!bInEnabled)
	{
		// Reset module back to default values in case re-enabled in the same session
		ClearLatencyResults();

		bFlashIndicatorEnabled = false;
	}

	bEnabled = bInEnabled;
}

bool FXeLLLatencyMarkers::GetEnabled()
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		return false;
	}
#endif

	if (DisableXeLLLatencyMarkers != 0)
		return false;

	return bEnabled;
}

bool FXeLLLatencyMarkers::GetAvailable()
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		return false;
	}
#endif

	if (DisableXeLLLatencyMarkers != 0)
		return false;

	return true;
}

void FXeLLLatencyMarkers::SetFlashIndicatorEnabled(bool bInEnabled)
{
	bFlashIndicatorEnabled = bInEnabled;
}

bool FXeLLLatencyMarkers::GetFlashIndicatorEnabled()
{
	if (DisableXeLLLatencyMarkers != 0)
		return false;

	return bFlashIndicatorEnabled;
}

bool FXeLLLatencyMarkers::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	bool bHandled = false;
	FString XeLLLatencyMarkers;
	APlayerController* PlayerController = (InWorld ? UGameplayStatics::GetPlayerController(InWorld, 0) : nullptr);
	ULocalPlayer* LocalPlayer = (PlayerController ? Cast<ULocalPlayer>(PlayerController->Player) : nullptr);

	if (FParse::Value(Cmd, TEXT("XeLLLatencyMarkers="), XeLLLatencyMarkers))
	{
		if (XeLLLatencyMarkers == "0")
		{
			DisableXeLLLatencyMarkers = true;

			if (LocalPlayer && LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->ViewportConsole)
			{
				LocalPlayer->ViewportClient->ViewportConsole->OutputText("XeLL Latency Markers: Off");
			}

			UE_LOG(LogLatencyMarkers, Log, TEXT("XeLL Latency Markers: Off"));
		}
		else if (XeLLLatencyMarkers == "1")
		{
			DisableXeLLLatencyMarkers = false;

			if (LocalPlayer && LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->ViewportConsole)
			{
				LocalPlayer->ViewportClient->ViewportConsole->OutputText("XeLL Latency Markers: On");
			}

			UE_LOG(LogLatencyMarkers, Log, TEXT("XeLL Latency Markers: On"));
		}

		bHandled = true;
	}

	return bHandled;
}

bool FXeLLLatencyMarkers::CountMarkerFrameNumbers(uint32 markerID, uint64 frameNumber)
{
	if (SentMarkersFrameNumbers[markerID] == MAX_uint64 || frameNumber > SentMarkersFrameNumbers[markerID])
	{
		SentMarkersFrameNumbers[markerID] = frameNumber;
		return true;
	}
	return false;
}

void FXeLLLatencyMarkers::SetInputSampleLatencyMarker(uint64 FrameNumber)
{
	if (GetEnabled() && (AllowRepeatedMarkers || CountMarkerFrameNumbers(XELL_INPUT_SAMPLE, FrameNumber)))
	{
		xell_result_t ret = xellAddMarkerData(XeLLContext, FrameNumber, XELL_INPUT_SAMPLE);
		if (ret != XELL_RESULT_SUCCESS)
		{
			UE_LOG(LogLatencyMarkers, Warning, TEXT("XeLL Add Latency Markers: [Input] failed!"));
		}
	}
}

void FXeLLLatencyMarkers::SetSimulationLatencyMarkerStart(uint64 FrameNumber)
{
	if (GetEnabled() && (AllowRepeatedMarkers || CountMarkerFrameNumbers(XELL_SIMULATION_START, FrameNumber)))
	{
		xell_result_t ret;
		ret = xellAddMarkerData(XeLLContext, FrameNumber, XELL_SIMULATION_START);
		if (ret != XELL_RESULT_SUCCESS)
		{
			UE_LOG(LogLatencyMarkers, Warning, TEXT("XeLL Add Latency Markers: [Sim Start] failed!"));
		}
	}
}

void FXeLLLatencyMarkers::SetSimulationLatencyMarkerEnd(uint64 FrameNumber)
{
	if (GetEnabled() && (AllowRepeatedMarkers || CountMarkerFrameNumbers(XELL_SIMULATION_END, FrameNumber)))
	{
		xell_result_t ret = xellAddMarkerData(XeLLContext, FrameNumber, XELL_SIMULATION_END);
		if (ret != XELL_RESULT_SUCCESS)
		{
			UE_LOG(LogLatencyMarkers, Warning, TEXT("XeLL Add Latency Markers: [Sim End] failed!"));
		}
	}
}

void FXeLLLatencyMarkers::SetRenderSubmitLatencyMarkerStart(uint64 FrameNumber)
{
	if (GetEnabled() && (AllowRepeatedMarkers || CountMarkerFrameNumbers(XELL_RENDERSUBMIT_START, FrameNumber)))
	{
		xell_result_t ret = xellAddMarkerData(XeLLContext, FrameNumber, XELL_RENDERSUBMIT_START);
		if (ret != XELL_RESULT_SUCCESS)
		{
			UE_LOG(LogLatencyMarkers, Warning, TEXT("XeLL Add Latency Markers: [Render Submit Start] failed!"));
		}
	}
}

void FXeLLLatencyMarkers::SetRenderSubmitLatencyMarkerEnd(uint64 FrameNumber)
{
	if (GetEnabled() && (AllowRepeatedMarkers || CountMarkerFrameNumbers(XELL_RENDERSUBMIT_END, FrameNumber)))
	{
		xell_result_t ret = xellAddMarkerData(XeLLContext, FrameNumber, XELL_RENDERSUBMIT_END);
		if (ret != XELL_RESULT_SUCCESS)
		{
			UE_LOG(LogLatencyMarkers, Warning, TEXT("XeLL Add Latency Markers: [Render Submit End] failed!"));
		}
	}
}

void FXeLLLatencyMarkers::SetPresentLatencyMarkerStart(uint64 FrameNumber)
{
	if (GetEnabled() && (AllowRepeatedMarkers || CountMarkerFrameNumbers(XELL_PRESENT_START, FrameNumber)))
	{
		xell_result_t ret = xellAddMarkerData(XeLLContext, FrameNumber, XELL_PRESENT_START);
		if (ret != XELL_RESULT_SUCCESS)
		{
			UE_LOG(LogLatencyMarkers, Warning, TEXT("XeLL Add Latency Markers: [Present Start] failed!"));
		}
	}
}

void FXeLLLatencyMarkers::SetPresentLatencyMarkerEnd(uint64 FrameNumber)
{
	if (GetEnabled() && (AllowRepeatedMarkers || CountMarkerFrameNumbers(XELL_PRESENT_END, FrameNumber)))
	{
		xell_result_t ret = xellAddMarkerData(XeLLContext, FrameNumber, XELL_PRESENT_END);
		if (ret != XELL_RESULT_SUCCESS)
		{
			UE_LOG(LogLatencyMarkers, Warning, TEXT("XeLL Add Latency Markers: [Present End] failed!"));
		}
	}
	FXeLLDelegates::OnPresentLatencyMarkerEnd.ExecuteIfBound(FrameNumber);
}

void FXeLLLatencyMarkers::SetFlashIndicatorLatencyMarker(uint64 FrameNumber)
{
	if (GetEnabled())
	{
		if (GetFlashIndicatorEnabled())
		{
			/*xell_result_t ret = xellAddMarkerData(XeLLContext, FrameNumber, XELL_TRIGGER_FLASH);
			if (ret != XELL_RESULT_SUCCESS)
			{
				UE_LOG(LogLatencyMarkers, Warning, TEXT("XeLL Add Latency Markers: [Trigger Flash] failed!"));
			}*/
		}
	}
}

void FXeLLLatencyMarkers::SetCustomLatencyMarker(uint32 MarkerId, uint64 FrameNumber)
{
	if (GetEnabled() && (AllowRepeatedMarkers || CountMarkerFrameNumbers(MarkerId, FrameNumber)))
	{
		xell_result_t ret = xellAddMarkerData(XeLLContext, FrameNumber, (xell_latency_marker_type_t)MarkerId);
		if (ret != XELL_RESULT_SUCCESS)
		{
			UE_LOG(LogLatencyMarkers, Warning, TEXT("XeLL Add Latency Markers: Custom Marker[%d] failed!"), MarkerId);
		}
	}
}
#endif
