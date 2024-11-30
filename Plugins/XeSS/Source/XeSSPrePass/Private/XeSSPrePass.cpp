/*******************************************************************************
 * Copyright 2021 Intel Corporation
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

#include "XeSSPrePass.h"

#include "XeSSMacros.h"

#include "CoreMinimal.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Launch/Resources/Version.h"
#include "ScenePrivate.h"
#include "XeSSUnreal.h"

#if XESS_ENGINE_VERSION_GEQ(5, 2)
#include "DataDrivenShaderPlatformInfo.h"
#endif // XESS_ENGINE_VERSION_GEQ(5, 2)

#define LOCTEXT_NAMESPACE "FXeSSPrePass"


const int32 GXeSSTileSizeX = FComputeShaderUtils::kGolden2DGroupSize;
const int32 GXeSSTileSizeY = FComputeShaderUtils::kGolden2DGroupSize;

class FXeSSVelocityFlattenCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FXeSSVelocityFlattenCS);
	SHADER_USE_PARAMETER_STRUCT(FXeSSVelocityFlattenCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FVector4f, InputSceneSize)
		SHADER_PARAMETER(FVector4f, OutputViewportSize)
		SHADER_PARAMETER(FVector4f, OutputViewportRect)

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GBufferVelocityTexture)

		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, ViewUniformBuffer)

		// Temporal upsample specific parameters.
		SHADER_PARAMETER(FVector2f, InputViewMin)
		SHADER_PARAMETER(FVector4f, InputViewSize)
		SHADER_PARAMETER(FVector2f, TemporalJitterPixels)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutVelocityTex)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		FPermutationDomain PermutationVector(Parameters.PermutationId);
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), GXeSSTileSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), GXeSSTileSizeY);
	}
}; // class FXeSSVelocityFlattenCS

IMPLEMENT_GLOBAL_SHADER(FXeSSVelocityFlattenCS, "/Plugin/XeSS/Private/FlattenVelocity.usf", "MainCS", SF_Compute);
DECLARE_GPU_STAT_NAMED(XeSSVelocityFlatten, TEXT("XeSS Velocity Flatten"));


FRDGTextureRef AddVelocityFlatteningXeSSPass(
	FRDGBuilder& GraphBuilder,
	FRDGTextureRef InSceneDepthTexture,
	FRDGTextureRef InVelocityTexture,
	const FViewInfo& View)
{
	check(InSceneDepthTexture);
	check(InVelocityTexture);

	RDG_GPU_STAT_SCOPE(GraphBuilder, XeSSVelocityFlatten);

	// Src rectangle.
	const FIntRect SrcRect = View.ViewRect;
	const FIntRect DestRect = FIntRect(FIntPoint::ZeroValue, View.GetSecondaryViewRectSize());

	FRDGTextureDesc SceneVelocityDesc = FRDGTextureDesc::Create2D(
		DestRect.Size(),
		PF_G16R16F,
		FClearValueBinding::Black,
		TexCreate_ShaderResource | TexCreate_UAV);

	FRDGTexture* OutputVelocityTexture = GraphBuilder.CreateTexture(
		SceneVelocityDesc,
		TEXT("Upscaled Velocity Texture"),
		ERDGTextureFlags::MultiFrame);

	{
		FXeSSVelocityFlattenCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FXeSSVelocityFlattenCS::FParameters>();

		// Setups common shader parameters
		const FIntPoint InputExtent = SrcRect.Size();
		const FIntRect InputViewRect = SrcRect;

		PassParameters->ViewUniformBuffer = View.ViewUniformBuffer;

		PassParameters->SceneDepthTexture = InSceneDepthTexture;
		PassParameters->GBufferVelocityTexture = InVelocityTexture;

		PassParameters->OutputViewportSize = FVector4f(
			DestRect.Width(), DestRect.Height(), 1.0f / float(DestRect.Width()), 1.0f / float(DestRect.Height()));
		PassParameters->OutputViewportRect = FVector4f(DestRect.Min.X, DestRect.Min.Y, DestRect.Max.X, DestRect.Max.Y);

		// Temporal upsample specific shader parameters.
		{
			PassParameters->TemporalJitterPixels = FVector2f(View.TemporalJitterPixels);
			PassParameters->InputViewMin = FVector2f(InputViewRect.Min.X, InputViewRect.Min.Y);
			PassParameters->InputViewSize = FVector4f(
				InputViewRect.Width(), InputViewRect.Height(), 1.0f / InputViewRect.Width(), 1.0f / InputViewRect.Height());
		}

		// UAVs
		{
			PassParameters->OutVelocityTex = GraphBuilder.CreateUAV(OutputVelocityTexture);
		}

		TShaderMapRef<FXeSSVelocityFlattenCS> ComputeShader(View.ShaderMap);

		ClearUnusedGraphResources(ComputeShader, PassParameters);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("XeSS %s %dx%d -> %dx%d",
				TEXT("Velocity Flattening"),
				SrcRect.Width(), SrcRect.Height(),
				DestRect.Width(), DestRect.Height()),
			ComputeShader,
			PassParameters,
			FComputeShaderUtils::GetGroupCount(DestRect.Size(), GXeSSTileSizeX));
	}

	return OutputVelocityTexture;
}

void FXeSSPrePass::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("XeSS"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/XeSS"), PluginShaderDir);
}

void FXeSSPrePass::ShutdownModule()
{

}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FXeSSPrePass, XeSSPrePass)
