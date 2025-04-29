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

#pragma once

#include "CoreMinimal.h"
#include "xess/xess.h"

class FXeSSSDKWrapperBase
{
public:
	FXeSSSDKWrapperBase(const TCHAR* InRHIName) : RHIName(InRHIName) {}
	virtual ~FXeSSSDKWrapperBase();
	bool IsLoaded() const { return bIsLoaded; }
	bool IsRHI(const TCHAR* InRHIName) const { return 0 == FCString::Strcmp(InRHIName, *RHIName); }
	bool Load();
	const TCHAR* GetRHIName() const { return *RHIName; }
	virtual bool IsSupported() const = 0;
	virtual const TCHAR* GetLibraryFileName() const = 0;
public:
	typedef xess_result_t(*xessGetVersion_t)(xess_version_t*);
	xessGetVersion_t xessGetVersion = nullptr;
	typedef xess_result_t(*xessGetIntelXeFXVersion_t)(xess_context_handle_t, xess_version_t*);
	xessGetIntelXeFXVersion_t xessGetIntelXeFXVersion = nullptr;
	typedef xess_result_t(*xessGetOptimalInputResolution_t)(xess_context_handle_t, const xess_2d_t*, xess_quality_settings_t, xess_2d_t*, xess_2d_t*, xess_2d_t*);
	xessGetOptimalInputResolution_t xessGetOptimalInputResolution = nullptr;
	typedef xess_result_t(*xessDestroyContext_t)(xess_context_handle_t);
	xessDestroyContext_t xessDestroyContext = nullptr;
	typedef xess_result_t(*xessSetLoggingCallback_t)(xess_context_handle_t, xess_logging_level_t, xess_app_log_callback_t);
	xessSetLoggingCallback_t xessSetLoggingCallback;
protected:
	virtual bool LoadRHISpecificAPIs() = 0;
	void* ModuleHandle = nullptr;
private:
	bool LoadGenericAPIs();
	bool bIsLoaded = false;
	FString RHIName;
};
