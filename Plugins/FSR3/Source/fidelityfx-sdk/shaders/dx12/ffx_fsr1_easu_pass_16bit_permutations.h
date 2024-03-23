#include "ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7.h"
#include "ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489.h"
#include "ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e.h"
#include "ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189.h"

typedef union ffx_fsr1_easu_pass_16bit_PermutationKey {
    struct {
        uint32_t FFX_FSR1_OPTION_APPLY_RCAS : 1;
        uint32_t FFX_FSR1_OPTION_RCAS_PASSTHROUGH_ALPHA : 1;
        uint32_t FFX_FSR1_OPTION_SRGB_CONVERSIONS : 1;
    };
    uint32_t index;
} ffx_fsr1_easu_pass_16bit_PermutationKey;

typedef struct ffx_fsr1_easu_pass_16bit_PermutationInfo {
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
} ffx_fsr1_easu_pass_16bit_PermutationInfo;

static const uint32_t g_ffx_fsr1_easu_pass_16bit_IndirectionTable[] = {
    3,
    0,
    3,
    0,
    2,
    1,
    2,
    1,
};

static const ffx_fsr1_easu_pass_16bit_PermutationInfo g_ffx_fsr1_easu_pass_16bit_PermutationInfo[] = {
    { g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_size, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_data, 1, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_CBVResourceNames, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_CBVResourceBindings, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_CBVResourceCounts, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_CBVResourceSpaces, 1, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_TextureSRVResourceNames, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_TextureSRVResourceBindings, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_TextureSRVResourceCounts, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_TextureSRVResourceSpaces, 1, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_TextureUAVResourceNames, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_TextureUAVResourceBindings, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_TextureUAVResourceCounts, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_SamplerResourceNames, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_SamplerResourceBindings, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_SamplerResourceCounts, g_ffx_fsr1_easu_pass_16bit_6af489f355924de1310e717ac3ea7fb7_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_size, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_data, 1, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_CBVResourceNames, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_CBVResourceBindings, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_CBVResourceCounts, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_CBVResourceSpaces, 1, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_TextureSRVResourceNames, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_TextureSRVResourceBindings, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_TextureSRVResourceCounts, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_TextureSRVResourceSpaces, 1, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_TextureUAVResourceNames, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_TextureUAVResourceBindings, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_TextureUAVResourceCounts, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_SamplerResourceNames, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_SamplerResourceBindings, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_SamplerResourceCounts, g_ffx_fsr1_easu_pass_16bit_93167785a94525ff22e2072ea4c67489_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_size, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_data, 1, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_CBVResourceNames, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_CBVResourceBindings, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_CBVResourceCounts, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_CBVResourceSpaces, 1, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_TextureSRVResourceNames, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_TextureSRVResourceBindings, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_TextureSRVResourceCounts, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_TextureSRVResourceSpaces, 1, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_TextureUAVResourceNames, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_TextureUAVResourceBindings, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_TextureUAVResourceCounts, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_SamplerResourceNames, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_SamplerResourceBindings, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_SamplerResourceCounts, g_ffx_fsr1_easu_pass_16bit_31870b7adcf3d614664ff5c2f8bdba6e_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_size, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_data, 1, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_CBVResourceNames, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_CBVResourceBindings, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_CBVResourceCounts, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_CBVResourceSpaces, 1, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_TextureSRVResourceNames, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_TextureSRVResourceBindings, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_TextureSRVResourceCounts, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_TextureSRVResourceSpaces, 1, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_TextureUAVResourceNames, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_TextureUAVResourceBindings, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_TextureUAVResourceCounts, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_SamplerResourceNames, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_SamplerResourceBindings, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_SamplerResourceCounts, g_ffx_fsr1_easu_pass_16bit_90ec3120a584932bf25e66175b481189_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

