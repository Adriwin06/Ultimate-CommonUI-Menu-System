#include "ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399.h"

typedef union ffx_frameinterpolation_pass_PermutationKey {
    struct {
        uint32_t FFX_FRAMEINTERPOLATION_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_frameinterpolation_pass_PermutationKey;

typedef struct ffx_frameinterpolation_pass_PermutationInfo {
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
} ffx_frameinterpolation_pass_PermutationInfo;

static const uint32_t g_ffx_frameinterpolation_pass_IndirectionTable[] = {
    0,
    0,
};

static const ffx_frameinterpolation_pass_PermutationInfo g_ffx_frameinterpolation_pass_PermutationInfo[] = {
    { g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_size, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_data, 1, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_CBVResourceNames, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_CBVResourceBindings, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_CBVResourceCounts, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_CBVResourceSpaces, 9, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_TextureSRVResourceNames, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_TextureSRVResourceBindings, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_TextureSRVResourceCounts, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_TextureSRVResourceSpaces, 1, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_TextureUAVResourceNames, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_TextureUAVResourceBindings, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_TextureUAVResourceCounts, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_SamplerResourceNames, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_SamplerResourceBindings, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_SamplerResourceCounts, g_ffx_frameinterpolation_pass_41c1786ba5298cdb6acdbb71b034b399_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

