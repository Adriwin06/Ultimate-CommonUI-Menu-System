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

#include <algorithm>

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <libloaderapi.h>  // NOLINT(build/include_order)
#include "Windows/HideWindowsPlatformTypes.h"
#elif PLATFORM_LINUX
#include <dlfcn.h>  // NOLINT(build/include_order)
#endif

#include "Containers/StringFwd.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Misc/StringBuilder.h"
#include "XeSSCommonUtil.h"

namespace XeSSCommon
{

struct FSDKVersion
{
	uint16_t major;
	uint16_t minor;
	uint16_t patch;
	uint16_t reserved;
};

#if PLATFORM_WINDOWS && defined(_WIN64)  // Only Win64 supported, Win32 not supported by design
const TCHAR* THIRD_PARTY_DLL_RELATIVE_PATH = TEXT("/Binaries/ThirdParty/Win64");
#elif PLATFORM_LINUX  // Linux means Linux64, no Linux32 in Unreal by design
const TCHAR* THIRD_PARTY_DLL_RELATIVE_PATH = TEXT("/Binaries/ThirdParty/Linux");
#else
const TCHAR* THIRD_PARTY_DLL_RELATIVE_PATH = TEXT("");
#endif

void PushThirdPartyDllDirectory()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("XeSS");
	FPlatformProcess::PushDllDirectory(*FPaths::Combine(Plugin->GetBaseDir(), THIRD_PARTY_DLL_RELATIVE_PATH));
}

void PopThirdPartyDllDirectory()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("XeSS");
	FPlatformProcess::PopDllDirectory(*FPaths::Combine(Plugin->GetBaseDir(), THIRD_PARTY_DLL_RELATIVE_PATH));
}

void SetSDKVersionConsoleVariable(IConsoleVariable* ConsoleVariable, const TCHAR* SDKName, void* SDKVersionData, size_t SDKVersionDataSize)
{
	FSDKVersion SDKVersion;

	check(sizeof(SDKVersion) == SDKVersionDataSize);
	memcpy(&SDKVersion, SDKVersionData, std::min(sizeof(SDKVersion), SDKVersionDataSize));

	TStringBuilder<32> VersionStringBuilder;
	VersionStringBuilder << SDKName << " version: " << SDKVersion.major << "." << SDKVersion.minor << "." << SDKVersion.patch;
	ConsoleVariable->Set(VersionStringBuilder.GetData());
}

}
