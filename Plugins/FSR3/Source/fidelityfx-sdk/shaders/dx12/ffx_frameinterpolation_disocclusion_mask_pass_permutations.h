#include "ffx_frameinterpolation_disocclusion_mask_pass_6f42120ba2ce2b46e281c0d25c4e6f2e.h"
#include "ffx_frameinterpolation_disocclusion_mask_pass_1fcc9c9d24b46f1f7b1a202927293019.h"

typedef union ffx_frameinterpolation_disocclusion_mask_pass_PermutationKey {
    struct {
        uint32_t FFX_FRAMEINTERPOLATION_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_frameinterpolation_disocclusion_mask_pass_PermutationKey;

typedef struct ffx_frameinterpolation_disocclusion_mask_pass_PermutationInfo {
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
} ffx_frameinterpolation_disocclusion_mask_pass_PermutationInfo;

static const uint32_t g_ffx_frameinterpolation_disocclusion_mask_pass_IndirectionTable[] = {
    0,
    1,
};

static const ffx_frameinterpolation_disocclusion_mask_pass_PermutationInfo g_ffx_frameinterpolation_disocclusion_mask_pass_PermutationInfo[] = {
    { g_ffx_frameinterpolation_disocclusion_mask_pass_6f42120ba2ce2b46e281c0d25c4e6f2e_size, g_ffx_frameinterpolation_disocclusion_mask_pass_6f42120ba2ce2b46e281c0d25c4e6f2e_data, 1, g_ffx_frameinterpolation_disocclusion_mask_pass_6f42120ba2ce2b46e281c0d25c4e6f2e_CBVResourceNames, g_ffx_frameinterpolation_disocclusion_mask_pass_6f42120ba2ce2b46e281c0d25c4e6f2e_CBVResourceBindings, g_ffx_frameinterpolation_disocclusion_mask_pass_6f42120ba2ce2b46e281c0d25c4e6f2e_CBVResourceCounts, g_ffx_frameinterpolation_disocclusion_mask_pass_6f42120ba2ce2b46e281c0d25c4e6f2e_CBVResourceSpaces, 6, g_ffx_frameinterpolation_disocclusion_mask_pass_6f42120ba2ce2b46e281c0d25c4e6f2e_TextureSRVResourceNames, g_ffx_frameinterpolation_disocclusion_mask_pass_6f42120ba2ce2b46e281c0d25c4e6f2e_TextureSRVResourceBindings, g_ffx_frameinterpolation_disocclusion_mask_pass_6f42120ba2ce2b46e281c0d25c4e6f2e_TextureSRVResourceCounts, g_ffx_frameinterpolation_disocclusion_mask_pass_6f42120ba2ce2b46e281c0d25c4e6f2e_TextureSRVResourceSpaces, 1, g_ffx_frameinterpolation_disocclusion_mask_pass_6f42120ba2ce2b46e281c0d25c4e6f2e_TextureUAVResourceNames, g_ffx_frameinterpolation_disocclusion_mask_pass_6f42120ba2ce2b46e281c0d25c4e6f2e_TextureUAVResourceBindings, g_ffx_frameinterpolation_disocclusion_mask_pass_6f42120ba2ce2b46e281c0d25c4e6f2e_TextureUAVResourceCounts, g_ffx_frameinterpolation_disocclusion_mask_pass_6f42120ba2ce2b46e281c0d25c4e6f2e_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_frameinterpolation_disocclusion_mask_pass_1fcc9c9d24b46f1f7b1a202927293019_size, g_ffx_frameinterpolation_disocclusion_mask_pass_1fcc9c9d24b46f1f7b1a202927293019_data, 1, g_ffx_frameinterpolation_disocclusion_mask_pass_1fcc9c9d24b46f1f7b1a202927293019_CBVResourceNames, g_ffx_frameinterpolation_disocclusion_mask_pass_1fcc9c9d24b46f1f7b1a202927293019_CBVResourceBindings, g_ffx_frameinterpolation_disocclusion_mask_pass_1fcc9c9d24b46f1f7b1a202927293019_CBVResourceCounts, g_ffx_frameinterpolation_disocclusion_mask_pass_1fcc9c9d24b46f1f7b1a202927293019_CBVResourceSpaces, 6, g_ffx_frameinterpolation_disocclusion_mask_pass_1fcc9c9d24b46f1f7b1a202927293019_TextureSRVResourceNames, g_ffx_frameinterpolation_disocclusion_mask_pass_1fcc9c9d24b46f1f7b1a202927293019_TextureSRVResourceBindings, g_ffx_frameinterpolation_disocclusion_mask_pass_1fcc9c9d24b46f1f7b1a202927293019_TextureSRVResourceCounts, g_ffx_frameinterpolation_disocclusion_mask_pass_1fcc9c9d24b46f1f7b1a202927293019_TextureSRVResourceSpaces, 1, g_ffx_frameinterpolation_disocclusion_mask_pass_1fcc9c9d24b46f1f7b1a202927293019_TextureUAVResourceNames, g_ffx_frameinterpolation_disocclusion_mask_pass_1fcc9c9d24b46f1f7b1a202927293019_TextureUAVResourceBindings, g_ffx_frameinterpolation_disocclusion_mask_pass_1fcc9c9d24b46f1f7b1a202927293019_TextureUAVResourceCounts, g_ffx_frameinterpolation_disocclusion_mask_pass_1fcc9c9d24b46f1f7b1a202927293019_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

