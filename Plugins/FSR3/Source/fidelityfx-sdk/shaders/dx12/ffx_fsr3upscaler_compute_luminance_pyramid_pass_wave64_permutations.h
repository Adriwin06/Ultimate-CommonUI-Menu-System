#include "ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c.h"

typedef union ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_PermutationKey {
    struct {
        uint32_t FFX_FSR3UPSCALER_OPTION_REPROJECT_USE_LANCZOS_TYPE : 1;
        uint32_t FFX_FSR3UPSCALER_OPTION_HDR_COLOR_INPUT : 1;
        uint32_t FFX_FSR3UPSCALER_OPTION_LOW_RESOLUTION_MOTION_VECTORS : 1;
        uint32_t FFX_FSR3UPSCALER_OPTION_JITTERED_MOTION_VECTORS : 1;
        uint32_t FFX_FSR3UPSCALER_OPTION_INVERTED_DEPTH : 1;
        uint32_t FFX_FSR3UPSCALER_OPTION_APPLY_SHARPENING : 1;
    };
    uint32_t index;
} ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_PermutationKey;

typedef struct ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_PermutationInfo {
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
} ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_PermutationInfo;

static const uint32_t g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_IndirectionTable[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

static const ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_PermutationInfo g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_PermutationInfo[] = {
    { g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_size, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_data, 2, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_CBVResourceNames, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_CBVResourceBindings, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_CBVResourceCounts, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_CBVResourceSpaces, 1, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_TextureSRVResourceNames, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_TextureSRVResourceBindings, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_TextureSRVResourceCounts, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_TextureSRVResourceSpaces, 4, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_TextureUAVResourceNames, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_TextureUAVResourceBindings, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_TextureUAVResourceCounts, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_SamplerResourceNames, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_SamplerResourceBindings, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_SamplerResourceCounts, g_ffx_fsr3upscaler_compute_luminance_pyramid_pass_wave64_d85bf09eccd25581a86b7e0ab8a9fc3c_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

