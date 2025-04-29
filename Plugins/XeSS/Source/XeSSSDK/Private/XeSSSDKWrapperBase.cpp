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

#include "XeSSSDKWrapperBase.h"

#include "XeSSCommonUtil.h"
#include "XeSSSDKPrivate.h"

FXeSSSDKWrapperBase::~FXeSSSDKWrapperBase()
{
	if (ModuleHandle)
	{
		FPlatformProcess::FreeDllHandle(ModuleHandle);
		ModuleHandle = nullptr;
	}
}

bool FXeSSSDKWrapperBase::Load()
{
	if (!ModuleHandle)
	{
		bIsLoaded = false;
		if (IsSupported())
		{
			const TCHAR* LibraryFileName = GetLibraryFileName();
			check(LibraryFileName && 0 < FCString::Strlen(LibraryFileName));

			XeSSCommon::PushThirdPartyDllDirectory();
			ModuleHandle = FPlatformProcess::GetDllHandle(LibraryFileName);
			XeSSCommon::PopThirdPartyDllDirectory();
			if (ModuleHandle)
			{
				bIsLoaded = LoadGenericAPIs();
				if (bIsLoaded)
				{
					bIsLoaded = LoadRHISpecificAPIs();
				}
			}
			else
			{
				UE_LOG(LogXeSSSDKModule, Error, TEXT("Failed to load dynamic library, file name: %s"), LibraryFileName);
			}
		}
	}
	return bIsLoaded;
}

bool FXeSSSDKWrapperBase::LoadGenericAPIs()
{
	check(ModuleHandle);

	XESS_SDK_LOAD_API(ModuleHandle, xessGetVersion);
	XESS_SDK_LOAD_API(ModuleHandle, xessGetIntelXeFXVersion);
	XESS_SDK_LOAD_API(ModuleHandle, xessGetOptimalInputResolution);
	XESS_SDK_LOAD_API(ModuleHandle, xessDestroyContext);
	XESS_SDK_LOAD_API(ModuleHandle, xessSetLoggingCallback);
	return true;
}
