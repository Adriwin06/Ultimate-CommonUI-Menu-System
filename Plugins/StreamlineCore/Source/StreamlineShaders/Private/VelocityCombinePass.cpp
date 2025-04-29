/*
* Copyright (c) 2022 - 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
*
* NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
* property and proprietary rights in and to this material, related
* documentation and any modifications thereto. Any use, reproduction,
* disclosure or distribution of this material and related documentation
* without an express license agreement from NVIDIA CORPORATION or
* its affiliates is strictly prohibited.
*/

#include "VelocityCombinePass.h"

#include "Runtime/Launch/Resources/Version.h"
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 2)
#include "DataDrivenShaderPlatformInfo.h"
#endif
#include "SceneRendering.h"
#include "ShaderPermutation.h"
#include "ScenePrivate.h"

const FIntPoint kVelocityCombineComputeTileSize ( FComputeShaderUtils::kGolden2DGroupSize, FComputeShaderUtils::kGolden2DGroupSize);

class FStreamlineVelocityCombineCS : public FGlobalShader
{
public:
	class FDilateMotionVectorsDim : SHADER_PERMUTATION_BOOL("DILATE_MOTION_VECTORS");
	class FSupportAlternateMotionVectorDim : SHADER_PERMUTATION_BOOL("SUPPORT_ALTERNATE_MOTION_VECTOR");
	using FPermutationDomain = TShaderPermutationDomain<FDilateMotionVectorsDim, FSupportAlternateMotionVectorDim>;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		// Only cook for the platforms/RHIs where DLSS-FG is supported, which is DX11,DX12 [on Win64]
		return 	IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
				IsPCPlatform(Parameters.Platform) &&
#if (ENGINE_MAJOR_VERSION == 4) && (ENGINE_MINOR_VERSION <= 26)
				IsD3DPlatform(Parameters.Platform, false);
#else
				IsD3DPlatform(Parameters.Platform);
#endif
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), kVelocityCombineComputeTileSize.X);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), kVelocityCombineComputeTileSize.Y);
	}

	DECLARE_GLOBAL_SHADER(FStreamlineVelocityCombineCS);
	SHADER_USE_PARAMETER_STRUCT(FStreamlineVelocityCombineCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Input images
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, VelocityTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, VelocityTextureSampler)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Velocity)

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DepthTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, DepthTextureSampler)

#if DLSS_ENGINE_USES_FVECTOR2D
		SHADER_PARAMETER(FVector2D, TemporalJitterPixels)
#else
		SHADER_PARAMETER(FVector2f, TemporalJitterPixels)
#endif

		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		
		// Output images
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutVelocityCombinedTexture)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, CombinedVelocity)

		// motion vectors to consider instead of the standard ones from the engine
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float2>, AlternateMotionVectorsTexture)

	END_SHADER_PARAMETER_STRUCT()
};


IMPLEMENT_GLOBAL_SHADER(FStreamlineVelocityCombineCS, "/Plugin/StreamlineCore/Private/VelocityCombine.usf", "VelocityCombineMain", SF_Compute);

FRDGTextureRef AddStreamlineVelocityCombinePass(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FRDGTextureRef InSceneDepthTexture,
	FRDGTextureRef InVelocityTexture,
	FRDGTextureRef AlternateMotionVectorTexture,
	bool bDilateMotionVectors
)
{
	const FIntRect InputViewRect = View.ViewRect;
	const FIntRect OutputViewRect = FIntRect( FIntPoint::ZeroValue, bDilateMotionVectors ? View.GetSecondaryViewRectSize() : View.ViewRect.Size());
	FRDGTextureDesc CombinedVelocityDesc =

	FRDGTextureDesc::Create2D(
		OutputViewRect.Size(),
		PF_G16R16F,
		FClearValueBinding::Black,
		TexCreate_ShaderResource | TexCreate_UAV);
	const TCHAR* OutputName = TEXT("Streamline.CombinedVelocity");

	FRDGTextureRef CombinedVelocityTexture = GraphBuilder.CreateTexture(
		CombinedVelocityDesc,
		OutputName);

	FStreamlineVelocityCombineCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FStreamlineVelocityCombineCS::FParameters>();

	const bool bHasAlternateMotionVectors = AlternateMotionVectorTexture != nullptr;

	// input velocity
	{
		PassParameters->VelocityTexture = InVelocityTexture;
		PassParameters->VelocityTextureSampler = TStaticSamplerState<SF_Point>::GetRHI();

		// we use InSceneDepthTexture here and not InVelocityTexture since the latter can be a 1x1 black texture
		check(InVelocityTexture->Desc.Extent == FIntPoint(1, 1) || InVelocityTexture->Desc.Extent == InSceneDepthTexture->Desc.Extent);
		FScreenPassTextureViewport velocityViewport(InSceneDepthTexture, InputViewRect);
		FScreenPassTextureViewportParameters velocityViewportParameters = GetScreenPassTextureViewportParameters(velocityViewport);
		PassParameters->Velocity = velocityViewportParameters;
	}
	// input depth
	{
		PassParameters->DepthTexture = InSceneDepthTexture;
		PassParameters->DepthTextureSampler = TStaticSamplerState<SF_Point>::GetRHI();
	}
	// replacement motion vectors for items like reflections that DLSS might prefer to track	
	{
		PassParameters->AlternateMotionVectorsTexture = AlternateMotionVectorTexture;
	}

	// output combined velocity
	{
		PassParameters->OutVelocityCombinedTexture = GraphBuilder.CreateUAV(CombinedVelocityTexture);

		FScreenPassTextureViewport CombinedVelocityViewport(CombinedVelocityTexture, OutputViewRect);
		FScreenPassTextureViewportParameters CombinedVelocityViewportParameters = GetScreenPassTextureViewportParameters(CombinedVelocityViewport);
		PassParameters->CombinedVelocity = CombinedVelocityViewportParameters;
	}

	// various state
	{

#if ENGINE_MAJOR_VERSION < 5
		PassParameters->TemporalJitterPixels = View.TemporalJitterPixels;
#else
		PassParameters->TemporalJitterPixels = FVector2f(View.TemporalJitterPixels); // LWC_TODO: Precision loss
#endif
		PassParameters->View = View.ViewUniformBuffer;
	}

	FStreamlineVelocityCombineCS::FPermutationDomain PermutationVector;
	PermutationVector.Set<FStreamlineVelocityCombineCS::FDilateMotionVectorsDim>(bDilateMotionVectors);
	PermutationVector.Set<FStreamlineVelocityCombineCS::FSupportAlternateMotionVectorDim>(bHasAlternateMotionVectors);

	TShaderMapRef<FStreamlineVelocityCombineCS> ComputeShader(View.ShaderMap, PermutationVector);

	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("Velocity Combine%s%s (%dx%d -> %dx%d)", 
			bDilateMotionVectors ? TEXT(" Dilate") : TEXT(""),
			bHasAlternateMotionVectors ? TEXT(" AlternateMotionVectors") : TEXT("SceneMotionVectors"),
			InputViewRect.Width(), InputViewRect.Height(),
			OutputViewRect.Width(), OutputViewRect.Height()
		),
		ComputeShader,
		PassParameters,
		FComputeShaderUtils::GetGroupCount(OutputViewRect.Size(), kVelocityCombineComputeTileSize));
		
	return CombinedVelocityTexture;
}