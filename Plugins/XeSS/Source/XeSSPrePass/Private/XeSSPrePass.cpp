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

#include "XeSSPrePass.h"

#include "Modules/ModuleManager.h"
#include "RenderGraphUtils.h"
#include "RHIDefinitions.h"
#include "ScenePrivate.h"
#include "XeSSUnrealCore.h"
#include "XeSSUnrealRHIIncludes.h"

const int32 GXeSSTileSizeX = FComputeShaderUtils::kGolden2DGroupSize;
const int32 GXeSSTileSizeY = FComputeShaderUtils::kGolden2DGroupSize;  // TODO(sunzhuoshi): use 16 on Xe-HPG GPUs?

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
};

IMPLEMENT_GLOBAL_SHADER(FXeSSVelocityFlattenCS, "/Plugin/XeSS/Private/FlattenVelocity.usf", "MainCS", SF_Compute);
DECLARE_GPU_STAT_NAMED(XeSSVelocityFlatten, TEXT("XeSS Velocity Flatten"));

class FXeSSVelocitySampleCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FXeSSVelocitySampleCS);
	SHADER_USE_PARAMETER_STRUCT(FXeSSVelocitySampleCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FVector4f, InputSceneSize)
		SHADER_PARAMETER(FVector4f, OutputViewportSize)
		SHADER_PARAMETER(FVector4f, OutputViewportRect)

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GBufferVelocityTexture)

		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, ViewUniformBuffer)

		// Temporal specific parameters.
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
};

IMPLEMENT_GLOBAL_SHADER(FXeSSVelocitySampleCS, "/Plugin/XeSS/Private/SampleVelocity.usf", "MainCS", SF_Compute);
DECLARE_GPU_STAT_NAMED(XeFGVelocitySample, TEXT("XeFG Velocity Sample"));

class FXeFGExtractUICS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FXeFGExtractUICS);
	SHADER_USE_PARAMETER_STRUCT(FXeFGExtractUICS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(float, AlphaThreshold)
		SHADER_PARAMETER_TEXTURE(Texture2D, BackBuffer)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutUITexture)
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
};

IMPLEMENT_GLOBAL_SHADER(FXeFGExtractUICS, "/Plugin/XeSS/Private/ExtractUI.usf", "MainCS", SF_Compute);
DECLARE_GPU_STAT_NAMED(FXeFGExtractUICS, TEXT("XeFG UI Extract"));

static FRDGTexture* AddFlattenVelocityPass(
	FRDGBuilder& GraphBuilder,
	FRDGTexture* InSceneDepthTexture,
	FRDGTexture* InVelocityTexture,
	const FViewInfo& View,
	const TCHAR* TextureName,
	FRDGEventName&& EventName)
{
	check(InSceneDepthTexture);
	check(InVelocityTexture);

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
		TextureName,
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
			DestRect.Width(), DestRect.Height(), 1.0f / static_cast<float>(DestRect.Width()), 1.0f / static_cast<float>(DestRect.Height()));
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
			std::forward<FRDGEventName>(EventName),
			ComputeShader,
			PassParameters,
			FComputeShaderUtils::GetGroupCount(DestRect.Size(), GXeSSTileSizeX));
	}

	return OutputVelocityTexture;
}

FRDGTexture* AddXeFGSampleVelocityPass(FRDGBuilder& GraphBuilder, FRDGTexture* InSceneDepthTexture, FRDGTexture* InVelocityTexture, const FViewInfo& View)
{
	check(InSceneDepthTexture);
	check(InVelocityTexture);

	const FIntRect SrcRect = View.ViewRect;
	const FIntRect DestRect = SrcRect;

	RDG_GPU_STAT_SCOPE(GraphBuilder, XeFGVelocitySample);
	FRDGTextureDesc SceneVelocityDesc = FRDGTextureDesc::Create2D(
		DestRect.Size(),
		PF_G16R16F,
		FClearValueBinding::Black,
		TexCreate_ShaderResource | TexCreate_UAV);

	FRDGTexture* OutputVelocityTexture = GraphBuilder.CreateTexture(
		SceneVelocityDesc,
		TEXT("XeFG::SampledVelocity"),
		ERDGTextureFlags::MultiFrame);

	{
		FXeSSVelocityFlattenCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FXeSSVelocityFlattenCS::FParameters>();

		// Setups common shader parameters
		PassParameters->ViewUniformBuffer = View.ViewUniformBuffer;

		PassParameters->SceneDepthTexture = InSceneDepthTexture;
		PassParameters->GBufferVelocityTexture = InVelocityTexture;

		PassParameters->OutputViewportSize = FVector4f(
			DestRect.Width(), DestRect.Height(), 1.0f / static_cast<float>(DestRect.Width()), 1.0f / static_cast<float>(DestRect.Height())
		);
		PassParameters->OutputViewportRect = FVector4f(DestRect.Min.X, DestRect.Min.Y, DestRect.Max.X, DestRect.Max.Y);

		// Temporal specific shader parameters.
		{
			PassParameters->TemporalJitterPixels = FVector2f(View.TemporalJitterPixels);
			PassParameters->InputViewMin = FVector2f(SrcRect.Min.X, SrcRect.Min.Y);
			PassParameters->InputViewSize = FVector4f(
				SrcRect.Width(), SrcRect.Height(), 1.0f / SrcRect.Width(), 1.0f / SrcRect.Height()
			);
		}

		// UAVs
		{
			PassParameters->OutVelocityTex = GraphBuilder.CreateUAV(OutputVelocityTexture);
		}

		TShaderMapRef<FXeSSVelocityFlattenCS> ComputeShader(View.ShaderMap);

		ClearUnusedGraphResources(ComputeShader, PassParameters);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("XeFG::Sample Velocity Texture (%dx%d -> %dx%d)",
				SrcRect.Width(), SrcRect.Height(),
				DestRect.Width(), DestRect.Height()
			),
			ComputeShader,
			PassParameters,
			FComputeShaderUtils::GetGroupCount(DestRect.Size(), GXeSSTileSizeX)
		);
	}
	return OutputVelocityTexture;
}

FRDGTextureRef AddXeSSFlattenVelocityPass(
	FRDGBuilder& GraphBuilder,
	FRDGTexture* InSceneDepthTexture,
	FRDGTexture* InVelocityTexture,
	const FViewInfo& View)
{
	const FIntRect SrcRect = View.ViewRect;
	const FIntRect DestRect = FIntRect(FIntPoint::ZeroValue, View.GetSecondaryViewRectSize());
	RDG_GPU_STAT_SCOPE(GraphBuilder, XeSSVelocityFlatten);

	return AddFlattenVelocityPass(GraphBuilder, InSceneDepthTexture, InVelocityTexture, View,
		TEXT("XeSS::UpscaledVelocity"),
		RDG_EVENT_NAME("XeSS::Flatten Velocity Texture (%dx%d -> %dx%d)",
			SrcRect.Width(), SrcRect.Height(),
			DestRect.Width(), DestRect.Height()
		)
	);
}

FRDGTexture* AddXeFGExtractUIPass(FRDGBuilder& GraphBuilder, float InAlphaThreshold, const XeSSUnreal::XTextureRHIRef& InBackBuffer)
{
	FIntPoint BackBufferSize = { static_cast<int32>(InBackBuffer->GetTexture2D()->GetSizeX()), static_cast<int32>(InBackBuffer->GetTexture2D()->GetSizeY()) };

	FRDGTextureDesc UITextureDesc = FRDGTextureDesc::Create2D(
		BackBufferSize,
		InBackBuffer->GetFormat(),
		FClearValueBinding::Black,
		TexCreate_ShaderResource | TexCreate_UAV
	);

	FRDGTexture* UITexture = GraphBuilder.CreateTexture(
		UITextureDesc,
		TEXT("XeFG::UIColorAndAlpha")
	);

	FXeFGExtractUICS::FParameters* PassParameters = GraphBuilder.AllocParameters<FXeFGExtractUICS::FParameters>();
	PassParameters->AlphaThreshold = FMath::Clamp(InAlphaThreshold, 0.0f, 1.0f);
	PassParameters->BackBuffer = InBackBuffer;
	PassParameters->OutUITexture = GraphBuilder.CreateUAV(UITexture);

	FXeFGExtractUICS::FPermutationDomain PermutationVector;
	TShaderMapRef<FXeFGExtractUICS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);

	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("XeFG::Extract UI"),
		ComputeShader,
		PassParameters,
		FComputeShaderUtils::GetGroupCount(BackBufferSize, GXeSSTileSizeX));

	return UITexture;
}
