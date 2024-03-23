#include "ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7.h"
#include "ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8.h"
#include "ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03.h"
#include "ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586.h"

typedef union ffx_fsr1_easu_pass_wave64_PermutationKey {
    struct {
        uint32_t FFX_FSR1_OPTION_APPLY_RCAS : 1;
        uint32_t FFX_FSR1_OPTION_RCAS_PASSTHROUGH_ALPHA : 1;
        uint32_t FFX_FSR1_OPTION_SRGB_CONVERSIONS : 1;
    };
    uint32_t index;
} ffx_fsr1_easu_pass_wave64_PermutationKey;

typedef struct ffx_fsr1_easu_pass_wave64_PermutationInfo {
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
} ffx_fsr1_easu_pass_wave64_PermutationInfo;

static const uint32_t g_ffx_fsr1_easu_pass_wave64_IndirectionTable[] = {
    1,
    0,
    1,
    0,
    3,
    2,
    3,
    2,
};

static const ffx_fsr1_easu_pass_wave64_PermutationInfo g_ffx_fsr1_easu_pass_wave64_PermutationInfo[] = {
    { g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_size, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_data, 1, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_CBVResourceNames, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_CBVResourceBindings, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_CBVResourceCounts, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_CBVResourceSpaces, 1, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_TextureSRVResourceNames, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_TextureSRVResourceBindings, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_TextureSRVResourceCounts, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_TextureSRVResourceSpaces, 1, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_TextureUAVResourceNames, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_TextureUAVResourceBindings, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_TextureUAVResourceCounts, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_SamplerResourceNames, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_SamplerResourceBindings, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_SamplerResourceCounts, g_ffx_fsr1_easu_pass_wave64_e7ed6eea06edf87c1c11fa24d21c22b7_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_size, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_data, 1, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_CBVResourceNames, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_CBVResourceBindings, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_CBVResourceCounts, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_CBVResourceSpaces, 1, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_TextureSRVResourceNames, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_TextureSRVResourceBindings, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_TextureSRVResourceCounts, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_TextureSRVResourceSpaces, 1, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_TextureUAVResourceNames, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_TextureUAVResourceBindings, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_TextureUAVResourceCounts, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_SamplerResourceNames, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_SamplerResourceBindings, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_SamplerResourceCounts, g_ffx_fsr1_easu_pass_wave64_436f11866e3bbcd96261ad5725c84de8_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_size, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_data, 1, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_CBVResourceNames, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_CBVResourceBindings, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_CBVResourceCounts, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_CBVResourceSpaces, 1, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_TextureSRVResourceNames, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_TextureSRVResourceBindings, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_TextureSRVResourceCounts, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_TextureSRVResourceSpaces, 1, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_TextureUAVResourceNames, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_TextureUAVResourceBindings, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_TextureUAVResourceCounts, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_SamplerResourceNames, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_SamplerResourceBindings, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_SamplerResourceCounts, g_ffx_fsr1_easu_pass_wave64_ddc1d1d00ffaebe7bf73bb25ce275d03_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_size, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_data, 1, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_CBVResourceNames, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_CBVResourceBindings, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_CBVResourceCounts, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_CBVResourceSpaces, 1, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_TextureSRVResourceNames, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_TextureSRVResourceBindings, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_TextureSRVResourceCounts, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_TextureSRVResourceSpaces, 1, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_TextureUAVResourceNames, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_TextureUAVResourceBindings, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_TextureUAVResourceCounts, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_SamplerResourceNames, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_SamplerResourceBindings, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_SamplerResourceCounts, g_ffx_fsr1_easu_pass_wave64_482ec4a721242d70db5e84e69c524586_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

