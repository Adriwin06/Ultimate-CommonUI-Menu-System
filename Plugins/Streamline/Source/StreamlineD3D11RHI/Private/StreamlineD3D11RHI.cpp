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

#include "StreamlineD3D11RHI.h"

#include "Features/IModularFeatures.h"
#include "GenericPlatform/GenericPlatformFile.h"
#if ENGINE_PROVIDES_ID3D11DYNAMICRHI
#include "ID3D11DynamicRHI.h"
#include "Windows/D3D11ThirdParty.h" // for dxgi1_6.h
#else
#include "D3D11RHIPrivate.h"
THIRD_PARTY_INCLUDES_START
#include "dxgi1_6.h"
THIRD_PARTY_INCLUDES_END
#endif

#include "D3D11Util.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Windows/IDXGISwapchainProvider.h"

#include "StreamlineAPI.h"
#include "StreamlineConversions.h"
#include "StreamlineRHI.h"
#include "sl.h"
#include "sl_dlss_g.h"


// The UE module
DEFINE_LOG_CATEGORY_STATIC(LogStreamlineD3D11RHI, Log, All);


#define LOCTEXT_NAMESPACE "StreamlineD3D11RHI"


class FStreamlineD3D11DXGISwapchainProvider : public IDXGISwapchainProvider
{
public:
	FStreamlineD3D11DXGISwapchainProvider(const FStreamlineRHI* InRHI) : StreamlineRHI(InRHI) {}

	virtual ~FStreamlineD3D11DXGISwapchainProvider() = default;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	bool SupportsRHI(ERHIInterfaceType RHIType) const override final { return RHIType == ERHIInterfaceType::D3D11; }
#else
	bool SupportsRHI(const TCHAR* RHIName) const override final { return FString(RHIName) == FString("D3D11"); }
#endif

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	const TCHAR* GetProviderName() const override final { return TEXT("FStreamlineD3D11DXGISwapchainProvider"); }
#else
	TCHAR* GetName() const override final
	{
		static TCHAR Name[] = TEXT("FStreamlineD3D11DXGISwapchainProvider");
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


class STREAMLINED3D11RHI_API FStreamlineD3D11RHI : public FStreamlineRHI
{
public:

	FStreamlineD3D11RHI(const FStreamlineRHICreateArguments& Arguments)
	:	FStreamlineRHI(Arguments)
#if ENGINE_PROVIDES_ID3D11DYNAMICRHI
		, D3D11RHI(CastDynamicRHI<ID3D11DynamicRHI>(Arguments.DynamicRHI))
#else
		, D3D11RHI(static_cast<FD3D11DynamicRHI*>(Arguments.DynamicRHI))
#endif
	{
		UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("%s Enter"), ANSI_TO_TCHAR(__FUNCTION__));

		check(D3D11RHI != nullptr);

#if ENGINE_PROVIDES_ID3D11DYNAMICRHI
		DXGI_ADAPTER_DESC DXGIAdapterDesc;
		D3D11RHI->RHIGetAdapter()->GetDesc(&DXGIAdapterDesc);
#elif ENGINE_MAJOR_VERSION > 4
		DXGI_ADAPTER_DESC DXGIAdapterDesc = D3D11RHI->GetAdapter().DXGIAdapterDesc;
#else
		ID3D11Device* NativeD3D11Device = static_cast<ID3D11Device*>(D3D11RHI->RHIGetNativeDevice());
		check(NativeD3D11Device != nullptr);

		TRefCountPtr<IDXGIDevice> DXGIDevice;
		NativeD3D11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)DXGIDevice.GetInitReference());
		check(DXGIDevice.IsValid());

		TRefCountPtr<IDXGIAdapter> DXGIAdapter;
		DXGIDevice->GetAdapter(DXGIAdapter.GetInitReference());
		check(DXGIAdapter.IsValid());

		DXGI_ADAPTER_DESC DXGIAdapterDesc;
		DXGIAdapter->GetDesc(&DXGIAdapterDesc);
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
				UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("Registering FStreamlineD3D11DXGISwapchainProvider as IDXGISwapchainProvider, due to %s"), *bSwapchainProvider.Get<1>());
				CustomSwapchainProvider = MakeUnique<FStreamlineD3D11DXGISwapchainProvider>(this);
				IModularFeatures::Get().RegisterModularFeature(IDXGISwapchainProvider::GetModularFeatureName(), CustomSwapchainProvider.Get());
				bIsSwapchainProviderInstalled = true;
			}
			else
			{
				UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("Skip registering IDXGISwapchainProvider, due to %s"), *bSwapchainProvider.Get<1>());
				bIsSwapchainProviderInstalled = false;
			}
		}

		UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("%s Leave"), ANSI_TO_TCHAR(__FUNCTION__));
	}

	virtual ~FStreamlineD3D11RHI()
	{
		UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("%s Enter"), ANSI_TO_TCHAR(__FUNCTION__));
		if (CustomSwapchainProvider.IsValid())
		{
			UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("Unregistering FStreamlineD3D11DXGISwapchainProvider as IDXGISwapchainProvider"));
			IModularFeatures::Get().UnregisterModularFeature(IDXGISwapchainProvider::GetModularFeatureName(), CustomSwapchainProvider.Get());
			CustomSwapchainProvider.Reset();
		}
		UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("%s Leave"), ANSI_TO_TCHAR(__FUNCTION__));
	}

	virtual void TagTextures(FRHICommandList& CmdList, uint32 InViewID, const TArrayView<const FRHIStreamlineResource> InResources) final
	{

#if ENGINE_PROVIDES_ID3D11DYNAMICRHI
		void* NativeCmdBuffer = D3D11RHI->RHIGetDeviceContext();
#else
		void* NativeCmdBuffer = D3D11RHI->GetDeviceContext();
#endif


		for (const FRHIStreamlineResource& Resource : InResources)
		{
			sl::Resource SLResource;
			FMemory::Memzero(SLResource);
			if (Resource.Texture && Resource.Texture->IsValid())
			{
				SLResource.native = Resource.Texture->GetNativeResource();
			}

			SLResource.type = sl::ResourceType::eTex2d;

			// no resource state in d3d11
			SLResource.state = 0;

			sl::ResourceTag Tag;
			Tag.resource = &SLResource;
			Tag.type = ToSL(Resource.StreamlineTag);
			// TODO: sl::ResourceLifecycle::eValidUntilPreset would be more efficient, are there any textures where it's applicable?
			Tag.lifecycle = sl::ResourceLifecycle::eOnlyValidNow;
			Tag.extent = ToSL(Resource.ViewRect);

			SLsetTag(sl::ViewportHandle(InViewID), &Tag, 1, NativeCmdBuffer);
			
		}
	}
	virtual void* GetCommandBuffer(FRHICommandList& CmdList, FRHITexture* Texture) override final
	{

#if ENGINE_PROVIDES_ID3D11DYNAMICRHI
		return D3D11RHI->RHIGetDeviceContext();
#else
		return D3D11RHI->GetDeviceContext();
#endif
	}

	virtual void PostStreamlineFeatureEvaluation(FRHICommandList& CmdList, FRHITexture* Texture) final
	{
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

		UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("DLSSG D3D11/DXGI Error %d"), LastError.hres);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		D3D11RHI->RHIVerifyResult(D3D11RHI->RHIGetDevice(), LastError.hres, "Streamline/DLSSG present", __FILE__, __LINE__);
#else
		VerifyD3D11Result(LastError.hres, "Streamline/DLSSG present", __FILE__, __LINE__, static_cast<ID3D11Device*>(GDynamicRHI->RHIGetNativeDevice()));
#endif
	}

	virtual bool IsStreamlineSwapchainProxy(void* NativeSwapchain) const override final
	{
		TRefCountPtr<IUnknown> NativeInterface;
		const sl::Result Result = SLgetNativeInterface(NativeSwapchain, IID_PPV_ARGS_Helper(NativeInterface.GetInitReference()));

		if (Result == sl::Result::eOk)
		{
			const bool bIsProxy = NativeInterface != NativeSwapchain;
			//UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("%s %s NativeInterface=%p NativeSwapchain=%p isProxy=%u "), ANSI_TO_TCHAR(__FUNCTION__), *CurrentThreadName(), NativeSwapchain, NativeInterface.GetReference(), bIsProxy);
			return bIsProxy;
		}
		else
		{
			UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("SLgetNativeInterface(%p) failed (%d, %s)"), NativeSwapchain, Result, ANSI_TO_TCHAR(sl::getResultAsStr(Result)));
		}
		return false;
	}


protected:

	
private:
#if ENGINE_PROVIDES_ID3D11DYNAMICRHI
	ID3D11DynamicRHI* D3D11RHI = nullptr;
#else
	FD3D11DynamicRHI* D3D11RHI = nullptr;
#endif
	LUID AdapterLuid;
	sl::AdapterInfo SLAdapterInfo;
	TUniquePtr<FStreamlineD3D11DXGISwapchainProvider> CustomSwapchainProvider;

};


/** IModuleInterface implementation */

void FStreamlineD3D11RHIModule::StartupModule()
{
	auto CVarInitializePlugin = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.InitializePlugin"));
	if (CVarInitializePlugin && !CVarInitializePlugin->GetBool() || (FParse::Param(FCommandLine::Get(), TEXT("slno"))))
	{
		UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("Initialization of StreamlineD3D11RHI is disabled."));
		return;
	}

	UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("%s Enter"), ANSI_TO_TCHAR(__FUNCTION__));
	if(FApp::CanEverRender())
	{
		if ((GDynamicRHI != nullptr) && (GDynamicRHI->GetName() == FString("D3D11")))
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
			UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("D3D11RHI is not the active DynamicRHI; skipping of setting up the custom swapchain factory"));
		}
	}
	else
	{
		UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("This UE instance does not render, skipping initalizing of Streamline and registering of custom DXGI and D3D11 functions"));
	}
	UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("%s Leave"), ANSI_TO_TCHAR(__FUNCTION__));
}

void FStreamlineD3D11RHIModule::ShutdownModule()
{
	auto CVarInitializePlugin = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.InitializePlugin"));
	if (CVarInitializePlugin && !CVarInitializePlugin->GetBool())
	{
		return;
	}

	UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("%s Enter"), ANSI_TO_TCHAR(__FUNCTION__));
	UE_LOG(LogStreamlineD3D11RHI, Log, TEXT("%s Leave"), ANSI_TO_TCHAR(__FUNCTION__));
}

TUniquePtr<FStreamlineRHI> FStreamlineD3D11RHIModule::CreateStreamlineRHI(const FStreamlineRHICreateArguments& Arguments)
{
	TUniquePtr<FStreamlineRHI> Result(new FStreamlineD3D11RHI(Arguments));
	return Result;
}

IMPLEMENT_MODULE(FStreamlineD3D11RHIModule, StreamlineD3D11RHI )
#undef LOCTEXT_NAMESPACE
