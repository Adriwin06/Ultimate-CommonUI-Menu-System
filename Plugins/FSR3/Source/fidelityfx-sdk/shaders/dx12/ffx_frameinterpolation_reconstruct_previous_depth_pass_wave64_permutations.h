#include "ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_82f4146f56d7b5208e3fcdbf665b1c2f.h"
#include "ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_e8f9cfc87bfa5eda460b869ddbf7e2e7.h"

typedef union ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_PermutationKey {
    struct {
        uint32_t FFX_FRAMEINTERPOLATION_OPTION_INVERTED_DEPTH : 1;
    };
    uint32_t index;
} ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_PermutationKey;

typedef struct ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_PermutationInfo {
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
} ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_PermutationInfo;

static const uint32_t g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_IndirectionTable[] = {
    1,
    0,
};

static const ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_PermutationInfo g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_PermutationInfo[] = {
    { g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_82f4146f56d7b5208e3fcdbf665b1c2f_size, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_82f4146f56d7b5208e3fcdbf665b1c2f_data, 1, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_82f4146f56d7b5208e3fcdbf665b1c2f_CBVResourceNames, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_82f4146f56d7b5208e3fcdbf665b1c2f_CBVResourceBindings, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_82f4146f56d7b5208e3fcdbf665b1c2f_CBVResourceCounts, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_82f4146f56d7b5208e3fcdbf665b1c2f_CBVResourceSpaces, 2, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_82f4146f56d7b5208e3fcdbf665b1c2f_TextureSRVResourceNames, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_82f4146f56d7b5208e3fcdbf665b1c2f_TextureSRVResourceBindings, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_82f4146f56d7b5208e3fcdbf665b1c2f_TextureSRVResourceCounts, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_82f4146f56d7b5208e3fcdbf665b1c2f_TextureSRVResourceSpaces, 1, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_82f4146f56d7b5208e3fcdbf665b1c2f_TextureUAVResourceNames, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_82f4146f56d7b5208e3fcdbf665b1c2f_TextureUAVResourceBindings, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_82f4146f56d7b5208e3fcdbf665b1c2f_TextureUAVResourceCounts, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_82f4146f56d7b5208e3fcdbf665b1c2f_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_e8f9cfc87bfa5eda460b869ddbf7e2e7_size, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_e8f9cfc87bfa5eda460b869ddbf7e2e7_data, 1, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_e8f9cfc87bfa5eda460b869ddbf7e2e7_CBVResourceNames, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_e8f9cfc87bfa5eda460b869ddbf7e2e7_CBVResourceBindings, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_e8f9cfc87bfa5eda460b869ddbf7e2e7_CBVResourceCounts, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_e8f9cfc87bfa5eda460b869ddbf7e2e7_CBVResourceSpaces, 2, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_e8f9cfc87bfa5eda460b869ddbf7e2e7_TextureSRVResourceNames, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_e8f9cfc87bfa5eda460b869ddbf7e2e7_TextureSRVResourceBindings, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_e8f9cfc87bfa5eda460b869ddbf7e2e7_TextureSRVResourceCounts, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_e8f9cfc87bfa5eda460b869ddbf7e2e7_TextureSRVResourceSpaces, 1, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_e8f9cfc87bfa5eda460b869ddbf7e2e7_TextureUAVResourceNames, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_e8f9cfc87bfa5eda460b869ddbf7e2e7_TextureUAVResourceBindings, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_e8f9cfc87bfa5eda460b869ddbf7e2e7_TextureUAVResourceCounts, g_ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_e8f9cfc87bfa5eda460b869ddbf7e2e7_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

