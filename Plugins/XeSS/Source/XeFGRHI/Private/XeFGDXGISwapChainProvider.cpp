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

#include "XeFGDXGISwapChainProvider.h"

#if XESS_ENGINE_WITH_XEFG_API
#include "Templates/RefCounting.h"
#include "XeFGRHI.h"
#include "XeFGRHIModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogXeFGDXGISwapChainProvider, Log, All);

HRESULT FXeFGDXGISwapChainProvider::CreateSwapChainForHwnd(IDXGIFactory2* pFactory, IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFulScreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
{
	HRESULT Result = E_FAIL;
	if (!bXeFGSwapChainCreated)
	{
		TRefCountPtr<IDXGISwapChain> XeFGSwapChain;
		Result = XeFGRHI->CreateInterpolationSwapChain(hWnd, pDesc, pFulScreenDesc, pFactory, __uuidof(*ppSwapChain), XeFGSwapChain.GetInitReference());
		if (SUCCEEDED(Result))
		{
			Result = XeFGSwapChain->QueryInterface(IID_PPV_ARGS(ppSwapChain));
			check(SUCCEEDED(Result));
			bXeFGSwapChainCreated = true;
			return S_OK;
		}
		else
		{
			UE_LOG(LogXeFGDXGISwapChainProvider, Error, TEXT("Failed to create XeFG swap chain, result: %d, window: %d, will try to create a native one"), Result, hWnd);
		}
	}
	Result = pFactory->CreateSwapChainForHwnd(pDevice, hWnd, pDesc, pFulScreenDesc, pRestrictToOutput, ppSwapChain);
	if (FAILED(Result))
	{
		UE_LOG(LogXeFGDXGISwapChainProvider, Error, TEXT("Failed to create native swap chain, result: %d, window: %d"), Result, hWnd);
		return Result;
	}
	return S_OK;
}

HRESULT FXeFGDXGISwapChainProvider::CreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
{
	HRESULT Result = E_FAIL;
	if (!bXeFGSwapChainCreated)
	{
		DXGI_SWAP_CHAIN_DESC1 Desc1{};
		Desc1.Width = pDesc->BufferDesc.Width;
		Desc1.Height = pDesc->BufferDesc.Height;
		Desc1.Format = pDesc->BufferDesc.Format;
		Desc1.SampleDesc = pDesc->SampleDesc;
		Desc1.BufferUsage = pDesc->BufferUsage;
		Desc1.BufferCount = pDesc->BufferCount;
		Desc1.Scaling = DXGI_SCALING_NONE;
		Desc1.SwapEffect = pDesc->SwapEffect;
		Desc1.Flags = pDesc->Flags;

		TRefCountPtr<IDXGIFactory2> Factory2;
		Result = pFactory->QueryInterface(IID_PPV_ARGS(Factory2.GetInitReference()));
		check(SUCCEEDED(Result));  // pFactory should always be type of IDXGIFactory2, according to Unreal code from version 4.27
		Result = XeFGRHI->CreateInterpolationSwapChain(pDesc->OutputWindow, &Desc1, nullptr, Factory2, __uuidof(*ppSwapChain), ppSwapChain);
		if (SUCCEEDED(Result))
		{
			bXeFGSwapChainCreated = true;
			return S_OK;
		}
		else
		{
			UE_LOG(LogXeFGDXGISwapChainProvider, Error, TEXT("Failed to create XeFG swap chain, result: %d, will try to create a native one"), Result);
		}
	}
	TRefCountPtr<IDXGISwapChain> NativeSwapChain;
	Result = pFactory->CreateSwapChain(pDevice, pDesc, NativeSwapChain.GetInitReference());
	if (FAILED(Result))
	{
		UE_LOG(LogXeFGDXGISwapChainProvider, Error, TEXT("Failed to create native swap chain, result: %d"), Result);
		return Result;
	}
	*ppSwapChain = NativeSwapChain.GetReference();
	NativeSwapChain->AddRef();
	return S_OK;
}
#endif
