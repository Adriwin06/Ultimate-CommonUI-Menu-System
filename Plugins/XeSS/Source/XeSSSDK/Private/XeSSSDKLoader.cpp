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

#include "XeSSSDKLoader.h"

#include "XeSSCommonUtil.h"
#include "XeSSSDKWrapperD3D11.h"
#include "XeSSSDKWrapperD3D12.h"
#include "XeSSSDKWrapperVulkan.h"

static TUniquePtr<FXeSSSDKWrapperBase> XeSSSDKWrapper;

FXeSSSDKWrapperBase* FXeSSSDKLoader::Load(const TCHAR* InRHIName)
{
	FString RHIName(InRHIName);
	if (XeSSSDKWrapper)
	{
		// Only one RHI loading supported to simplify code
		check(RHIName == XeSSSDKWrapper->GetRHIName());
		return XeSSSDKWrapper.Get();
	}

	if (RHIName == TEXT("D3D12"))
	{
		XeSSSDKWrapper.Reset(new FXeSSSDKWrapperD3D12(InRHIName));
	}
	else if (RHIName == TEXT("D3D11"))
	{
		XeSSSDKWrapper.Reset(new FXeSSSDKWrapperD3D11(InRHIName));
	}
	else if (RHIName == TEXT("Vulkan"))
	{
		XeSSSDKWrapper.Reset(new FXeSSSDKWrapperVulkan(InRHIName));
	}
	if (XeSSSDKWrapper)
	{
		XeSSSDKWrapper->Load();
	}
	return XeSSSDKWrapper.Get();
}

void FXeSSSDKLoader::Unload()
{
	XeSSSDKWrapper.Reset(nullptr);
}
