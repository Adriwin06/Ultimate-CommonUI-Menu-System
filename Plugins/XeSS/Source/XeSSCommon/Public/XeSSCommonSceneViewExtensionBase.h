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

#include "SceneViewExtension.h"
#include "XeSSCommonMacros.h"

class FXeSSCommonSceneViewExtensionBase : public FSceneViewExtensionBase
{
public:
	explicit FXeSSCommonSceneViewExtensionBase(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) {}
	void BeginRenderViewFamily(FSceneViewFamily& ViewFamily) override {}
	void SetupView(FSceneViewFamily& ViewFamily, FSceneView& View) override {}
	void SetupViewFamily(FSceneViewFamily& ViewFamily) override {}

protected:
	// InOptionalView will be nullptr for Unreal versions below 5.5
	XESSCOMMON_API virtual void SubscribeToPostProcessingPass_Unified(EPostProcessingPass Pass, const FSceneView* InOptionalView, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) {}

private:
#if ENGINE_MAJOR_VERSION == 4
	void PreRenderViewFamily_RenderThread(FRHICommandListImmediate&, FSceneViewFamily&) override {}
	void PreRenderView_RenderThread(FRHICommandListImmediate&, FSceneView&) override {}
#endif
#if XESS_ENGINE_VERSION_GEQ(5, 5)
	XESSCOMMON_API void SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;
#else
	XESSCOMMON_API void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;
#endif
};
