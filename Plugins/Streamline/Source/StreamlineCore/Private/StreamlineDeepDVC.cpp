/*
* Copyright (c) 2022 - 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
*
* NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
* property and proprietary rights in and to this material, related
* documentation and any modifications thereto. Any use, reproduction,
* disclosure or distribution of this material and related documentation
* without an express license agreement from NVIDIA CORPORATION or
* its affiliates is strictly prohibited.
*/
#include "StreamlineDeepDVC.h"
#include "StreamlineCore.h"
#include "StreamlineCorePrivate.h"
#include "StreamlineAPI.h"
#include "StreamlineRHI.h"
#include "sl_helpers.h"
#include "sl_deepdvc.h"
#include "UIHintExtractionPass.h"

#include "CoreMinimal.h"
#include "Framework/Application/SlateApplication.h"
#include "RenderGraphBuilder.h"
#include "Runtime/Launch/Resources/Version.h"
#include "ScenePrivate.h"
#include "SystemTextures.h"
#include "HAL/PlatformApplicationMisc.h"

static TAutoConsoleVariable<int32> CVarStreamlineDeepDVCEnable(
	TEXT("r.Streamline.DeepDVC.Enable"),
	0,
	TEXT("DeepDVC mode (default = 0)\n")
	TEXT("0: off\n")
	TEXT("1: always on\n"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarStreamlineDeepDVCIntensity(
	TEXT("r.Streamline.DeepDVC.Intensity"),
	0.5,
	TEXT("DeepDVC Intensity (default = 0.5, range [0..1])\n")
	TEXT("Controls how strong or subtle the filter effect will be on an image.\n")
	TEXT("A low intensity will keep the images closer to the original, while a high intensity will make the filter effect more pronounced.\n")
	TEXT("Note: '0' disables DeepDVC implicitely\n"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarStreamlineDeepDVCSaturationBoost(
	TEXT("r.Streamline.DeepDVC.SaturationBoost"),
	0.5,
	TEXT("DeepDVC SaturationBoost(default = 0.5) [0..1]\n")
	TEXT("Enhances the colors in them image, making them more vibrant and eye-catching.\n")
	TEXT("This setting will only be active if r.Streamline.DeepDVC.Intensity is relatively high. Once active, colors pop up more, making the image look more lively.\n")
	TEXT("Note: Applied only when r.Streamline.DeepDVC.Intensity > 0\n"),
	ECVF_Default);

static EStreamlineFeatureSupport GStreamlineDeepDVCSupport = EStreamlineFeatureSupport::NotSupported;

namespace
{
	float GLastDeepDVCVRAMEstimate = 0;
}

STREAMLINECORE_API EStreamlineFeatureSupport QueryStreamlineDeepDVCSupport()
{
	static bool bStreamlineDeepDVCSupportedInitialized = false;

	if (!bStreamlineDeepDVCSupportedInitialized)
	{
		if (!FApp::CanEverRender( ))
		{
			GStreamlineDeepDVCSupport = EStreamlineFeatureSupport::NotSupported;
		}
		else if (!IsRHIDeviceNVIDIA())
		{
			GStreamlineDeepDVCSupport = EStreamlineFeatureSupport::NotSupportedIncompatibleHardware;
		}
		else if(!IsStreamlineSupported())
		{
			GStreamlineDeepDVCSupport = EStreamlineFeatureSupport::NotSupported;
		}
		else
		{
			FStreamlineRHI* StreamlineRHI = GetPlatformStreamlineRHI();
			if (StreamlineRHI->IsDeepDVCSupportedByRHI())
			{
				const sl::Feature Feature = sl::kFeatureDeepDVC;
				sl::Result SupportedResult = SLisFeatureSupported(Feature, *StreamlineRHI->GetAdapterInfo());
				LogStreamlineFeatureSupport(Feature, *StreamlineRHI->GetAdapterInfo());

				GStreamlineDeepDVCSupport = TranslateStreamlineResult(SupportedResult);

			}
			else
			{
				GStreamlineDeepDVCSupport = EStreamlineFeatureSupport::NotSupportedIncompatibleRHI;
			}
		}

		// setting this to true here so we don't recurse when we call GetDeepDVCStatusFromStreamline, which calls us
		bStreamlineDeepDVCSupportedInitialized = true;

		if (EStreamlineFeatureSupport::Supported == GStreamlineDeepDVCSupport)
		{
			// to get the min suppported width/height
			GetDeepDVCStatusFromStreamline();
		}
	}

	return GStreamlineDeepDVCSupport;
}

bool IsStreamlineDeepDVCSupported()
{
	return EStreamlineFeatureSupport::Supported == QueryStreamlineDeepDVCSupport();
}

static sl::DeepDVCMode SLDeepDVCModeFromCvar()
{
	int32 DeepDVCMode = CVarStreamlineDeepDVCEnable.GetValueOnAnyThread();
	switch (DeepDVCMode)
	{
	case 0:
		return sl::DeepDVCMode::eOff;
	case 1:
		return sl::DeepDVCMode::eOn;
	default:
		UE_LOG(LogStreamline, Error, TEXT("Invalid r.Streamline.DeepDVC.Enable value %d"), DeepDVCMode);
		return sl::DeepDVCMode::eOff;
	}
}

bool IsDeepDVCActive()
{
	if (!IsStreamlineDeepDVCSupported())
	{
		return false;
	}
	else
	{
		return SLDeepDVCModeFromCvar() != sl::DeepDVCMode::eOff ? true : false;
	}
}

float SLDeepDVCIntensityFromCvar()
{
	return  CVarStreamlineDeepDVCIntensity.GetValueOnAnyThread();
}
float SLDeepDVCSaturationBoostFromCvar()
{
	return  CVarStreamlineDeepDVCSaturationBoost.GetValueOnAnyThread();
}

DECLARE_STATS_GROUP(TEXT("DeepDVC"), STATGROUP_DeepDVC, STATCAT_Advanced);
DECLARE_FLOAT_COUNTER_STAT(TEXT("DeepDVC: VRAM Estimate (MiB)"), STAT_DeepDVCVRAMEstimate, STATGROUP_DeepDVC);

void GetDeepDVCStatusFromStreamline()
{
	GLastDeepDVCVRAMEstimate = 0;

	if (IsStreamlineDeepDVCSupported())
	{
		// INSERT AWKWARD MUPPET FACE HERE

	//	static const auto CVarStreamlineViewIdOverride = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.ViewIdOverride"));
		//checkf(CVarStreamlineViewIdOverride && CVarStreamlineViewIdOverride->GetInt() != 0, TEXT("r.Streamline.ViewIdOverride must be set to 1 since DeepDVC only supports a single viewport."));

		sl::ViewportHandle Viewport(0);

		sl::DeepDVCState State;

		sl::DeepDVCOptions StreamlineConstantsDeepDVC;

		StreamlineConstantsDeepDVC.mode = SLDeepDVCModeFromCvar();

		CALL_SL_FEATURE_FN(sl::kFeatureDeepDVC, slDeepDVCGetState, Viewport, State);

		GLastDeepDVCVRAMEstimate = float(State.estimatedVRAMUsageInBytes) / (1024 * 1024);
		SET_FLOAT_STAT(STAT_DeepDVCVRAMEstimate, GLastDeepDVCVRAMEstimate);
	}

}

namespace
{

void SetStreamlineDeepDVCState(FRHICommandListImmediate& RHICmdList, uint32 ViewID, const FIntRect& SecondaryViewRect, float Intensity, float SaturationBoost)
{
	Intensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
	SaturationBoost = FMath::Clamp(SaturationBoost, 0.0f, 1.0f);
	if (IsStreamlineDeepDVCSupported())
	{
		sl::DeepDVCMode DeepDVCMode = SLDeepDVCModeFromCvar();
		RHICmdList.EnqueueLambda(
			[ViewID, DeepDVCMode, Intensity, SaturationBoost](FRHICommandListImmediate& Cmd) mutable
			{
				sl::DeepDVCOptions StreamlineConstantsDeepDVC;
				StreamlineConstantsDeepDVC.mode = DeepDVCMode;
				StreamlineConstantsDeepDVC.intensity = Intensity;
				StreamlineConstantsDeepDVC.saturationBoost = SaturationBoost;
				CALL_SL_FEATURE_FN(sl::kFeatureDeepDVC, slDeepDVCSetOptions, sl::ViewportHandle(ViewID), StreamlineConstantsDeepDVC);
			});
	}
}

BEGIN_SHADER_PARAMETER_STRUCT(FSLDeepDVCShaderParameters, )
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorWithoutHUD)
// Fake output to trigger pass execution
#if (ENGINE_MAJOR_VERSION == 4) && (ENGINE_MINOR_VERSION == 25)
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorAfterTonemap)
#else
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, RenderPassTriggerDummy)
#endif
END_SHADER_PARAMETER_STRUCT()
}


void AddStreamlineDeepDVCStateRenderPass(FRDGBuilder& GraphBuilder, uint32 ViewID, const FIntRect& SecondaryViewRect, float Intensity, float SaturationBoost)
{

	FSLDeepDVCShaderParameters* PassParameters = GraphBuilder.AllocParameters<FSLDeepDVCShaderParameters>();

#if (ENGINE_MAJOR_VERSION > 4) || (ENGINE_MINOR_VERSION > 26)
	PassParameters->RenderPassTriggerDummy = GSystemTextures.GetBlackDummy(GraphBuilder);
#else
	PassParameters->RenderPassTriggerDummy = GraphBuilder.RegisterExternalTexture(GSystemTextures.BlackDummy);
#endif

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("Streamline DeepDVC ViewID=%u", ViewID),
		PassParameters,
#if (ENGINE_MAJOR_VERSION == 4) && (ENGINE_MINOR_VERSION == 25)
		ERDGPassFlags::Compute,
#else
		ERDGPassFlags::Compute | ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass | ERDGPassFlags::NeverCull,
#endif
		[PassParameters, ViewID, SecondaryViewRect, Intensity, SaturationBoost](FRHICommandListImmediate& RHICmdList) mutable
		{
			// output dummy to trigger RDG pass exeution
			check(PassParameters->RenderPassTriggerDummy);
			PassParameters->RenderPassTriggerDummy->MarkResourceAsUsed();


			RHICmdList.EnqueueLambda(
				[ViewID, SecondaryViewRect, Intensity, SaturationBoost](FRHICommandListImmediate& Cmd) mutable
				{
					SetStreamlineDeepDVCState(Cmd, ViewID, SecondaryViewRect, Intensity, SaturationBoost);
				});
		});
}

void AddStreamlineDeepDVCEvaluateRenderPass(FStreamlineRHI* StreamlineRHIExtensions, FRDGBuilder& GraphBuilder, uint32 ViewID, const FIntRect& SecondaryViewRect, FRDGTextureRef SLSceneColorWithoutHUD)
{

	FSLDeepDVCShaderParameters* PassParameters = GraphBuilder.AllocParameters<FSLDeepDVCShaderParameters>();
	PassParameters->SceneColorWithoutHUD = SLSceneColorWithoutHUD;

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("Streamline DeepDVC Evaluate ViewID=%u", ViewID),
		PassParameters,
#if (ENGINE_MAJOR_VERSION == 4) && (ENGINE_MINOR_VERSION == 25)
		ERDGPassFlags::Compute,
#else
		ERDGPassFlags::Compute | ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass | ERDGPassFlags::NeverCull,
#endif
		[StreamlineRHIExtensions, PassParameters, ViewID, SecondaryViewRect](FRHICommandListImmediate& RHICmdList) mutable
		{
			check(PassParameters->SceneColorWithoutHUD);
			PassParameters->SceneColorWithoutHUD->MarkResourceAsUsed();
			FRHITexture* DeepDVCInputOutput = PassParameters->SceneColorWithoutHUD->GetRHI();
			RHICmdList.EnqueueLambda(
				[StreamlineRHIExtensions, DeepDVCInputOutput, ViewID, SecondaryViewRect](FRHICommandListImmediate& Cmd) mutable
				{
					sl::FrameToken* FrameToken = FStreamlineCoreModule::GetStreamlineRHI()->GetFrameToken(GFrameCounter);

					FRHIStreamlineResource DeeDVCResource{ DeepDVCInputOutput , SecondaryViewRect, EStreamlineResource::ScalingOutputColor};
					StreamlineRHIExtensions->StreamlineEvaluateDeepDVC(Cmd, DeeDVCResource, FrameToken, ViewID);
				});
		});
}

