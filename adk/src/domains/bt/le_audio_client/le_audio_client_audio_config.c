/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief    Table and interface function used LE Audio Client Config
*/

#include "le_audio_client_audio_config.h"
#include <le_audio_client_broadcast_private.h>
#include "logging.h"
#include "kymera.h"
#include <panic.h>

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

/*! LC3 blocks per SDU for broadcast */
#define LE_AUDIO_CLIENT_BROADCAST_LC3_BLOCKS_PER_SDU        1

/*! Number of BIS for high quality configuration */
#define LE_AUDIO_CLIENT_BROADCAST_HIGH_QUALITY_NUM_BIS      2

/*! Stereo BIS audio location configuration */
#define LE_AUDIO_CLIENT_STEREO_BIS_BROADCAST                (BAP_AUDIO_LOCATION_FL | BAP_AUDIO_LOCATION_FR)

/*! Structure to hold broadcast audio configuration */
le_audio_client_audio_broadcast_config_t lea_client_bcast_config;

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#if defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

static const le_audio_client_config_interface_t *le_audio_config_callback_configs = NULL;

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

/*! As per QBCE Spec(section 5.7.97), the PHY for Q2Q should be 0x80 */
#define LE_AUDIO_CLIENT_Q2Q_PHY                   (0x80)

/*! Use unframed for framing type */
#define LE_AUDIO_CLIENT_UNFRAMED                  (0x0)

/*! RTN from Central to Peripheral value for aptX Adaptive */
#define LE_AUDIO_CLIENT_RTN_APTX_ADAPTIVE_C_TO_P  (0xf)

/*! RTN from Central to Peripheral value for Gaming without VBC */
#define LE_AUDIO_CLIENT_RTN_GAME_NO_VBC_C_TO_P    (0x5)

/*! RTN from Central to Peripheral value for Gaming with VBC */
#define LE_AUDIO_CLIENT_RTN_GAME_WITH_VBC_C_TO_P  (0x3)

/*! RTN from Peripheral to Central value for Gaming with VBC */
#define LE_AUDIO_CLIENT_RTN_GAME_WITH_VBC_P_TO_C  (0x3)

/*! RTN from Central to Peripheral value for Gaming without VBC */
#define LE_AUDIO_CLIENT_APTX_LITE_RTN_GAME_NO_VBC_C_TO_P    (0x3)

/*! RTN from Central to Peripheral value for Gaming with VBC */
#define LE_AUDIO_CLIENT_APTX_LITE_RTN_GAME_WITH_VBC_C_TO_P  (0x3)

/*! RTN from Peripheral to Central value for Gaming with VBC */
#define LE_AUDIO_CLIENT_APTX_LITE_RTN_GAME_WITH_VBC_P_TO_C  (0x7)

/*! Audio Config for gaming */
le_audio_client_audio_config_t  le_audio_client_audio_config_gaming =
{
    .target_latency             = CAP_CLIENT_TARGET_LOWER_LATENCY,
    .sink_stream_capability     = CAP_CLIENT_STREAM_CAPABILITY_48_1,
    .source_stream_capability   = CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN,
    .framing                    = LE_AUDIO_CLIENT_UNFRAMED,
    .phy_ctop                   = LE_AUDIO_CLIENT_Q2Q_PHY,
    .phy_ptoc                   = LE_AUDIO_CLIENT_Q2Q_PHY,
    .rtn_ctop                   = LE_AUDIO_CLIENT_RTN_GAME_NO_VBC_C_TO_P,
    .rtn_ptoc                   = 0x0
};

/*! Audio Config for gaming with VBC */
le_audio_client_audio_config_t  le_audio_client_audio_config_gaming_with_vbc =
{
    .target_latency             = CAP_CLIENT_TARGET_LOWER_LATENCY,
    .sink_stream_capability     = CAP_CLIENT_STREAM_CAPABILITY_48_1,
    .source_stream_capability   = CAP_CLIENT_STREAM_CAPABILITY_16_1,
    .framing                    = LE_AUDIO_CLIENT_UNFRAMED,
    .phy_ctop                   = LE_AUDIO_CLIENT_Q2Q_PHY,
    .phy_ptoc                   = LE_AUDIO_CLIENT_Q2Q_PHY,
    .rtn_ctop                   = LE_AUDIO_CLIENT_RTN_GAME_WITH_VBC_C_TO_P,
    .rtn_ptoc                   = LE_AUDIO_CLIENT_RTN_GAME_WITH_VBC_P_TO_C
};

/*! Audio Config for media usecase */
le_audio_client_audio_config_t  le_audio_client_audio_config_media =
{
    .target_latency             = CAP_CLIENT_TARGET_HIGH_RELIABILITY,
    .sink_stream_capability     = CAP_CLIENT_STREAM_CAPABILITY_48_2,
    .source_stream_capability   = CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN,
    .framing                    = LE_AUDIO_CLIENT_UNFRAMED,
    .phy_ctop                   = LE_AUDIO_CLIENT_Q2Q_PHY,
    .phy_ptoc                   = LE_AUDIO_CLIENT_Q2Q_PHY,
    .rtn_ctop                   = LE_AUDIO_CLIENT_RTN_GAME_NO_VBC_C_TO_P,
    .rtn_ptoc                   = 0x0
};

#ifdef INCLUDE_LE_APTX_ADAPTIVE
/*! QHS MAP table for LE aptX Adaptive media usecase */
const QCOM_CON_MANAGER_CIS_QHS_PARAMS_T aptx_adaptive_cis_qhs_map[QCOM_MAX_NUM_OF_QHS_RATE] =
{
    /* bn_c_to_p    bn_p_to_c     max_pdu_c_to_p     max_pdu_p_to_c */
    {0,             0,            0,                 0               },    /* QHS6 */
    {0,             0,            0,                 0               },    /* QHS5 */
    {2,             2,            608,               0               },    /* QHS4 */
    {1,             1,            452,               0               },    /* QHS3 */
    {1,             1,            300,               0               }     /* QHS2 */
};

/*! Audio Config for LE aptX Adaptive media usecase */
le_audio_client_audio_config_t  le_audio_client_audio_config_aptx_adaptive_media =
{
    .target_latency             = CAP_CLIENT_TARGET_HIGH_RELIABILITY,
    .sink_stream_capability     = CAP_CLIENT_STREAM_CAPABILITY_APTX_ADAPTIVE_48_1,
    .source_stream_capability   = CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN,
    .framing                    = LE_AUDIO_CLIENT_UNFRAMED,
    .phy_ctop                   = LE_AUDIO_CLIENT_Q2Q_PHY,
    .phy_ptoc                   = LE_AUDIO_CLIENT_Q2Q_PHY,
    .rtn_ctop                   = LE_AUDIO_CLIENT_RTN_APTX_ADAPTIVE_C_TO_P,
    .rtn_ptoc                   = 0x0
};
#endif /* INCLUDE_LE_APTX_ADAPTIVE */

/*! Audio Config for conversational usecase */
le_audio_client_audio_config_t  le_audio_client_audio_config_conversational =
{
    .target_latency             = CAP_CLIENT_TARGET_BALANCE_LATENCY_AND_RELIABILITY,
    .sink_stream_capability     = TMAP_CLIENT_STREAM_CAPABILITY_32_2,
    .source_stream_capability   = TMAP_CLIENT_STREAM_CAPABILITY_32_2,
    .framing                    = LE_AUDIO_CLIENT_UNFRAMED,
    .phy_ctop                   = LE_AUDIO_CLIENT_Q2Q_PHY,
    .phy_ptoc                   = LE_AUDIO_CLIENT_Q2Q_PHY,
    .rtn_ctop                   = LE_AUDIO_CLIENT_RTN_GAME_WITH_VBC_C_TO_P,
    .rtn_ptoc                   = LE_AUDIO_CLIENT_RTN_GAME_WITH_VBC_P_TO_C
};

#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
/*! Audio Config for gaming using VS Aptx Lite */
le_audio_client_audio_config_t  le_audio_client_audio_config_gaming_aptx_lite =
{
    .target_latency             = CAP_CLIENT_TARGET_LOWER_LATENCY,
    .sink_stream_capability     = CAP_CLIENT_STREAM_CAPABILITY_APTX_LITE_48_1,
    .source_stream_capability   = CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN,
    .framing                    = LE_AUDIO_CLIENT_UNFRAMED,
    .phy_ctop                   = LE_AUDIO_CLIENT_Q2Q_PHY,
    .phy_ptoc                   = LE_AUDIO_CLIENT_Q2Q_PHY,
    .rtn_ctop                   = LE_AUDIO_CLIENT_APTX_LITE_RTN_GAME_NO_VBC_C_TO_P,
    .rtn_ptoc                   = 0x0
};

/*! Audio Config for gaming with VBC using VS Aptx Lite */
le_audio_client_audio_config_t  le_audio_client_audio_config_gaming_with_vbc_aptx_lite =
{
    .target_latency             = CAP_CLIENT_TARGET_LOWER_LATENCY,
    .sink_stream_capability     = CAP_CLIENT_STREAM_CAPABILITY_APTX_LITE_48_1,
    .source_stream_capability   = CAP_CLIENT_STREAM_CAPABILITY_APTX_LITE_16_1 ,
    .framing                    = LE_AUDIO_CLIENT_UNFRAMED,
    .phy_ctop                   = LE_AUDIO_CLIENT_Q2Q_PHY,
    .phy_ptoc                   = LE_AUDIO_CLIENT_Q2Q_PHY,
    .rtn_ctop                   = LE_AUDIO_CLIENT_APTX_LITE_RTN_GAME_WITH_VBC_C_TO_P,
    .rtn_ptoc                   = LE_AUDIO_CLIENT_APTX_LITE_RTN_GAME_WITH_VBC_P_TO_C
};

#endif /* INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE */

const le_audio_client_audio_config_t* leAudioClient_GetAudioConfig(CapClientContext audio_context, uint8 codec)
{
    le_audio_client_audio_config_t *audio_configuration = NULL;

#ifndef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
    UNUSED(codec);
#endif

    switch (audio_context)
    {
        case CAP_CLIENT_CONTEXT_TYPE_GAME:
#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
            audio_configuration = codec == KYMERA_LE_AUDIO_CODEC_APTX_LITE ? &le_audio_client_audio_config_gaming_aptx_lite : &le_audio_client_audio_config_gaming;
#else
            audio_configuration = &le_audio_client_audio_config_gaming;
#endif
        break;

        case CAP_CLIENT_CONTEXT_TYPE_GAME_WITH_VBC:
#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
            audio_configuration = codec == KYMERA_LE_AUDIO_CODEC_APTX_LITE ? &le_audio_client_audio_config_gaming_with_vbc_aptx_lite : &le_audio_client_audio_config_gaming_with_vbc;
#else
            audio_configuration = &le_audio_client_audio_config_gaming_with_vbc;
#endif
        break;

        case CAP_CLIENT_CONTEXT_TYPE_MEDIA:
#ifdef INCLUDE_LE_APTX_ADAPTIVE
            audio_configuration = codec == KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE ? &le_audio_client_audio_config_aptx_adaptive_media : &le_audio_client_audio_config_media;
#else
            audio_configuration = &le_audio_client_audio_config_media;
#endif
        break;

        case CAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL:
            audio_configuration = &le_audio_client_audio_config_conversational;
        break;

        default:
        break;
    }

    return audio_configuration;
}

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

void LeAudioClient_RegisterConfigInterface(const le_audio_client_config_interface_t * lea_audio_config)
{
    PanicNull((void *)lea_audio_config);

    le_audio_config_callback_configs = lea_audio_config;
}

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

void leAudioClient_AudioConfigInit(void)
{
#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
    lea_client_bcast_config.sub_group_info =
            (TmapClientBigSubGroup*) PanicUnlessMalloc(sizeof(TmapClientBigSubGroup) +
                                                       (TMAP_BROADCAST_MAX_SUPPORTED_BIS *
                                                        sizeof(TmapClientBisInfo)));
#endif
}

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

void leAudioClient_SetBroadcastAudioConfigType(le_audio_client_broadcast_config_type_t config_type)
{
    lea_client_bcast_config.config_type = config_type;
}

static BapAudioLocation leAudioClient_GetAudioLocation(uint16 no_of_bis, uint16 no_of_audio_channels_per_bis, bool is_right)
{
    BapAudioLocation audio_location;

    if (no_of_bis == 2)
    {
        /* Dual BIS configuration */
        audio_location = is_right ? BAP_AUDIO_LOCATION_FR : BAP_AUDIO_LOCATION_FL;
    }
    else
    {
        /* Mono or Stereo BIS configuration */
        audio_location = (no_of_audio_channels_per_bis == 2) ? LE_AUDIO_CLIENT_STEREO_BIS_BROADCAST : BAP_AUDIO_LOCATION_MONO;
    }

    return audio_location;
}

static void leAudioClient_SetBroadcastBisInfo(TmapClientBigSubGroup *subgroup_info, LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *bcast_config)
{
    uint16 i;

    subgroup_info->numBis = bcast_config->number_of_bis;

    for (i = 0; i < subgroup_info->numBis; i++)
    {
        /* bisInfo[0] is always for Audio location Left */
        subgroup_info->bisInfo[i].audioLocation = leAudioClient_GetAudioLocation(bcast_config->number_of_bis,
                                                                                 bcast_config->no_of_audio_channels_per_bis,
                                                                                 i != 0);
        subgroup_info->bisInfo[i].config = bcast_config->broadcast_stream_capability;
        subgroup_info->bisInfo[i].targetLatency = bcast_config->target_latency;
        subgroup_info->bisInfo[i].lc3BlocksPerSdu = LE_AUDIO_CLIENT_BROADCAST_LC3_BLOCKS_PER_SDU;
    }
}

const le_audio_client_audio_broadcast_config_t* leAudioClient_GetBroadcastAudioConfig(void)
{
    le_audio_client_audio_broadcast_config_t *audio_configuration = NULL;

    switch (lea_client_bcast_config.config_type)
    {
        case LE_AUDIO_CLIENT_BROADCAST_CONFIG_TYPE_HQ:
        {
            LE_AUDIO_CLIENT_BROADCAST_CONFIG_T bcast_audio_config;
            LE_AUDIO_CLIENT_BROADCAST_NAME_CODE_T bcast_name_code;
            memset(&bcast_audio_config, 0, sizeof(bcast_audio_config));
            memset(&bcast_name_code, 0, sizeof(bcast_name_code));

            audio_configuration = &lea_client_bcast_config;
            le_audio_config_callback_configs->GetBroadcastAudioConfig(&bcast_audio_config);
            le_audio_config_callback_configs->GetBroadcastNameAndEncryptionCode(&bcast_name_code);
            audio_configuration->broadcast_type = bcast_audio_config.broadcast_type;
            audio_configuration->num_sub_group = LE_AUDIO_CLIENT_BROADCAST_NUM_SUB_GROUPS;
            audio_configuration->presentation_delay = bcast_audio_config.presentation_delay;
            audio_configuration->broadcast_id = le_audio_config_callback_configs->GetBroadcastId();
            audio_configuration->sub_group_info->config = bcast_audio_config.broadcast_stream_capability;
            audio_configuration->sub_group_info->targetLatency = bcast_audio_config.target_latency;
            audio_configuration->sub_group_info->lc3BlocksPerSdu = LE_AUDIO_CLIENT_BROADCAST_LC3_BLOCKS_PER_SDU;
            audio_configuration->sub_group_info->useCase = bcast_audio_config.audio_context;
            audio_configuration->sub_group_info->metadataLen = 0;
            audio_configuration->sub_group_info->metadata = NULL;
            audio_configuration->broadcast_config_params.rtn = bcast_audio_config.rtn;
            audio_configuration->broadcast_config_params.sduSize = bcast_audio_config.sdu_size;
            audio_configuration->broadcast_config_params.maxCodecFramesPerSdu = bcast_audio_config.max_codec_frames_per_sdu;
            audio_configuration->broadcast_config_params.targetLatency = bcast_audio_config.target_latency;
            audio_configuration->broadcast_config_params.maxLatency = bcast_audio_config.max_latency;
            audio_configuration->broadcast_config_params.phy = bcast_audio_config.phy;
            audio_configuration->broadcast_config_params.sduInterval = bcast_audio_config.sdu_interval;
            audio_configuration->broadcast_config_params.subGroupConfig = bcast_audio_config.broadcast_stream_capability;

            leAudioClient_SetBroadcastBisInfo(audio_configuration->sub_group_info, &bcast_audio_config);

            audio_configuration->broadcast_source_name = bcast_name_code.broadcast_source_name;
            audio_configuration->broadcast_source_name_len = bcast_name_code.broadcast_source_name_len;
            audio_configuration->broadcast_code = bcast_name_code.broadcast_code;

            DEBUG_LOG("leAudioClient_GetBroadcastAudioConfig Stream config: 0x%x, targetLatency %d, useCase %d, name len %d, Encrypted %d",
                        bcast_audio_config.broadcast_stream_capability,
                        bcast_audio_config.target_latency,
                        bcast_audio_config.audio_context,
                        audio_configuration->broadcast_source_name_len,
                        audio_configuration->broadcast_code != NULL);
            DEBUG_LOG("    rtn: 0x%x, sdu_size 0x%x, max_codec_frame_per_sdu 0x%x, max_latency 0x%x, phy: 0x%x, sdu_interval: 0x%x",
                        bcast_audio_config.rtn,
                        bcast_audio_config.sdu_size,
                        bcast_audio_config.max_codec_frames_per_sdu,
                        bcast_audio_config.max_latency,
                        bcast_audio_config.phy,
                        bcast_audio_config.sdu_interval);
        }
        break;

        default:
        {
            Panic();
        }
        break;
    }

    return audio_configuration;
}

void leAudioClient_GetBroadcastAdvConfig(CapClientBcastSrcAdvParams *bcast_adv_settings)
{
    CapClientBcastSrcAdvParams *bcast_adv_params;

    le_audio_config_callback_configs->GetBroadcastAdvParams(&bcast_adv_params);

    bcast_adv_settings->advEventProperties = bcast_adv_params->advEventProperties;
    bcast_adv_settings->advIntervalMin = bcast_adv_params->advIntervalMin;
    bcast_adv_settings->advIntervalMax = bcast_adv_params->advIntervalMax;
    bcast_adv_settings->primaryAdvPhy = bcast_adv_params->primaryAdvPhy;
    bcast_adv_settings->primaryAdvChannelMap = bcast_adv_params->primaryAdvChannelMap;
    bcast_adv_settings->secondaryAdvMaxSkip = bcast_adv_params->secondaryAdvMaxSkip;
    bcast_adv_settings->secondaryAdvPhy = bcast_adv_params->secondaryAdvPhy;
    bcast_adv_settings->advSid = bcast_adv_params->advSid;
    bcast_adv_settings->periodicAdvIntervalMin = bcast_adv_params->periodicAdvIntervalMin;
    bcast_adv_settings->periodicAdvIntervalMax = bcast_adv_params->periodicAdvIntervalMax;
    bcast_adv_settings->advertisingTransmitPower = bcast_adv_params->advertisingTransmitPower;
}

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
