/*
* Copyright (c) 2020 - 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
*
* NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
* property and proprietary rights in and to this material, related
* documentation and any modifications thereto. Any use, reproduction,
* disclosure or distribution of this material and related documentation
* without an express license agreement from NVIDIA CORPORATION or
* its affiliates is strictly prohibited.
*/

#include "DLSSUpscalerHistory.h"
#include "DLSSUpscalerPrivate.h"
#include "NGXRHI.h"


#include "PostProcess/SceneRenderTargets.h"
#include "PostProcess/PostProcessing.h"



#define LOCTEXT_NAMESPACE "FDLSSModule"


FDLSSUpscalerHistory::FDLSSUpscalerHistory(FDLSSStateRef InDLSSState, ENGXDLSSDenoiserMode InDenoiserMode)
	: DLSSState(InDLSSState), DenoiserMode(InDenoiserMode)
{
}

FDLSSUpscalerHistory::~FDLSSUpscalerHistory()
{
}


#undef LOCTEXT_NAMESPACE