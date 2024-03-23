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

#include "FFXD3D12Backend.h"
#include "FFXSharedBackend.h"
#include "../../FFXFrameInterpolation/Public/FFXFrameInterpolationModule.h"
#include "../../FFXFrameInterpolation/Public/IFFXFrameInterpolation.h"
#include "CoreMinimal.h"
#include "Interfaces/IPluginManager.h"
#include "RenderGraphResources.h"
#include "Features/IModularFeatures.h"
#include "FFXFSR3Settings.h"

#include "FFXFrameInterpolationApi.h"
#include "FFXD3D12Includes.h"

#if UE_VERSION_AT_LEAST(5, 2, 0)
#define FFX_UE_SUPPORTS_SWAPCHAIN_PROVIDER_V1 1
#else
#define FFX_UE_SUPPORTS_SWAPCHAIN_PROVIDER_V1 0
#endif

#if UE_VERSION_AT_LEAST(5, 3, 0)
#define FFX_UE_SUPPORTS_SWAPCHAIN_PROVIDER_V2 1
#else
#define FFX_UE_SUPPORTS_SWAPCHAIN_PROVIDER_V2 0
#endif

#if FFX_UE_SUPPORTS_SWAPCHAIN_PROVIDER_V1
#include "Windows/IDXGISwapchainProvider.h"
#endif

#include <mutex>

//-------------------------------------------------------------------------------------
// Definitions and includes to interact with the internals of the D3D12RHI.
//-------------------------------------------------------------------------------------
#if PLATFORM_WINDOWS
#define GetD3D11CubeFace GetD3D12CubeFace
#define VerifyD3D11Result VerifyD3D12Result
#define GetD3D11TextureFromRHITexture GetD3D12TextureFromRHITexture
#define FRingAllocation FRingAllocation_D3D12
#define GetRenderTargetFormat GetRenderTargetFormat_D3D12
#define ED3D11ShaderOffsetBuffer ED3D12ShaderOffsetBuffer
#define FindShaderResourceDXGIFormat FindShaderResourceDXGIFormat_D3D12
#define FindUnorderedAccessDXGIFormat FindUnorderedAccessDXGIFormat_D3D12
#define FindDepthStencilDXGIFormat FindDepthStencilDXGIFormat_D3D12
#define HasStencilBits HasStencilBits_D3D12
#define FVector4VertexDeclaration FVector4VertexDeclaration_D3D12
#define GLOBAL_CONSTANT_BUFFER_INDEX GLOBAL_CONSTANT_BUFFER_INDEX_D3D12
#define MAX_CONSTANT_BUFFER_SLOTS MAX_CONSTANT_BUFFER_SLOTS_D3D12
#define FD3DGPUProfiler FD3D12GPUProfiler
#define FRangeAllocator FRangeAllocator_D3D12

#ifndef WITH_NVAPI
#define WITH_NVAPI 0
#endif
#ifndef NV_AFTERMATH
#define NV_AFTERMATH 0
#endif // !NV_AFTERMATH
#ifndef INTEL_EXTENSIONS
#define INTEL_EXTENSIONS 0
#endif // !INTEL_EXTENSIONS

#include "D3D12RHIPrivate.h"
#include "D3D12Util.h"

#undef GetD3D11CubeFace
#undef VerifyD3D11Result
#undef GetD3D11TextureFromRHITexture
#undef FRingAllocation
#undef GetRenderTargetFormat
#undef ED3D11ShaderOffsetBuffer
#undef FindShaderResourceDXGIFormat
#undef FindUnorderedAccessDXGIFormat
#undef FindDepthStencilDXGIFormat
#undef HasStencilBits
#undef FVector4VertexDeclaration
#undef GLOBAL_CONSTANT_BUFFER_INDEX
#undef MAX_CONSTANT_BUFFER_SLOTS
#undef FD3DGPUProfiler
#undef FRangeAllocator
#endif // PLATFORM_WINDOWS

#define LOCTEXT_NAMESPACE "FFXD3D12Backend"

//-------------------------------------------------------------------------------------
// Helper variable declarations.
//-------------------------------------------------------------------------------------
IMPLEMENT_MODULE(FFXD3D12BackendModule, FFXD3D12Backend)
extern ENGINE_API float GAverageFPS;
extern ENGINE_API float GAverageMS;
#if FFX_UE_SUPPORTS_SWAPCHAIN_PROVIDER_V1
TCHAR SwapChainProviderName[] = TEXT("FSR3SwapchainProvider");
#endif

//-------------------------------------------------------------------------------------
// Static helper functions.
//-------------------------------------------------------------------------------------
static EPixelFormat ffxGetSurfaceFormatDX12ToUE(DXGI_FORMAT format)
{
	EPixelFormat UEFormat = PF_Unknown;
	for (uint32 i = 0; i < PF_MAX; i++)
	{
		DXGI_FORMAT PlatformFormat = (DXGI_FORMAT)GPixelFormats[i].PlatformFormat;
		if (PlatformFormat == format)
		{
			UEFormat = (EPixelFormat)i;
			break;
		}
	}
	return UEFormat;
}

static ETextureCreateFlags ffxGetSurfaceFlagsDX12ToUE(D3D12_RESOURCE_FLAGS flags)
{
	ETextureCreateFlags NewFlags = ETextureCreateFlags::None;
	switch(flags)
	{
        case D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET:
			NewFlags |= ETextureCreateFlags::RenderTargetable;
			NewFlags |= ETextureCreateFlags::ShaderResource;
			break;
        case D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL:
			NewFlags |= ETextureCreateFlags::DepthStencilTargetable;
			NewFlags |= ETextureCreateFlags::ShaderResource;
			break;
        case D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS:
			NewFlags |= ETextureCreateFlags::UAV;
			NewFlags |= ETextureCreateFlags::ShaderResource;
			break;
        case D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE:
			NewFlags |= ETextureCreateFlags::DisableSRVCreation;
			NewFlags &= ~ETextureCreateFlags::ShaderResource;
			break;
        case D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER:
        case D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS:
			NewFlags |= ETextureCreateFlags::Shared;
			break;
        case D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY:
        case D3D12_RESOURCE_FLAG_VIDEO_ENCODE_REFERENCE_ONLY:
		case D3D12_RESOURCE_FLAG_NONE:
		default:
			break;
	}
	return NewFlags;
}

FfxResource ffxGetResourceDX12(ID3D12Resource* dx12Resource, const wchar_t* name, FfxResourceStates state)
{
    FfxResource resource = {};
    resource.resource = reinterpret_cast<void*>(dx12Resource);
    resource.state = state;

    if (dx12Resource) {
        resource.description.flags = FFX_RESOURCE_FLAGS_NONE;
        resource.description.width = (uint32_t)dx12Resource->GetDesc().Width;
        resource.description.height = dx12Resource->GetDesc().Height;
        resource.description.depth = dx12Resource->GetDesc().DepthOrArraySize;
        resource.description.mipCount = dx12Resource->GetDesc().MipLevels;
        resource.description.format = ffxGetSurfaceFormatDX12(dx12Resource->GetDesc().Format);
        
        switch (dx12Resource->GetDesc().Dimension) {

        case D3D12_RESOURCE_DIMENSION_BUFFER:
            resource.description.type = FFX_RESOURCE_TYPE_BUFFER;
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            resource.description.type = FFX_RESOURCE_TYPE_TEXTURE1D;
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            resource.description.type = FFX_RESOURCE_TYPE_TEXTURE2D;
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            resource.description.type = FFX_RESOURCE_TYPE_TEXTURE3D;
            break;
        default:
            break;
        }
    }
#ifdef _DEBUG
    if (name) {
        wcscpy_s(resource.name, name);
    }
#endif

    return resource;
}

//-------------------------------------------------------------------------------------
// The D3D12 implementation of the FFX shared backend that interacts with the D3D12RHI.
//-------------------------------------------------------------------------------------
class FFXD3D12Backend : public IFFXSharedBackend
{
	struct FFXFrameResources
	{
		TRefCountPtr<FRHIResource> FIResources;
		TRefCountPtr<IRefCountedObject> FSR3Resources;
	};

	TQueue<FFXFrameResources> FrameResources;
	uint32 NumFrameResources;
	static double LastTime;
	static float AverageTime;
	static float AverageFPS;
public:
	static FFXD3D12Backend sFFXD3D12Backend;

	FFXD3D12Backend()
	{
		NumFrameResources = 0;
	}

	virtual ~FFXD3D12Backend()
	{
	}

	void Init() final
	{

	}

	EFFXBackendAPI GetAPI() const
	{
		return EFFXBackendAPI::D3D12;
	}
	void SetFeatureLevel(FfxInterface& OutInterface, ERHIFeatureLevel::Type FeatureLevel) final
	{

	}
	size_t GetGetScratchMemorySize() final
	{
		return ffxGetScratchMemorySizeDX12(12);
	}
	FfxErrorCode CreateInterface(FfxInterface& OutInterface, uint32 MaxContexts) final
	{
		FfxErrorCode Code = FFX_OK;
		if (OutInterface.device == nullptr)
		{
			size_t InscratchBufferSize = GetGetScratchMemorySize();
			void* InScratchBuffer = FMemory::Malloc(InscratchBufferSize);
			FMemory::Memzero(InScratchBuffer, InscratchBufferSize);
			Code = ffxGetInterfaceDX12(&OutInterface, (ID3D12Device*)GetNativeDevice(), InScratchBuffer, InscratchBufferSize, MaxContexts);
			if (Code != FFX_OK)
			{
				FMemory::Free(InScratchBuffer);
				FMemory::Memzero(OutInterface);
			}
		}
		else
		{
			Code = FFX_ERROR_INVALID_ARGUMENT;
		}
		return Code;
	}
	FfxDevice GetDevice(void* device) final
	{
		return ffxGetDeviceDX12((ID3D12Device*)device);
	}
	FfxCommandList GetCommandList(void* list) final
	{
		return ffxGetCommandListDX12((ID3D12CommandList*) list);
	}
	FfxResource GetResource(void* resource, wchar_t* name, FfxResourceStates state, uint32 shaderComponentMapping) final
	{
		return ffxGetResourceDX12((ID3D12Resource*)resource, name, state);
	}
	FfxCommandQueue GetCommandQueue(void* cmdQueue) final
	{
		return ffxGetCommandQueueDX12((ID3D12CommandQueue*)cmdQueue);
	}
	FfxSwapchain GetSwapchain(void* swapChain) final
	{
		IDXGISwapChain4* SwapChain4 = nullptr;
		if (swapChain)
		{
			((IDXGISwapChain1*)swapChain)->QueryInterface<IDXGISwapChain4>(&SwapChain4);
			check(SwapChain4);
			((IDXGISwapChain1*)swapChain)->Release();
		}

		return ffxGetSwapchainDX12(SwapChain4);
	}

	FfxDevice GetNativeDevice() final
	{
		ID3D12Device* device = (ID3D12Device*)GDynamicRHI->RHIGetNativeDevice();
		return ffxGetDeviceDX12(device);
	}

	FfxResource GetNativeResource(FRHITexture* Texture, FfxResourceStates State) final
	{
		return ffxGetResourceDX12((ID3D12Resource*)Texture->GetNativeResource(), nullptr, State);
	}

	FfxResource GetNativeResource(FRDGTexture* Texture, FfxResourceStates State) final
	{
		return GetNativeResource(Texture->GetRHI(), State);
	}

	FfxCommandList GetNativeCommandBuffer(FRHICommandListImmediate& RHICmdList) final
	{
		FfxCommandList CmdList = ffxGetCommandListDX12((ID3D12CommandList*)GetID3D12DynamicRHI()->RHIGetGraphicsCommandList(0));
		return CmdList;
	}

	uint32 GetNativeTextureFormat(FRHITexture* Texture)
	{
		return 0;
	}
	FfxShaderModel GetSupportedShaderModel()
	{
		FfxShaderModel ShaderModel = FFX_SHADER_MODEL_5_1;
		ID3D12Device* dx12Device = (ID3D12Device*)GDynamicRHI->RHIGetNativeDevice();
		D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_6 };
		if (dx12Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(D3D12_FEATURE_DATA_SHADER_MODEL)) >= 0)
		{
			switch (shaderModel.HighestShaderModel)
			{
			case D3D_SHADER_MODEL_5_1:
				ShaderModel = FFX_SHADER_MODEL_5_1;
				break;
			case D3D_SHADER_MODEL_6_0:
				ShaderModel = FFX_SHADER_MODEL_6_0;
				break;
			case D3D_SHADER_MODEL_6_1:
				ShaderModel = FFX_SHADER_MODEL_6_1;
				break;
			case D3D_SHADER_MODEL_6_2:
				ShaderModel = FFX_SHADER_MODEL_6_2;
				break;
			case D3D_SHADER_MODEL_6_3:
				ShaderModel = FFX_SHADER_MODEL_6_3;
				break;
			case D3D_SHADER_MODEL_6_4:
				ShaderModel = FFX_SHADER_MODEL_6_4;
				break;
			case D3D_SHADER_MODEL_6_5:
				ShaderModel = FFX_SHADER_MODEL_6_5;
				break;
			case D3D_SHADER_MODEL_6_6:
			default:
				ShaderModel = FFX_SHADER_MODEL_6_6;
				break;
			}
		}

		return ShaderModel;
	}
	bool IsFloat16Supported()
	{
		bool bIsSupported = false;
		ID3D12Device* dx12Device = (ID3D12Device*)GDynamicRHI->RHIGetNativeDevice();
		// check if we have 16bit floating point.
		D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12Options = {};
		if (SUCCEEDED(dx12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12Options, sizeof(d3d12Options)))) {

			bIsSupported = !!(d3d12Options.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT);
		}
		return bIsSupported;
	}

	static D3D12_RESOURCE_STATES GetDX12StateFromResourceState(FfxResourceStates state)
	{
		switch (state) {

			case(FFX_RESOURCE_STATE_GENERIC_READ):
				return D3D12_RESOURCE_STATE_GENERIC_READ;
			case(FFX_RESOURCE_STATE_UNORDERED_ACCESS):
				return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			case (FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ):
				return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			case(FFX_RESOURCE_STATE_COMPUTE_READ):
				return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			case (FFX_RESOURCE_STATE_PIXEL_READ):
				return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			case FFX_RESOURCE_STATE_COPY_SRC:
				return D3D12_RESOURCE_STATE_COPY_SOURCE;
			case FFX_RESOURCE_STATE_COPY_DEST:
				return D3D12_RESOURCE_STATE_COPY_DEST;
			case FFX_RESOURCE_STATE_INDIRECT_ARGUMENT:
				return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
			default:
				return D3D12_RESOURCE_STATE_COMMON;
		}
	}

	void ForceUAVTransition(FRHICommandListImmediate& RHICmdList, FRHITexture* OutputTexture, ERHIAccess Access)
	{
		FRHITransitionInfo Info(OutputTexture, ERHIAccess::Unknown, Access);
		RHICmdList.Transition(Info);
	}

	static FfxErrorCode FFXFrameInterpolationUiCompositionCallback(const FfxPresentCallbackDescription* params)
	{
		static const auto CVarFSR3Enabled = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FidelityFX.FSR3.Enabled"));
		static const auto CVarFIEnabled = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FidelityFX.FI.Enabled"));
		sFFXD3D12Backend.ReleaseFrameResources();

		ffxFrameInterpolationUiComposition(params);

		{
			double CurrentTime = FPlatformTime::Seconds();
			float FrameTimeMS = (float)((CurrentTime - LastTime) * 1000.0);
			AverageTime = AverageTime * 0.75f + FrameTimeMS * 0.25f;
			LastTime = CurrentTime;
			AverageFPS = 1000.f / AverageTime;

			static IConsoleVariable* CVarFFXFIUpdateFrameTime = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FI.UpdateGlobalFrameTime"));
			if (CVarFFXFIUpdateFrameTime && CVarFFXFIUpdateFrameTime->GetInt() != 0 && (CVarFIEnabled && CVarFIEnabled->GetValueOnAnyThread() != 0) && (CVarFSR3Enabled && CVarFSR3Enabled->GetValueOnAnyThread() != 0))
			{
				GAverageMS = AverageTime;
				GAverageFPS = AverageFPS;
			}
		}

		return FFX_OK;
	}

	void UpdateSwapChain(FfxInterface& Interface, void* SwapChain, bool mode, bool allowAsyncWorkloads, bool showDebugView)
	{
		FfxSwapchain ffxSwapChain = GetSwapchain(SwapChain);

		if (ffxSwapChain && Interface.fpSwapChainConfigureFrameGeneration)
		{
			FfxFrameGenerationConfig Config;
			FMemory::Memzero(Config);
			Config.presentCallback = &FFXFrameInterpolationUiCompositionCallback;
			Config.swapChain = ffxSwapChain;
			Config.frameGenerationEnabled = mode;
			Config.allowAsyncWorkloads = allowAsyncWorkloads;
#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT)
			Config.flags |= showDebugView ? FFX_FRAMEINTERPOLATION_DISPATCH_DRAW_DEBUG_VIEW : 0;
#endif
			Interface.fpSwapChainConfigureFrameGeneration(&Config);
		}
	}

	FfxResource GetInterpolationOutput(FfxSwapchain SwapChain)
	{
		return ffxGetFrameinterpolationTextureDX12(SwapChain);
	}

	FfxCommandList GetInterpolationCommandList(FfxSwapchain SwapChain)
	{
		FfxCommandList CmdList = nullptr;
		FfxErrorCode Code = ffxGetFrameinterpolationCommandlistDX12(SwapChain, CmdList);
		check(Code == FFX_OK);
		return CmdList;
	}

	void BindUITexture(FfxSwapchain gameSwapChain, FfxResource uiResource)
	{
		ffxRegisterFrameinterpolationUiResourceDX12(gameSwapChain, uiResource);
	}

	FFXSharedResource CreateResource(FfxInterface& Interface, const FfxCreateResourceDescription* desc) final
	{
		FFXSharedResource Result = {};
		FfxResourceInternal Internal = {};
		Interface.fpCreateResource(&Interface, desc, 0, &Internal);
		Result.Resource = Interface.fpGetResource(&Interface, Internal);
		Result.Data = (void*)((uintptr_t)Internal.internalIndex);
		return Result;
	}

	FfxErrorCode ReleaseResource(FfxInterface& Interface, FFXSharedResource Resource) final
	{
		FfxResourceInternal Internal = {};
		Internal.internalIndex = ((uintptr_t)Resource.Data);
		return Interface.fpDestroyResource(&Interface, Internal, 0);
	}

	void RegisterFrameResources(FRHIResource* FIResources, IRefCountedObject* FSR3Resources) final
	{
		FFXFrameResources Resources;
		Resources.FIResources = FIResources;
		Resources.FSR3Resources = FSR3Resources;
		FrameResources.Enqueue(Resources);
		NumFrameResources++;
	}

	void ReleaseFrameResources()
	{
		while (NumFrameResources > 6)
		{
			FrameResources.Pop();
			NumFrameResources--;
		}
	}

	bool GetAverageFrameTimes(float& AvgTimeMs, float& AvgFPS) final
	{
		AvgTimeMs = AverageTime;
		AvgFPS = AverageFPS;
		return true;
	}

	void CopySubRect(FfxCommandList CmdList, FfxResource Src, FfxResource Dst, FIntPoint OutputExtents, FIntPoint OutputPoint) final
	{
		ID3D12GraphicsCommandList* pCmdList = (ID3D12GraphicsCommandList*)CmdList;
		ID3D12Resource* SrcRes = (ID3D12Resource*)Src.resource;
		ID3D12Resource* DstRes = (ID3D12Resource*)Dst.resource;

		D3D12_RESOURCE_BARRIER barriers[2] = {};
		barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[0].Transition.pResource = SrcRes;
		barriers[0].Transition.StateBefore = GetDX12StateFromResourceState(Src.state);
		barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

		barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[1].Transition.pResource = DstRes;
		barriers[1].Transition.StateBefore = GetDX12StateFromResourceState(Dst.state);
		barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		pCmdList->ResourceBarrier(_countof(barriers), barriers);

		CD3DX12_TEXTURE_COPY_LOCATION SrcLoc(SrcRes, 0);
		CD3DX12_TEXTURE_COPY_LOCATION DstLoc(DstRes, 0);
		CD3DX12_BOX SrcBox(0, 0, OutputExtents.X, OutputExtents.Y);

		pCmdList->CopyTextureRegion(&DstLoc, OutputPoint.X, OutputPoint.Y, 0, &SrcLoc, nullptr);

		std::swap(barriers[0].Transition.StateBefore, barriers[0].Transition.StateAfter);
		std::swap(barriers[1].Transition.StateBefore, barriers[1].Transition.StateAfter);
		pCmdList->ResourceBarrier(_countof(barriers), barriers);
	}
};
double FFXD3D12Backend::LastTime = FPlatformTime::Seconds();
float FFXD3D12Backend::AverageTime = 0.f;
float FFXD3D12Backend::AverageFPS = 0.f;
FFXD3D12Backend FFXD3D12Backend::sFFXD3D12Backend;

//-------------------------------------------------------------------------------------
// Factory/provider implementation used to create & insert the proxy swapchain.
//-------------------------------------------------------------------------------------
class FFXD3D12BackendDXGIFactory2Wrapper : public IDXGIFactory2,
#if FFX_UE_SUPPORTS_SWAPCHAIN_PROVIDER_V1
	public IDXGISwapchainProvider, 
#endif
	private FThreadSafeRefCountedObject
{
	IDXGIFactory* Inner;
	IDXGIFactory2* Inner2;
	IFFXFrameInterpolation* FFXFrameInterpolation;
	FFXD3D12Backend& Backend;
public:
	FFXD3D12BackendDXGIFactory2Wrapper(IFFXFrameInterpolation* InFFXFrameInterpolation)
	: Inner(nullptr)
	, Inner2(nullptr)
	, FFXFrameInterpolation(InFFXFrameInterpolation)
	, Backend(FFXD3D12Backend::sFFXD3D12Backend)
	{
#if FFX_UE_SUPPORTS_SWAPCHAIN_PROVIDER_V1
		IModularFeatures::Get().RegisterModularFeature("DXGISwapchainProvider", this);
#endif
	}

	void Init(IDXGIFactory2* Original)
	{
		Inner = Original;
		Inner2 = Original;
		Inner->AddRef();
		check(Inner && Inner2);
	}

	virtual ~FFXD3D12BackendDXGIFactory2Wrapper()
	{
#if FFX_UE_SUPPORTS_SWAPCHAIN_PROVIDER_V1
		IModularFeatures::Get().UnregisterModularFeature("DXGISwapchainProvider", this);
#endif
		if (Inner)
		{
			Inner->Release();
		}
	}


#if FFX_UE_SUPPORTS_SWAPCHAIN_PROVIDER_V2
	const TCHAR* GetProviderName(void) const
	{
		return SwapChainProviderName;
	}
#endif

#if FFX_UE_SUPPORTS_SWAPCHAIN_PROVIDER_V1
	bool SupportsRHI(ERHIInterfaceType RHIType) const
	{
		return RHIType == ERHIInterfaceType::D3D12;
	}

	TCHAR* GetName() const
	{
		return SwapChainProviderName;
	}

	HRESULT CreateSwapChainForHwnd(
		IDXGIFactory2* pFactory,
		IUnknown* pDevice,
		HWND hWnd,
		const DXGI_SWAP_CHAIN_DESC1* pDesc,
		const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullScreenDesc,
		IDXGIOutput* pRestrictToOutput,
		IDXGISwapChain1** ppSwapChain) override
	{
		Inner = pFactory;
		Inner2 = pFactory;
		check(Inner);
		HRESULT Res = CreateSwapChainForHwnd(pDevice, hWnd, pDesc, pFullScreenDesc, pRestrictToOutput, ppSwapChain);
		Inner = nullptr;
		Inner2 = nullptr;
		return Res;
	}

	HRESULT CreateSwapChain(
		IDXGIFactory* pFactory,
		IUnknown* pDevice,
		DXGI_SWAP_CHAIN_DESC* pDesc,
		IDXGISwapChain** ppSwapChain) override
	{
		Inner = pFactory;
		check(Inner);
		HRESULT Res = CreateSwapChain(pDevice, pDesc, ppSwapChain);
		Inner = nullptr;
		return Res;
	}
#endif


	BOOL STDMETHODCALLTYPE IsWindowedStereoEnabled(void) final
	{
		return Inner2->IsWindowedStereoEnabled();
	}

	HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd(
		/* [annotation][in] */
		_In_  IUnknown* pDevice,
		/* [annotation][in] */
		_In_  HWND hWnd,
		/* [annotation][in] */
		_In_  const DXGI_SWAP_CHAIN_DESC1* pDesc,
		/* [annotation][in] */
		_In_opt_  const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
		/* [annotation][in] */
		_In_opt_  IDXGIOutput* pRestrictToOutput,
		/* [annotation][out] */
		_COM_Outptr_  IDXGISwapChain1** ppSwapChain) final
	{
		return Inner2->CreateSwapChainForHwnd(pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
	}

	HRESULT STDMETHODCALLTYPE CreateSwapChainForCoreWindow(
		/* [annotation][in] */
		_In_  IUnknown* pDevice,
		/* [annotation][in] */
		_In_  IUnknown* pWindow,
		/* [annotation][in] */
		_In_  const DXGI_SWAP_CHAIN_DESC1* pDesc,
		/* [annotation][in] */
		_In_opt_  IDXGIOutput* pRestrictToOutput,
		/* [annotation][out] */
		_COM_Outptr_  IDXGISwapChain1** ppSwapChain) final
	{
		return Inner2->CreateSwapChainForCoreWindow(pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
	}

	HRESULT STDMETHODCALLTYPE GetSharedResourceAdapterLuid(
		/* [annotation] */
		_In_  HANDLE hResource,
		/* [annotation] */
		_Out_  LUID* pLuid) final
	{
		return Inner2->GetSharedResourceAdapterLuid(hResource, pLuid);
	}

	HRESULT STDMETHODCALLTYPE RegisterStereoStatusWindow(
		/* [annotation][in] */
		_In_  HWND WindowHandle,
		/* [annotation][in] */
		_In_  UINT wMsg,
		/* [annotation][out] */
		_Out_  DWORD* pdwCookie) final
	{
		return Inner2->RegisterStereoStatusWindow(WindowHandle, wMsg, pdwCookie);
	}

	HRESULT STDMETHODCALLTYPE RegisterStereoStatusEvent(
		/* [annotation][in] */
		_In_  HANDLE hEvent,
		/* [annotation][out] */
		_Out_  DWORD* pdwCookie) final
	{
		return Inner2->RegisterStereoStatusEvent(hEvent, pdwCookie);
	}

	void STDMETHODCALLTYPE UnregisterStereoStatus(
		/* [annotation][in] */
		_In_  DWORD dwCookie) final
	{
		Inner2->UnregisterStereoStatus(dwCookie);
	}

	HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusWindow(
		/* [annotation][in] */
		_In_  HWND WindowHandle,
		/* [annotation][in] */
		_In_  UINT wMsg,
		/* [annotation][out] */
		_Out_  DWORD* pdwCookie) final
	{
		return Inner2->RegisterOcclusionStatusWindow(WindowHandle, wMsg, pdwCookie);
	}

	HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusEvent(
		/* [annotation][in] */
		_In_  HANDLE hEvent,
		/* [annotation][out] */
		_Out_  DWORD* pdwCookie) final
	{
		return Inner2->RegisterOcclusionStatusEvent(hEvent, pdwCookie);
	}

	void STDMETHODCALLTYPE UnregisterOcclusionStatus(
		/* [annotation][in] */
		_In_  DWORD dwCookie) final
	{
		Inner2->UnregisterOcclusionStatus(dwCookie);
	}

	HRESULT STDMETHODCALLTYPE CreateSwapChainForComposition(
		/* [annotation][in] */
		_In_  IUnknown* pDevice,
		/* [annotation][in] */
		_In_  const DXGI_SWAP_CHAIN_DESC1* pDesc,
		/* [annotation][in] */
		_In_opt_  IDXGIOutput* pRestrictToOutput,
		/* [annotation][out] */
		_COM_Outptr_  IDXGISwapChain1** ppSwapChain) final
	{
		return Inner2->CreateSwapChainForComposition(pDevice, pDesc, pRestrictToOutput, ppSwapChain);
	}

	HRESULT STDMETHODCALLTYPE EnumAdapters1(
		/* [in] */ UINT Adapter,
		/* [annotation][out] */
		_COM_Outptr_  IDXGIAdapter1** ppAdapter) final
	{
		return Inner2->EnumAdapters1(Adapter, ppAdapter);
	}

	BOOL STDMETHODCALLTYPE IsCurrent(void) final
	{
		return Inner2->IsCurrent();
	}

	HRESULT STDMETHODCALLTYPE EnumAdapters(
		/* [in] */ UINT Adapter,
		/* [annotation][out] */
		_COM_Outptr_  IDXGIAdapter** ppAdapter) final
	{
		return Inner->EnumAdapters(Adapter, ppAdapter);
	}

	HRESULT STDMETHODCALLTYPE MakeWindowAssociation(
		HWND WindowHandle,
		UINT Flags) final
	{
		return Inner->MakeWindowAssociation(WindowHandle, Flags);
	}

	HRESULT STDMETHODCALLTYPE GetWindowAssociation(
		/* [annotation][out] */
		_Out_  HWND* pWindowHandle) final
	{
		return Inner->GetWindowAssociation(pWindowHandle);
	}

	HRESULT STDMETHODCALLTYPE CreateSwapChain(
		/* [annotation][in] */
		_In_  IUnknown* pDevice,
		/* [annotation][in] */
		_In_  DXGI_SWAP_CHAIN_DESC* pDesc,
		/* [annotation][out] */
		_COM_Outptr_  IDXGISwapChain** ppSwapChain) final
	{
		HRESULT Result = E_INVALIDARG;
		IDXGISwapChain* RawSwapChain = nullptr;
		FfxInterface* Interface = nullptr;
		bool const bOverrideSwapChain = ((CVarFSR3OverrideSwapChainDX12.GetValueOnAnyThread() != 0) || FParse::Param(FCommandLine::Get(), TEXT("fsr3swapchain")));
		if (bOverrideSwapChain)
		{
			FfxSwapchain ffxSwapChain = nullptr;
			ID3D12CommandQueue* CmdQueue = (ID3D12CommandQueue*)pDevice;
			check(CmdQueue);
			FfxErrorCode Code = ffxCreateFrameinterpolationSwapchainDX12(pDesc, CmdQueue, Inner, ffxSwapChain);
			if (Code == FFX_OK)
			{
				RawSwapChain = ffxGetDX12SwapchainPtr(ffxSwapChain);
				Result = S_OK;
			}
			else
			{
				Result = (HRESULT)Code;
			}
		}
		else
		{
			Result = Inner->CreateSwapChain(pDevice, pDesc, &RawSwapChain);			
		}
		if (Result == S_OK)
		{
			FIntPoint SwapChainSize = FIntPoint(pDesc->BufferDesc.Width, pDesc->BufferDesc.Height);
			uint32 Flags = 0;
			Flags |= bool(ERHIZBuffer::IsInverted) ? FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED : 0;
			Flags |= FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INFINITE;
			FfxSurfaceFormat SurfaceFormat = ffxGetSurfaceFormatDX12(pDesc->BufferDesc.Format);
			IFFXFrameInterpolationCustomPresent* CustomPresent = FFXFrameInterpolation->CreateCustomPresent(&Backend, Flags, SwapChainSize, SwapChainSize, (FfxSwapchain)RawSwapChain, (FfxCommandQueue)pDevice, SurfaceFormat, &FFXD3D12Backend::FFXFrameInterpolationUiCompositionCallback);
			if (CustomPresent)
			{
				*ppSwapChain = RawSwapChain;
				if (bOverrideSwapChain)
				{
					CustomPresent->SetMode(EFFXFrameInterpolationPresentModeNative);
				}
			}
			else
			{
				Result = E_OUTOFMEMORY;
			}
		}

		return Result;
	}

	HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(
		/* [in] */ HMODULE Module,
		/* [annotation][out] */
		_COM_Outptr_  IDXGIAdapter * *ppAdapter) final
	{
		return Inner->CreateSoftwareAdapter(Module, ppAdapter);
	}

	HRESULT STDMETHODCALLTYPE SetPrivateData(
		/* [annotation][in] */
		_In_  REFGUID Name,
		/* [in] */ UINT DataSize,
		/* [annotation][in] */
		_In_reads_bytes_(DataSize)  const void* pData) final
	{
		return Inner->SetPrivateData(Name, DataSize, pData);
	}

	HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
		/* [annotation][in] */
		_In_  REFGUID Name,
		/* [annotation][in] */
		_In_opt_  const IUnknown* pUnknown) final
	{
		return Inner->SetPrivateDataInterface(Name, pUnknown);
	}

	HRESULT STDMETHODCALLTYPE GetPrivateData(
		/* [annotation][in] */
		_In_  REFGUID Name,
		/* [annotation][out][in] */
		_Inout_  UINT* pDataSize,
		/* [annotation][out] */
		_Out_writes_bytes_(*pDataSize)  void* pData) final
	{
		return Inner->GetPrivateData(Name, pDataSize, pData);
	}

	HRESULT STDMETHODCALLTYPE GetParent(
		/* [annotation][in] */
		_In_  REFIID riid,
		/* [annotation][retval][out] */
		_COM_Outptr_  void** ppParent) final
	{
		return Inner->GetParent(riid, ppParent);
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) final
	{
		return Inner->QueryInterface(riid, ppvObject);
	}

	ULONG STDMETHODCALLTYPE AddRef(void) final
	{
		return FThreadSafeRefCountedObject::AddRef();
	}

	ULONG STDMETHODCALLTYPE Release(void) final
	{
		return FThreadSafeRefCountedObject::Release();
	}
};
static TRefCountPtr<FFXD3D12BackendDXGIFactory2Wrapper> GFFXFSR3DXGISwapChainFactory;

//-------------------------------------------------------------------------------------
// Accessor for the FD3D12Adapter on 5.1 so we can replace the DXGI factory to insert the proxy swapchain.
//-------------------------------------------------------------------------------------
#if !FFX_UE_SUPPORTS_SWAPCHAIN_PROVIDER_V1
class FFXD3D12BackendAdapter : public FD3D12Adapter
{
public:
	inline void WrapDXGIFactory()
	{
		GFFXFSR3DXGISwapChainFactory->Init(DxgiFactory2.GetReference());
		DxgiFactory2.SafeRelease();
		DxgiFactory2 = GFFXFSR3DXGISwapChainFactory;
	}
};
#endif

//-------------------------------------------------------------------------------------
// Implementation for FFXD3D12BackendModule.
//-------------------------------------------------------------------------------------
void FFXD3D12BackendModule::StartupModule()
{
	if (CVarFSR3UseNativeDX12.GetValueOnAnyThread() != 0 || FParse::Param(FCommandLine::Get(), TEXT("fsr3native")))
	{
		IFFXFrameInterpolationModule* FFXFrameInterpolationModule = FModuleManager::GetModulePtr<IFFXFrameInterpolationModule>(TEXT("FFXFrameInterpolation"));
		if (FFXFrameInterpolationModule)
		{
			IFFXFrameInterpolation* FFXFrameInterpolation = FFXFrameInterpolationModule->GetImpl();
			check(FFXFrameInterpolation);

			GFFXFSR3DXGISwapChainFactory = new FFXD3D12BackendDXGIFactory2Wrapper(FFXFrameInterpolation);

#if !FFX_UE_SUPPORTS_SWAPCHAIN_PROVIDER_V1
			auto& Adapter = ((FD3D12DynamicRHI*)GetID3D12DynamicRHI())->GetAdapter();
			FFXD3D12BackendAdapter* Wrapper = (FFXD3D12BackendAdapter*)&Adapter;
			Wrapper->WrapDXGIFactory();
#endif
		}
	}
}

void FFXD3D12BackendModule::ShutdownModule()
{
	GFFXFSR3DXGISwapChainFactory.SafeRelease();
}

IFFXSharedBackend* FFXD3D12BackendModule::GetBackend()
{
	return &FFXD3D12Backend::sFFXD3D12Backend;
}

#undef LOCTEXT_NAMESPACE