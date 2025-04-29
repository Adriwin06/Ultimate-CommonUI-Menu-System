/*******************************************************************************
 * Copyright 2023 Intel Corporation
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

#pragma once

#include "CoreMinimal.h"
#include "XeSSCommonMacros.h"

#if XESS_ENGINE_WITH_XEFG_API
#include "XeSSUnrealD3D12RHIIncludes_dxgi.h"

#include "Windows/IDXGISwapchainProvider.h"

class FXeFGRHI;

class FXeFGDXGISwapChainProvider : public IDXGISwapchainProvider
{
public:
	explicit FXeFGDXGISwapChainProvider(FXeFGRHI* InXeFGRHI) : XeFGRHI(InXeFGRHI) {}
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	bool SupportsRHI(ERHIInterfaceType RHIType) const override { return RHIType == ERHIInterfaceType::D3D12; }
#else
	bool SupportsRHI(const TCHAR* RHIName) const override { return (0 == FCString::Strcmp(RHIName, TEXT("D3D12"))); }
#endif

#if XESS_ENGINE_VERSION_GEQ(5, 3)
	const TCHAR* GetProviderName() const override { return TEXT("FXeFGDXGISwapChainProvider"); };
#else
	TCHAR* GetName() const override { return TEXT("FXeFGDXGISwapChainProvider"); }
#endif

	HRESULT CreateSwapChainForHwnd(IDXGIFactory2* pFactory, IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFulScreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) override;
	HRESULT CreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain) override;

private:
	bool bXeFGSwapChainCreated = false;  // Wordaround for multiple swap chains, only override the first one
	FXeFGRHI* XeFGRHI = nullptr;
};
#endif
