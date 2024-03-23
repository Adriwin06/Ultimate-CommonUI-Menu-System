#pragma once

#include "XeSSMacros.h"

#if XESS_ENGINE_VERSION_GEQ(5, 3)
#include "CoreMinimal.h"
#include "TemporalUpscaler.h"

class FXeSSUpscaler;

class FXeSSHistory final : public UE::Renderer::Private::ITemporalUpscaler::IHistory
{
public:
	FXeSSHistory(FXeSSUpscaler* InXeSSUpscaler);
	virtual ~FXeSSHistory();
	virtual const TCHAR* GetDebugName() const final;
	virtual uint64 GetGPUSizeBytes() const final;
	virtual uint32 Release() const final { return --RefCount; }
	virtual uint32 AddRef() const final { return ++RefCount; }
	virtual uint32 GetRefCount() const final { return RefCount; }
private:
	mutable uint32 RefCount = 0;
	FXeSSUpscaler* XeSSUpscaler = nullptr;
};
#endif // #if XESS_ENGINE_VERSION_GEQ(5, 3)
