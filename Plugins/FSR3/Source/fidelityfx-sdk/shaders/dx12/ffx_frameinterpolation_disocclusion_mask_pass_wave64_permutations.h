#include "ffx_frameinterpolation_disocclusion_mask_pass_wave64_f44f016bf5ee0d398a30e8dc7155df0e.h"
#include "ffx_frameinterpolation_disocclusion_mask_pass_wave64_075d668b905ef664ca7d67c5ae3c75d3.h"

typedef union ffx_frameinterpolation_disocclusion_mask_pass_wave64_PermutationKey {
    struct {
        uint32_t FFX_FRAMEINTERPOLATION_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_frameinterpolation_disocclusion_mask_pass_wave64_PermutationKey;

typedef struct ffx_frameinterpolation_disocclusion_mask_pass_wave64_PermutationInfo {
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
} ffx_frameinterpolation_disocclusion_mask_pass_wave64_PermutationInfo;

static const uint32_t g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_IndirectionTable[] = {
    0,
    1,
};

static const ffx_frameinterpolation_disocclusion_mask_pass_wave64_PermutationInfo g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_PermutationInfo[] = {
    { g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_f44f016bf5ee0d398a30e8dc7155df0e_size, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_f44f016bf5ee0d398a30e8dc7155df0e_data, 1, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_f44f016bf5ee0d398a30e8dc7155df0e_CBVResourceNames, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_f44f016bf5ee0d398a30e8dc7155df0e_CBVResourceBindings, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_f44f016bf5ee0d398a30e8dc7155df0e_CBVResourceCounts, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_f44f016bf5ee0d398a30e8dc7155df0e_CBVResourceSpaces, 6, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_f44f016bf5ee0d398a30e8dc7155df0e_TextureSRVResourceNames, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_f44f016bf5ee0d398a30e8dc7155df0e_TextureSRVResourceBindings, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_f44f016bf5ee0d398a30e8dc7155df0e_TextureSRVResourceCounts, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_f44f016bf5ee0d398a30e8dc7155df0e_TextureSRVResourceSpaces, 1, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_f44f016bf5ee0d398a30e8dc7155df0e_TextureUAVResourceNames, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_f44f016bf5ee0d398a30e8dc7155df0e_TextureUAVResourceBindings, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_f44f016bf5ee0d398a30e8dc7155df0e_TextureUAVResourceCounts, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_f44f016bf5ee0d398a30e8dc7155df0e_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_075d668b905ef664ca7d67c5ae3c75d3_size, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_075d668b905ef664ca7d67c5ae3c75d3_data, 1, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_075d668b905ef664ca7d67c5ae3c75d3_CBVResourceNames, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_075d668b905ef664ca7d67c5ae3c75d3_CBVResourceBindings, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_075d668b905ef664ca7d67c5ae3c75d3_CBVResourceCounts, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_075d668b905ef664ca7d67c5ae3c75d3_CBVResourceSpaces, 6, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_075d668b905ef664ca7d67c5ae3c75d3_TextureSRVResourceNames, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_075d668b905ef664ca7d67c5ae3c75d3_TextureSRVResourceBindings, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_075d668b905ef664ca7d67c5ae3c75d3_TextureSRVResourceCounts, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_075d668b905ef664ca7d67c5ae3c75d3_TextureSRVResourceSpaces, 1, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_075d668b905ef664ca7d67c5ae3c75d3_TextureUAVResourceNames, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_075d668b905ef664ca7d67c5ae3c75d3_TextureUAVResourceBindings, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_075d668b905ef664ca7d67c5ae3c75d3_TextureUAVResourceCounts, g_ffx_frameinterpolation_disocclusion_mask_pass_wave64_075d668b905ef664ca7d67c5ae3c75d3_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

