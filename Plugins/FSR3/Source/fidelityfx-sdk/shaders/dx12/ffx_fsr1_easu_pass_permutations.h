#include "ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd.h"
#include "ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6.h"
#include "ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2.h"
#include "ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd.h"

typedef union ffx_fsr1_easu_pass_PermutationKey {
    struct {
        uint32_t FFX_FSR1_OPTION_APPLY_RCAS : 1;
        uint32_t FFX_FSR1_OPTION_RCAS_PASSTHROUGH_ALPHA : 1;
        uint32_t FFX_FSR1_OPTION_SRGB_CONVERSIONS : 1;
    };
    uint32_t index;
} ffx_fsr1_easu_pass_PermutationKey;

typedef struct ffx_fsr1_easu_pass_PermutationInfo {
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
} ffx_fsr1_easu_pass_PermutationInfo;

static const uint32_t g_ffx_fsr1_easu_pass_IndirectionTable[] = {
    1,
    2,
    1,
    2,
    3,
    0,
    3,
    0,
};

static const ffx_fsr1_easu_pass_PermutationInfo g_ffx_fsr1_easu_pass_PermutationInfo[] = {
    { g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_size, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_data, 1, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_CBVResourceNames, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_CBVResourceBindings, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_CBVResourceCounts, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_CBVResourceSpaces, 1, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_TextureSRVResourceNames, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_TextureSRVResourceBindings, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_TextureSRVResourceCounts, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_TextureSRVResourceSpaces, 1, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_TextureUAVResourceNames, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_TextureUAVResourceBindings, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_TextureUAVResourceCounts, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_SamplerResourceNames, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_SamplerResourceBindings, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_SamplerResourceCounts, g_ffx_fsr1_easu_pass_92ce97a0cc3a37103746f3d387895cbd_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_size, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_data, 1, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_CBVResourceNames, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_CBVResourceBindings, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_CBVResourceCounts, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_CBVResourceSpaces, 1, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_TextureSRVResourceNames, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_TextureSRVResourceBindings, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_TextureSRVResourceCounts, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_TextureSRVResourceSpaces, 1, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_TextureUAVResourceNames, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_TextureUAVResourceBindings, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_TextureUAVResourceCounts, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_SamplerResourceNames, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_SamplerResourceBindings, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_SamplerResourceCounts, g_ffx_fsr1_easu_pass_91c0fc09c7a8eed03967b50077c84ba6_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_size, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_data, 1, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_CBVResourceNames, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_CBVResourceBindings, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_CBVResourceCounts, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_CBVResourceSpaces, 1, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_TextureSRVResourceNames, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_TextureSRVResourceBindings, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_TextureSRVResourceCounts, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_TextureSRVResourceSpaces, 1, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_TextureUAVResourceNames, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_TextureUAVResourceBindings, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_TextureUAVResourceCounts, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_SamplerResourceNames, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_SamplerResourceBindings, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_SamplerResourceCounts, g_ffx_fsr1_easu_pass_6238c2d086b2702e5dbaf550759be1d2_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
    { g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_size, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_data, 1, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_CBVResourceNames, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_CBVResourceBindings, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_CBVResourceCounts, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_CBVResourceSpaces, 1, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_TextureSRVResourceNames, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_TextureSRVResourceBindings, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_TextureSRVResourceCounts, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_TextureSRVResourceSpaces, 1, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_TextureUAVResourceNames, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_TextureUAVResourceBindings, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_TextureUAVResourceCounts, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_TextureUAVResourceSpaces, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_SamplerResourceNames, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_SamplerResourceBindings, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_SamplerResourceCounts, g_ffx_fsr1_easu_pass_417a95cd6e526219f0bcbeb838184cfd_SamplerResourceSpaces, 0, 0, 0, 0, 0, },
};

