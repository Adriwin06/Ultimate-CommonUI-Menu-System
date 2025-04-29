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

#include "XeSSCommonMacros.h"

#if XESS_ENGINE_VERSION_GEQ(5, 1)
#include "Misc/ConfigUtilities.h"
#include "IVulkanDynamicRHI.h"
#else
#include "Misc/ConfigCacheIni.h"
#include "VulkanRHIBridge.h"
#endif

#if ENGINE_MAJOR_VERSION >= 5
#include "SceneTextures.h"
#else
#include "PostProcess/SceneRenderTargets.h"
#endif

#if XESS_ENGINE_VERSION_GEQ(5, 3)
#include "PostProcess/PostProcessMaterialInputs.h"
#else
#include "PostProcess/PostProcessMaterial.h"
#endif
#include "RenderGraphResources.h"
#include "RHICommandList.h"
#include "SceneRendering.h"
#include "ScreenPass.h"
#include "VulkanRHIPrivate.h"
#include "XeSSUnrealCore.h"
#include "XeSSUnrealRenderer.h"
#include "XeSSUnrealRHI.h"
#include "XeSSUnrealVulkanRHI.h"

namespace XeSSUnreal
{

void ApplyCVarSettingsFromIni(const TCHAR* InSectionBaseName, const TCHAR* InIniFilename, uint32 SetBy, bool bAllowCheating)
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	UE::ConfigUtilities::ApplyCVarSettingsFromIni(InSectionBaseName, InIniFilename, SetBy, bAllowCheating);
#else
	::ApplyCVarSettingsFromIni(InSectionBaseName, InIniFilename, SetBy, bAllowCheating);
#endif
}

XRHIBuffer* GetRHIBuffer(TRDGBufferAccess<ERHIAccess::UAVCompute> BufferAccess)
{
#if ENGINE_MAJOR_VERSION >= 5
	return BufferAccess->GetRHI();
#else
	return BufferAccess->GetRHIStructuredBuffer();
#endif
}

void* LockRHIBuffer(FRHICommandListImmediate& CommandList, XRHIBuffer* Buffer, uint32 Offset, uint32 SizeRHI, EResourceLockMode LockMode)
{
#if ENGINE_MAJOR_VERSION >= 5
	return CommandList.LockBuffer(Buffer, Offset, SizeRHI, LockMode);
#else
	return CommandList.LockStructuredBuffer(Buffer, Offset, SizeRHI, LockMode);
#endif
}

void UnlockRHIBuffer(FRHICommandListImmediate& CommandList, XRHIBuffer* Buffer)
{
#if ENGINE_MAJOR_VERSION >= 5
	CommandList.UnlockBuffer(Buffer);
#else
	CommandList.UnlockStructuredBuffer(Buffer);
#endif
}

const XSceneTextures& GetSceneTextures(const FViewInfo& ViewInfo, FRDGBuilder& GraphBuilder)
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	(void)GraphBuilder;
	return ViewInfo.GetSceneTextures();
#elif XESS_ENGINE_VERSION_GEQ(5, 0)
	(void)ViewInfo;
	return FSceneTextures::Get(GraphBuilder);
#else
	return FSceneRenderTargets::Get(GraphBuilder.RHICmdList);
#endif
}

FRDGTexture* GetSceneDepthTexture(const XSceneTextures& SceneTextures, FRDGBuilder& GraphBuilder)
{
#if ENGINE_MAJOR_VERSION >= 5
	(void)GraphBuilder;
	return SceneTextures.Depth.Resolve;
#else
	return GraphBuilder.RegisterExternalTexture(SceneTextures.SceneDepthZ);
#endif
}

FScreenPassTexture ReturnUntouchedSceneColorForPostProcessing(const FPostProcessMaterialInputs& Inputs, FRDGBuilder& GraphBuilder)
{
#if XESS_ENGINE_VERSION_GEQ(5, 4)
	return Inputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
#else
	if (Inputs.OverrideOutput.IsValid())
	{
		return Inputs.OverrideOutput;
	}
	else
	{
		return FScreenPassTexture(Inputs.GetInput(EPostProcessMaterialInput::SceneColor));
	}
#endif
}

FScreenPassTexture GetInputScreenPassTexture(const FPostProcessMaterialInputs& Inputs, EPostProcessMaterialInput Input, FRDGBuilder& GraphBuilder)
{
#if XESS_ENGINE_VERSION_GEQ(5, 4)
	return FScreenPassTexture::CopyFromSlice(GraphBuilder, Inputs.GetInput(Input));
#else
	(void)GraphBuilder;
	return Inputs.GetInput(Input);
#endif
}

}
