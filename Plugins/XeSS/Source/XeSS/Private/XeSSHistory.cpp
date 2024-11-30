#include "XeSSHistory.h"

#if XESS_ENGINE_VERSION_GEQ(5, 3)
#include "XeSSUpscaler.h"

FXeSSHistory::FXeSSHistory(FXeSSUpscaler* InXeSSUpscaler)
{
	XeSSUpscaler = InXeSSUpscaler;
}

FXeSSHistory::~FXeSSHistory()
{
}

const TCHAR* FXeSSHistory::GetDebugName() const
{
	// WORKAROUND: use the same CHART* of the upscaler to pass check in Unreal 5.3 Preview, which is a bug 
	return XeSSUpscaler->GetDebugName();
}

uint64 FXeSSHistory::GetGPUSizeBytes() const
{
	// TODO: finish it
	return 0;
}
#endif // #if XESS_ENGINE_VERSION_GEQ(5, 3)
