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

#include "XeSSRHI.h"

#include "XeSSMacros.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "xess_d3d12.h"
#include "xess_d3d12_debug.h"
#include "xess_debug.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "RenderGraphResources.h"
#include "Misc/Paths.h"
#include "XeSSUnrealIncludes.h"
#include "XeSSUtil.h"

extern TAutoConsoleVariable<FString> GCVarXeSSVersion;
// The UE module
DEFINE_LOG_CATEGORY_STATIC(LogXeSSRHI, Log, All);

#define LOCTEXT_NAMESPACE "FXeSSPlugin"

struct FResolutionFractionSetting
{
	float Min = 0.f;
	float Max = 0.f;
	float Optimal = 0.f;
};
static FResolutionFractionSetting ResolutionFractionSettings[XeSSUtil::XESS_QUALITY_SETTING_COUNT] = {};
static float MinResolutionFraction = 100.f;
static float MaxResolutionFraction = 0.f;

static TAutoConsoleVariable<int32> CVarXeSSFrameDumpStart(
	TEXT("r.XeSS.FrameDump.Start"),
	0,
	TEXT("Captures of all input resources passed to XeSS for the specified number of frames."),
	ECVF_Default);

static TAutoConsoleVariable<FString> CVarXeSSFrameDumpMode(
	TEXT("r.XeSS.FrameDump.Mode"),
	TEXT("all"),
	TEXT("[default: all] Dump mode, available values: inputs, all."),
	ECVF_Default);

static TAutoConsoleVariable<FString> CVarXeSSFrameDumpPath(
	TEXT("r.XeSS.FrameDump.Path"),
	TEXT("."),
	TEXT("Select path for frame capture dumps, if not specified the game's binary directory will be used."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarXeSSOptimalScreenPercentage(
	TEXT("r.XeSS.OptimalScreenPercentage"),
	100.f,
	TEXT("Optimal screen percentage for current XeSS quality."),
	ECVF_ReadOnly);

static TAutoConsoleVariable<int32> CVarXeSSAutoExposure(
	TEXT("r.XeSS.AutoExposure"),
	1,
	TEXT("[default: 1] Use XeSS internal auto exposure."),
	ECVF_Default | ECVF_RenderThreadSafe);

// Temporary workaround for missing resource barrier flush in UE5
inline void ForceBeforeResourceTransition(ID3D12GraphicsCommandList& D3D12CmdList, const xess_d3d12_execute_params_t& ExecuteParams)
{
#if ENGINE_MAJOR_VERSION >= 5
	TArray<CD3DX12_RESOURCE_BARRIER> OutTransitions;
	OutTransitions.Add(CD3DX12_RESOURCE_BARRIER::Transition(ExecuteParams.pColorTexture,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	OutTransitions.Add(CD3DX12_RESOURCE_BARRIER::Transition(ExecuteParams.pVelocityTexture,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	D3D12CmdList.ResourceBarrier(OutTransitions.Num(), OutTransitions.GetData());
#endif
};

inline void ForceAfterResourceTransition(ID3D12GraphicsCommandList& D3D12CmdList, const xess_d3d12_execute_params_t& ExecuteParams)
{
#if ENGINE_MAJOR_VERSION >= 5
	TArray<CD3DX12_RESOURCE_BARRIER> OutTransitions;
	OutTransitions.Add(CD3DX12_RESOURCE_BARRIER::Transition(ExecuteParams.pColorTexture,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	OutTransitions.Add(CD3DX12_RESOURCE_BARRIER::Transition(ExecuteParams.pVelocityTexture,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	D3D12CmdList.ResourceBarrier(OutTransitions.Num(), OutTransitions.GetData());
#endif
};




bool FXeSSRHI::EffectRecreationIsRequired(const FXeSSInitArguments& NewArgs) const {
	if (InitArgs.OutputWidth != NewArgs.OutputWidth ||
		InitArgs.OutputHeight != NewArgs.OutputHeight ||
		InitArgs.QualitySetting != NewArgs.QualitySetting ||
		InitArgs.InitFlags != NewArgs.InitFlags)
	{
		return true;
	}
	return false;
}

float FXeSSRHI::GetOptimalResolutionFraction()
{
	return GetOptimalResolutionFraction(QualitySetting);
}

float FXeSSRHI::GetMinSupportedResolutionFraction()
{
	return MinResolutionFraction;
}

float FXeSSRHI::GetMaxSupportedResolutionFraction()
{
	return MaxResolutionFraction;
}

float FXeSSRHI::GetOptimalResolutionFraction(const xess_quality_settings_t InQualitySetting)
{
	check(XeSSUtil::IsValid(InQualitySetting));

	return ResolutionFractionSettings[XeSSUtil::ToIndex(InQualitySetting)].Optimal;
}

uint32 FXeSSRHI::GetXeSSInitFlags()
{
	uint32 InitFlags = XESS_INIT_FLAG_HIGH_RES_MV;
	if (CVarXeSSAutoExposure->GetBool())
	{
		InitFlags |= XESS_INIT_FLAG_ENABLE_AUTOEXPOSURE;
	}
	return InitFlags;
}

FXeSSRHI::FXeSSRHI(FDynamicRHI* DynamicRHI)
	: D3D12RHI(static_cast<XeSSUnreal::XD3D12DynamicRHI*>(DynamicRHI))
{
	// TODO: use device index
	ID3D12Device* Direct3DDevice = XeSSUnreal::GetDevice(D3D12RHI, 0);

	check(D3D12RHI);
	check(Direct3DDevice);

	xess_result_t Result = xessD3D12CreateContext(Direct3DDevice, &XeSSContext);
	if (Result == XESS_RESULT_SUCCESS)
	{
		UE_LOG(LogXeSSRHI, Log, TEXT("Intel XeSS effect supported"));
	}
	else
	{
		UE_LOG(LogXeSSRHI, Log, TEXT("Intel XeSS effect NOT supported"));
		return;
	}

	// Print XeFX library version if it was loaded, XeFX will only be used when running on Intel platforms
	xess_version_t XeFXLibVersion;
	if (xessGetIntelXeFXVersion(XeSSContext, &XeFXLibVersion) != XESS_RESULT_SUCCESS)
	{
		UE_LOG(LogXeSSRHI, Warning, TEXT("Error when calling XeSS function: xessGetIntelXeFXVersion"));
		return;
	}

	// Append XeFX library info to version string when running on Intel
	if (XeFXLibVersion.major != XeFXLibVersion.minor != XeFXLibVersion.patch != 0)
	{
		TStringBuilder<32> VersionStringBuilder;
		VersionStringBuilder << GCVarXeSSVersion->GetString() << " XeFX version: "
			<< XeFXLibVersion.major << "." << XeFXLibVersion.minor << "." << XeFXLibVersion.patch;
		GCVarXeSSVersion->Set(VersionStringBuilder.GetData());

		UE_LOG(LogXeSSRHI, Log, TEXT("Loading Intel XeFX library %d.%d.%d"),
			XeFXLibVersion.major, XeFXLibVersion.minor, XeFXLibVersion.patch);
	}

	InitResolutionFractions();

	// Pre-build XeSS kernel
	xessD3D12BuildPipelines(XeSSContext, nullptr, true, GetXeSSInitFlags());

	static const auto CVarXeSSEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Enabled"));
	static const auto CVarXeSSQuality = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Quality"));

	// Register callback to handle frame capture requests
	CVarXeSSFrameDumpStart->AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateLambda([this](IConsoleVariable* InVariable)
	{
		if (!CVarXeSSEnabled->AsVariableInt()->GetValueOnGameThread())
		{
			UE_LOG(LogXeSS, Error, TEXT("XeSS is not enabled - please make sure r.XeSS.Enabled is set to 1 before starting frame capture."));
			return;
		}
		TriggerFrameCapture(InVariable->GetInt());
	}));

	// Handle value set by ini file
	HandleXeSSEnabledSet(CVarXeSSEnabled->AsVariable());
	// NOTE: OnChangedCallback will always be called when set even if the value is not changed 
	CVarXeSSEnabled->AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateRaw(this, &FXeSSRHI::HandleXeSSEnabledSet));

	// Handle value set by in file
	HandleXeSSQualitySet(CVarXeSSQuality->AsVariable());
	CVarXeSSQuality->AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateRaw(this, &FXeSSRHI::HandleXeSSQualitySet));

	bXeSSInitialized = true;
}

FXeSSRHI::~FXeSSRHI()
{
	if (!bXeSSInitialized)
	{
		return;
	}

	xess_result_t Result = xessDestroyContext(XeSSContext);
	if (Result == XESS_RESULT_SUCCESS)
	{
		UE_LOG(LogXeSSRHI, Log, TEXT("Removed Intel XeSS effect"));
	}
	else
	{
		UE_LOG(LogXeSSRHI, Warning, TEXT("Failed to remove XeSS effect"));
		return;
	}
}

void FXeSSRHI::RHIInitializeXeSS(const FXeSSInitArguments& InArguments)
{
	if (!bXeSSInitialized)
	{
		return;
	}

	InitArgs = InArguments;
	QualitySetting = XeSSUtil::ToXeSSQualitySetting(InArguments.QualitySetting);

	xess_d3d12_init_params_t InitParams = {};
	InitParams.outputResolution.x = InArguments.OutputWidth;
	InitParams.outputResolution.y = InArguments.OutputHeight;
	InitParams.initFlags = InArguments.InitFlags;
	InitParams.qualitySetting = QualitySetting;
	InitParams.pPipelineLibrary = nullptr;

	// Add DLL search path for XeFX.dll and XeFX_Loader.dll
	// NOTE: it is a MUST, for former adding in starting up module may be cleared by engine or other plugins
	SetDllDirectory(*FPaths::Combine(IPluginManager::Get().FindPlugin("XeSS")->GetBaseDir(), TEXT("/Binaries/ThirdParty/Win64")));

	if (XESS_RESULT_SUCCESS != xessD3D12Init(XeSSContext, &InitParams))
	{
		UE_LOG(LogXeSSRHI, Error, TEXT("Failed to initialize Intel XeSS."));
	}
}

void FXeSSRHI::RHIExecuteXeSS(FRHICommandList& CmdList,const FXeSSExecuteArguments& InArguments)
{
	if (!bXeSSInitialized)
	{
		return;
	}

	xess_d3d12_execute_params_t ExecuteParams{};
	ExecuteParams.pColorTexture = XeSSUnreal::GetResource(D3D12RHI, InArguments.ColorTexture);
	ExecuteParams.pVelocityTexture = XeSSUnreal::GetResource(D3D12RHI, InArguments.VelocityTexture);
	ExecuteParams.pOutputTexture = XeSSUnreal::GetResource(D3D12RHI, InArguments.OutputTexture);
	ExecuteParams.jitterOffsetX = InArguments.JitterOffsetX;
	ExecuteParams.jitterOffsetY = InArguments.JitterOffsetY;
	ExecuteParams.resetHistory = InArguments.bCameraCut;
	ExecuteParams.inputWidth = InArguments.SrcViewRect.Width();
	ExecuteParams.inputHeight = InArguments.SrcViewRect.Height();
	ExecuteParams.inputColorBase.x = InArguments.SrcViewRect.Min.X;
	ExecuteParams.inputColorBase.y = InArguments.SrcViewRect.Min.Y;
	ExecuteParams.outputColorBase.x = InArguments.DstViewRect.Min.X;
	ExecuteParams.outputColorBase.y = InArguments.DstViewRect.Min.Y;
	ExecuteParams.exposureScale = 1.0f;

	// Execute
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	
	const uint32 DeviceIndex = D3D12RHI->RHIGetResourceDeviceIndex(InArguments.ColorTexture);
	ID3D12GraphicsCommandList* D3D12CmdList = D3D12RHI->RHIGetGraphicsCommandList(CmdList,DeviceIndex);
#else // XESS_ENGINE_VERSION_GEQ(5, 1)
	FD3D12CommandContext& CommandContext = GetD3D12TextureFromRHITexture(InArguments.ColorTexture)->GetParentDevice()->GetCommandContext();
	ID3D12GraphicsCommandList* D3D12CmdList = CommandContext.CommandListHandle.GraphicsCommandList();
#endif // XESS_ENGINE_VERSION_GEQ(5, 1)

	ForceBeforeResourceTransition(*D3D12CmdList, ExecuteParams);

	if (XESS_RESULT_SUCCESS != xessD3D12Execute(XeSSContext, D3D12CmdList, &ExecuteParams))
	{
		UE_LOG(LogXeSSRHI, Error, TEXT("Error when executing XeSS."));
	}

	ForceAfterResourceTransition(*D3D12CmdList, ExecuteParams);

#if XESS_ENGINE_VERSION_GEQ(5, 1)
	D3D12RHI->RHIFinishExternalComputeWork(CmdList,DeviceIndex, D3D12CmdList);
#else // XESS_ENGINE_VERSION_GEQ(5, 1)
	// Make sure root signature and heap state is reset
	CommandContext.StateCache.ForceSetComputeRootSignature();
	CommandContext.StateCache.GetDescriptorCache()->SetCurrentCommandList(CommandContext.CommandListHandle);
#endif // XESS_ENGINE_VERSION_GEQ(5, 1)
}

void FXeSSRHI::InitResolutionFractions()
{
	for (int32 QualitySettingInt = XeSSUtil::XESS_QUALITY_SETTING_MIN; QualitySettingInt <= XeSSUtil::XESS_QUALITY_SETTING_MAX; ++QualitySettingInt)
	{
		// Use D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION(16384) to avoid potential API errors
		xess_2d_t OutputResolution{ D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION };
		xess_2d_t MinInputResolution{};
		xess_2d_t MaxInputResolution{};
		xess_2d_t OptimalInputResolution{};
		xess_quality_settings_t TempQualitySetting = static_cast<xess_quality_settings_t>(QualitySettingInt);
		xess_result_t Result = xessGetOptimalInputResolution(XeSSContext, &OutputResolution, TempQualitySetting, &OptimalInputResolution, &MinInputResolution, &MaxInputResolution);
		if (Result != XESS_RESULT_SUCCESS)
		{
			UE_LOG(LogXeSSRHI, Warning, TEXT("Error when calling xessGetInputResolution."));
			continue;
		}
		FResolutionFractionSetting Setting;
		Setting.Optimal = float(OptimalInputResolution.x) / float(OutputResolution.x);
		Setting.Min = float(MinInputResolution.x) / float(OutputResolution.x);
		Setting.Max = float(MaxInputResolution.x) / float(OutputResolution.x);
		if (Setting.Min < MinResolutionFraction)
		{
			MinResolutionFraction = Setting.Min;
		}
		if (Setting.Max > MaxResolutionFraction)
		{
			MaxResolutionFraction = Setting.Max;
		}
		ResolutionFractionSettings[XeSSUtil::ToIndex(TempQualitySetting)] = Setting;
	}
}

void FXeSSRHI::TriggerFrameCapture(int FrameCount) const
{
	if (FrameCount > 0)
	{
		FString DumpPath = FPaths::ConvertRelativePathToFull(CVarXeSSFrameDumpPath.GetValueOnAnyThread());		
		DumpPath = FPaths::Combine(*DumpPath, FString("XeSS_Dump"));

		if (!IFileManager::Get().MakeDirectory(*DumpPath, /*Tree*/true))
		{
			UE_LOG(LogXeSSRHI, Error, TEXT("XeSS Frame Capture: failed to create directory %s."), *DumpPath);
			return;
		}

		xess_dump_parameters_t DumpParameters = {};
		xess_dump_element_bits_t DumpElementsMask = XESS_DUMP_ALL;
		FString DumpMode = CVarXeSSFrameDumpMode.GetValueOnAnyThread().ToLower();
		if (DumpMode.Equals(TEXT("inputs"))) 
		{
			DumpElementsMask = XESS_DUMP_ALL_INPUTS;
		}		
		DumpParameters.path = TCHAR_TO_ANSI(*DumpPath);
		DumpParameters.frame_idx = GFrameNumber;
		DumpParameters.frame_count = FrameCount;
		DumpParameters.dump_elements_mask = DumpElementsMask;

		if (XESS_RESULT_SUCCESS != xessStartDump(XeSSContext, &DumpParameters))
		{
			UE_LOG(LogXeSSRHI, Error, TEXT("Error when dumping XeSS debug data."));
		}
	}
}

void FXeSSRHI::TriggerResourceTransitions(FRHICommandListImmediate& RHICmdList, TRDGBufferAccess<ERHIAccess::UAVCompute> DummyBufferAccess) const
{
#if ENGINE_MAJOR_VERSION >= 5
	FRHIBuffer* DummyBuffer = DummyBufferAccess->GetRHI();
	// Using the dummy buffer to trigger a resource transition
	RHICmdList.LockBuffer(DummyBuffer, 0, sizeof(float), EResourceLockMode::RLM_WriteOnly);
	RHICmdList.UnlockBuffer(DummyBuffer);
#else
	FRHIStructuredBuffer* DummyBuffer = DummyBufferAccess->GetRHIStructuredBuffer();
	// Using the dummy structured buffer to trigger a resource transition
	RHICmdList.LockStructuredBuffer(DummyBuffer, 0, sizeof(float), EResourceLockMode::RLM_WriteOnly);
	RHICmdList.UnlockStructuredBuffer(DummyBuffer);
#endif // ENGINE_MAJOR_VERSION >= 5
}

void FXeSSRHI::HandleXeSSEnabledSet(IConsoleVariable* Variable)
{
	// Return if no change as bool
	if (bCurrentXeSSEnabled == Variable->GetBool())
	{
		return;
	}
	bCurrentXeSSEnabled = Variable->GetBool();
	if (bCurrentXeSSEnabled)
	{
		// Re-initialize XeSS each time it is re-enabled
		InitArgs = FXeSSInitArguments();
	}
}

void FXeSSRHI::HandleXeSSQualitySet(IConsoleVariable* Variable)
{
	CVarXeSSOptimalScreenPercentage->Set(100.f *
		GetOptimalResolutionFraction(
			XeSSUtil::ToXeSSQualitySetting(Variable->GetInt())
		)
	);
}

#undef LOCTEXT_NAMESPACE

