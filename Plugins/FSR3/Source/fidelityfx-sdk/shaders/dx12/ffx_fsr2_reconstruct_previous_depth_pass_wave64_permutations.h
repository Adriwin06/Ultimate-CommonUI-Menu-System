#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_a6934b55236240e3ff3606984bcf9cef.h"
#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_9b3b2b81b16e64edf75b80e57902a0d3.h"
#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_ee15c06490926d43dcde25a0427e8795.h"
#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_ff928969fe87d2ad58215f63cc92cf7f.h"
#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_b5c868ba3c8d9cb9c6ba8686ffe68948.h"
#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_8f3ae33a7a5edb614af0893c3690b434.h"
#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_1e941320b636610047e06a31487dd2f7.h"
#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_7404ddc358cbf462685fd67dfe29bca2.h"
#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_1c78f94183a6a52c01c4eaf16ac0c452.h"
#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_f390a2b1f95a8fb9b3c86cde634879b1.h"
#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_3b50f75a98f0fb42dfb3eeae2a9df4c4.h"
#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_5aaf3e6e81b142fcf1318de528266052.h"
#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_72db05e8545732a0e79e726c857ad8e4.h"
#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_c4dd7b42b6999c1556a25f4ae467e6ea.h"
#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_c1fe0565779917f3d2d84f353ae40643.h"
#include "ffx_fsr2_reconstruct_previous_depth_pass_wave64_d4a2b3949c58eb67733210d738df64b7.h"

typedef union ffx_fsr2_reconstruct_previous_depth_pass_wave64_PermutationKey {
    struct {
        uint32_t FFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE : 1;
        uint32_t FFX_FSR2_OPTION_HDR_COLOR_INPUT : 1;
        uint32_t FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS : 1;
        uint32_t FFX_FSR2_OPTION_JITTERED_MOTION_VECTORS : 1;
        uint32_t FFX_FSR2_OPTION_INVERTED_DEPTH : 1;
        uint32_t FFX_FSR2_OPTION_APPLY_SHARPENING : 1;
    };
    uint32_t index;
} ffx_fsr2_reconstruct_previous_depth_pass_wave64_PermutationKey;

typedef struct ffx_fsr2_reconstruct_previous_depth_pass_wave64_PermutationInfo {
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
} ffx_fsr2_reconstruct_previous_depth_pass_wave64_PermutationInfo;

static const uint32_t g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_IndirectionTable[] = {
    11,
    11,
    7,
    7,
    1,
    1,
    5,
    5,
    13,
    13,
    14,
    14,
    9,
    9,
    4,
    4,
    15,
    15,
    10,
    10,
    8,
    8,
    6,
    6,
    12,
    12,
    2,
    2,
    0,
    0,
    3,
    3,
    11,
    11,
    7,
    7,
    1,
    1,
    5,
    5,
    13,
    13,
    14,
    14,
    9,
    9,
    4,
    4,
    15,
    15,
    10,
    10,
    8,
    8,
    6,
    6,
    12,
    12,
    2,
    2,
    0,
    0,
    3,
    3,
};

static const ffx_fsr2_reconstruct_previous_depth_pass_wave64_PermutationInfo g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_PermutationInfo[] = {
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_a6934b55236240e3ff3606984bcf9cef_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_a6934b55236240e3ff3606984bcf9cef_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_a6934b55236240e3ff3606984bcf9cef_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_a6934b55236240e3ff3606984bcf9cef_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_a6934b55236240e3ff3606984bcf9cef_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_a6934b55236240e3ff3606984bcf9cef_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_a6934b55236240e3ff3606984bcf9cef_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_a6934b55236240e3ff3606984bcf9cef_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_a6934b55236240e3ff3606984bcf9cef_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_a6934b55236240e3ff3606984bcf9cef_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_a6934b55236240e3ff3606984bcf9cef_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_a6934b55236240e3ff3606984bcf9cef_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_a6934b55236240e3ff3606984bcf9cef_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_a6934b55236240e3ff3606984bcf9cef_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_9b3b2b81b16e64edf75b80e57902a0d3_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_9b3b2b81b16e64edf75b80e57902a0d3_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_9b3b2b81b16e64edf75b80e57902a0d3_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_9b3b2b81b16e64edf75b80e57902a0d3_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_9b3b2b81b16e64edf75b80e57902a0d3_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_9b3b2b81b16e64edf75b80e57902a0d3_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_9b3b2b81b16e64edf75b80e57902a0d3_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_9b3b2b81b16e64edf75b80e57902a0d3_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_9b3b2b81b16e64edf75b80e57902a0d3_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_9b3b2b81b16e64edf75b80e57902a0d3_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_9b3b2b81b16e64edf75b80e57902a0d3_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_9b3b2b81b16e64edf75b80e57902a0d3_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_9b3b2b81b16e64edf75b80e57902a0d3_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_9b3b2b81b16e64edf75b80e57902a0d3_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ee15c06490926d43dcde25a0427e8795_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ee15c06490926d43dcde25a0427e8795_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ee15c06490926d43dcde25a0427e8795_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ee15c06490926d43dcde25a0427e8795_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ee15c06490926d43dcde25a0427e8795_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ee15c06490926d43dcde25a0427e8795_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ee15c06490926d43dcde25a0427e8795_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ee15c06490926d43dcde25a0427e8795_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ee15c06490926d43dcde25a0427e8795_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ee15c06490926d43dcde25a0427e8795_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ee15c06490926d43dcde25a0427e8795_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ee15c06490926d43dcde25a0427e8795_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ee15c06490926d43dcde25a0427e8795_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ee15c06490926d43dcde25a0427e8795_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ff928969fe87d2ad58215f63cc92cf7f_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ff928969fe87d2ad58215f63cc92cf7f_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ff928969fe87d2ad58215f63cc92cf7f_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ff928969fe87d2ad58215f63cc92cf7f_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ff928969fe87d2ad58215f63cc92cf7f_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ff928969fe87d2ad58215f63cc92cf7f_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ff928969fe87d2ad58215f63cc92cf7f_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ff928969fe87d2ad58215f63cc92cf7f_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ff928969fe87d2ad58215f63cc92cf7f_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ff928969fe87d2ad58215f63cc92cf7f_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ff928969fe87d2ad58215f63cc92cf7f_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ff928969fe87d2ad58215f63cc92cf7f_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ff928969fe87d2ad58215f63cc92cf7f_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_ff928969fe87d2ad58215f63cc92cf7f_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_b5c868ba3c8d9cb9c6ba8686ffe68948_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_b5c868ba3c8d9cb9c6ba8686ffe68948_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_b5c868ba3c8d9cb9c6ba8686ffe68948_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_b5c868ba3c8d9cb9c6ba8686ffe68948_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_b5c868ba3c8d9cb9c6ba8686ffe68948_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_b5c868ba3c8d9cb9c6ba8686ffe68948_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_b5c868ba3c8d9cb9c6ba8686ffe68948_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_b5c868ba3c8d9cb9c6ba8686ffe68948_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_b5c868ba3c8d9cb9c6ba8686ffe68948_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_b5c868ba3c8d9cb9c6ba8686ffe68948_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_b5c868ba3c8d9cb9c6ba8686ffe68948_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_b5c868ba3c8d9cb9c6ba8686ffe68948_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_b5c868ba3c8d9cb9c6ba8686ffe68948_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_b5c868ba3c8d9cb9c6ba8686ffe68948_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_8f3ae33a7a5edb614af0893c3690b434_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_8f3ae33a7a5edb614af0893c3690b434_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_8f3ae33a7a5edb614af0893c3690b434_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_8f3ae33a7a5edb614af0893c3690b434_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_8f3ae33a7a5edb614af0893c3690b434_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_8f3ae33a7a5edb614af0893c3690b434_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_8f3ae33a7a5edb614af0893c3690b434_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_8f3ae33a7a5edb614af0893c3690b434_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_8f3ae33a7a5edb614af0893c3690b434_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_8f3ae33a7a5edb614af0893c3690b434_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_8f3ae33a7a5edb614af0893c3690b434_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_8f3ae33a7a5edb614af0893c3690b434_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_8f3ae33a7a5edb614af0893c3690b434_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_8f3ae33a7a5edb614af0893c3690b434_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1e941320b636610047e06a31487dd2f7_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1e941320b636610047e06a31487dd2f7_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1e941320b636610047e06a31487dd2f7_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1e941320b636610047e06a31487dd2f7_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1e941320b636610047e06a31487dd2f7_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1e941320b636610047e06a31487dd2f7_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1e941320b636610047e06a31487dd2f7_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1e941320b636610047e06a31487dd2f7_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1e941320b636610047e06a31487dd2f7_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1e941320b636610047e06a31487dd2f7_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1e941320b636610047e06a31487dd2f7_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1e941320b636610047e06a31487dd2f7_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1e941320b636610047e06a31487dd2f7_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1e941320b636610047e06a31487dd2f7_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_7404ddc358cbf462685fd67dfe29bca2_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_7404ddc358cbf462685fd67dfe29bca2_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_7404ddc358cbf462685fd67dfe29bca2_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_7404ddc358cbf462685fd67dfe29bca2_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_7404ddc358cbf462685fd67dfe29bca2_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_7404ddc358cbf462685fd67dfe29bca2_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_7404ddc358cbf462685fd67dfe29bca2_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_7404ddc358cbf462685fd67dfe29bca2_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_7404ddc358cbf462685fd67dfe29bca2_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_7404ddc358cbf462685fd67dfe29bca2_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_7404ddc358cbf462685fd67dfe29bca2_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_7404ddc358cbf462685fd67dfe29bca2_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_7404ddc358cbf462685fd67dfe29bca2_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_7404ddc358cbf462685fd67dfe29bca2_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1c78f94183a6a52c01c4eaf16ac0c452_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1c78f94183a6a52c01c4eaf16ac0c452_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1c78f94183a6a52c01c4eaf16ac0c452_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1c78f94183a6a52c01c4eaf16ac0c452_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1c78f94183a6a52c01c4eaf16ac0c452_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1c78f94183a6a52c01c4eaf16ac0c452_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1c78f94183a6a52c01c4eaf16ac0c452_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1c78f94183a6a52c01c4eaf16ac0c452_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1c78f94183a6a52c01c4eaf16ac0c452_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1c78f94183a6a52c01c4eaf16ac0c452_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1c78f94183a6a52c01c4eaf16ac0c452_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1c78f94183a6a52c01c4eaf16ac0c452_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1c78f94183a6a52c01c4eaf16ac0c452_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_1c78f94183a6a52c01c4eaf16ac0c452_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_f390a2b1f95a8fb9b3c86cde634879b1_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_f390a2b1f95a8fb9b3c86cde634879b1_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_f390a2b1f95a8fb9b3c86cde634879b1_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_f390a2b1f95a8fb9b3c86cde634879b1_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_f390a2b1f95a8fb9b3c86cde634879b1_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_f390a2b1f95a8fb9b3c86cde634879b1_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_f390a2b1f95a8fb9b3c86cde634879b1_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_f390a2b1f95a8fb9b3c86cde634879b1_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_f390a2b1f95a8fb9b3c86cde634879b1_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_f390a2b1f95a8fb9b3c86cde634879b1_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_f390a2b1f95a8fb9b3c86cde634879b1_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_f390a2b1f95a8fb9b3c86cde634879b1_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_f390a2b1f95a8fb9b3c86cde634879b1_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_f390a2b1f95a8fb9b3c86cde634879b1_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_3b50f75a98f0fb42dfb3eeae2a9df4c4_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_3b50f75a98f0fb42dfb3eeae2a9df4c4_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_3b50f75a98f0fb42dfb3eeae2a9df4c4_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_3b50f75a98f0fb42dfb3eeae2a9df4c4_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_3b50f75a98f0fb42dfb3eeae2a9df4c4_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_3b50f75a98f0fb42dfb3eeae2a9df4c4_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_3b50f75a98f0fb42dfb3eeae2a9df4c4_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_3b50f75a98f0fb42dfb3eeae2a9df4c4_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_3b50f75a98f0fb42dfb3eeae2a9df4c4_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_3b50f75a98f0fb42dfb3eeae2a9df4c4_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_3b50f75a98f0fb42dfb3eeae2a9df4c4_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_3b50f75a98f0fb42dfb3eeae2a9df4c4_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_3b50f75a98f0fb42dfb3eeae2a9df4c4_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_3b50f75a98f0fb42dfb3eeae2a9df4c4_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_5aaf3e6e81b142fcf1318de528266052_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_5aaf3e6e81b142fcf1318de528266052_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_5aaf3e6e81b142fcf1318de528266052_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_5aaf3e6e81b142fcf1318de528266052_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_5aaf3e6e81b142fcf1318de528266052_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_5aaf3e6e81b142fcf1318de528266052_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_5aaf3e6e81b142fcf1318de528266052_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_5aaf3e6e81b142fcf1318de528266052_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_5aaf3e6e81b142fcf1318de528266052_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_5aaf3e6e81b142fcf1318de528266052_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_5aaf3e6e81b142fcf1318de528266052_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_5aaf3e6e81b142fcf1318de528266052_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_5aaf3e6e81b142fcf1318de528266052_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_5aaf3e6e81b142fcf1318de528266052_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_72db05e8545732a0e79e726c857ad8e4_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_72db05e8545732a0e79e726c857ad8e4_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_72db05e8545732a0e79e726c857ad8e4_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_72db05e8545732a0e79e726c857ad8e4_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_72db05e8545732a0e79e726c857ad8e4_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_72db05e8545732a0e79e726c857ad8e4_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_72db05e8545732a0e79e726c857ad8e4_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_72db05e8545732a0e79e726c857ad8e4_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_72db05e8545732a0e79e726c857ad8e4_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_72db05e8545732a0e79e726c857ad8e4_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_72db05e8545732a0e79e726c857ad8e4_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_72db05e8545732a0e79e726c857ad8e4_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_72db05e8545732a0e79e726c857ad8e4_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_72db05e8545732a0e79e726c857ad8e4_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c4dd7b42b6999c1556a25f4ae467e6ea_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c4dd7b42b6999c1556a25f4ae467e6ea_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c4dd7b42b6999c1556a25f4ae467e6ea_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c4dd7b42b6999c1556a25f4ae467e6ea_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c4dd7b42b6999c1556a25f4ae467e6ea_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c4dd7b42b6999c1556a25f4ae467e6ea_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c4dd7b42b6999c1556a25f4ae467e6ea_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c4dd7b42b6999c1556a25f4ae467e6ea_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c4dd7b42b6999c1556a25f4ae467e6ea_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c4dd7b42b6999c1556a25f4ae467e6ea_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c4dd7b42b6999c1556a25f4ae467e6ea_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c4dd7b42b6999c1556a25f4ae467e6ea_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c4dd7b42b6999c1556a25f4ae467e6ea_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c4dd7b42b6999c1556a25f4ae467e6ea_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c1fe0565779917f3d2d84f353ae40643_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c1fe0565779917f3d2d84f353ae40643_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c1fe0565779917f3d2d84f353ae40643_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c1fe0565779917f3d2d84f353ae40643_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c1fe0565779917f3d2d84f353ae40643_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c1fe0565779917f3d2d84f353ae40643_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c1fe0565779917f3d2d84f353ae40643_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c1fe0565779917f3d2d84f353ae40643_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c1fe0565779917f3d2d84f353ae40643_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c1fe0565779917f3d2d84f353ae40643_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c1fe0565779917f3d2d84f353ae40643_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c1fe0565779917f3d2d84f353ae40643_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c1fe0565779917f3d2d84f353ae40643_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_c1fe0565779917f3d2d84f353ae40643_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_d4a2b3949c58eb67733210d738df64b7_size, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_d4a2b3949c58eb67733210d738df64b7_data, 1, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_d4a2b3949c58eb67733210d738df64b7_CBVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_d4a2b3949c58eb67733210d738df64b7_CBVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_d4a2b3949c58eb67733210d738df64b7_CBVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_d4a2b3949c58eb67733210d738df64b7_CBVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_d4a2b3949c58eb67733210d738df64b7_TextureSRVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_d4a2b3949c58eb67733210d738df64b7_TextureSRVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_d4a2b3949c58eb67733210d738df64b7_TextureSRVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_d4a2b3949c58eb67733210d738df64b7_TextureSRVResourceSpaces, 4, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_d4a2b3949c58eb67733210d738df64b7_TextureUAVResourceNames, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_d4a2b3949c58eb67733210d738df64b7_TextureUAVResourceBindings, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_d4a2b3949c58eb67733210d738df64b7_TextureUAVResourceCounts, g_ffx_fsr2_reconstruct_previous_depth_pass_wave64_d4a2b3949c58eb67733210d738df64b7_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
};

