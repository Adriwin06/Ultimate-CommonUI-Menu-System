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

#include "XeSSSDKWrapperD3D11.h"

#include "RHI.h"
#include "XeSSSDKPrivate.h"

bool FXeSSSDKWrapperD3D11::IsSupported() const
{
#if PLATFORM_WINDOWS
	return IsRHIDeviceIntel();
#else
	return false;
#endif
}

bool FXeSSSDKWrapperD3D11::LoadRHISpecificAPIs()
{
	check(ModuleHandle);

	XESS_SDK_LOAD_API(ModuleHandle, xessD3D11CreateContext);
	XESS_SDK_LOAD_API(ModuleHandle, xessD3D11Execute);
	XESS_SDK_LOAD_API(ModuleHandle, xessD3D11Init);
	return true;
}

const TCHAR* FXeSSSDKWrapperD3D11::GetLibraryFileName() const
{
#if PLATFORM_WINDOWS
	return TEXT("libxess_dx11.dll");
#else
	return TEXT("");
#endif
}
