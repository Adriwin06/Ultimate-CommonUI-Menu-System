#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#ifndef ANTILAG2_API
#if PLATFORM_WINDOWS
#define ANTILAG2_API __declspec(dllexport)
#else
#define ANTILAG2_API 
#endif
#endif

class ANTILAG2_API IAntiLag2Module : public IModuleInterface
{
public:
	virtual void MarkEndOfFrameRendering() = 0;
	virtual void SetFrameType(bool const bInterpolatedFrame) = 0;
};
