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

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "xess/xess.h"

namespace XeSSUtil
{
	constexpr int32 ON_SCREEN_MESSAGE_KEY_DEFAULT = -1;
	constexpr int32 ON_SCREEN_MESSAGE_KEY_INCORRECT_SCREEN_PERCENTAGE = 0;
	constexpr int32 ON_SCREEN_MESSAGE_KEY_UNSUPPORTED_RHI = 1;

	// QUALITY EDIT:
	constexpr int32 XESS_QUALITY_SETTING_MIN = XESS_QUALITY_SETTING_ULTRA_PERFORMANCE;
	constexpr int32 XESS_QUALITY_SETTING_MAX = XESS_QUALITY_SETTING_AA;
	constexpr int32 XESS_QUALITY_SETTING_COUNT = XESS_QUALITY_SETTING_MAX - XESS_QUALITY_SETTING_MIN + 1;

	inline bool IsValid(xess_quality_settings_t QualitySetting)
	{
		return QualitySetting >= XESS_QUALITY_SETTING_MIN && QualitySetting <= XESS_QUALITY_SETTING_MAX;
	}

	inline int32 ToIndex(xess_quality_settings_t QualitySetting)
	{
		check(IsValid(QualitySetting));
		return QualitySetting - XESS_QUALITY_SETTING_MIN;
	}

	inline int32 ToCVarInt(xess_quality_settings_t QualitySetting)
	{
		check(IsValid(QualitySetting));
		return QualitySetting - XESS_QUALITY_SETTING_MIN;
	}

	inline xess_quality_settings_t ToXeSSQualitySetting(int32 CVarInt)
	{
		xess_quality_settings_t QualitySetting = static_cast<xess_quality_settings_t>(XESS_QUALITY_SETTING_MIN + CVarInt);

		if (IsValid(QualitySetting))
		{
			return QualitySetting;
		}
		return XESS_QUALITY_SETTING_BALANCED;
	}

	inline void AddErrorMessageToScreen(const FString& ErrorMessage, int32 Key = ON_SCREEN_MESSAGE_KEY_DEFAULT)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(Key, 3600, FColor::Red, ErrorMessage);
		}
	}

	inline void RemoveMessageFromScreen(int32 Key)
	{
		if (GEngine)
		{
			GEngine->RemoveOnScreenDebugMessage(Key);
		}
	}
}
