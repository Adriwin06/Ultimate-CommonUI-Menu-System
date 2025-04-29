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
#pragma once

#include "CoreMinimal.h"
#include "StreamlineLibrary.h"
DECLARE_LOG_CATEGORY_EXTERN(LogStreamlineBlueprint, Verbose, All);

#if WITH_STREAMLINE

#define TRY_INIT_STREAMLINE_LIBRARY_AND_RETURN(ReturnValueOrEmptyOrVoidPreFiveThree) \
if (!TryInitStreamlineLibrary()) \
{ \
	UE_LOG(LogStreamlineBlueprint, Error, TEXT("%s should not be called before PostEngineInit"), ANSI_TO_TCHAR(__FUNCTION__)); \
	return ReturnValueOrEmptyOrVoidPreFiveThree; \
}

#else

#define TRY_INIT_STREAMLINE_LIBRARY_AND_RETURN(ReturnValueWhichCanBeEmpty) 

#endif

