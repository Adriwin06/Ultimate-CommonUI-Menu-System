// This file is part of the FidelityFX Super Resolution 3.0 Unreal Engine Plugin.
//
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "FFXFSR3ViewExtension.h"
#include "FFXFSR3TemporalUpscaler.h"
#include "FFXFSR3TemporalUpscalerProxy.h"
#include "FFXFSR3TemporalUpscaling.h"
#include "PostProcess/PostProcessing.h"
#include "Materials/Material.h"

#include "ScenePrivate.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "LandscapeProxy.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

static TAutoConsoleVariable<int32> CVarEnableFSR3(
	TEXT("r.FidelityFX.FSR3.Enabled"),
	1,
	TEXT("Enable FidelityFX Super Resolution for Temporal Upscale"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarEnableFSR3InEditor(
	TEXT("r.FidelityFX.FSR3.EnabledInEditorViewport"),
	0,
	TEXT("Enable FidelityFX Super Resolution for Temporal Upscale in the Editor viewport by default."),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarFSR3AdjustMipBias(
	TEXT("r.FidelityFX.FSR3.AdjustMipBias"),
	1,
	TEXT("Allow FSR3 to adjust the minimum global texture mip bias (r.ViewTextureMipBias.Min & r.ViewTextureMipBias.Offset)"),
	ECVF_ReadOnly);

static TAutoConsoleVariable<int32> CVarFSR3ForceVertexDeformationOutputsVelocity(
	TEXT("r.FidelityFX.FSR3.ForceVertexDeformationOutputsVelocity"),
	1,
	TEXT("Allow FSR3 to enable r.Velocity.EnableVertexDeformation to ensure that materials that use World-Position-Offset render valid velocities."),
	ECVF_ReadOnly);

static TAutoConsoleVariable<int32> CVarFSR3ForceLandscapeHISMMobility(
	TEXT("r.FidelityFX.FSR3.ForceLandscapeHISMMobility"),
	0,
	TEXT("Allow FSR3 to force the mobility of Landscape actors Hierarchical Instance Static Mesh components that use World-Position-Offset materials so they render valid velocities.\nSetting 1/'All Instances' is faster on the CPU, 2/'Instances with World-Position-Offset' is faster on the GPU."),
	ECVF_ReadOnly);

static void ForceLandscapeHISMMobility(FSceneViewFamily& InViewFamily, ALandscapeProxy* Landscape)
{
	for (FCachedLandscapeFoliage::TGrassSet::TIterator Iter(Landscape->FoliageCache.CachedGrassComps); Iter; ++Iter)
	{
		ULandscapeComponent* Component = (*Iter).Key.BasedOn.Get();
		if (Component)
		{
			UHierarchicalInstancedStaticMeshComponent* Used = (*Iter).Foliage.Get();
			if (Used && Used->Mobility == EComponentMobility::Static)
			{
				if (CVarFSR3ForceLandscapeHISMMobility.GetValueOnGameThread() == 2)
				{
					TArray<FStaticMaterial> const& Materials = Used->GetStaticMesh()->GetStaticMaterials();
					for (auto MaterialInfo : Materials)
					{
						const UMaterial* Material = MaterialInfo.MaterialInterface->GetMaterial_Concurrent();
						if (const FMaterialResource* MaterialResource = Material->GetMaterialResource(InViewFamily.GetFeatureLevel()))
						{
							check(IsInGameThread());
							bool bAlwaysHasVelocity = MaterialResource->MaterialModifiesMeshPosition_GameThread();
							if (bAlwaysHasVelocity)
							{
								Used->Mobility = EComponentMobility::Stationary;
								break;
							}
						}
					}
				}
				else
				{
					Used->Mobility = EComponentMobility::Stationary;
				}
			}
		}
	}
}

FFXFSR3ViewExtension::FFXFSR3ViewExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister)
{
	static IConsoleVariable* CVarMinAutomaticViewMipBiasMin = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ViewTextureMipBias.Min"));
	static IConsoleVariable* CVarMinAutomaticViewMipBiasOffset = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ViewTextureMipBias.Offset"));
	static IConsoleVariable* CVarVertexDeformationOutputsVelocity = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Velocity.EnableVertexDeformation"));
	static IConsoleVariable* CVarVelocityEnableLandscapeGrass = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Velocity.EnableLandscapeGrass"));
	static IConsoleVariable* CVarReactiveMask = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR3.CreateReactiveMask"));
	static IConsoleVariable* CVarSeparateTranslucency = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SeparateTranslucency"));
	static IConsoleVariable* CVarSSRExperimentalDenoiser = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SSR.ExperimentalDenoiser"));
	static IConsoleVariable* CVarFSR3SSRExperimentalDenoiser = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR3.UseSSRExperimentalDenoiser"));

	PreviousFSR3State = CVarEnableFSR3.GetValueOnAnyThread();
	PreviousFSR3StateRT = CVarEnableFSR3.GetValueOnAnyThread();
	CurrentFSR3StateRT = CVarEnableFSR3.GetValueOnAnyThread();
	SSRExperimentalDenoiser = 0;
	VertexDeformationOutputsVelocity = CVarVertexDeformationOutputsVelocity ? CVarVertexDeformationOutputsVelocity->GetInt() : 0;
	VelocityEnableLandscapeGrass = CVarVelocityEnableLandscapeGrass ? CVarVelocityEnableLandscapeGrass->GetInt() : 0;
	MinAutomaticViewMipBiasMin = CVarMinAutomaticViewMipBiasMin ? CVarMinAutomaticViewMipBiasMin->GetFloat() : 0;
	MinAutomaticViewMipBiasOffset = CVarMinAutomaticViewMipBiasOffset ? CVarMinAutomaticViewMipBiasOffset->GetFloat() : 0;
	SeparateTranslucency = CVarSeparateTranslucency ? CVarSeparateTranslucency->GetInt() : 0;
	SSRExperimentalDenoiser = CVarSSRExperimentalDenoiser ? CVarSSRExperimentalDenoiser->GetInt() : 0;

	IFFXFSR3TemporalUpscalingModule& FSR3ModuleInterface = FModuleManager::GetModuleChecked<IFFXFSR3TemporalUpscalingModule>(TEXT("FFXFSR3TemporalUpscaling"));
	if (FSR3ModuleInterface.GetTemporalUpscaler() == nullptr)
	{
		FFXFSR3TemporalUpscalingModule& FSR3Module = (FFXFSR3TemporalUpscalingModule&)FSR3ModuleInterface;
		TSharedPtr<FFXFSR3TemporalUpscaler, ESPMode::ThreadSafe> FSR3TemporalUpscaler = MakeShared<FFXFSR3TemporalUpscaler, ESPMode::ThreadSafe>();
		FSR3Module.SetTemporalUpscaler(FSR3TemporalUpscaler);
	}

	if (CVarEnableFSR3.GetValueOnAnyThread())
	{
		// Initialize by default for game, but not the editor unless we intend to use FSR3 in the viewport by default
		if (!GIsEditor || CVarEnableFSR3InEditor.GetValueOnAnyThread())
		{
			// Set this at startup so that it will apply consistently
			if (CVarFSR3AdjustMipBias.GetValueOnGameThread())
			{
				if (CVarMinAutomaticViewMipBiasMin != nullptr)
				{
					CVarMinAutomaticViewMipBiasMin->Set(float(0.f + log2(1.f / 3.0f) - 1.0f + FLT_EPSILON), EConsoleVariableFlags::ECVF_SetByCode);
				}
				if (CVarMinAutomaticViewMipBiasOffset != nullptr)
				{
					CVarMinAutomaticViewMipBiasOffset->Set(float(-1.0f + FLT_EPSILON), EConsoleVariableFlags::ECVF_SetByCode);
				}
			}

			if (CVarFSR3ForceVertexDeformationOutputsVelocity.GetValueOnGameThread())
			{
				if (CVarVertexDeformationOutputsVelocity != nullptr)
				{
					CVarVertexDeformationOutputsVelocity->Set(1, EConsoleVariableFlags::ECVF_SetByCode);
				}
				if (CVarVelocityEnableLandscapeGrass != nullptr)
				{
					CVarVelocityEnableLandscapeGrass->Set(1, EConsoleVariableFlags::ECVF_SetByCode);
				}
			}

			if (CVarReactiveMask && CVarReactiveMask->GetInt())
			{
				if (CVarSeparateTranslucency != nullptr)
				{
					CVarSeparateTranslucency->Set(1, EConsoleVariableFlags::ECVF_SetByCode);
				}

				if (CVarSSRExperimentalDenoiser != nullptr)
				{
					if (CVarFSR3SSRExperimentalDenoiser)
					{
						CVarFSR3SSRExperimentalDenoiser->Set(SSRExperimentalDenoiser, EConsoleVariableFlags::ECVF_SetByCode);
					}
					CVarSSRExperimentalDenoiser->Set(1, EConsoleVariableFlags::ECVF_SetByCode);
				}
			}
		}
		else
		{
			// Pretend it is disabled so that when the Editor does enable FSR3 the state change is picked up properly.
			PreviousFSR3State = false;
			PreviousFSR3StateRT = false;
			CurrentFSR3StateRT = false;
		}
	}
	else
	{
		// Disable FSR3 as it could not be initialised, this avoids errors if it is enabled later.
		PreviousFSR3State = false;
		PreviousFSR3StateRT = false;
		CurrentFSR3StateRT = false;
		CVarEnableFSR3->Set(0, EConsoleVariableFlags::ECVF_SetByGameOverride);
	}
}

void FFXFSR3ViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	if (InViewFamily.GetFeatureLevel() >= ERHIFeatureLevel::SM5)
	{
		static IConsoleVariable* CVarVertexDeformationOutputsVelocity = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Velocity.EnableVertexDeformation"));
		static IConsoleVariable* CVarVelocityEnableLandscapeGrass = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Velocity.EnableLandscapeGrass"));
		IFFXFSR3TemporalUpscalingModule& FSR3ModuleInterface = FModuleManager::GetModuleChecked<IFFXFSR3TemporalUpscalingModule>(TEXT("FFXFSR3TemporalUpscaling"));
		check(FSR3ModuleInterface.GetFSR3Upscaler());
		int32 EnableFSR3 = CVarEnableFSR3.GetValueOnAnyThread();
		FSR3ModuleInterface.GetFSR3Upscaler()->Initialize();

		if (EnableFSR3)
		{
			if (CVarFSR3ForceVertexDeformationOutputsVelocity.GetValueOnGameThread() && CVarVertexDeformationOutputsVelocity != nullptr && VertexDeformationOutputsVelocity == 0 && CVarVertexDeformationOutputsVelocity->GetInt() == 0)
			{
				VertexDeformationOutputsVelocity = CVarVertexDeformationOutputsVelocity->GetInt();
				CVarVertexDeformationOutputsVelocity->Set(1, EConsoleVariableFlags::ECVF_SetByCode);
			}

			if (CVarFSR3ForceVertexDeformationOutputsVelocity.GetValueOnGameThread() && CVarVelocityEnableLandscapeGrass != nullptr && VelocityEnableLandscapeGrass == 0 && CVarVelocityEnableLandscapeGrass->GetInt() == 0)
			{
				VelocityEnableLandscapeGrass = CVarVelocityEnableLandscapeGrass->GetInt();
				CVarVelocityEnableLandscapeGrass->Set(1, EConsoleVariableFlags::ECVF_SetByCode);
			}

			if (CVarFSR3ForceLandscapeHISMMobility.GetValueOnGameThread())
			{
				// Landscape Hierarchical Instanced Static Mesh components are usually foliage and thus might use WPO.
				// To make it generate motion vectors it can't be Static which is hard-coded into the Engine.
				for (ALandscapeProxy* Landscape : TObjectRange<ALandscapeProxy>(RF_ClassDefaultObject | RF_ArchetypeObject, true, EInternalObjectFlags::Garbage))
				{
					ForceLandscapeHISMMobility(InViewFamily, Landscape);
				}
			}
		}

		if (PreviousFSR3State != EnableFSR3)
		{
			// Update tracking of the FSR3 state when it is changed
			PreviousFSR3State = EnableFSR3;
			static IConsoleVariable* CVarMinAutomaticViewMipBiasMin = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ViewTextureMipBias.Min"));
			static IConsoleVariable* CVarMinAutomaticViewMipBiasOffset = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ViewTextureMipBias.Offset"));
			static IConsoleVariable* CVarSeparateTranslucency = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SeparateTranslucency"));
			static IConsoleVariable* CVarReactiveMask = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR3.CreateReactiveMask"));
			static IConsoleVariable* CVarSSRExperimentalDenoiser = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SSR.ExperimentalDenoiser"));
			static IConsoleVariable* CVarFSR3SSRExperimentalDenoiser = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR3.UseSSRExperimentalDenoiser"));

			if (EnableFSR3)
			{
				// When toggling reapply the settings that FSR3 wants to override
				if (CVarFSR3AdjustMipBias.GetValueOnGameThread())
				{
					if (CVarMinAutomaticViewMipBiasMin != nullptr)
					{
						MinAutomaticViewMipBiasMin = CVarMinAutomaticViewMipBiasMin->GetFloat();
						CVarMinAutomaticViewMipBiasMin->Set(float(0.f + log2(1.f / 3.0f) - 1.0f + FLT_EPSILON), EConsoleVariableFlags::ECVF_SetByCode);
					}
					if (CVarMinAutomaticViewMipBiasOffset != nullptr)
					{
						MinAutomaticViewMipBiasOffset = CVarMinAutomaticViewMipBiasOffset->GetFloat();
						CVarMinAutomaticViewMipBiasOffset->Set(float(-1.0f + FLT_EPSILON), EConsoleVariableFlags::ECVF_SetByCode);
					}
				}

				if (CVarFSR3ForceVertexDeformationOutputsVelocity.GetValueOnGameThread())
				{
					if (CVarVertexDeformationOutputsVelocity != nullptr)
					{
						CVarVertexDeformationOutputsVelocity->Set(1, EConsoleVariableFlags::ECVF_SetByCode);
					}
					if (CVarVelocityEnableLandscapeGrass != nullptr)
					{
						CVarVelocityEnableLandscapeGrass->Set(1, EConsoleVariableFlags::ECVF_SetByCode);
					}
				}

				if (CVarReactiveMask && CVarReactiveMask->GetInt())
				{
					if (CVarSeparateTranslucency != nullptr)
					{
						SeparateTranslucency = CVarSeparateTranslucency->GetInt();
						CVarSeparateTranslucency->Set(1, EConsoleVariableFlags::ECVF_SetByCode);
					}
					if (CVarSSRExperimentalDenoiser != nullptr)
					{
						SSRExperimentalDenoiser = CVarSSRExperimentalDenoiser->GetInt();
						if (CVarFSR3SSRExperimentalDenoiser != nullptr)
						{
							CVarFSR3SSRExperimentalDenoiser->Set(SSRExperimentalDenoiser, EConsoleVariableFlags::ECVF_SetByCode);
						}
						CVarSSRExperimentalDenoiser->Set(1, EConsoleVariableFlags::ECVF_SetByCode);
					}
				}
			}
			// Put the variables FSR3 modifies back to the way they were when FSR3 was toggled on.
			else
			{
				if (CVarFSR3AdjustMipBias.GetValueOnGameThread())
				{
					if (CVarMinAutomaticViewMipBiasMin != nullptr)
					{
						CVarMinAutomaticViewMipBiasMin->Set(MinAutomaticViewMipBiasMin, EConsoleVariableFlags::ECVF_SetByCode);
					}
					if (CVarMinAutomaticViewMipBiasOffset != nullptr)
					{
						CVarMinAutomaticViewMipBiasOffset->Set(MinAutomaticViewMipBiasOffset, EConsoleVariableFlags::ECVF_SetByCode);
					}
				}

				if (CVarFSR3ForceVertexDeformationOutputsVelocity.GetValueOnGameThread())
				{
					if (CVarVertexDeformationOutputsVelocity != nullptr)
					{
						CVarVertexDeformationOutputsVelocity->Set(VertexDeformationOutputsVelocity, EConsoleVariableFlags::ECVF_SetByCode);
					}
					if (CVarVelocityEnableLandscapeGrass != nullptr)
					{
						CVarVelocityEnableLandscapeGrass->Set(VelocityEnableLandscapeGrass, EConsoleVariableFlags::ECVF_SetByCode);
					}
				}

				if (CVarReactiveMask && CVarReactiveMask->GetInt())
				{
					if (CVarSeparateTranslucency != nullptr)
					{
						CVarSeparateTranslucency->Set(SeparateTranslucency, EConsoleVariableFlags::ECVF_SetByCode);
					}
					if (CVarSSRExperimentalDenoiser != nullptr)
					{
						CVarSSRExperimentalDenoiser->Set(SSRExperimentalDenoiser, EConsoleVariableFlags::ECVF_SetByCode);
					}
				}
			}
		}
	}
}

void FFXFSR3ViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	if (InViewFamily.GetFeatureLevel() >= ERHIFeatureLevel::SM5)
	{
		IFFXFSR3TemporalUpscalingModule& FSR3ModuleInterface = FModuleManager::GetModuleChecked<IFFXFSR3TemporalUpscalingModule>(TEXT("FFXFSR3TemporalUpscaling"));
		FFXFSR3TemporalUpscaler* Upscaler = FSR3ModuleInterface.GetFSR3Upscaler();
		bool IsTemporalUpscalingRequested = false;
		bool bIsGameView = !WITH_EDITOR;
		for (int i = 0; i < InViewFamily.Views.Num(); i++)
		{
			const FSceneView* InView = InViewFamily.Views[i];
			if (ensure(InView))
			{
				if (Upscaler)
				{
					FGlobalShaderMap* GlobalMap = GetGlobalShaderMap(InViewFamily.GetFeatureLevel());
					Upscaler->SetSSRShader(GlobalMap);
				}

				bIsGameView |= InView->bIsGameView;

				// Don't run FSR3 if Temporal Upscaling is unused.
				IsTemporalUpscalingRequested |= (InView->PrimaryScreenPercentageMethod == EPrimaryScreenPercentageMethod::TemporalUpscale);
			}
		}

#if WITH_EDITOR
		IsTemporalUpscalingRequested &= Upscaler->IsEnabledInEditor();
#endif

		if (IsTemporalUpscalingRequested && CVarEnableFSR3.GetValueOnAnyThread() && (InViewFamily.GetTemporalUpscalerInterface() == nullptr))
		{
			static const auto CVarFSR3EnabledInEditor = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FidelityFX.FSR3.EnabledInEditorViewport"));
			if (!WITH_EDITOR || (CVarFSR3EnabledInEditor && CVarFSR3EnabledInEditor->GetValueOnGameThread() == 1) || bIsGameView)
			{
				Upscaler->UpdateDynamicResolutionState();
				InViewFamily.SetTemporalUpscalerInterface(new FFXFSR3TemporalUpscalerProxy(Upscaler));
			}
		}
	}
}

void FFXFSR3ViewExtension::PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily)
{
	if (InViewFamily.GetFeatureLevel() >= ERHIFeatureLevel::SM5)
	{
		// When the FSR3 plugin is enabled/disabled dispose of any previous history as it will be invalid if it comes from another upscaler
		CurrentFSR3StateRT = CVarEnableFSR3.GetValueOnRenderThread();
		if (PreviousFSR3StateRT != CurrentFSR3StateRT)
		{
			// This also requires updating our tracking of the FSR3 state
			PreviousFSR3StateRT = CurrentFSR3StateRT;
#if UE_VERSION_OLDER_THAN(5, 3, 0)
			for (auto* SceneView : InViewFamily.Views)
			{
				if (SceneView->bIsViewInfo)
				{
					FViewInfo* View = (FViewInfo*)SceneView;
					View->PrevViewInfo.CustomTemporalAAHistory.SafeRelease();
					if (!View->bStatePrevViewInfoIsReadOnly && View->ViewState)
					{
						View->ViewState->PrevFrameViewInfo.CustomTemporalAAHistory.SafeRelease();
					}
				}
			}
#endif
		}
	}
}

void FFXFSR3ViewExtension::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
	// FSR3 can access the previous frame of Lumen data at this point, but not later where it will be replaced with the current frame's which won't be accessible when FSR3 runs.
	if (InView.GetFeatureLevel() >= ERHIFeatureLevel::SM5)
	{
		if (CVarEnableFSR3.GetValueOnAnyThread())
		{
			IFFXFSR3TemporalUpscalingModule& FSR3ModuleInterface = FModuleManager::GetModuleChecked<IFFXFSR3TemporalUpscalingModule>(TEXT("FFXFSR3TemporalUpscaling"));
			FSR3ModuleInterface.GetFSR3Upscaler()->SetLumenReflections(InView);
		}
	}
}

void FFXFSR3ViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs)
{
	// FSR3 requires the separate translucency data which is only available through the post-inputs so bind them to the upscaler now.
	if (View.GetFeatureLevel() >= ERHIFeatureLevel::SM5)
	{
		if (CVarEnableFSR3.GetValueOnAnyThread())
		{
			IFFXFSR3TemporalUpscalingModule& FSR3ModuleInterface = FModuleManager::GetModuleChecked<IFFXFSR3TemporalUpscalingModule>(TEXT("FFXFSR3TemporalUpscaling"));
			FSR3ModuleInterface.GetFSR3Upscaler()->SetPostProcessingInputs(Inputs);
		}
	}
}

void FFXFSR3ViewExtension::PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily)
{
	// As FSR3 retains pointers/references to objects the engine is not expecting clear them out now to prevent leaks or accessing dangling pointers.
	if (InViewFamily.GetFeatureLevel() >= ERHIFeatureLevel::SM5)
	{
		if (CVarEnableFSR3.GetValueOnAnyThread())
		{
			IFFXFSR3TemporalUpscalingModule& FSR3ModuleInterface = FModuleManager::GetModuleChecked<IFFXFSR3TemporalUpscalingModule>(TEXT("FFXFSR3TemporalUpscaling"));
			FSR3ModuleInterface.GetFSR3Upscaler()->EndOfFrame();
		}
	}
}
