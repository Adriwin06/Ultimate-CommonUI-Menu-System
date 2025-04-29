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
#include "StreamlineReflexCamera.h"
#include "StreamlineReflex.h"

#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "RHI.h"
#include "Runtime/Engine/Classes/GameFramework/PlayerController.h"
#include "Runtime/Engine/Classes/Components/PrimitiveComponent.h"
#include "Runtime/Engine/Classes/GameFramework/Pawn.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "Runtime/Launch/Resources/Version.h"
#include "ScenePrivate.h"

#include "StreamlineCorePrivate.h"



#include "CollisionQueryParams.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
#include "Null/NullPlatformApplicationMisc.h"
#endif

// that is right now defined for some custom engine branches
#ifndef WITH_LATE_UPDATE_MATRIX
#pragma message( "NoWarp mask and predictive rendering support disabled (WITH_LATE_UPDATE_MATRIX not defined)" )
#define WITH_LATE_UPDATE_MATRIX 0
#else 
#if WITH_LATE_UPDATE_MATRIX
#pragma message( "NoWarp mask and predictive rendering support enabled (WITH_LATE_UPDATE_MATRIX is 1)" )
#else
#pragma message( "NoWarp mask and predictive rendering support DISABLED (WITH_LATE_UPDATE_MATRIX is 0)" )
#endif 

#endif

#include "StreamlineAPI.h"
#include "StreamlineConversions.h"
#include "StreamlineCore.h"
#include "StreamlineCorePrivate.h"
#include "StreamlineDLSSG.h"
#include "StreamlineLatewarp.h"
#include "StreamlineRHI.h"

static TAutoConsoleVariable<bool> CVarStreamlineReflexCameraPredictor(
	TEXT("r.Streamline.Reflex.CameraPredictor"),
	0,
	TEXT("Which predictive rendering camera predictor to use (default = 0)\n")
	TEXT("0: Use the first person predictor\n")
	TEXT("1: Use the third person predictor\n"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<bool> CVarStreamlineReflexPredictiveRendering(
	TEXT("r.Streamline.Reflex.PredictiveRendering"),
#if WITH_LATE_UPDATE_MATRIX
	1,
	TEXT("Whether predictive rendering is enabled or not. (default = 1) since custom engine \n"),
#else
	0,
	TEXT("Whether predictive rendering is enabled or not. (default = 0), since stock engine\n"),
#endif

	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<bool> CVarStreamlineReflexClipCorrection(
	TEXT("r.Streamline.Reflex.ClipCorrection"), 0,
	TEXT("Whether clip correction is enabled or not. (default = 0)\n"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarStreamlineReflexClipRadius(
	TEXT("r.Streamline.Reflex.ClipRadius"), 10.f,
	TEXT("Collision radius for camera extrapolation clipping. (default = 10.f)\n"),
	ECVF_RenderThreadSafe);


static TAutoConsoleVariable<int32> CVarStreamlineReflexActorDebug(
	TEXT("r.Streamline.Reflex.ActorDebug"), 0,
	TEXT("Whether Actor debug messages are shown. (default = 0)\n"),
	ECVF_RenderThreadSafe);


static TAutoConsoleVariable<int32> CVarStreamlineReflexPredictiveRenderingLateUpdateMode(
	TEXT("r.Streamline.Reflex.PredictiveRendering.LateUpdateMode"), 1,
	TEXT("Select how the late update matrix is applied. (default = 1)\n"),
	ECVF_RenderThreadSafe);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
FCriticalSection GameThreadDebugMessagesCS;
TArray<FString> GameThreadDebugMessages;

FCriticalSection RenderThreadDebugMessagesCS;
TArray<FString> RenderThreadDebugMessages;

FDelegateHandle ReflexCameraOnScreenMessagesDelegateHandle;
void GetReflexCameraOnScreenMessages(TMultiMap<FCoreDelegates::EOnScreenMessageSeverity, FText>& OutMessages)
{
	check(IsInGameThread());
	if (CVarStreamlineReflexActorDebug.GetValueOnGameThread())
	{
		{
			FScopeLock Lock(&GameThreadDebugMessagesCS);

			for (auto String : GameThreadDebugMessages)
			{
				OutMessages.Add(FCoreDelegates::EOnScreenMessageSeverity::Info, FText::FromString(String));
			}
		}

		{
			FScopeLock Lock(&RenderThreadDebugMessagesCS);
			for (auto String : RenderThreadDebugMessages)
			{
				OutMessages.Add(FCoreDelegates::EOnScreenMessageSeverity::Info, FText::FromString(String));
			}
		}
	}
}

void DumpActor(const TCHAR* Stage, const AActor* Actor)
{
	const int32 ActorDebug = CVarStreamlineReflexActorDebug.GetValueOnAnyThread();
	if (Actor && ActorDebug != 0)
	{
		TArray<FString>& DebugMessages = IsInGameThread() ? GameThreadDebugMessages : RenderThreadDebugMessages;
		DebugMessages.Add(FString::Printf(TEXT("<%s,%s> %s (A%s)"), Stage,*CurrentThreadName(),  *Actor->GetName(), *Actor->GetClass()->GetName()));

		if (const AActor* ParentActor = Actor->GetParentActor())
		{
			DebugMessages.Add(FString::Printf(TEXT("ParentActor %s (A%s)"), *ParentActor->GetName(), *ParentActor->GetClass()->GetName()));
		}

		if (const AActor* ParentComponent = Actor->GetParentActor())
		{
			DebugMessages.Add(FString::Printf(TEXT("ParentComponent %s(U%s))"), *ParentComponent->GetName(), *ParentComponent->GetClass()->GetName()));
		}


		TArray<FString> OtherComponentNames;
		TArray<FString> PrimitiveComponentNames;
		for (auto& Component : Actor->GetComponents())
		{
			bool bIsPrimitiveComponent = false;

			
			FString OwnerName;

			if(Component->GetOwner() && Component->GetOwner()!= Actor)
			{
				OwnerName = FString::Printf(TEXT(" Owner: %s"), *Component->GetOwner()->GetName());
			}
			FString PrimitiveExtra;
			UPrimitiveComponent* PrimitiveComponent = nullptr;
			FPrimitiveSceneProxy* SceneProxy = nullptr;
			if (Component->GetClass()->IsChildOf(UPrimitiveComponent::StaticClass()))
			{
				bIsPrimitiveComponent = true;
				PrimitiveComponent = dynamic_cast<UPrimitiveComponent*>(Component);

				SceneProxy = PrimitiveComponent->SceneProxy;

				if (SceneProxy)
				{
					FPrimitiveSceneInfo* SceneInfo = SceneProxy->GetPrimitiveSceneInfo();
				
					int32 NumPrimitives = 0;

					FMatrix LateUpdateMatrix = FMatrix(EForceInit::ForceInitToZero);
#if WITH_LATE_UPDATE_MATRIX					
					LateUpdateMatrix = SceneProxy->GetLateUpdateMatrix();
#endif
					FMatrix LocalToWorld=SceneProxy->GetLocalToWorld();
					PrimitiveExtra = FString::Printf(TEXT(" %s I=%u, PI=%u L2W %s LU %s "), *SceneProxy->GetResourceName().ToString(), 
						SceneInfo->GetIndex(), SceneInfo->GetPersistentIndex().Index,
						*LocalToWorld.ToString(), *LateUpdateMatrix.ToString()
					);
				
					if (!SceneProxy->IsDrawnInGame()
#if RHI_RAYTRACING
					    || !SceneInfo->bDrawInGame
#endif
						)

					{
						bIsPrimitiveComponent = false;
					}
				}
				else
				{
					bIsPrimitiveComponent = false;
				}
			}
			
			TArray<FString>& ComponentNames = bIsPrimitiveComponent ? PrimitiveComponentNames : OtherComponentNames;

			if (ActorDebug >= 2)
			{
				FString ComponentClassName = Component->GetClass()->GetName();
				ComponentNames.Add(FString::Printf(TEXT("%s%s(U%s)%s"), *Component->GetName(), *PrimitiveExtra, *ComponentClassName, *OwnerName));
			}
			else if (ActorDebug >= 1)
			{
				ComponentNames.Add(FString::Printf(TEXT("%s"), *Component->GetName()));
			}
		}

		if(PrimitiveComponentNames.Num())
		{
			if (ActorDebug < 2)
			{
				const FString PrimitiveComponentList = FString::Join(PrimitiveComponentNames, TEXT(", "));
				DebugMessages.Add(FString::Printf(TEXT("  Primitives[%u] %s"), PrimitiveComponentNames.Num(), *PrimitiveComponentList));
			}
			else
			{
				for (int32 Index = 0; Index < PrimitiveComponentNames.Num(); ++Index)
				{
					DebugMessages.Add(FString::Printf(TEXT("  Primitives[%u] %s"), Index, *PrimitiveComponentNames[Index]));
				}
			}
		}

		if(OtherComponentNames.Num())
		{
			const FString OtherComponentList = FString::Join(OtherComponentNames, TEXT(", "));

			DebugMessages.Add(FString::Printf(TEXT("  Components[%u] %s"), OtherComponentNames.Num(), *OtherComponentList));
		}

		TArray<AActor*> Children;
		Actor->GetAllChildActors(Children);
		if (Children.Num())
		{
			DebugMessages.Add(FString::Printf(TEXT("  ChildActors[%u]"), Children.Num()));
		}
	}
}
#endif 

void FStreamlineCameraManager::LateUpdate_GameThread(APlayerController* Player, uint64 FrameID)
{
	check(IsInGameThread());

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!ReflexCameraOnScreenMessagesDelegateHandle.IsValid())
	{
		ReflexCameraOnScreenMessagesDelegateHandle = FCoreDelegates::OnGetOnScreenMessages.AddStatic(&GetReflexCameraOnScreenMessages);
	}

	if(CVarStreamlineReflexActorDebug.GetValueOnAnyThread())
	{
		FScopeLock Lock(&GameThreadDebugMessagesCS);
		GameThreadDebugMessages.Empty();
		if (Player)
		{
			DumpActor(TEXT("SetupViewPoint"), Player);

			if (Player->GetPawnOrSpectator())
			{
				DumpActor(TEXT("SetupViewPoint"), Player->GetPawnOrSpectator());
			}
		}
		//DumpActor(TEXT("SetupViewPoint"), Player->PlayerCameraManager);
		GameThreadDebugMessages.AddDefaulted();
	}
#endif

	if (!DoesFeatureUseCameraData())
	{
		return;
	}

	if (!Player || !Player->GetPawnOrSpectator())
	{
		return;
	}
	USceneComponent* Component = Player->GetPawnOrSpectator()->GetRootComponent();

	FLateUpdateState& LateUpdateData = UpdateStates[FrameID % FramesInFlight];
	LateUpdateData.FrameID = FrameID;
	LateUpdateData.Primitives.Reset();
	LateUpdateData.World = Component->GetWorld();
	LateUpdateData.CollisionParams = FCollisionQueryParams(SCENE_QUERY_STAT(CameraPen), false, Player);
	GatherLateUpdatePrimitives(FrameID, Component, LateUpdateData.CollisionParams);
}

void FStreamlineCameraManager::CacheSceneInfo(int64 FrameID, USceneComponent* Component, FCollisionQueryParams& CollisionParams)
{
	ensureMsgf(!Component->IsUsingAbsoluteLocation() && !Component->IsUsingAbsoluteRotation(), TEXT("SceneComponents that use absolute location or rotation are not supported by the LateUpdateManager"));
	// If a scene proxy is present, cache it
	UPrimitiveComponent* PrimitiveComponent = dynamic_cast<UPrimitiveComponent*>(Component);
	if (PrimitiveComponent && PrimitiveComponent->SceneProxy)
	{
		CollisionParams.AddIgnoredComponent(PrimitiveComponent);
		FPrimitiveSceneInfo* PrimitiveSceneInfo = PrimitiveComponent->SceneProxy->GetPrimitiveSceneInfo();
		if (PrimitiveSceneInfo && PrimitiveSceneInfo->IsIndexValid())
		{
			PrimitiveComponent->SetRenderCustomDepth(true);
			PrimitiveComponent->SetCustomDepthStencilValue(1);
			UpdateStates[FrameID % FramesInFlight].Primitives.Emplace(PrimitiveSceneInfo, PrimitiveSceneInfo->GetIndex());
		}
	}
}

void FStreamlineCameraManager::GatherLateUpdatePrimitives(int64 FrameID, USceneComponent* ParentComponent, FCollisionQueryParams& CollisionParams)
{
	CacheSceneInfo(FrameID, ParentComponent, CollisionParams);

	TArray<USceneComponent*> Components;
	ParentComponent->GetChildrenComponents(true, Components);
	for (USceneComponent* Component : Components)
	{
		if (Component != nullptr)
		{
			CacheSceneInfo(FrameID, Component, CollisionParams);
		}
	}
}

void FStreamlineCameraManager::PreRenderViewFamily_RenderThread(FSceneViewFamily& InViewFamily, uint64 FrameID)
{
	check(IsInRenderingThread());
	if (!DoesFeatureUseCameraData())
	{
		return;
	}

	const FSceneView* MainView = InViewFamily.Views[0];
	check(MainView);

	if (!MainView->bCameraCut && FrameID > 0 && CVarStreamlineReflexPredictiveRendering.GetValueOnRenderThread())
	{
		const FLateUpdateState& LateUpdateData = UpdateStates[FrameID % FramesInFlight];
		if (LateUpdateData.FrameID == FrameID)
		{
			FVector CurrentViewOrigin = MainView->ViewLocation;
			FMatrix CurrentViewRotation = FInverseRotationMatrix(MainView->ViewRotation)
				* FMatrix(
					FPlane(0, 0, 1, 0),
					FPlane(1, 0, 0, 0),
					FPlane(0, 1, 0, 0),
					FPlane(0, 0, 0, 1));
			FMatrix CurrentViewMatrix = FTranslationMatrix(-CurrentViewOrigin) * CurrentViewRotation;

			FVector PredictedViewOrigin = LateUpdateData.UpdatedWorldToView.GetOrigin();
			FMatrix PredictedViewRotation = LateUpdateData.UpdatedWorldToView.RemoveTranslation();
			FMatrix PredictedViewMatrix = FTranslationMatrix(PredictedViewOrigin) * PredictedViewRotation;

			const FTransform CurrentTransform = FTransform(CurrentViewMatrix);
			const FTransform PredictedTransform = FTransform(PredictedViewMatrix);

			FMatrix LateUpdateMatrix = (CurrentTransform * PredictedTransform.Inverse()).ToMatrixWithScale();

			LateUpdate_RenderThread(InViewFamily.Scene, FrameID, LateUpdateMatrix);
		}
	}
	else
	{
		LateUpdate_RenderThread(InViewFamily.Scene, FrameID, FMatrix::Identity);
	}
}

void FStreamlineCameraManager::LateUpdate_RenderThread(FSceneInterface* Scene, uint64 FrameID, const FMatrix& LateUpdateTransform)
{
	check(IsInRenderingThread());
	
	if (!DoesFeatureUseCameraData())
	{
		return;
	}

	FLateUpdateState& LateUpdateData = UpdateStates[FrameID % FramesInFlight];
	if (!LateUpdateData.Primitives.Num())
	{
		return;
	}

	bool bIndicesHaveChanged = false;

	// Apply delta to the cached scene proxies
	// Also check whether any primitive indices have changed, in case the scene has been modified in the meantime.
	for (auto& PrimitivePair : LateUpdateData.Primitives)
	{
		FPrimitiveSceneInfo* RetrievedSceneInfo = Scene->GetPrimitiveSceneInfo(PrimitivePair.Value);
		FPrimitiveSceneInfo* CachedSceneInfo = PrimitivePair.Key;

		// If the retrieved scene info is different than our cached scene info then the scene has changed in the meantime
		// and we need to search through the entire scene to make sure it still exists.
		if (CachedSceneInfo != RetrievedSceneInfo)
		{
			bIndicesHaveChanged = true;
			break; // No need to continue here, as we are going to brute force the scene primitives below anyway.
		}
		else if (CachedSceneInfo->Proxy)
		{
#if WITH_LATE_UPDATE_MATRIX
			if (CVarStreamlineReflexPredictiveRenderingLateUpdateMode.GetValueOnRenderThread() == 1)
			{
				// TODO: ApplyLateUpdateTransform gets overriden. Needs to be a callback from RendererScene
				CachedSceneInfo->Proxy->SetLateUpdateTransform(LateUpdateTransform);
			}
#endif
			PrimitivePair.Value = -1; // Set the cached index to -1 to indicate that this primitive was already processed
		}
	}

	// Indices have changed, so we need to scan the entire scene for primitives that might still exist
	if (bIndicesHaveChanged)
	{
		int32 Index = 0;
		FPrimitiveSceneInfo* RetrievedSceneInfo;
		RetrievedSceneInfo = Scene->GetPrimitiveSceneInfo(Index++);
		while (RetrievedSceneInfo)
		{
			if (RetrievedSceneInfo->Proxy && LateUpdateData.Primitives.Contains(RetrievedSceneInfo) &&
				LateUpdateData.Primitives[RetrievedSceneInfo] >= 0)
			{
#if WITH_LATE_UPDATE_MATRIX
				if (CVarStreamlineReflexPredictiveRenderingLateUpdateMode.GetValueOnRenderThread() == 1)
				{
					// TODO: ApplyLateUpdateTransform gets overriden. Needs to be a callback from RendererScene
					RetrievedSceneInfo->Proxy->SetLateUpdateTransform(LateUpdateTransform);
				}
#endif
			}
			RetrievedSceneInfo = Scene->GetPrimitiveSceneInfo(Index++);
		}
	}
}

void FStreamlineCameraManager::PreRenderView_RenderThread(FSceneView& InView, uint64 FrameID)
{
	check(IsInRenderingThread());
	if (!DoesFeatureUseCameraData())
	{
		return;
	}

	if (!InView.bCameraCut && FrameID > 0 && CVarStreamlineReflexPredictiveRendering.GetValueOnRenderThread())
	{
		const FLateUpdateState& LateUpdateData = UpdateStates[FrameID % FramesInFlight];
		if (LateUpdateData.FrameID == FrameID)
		{
			InView.UpdateProjectionMatrix(LateUpdateData.UpdatedViewToClip);

			// Since we can't set the view matrix directly, set it indirectly, accounting for UE's change in coordinate system
			InView.ViewLocation = -LateUpdateData.UpdatedWorldToView.GetOrigin();
			InView.ViewRotation = FMatrix(
				FPlane(LateUpdateData.UpdatedWorldToView.M[0][2], LateUpdateData.UpdatedWorldToView.M[0][0], LateUpdateData.UpdatedWorldToView.M[0][1], 0.f),
				FPlane(LateUpdateData.UpdatedWorldToView.M[1][2], LateUpdateData.UpdatedWorldToView.M[1][0], LateUpdateData.UpdatedWorldToView.M[1][1], 0.f),
				FPlane(LateUpdateData.UpdatedWorldToView.M[2][2], LateUpdateData.UpdatedWorldToView.M[2][0], LateUpdateData.UpdatedWorldToView.M[2][1], 0.f),
				FPlane(0.f, 0.f, 0.f, 1.f)
			).GetTransposed().Rotator();
			InView.UpdateViewMatrix();
		}
	}
}

void FStreamlineCameraManager::PostRenderView_RenderThread(FSceneView& InView, uint64 FrameID)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!CVarStreamlineReflexActorDebug.GetValueOnRenderThread())
	{
		return;
	}
	FScopeLock Lock(&RenderThreadDebugMessagesCS);
	RenderThreadDebugMessages.Empty();
	DumpActor(TEXT("PostRenderView"), InView.ViewActor);
#endif
}


void FStreamlineCameraManager::SetCameraData(FSceneView& InView, uint64 FrameID)
{
	check(IsInGameThread());
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if(false)
	{
		FScopeLock Lock(&GameThreadDebugMessagesCS);
		DumpActor(TEXT("SetupView"), InView.ViewActor);
	}
#endif


	if (!DoesFeatureUseCameraData())
	{
		return;
	}

	// Ignore multiple sets per frame
	if (ViewPredictionData[0].FrameID == FrameID)
	{
		return;
	}

	const FViewMatrices& CurrentViewMatrices = FViewMatrices(InView.ViewMatrices);
	const FVector CurrentTranslation = CurrentViewMatrices.GetPreViewTranslation();

	const FMatrix ViewMatrix = CurrentViewMatrices.GetViewMatrix();
	const FMatrix WorldToView = FMatrix(
		FPlane(ViewMatrix.M[0][0], ViewMatrix.M[0][1], ViewMatrix.M[0][2], ViewMatrix.M[0][3]),
		FPlane(ViewMatrix.M[1][0], ViewMatrix.M[1][1], ViewMatrix.M[1][2], ViewMatrix.M[0][3]),
		FPlane(ViewMatrix.M[2][0], ViewMatrix.M[2][1], ViewMatrix.M[2][2], ViewMatrix.M[0][3]),
		FPlane(CurrentTranslation.X, CurrentTranslation.Y, CurrentTranslation.Z, 1.f));

	const FMatrix ProjectionMatrix = CurrentViewMatrices.ComputeProjectionNoAAMatrix();

	sl::FrameToken* FrameToken = FStreamlineCoreModule::GetStreamlineRHI()->GetFrameToken(FrameID);

	sl::ReflexCameraData cameraData{};
	cameraData.prevRenderedWorldToViewMatrix = ToSL(FRHIStreamlineArguments::FMatrix44f(PrevRenderedWorldToView));
	cameraData.prevRenderedViewToClipMatrix = ToSL(FRHIStreamlineArguments::FMatrix44f(PrevRenderedViewToClip));
	cameraData.worldToViewMatrix = ToSL(FRHIStreamlineArguments::FMatrix44f(WorldToView));
	cameraData.viewToClipMatrix = ToSL(FRHIStreamlineArguments::FMatrix44f(ProjectionMatrix));

	CALL_SL_FEATURE_FN(sl::kFeatureReflex, slReflexSetCameraData, sl::ViewportHandle(0), *FrameToken, cameraData);


	// Predictive Rendering

	FViewPredictionData FrameData;
	FrameData.FrameID = FrameID;
	FrameData.DeltaTime = FApp::GetDeltaTime();
	FrameData.Rotation = WorldToView.RemoveTranslation().ToQuat();
	FrameData.Translation = CurrentTranslation;

#if (ENGINE_MAJOR_VERSION  == 4) || ((ENGINE_MAJOR_VERSION  == 5) && (ENGINE_MINOR_VERSION  < 3))
	float tanHalfFov = InView.ViewMatrices.GetInvProjectionMatrix().M[0][0];
#else
	float tanHalfFov = InView.ViewMatrices.GetTanHalfFov().X;
#endif
	FrameData.HFov = atan(tanHalfFov);

	if (!InView.bCameraCut && CVarStreamlineReflexPredictiveRendering.GetValueOnGameThread() &&
		(ViewPredictionData[1].FrameID + 1 == ViewPredictionData[0].FrameID) && 
		(ViewPredictionData[0].FrameID + 1 == FrameData.FrameID))
	{
		FLateUpdateState& LateUpdateData = UpdateStates[FrameID % FramesInFlight];
		LateUpdateData.UpdatedWorldToView = WorldToView;
		LateUpdateData.UpdatedViewToClip = ProjectionMatrix;

		float dtnm1 = ViewPredictionData[0].DeltaTime;
		float dt = FrameData.DeltaTime;
		float dtnp1 = FrameData.DeltaTime; // TODO: Proper estimate of next frame's deltaTime

		// Rotation prediction
		FQuat nm2r = ViewPredictionData[1].Rotation;
		FQuat nm1r = ViewPredictionData[0].Rotation;
		FQuat nr = FrameData.Rotation;

		nm1r.EnforceShortestArcWith(nm2r);
		nr.EnforceShortestArcWith(nm1r);

		FQuat deltaQ1 = nm1r * nm2r.Inverse();
		FQuat deltaQ2 = nr * nm1r.Inverse();
		// Approximate angular velocity

		FVector omegaFuture;
		if (CVarStreamlineReflexCameraPredictor.GetValueOnGameThread() == 0)
		{
			FVector omega1 = 2 / dtnm1 * FVector(deltaQ1.X, deltaQ1.Y, deltaQ1.Z);
			FVector omega2 = 2 / dt * FVector(deltaQ2.X, deltaQ2.Y, deltaQ2.Z);
			FVector alpha = (omega2 - omega1) / dt;
			omegaFuture = omega2 + alpha * dtnp1;
		} else {
			FVector omega2 = 2 / dt * FVector(deltaQ2.X, deltaQ2.Y, deltaQ2.Z);
			omegaFuture = omega2;
		}

		FQuat deltaQFuture = FQuat::Identity;

		const float omega_mag = FMath::Sqrt(omegaFuture.X * omegaFuture.X + omegaFuture.Y * omegaFuture.Y + omegaFuture.Z * omegaFuture.Z);
		if (omega_mag > 0.f)
		{
			float half_theta = omega_mag * dtnp1 / 2.0f;

			float s, c;
			FMath::SinCos(&s, &c, half_theta);

			deltaQFuture = FQuat(omegaFuture.X * s / omega_mag, omegaFuture.Y * s / omega_mag, omegaFuture.Z * s / omega_mag, c);
		}

		FQuat qFuture = deltaQFuture * nr;
		qFuture.Normalize();

		FMatrix predictedRotationMatrix = qFuture.ToMatrix();

		// Translation prediction
		FVector nm2p = ViewPredictionData[1].Translation;
		FVector nm1p = ViewPredictionData[0].Translation;
		FVector np = FrameData.Translation;

		FVector vnm1 = (nm1p - nm2p) / dtnm1;
		FVector v = (np - nm1p) / dt;
		FVector a = (v - vnm1) / dt;

		FVector predictedPos = np + dtnp1 * (v + 0.5f * a * dtnp1);

		LateUpdateData.UpdatedWorldToView = FMatrix(
			FPlane(predictedRotationMatrix.M[0][0], predictedRotationMatrix.M[0][1], predictedRotationMatrix.M[0][2], 0),
			FPlane(predictedRotationMatrix.M[1][0], predictedRotationMatrix.M[1][1], predictedRotationMatrix.M[1][2], 0),
			FPlane(predictedRotationMatrix.M[2][0], predictedRotationMatrix.M[2][1], predictedRotationMatrix.M[2][2], 0),
			FPlane(predictedPos.X, predictedPos.Y, predictedPos.Z, 1.f));

		if (CVarStreamlineReflexClipCorrection.GetValueOnGameThread())
		{
			const FVector CameraStart = -CurrentTranslation;
			const FVector CameraEnd = -LateUpdateData.UpdatedWorldToView.GetOrigin();

			FHitResult Hit;
			FCollisionShape SphereShape = FCollisionShape::MakeSphere(CVarStreamlineReflexClipRadius.GetValueOnGameThread());

			const bool bHit = LateUpdateData.World->SweepSingleByChannel(Hit, CameraStart, CameraEnd, FQuat::Identity, ECC_Camera, SphereShape, LateUpdateData.CollisionParams);
			if (bHit)
			{
				LateUpdateData.UpdatedWorldToView.SetOrigin(-Hit.Location);
			}
		}

		if (InView.IsPerspectiveProjection())
		{
			// Predict FOV
			float nm1f = ViewPredictionData[0].HFov;
			float nf = FrameData.HFov;

			float vf = (nf - nm1f) / dt;

			float predictedHFov = nf + dtnp1 * vf;
			float invTanHFov = 1.f / tan(predictedHFov);

			// TODO: Predict aspect ratio
			float invar = LateUpdateData.UpdatedViewToClip.M[1][1] / LateUpdateData.UpdatedViewToClip.M[0][0];
			LateUpdateData.UpdatedViewToClip.M[0][0] = invTanHFov;
			LateUpdateData.UpdatedViewToClip.M[1][1] = invar * invTanHFov;
		}

		PrevRenderedWorldToView = LateUpdateData.UpdatedWorldToView;
		PrevRenderedViewToClip = LateUpdateData.UpdatedViewToClip;
	}
	else
	{
		PrevRenderedWorldToView = WorldToView;
		PrevRenderedViewToClip = ProjectionMatrix;
	}

	ViewPredictionData[1] = ViewPredictionData[0];
	ViewPredictionData[0] = FrameData;
}

bool DoesFeatureUseCameraData()
{
	return ForceTagStreamlineBuffers() || IsLatewarpActive();
}

