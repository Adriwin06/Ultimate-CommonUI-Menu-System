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
#endif

#include "ScenePrivate.h" // for FSceneViewState, FTemporalAAHistory
#include "XeSSHistory.h"
#include "XeSSPrePass.h"
#include "XeSSRHI.h"
#include "XeSSSettings.h"
#include "XeSSUnrealCore.h"
#include "XeSSUnrealRenderer.h"
#include "XeSSUtil.h"

// It SHOULD be enough, for 0.001% * 7860 (8K) = 0.0786 (pixel)
constexpr float SCREEN_PERCENTAGE_ERROR_TOLERANCE = 0.001f;
// HACK: Variables to save previous global ones
#if XESS_ENGINE_VERSION_LSS(5, 1)
ICustomStaticScreenPercentage* PreviousGCustomStaticScreenPercentage = nullptr;

	#if ENGINE_MAJOR_VERSION < 5
const ITemporalUpscaler* PreviousGTemporalUpscaler = nullptr;
	#endif

#endif

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

FXeSSPassParameters::FXeSSPassParameters(const FViewInfo& View, const XeSSUnreal::XPassInputs& PassInputs)
	: InputViewRect(View.ViewRect)
	, OutputViewRect(FIntPoint::ZeroValue, View.GetSecondaryViewRectSize())
	, SceneColorTexture(XESS_UNREAL_GET_PASS_INPUTS_TEXTURE(PassInputs, SceneColor))
	, SceneDepthTexture(XESS_UNREAL_GET_PASS_INPUTS_TEXTURE(PassInputs, SceneDepth))
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
	#endif
	}
	else
	{
		PreviousGCustomStaticScreenPercentage = GCustomStaticScreenPercentage;
		GCustomStaticScreenPercentage = this;

	#if ENGINE_MAJOR_VERSION < 5
		PreviousGTemporalUpscaler = GTemporalUpscaler;
		GTemporalUpscaler = this;
	#endif
	}
}
#endif

BEGIN_SHADER_PARAMETER_STRUCT(FXeSSShaderParameters, )
	// Exec parameters
	RDG_TEXTURE_ACCESS(InputColor, ERHIAccess::SRVCompute)
	RDG_TEXTURE_ACCESS(InputVelocity, ERHIAccess::SRVCompute)

	// Only used as WA to force Resource Transition Barrier
	RDG_BUFFER_ACCESS(DummyBuffer, ERHIAccess::UAVCompute)

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

	FXeSSRHI* LocalXeSSRHI = UpscalerXeSSRHI;
	const FIntPoint OutputExtent = Inputs.GetOutputExtent();
	// Whether to use camera cut shader permutation or not.
	const bool bCameraCut = !InputHistory.IsValid() || View.bCameraCut;
	const FVector2f JitterOffset = FVector2f(View.TemporalJitterPixels);
	const int32 QualitySetting = CVarXeSSQuality.GetValueOnAnyThread();
	const uint32 InitFlags = UpscalerXeSSRHI->GetXeSSInitFlags();

	// Create outputs
	FRDGTextureDesc OutputColorDesc = Inputs.SceneColorTexture->Desc;
	OutputColorDesc.Extent = OutputExtent;
	OutputColorDesc.Flags = TexCreate_ShaderResource | TexCreate_UAV;

	FRDGTexture* OutputSceneColor = GraphBuilder.CreateTexture(
		OutputColorDesc,
		TEXT("XeSSOutputSceneColor"),
		ERDGTextureFlags::MultiFrame);

	FXeSSShaderParameters* PassParameters = GraphBuilder.AllocParameters<FXeSSShaderParameters>();
	PassParameters->InputColor = Inputs.SceneColorTexture;
	PassParameters->InputVelocity = Inputs.SceneVelocityTexture;
	PassParameters->SceneColorOutput = FRDGTextureAccess(OutputSceneColor, ERHIAccess::UAVCompute);
	PassParameters->DummyBuffer = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateStructuredDesc((uint32)sizeof(float), 1u), TEXT("ForceTransitionDummyBuffer"));

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("XeSS Main Pass"),
		PassParameters,
		ERDGPassFlags::Compute,
		[bCameraCut, InitFlags, Inputs, JitterOffset, LocalXeSSRHI, PassParameters, QualitySetting](FRHICommandListImmediate& RHICmdList)
		{
			FXeSSInitArguments InitArgsXeSS;

			InitArgsXeSS.OutputWidth = Inputs.OutputViewRect.Width();
			InitArgsXeSS.OutputHeight = Inputs.OutputViewRect.Height();
			InitArgsXeSS.QualitySetting = QualitySetting;
			InitArgsXeSS.InitFlags = InitFlags;

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

			ExecArgsXeSS.JitterOffsetX = JitterOffset.X;
			ExecArgsXeSS.JitterOffsetY = JitterOffset.Y;
			ExecArgsXeSS.bCameraCut = bCameraCut;
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
					}
				);
			}

			// Make sure all resource transitions barriers are executed before RHIExecuteXeSS is called
			LocalXeSSRHI->TriggerResourceTransitions(RHICmdList, PassParameters->DummyBuffer);
			RHICmdList.EnqueueLambda(
				[LocalXeSSRHI, ExecArgsXeSS](FRHICommandListImmediate& Cmd)
				{
					LocalXeSSRHI->RHIExecuteXeSS(ExecArgsXeSS);
				}
			);
		});

	if (!View.bStatePrevViewInfoIsReadOnly)
	{
		OutputHistory->SafeRelease();

		GraphBuilder.QueueTextureExtraction(OutputSceneColor, &OutputHistory->RT[0]);
		OutputHistory->ViewportRect = Inputs.OutputViewRect;
		OutputHistory->ReferenceBufferSize = OutputExtent;
	}

	return OutputSceneColor;
};

#if XESS_ENGINE_VERSION_GEQ(5, 3)
XeSSUnreal::XTemporalUpscaler::FOutputs FXeSSUpscaler::AddPasses(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const FInputs& PassInputs) const
#elif XESS_ENGINE_VERSION_GEQ(5, 0)
XeSSUnreal::XTemporalUpscaler::FOutputs FXeSSUpscaler::AddPasses(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& ViewInfo,
	const FPassInputs& PassInputs) const
#else
void FXeSSUpscaler::AddPasses(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& ViewInfo,
	const FPassInputs& PassInputs,
	FRDGTextureRef* OutSceneColorTexture,
	FIntRect* OutSceneColorViewRect,
	FRDGTextureRef* OutSceneColorHalfResTexture,
	FIntRect* OutSceneColorHalfResViewRect) const
#endif
{
	RDG_EVENT_SCOPE(GraphBuilder, "XeSS Pass");
	FRDGTextureRef SceneVelocityTexture = XESS_UNREAL_GET_PASS_INPUTS_TEXTURE(PassInputs, SceneVelocity);

#if XESS_ENGINE_VERSION_GEQ(5, 3)
	XeSSUnreal::XTemporalUpscaler::FOutputs Outputs{};

	check(View.bIsViewInfo);
	check(IsXeSSEnabled());

	const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(View);
#else
	#if XESS_ENGINE_VERSION_GEQ(5, 0)
	XeSSUnreal::XTemporalUpscaler::FOutputs Outputs{};

	if (!IsXeSSEnabled())
	{
		return Outputs;
	}
	#else
	// HACK: exit if XeSS upscaler is not active, allows to have multiple upscalers loaded by project
	if (!IsXeSSEnabled())
	{
		return;
	}
	#endif

#endif

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
	#endif
	return Outputs;
#else
	// HACK: Fix crash issue when activated with other upscaler plugins at the same time
	if (!XeSSOutput)
	{
		return;
	}
	*OutSceneColorTexture = XeSSOutput;
	*OutSceneColorViewRect = XeSSMainParameters.OutputViewRect;
#endif
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
	#endif
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
	#endif
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
	#endif
	checkf(GCustomStaticScreenPercentage == this, TEXT("GCustomStaticScreenPercentage is not set to a XeSS, please make sure no other upscaling plug is enabled."));

	ViewFamily.SetTemporalUpscalerInterface(this);

	if (ViewFamily.EngineShowFlags.ScreenPercentage && !ViewFamily.GetScreenPercentageInterface())
	{
		const float ResolutionFraction = UpscalerXeSSRHI->GetOptimalResolutionFraction();
		ViewFamily.SetScreenPercentageInterface(new FLegacyScreenPercentageDriver(
			ViewFamily, ResolutionFraction
	#if ENGINE_MAJOR_VERSION < 5
			, /*InAllowPostProcessSettingsScreenPercentage*/ false
	#endif
		));
	}
}
#endif

#endif

float FXeSSUpscaler::GetMinUpsampleResolutionFraction() const
{
	return UpscalerXeSSRHI->GetMinSupportedResolutionFraction();
}

float FXeSSUpscaler::GetMaxUpsampleResolutionFraction() const
{
	return UpscalerXeSSRHI->GetMaxSupportedResolutionFraction();
}

#if XESS_ENGINE_VERSION_GEQ(5, 1)
XeSSUnreal::XTemporalUpscaler* FXeSSUpscaler::Fork_GameThread(const FSceneViewFamily& ViewFamily) const
{
	return new FXeSSUpscaler(FXeSSUpscaler::UpscalerXeSSRHI);
}
void FXeSSUpscaler::SetupViewFamily(FSceneViewFamily& ViewFamily)
{
	ViewFamily.SetTemporalUpscalerInterface(new FXeSSUpscaler(UpscalerXeSSRHI));
}
#endif

FXeSSRHI* FXeSSUpscaler::UpscalerXeSSRHI;

FXeSSUpscaler::FXeSSUpscaler(FXeSSRHI* InXeSSRHI)
{
	UpscalerXeSSRHI = InXeSSRHI;

#if XESS_ENGINE_VERSION_GEQ(5, 3)
	DummyHistory = new FXeSSHistory(this);
#endif

#if XESS_ENGINE_VERSION_LSS(5, 1)
	// Handle value set by ini file
	HandleXeSSEnabledSet(CVarXeSSEnabled->AsVariable());
	// NOTE: OnChangedCallback will always be called when set even if the value is not changed 
	CVarXeSSEnabled->AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateRaw(this, &FXeSSUpscaler::HandleXeSSEnabledSet));
#endif

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
#endif
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
	#endif
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
#endif
