#include "ffx_fsr1_rcas_pass_16bit_efeb5fb331ba6590c4473d717cd53e65.h"

typedef union ffx_fsr1_rcas_pass_16bit_PermutationKey {
    struct {
        uint32_t FFX_FSR1_OPTION_APPLY_RCAS : 1;
        uint32_t FFX_FSR1_OPTION_RCAS_PASSTHROUGH_ALPHA : 1;
        uint32_t FFX_FSR1_OPTION_SRGB_CONVERSIONS : 1;
    };
    uint32_t index;
} ffx_fsr1_rcas_pass_16bit_PermutationKey;

typedef struct ffx_fsr1_rcas_pass_16bit_PermutationInfo {
    const uint32_t       blobSize;
    const unsigned char* blobData;


    const uint32_t  numConstantBuffers;
    const char**    constantBufferNames;
    const uint32_t* constantBufferBindings;
    const uint32_t* constantBufferCounts;
    const uint32_t* constantBufferSpaces;

    const uint32_t  numSRVTextures;
    const char**    srvTextureNames;
    const uint32_t* srvTextureBindings;
    const uint32_t* srvTextureCounts;
    const uint32_t* srvTextureSpaces;

    const uint32_t  numUAVTextures;
    const char**    uavTextureNames;
    const uint32_t* uavTextureBindings;
    const uint32_t* uavTextureCounts;
    const uint32_t* uavTextureSpaces;

    const uint32_t  numSRVBuffers;
    const char**    srvBufferNames;
    const uint32_t* srvBufferBindings;
    const uint32_t* srvBufferCounts;
    const uint32_t* srvBufferSpaces;

    const uint32_t  numUAVBuffers;
    const char**    uavBufferNames;
    const uint32_t* uavBufferBindings;
    const uint32_t* uavBufferCounts;
    const uint32_t* uavBufferSpaces;

    const uint32_t  numSamplers;
    const char**    samplerNames;
    const uint32_t* samplerBindings;
    const uint32_t* samplerCounts;
    const uint32_t* samplerSpaces;

    const uint32_t  numRTAccelerationStructures;
    const char**    rtAccelerationStructureNames;
    const uint32_t* rtAccelerationStructureBindings;
    const uint32_t* rtAccelerationStructureCounts;
    const uint32_t* rtAccelerationStructureSpaces;
} ffx_fsr1_rcas_pass_16bit_PermutationInfo;

static const uint32_t g_ffx_fsr1_rcas_pass_16bit_IndirectionTable[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

static const ffx_fsr1_rcas_pass_16bit_PermutationInfo g_ffx_fsr1_rcas_pass_16bit_PermutationInfo[] = {
    { g_ffx_fsr1_rcas_pass_16bit_efeb5fb331ba6590c4473d717cd53e65_size, g_ffx_fsr1_rcas_pass_16bit_efeb5fb331ba6590c4473d717cd53e65_data, 1, g_ffx_fsr1_rcas_pass_16bit_efeb5fb331ba6590c4473d717cd53e65_CBVResourceNames, g_ffx_fsr1_rcas_pass_16bit_efeb5fb331ba6590c4473d717cd53e65_CBVResourceBindings, g_ffx_fsr1_rcas_pass_16bit_efeb5fb331ba6590c4473d717cd53e65_CBVResourceCounts, g_ffx_fsr1_rcas_pass_16bit_efeb5fb331ba6590c4473d717cd53e65_CBVResourceSpaces, 1, g_ffx_fsr1_rcas_pass_16bit_efeb5fb331ba6590c4473d717cd53e65_TextureSRVResourceNames, g_ffx_fsr1_rcas_pass_16bit_efeb5fb331ba6590c4473d717cd53e65_TextureSRVResourceBindings, g_ffx_fsr1_rcas_pass_16bit_efeb5fb331ba6590c4473d717cd53e65_TextureSRVResourceCounts, g_ffx_fsr1_rcas_pass_16bit_efeb5fb331ba6590c4473d717cd53e65_TextureSRVResourceSpaces, 1, g_ffx_fsr1_rcas_pass_16bit_efeb5fb331ba6590c4473d717cd53e65_TextureUAVResourceNames, g_ffx_fsr1_rcas_pass_16bit_efeb5fb331ba6590c4473d717cd53e65_TextureUAVResourceBindings, g_ffx_fsr1_rcas_pass_16bit_efeb5fb331ba6590c4473d717cd53e65_TextureUAVResourceCounts, g_ffx_fsr1_rcas_pass_16bit_efeb5fb331ba6590c4473d717cd53e65_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

