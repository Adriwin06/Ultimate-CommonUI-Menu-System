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
#include "SceneView.h"
#include "Misc/CoreMisc.h"
#include "Tickable.h"
#include "CollisionQueryParams.h"
#include "Windows/WindowsApplication.h"
#include "Performance/MaxTickRateHandlerModule.h"
#include "Performance/LatencyMarkerModule.h"
#include "sl_helpers.h"
#include "sl_reflex.h"

class FStreamlineRHI;

class FStreamlineCameraManager
{
public:
	FStreamlineCameraManager() : PrevRenderedWorldToView(FMatrix::Identity), PrevRenderedViewToClip(FMatrix::Identity) {}
	void SetCameraData(FSceneView& InView, uint64 FrameID);
	void LateUpdate_GameThread(APlayerController* Player, uint64 FrameID);
	void PreRenderViewFamily_RenderThread(FSceneViewFamily& InViewFamily, uint64 FrameID);
	void PreRenderView_RenderThread(FSceneView& InView, uint64 FrameID);
	void PostRenderView_RenderThread(FSceneView& InView, uint64 FrameID);
private:
	void CacheSceneInfo(int64 FrameID, USceneComponent* Component, FCollisionQueryParams& CollisionParams);
	void GatherLateUpdatePrimitives(int64 FrameID, USceneComponent* ParentComponent, FCollisionQueryParams& CollisionParams);
	void LateUpdate_RenderThread(FSceneInterface* Scene, uint64 FrameID, const FMatrix& LateUpdateTransform);

	struct FLateUpdateState
	{
		/** Frame ID for tracking */
		int64 FrameID;
		/** Primitives that need late update before rendering */
		TMap<FPrimitiveSceneInfo*, int32> Primitives;
		/** Collision parameters for late update clip prevention */
		FCollisionQueryParams CollisionParams;
		UWorld* World;
		/** Matrix data for predictive rendering */
		FMatrix UpdatedWorldToView, UpdatedViewToClip;
	};

	struct FViewPredictionData
	{
		int64 FrameID;
		float DeltaTime;
		FQuat Rotation;
		FVector Translation;
		float HFov;
	};

	FMatrix PrevRenderedWorldToView, PrevRenderedViewToClip;

	const static size_t FramesInFlight = 3;
	FLateUpdateState UpdateStates[FramesInFlight];

	FViewPredictionData ViewPredictionData[2];
};

bool DoesFeatureUseCameraData();
