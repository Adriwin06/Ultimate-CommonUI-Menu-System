#include "ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626.h"
#include "ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7.h"
#include "ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012.h"
#include "ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301.h"
#include "ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67.h"
#include "ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e.h"
#include "ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2.h"
#include "ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c.h"

typedef union ffx_fsr2_depth_clip_pass_PermutationKey {
    struct {
        uint32_t FFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE : 1;
        uint32_t FFX_FSR2_OPTION_HDR_COLOR_INPUT : 1;
        uint32_t FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS : 1;
        uint32_t FFX_FSR2_OPTION_JITTERED_MOTION_VECTORS : 1;
        uint32_t FFX_FSR2_OPTION_INVERTED_DEPTH : 1;
        uint32_t FFX_FSR2_OPTION_APPLY_SHARPENING : 1;
    };
    uint32_t index;
} ffx_fsr2_depth_clip_pass_PermutationKey;

typedef struct ffx_fsr2_depth_clip_pass_PermutationInfo {
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
} ffx_fsr2_depth_clip_pass_PermutationInfo;

static const uint32_t g_ffx_fsr2_depth_clip_pass_IndirectionTable[] = {
    6,
    6,
    6,
    6,
    4,
    4,
    4,
    4,
    7,
    7,
    7,
    7,
    2,
    2,
    2,
    2,
    5,
    5,
    5,
    5,
    3,
    3,
    3,
    3,
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
    6,
    6,
    6,
    6,
    4,
    4,
    4,
    4,
    7,
    7,
    7,
    7,
    2,
    2,
    2,
    2,
    5,
    5,
    5,
    5,
    3,
    3,
    3,
    3,
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
};

static const ffx_fsr2_depth_clip_pass_PermutationInfo g_ffx_fsr2_depth_clip_pass_PermutationInfo[] = {
    { g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_size, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_data, 1, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_CBVResourceNames, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_CBVResourceBindings, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_CBVResourceCounts, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_CBVResourceSpaces, 9, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_TextureSRVResourceNames, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_TextureSRVResourceBindings, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_TextureSRVResourceCounts, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_TextureSRVResourceSpaces, 2, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_TextureUAVResourceNames, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_TextureUAVResourceBindings, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_TextureUAVResourceCounts, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_SamplerResourceNames, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_SamplerResourceBindings, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_SamplerResourceCounts, g_ffx_fsr2_depth_clip_pass_2481ba0ddba807c48661b5f54d4da626_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_size, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_data, 1, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_CBVResourceNames, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_CBVResourceBindings, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_CBVResourceCounts, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_CBVResourceSpaces, 9, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_TextureSRVResourceNames, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_TextureSRVResourceBindings, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_TextureSRVResourceCounts, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_TextureSRVResourceSpaces, 2, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_TextureUAVResourceNames, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_TextureUAVResourceBindings, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_TextureUAVResourceCounts, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_SamplerResourceNames, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_SamplerResourceBindings, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_SamplerResourceCounts, g_ffx_fsr2_depth_clip_pass_daf1385a618b8db14bcb97da9f0fd2c7_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_size, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_data, 1, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_CBVResourceNames, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_CBVResourceBindings, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_CBVResourceCounts, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_CBVResourceSpaces, 9, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_TextureSRVResourceNames, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_TextureSRVResourceBindings, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_TextureSRVResourceCounts, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_TextureSRVResourceSpaces, 2, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_TextureUAVResourceNames, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_TextureUAVResourceBindings, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_TextureUAVResourceCounts, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_SamplerResourceNames, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_SamplerResourceBindings, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_SamplerResourceCounts, g_ffx_fsr2_depth_clip_pass_b24634a852ff73de77de28a9d4e14012_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_size, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_data, 1, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_CBVResourceNames, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_CBVResourceBindings, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_CBVResourceCounts, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_CBVResourceSpaces, 9, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_TextureSRVResourceNames, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_TextureSRVResourceBindings, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_TextureSRVResourceCounts, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_TextureSRVResourceSpaces, 2, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_TextureUAVResourceNames, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_TextureUAVResourceBindings, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_TextureUAVResourceCounts, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_SamplerResourceNames, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_SamplerResourceBindings, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_SamplerResourceCounts, g_ffx_fsr2_depth_clip_pass_dcc2ef3fa2a44bfc193dbfcf15098301_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_size, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_data, 1, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_CBVResourceNames, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_CBVResourceBindings, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_CBVResourceCounts, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_CBVResourceSpaces, 9, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_TextureSRVResourceNames, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_TextureSRVResourceBindings, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_TextureSRVResourceCounts, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_TextureSRVResourceSpaces, 2, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_TextureUAVResourceNames, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_TextureUAVResourceBindings, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_TextureUAVResourceCounts, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_SamplerResourceNames, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_SamplerResourceBindings, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_SamplerResourceCounts, g_ffx_fsr2_depth_clip_pass_cc16256d30a187a8021eae44221bdd67_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_size, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_data, 1, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_CBVResourceNames, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_CBVResourceBindings, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_CBVResourceCounts, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_CBVResourceSpaces, 9, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_TextureSRVResourceNames, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_TextureSRVResourceBindings, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_TextureSRVResourceCounts, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_TextureSRVResourceSpaces, 2, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_TextureUAVResourceNames, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_TextureUAVResourceBindings, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_TextureUAVResourceCounts, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_SamplerResourceNames, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_SamplerResourceBindings, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_SamplerResourceCounts, g_ffx_fsr2_depth_clip_pass_96bde1095ecdeb746f6433578c77662e_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_size, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_data, 1, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_CBVResourceNames, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_CBVResourceBindings, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_CBVResourceCounts, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_CBVResourceSpaces, 9, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_TextureSRVResourceNames, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_TextureSRVResourceBindings, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_TextureSRVResourceCounts, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_TextureSRVResourceSpaces, 2, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_TextureUAVResourceNames, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_TextureUAVResourceBindings, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_TextureUAVResourceCounts, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_SamplerResourceNames, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_SamplerResourceBindings, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_SamplerResourceCounts, g_ffx_fsr2_depth_clip_pass_07d1b7b2c460ef3ed8a1b1cb5673ebd2_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_size, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_data, 1, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_CBVResourceNames, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_CBVResourceBindings, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_CBVResourceCounts, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_CBVResourceSpaces, 9, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_TextureSRVResourceNames, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_TextureSRVResourceBindings, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_TextureSRVResourceCounts, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_TextureSRVResourceSpaces, 2, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_TextureUAVResourceNames, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_TextureUAVResourceBindings, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_TextureUAVResourceCounts, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_SamplerResourceNames, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_SamplerResourceBindings, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_SamplerResourceCounts, g_ffx_fsr2_depth_clip_pass_d831ef8800c3fabfd2eacc4391e75d5c_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

