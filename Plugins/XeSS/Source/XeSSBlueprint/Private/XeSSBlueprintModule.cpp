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

#include "XeSSBlueprintModule.h"

#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "XeSSBlueprintLibrary.h"

#if WITH_XESS
#include "XeSSModule.h"
#endif

void FXeSSBlueprint::StartupModule()
{
#if WITH_XESS
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FXeSSBlueprint::DelayInit);
#endif
}

void FXeSSBlueprint::ShutdownModule()
{
#if WITH_XESS
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
#endif
}

void FXeSSBlueprint::DelayInit()
{
	FXeSS* XeSS = nullptr;
#if WITH_XESS
	XeSS = &FModuleManager::LoadModuleChecked<FXeSS>(TEXT("XeSSCore"));
#endif
	UXeSSBlueprintLibrary::Init(XeSS);
}

IMPLEMENT_MODULE(FXeSSBlueprint, XeSSBlueprint)
