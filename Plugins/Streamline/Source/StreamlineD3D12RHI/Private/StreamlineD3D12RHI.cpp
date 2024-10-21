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

#include "StreamlineD3D12RHI.h"

#include "Features/IModularFeatures.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformMisc.h"
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_PROVIDES_ID3D12DYNAMICRHI
#include "ID3D12DynamicRHI.h"
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
#include "Windows/WindowsD3D12ThirdParty.h" // for dxgi1_6.h
#else
#include "Windows/D3D12ThirdParty.h" // for dxgi1_6.h
#endif
#else
#include "D3D12RHIPrivate.h"
THIRD_PARTY_INCLUDES_START
#include "dxgi1_6.h"
THIRD_PARTY_INCLUDES_END
#endif
#include "HAL/IConsoleManager.h"


class FD3D12Device;
#ifndef DX_MAX_MSAA_COUNT
#define DX_MAX_MSAA_COUNT	8
#endif
#if !defined(D3D12_RHI_RAYTRACING)
#define D3D12_RHI_RAYTRACING (RHI_RAYTRACING)
#endif

struct FShaderCodePackedResourceCounts;

#if ENGINE_MAJOR_VERSION < 5 || ENGINE_MINOR_VERSION < 3
#include "D3D12Util.h"
#endif

#if ENGINE_PROVIDES_ID3D12DYNAMICRHI && ENGINE_ID3D12DYNAMICRHI_NEEDS_CMDLIST
	#define RHICMDLIST_ARG_PASSTHROUGH CmdList,
#else
	#define RHICMDLIST_ARG_PASSTHROUGH 
#endif

#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Windows/IDXGISwapchainProvider.h"

#include "StreamlineAPI.h"
#include "StreamlineConversions.h"
#include "StreamlineRHI.h"
#include "sl.h"
#include "sl_dlss_g.h"



// The UE module
DEFINE_LOG_CATEGORY_STATIC(LogStreamlineD3D12RHI, Log, All);


#define LOCTEXT_NAMESPACE "StreamlineD3D12RHI"


class FStreamlineD3D12DXGISwapchainProvider : public IDXGISwapchainProvider
{
public:
	FStreamlineD3D12DXGISwapchainProvider(const FStreamlineRHI* InRHI) : StreamlineRHI(InRHI) {}

	virtual ~FStreamlineD3D12DXGISwapchainProvider() = default;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	bool SupportsRHI(ERHIInterfaceType RHIType) const override final { return RHIType == ERHIInterfaceType::D3D12; }
#else
	bool SupportsRHI(const TCHAR* RHIName) const override final { return FString(RHIName) == FString("D3D12"); }
#endif

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	const TCHAR* GetProviderName() const override final { return TEXT("FStreamlineD3D12DXGISwapchainProvider"); }
#else
	TCHAR* GetName() const override final
	{
		static TCHAR Name[] = TEXT("FStreamlineD3D12DXGISwapchainProvider");
		return Name;
	}
#endif

	HRESULT CreateSwapChainForHwnd(IDXGIFactory2* pFactory, IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullScreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) override final
	{
		HRESULT DXGIResult = E_FAIL;
		if (!StreamlineRHI->IsSwapchainHookingAllowed())
		{
			DXGIResult = pFactory->CreateSwapChainForHwnd(pDevice, hWnd, pDesc, pFullScreenDesc, pRestrictToOutput, ppSwapChain);
		}
		else
		{
			// TODO: what happens if a second swapchain is created while PIE is active?
			IDXGIFactory2* SLFactory = pFactory;
			sl::Result SLResult = SLUpgradeInterface(reinterpret_cast<void**>(&SLFactory));
			checkf(SLResult == sl::Result::eOk, TEXT("%s: error upgrading IDXGIFactory (%s)"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(sl::getResultAsStr(SLResult)));
			DXGIResult = SLFactory->CreateSwapChainForHwnd(pDevice, hWnd, pDesc, pFullScreenDesc, pRestrictToOutput, ppSwapChain);
		}

		StreamlineRHI->OnSwapchainCreated(*ppSwapChain);
		return DXGIResult;
	}

	HRESULT CreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain) override final
	{
		HRESULT DXGIResult = E_FAIL;
		if (!StreamlineRHI->IsSwapchainHookingAllowed())
		{
			DXGIResult = pFactory->CreateSwapChain(pDevice, pDesc, ppSwapChain);
		}
		else
		{
			// TODO: what happens if a second swapchain is created while PIE is active?
			IDXGIFactory* SLFactory = pFactory;
			sl::Result SLResult = SLUpgradeInterface(reinterpret_cast<void**>(&SLFactory));
			checkf(SLResult == sl::Result::eOk, TEXT("%s: error upgrading IDXGIFactory (%s)"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(sl::getResultAsStr(SLResult)));
			DXGIResult = SLFactory->CreateSwapChain(pDevice, pDesc, ppSwapChain);
		}

		StreamlineRHI->OnSwapchainCreated(*ppSwapChain);
		return DXGIResult;
	}
private:
	const FStreamlineRHI* StreamlineRHI;
};


class STREAMLINED3D12RHI_API FStreamlineD3D12RHI : public FStreamlineRHI
{
public:

	FStreamlineD3D12RHI(const FStreamlineRHICreateArguments& Arguments)
	:	FStreamlineRHI(Arguments)
#if ENGINE_PROVIDES_ID3D12DYNAMICRHI
		, D3D12RHI(CastDynamicRHI<ID3D12DynamicRHI>(Arguments.DynamicRHI))
#else
		, D3D12RHI(static_cast<FD3D12DynamicRHI*>(Arguments.DynamicRHI))
#endif
	{
		UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("%s Enter"), ANSI_TO_TCHAR(__FUNCTION__));

		check(D3D12RHI != nullptr);
#if ENGINE_PROVIDES_ID3D12DYNAMICRHI
		TArray<FD3D12MinimalAdapterDesc> AdapterDescs = D3D12RHI->RHIGetAdapterDescs();
		check(AdapterDescs.Num() > 0);
		if (AdapterDescs.Num() > 1)
		{
			UE_LOG(LogStreamlineD3D12RHI, Warning, TEXT("%s: found %d adapters, using first one found to query feature availability"), ANSI_TO_TCHAR(__FUNCTION__), AdapterDescs.Num());
		}
		const DXGI_ADAPTER_DESC& DXGIAdapterDesc = AdapterDescs[0].Desc;
#else
		const DXGI_ADAPTER_DESC& DXGIAdapterDesc = D3D12RHI->GetAdapter().GetD3DAdapterDesc();
#endif
		AdapterLuid = DXGIAdapterDesc.AdapterLuid;
		SLAdapterInfo.deviceLUID = reinterpret_cast<uint8_t*>(&AdapterLuid);
		SLAdapterInfo.deviceLUIDSizeInBytes = sizeof(AdapterLuid);
		SLAdapterInfo.vkPhysicalDevice = nullptr;

		if (IsStreamlineSupported())
		{
			TTuple<bool, FString> bSwapchainProvider = IsSwapChainProviderRequired(SLAdapterInfo);
			if (bSwapchainProvider.Get<0>())
			{
				UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("Registering FStreamlineD3D12DXGISwapchainProvider as IDXGISwapchainProvider, due to %s"), *bSwapchainProvider.Get<1>());
				CustomSwapchainProvider = MakeUnique<FStreamlineD3D12DXGISwapchainProvider>(this);
				IModularFeatures::Get().RegisterModularFeature(IDXGISwapchainProvider::GetModularFeatureName(), CustomSwapchainProvider.Get());
				bIsSwapchainProviderInstalled = true;
			}
			else
			{
				UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("Skip registering IDXGISwapchainProvider, due to %s"), *bSwapchainProvider.Get<1>());
				bIsSwapchainProviderInstalled = false;
			}
		}

		UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("%s Leave"), ANSI_TO_TCHAR(__FUNCTION__));
	}

	virtual ~FStreamlineD3D12RHI()
	{
		UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("%s Enter"), ANSI_TO_TCHAR(__FUNCTION__));
		if (CustomSwapchainProvider.IsValid())
		{
			UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("Unregistering FStreamlineD3D12DXGISwapchainProvider as IDXGISwapchainProvider"));
			IModularFeatures::Get().UnregisterModularFeature(IDXGISwapchainProvider::GetModularFeatureName(), CustomSwapchainProvider.Get());
			CustomSwapchainProvider.Reset();
		}
		UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("%s Leave"), ANSI_TO_TCHAR(__FUNCTION__));
	}

	virtual void TagTextures(FRHICommandList& CmdList, uint32 InViewID, const TArrayView<const FRHIStreamlineResource> InResources) final
	{
		if (!InResources.Num()) // IsEmpty is only 5.1+
		{
			return;
		}
		

#if ENGINE_PROVIDES_ID3D12DYNAMICRHI
		ID3D12GraphicsCommandList* NativeCmdList = nullptr;
#else
		ID3D12CommandList*         NativeCmdList = nullptr;
		FD3D12Device* D3D12Device = nullptr;
#endif

	

		for (const FRHIStreamlineResource& Resource : InResources)
		{
			if (Resource.Texture)
			{
				// that's inconsistent with below, but...
				check(Resource.Texture->IsValid());

#if ENGINE_PROVIDES_ID3D12DYNAMICRHI
				NativeCmdList = D3D12RHI->RHIGetGraphicsCommandList(RHICMDLIST_ARG_PASSTHROUGH D3D12RHI->RHIGetResourceDeviceIndex(Resource.Texture));
#else
				FD3D12TextureBase* DeviceQueryD3D12Texture = GetD3D12TextureFromRHITexture(Resource.Texture);
				D3D12Device = DeviceQueryD3D12Texture->GetParentDevice();
				NativeCmdList = D3D12Device->GetDefaultCommandContext().CommandListHandle.CommandList();
#endif

				// TODO check that all resources have the same device index. So if that ever changes we might need to split the calls into slTag into per command list/per device index calls.
				// for now we take any commandlist
				break;
			}
		}

		struct FStreamlineD3D12Transition
		{
			FRHITexture* Texture;
			D3D12_RESOURCE_STATES State;
			uint32 SubresouceIndex;
		};

		auto TransitionResource = [&](const FStreamlineD3D12Transition& Transition)
		{
#if ENGINE_PROVIDES_ID3D12DYNAMICRHI
			D3D12RHI->RHITransitionResource(CmdList, Transition.Texture, Transition.State, Transition.SubresouceIndex);
#else

			const FD3D12TextureBase* D3D12Texture = GetD3D12TextureFromRHITexture(Transition.Texture);
#if ENGINE_MAJOR_VERSION == 5
			D3D12RHI->TransitionResource(D3D12Device->GetDefaultCommandContext().CommandListHandle, D3D12Texture->GetResource(), D3D12_RESOURCE_STATE_TBD, Transition.State, Transition.SubresouceIndex, FD3D12DynamicRHI::ETransitionMode::Apply);
#else
			D3D12RHI->TransitionResource(D3D12Device->GetDefaultCommandContext().CommandListHandle, D3D12Texture->GetResource(), Transition.State, Transition.SubresouceIndex);
#endif
#endif
		};

		// adding + 1 to get to the count
		constexpr uint32 AllocatorNum = uint32(EStreamlineResource::Last) + 1;

		// if all input resources are nullptr, those arrays stay empty below
		TArray<FStreamlineD3D12Transition, TInlineAllocator<AllocatorNum>> PreTagTransitions;
		TArray<FStreamlineD3D12Transition, TInlineAllocator<AllocatorNum>> PostTagTransitions;

		// those get filled in also for null input resource so we can "Streamline nulltag" them
		TArray<sl::Resource, TInlineAllocator<AllocatorNum>> SLResources;
		TArray<sl::ResourceTag, TInlineAllocator<AllocatorNum>> SLTags;

		for(const FRHIStreamlineResource&  Resource : InResources)
		{
			sl::Resource SLResource;
			FMemory::Memzero(SLResource);
			SLResource.type = sl::ResourceType::eCount;

			sl::ResourceTag SLTag;
			SLTag.type = ToSL(Resource.StreamlineTag);
			// TODO: sl::ResourceLifecycle::eValidUntilPresent would be more efficient, are there any textures where it's applicable?
			SLTag.lifecycle = sl::ResourceLifecycle::eOnlyValidNow;

			if(Resource.Texture && Resource.Texture->IsValid())
			{
				SLResource.native = Resource.Texture->GetNativeResource();
				SLResource.type = sl::ResourceType::eTex2d;

				switch (Resource.StreamlineTag)
				{
					case EStreamlineResource::Depth:
						// note: subresources are in different states, so we add a transition sandwich
						// subresource 0 is D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
						// subresource 1 is D3D12_RESOURCE_STATE_DEPTH_WRITE

						PreTagTransitions.Add( { Resource.Texture, D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 1 });
						SLResource.state = PreTagTransitions.Last().State;

						PostTagTransitions.Add({ Resource.Texture, D3D12_RESOURCE_STATE_DEPTH_WRITE, 1 });
						break;
					case EStreamlineResource::MotionVectors:
						SLResource.state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
						break;
					case EStreamlineResource::HUDLessColor:
						SLResource.state = D3D12_RESOURCE_STATE_COPY_DEST;
						break;
					case EStreamlineResource::UIColorAndAlpha:
						SLResource.state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
						break;
					case EStreamlineResource::Backbuffer:
						SLResource.state = 0;
					case EStreamlineResource::ScalingOutputColor:
					SLResource.state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
						break;
					default:
						checkf(false, TEXT("Unimplemented tag type (streamline plugin developer should fix)"));
						SLResource.state = D3D12_RESOURCE_STATE_COMMON;
						break;
				}

				SLTag.extent = ToSL(Resource.ViewRect);

			} // if resource is valid
			else
			{
				// explicitely nulltagging so SL removes it from it's internal book keeping
				SLResource.native = nullptr;
			}

			// order matters here so we first put the resource into our array and then point the sltag at the resource in the array
			// Note: we have an TInline Allocator so our memory is pre-allocated so we should not have a re-allocation here (which then would invalidate pointers previously stored)
			SLResources.Add(SLResource);
			SLTag.resource = &SLResources.Last();
			SLTags.Add(SLTag);
		} 
		

		// transition any resources before
		for (FStreamlineD3D12Transition& Transition : PreTagTransitions)
		{
			TransitionResource(Transition);
		}


		//flush transitions
		// if we nulltag D3D12Device is nullptr and PreTagTransitions  is empty
		if (PreTagTransitions.Num())
		{
		
#if ENGINE_PROVIDES_ID3D12DYNAMICRHI
		// TODO 5.1+ support
#else
			D3D12Device->GetDefaultCommandContext().CommandListHandle.FlushResourceBarriers();
#endif
		}

		// tag all the things
		// note that NativeCmdList might be null if we only have resources to "Streamline nulltag"

		SLsetTag(sl::ViewportHandle(InViewID), SLTags.GetData(), SLTags.Num(), NativeCmdList);

		// then transition back to what was before
		for (FStreamlineD3D12Transition& Transition : PostTagTransitions)
		{
			TransitionResource(Transition);
		}

		// TODO flush transitions again?
	}

	virtual void* GetCommandBuffer(FRHICommandList& CmdList, FRHITexture* Texture) override final
	{
#if ENGINE_PROVIDES_ID3D12DYNAMICRHI
		ID3D12GraphicsCommandList* NativeCmdList = D3D12RHI->RHIGetGraphicsCommandList(RHICMDLIST_ARG_PASSTHROUGH D3D12RHI->RHIGetResourceDeviceIndex(Texture));
#else
		FD3D12TextureBase* D3D12Texture = GetD3D12TextureFromRHITexture(Texture);
		FD3D12Device* Device = D3D12Texture->GetParentDevice();
		ID3D12CommandList* NativeCmdList = Device->GetDefaultCommandContext().CommandListHandle.CommandList();
#endif
		return static_cast<void*>(NativeCmdList);
	}


	void PostStreamlineFeatureEvaluation(FRHICommandList& CmdList, FRHITexture* Texture) final
	{
#if ENGINE_PROVIDES_ID3D12DYNAMICRHI
		const uint32 DeviceIndex = D3D12RHI->RHIGetResourceDeviceIndex(Texture);
		D3D12RHI->RHIFinishExternalComputeWork(RHICMDLIST_ARG_PASSTHROUGH DeviceIndex, D3D12RHI->RHIGetGraphicsCommandList(RHICMDLIST_ARG_PASSTHROUGH DeviceIndex));
#else
		FD3D12Device* Device = D3D12RHI->GetAdapter().GetDevice(CmdList.GetGPUMask().ToIndex());
		Device->GetCommandContext().StateCache.ForceSetComputeRootSignature();
		Device->GetCommandContext().StateCache.GetDescriptorCache()->SetCurrentCommandList(Device->GetCommandContext().CommandListHandle);
#endif
	}

	virtual const sl::AdapterInfo* GetAdapterInfo() override final
	{
		return &SLAdapterInfo;
	}

	virtual bool IsDLSSGSupportedByRHI() const override final
	{
		return true;
	}
	
	virtual bool IsDeepDVCSupportedByRHI() const override final
	{
		return true;
	}	

	virtual void APIErrorHandler(const sl::APIError& LastError) final
	{
		// Not all DXGI return codes are errors, e.g. DXGI_STATUS_OCCLUDED
		if (IsDXGIStatus(LastError.hres))
		{
			return;
		}

		TCHAR ErrorMessage[1024];
		FPlatformMisc::GetSystemErrorMessage(ErrorMessage, 1024, LastError.hres);
		UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("DLSSG D3D12/DXGI Error 0x%x (%s)"), LastError.hres, ErrorMessage);

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		D3D12RHI->RHIVerifyResult(static_cast<ID3D12Device*>(D3D12RHI->RHIGetNativeDevice()), LastError.hres, "Streamline/DLSSG present", __FILE__, __LINE__);
#else


	// that should be set in the 5.1 to 4.27 backport branches that have D3D12RHI_API for VerifyD3D12Result
	// and optionally a 5.2 NVRTX branch
#if!defined HAS_VERIFYD3D12_DLL_EXPORT
#define HAS_VERIFYD3D12_DLL_EXPORT (defined (ENGINE_STREAMLINE_VERSION) && ENGINE_STREAMLINE_VERSION >=3 ) 
#endif

#if IS_MONOLITHIC || HAS_VERIFYD3D12_DLL_EXPORT
		VerifyD3D12Result(LastError.hres, "Streamline/DLSSG present", __FILE__, __LINE__,static_cast<ID3D12Device*>(GDynamicRHI->RHIGetNativeDevice()));
#else
		using VerifyD3D12ResultPtrType = void (HRESULT, const ANSICHAR* , const ANSICHAR* , uint32 , ID3D12Device*, FString );
		VerifyD3D12ResultPtrType* VerifyD3D12ResultPtr = nullptr;
		const TCHAR* VerifyD3D12ResultDemangledName = TEXT("?VerifyD3D12Result@D3D12RHI@@YAXJPEBD0IPEAUID3D12Device@@VFString@@@Z");

		const FString D3D12RHIBinaryPath = FModuleManager::Get().GetModuleFilename(FName(TEXT("D3D12RHI")));
		void*D3D12BinaryDLL = FPlatformProcess::GetDllHandle(*D3D12RHIBinaryPath);

		VerifyD3D12ResultPtr = (VerifyD3D12ResultPtrType*)(FWindowsPlatformProcess::GetDllExport(D3D12BinaryDLL, VerifyD3D12ResultDemangledName));
		UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("%s = %p"), VerifyD3D12ResultDemangledName, VerifyD3D12ResultPtr);

		if (VerifyD3D12ResultPtr)
		{
			VerifyD3D12ResultPtr(LastError.hres, "Streamline/DLSSG present", __FILE__, __LINE__, static_cast<ID3D12Device*>(GDynamicRHI->RHIGetNativeDevice()), FString());
		}
		else
		{
			UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("Please add a D3D12RHI_API to the declaration of VerifyD3D12Result in D3D12Util.h to allow non monolithic builds to pipe handling of this error into the D3D12RHI DX/DXGI error handling system"));
		}
#endif

#endif
	}

	virtual bool IsStreamlineSwapchainProxy(void* NativeSwapchain) const override final
	{
		TRefCountPtr<IUnknown> NativeInterface;
		const sl::Result Result = SLgetNativeInterface(NativeSwapchain, IID_PPV_ARGS_Helper(NativeInterface.GetInitReference()));

		if (Result == sl::Result::eOk)
		{
			const bool bIsProxy = NativeInterface != NativeSwapchain;
			//UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("%s %s NativeInterface=%p NativeSwapchain=%p isProxy=%u "), ANSI_TO_TCHAR(__FUNCTION__), *CurrentThreadName(), NativeSwapchain, NativeInterface.GetReference(), bIsProxy);
			return bIsProxy;
		}
		else
		{
			UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("SLgetNativeInterface(%p) failed (%d, %s)"), NativeSwapchain,  Result, ANSI_TO_TCHAR(sl::getResultAsStr(Result)));
		}
		return false;
	}
	

protected:

private:
#if ENGINE_PROVIDES_ID3D12DYNAMICRHI
	ID3D12DynamicRHI* D3D12RHI = nullptr;
#else
	FD3D12DynamicRHI* D3D12RHI = nullptr;
#endif
	LUID AdapterLuid;
	sl::AdapterInfo SLAdapterInfo;
	TUniquePtr<FStreamlineD3D12DXGISwapchainProvider> CustomSwapchainProvider;

};


/** IModuleInterface implementation */

void FStreamlineD3D12RHIModule::StartupModule()
{
	auto CVarInitializePlugin = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.InitializePlugin"));
	if (CVarInitializePlugin && !CVarInitializePlugin->GetBool() ||  (FParse::Param(FCommandLine::Get(), TEXT("slno"))))
	{
		UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("Initialization of StreamlineD3D12RHI is disabled."));
		return;
	}

	UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("%s Enter"), ANSI_TO_TCHAR(__FUNCTION__));
	if(FApp::CanEverRender())
	{
		if ((GDynamicRHI != nullptr) && (GDynamicRHI->GetName() == FString("D3D12")))
		{
			FStreamlineRHIModule& StreamlineRHIModule = FModuleManager::LoadModuleChecked<FStreamlineRHIModule>(TEXT("StreamlineRHI"));
			if (AreStreamlineFunctionsLoaded())
			{
				StreamlineRHIModule.InitializeStreamline();
				if (IsStreamlineSupported())
				{
					sl::Result Result = SLsetD3DDevice(GDynamicRHI->RHIGetNativeDevice());
					checkf(Result == sl::Result::eOk, TEXT("%s: SLsetD3DDevice failed (%s)"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(sl::getResultAsStr(Result)));
				}
			}
		}
		else
		{
			UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("D3D12RHI is not the active DynamicRHI; skipping of setting up the custom swapchain factory"));
		}
	}
	else
	{
		UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("This UE instance does not render, skipping initalizing of Streamline and registering of custom DXGI and D3D12 functions"));
	}
	UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("%s Leave"), ANSI_TO_TCHAR(__FUNCTION__));
}

void FStreamlineD3D12RHIModule::ShutdownModule()
{
	auto CVarInitializePlugin = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.InitializePlugin"));
	if (CVarInitializePlugin && !CVarInitializePlugin->GetBool())
	{
		return;
	}

	UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("%s Enter"), ANSI_TO_TCHAR(__FUNCTION__));
	UE_LOG(LogStreamlineD3D12RHI, Log, TEXT("%s Leave"), ANSI_TO_TCHAR(__FUNCTION__));
}

TUniquePtr<FStreamlineRHI> FStreamlineD3D12RHIModule::CreateStreamlineRHI(const FStreamlineRHICreateArguments& Arguments)
{
	TUniquePtr<FStreamlineRHI> Result(new FStreamlineD3D12RHI(Arguments));
	return Result;
}

IMPLEMENT_MODULE(FStreamlineD3D12RHIModule, StreamlineD3D12RHI )
#undef LOCTEXT_NAMESPACE
