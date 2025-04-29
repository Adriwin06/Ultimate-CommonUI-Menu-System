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

#include "XeSSD3D12RHI.h"

#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"
#include "Runtime/Launch/Resources/Version.h"
#include "XeSSUnrealD3D12RHIIncludes.h"
#include "XeSSUtil.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "xess/xess_d3d12.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "XeSSSDKWrapperD3D12.h"

DEFINE_LOG_CATEGORY_STATIC(LogXeSSD3D12RHI, Log, All);

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

FXeSSD3D12RHI::FXeSSD3D12RHI() :
	D3D12RHI(static_cast<XeSSUnreal::XD3D12DynamicRHI*>(GDynamicRHI))
{
}

FXeSSD3D12RHI::~FXeSSD3D12RHI()
{
}

void FXeSSD3D12RHI::OnXeSSSDKLoaded()
{
	check(XeSSSDKWrapper);

	XeSSSDKWrapperD3D12 = static_cast<FXeSSSDKWrapperD3D12*>(XeSSSDKWrapper);
}

void FXeSSD3D12RHI::RHIInitializeXeSS(const FXeSSInitArguments& InArguments)
{
	if (!IsXeSSInitialized())
	{
		return;
	}
	InitArgs = InArguments;

	xess_d3d12_init_params_t InitParams = {};
	InitParams.outputResolution.x = InArguments.OutputWidth;
	InitParams.outputResolution.y = InArguments.OutputHeight;
	InitParams.initFlags = InArguments.InitFlags;
	InitParams.qualitySetting = XeSSUtil::ToXeSSQualitySetting(InArguments.QualitySetting);
	InitParams.pPipelineLibrary = nullptr;

	xess_result_t Result = XeSSSDKWrapperD3D12->xessD3D12Init(XeSSContext, &InitParams);
	if (XESS_RESULT_SUCCESS != Result)
	{
		UE_LOG(LogXeSSD3D12RHI, Error, TEXT("Failed to initialize Intel XeSS, result: %d"), Result);
	}
}

void FXeSSD3D12RHI::RHIExecuteXeSS(const FXeSSExecuteArguments& InArguments, FRHICommandListBase& ExecutingCmdList)
{
	if (!IsXeSSInitialized())
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

	ID3D12GraphicsCommandList* D3D12CmdList = XeSSUnreal::RHIGetGraphicsCommandList(D3D12RHI, ExecutingCmdList);

	ForceBeforeResourceTransition(*D3D12CmdList, ExecuteParams);
	xess_result_t Result = XeSSSDKWrapperD3D12->xessD3D12Execute(XeSSContext, D3D12CmdList, &ExecuteParams);
	if (XESS_RESULT_SUCCESS != Result)
	{
		UE_LOG(LogXeSSD3D12RHI, Error, TEXT("Failed to execute XeSS, result: %d"), Result);
	}
	ForceAfterResourceTransition(*D3D12CmdList, ExecuteParams);
	XeSSUnreal::RHIFinishExternalComputeWork(D3D12RHI, ExecutingCmdList, D3D12CmdList);
}

xess_result_t FXeSSD3D12RHI::CreateXeSSContext(xess_context_handle_t* OutXeSSContext)
{
	ID3D12Device* Direct3DDevice = XeSSUnreal::GetDevice(D3D12RHI);

	check(Direct3DDevice);
	return XeSSSDKWrapperD3D12->xessD3D12CreateContext(Direct3DDevice, OutXeSSContext);
}

xess_result_t FXeSSD3D12RHI::BuildXeSSPipelines(xess_context_handle_t InXeSSContext, uint32_t InInitFlags)
{
	return XeSSSDKWrapperD3D12->xessD3D12BuildPipelines(InXeSSContext, nullptr, true, InInitFlags);
}
