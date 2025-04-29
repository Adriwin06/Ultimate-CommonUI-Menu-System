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

#include "FFXFrameInterpolationModule.h"
#include "LogFFXFrameInterpolation.h"
#include "FFXFrameInterpolation.h"

#include "CoreMinimal.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/ConfigCacheIni.h"
#if UE_VERSION_AT_LEAST(5, 1, 0)
#include "Misc/ConfigUtilities.h"
#endif

IMPLEMENT_MODULE(FFXFrameInterpolationModule, FFXFrameInterpolation)

#define LOCTEXT_NAMESPACE "FFXFrameInterpolation"

DEFINE_LOG_CATEGORY(LogFFXFI);

void FFXFrameInterpolationModule::StartupModule()
{
	Impl = new FFXFrameInterpolation;
    UE_LOG(LogFFXFI, Log, TEXT("FFX Frame Interpolation Module Started"));
}

void FFXFrameInterpolationModule::ShutdownModule()
{
	delete Impl;
    UE_LOG(LogFFXFI, Log, TEXT("FFX Frame Interpolation Module Shutdown"));
}

IFFXFrameInterpolation* FFXFrameInterpolationModule::GetImpl()
{
	return Impl;
}
