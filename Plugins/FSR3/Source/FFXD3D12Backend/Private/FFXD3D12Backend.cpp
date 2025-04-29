// This file is part of the FidelityFX Super Resolution 3.1 Unreal Engine Plugin.
//
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include "IAntiLag2.h"

#include <mutex>

#define LOCTEXT_NAMESPACE "FFXD3D12Backend"

DECLARE_LOG_CATEGORY_EXTERN(LogFFXD3D12, Verbose, All);
DEFINE_LOG_CATEGORY(LogFFXD3D12);

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

#if UE_VERSION_AT_LEAST(5, 0, 0)
#define FFX_TEXTURE_CREATE_FLAGS(Name) ETextureCreateFlags::Name
#else
#define FFX_TEXTURE_CREATE_FLAGS(Name) TexCreate_##Name
#endif

static ETextureCreateFlags ffxGetSurfaceFlagsDX12ToUE(D3D12_RESOURCE_FLAGS flags)
{
	ETextureCreateFlags NewFlags = FFX_TEXTURE_CREATE_FLAGS(None);
	switch(flags)
	{
        case D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET:
			NewFlags |= FFX_TEXTURE_CREATE_FLAGS(RenderTargetable);
			NewFlags |= FFX_TEXTURE_CREATE_FLAGS(ShaderResource);
			break;
        case D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL:
			NewFlags |= FFX_TEXTURE_CREATE_FLAGS(DepthStencilTargetable);
			NewFlags |= FFX_TEXTURE_CREATE_FLAGS(ShaderResource);
			break;
        case D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS:
			NewFlags |= FFX_TEXTURE_CREATE_FLAGS(UAV);
			NewFlags |= FFX_TEXTURE_CREATE_FLAGS(ShaderResource);
			break;
        case D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE:
			NewFlags |= FFX_TEXTURE_CREATE_FLAGS(DisableSRVCreation);
			NewFlags &= ~FFX_TEXTURE_CREATE_FLAGS(ShaderResource);
			break;
        case D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER:
        case D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS:
			NewFlags |= FFX_TEXTURE_CREATE_FLAGS(Shared);
			break;
        case D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY:
#if UE_VERSION_AT_LEAST(5, 0, 0)
        case D3D12_RESOURCE_FLAG_VIDEO_ENCODE_REFERENCE_ONLY:
#endif
		case D3D12_RESOURCE_FLAG_NONE:
		default:
			break;
	}
	return NewFlags;
}

// {BEED74B2-282E-4AA3-BBF7-534560507A45}
static const GUID IID_IFfxFrameInterpolationSwapChain = { 0xbeed74b2, 0x282e, 0x4aa3, {0xbb, 0xf7, 30, 0x45, 0x60, 0x50, 0x7a, 0x45} };
static const GUID IID_IFfxD3D12SwapChain = { 0x51a371da, 0x9961, 0x438c, {0x81, 0x21, 0xe2, 0x67, 0xf4, 0x30, 0x78, 0x30} };

class DECLSPEC_UUID("51A371DA-9961-438C-8121-E267F4307830") FFXD3D12SwapChain : public IDXGISwapChain4, private FThreadSafeRefCountedObject
{
public:
	FFXD3D12SwapChain(IFFXSharedBackend* InBackend, ffxContext InContext, IDXGISwapChain4* InSwapChain)
	: Context(InContext)
	, SwapChain(InSwapChain)
	, Backend(InBackend)
	{
	}
	virtual ~FFXD3D12SwapChain()
	{
		Backend->ffxDestroyContext(&Context);
	}

	ffxContext* GetContext()
	{
		return &Context;
	}

	IDXGISwapChain4* GetSwapChain()
	{
		return SwapChain;
	}

	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject)
	{
		const GUID guidReplacements[] = {
		__uuidof(this),
		IID_IUnknown,
		IID_IDXGIObject,
		IID_IDXGIDeviceSubObject,
		IID_IDXGISwapChain,
		IID_IDXGISwapChain1,
		IID_IDXGISwapChain2,
		IID_IDXGISwapChain3,
		IID_IDXGISwapChain4,
		IID_IFfxD3D12SwapChain
		};

		for (auto guid : guidReplacements)
		{
			if (IsEqualGUID(riid, guid) == 1)
			{
				AddRef();
				*ppvObject = this;
				return S_OK;
			}
		}

		if (IsEqualGUID(riid, IID_IFfxFrameInterpolationSwapChain) == 1)
		{
			SwapChain->AddRef();
			*ppvObject = SwapChain;
			return S_OK;
		}

		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef(void) final
	{
		return FThreadSafeRefCountedObject::AddRef();
	}

	ULONG STDMETHODCALLTYPE Release(void) final
	{
		return FThreadSafeRefCountedObject::Release();
	}

	// IDXGIObject
	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void* pData)
	{
		return SwapChain->SetPrivateData(Name, DataSize, pData);
	}
	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown)
	{
		return SwapChain->SetPrivateDataInterface(Name, pUnknown);
	}
	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData)
	{
		return SwapChain->GetPrivateData(Name, pDataSize, pData);
	}
	virtual HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void** ppParent)
	{
		return SwapChain->GetParent(riid, ppParent);
	}

	// IDXGIDeviceSubObject
	virtual HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void** ppDevice)
	{
		return SwapChain->GetDevice(riid, ppDevice);
	}

	// IDXGISwapChain1
	virtual HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags)
	{
		return SwapChain->Present(SyncInterval, Flags);
	}
	virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void** ppSurface)
	{
		return SwapChain->GetBuffer(Buffer, riid, ppSurface);
	}
	virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, IDXGIOutput* pTarget)
	{
		return SwapChain->SetFullscreenState(Fullscreen, pTarget);
	}
	virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL* pFullscreen, IDXGIOutput** ppTarget)
	{
		return SwapChain->GetFullscreenState(pFullscreen, ppTarget);
	}
	virtual HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC* pDesc)
	{
		return SwapChain->GetDesc(pDesc);
	}
	virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
	{
		return SwapChain->ResizeBuffers(BufferCount, Width, Height, NewFormat, SwapChainFlags);
	}
	virtual HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters)
	{
		return SwapChain->ResizeTarget(pNewTargetParameters);
	}
	virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput** ppOutput)
	{
		return SwapChain->GetContainingOutput(ppOutput);
	}
	virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats)
	{
		return SwapChain->GetFrameStatistics(pStats);
	}
	virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT* pLastPresentCount)
	{
		return SwapChain->GetLastPresentCount(pLastPresentCount);
	}

	// IDXGISwapChain1
	virtual HRESULT STDMETHODCALLTYPE GetDesc1(DXGI_SWAP_CHAIN_DESC1* pDesc)
	{
		return SwapChain->GetDesc1(pDesc);
	}
	virtual HRESULT STDMETHODCALLTYPE GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pDesc)
	{
		return SwapChain->GetFullscreenDesc(pDesc);
	}
	virtual HRESULT STDMETHODCALLTYPE GetHwnd(HWND* pHwnd)
	{
		return SwapChain->GetHwnd(pHwnd);
	}
	virtual HRESULT STDMETHODCALLTYPE GetCoreWindow(REFIID refiid, void** ppUnk)
	{
		return SwapChain->GetCoreWindow(refiid, ppUnk);
	}
	virtual HRESULT STDMETHODCALLTYPE Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters)
	{
		return SwapChain->Present1(SyncInterval, PresentFlags, pPresentParameters);
	}
	virtual BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported(void)
	{
		return SwapChain->IsTemporaryMonoSupported();
	}
	virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput(IDXGIOutput** ppRestrictToOutput)
	{
		return SwapChain->GetRestrictToOutput(ppRestrictToOutput);
	}
	virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor(const DXGI_RGBA* pColor)
	{
		return SwapChain->SetBackgroundColor(pColor);
	}
	virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor(DXGI_RGBA* pColor)
	{
		return SwapChain->GetBackgroundColor(pColor);
	}
	virtual HRESULT STDMETHODCALLTYPE SetRotation(DXGI_MODE_ROTATION Rotation)
	{
		return SwapChain->SetRotation(Rotation);
	}
	virtual HRESULT STDMETHODCALLTYPE GetRotation(DXGI_MODE_ROTATION* pRotation)
	{
		return SwapChain->GetRotation(pRotation);
	}

	// IDXGISwapChain2
	virtual HRESULT STDMETHODCALLTYPE SetSourceSize(UINT Width, UINT Height)
	{
		return SwapChain->SetSourceSize(Width, Height);
	}
	virtual HRESULT STDMETHODCALLTYPE GetSourceSize(UINT* pWidth, UINT* pHeight)
	{
		return SwapChain->GetSourceSize(pWidth, pHeight);
	}
	virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT MaxLatency)
	{
		return SwapChain->SetMaximumFrameLatency(MaxLatency);
	}
	virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(UINT* pMaxLatency)
	{
		return SwapChain->GetMaximumFrameLatency(pMaxLatency);
	}
	virtual HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject(void)
	{
		return SwapChain->GetFrameLatencyWaitableObject();
	}
	virtual HRESULT STDMETHODCALLTYPE SetMatrixTransform(const DXGI_MATRIX_3X2_F* pMatrix)
	{
		return SwapChain->SetMatrixTransform(pMatrix);
	}
	virtual HRESULT STDMETHODCALLTYPE GetMatrixTransform(DXGI_MATRIX_3X2_F* pMatrix)
	{
		return SwapChain->GetMatrixTransform(pMatrix);
	}

	// IDXGISwapChain3
	virtual UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex(void)
	{
		return SwapChain->GetCurrentBackBufferIndex();
	}
	virtual HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(DXGI_COLOR_SPACE_TYPE ColorSpace, UINT* pColorSpaceSupport)
	{
		return SwapChain->CheckColorSpaceSupport(ColorSpace, pColorSpaceSupport);
	}
	virtual HRESULT STDMETHODCALLTYPE SetColorSpace1(DXGI_COLOR_SPACE_TYPE ColorSpace)
	{
		return SwapChain->SetColorSpace1(ColorSpace);
	}
	virtual HRESULT STDMETHODCALLTYPE ResizeBuffers1(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue)
	{
		return SwapChain->ResizeBuffers1(BufferCount, Width, Height, Format, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
	}

	// IDXGISwapChain4
	virtual HRESULT STDMETHODCALLTYPE SetHDRMetaData(DXGI_HDR_METADATA_TYPE Type, UINT Size, void* pMetaData)
	{
		return SwapChain->SetHDRMetaData(Type, Size, pMetaData);
	}

private:
	ffxContext Context;
	IDXGISwapChain4* SwapChain;
	IFFXSharedBackend* Backend;
};

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

	FFXSharedAllocCallbacks AllocCbs;
	ffxFunctions FfxFunctions;
	void* FfxModule;
	TQueue<TPair<uint64, FFXFrameResources>> FrameResources;
	uint32 NumPresentsIssued;
	static double LastTime;
	static float AverageTime;
	static float AverageFPS;
public:
	static FFXD3D12Backend sFFXD3D12Backend;

	FFXD3D12Backend()
	{
		NumPresentsIssued = 0;
		FMemory::Memzero(FfxFunctions);
		FfxModule = nullptr;
	}

	virtual ~FFXD3D12Backend()
	{
		if (FfxModule)
		{
			FPlatformProcess::FreeDllHandle(FfxModule);
		}
	}

	bool LoadDLL()
	{
		bool bOk = false;

		FString Name = TEXT("amd_fidelityfx_dx12.dll");

#if WITH_EDITOR
		FModuleStatus ModuleStatus;
		if (FModuleManager::Get().QueryModule(TEXT("FFXD3D12Backend"), ModuleStatus))
		{
			FString Dir = FPaths::Combine(FPaths::GetPath(ModuleStatus.FilePath), TEXT("../../Source/fidelityfx-sdk/PrebuiltSignedDLL"));
			FPaths::CollapseRelativeDirectories(Dir);
			FPlatformProcess::AddDllDirectory(*Dir);
			Name = FPaths::Combine(Dir, Name);
		}
#endif

		FfxModule = FPlatformProcess::GetDllHandle(*Name);
		if (FfxModule)
		{
			ffxLoadFunctions(&FfxFunctions, (HMODULE)FfxModule);
			bOk = true;
		}

		return bOk;
	}

	ffxReturnCode_t ffxCreateContext(ffxContext* context, ffxCreateContextDescHeader* desc) final
	{
		ffxCreateBackendDX12Desc Dx12Header;
		Dx12Header.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
		Dx12Header.header.pNext = nullptr;
		Dx12Header.device = (ID3D12Device*)GDynamicRHI->RHIGetNativeDevice();
		desc->pNext = (ffxApiHeader*)&Dx12Header;

		return FfxFunctions.CreateContext(context, desc, &AllocCbs.Cbs);
	}

	ffxReturnCode_t ffxDestroyContext(ffxContext* context) final
	{
		return FfxFunctions.DestroyContext(context, &AllocCbs.Cbs);
	}

	ffxReturnCode_t ffxConfigure(ffxContext* context, const ffxConfigureDescHeader* desc) final
	{
		return FfxFunctions.Configure(context, desc);
	}

	ffxReturnCode_t ffxQuery(ffxContext* context, ffxQueryDescHeader* desc) final
	{
		return FfxFunctions.Query(context, desc);
	}

	ffxReturnCode_t ffxDispatch(ffxContext* context, const ffxDispatchDescHeader* desc) final
	{
		return FfxFunctions.Dispatch(context, desc);
	}

	void Init() final
	{

	}

	EFFXBackendAPI GetAPI() const
	{
		return EFFXBackendAPI::D3D12;
	}
	void SetFeatureLevel(ffxContext* context, ERHIFeatureLevel::Type FeatureLevel) final
	{

	}
	FfxSwapchain GetSwapchain(void* swapChain) final
	{
		FFXD3D12SwapChain* SwapChain4 = nullptr;
		if (swapChain)
		{
			((IDXGISwapChain1*)swapChain)->QueryInterface<FFXD3D12SwapChain>(&SwapChain4);
			if (SwapChain4)
				((IDXGISwapChain1*)swapChain)->Release();
		}

		return reinterpret_cast<FfxSwapchain>(SwapChain4);
	}

	FfxApiResource GetNativeResource(FRHITexture* Texture, FfxApiResourceState State) final
	{
		return ffxApiGetResourceDX12((ID3D12Resource*)Texture->GetNativeResource(), State);
	}

	FfxApiResource GetNativeResource(FRDGTexture* Texture, FfxApiResourceState State) final
	{
		return GetNativeResource(Texture->GetRHI(), State);
	}

	FfxCommandList GetNativeCommandBuffer(FRHICommandListImmediate& RHICmdList, FRHITexture* Texture) final
	{
#if UE_VERSION_AT_LEAST(5, 5, 0)
		ID3D12DynamicRHI* DynamicRHI = GetID3D12DynamicRHI();
		uint32 const DeviceIndex = DynamicRHI->RHIGetResourceDeviceIndex(Texture);
		FfxCommandList CmdList = reinterpret_cast<FfxCommandList>((ID3D12CommandList*)DynamicRHI->RHIGetGraphicsCommandList(RHICmdList, DeviceIndex));
#elif UE_VERSION_AT_LEAST(5, 1, 0)
		ID3D12DynamicRHI* DynamicRHI = GetID3D12DynamicRHI();
		uint32 const DeviceIndex = DynamicRHI->RHIGetResourceDeviceIndex(Texture);
		FfxCommandList CmdList = reinterpret_cast<FfxCommandList>((ID3D12CommandList*)DynamicRHI->RHIGetGraphicsCommandList(DeviceIndex));
#else
		void* CmdList = ((FD3D12CommandContext&)RHICmdList.GetContext()).CommandListHandle.GraphicsCommandList();
#endif
		return CmdList;
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

	static FfxResource FFXConvertResource(FfxApiResource ApiResource)
	{
		FfxResource Resource;
		FMemory::Memzero(Resource);

		Resource.resource = ApiResource.resource;
		Resource.state = (FfxResourceStates)ApiResource.state;
		memcpy(&Resource.description, &ApiResource.description, sizeof(FfxResourceDescription));

		return Resource;
	}

	static ffxReturnCode_t FFXFrameInterpolationUiCompositionCallback(ffxCallbackDescFrameGenerationPresent* params, void* unusedUserCtx)
	{
		if (!params->isGeneratedFrame)
		{
			sFFXD3D12Backend.ReleaseFrameResources(params->frameID);
		}

		FfxPresentCallbackDescription PresentParams;
		PresentParams.device = (FfxDevice)params->device;
		PresentParams.commandList = (FfxCommandList)params->commandList;
		PresentParams.isInterpolatedFrame = params->isGeneratedFrame;
		PresentParams.frameID = params->frameID;
		PresentParams.usePremulAlpha = false;
		PresentParams.currentBackBuffer = FFXConvertResource(params->currentBackBuffer);
		PresentParams.currentUI = FFXConvertResource(params->currentUI);
		PresentParams.outputSwapChainBuffer = FFXConvertResource(params->outputSwapChainBuffer);

		ffxFrameInterpolationUiComposition(&PresentParams, unusedUserCtx);

		if (params->isGeneratedFrame)
		{
			FFX_RENDER_TEST_CAPTURE_PASS_BEGIN_DX12(TEXT("FFXFrameInterpolationUiCompositionCallback"));
				FFX_RENDER_TEST_CAPTURE_PASS_ADD_DX12(params->device, params->commandList, PresentParams.outputSwapChainBuffer.resource, (uint32_t)GetDX12StateFromResourceState(PresentParams.outputSwapChainBuffer.state), 3, "SwapChainBuffer");
			FFX_RENDER_TEST_CAPTURE_PASS_END_DX12;
		}

		{
			double CurrentTime = FPlatformTime::Seconds();
			float FrameTimeMS = (float)((CurrentTime - LastTime) * 1000.0);
			AverageTime = AverageTime * 0.75f + FrameTimeMS * 0.25f;
			LastTime = CurrentTime;
			AverageFPS = 1000.f / AverageTime;

			if (CVarFFXFIUpdateGlobalFrameTime.GetValueOnAnyThread() != 0 && (CVarEnableFFXFI.GetValueOnAnyThread() != 0))
			{
				GAverageMS = AverageTime;
				GAverageFPS = AverageFPS;
			}
		}

		IAntiLag2Module* AntiLag2Interface = (IAntiLag2Module*)FModuleManager::Get().GetModule(TEXT("AntiLag2"));
		if (AntiLag2Interface)
		{
			AntiLag2Interface->SetFrameType(PresentParams.isInterpolatedFrame);
		}

		return FFX_API_RETURN_OK;
	}

	void UpdateSwapChain(ffxContext* Context, ffxConfigureDescFrameGeneration& Desc)
	{
		if (Context)
		{
			FFXD3D12SwapChain* SwapChain = (FFXD3D12SwapChain*)Desc.swapChain;
			Desc.swapChain = Desc.swapChain ? SwapChain->GetSwapChain() : nullptr;
			Desc.presentCallback = Desc.swapChain ? &FFXFrameInterpolationUiCompositionCallback : nullptr;

			auto Code = ffxConfigure(Context, &Desc.header);
			check(Code == FFX_API_RETURN_OK);
		}
	}

	void UpdateSwapChain(ffxContext* Context, ffxConfigureDescFrameGeneration& Desc, ffxConfigureDescFrameGenerationRegisterDistortionFieldResource& DescDistortion)
	{
		Desc.header.pNext = &(DescDistortion.header);
		UpdateSwapChain(Context, Desc);
	}

	FfxApiResource GetInterpolationOutput(FfxSwapchain SwapChain)
	{
		FfxApiResource Output;
		FMemory::Memzero(Output);

		FFXD3D12SwapChain* D3D12Swapchain = (FFXD3D12SwapChain*)SwapChain;
		if (D3D12Swapchain)
		{
			ffxQueryDescFrameGenerationSwapChainInterpolationTextureDX12 Desc;
			FMemory::Memzero(Desc);
			Desc.header.type = FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONTEXTURE_DX12;
			Desc.pOutTexture = &Output;
			auto Code = ffxQuery(D3D12Swapchain->GetContext(), &Desc.header);
			check(Code == FFX_API_RETURN_OK);
		}

		return Output;
	}

	void* GetInterpolationCommandList(FfxSwapchain SwapChain)
	{
		FfxCommandList CmdList = nullptr;

		FFXD3D12SwapChain* D3D12Swapchain = (FFXD3D12SwapChain*)SwapChain;
		if (D3D12Swapchain)
		{
			ffxQueryDescFrameGenerationSwapChainInterpolationCommandListDX12 Desc;
			FMemory::Memzero(Desc);
			Desc.header.type = FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONCOMMANDLIST_DX12;
			Desc.pOutCommandList = &CmdList;
			auto Code = ffxQuery(D3D12Swapchain->GetContext(), &Desc.header);
			check(Code == FFX_API_RETURN_OK);
		}
		
		return CmdList;
	}

	void RegisterFrameResources(FRHIResource* FIResources, uint64 FrameID) final
	{
		FFXFrameResources Resources;
		Resources.FIResources = FIResources;
		FrameResources.Enqueue(TPair<uint64, FFXFrameResources>(FrameID, Resources));
	}

	void ReleaseFrameResources(uint64 FrameID)
	{
		TPair<uint64, FFXFrameResources> Resources;
		while (FrameResources.Peek(Resources) && ((FrameID - (Resources.Key <= FrameID ? Resources.Key : 0llu)) > (3llu)))
		{
			FrameResources.Pop();
		}
		NumPresentsIssued = FrameID;
	}

	bool GetAverageFrameTimes(float& AvgTimeMs, float& AvgFPS) final
	{
		AvgTimeMs = AverageTime;
		AvgFPS = AverageFPS;
		return true;
	}

	void CopySubRect(FfxCommandList CmdList, FfxApiResource Src, FfxApiResource Dst, FIntPoint OutputExtents, FIntPoint OutputPoint) final
	{
		ID3D12GraphicsCommandList* pCmdList = (ID3D12GraphicsCommandList*)CmdList;
		ID3D12Resource* SrcRes = (ID3D12Resource*)Src.resource;
		ID3D12Resource* DstRes = (ID3D12Resource*)Dst.resource;

		D3D12_RESOURCE_BARRIER barriers[2] = {};
		barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[0].Transition.pResource = SrcRes;
		barriers[0].Transition.StateBefore = GetDX12StateFromResourceState((FfxResourceStates)Src.state);
		barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

		barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[1].Transition.pResource = DstRes;
		barriers[1].Transition.StateBefore = GetDX12StateFromResourceState((FfxResourceStates)Dst.state);
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

	void Flush(FRHITexture* Tex, FRHICommandListImmediate& RHICmdList) final
	{
#if UE_VERSION_OLDER_THAN(5, 1, 0)
		RHICmdList.SubmitCommandsHint();
#else
		RHICmdList.EnqueueLambda([this, Tex](FRHICommandListImmediate& cmd)
		{
			ID3D12DynamicRHI* DynamicRHI = GetID3D12DynamicRHI();
			uint32 const DeviceIndex = DynamicRHI->RHIGetResourceDeviceIndex(Tex);
#if UE_VERSION_AT_LEAST(5, 5, 0)
			DynamicRHI->RHIFinishExternalComputeWork(cmd, DeviceIndex, (ID3D12GraphicsCommandList*)GetNativeCommandBuffer(cmd, Tex));
#else
			DynamicRHI->RHIFinishExternalComputeWork(DeviceIndex, (ID3D12GraphicsCommandList*)GetNativeCommandBuffer(cmd, Tex));
#endif
		});
#endif
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
		HRESULT Result = E_INVALIDARG;
		FFXD3D12SwapChain* D3D12SwapChain = nullptr;
		IDXGISwapChain* RawSwapChain = nullptr;
		FfxInterface* Interface = nullptr;
		bool bOverrideSwapChain = ((CVarFSR3OverrideSwapChainDX12.GetValueOnAnyThread() != 0) || FParse::Param(FCommandLine::Get(), TEXT("fsr3swapchain")));

		// Don't override the swapchain in the Editor - it causes too many issues
#if WITH_EDITORONLY_DATA
		bOverrideSwapChain &= !GIsEditor;
#endif

		HWND ParentWindow = hWnd ? ::GetParent(hWnd) : nullptr;
		if (bOverrideSwapChain && !ParentWindow)
		{
			ID3D12CommandQueue* CmdQueue = (ID3D12CommandQueue*)pDevice;
			check(CmdQueue);

			DXGI_SWAP_CHAIN_DESC1 DescCopy = *pDesc;
			DXGI_SWAP_CHAIN_FULLSCREEN_DESC FullscreenDescCopy = *pFullscreenDesc;

			ffxCreateContextDescFrameGenerationSwapChainForHwndDX12 SwapChainDesc = {};
			SwapChainDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12;
			SwapChainDesc.header.pNext = nullptr;
			SwapChainDesc.swapchain = (IDXGISwapChain4**)&RawSwapChain;
			SwapChainDesc.hwnd = hWnd;
			SwapChainDesc.desc = &DescCopy;
			SwapChainDesc.fullscreenDesc = &FullscreenDescCopy;
			SwapChainDesc.dxgiFactory = Inner;
			SwapChainDesc.gameQueue = CmdQueue;

			ffxContext SwapChain = nullptr;
			auto ReturnCode = Backend.ffxCreateContext(&SwapChain, (ffxCreateContextDescHeader*)&SwapChainDesc);
			if (ReturnCode == FFX_API_RETURN_OK)
			{
				D3D12SwapChain = new FFXD3D12SwapChain(&Backend, SwapChain, (IDXGISwapChain4*)RawSwapChain);
				D3D12SwapChain->AddRef();
				Result = S_OK;

				if (FParse::Param(FCommandLine::Get(), TEXT("fsr3hudless")))
				{
					CVarFFXFIUIMode->Set(1);
				}
				if (FParse::Param(FCommandLine::Get(), TEXT("fsr3async")))
				{
					CVarFSR3AllowAsyncWorkloads->Set(1);
				}
			}
			else
			{
				Result = (HRESULT)ReturnCode;
			}
		}
		else
		{
			Result = Inner2->CreateSwapChainForHwnd(pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, (IDXGISwapChain1**)&RawSwapChain);
		}
		if (Result == S_OK)
		{
			FIntPoint SwapChainSize = FIntPoint(pDesc->Width, pDesc->Height);
			uint32 Flags = 0;
			Flags |= bool(ERHIZBuffer::IsInverted) ? FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED : 0;
			Flags |= FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INFINITE;
			FfxApiSurfaceFormat SurfaceFormat = (FfxApiSurfaceFormat)ffxApiGetSurfaceFormatDX12(pDesc->Format);
			IFFXFrameInterpolationCustomPresent* CustomPresent = FFXFrameInterpolation->CreateCustomPresent(&Backend, Flags, SwapChainSize, SwapChainSize, (FfxSwapchain)(D3D12SwapChain ? D3D12SwapChain : RawSwapChain), (FfxCommandQueue)pDevice, SurfaceFormat, EFFXBackendAPI::D3D12);
			if (CustomPresent)
			{
				*ppSwapChain = (IDXGISwapChain1*)(D3D12SwapChain ? D3D12SwapChain : RawSwapChain);
				if (bOverrideSwapChain)
				{
					bool bAllowAsyncWorkloads = (CVarFSR3AllowAsyncWorkloads.GetValueOnAnyThread() != 0);
					bool bUIMode = (CVarFFXFIUIMode.GetValueOnAnyThread() != 0);
					if (bAllowAsyncWorkloads || bUIMode)
					{
						CustomPresent->SetMode(EFFXFrameInterpolationPresentModeNative);
					}
					else
					{
						CustomPresent->SetUseFFXSwapchain(true);
					}
				}
			}
			else
			{
				Result = E_OUTOFMEMORY;
			}
		}

		return Result;
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
		FFXD3D12SwapChain* D3D12SwapChain = nullptr;
		IDXGISwapChain* RawSwapChain = nullptr;
		FfxInterface* Interface = nullptr;
		bool bOverrideSwapChain = ((CVarFSR3OverrideSwapChainDX12.GetValueOnAnyThread() != 0) || FParse::Param(FCommandLine::Get(), TEXT("fsr3swapchain")));

		// Don't override the swapchain in the Editor - it causes too many issues
#if WITH_EDITORONLY_DATA
		bOverrideSwapChain &= !GIsEditor;
#endif

		HWND ParentWindow = pDesc->OutputWindow ? ::GetParent(pDesc->OutputWindow) : nullptr;
		if (bOverrideSwapChain && !ParentWindow)
		{
			ID3D12CommandQueue* CmdQueue = (ID3D12CommandQueue*)pDevice;
			check(CmdQueue);

			ffxCreateContextDescFrameGenerationSwapChainNewDX12 SwapChainDesc = {};
			SwapChainDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_NEW_DX12;
			SwapChainDesc.header.pNext = nullptr;
			SwapChainDesc.swapchain = (IDXGISwapChain4**)&RawSwapChain;
			SwapChainDesc.desc = pDesc;
			SwapChainDesc.dxgiFactory = Inner;
			SwapChainDesc.gameQueue = CmdQueue;

			ffxContext SwapChain = nullptr;
			auto ReturnCode = Backend.ffxCreateContext(&SwapChain, (ffxCreateContextDescHeader*)&SwapChainDesc);
			if (ReturnCode == FFX_API_RETURN_OK)
			{
				D3D12SwapChain = new FFXD3D12SwapChain(&Backend, SwapChain, (IDXGISwapChain4*)RawSwapChain);
				D3D12SwapChain->AddRef();
				Result = S_OK;

				if (FParse::Param(FCommandLine::Get(), TEXT("fsr3hudless")))
				{
					CVarFFXFIUIMode->Set(1);
				}
				if (FParse::Param(FCommandLine::Get(), TEXT("fsr3async")))
				{
					CVarFSR3AllowAsyncWorkloads->Set(1);
				}
			}
			else
			{
				Result = (HRESULT)ReturnCode;
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
			FfxApiSurfaceFormat SurfaceFormat = (FfxApiSurfaceFormat)ffxApiGetSurfaceFormatDX12(pDesc->BufferDesc.Format);
			IFFXFrameInterpolationCustomPresent* CustomPresent = FFXFrameInterpolation->CreateCustomPresent(&Backend, Flags, SwapChainSize, SwapChainSize, (FfxSwapchain)(D3D12SwapChain ? D3D12SwapChain : RawSwapChain), (FfxCommandQueue)pDevice, SurfaceFormat, EFFXBackendAPI::D3D12);
			if (CustomPresent)
			{
				*ppSwapChain = D3D12SwapChain ? D3D12SwapChain : RawSwapChain;
				if (bOverrideSwapChain)
				{
					bool bAllowAsyncWorkloads = (CVarFSR3AllowAsyncWorkloads.GetValueOnAnyThread() != 0);
					bool bUIMode = (CVarFFXFIUIMode.GetValueOnAnyThread() != 0);
					if (bAllowAsyncWorkloads || bUIMode)
					{
						CustomPresent->SetMode(EFFXFrameInterpolationPresentModeNative);
					}
					else
					{
						CustomPresent->SetUseFFXSwapchain(true);
					}
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
// Accessor for the FD3D12Adapter on <= 5.1 so we can replace the DXGI factory to insert the proxy swapchain.
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
	if (!FParse::Param(FCommandLine::Get(), TEXT("fsr3rhi")) && (CVarFSR3UseNativeDX12.GetValueOnAnyThread() != 0 || FParse::Param(FCommandLine::Get(), TEXT("fsr3native"))))
	{
		if (FFXD3D12Backend::sFFXD3D12Backend.LoadDLL())
		{

			IFFXFrameInterpolationModule* FFXFrameInterpolationModule = FModuleManager::GetModulePtr<IFFXFrameInterpolationModule>(TEXT("FFXFrameInterpolation"));
			bool bOverrideSwapChain = FFXFrameInterpolationModule != nullptr;

			// Don't override the swapchain in the Editor - it causes too many issues
#if WITH_EDITORONLY_DATA
			bOverrideSwapChain &= !GIsEditor;
#endif
			if (bOverrideSwapChain)
			{
				IFFXFrameInterpolation* FFXFrameInterpolation = FFXFrameInterpolationModule->GetImpl();
				check(FFXFrameInterpolation);

				GFFXFSR3DXGISwapChainFactory = new FFXD3D12BackendDXGIFactory2Wrapper(FFXFrameInterpolation);

#if !FFX_UE_SUPPORTS_SWAPCHAIN_PROVIDER_V1
#if UE_VERSION_AT_LEAST(5, 1, 0)
				auto& Adapter = ((FD3D12DynamicRHI*)GetID3D12DynamicRHI())->GetAdapter();
#else
				static const auto CVarHDRMinLuminanceLog10 = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HDR.Display.MinLuminanceLog10"));
				static const auto CVarHDRMaxLuminance = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HDR.Display.MaxLuminance"));

				auto& Adapter = ((FD3D12DynamicRHI*)GDynamicRHI)->GetAdapter();

				IDXGIAdapter* DXGIAdapter = Adapter.GetAdapter();

				for (uint32 DisplayIndex = 0; true; ++DisplayIndex)
				{
					TRefCountPtr<IDXGIOutput> DXGIOutput;
					if (S_OK != DXGIAdapter->EnumOutputs(DisplayIndex, DXGIOutput.GetInitReference()))
					{
						break;
					}

					TRefCountPtr<IDXGIOutput6> Output6;
					if (SUCCEEDED(DXGIOutput->QueryInterface(IID_PPV_ARGS(Output6.GetInitReference()))))
					{
						DXGI_OUTPUT_DESC1 OutputDesc;
						if (Output6->GetDesc1(&OutputDesc) == S_OK)
						{
							// Check for HDR support on the display.
							const bool bDisplaySupportsHDROutput = (OutputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
							if (bDisplaySupportsHDROutput)
							{
								CVarHDRMaxLuminance->Set(OutputDesc.MaxLuminance, ECVF_SetByConstructor);
								CVarHDRMinLuminanceLog10->Set(static_cast<float>(log10(OutputDesc.MinLuminance)), ECVF_SetByConstructor);
								break;
							}
						}
					}
				}

#endif
				FFXD3D12BackendAdapter* Wrapper = (FFXD3D12BackendAdapter*)&Adapter;
				Wrapper->WrapDXGIFactory();
#endif
			}
		}
		else if (FParse::Param(FCommandLine::Get(), TEXT("fsr3native")))
		{
			UE_LOG(LogFFXD3D12, Fatal, TEXT("FSR3 D3D12 Module Could Not Load amd_fidelityfx_dx12.dll when forcing its use with -fsr3native"));
		}
		else
		{
			CVarFSR3UseNativeDX12->Set(0, ECVF_SetByCode);
			CVarFSR3UseRHI->Set(1, ECVF_SetByCode);
			UE_LOG(LogFFXD3D12, Error, TEXT("FSR3 D3D12 Module Could Not Load amd_fidelityfx_dx12.dll - falling back to RHI implementation"));
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