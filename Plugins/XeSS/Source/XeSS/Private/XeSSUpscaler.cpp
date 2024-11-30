/*******************************************************************************
 * Copyright 2021 Intel Corporation
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

#include "XeSSUpscaler.h"

#include "Engine/GameViewportClient.h"

#if XESS_ENGINE_VERSION_LSS(5, 1)
#include "LegacyScreenPercentageDriver.h"
#endif // XESS_ENGINE_VERSION_LSS(5, 1)

#include "ScenePrivate.h" // for FSceneViewState, FTemporalAAHistory
#include "XeSSHistory.h"
#include "XeSSPrePass.h"
#include "XeSSRHI.h"
#include "XeSSSettings.h"
#include "XeSSUnreal.h"
#include "XeSSUtil.h"

#if XESS_ENGINE_VERSION_GEQ(5, 3)
using namespace UE::Renderer::Private;
#endif // XESS_ENGINE_VERSION_GEQ(5, 3)

#define LOCTEXT_NAMESPACE "FXeSSPlugin"

// It SHOULD be enough, for 0.001% * 7860 (8K) = 0.0786 (pixel)
constexpr float SCREEN_PERCENTAGE_ERROR_TOLERANCE = 0.001f;
// HACK: Variables to save previous global ones
#if XESS_ENGINE_VERSION_LSS(5, 1)
ICustomStaticScreenPercentage* PreviousGCustomStaticScreenPercentage = nullptr;

#if ENGINE_MAJOR_VERSION < 5
const ITemporalUpscaler* PreviousGTemporalUpscaler = nullptr;
#endif // ENGINE_MAJOR_VERSION < 5

#endif // XESS_ENGINE_VERSION_LSS(5, 1)

static TAutoConsoleVariable<int32> CVarXeSSEnabled(
	TEXT("r.XeSS.Enabled"),
	0,
	TEXT("[default: 0] Set to 1 to use XeSS instead of TAAU or any other upscaling method."),
	ECVF_Default | ECVF_RenderThreadSafe);

// QUALITY EDIT:
static TAutoConsoleVariable<int32> CVarXeSSQuality(
	TEXT("r.XeSS.Quality"),
	2,
	TEXT("[default: 2] Set XeSS quality setting.")
	TEXT(" 0: Ultra Performance")
	TEXT(" 1: Performance")
	TEXT(" 2: Balanced")
	TEXT(" 3: Quality")
	TEXT(" 4: Ultra Quality")
	TEXT(" 5: Ultra Quality Plus")
	TEXT(" 6: Anti-Aliasing"),
	ECVF_Default | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarXeSSPreExposure(
	TEXT("r.XeSS.Experimental.PreExposure"),
	1,
	TEXT("[default: 1] Whether to enable pre-exposure. It just unifies commands across different Unreal versions."),
	ECVF_Default);

DECLARE_GPU_STAT_NAMED(XeSS, TEXT("XeSS"));

FXeSSPassParameters::FXeSSPassParameters(const FViewInfo& View, const XPassInputs& PassInputs)
	: InputViewRect(View.ViewRect)
	, OutputViewRect(FIntPoint::ZeroValue, View.GetSecondaryViewRectSize())
#if XESS_ENGINE_VERSION_GEQ(5, 3)
	, SceneColorTexture(PassInputs.SceneColor.Texture)
	, SceneDepthTexture(PassInputs.SceneDepth.Texture)
#else // XESS_ENGINE_VERSION_GEQ(5, 3)
	, SceneColorTexture(PassInputs.SceneColorTexture)
	, SceneDepthTexture(PassInputs.SceneDepthTexture)
#endif // XESS_ENGINE_VERSION_GEQ(5, 3)
{ }

FIntPoint FXeSSPassParameters::GetOutputExtent() const
{
	check(Validate());
	check(SceneColorTexture);

	FIntPoint InputExtent = SceneColorTexture->Desc.Extent;

	check(OutputViewRect.Min == FIntPoint::ZeroValue);
	FIntPoint QuantizedPrimaryUpscaleViewSize;
	QuantizeSceneBufferSize(OutputViewRect.Size(), QuantizedPrimaryUpscaleViewSize);

	return FIntPoint(
		FMath::Max(InputExtent.X, QuantizedPrimaryUpscaleViewSize.X),
		FMath::Max(InputExtent.Y, QuantizedPrimaryUpscaleViewSize.Y));
}

bool FXeSSPassParameters::Validate() const
{
	check(OutputViewRect.Min == FIntPoint::ZeroValue);
	return true;
}

const TCHAR* FXeSSUpscaler::GetDebugName() const
{
	return TEXT("FXeSSUpscaler");
}

bool FXeSSUpscaler::IsXeSSEnabled() const
{
	static const auto TAAUpscalerEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("r.TemporalAA.Upscaler"))->GetInt();

#if ENGINE_MAJOR_VERSION < 5
	return (TAAUpscalerEnabled != 0) && (CVarXeSSEnabled.GetValueOnAnyThread() != 0) && (GTemporalUpscaler == this);
#else
	return (TAAUpscalerEnabled != 0) && (CVarXeSSEnabled.GetValueOnAnyThread() != 0);
#endif
}

#if XESS_ENGINE_VERSION_LSS(5, 1)
// HACK: assignment of GTemporalUpscaler and GCustomStaticScreenPercentage moved from StartupModule()
void FXeSSUpscaler::HandleXeSSEnabledSet(IConsoleVariable* Variable)
{
	// Return if no change as bool
	if (bCurrentXeSSEnabled == Variable->GetBool())
	{
		return;
	}
	bCurrentXeSSEnabled = Variable->GetBool();
	if (!bCurrentXeSSEnabled)
	{
		GCustomStaticScreenPercentage = PreviousGCustomStaticScreenPercentage;

#if ENGINE_MAJOR_VERSION < 5
		GTemporalUpscaler = PreviousGTemporalUpscaler;
#endif // ENGINE_MAJOR_VERSION < 5
	}
	else
	{
		PreviousGCustomStaticScreenPercentage = GCustomStaticScreenPercentage;
		GCustomStaticScreenPercentage = this;

#if ENGINE_MAJOR_VERSION < 5
		PreviousGTemporalUpscaler = GTemporalUpscaler;
		GTemporalUpscaler = this;
#endif // ENGINE_MAJOR_VERSION < 5
	}
}
#endif // XESS_ENGINE_VERSION_LSS(5, 1)

BEGIN_SHADER_PARAMETER_STRUCT(FXeSSShaderParameters, )
	// Init parameters
	SHADER_PARAMETER(FIntRect, OutputRect)

	// Exec parameters
	RDG_TEXTURE_ACCESS(InputColor, ERHIAccess::SRVCompute)
	RDG_TEXTURE_ACCESS(InputVelocity, ERHIAccess::SRVCompute)

	// Only used as WA to force Resource Transition Barrier
	RDG_BUFFER_ACCESS(DummyBuffer, ERHIAccess::UAVCompute)

	SHADER_PARAMETER(FVector2f, JitterOffset)
	SHADER_PARAMETER(uint32, bCameraCut)
	SHADER_PARAMETER(int32, QualitySetting)
	SHADER_PARAMETER(uint32, InitFlags)
	// Output
	RDG_TEXTURE_ACCESS_DYNAMIC(SceneColorOutput)
END_SHADER_PARAMETER_STRUCT()

FRDGTextureRef FXeSSUpscaler::AddMainXeSSPass(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	const FXeSSPassParameters& Inputs,
	const FTemporalAAHistory& InputHistory,
	FTemporalAAHistory* OutputHistory) const
{
	check(Inputs.SceneColorTexture);
	check(Inputs.SceneDepthTexture);
	check(Inputs.SceneVelocityTexture);

	// HACK: exit if XeSS upscaler is not active, allows to have multiple upscalers loaded by project
	// TODO: check if it could be replaced with "check"
	if (!IsXeSSEnabled())
	{
		return nullptr;
	}

	RDG_GPU_STAT_SCOPE(GraphBuilder, XeSS);

	FXeSSRHI* LocalXeSSRHI = this->UpscalerXeSSRHI;
	// Whether to use camera cut shader permutation or not.
	const bool bCameraCut = !InputHistory.IsValid() || View.bCameraCut;

	const FIntPoint OutputExtent = Inputs.GetOutputExtent();
	const FIntRect SrcRect = Inputs.InputViewRect;
	const FIntRect DestRect = Inputs.OutputViewRect;

	// Create outputs
	FRDGTextureDesc OutputColorDesc = Inputs.SceneColorTexture->Desc;
	OutputColorDesc.Extent = OutputExtent;
	OutputColorDesc.Flags = TexCreate_ShaderResource | TexCreate_UAV;

	FRDGTexture* OutputSceneColor = GraphBuilder.CreateTexture(
		OutputColorDesc,
		TEXT("XeSSOutputSceneColor"),
		ERDGTextureFlags::MultiFrame);

	FXeSSShaderParameters* PassParameters = GraphBuilder.AllocParameters<FXeSSShaderParameters>();
	PassParameters->OutputRect = DestRect;

	PassParameters->InputColor = Inputs.SceneColorTexture;
	PassParameters->InputVelocity = Inputs.SceneVelocityTexture;

	PassParameters->JitterOffset = FVector2f(View.TemporalJitterPixels);
	PassParameters->bCameraCut = bCameraCut;
	PassParameters->SceneColorOutput = FRDGTextureAccess(OutputSceneColor, ERHIAccess::UAVCompute);
	PassParameters->QualitySetting = CVarXeSSQuality.GetValueOnAnyThread();
	PassParameters->InitFlags = LocalXeSSRHI->GetXeSSInitFlags();
	PassParameters->DummyBuffer = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateStructuredDesc((uint32)sizeof(float), 1u), TEXT("ForceTransitionDummyBuffer"));

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("XeSS Main Pass"),
		PassParameters,
		ERDGPassFlags::Compute,
		[LocalXeSSRHI, PassParameters, Inputs](FRHICommandListImmediate& RHICmdList)
		{
			FXeSSInitArguments InitArgsXeSS;

			InitArgsXeSS.OutputWidth = PassParameters->OutputRect.Width();
			InitArgsXeSS.OutputHeight = PassParameters->OutputRect.Height();
			InitArgsXeSS.QualitySetting = PassParameters->QualitySetting;
			InitArgsXeSS.InitFlags = PassParameters->InitFlags;

			FXeSSExecuteArguments ExecArgsXeSS;
			check(PassParameters->InputColor);
			PassParameters->InputColor->MarkResourceAsUsed();
			ExecArgsXeSS.ColorTexture = PassParameters->InputColor->GetRHI();

			check(PassParameters->InputVelocity);
			PassParameters->InputVelocity->MarkResourceAsUsed();
			ExecArgsXeSS.VelocityTexture = PassParameters->InputVelocity->GetRHI();

			check(PassParameters->SceneColorOutput);
			PassParameters->SceneColorOutput->MarkResourceAsUsed();
			ExecArgsXeSS.OutputTexture = PassParameters->SceneColorOutput->GetRHI();

			ExecArgsXeSS.JitterOffsetX = PassParameters->JitterOffset.X;
			ExecArgsXeSS.JitterOffsetY = PassParameters->JitterOffset.Y;

			ExecArgsXeSS.bCameraCut = PassParameters->bCameraCut;

			ExecArgsXeSS.SrcViewRect = Inputs.InputViewRect;
			ExecArgsXeSS.DstViewRect = Inputs.OutputViewRect;

			check(PassParameters->DummyBuffer);
			PassParameters->DummyBuffer->MarkResourceAsUsed();

			if (LocalXeSSRHI->EffectRecreationIsRequired(InitArgsXeSS))
			{
				// Invalidate history if XeSS is re-initialized
				ExecArgsXeSS.bCameraCut = 1;
				// Make sure all cmd lists in flight have complete before XeSS re-initialized
				RHICmdList.BlockUntilGPUIdle();
				RHICmdList.EnqueueLambda(
					[LocalXeSSRHI, InitArgsXeSS](FRHICommandListImmediate& Cmd)
					{
						LocalXeSSRHI->RHIInitializeXeSS(InitArgsXeSS);
					});
			}

			// Make sure all resource transitions barriers are executed before RHIExecuteXeSS is called
			LocalXeSSRHI->TriggerResourceTransitions(RHICmdList, PassParameters->DummyBuffer);
			RHICmdList.EnqueueLambda(
				[LocalXeSSRHI, ExecArgsXeSS](FRHICommandListImmediate& Cmd)
				{
					LocalXeSSRHI->RHIExecuteXeSS(Cmd,ExecArgsXeSS);
				});
		});

	if (!View.bStatePrevViewInfoIsReadOnly)
	{
		OutputHistory->SafeRelease();

		GraphBuilder.QueueTextureExtraction(OutputSceneColor, &OutputHistory->RT[0]);
		OutputHistory->ViewportRect = DestRect;
		OutputHistory->ReferenceBufferSize = OutputExtent;
	}

	return OutputSceneColor;
};

#if XESS_ENGINE_VERSION_GEQ(5, 3)
ITemporalUpscaler::FOutputs FXeSSUpscaler::AddPasses(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const FInputs& PassInputs) const
#elif XESS_ENGINE_VERSION_GEQ(5, 0)
ITemporalUpscaler::FOutputs FXeSSUpscaler::AddPasses(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& ViewInfo,
	const FPassInputs& PassInputs) const
#else // XESS_ENGINE_VERSION_GEQ(5, 0)
void FXeSSUpscaler::AddPasses(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& ViewInfo,
	const FPassInputs& PassInputs,
	FRDGTextureRef* OutSceneColorTexture,
	FIntRect* OutSceneColorViewRect,
	FRDGTextureRef* OutSceneColorHalfResTexture,
	FIntRect* OutSceneColorHalfResViewRect) const
#endif // XESS_ENGINE_VERSION_GEQ(5, 3)
{
	RDG_EVENT_SCOPE(GraphBuilder, "XeSS Pass");
	FRDGTextureRef SceneVelocityTexture;

#if XESS_ENGINE_VERSION_GEQ(5, 3)
	ITemporalUpscaler::FOutputs Outputs{};

	check(View.bIsViewInfo);
	check(IsXeSSEnabled());

	const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(View);

	SceneVelocityTexture = PassInputs.SceneVelocity.Texture;
#else // XESS_ENGINE_VERSION_GEQ(5, 3)
	SceneVelocityTexture = PassInputs.SceneVelocityTexture;

	#if XESS_ENGINE_VERSION_GEQ(5, 0)
	ITemporalUpscaler::FOutputs Outputs{};

	if (!IsXeSSEnabled())
	{
		return Outputs;
	}
	#else // XESS_ENGINE_VERSION_GEQ(5, 0)
	// HACK: exit if XeSS upscaler is not active, allows to have multiple upscalers loaded by project
	if (!IsXeSSEnabled())
	{
		return;
	}
	#endif // // XESS_ENGINE_VERSION_GEQ(5, 0)

#endif // XESS_ENGINE_VERSION_GEQ(5, 3)

	FXeSSPassParameters XeSSMainParameters(ViewInfo, PassInputs);
	XeSSMainParameters.SceneVelocityTexture = AddVelocityFlatteningXeSSPass(GraphBuilder,
		XeSSMainParameters.SceneDepthTexture,
		SceneVelocityTexture,
		ViewInfo);

	const FTemporalAAHistory& InputHistory = ViewInfo.PrevViewInfo.TemporalAAHistory;
	FTemporalAAHistory& OutputHistory = ViewInfo.ViewState->PrevFrameViewInfo.TemporalAAHistory;

	const FRDGTextureRef XeSSOutput = AddMainXeSSPass(
		GraphBuilder,
		ViewInfo,
		XeSSMainParameters,
		InputHistory,
		&OutputHistory);

#if XESS_ENGINE_VERSION_GEQ(5, 0)
	if (!XeSSOutput)
	{
		return Outputs;
	}
	Outputs.FullRes.Texture = XeSSOutput;
	Outputs.FullRes.ViewRect = XeSSMainParameters.OutputViewRect;
	#if XESS_ENGINE_VERSION_GEQ(5, 3)
	Outputs.NewHistory = DummyHistory;
	#endif // XESS_ENGINE_VERSION_GEQ(5, 3)
	return Outputs;
#else // XESS_ENGINE_VERSION_GEQ(5, 0)
	// HACK: Fix crash issue when activated with other upscaler plugins at the same time
	if (!XeSSOutput)
	{
		return;
	}
	*OutSceneColorTexture = XeSSOutput;
	*OutSceneColorViewRect = XeSSMainParameters.OutputViewRect;
#endif // XESS_ENGINE_VERSION_GEQ(5, 0)
}

#if XESS_ENGINE_VERSION_LSS(5, 1)
// Inherited via ICustomStaticScreenPercentage
void FXeSSUpscaler::SetupMainGameViewFamily(FSceneViewFamily& ViewFamily)
{
	if (!IsXeSSEnabled())
	{
		return;
	}
#if ENGINE_MAJOR_VERSION < 5
	checkf(GTemporalUpscaler == this, TEXT("GTemporalUpscaler is not set to a XeSS, please make sure no other upscaling plug is enabled."));
#endif // ENGINE_MAJOR_VERSION < 5
	checkf(GCustomStaticScreenPercentage == this, TEXT("GCustomStaticScreenPercentage is not set to a XeSS, please make sure no other upscaling plug is enabled."));

	if (!GIsEditor || GetDefault<UXeSSSettings>()->bEnableXeSSInEditorViewports)
	{
		ViewFamily.SetTemporalUpscalerInterface(this);

		if (ViewFamily.EngineShowFlags.ScreenPercentage && !ViewFamily.GetScreenPercentageInterface())
		{
			const float ResolutionFraction = UpscalerXeSSRHI->GetOptimalResolutionFraction();
			ViewFamily.SetScreenPercentageInterface(new FLegacyScreenPercentageDriver(
				ViewFamily, ResolutionFraction
#if ENGINE_MAJOR_VERSION < 5 
				, /*InAllowPostProcessSettingsScreenPercentage*/ false
#endif // ENGINE_MAJOR_VERSION < 5
			));
		}
	}
}

#if XESS_ENGINE_VERSION_GEQ(4, 27)
void FXeSSUpscaler::SetupViewFamily(FSceneViewFamily& ViewFamily, TSharedPtr<ICustomStaticScreenPercentageData> InScreenPercentageDataInterface)
{
	check(InScreenPercentageDataInterface.IsValid());
	if (!IsXeSSEnabled())
	{
		return;
	}
#if ENGINE_MAJOR_VERSION < 5 
	checkf(GTemporalUpscaler == this, TEXT("GTemporalUpscaler is not set to a XeSS, please make sure no other upscaling plug is enabled."));
#endif // ENGINE_MAJOR_VERSION < 5
	checkf(GCustomStaticScreenPercentage == this, TEXT("GCustomStaticScreenPercentage is not set to a XeSS, please make sure no other upscaling plug is enabled."));

	ViewFamily.SetTemporalUpscalerInterface(this);

	if (ViewFamily.EngineShowFlags.ScreenPercentage && !ViewFamily.GetScreenPercentageInterface())
	{
		const float ResolutionFraction = UpscalerXeSSRHI->GetOptimalResolutionFraction();
		ViewFamily.SetScreenPercentageInterface(new FLegacyScreenPercentageDriver(
			ViewFamily, ResolutionFraction
#if ENGINE_MAJOR_VERSION < 5
			, /*InAllowPostProcessSettingsScreenPercentage*/ false
#endif ENGINE_MAJOR_VERSION < 5
		));
	}
}
#endif // XESS_ENGINE_VERSION_GEQ(4, 27)

#endif // XESS_ENGINE_VERSION_LSS(5, 1)

float FXeSSUpscaler::GetMinUpsampleResolutionFraction() const
{
	return UpscalerXeSSRHI->GetMinSupportedResolutionFraction();
}

float FXeSSUpscaler::GetMaxUpsampleResolutionFraction() const
{
	return UpscalerXeSSRHI->GetMaxSupportedResolutionFraction();
}

#if XESS_ENGINE_VERSION_GEQ(5, 1)
ITemporalUpscaler* FXeSSUpscaler::Fork_GameThread(const FSceneViewFamily& ViewFamily) const
{
	return new FXeSSUpscaler(FXeSSUpscaler::UpscalerXeSSRHI);
}
void FXeSSUpscaler::SetupViewFamily(FSceneViewFamily& ViewFamily)
{
	ViewFamily.SetTemporalUpscalerInterface(new FXeSSUpscaler(UpscalerXeSSRHI));
}
#endif // XESS_ENGINE_VERSION_GEQ(5, 1)

FXeSSRHI* FXeSSUpscaler::UpscalerXeSSRHI;

FXeSSUpscaler::FXeSSUpscaler(FXeSSRHI* InXeSSRHI)
{
	UpscalerXeSSRHI = InXeSSRHI;

#if XESS_ENGINE_VERSION_GEQ(5, 3)
	DummyHistory = new FXeSSHistory(this);
#endif // XESS_ENGINE_VERSION_GEQ(5, 3)

#if XESS_ENGINE_VERSION_LSS(5, 1)
	// Handle value set by ini file
	HandleXeSSEnabledSet(CVarXeSSEnabled->AsVariable());
	// NOTE: OnChangedCallback will always be called when set even if the value is not changed 
	CVarXeSSEnabled->AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateRaw(this, &FXeSSUpscaler::HandleXeSSEnabledSet));
#endif // XESS_ENGINE_VERSION_LSS(5, 1)

	CVarXeSSPreExposure->AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateLambda([this](IConsoleVariable* InVariable)
	{
		auto CVarEyeAdaptationPreExposureOverride = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EyeAdaptation.PreExposureOverride"));

		if (InVariable->GetBool())
		{
			CVarEyeAdaptationPreExposureOverride->Set(0.f);
		}
		else
		{
			CVarEyeAdaptationPreExposureOverride->Set(1.f);
		}
#if ENGINE_MAJOR_VERSION == 4
		auto CVarUsePreExposure = IConsoleManager::Get().FindConsoleVariable(TEXT("r.UsePreExposure"));
		if (InVariable->GetBool())
		{
			CVarUsePreExposure->Set(1);
		}
		else
		{
			CVarUsePreExposure->Set(0);
		}
#endif // ENGINE_MAJOR_VERSION == 4
	}));

}

FXeSSUpscaler::~FXeSSUpscaler()
{
}

#if XESS_ENGINE_VERSION_GEQ(5, 1)
bool FXeSSUpscalerViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	static const auto ScreenPercentage = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
	bool IsXeSSEnabled = XeSSUpscaler->IsXeSSEnabled();

	if (Context.Viewport == nullptr || !GEngine)
	{
		return false;
	}
#if WITH_EDITOR
	if (GIsEditor) 
	{
		if (!IsXeSSEnabled || !GetDefault<UXeSSSettings>()->bEnableXeSSInEditorViewports)
		{
			return false;
		}
		if (Context.Viewport->IsPlayInEditorViewport())
		{
			if (!GIsPlayInEditorWorld) 
			{
				return false;
			}
			return true;
		}
		else 
		{
			// Editor viewports not supported right now
			return false;
		}
	}
	else 
#endif // WITH_EDITOR
	{
		// Game viewport
		if (Context.Viewport->GetClient() == GEngine->GameViewport)
		{
			if (IsXeSSEnabled)
			{
				float MinUpsampleScreenPercentage = XeSSUpscaler->GetMinUpsampleResolutionFraction() * 100.f;
				float MaxUpsampleScreenPercentage = XeSSUpscaler->GetMaxUpsampleResolutionFraction() * 100.f;
				float CurrentScreenPercentage = ScreenPercentage->GetFloat();

				if (CurrentScreenPercentage >= MinUpsampleScreenPercentage &&
					CurrentScreenPercentage <= MaxUpsampleScreenPercentage ||
					FMath::IsNearlyEqual(CurrentScreenPercentage, MinUpsampleScreenPercentage, SCREEN_PERCENTAGE_ERROR_TOLERANCE) ||
					FMath::IsNearlyEqual(CurrentScreenPercentage, MaxUpsampleScreenPercentage, SCREEN_PERCENTAGE_ERROR_TOLERANCE))
				{
					XeSSUtil::RemoveMessageFromScreen(XeSSUtil::ON_SCREEN_MESSAGE_KEY_INCORRECT_SCREEN_PERCENTAGE);
					return true;
				}
				else
				{
					XeSSUtil::AddErrorMessageToScreen(
						FString::Printf(TEXT("XeSS is off due to invalid screen percentage, supported range: %.3f - %.3f"), 
							MinUpsampleScreenPercentage, MaxUpsampleScreenPercentage),
						XeSSUtil::ON_SCREEN_MESSAGE_KEY_INCORRECT_SCREEN_PERCENTAGE
					);
					return false;
				}
			}
		}
		return false;
	}
}

void FXeSSUpscalerViewExtension::BeginRenderViewFamily(FSceneViewFamily& ViewFamily)
{
	if (!ViewFamily.bRealtimeUpdate ||
		!ViewFamily.EngineShowFlags.AntiAliasing ||
		!ViewFamily.EngineShowFlags.ScreenPercentage ||
		ViewFamily.ViewMode != EViewModeIndex::VMI_Lit)
	{
		return;
	}
	if (!ViewFamily.GetTemporalUpscalerInterface())
	{
		check(XeSSUpscaler);
		XeSSUpscaler->SetupViewFamily(ViewFamily);
	}
}
#endif // XESS_ENGINE_VERSION_GEQ(5, 1)

#undef LOCTEXT_NAMESPACE
