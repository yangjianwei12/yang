/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    av_state_machines
\brief      AV sink callback interface implementation

    Implements the callback interface for the AV sink instance type used for earbuds and headset
*/

#include "av_callback_interface.h"
#include "a2dp.h"
#include "av.h"
#include "av_config.h"
#include <logging.h>
#include <feature.h>
#ifdef USE_SYNERGY
#include <panic.h>
#include <avrcp_lib.h>
#endif /* USE_SYNERGY */
#include "a2dp_profile_caps.h"

#ifdef INCLUDE_APTX_ADAPTIVE
#include "a2dp_profile_caps_aptx_adaptive.h"
#endif
#ifndef INCLUDE_AV_SOURCE

#define APTX_AD_SAMPLING_FREQ_48000 (1 << 4)
#define APTX_AD_SAMPLING_FREQ_44100 (1 << 3)

#define APTX_AD_CHANNEL_MODE_TWS_PLUS (1 << 5)
#define APTX_AD_CHANNEL_MODE_STEREO (1 << 1)

#define APTX_AD_LL_TTP_MIN_IN_1MS  0        // Minimum latency in milliseconds for low-latency mode
#define APTX_AD_LL_TTP_MAX_IN_4MS  75       // Max latency for low-latency mode in 4ms units (i.e. 75*4ms)
#define APTX_AD_HQ_TTP_MIN_IN_1MS  0        // Minimum latency in milliseconds for HQ mode
#define APTX_AD_HQ_TTP_MAX_IN_4MS  75       // Max latency for HQ mode in 4ms units (i.e. 75*4ms)
#define APTX_AD_TWS_TTP_MIN_IN_1MS 100      // Minimum latency in milliseconds for TWS mode
#define APTX_AD_TWS_TTP_MAX_IN_4MS 75       // Max latency for TWS mode in 4ms units (i.e. 75*4ms)

#define APTX_AD_CAPABILITY_EXTENSION_VERSION_NUMBER           0x01
#define APTX_AD_SUPPORTED_FEATURES                            0x0000000F
#define APTX_AD_FIRST_SETUP_PREFERENCE                        0x02
#define APTX_AD_SECOND_SETUP_PREFERENCE                       0x03
#define APTX_AD_THIRD_SETUP_PREFERENCE                        0x03
#define APTX_AD_FOURTH_SETUP_PREFERENCE                       0x03
#define APTX_AD_NO_FURTHER_EXPANSION                          0x00
#define APTX_AD_CAPABILITY_EXTENSION_END                      0x00

/*! Default SBC Capabilities
    Default capabilities that an application can pass to the A2DP library during initialisation.

    Support all features and full bitpool range. Note that we trust the source
    to choose a bitpool value suitable for the Bluetooth bandwidth.
*/
const uint8 sbc_caps_sink[] =
{
    AVDTP_SERVICE_MEDIA_TRANSPORT,
    0,
    AVDTP_SERVICE_MEDIA_CODEC,
    6,
    AVDTP_MEDIA_TYPE_AUDIO<<2,
    AVDTP_MEDIA_CODEC_SBC,

    SBC_SAMPLING_FREQ_44100     | SBC_SAMPLING_FREQ_48000    |
    SBC_CHANNEL_MODE_MONO       | SBC_CHANNEL_MODE_DUAL_CHAN | SBC_CHANNEL_MODE_STEREO    | SBC_CHANNEL_MODE_JOINT_STEREO,

    SBC_BLOCK_LENGTH_4          | SBC_BLOCK_LENGTH_8         | SBC_BLOCK_LENGTH_12        | SBC_BLOCK_LENGTH_16        |
    SBC_SUBBANDS_4              | SBC_SUBBANDS_8             | SBC_ALLOCATION_SNR         | SBC_ALLOCATION_LOUDNESS,

    SBC_BITPOOL_MIN,
    SBC_BITPOOL_HIGH_QUALITY,

    AVDTP_SERVICE_CONTENT_PROTECTION,
    2,
    AVDTP_CP_TYPE_SCMS_LSB,
    AVDTP_CP_TYPE_SCMS_MSB,

    AVDTP_SERVICE_DELAY_REPORTING,
    0
};
#ifndef USE_SYNERGY
const uint8 sbc_caps_src[] =
{
    AVDTP_SERVICE_MEDIA_TRANSPORT,
    0,
    AVDTP_SERVICE_MEDIA_CODEC,
    6,
    AVDTP_MEDIA_TYPE_AUDIO<<2,
    AVDTP_MEDIA_CODEC_SBC,

    SBC_SAMPLING_FREQ_44100     | SBC_SAMPLING_FREQ_48000    |
    SBC_CHANNEL_MODE_MONO,

    SBC_BLOCK_LENGTH_16         | SBC_SUBBANDS_8             | SBC_ALLOCATION_SNR         | SBC_ALLOCATION_LOUDNESS,

    SBC_BITPOOL_MIN,
    SBC_BITPOOL_HIGH_QUALITY
};
#endif

/*! Default AAC/AAC+ Capabilities
    Default capabilities that an application can pass to the A2DP library during initialisation.

    Support all features.
*/
static const uint8 aac_caps_sink[] =
{
    AVDTP_SERVICE_MEDIA_TRANSPORT,
    0,
    AVDTP_SERVICE_MEDIA_CODEC,
    8,
    AVDTP_MEDIA_TYPE_AUDIO << 2,
    AVDTP_MEDIA_CODEC_MPEG2_4_AAC,

    AAC_MPEG2_AAC_LC | AAC_MPEG4_AAC_LC,
    AAC_SAMPLE_44100,
    AAC_SAMPLE_48000 | AAC_CHANNEL_1 | AAC_CHANNEL_2,
    AAC_VBR | AAC_BITRATE_3,
    AAC_BITRATE_4,
    AAC_BITRATE_5,

    AVDTP_SERVICE_CONTENT_PROTECTION,
    2,
    AVDTP_CP_TYPE_SCMS_LSB,
    AVDTP_CP_TYPE_SCMS_MSB,

    AVDTP_SERVICE_DELAY_REPORTING,
    0
};

#if !(defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER))
/*! Default apt-X Capabilities
    Default capabilities that an application can pass to the A2DP library during initialisation.
*/
static const uint8 aptx_caps_sink[] =
{
    AVDTP_SERVICE_MEDIA_TRANSPORT,
    0,
    AVDTP_SERVICE_MEDIA_CODEC,
    9,
    AVDTP_MEDIA_TYPE_AUDIO << 2,
    AVDTP_MEDIA_CODEC_NONA2DP,

    (A2DP_APT_VENDOR_ID >> 24) & 0xFF,    /* A2DP_APT_VENDOR_ID is defined backwards (0x4f000000 for ID 0x4f), so write octets in reverse order */
    (A2DP_APT_VENDOR_ID >> 16) & 0xFF,
    (A2DP_APT_VENDOR_ID >>  8) & 0xFF,
    (A2DP_APT_VENDOR_ID >>  0) & 0xFF,

    (A2DP_CSR_APTX_CODEC_ID >> 8) & 0xFF, /* A2DP_CSR_APTX_CODEC_ID is defined backwares (0x0100 for ID 0x01), so write octets in reverse order */
    (A2DP_CSR_APTX_CODEC_ID >> 0) & 0xFF,

    APTX_SAMPLING_FREQ_44100 | APTX_SAMPLING_FREQ_48000 | APTX_CHANNEL_MODE_STEREO,

    AVDTP_SERVICE_CONTENT_PROTECTION,
    2,
    AVDTP_CP_TYPE_SCMS_LSB,
    AVDTP_CP_TYPE_SCMS_MSB,

    AVDTP_SERVICE_DELAY_REPORTING,
    0
};
#endif /* !(defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER)) */

/* aptX HD is not recomended for TWM products */
#if defined(INCLUDE_APTX_HD) && !defined(INCLUDE_MIRRORING)
/*! Default apt-X Capabilities
    Default capabilities that an application can pass to the A2DP library during initialisation.
*/
static const uint8 aptxhd_caps_sink[] =
{
    AVDTP_SERVICE_MEDIA_TRANSPORT,
    0,
    AVDTP_SERVICE_MEDIA_CODEC,
    13,
    AVDTP_MEDIA_TYPE_AUDIO << 2,
    AVDTP_MEDIA_CODEC_NONA2DP,

    (A2DP_QTI_VENDOR_ID >> 24) & 0xFF,      /* A2DP_QTI_VENDOR_ID is defined backwards (0xd7000000 for ID 0xd7), so write octets in reverse order */
    (A2DP_QTI_VENDOR_ID >> 16) & 0xFF,
    (A2DP_QTI_VENDOR_ID >>  8) & 0xFF,
    (A2DP_QTI_VENDOR_ID >>  0) & 0xFF,

    (A2DP_QTI_APTXHD_CODEC_ID >> 8) & 0xFF, /* A2DP_QTI_APTXHD_CODEC_ID is defined backwards (0x2400 for ID 0x24), so write octets in reverse order */
    (A2DP_QTI_APTXHD_CODEC_ID >> 0) & 0xFF,

    APTX_SAMPLING_FREQ_44100 | APTX_SAMPLING_FREQ_48000 | APTX_CHANNEL_MODE_STEREO,

    APTX_HD_RESERVED_BYTE,
    APTX_HD_RESERVED_BYTE,
    APTX_HD_RESERVED_BYTE,
    APTX_HD_RESERVED_BYTE,

    AVDTP_SERVICE_CONTENT_PROTECTION,
    2,
    AVDTP_CP_TYPE_SCMS_LSB,
    AVDTP_CP_TYPE_SCMS_MSB,

    AVDTP_SERVICE_DELAY_REPORTING,
    0
};
#endif /* defined(INCLUDE_APTX_HD) && !defined(INCLUDE_MIRRORING) */

#ifdef USE_SYNERGY
/*!@{ \name Standard sink endpoints
    \brief Predefined endpoints for audio Sink end point configurations, applicable to standard TWS and incoming TWS+ */
    /*! SBC */
const a2dpSepConfigType av_sbc_snk_sep  = {AV_SEID_SBC_SNK, CSR_BT_AV_AUDIO, CSR_BT_AV_SINK, sizeof(sbc_caps_sink), sbc_caps_sink};
    /*! AAC */
const a2dpSepConfigType av_aac_snk_sep  = {AV_SEID_AAC_SNK, CSR_BT_AV_AUDIO, CSR_BT_AV_SINK, sizeof(aac_caps_sink), aac_caps_sink};

#if !(defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER))
    /*! APTX */
const a2dpSepConfigType av_aptx_snk_sep = {AV_SEID_APTX_SNK, CSR_BT_AV_AUDIO, CSR_BT_AV_SINK, sizeof(aptx_caps_sink), aptx_caps_sink};
#endif /* !(defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER)) */

#if defined(INCLUDE_APTX_HD) && !defined(INCLUDE_MIRRORING)
const a2dpSepConfigType av_aptxhd_snk_sep  = {AV_SEID_APTXHD_SNK, CSR_BT_AV_AUDIO, CSR_BT_AV_SINK, sizeof(aptxhd_caps_sink), aptxhd_caps_sink};
#endif /* defined(INCLUDE_APTX_HD) && !defined(INCLUDE_MIRRORING) */

/*!@} */


#else

/*!@{ \name Standard TWS sink endpoints
    \brief Predefined endpoints for audio Sink end point configurations, applicable to standard TWS and incoming TWS+ */
    /*! SBC */
const sep_config_type av_sbc_snk_sep     = {AV_SEID_SBC_SNK,      DECODE_RESOURCE_ID, sep_media_type_audio, a2dp_sink, TRUE, 0, sizeof(sbc_caps_sink),  sbc_caps_sink};
    /*! AAC */
const sep_config_type av_aac_snk_sep     = {AV_SEID_AAC_SNK,      DECODE_RESOURCE_ID, sep_media_type_audio, a2dp_sink, TRUE, 0, sizeof(aac_caps_sink),  aac_caps_sink};
    /*! APTX */
const sep_config_type av_aptx_snk_sep    = {AV_SEID_APTX_SNK,     DECODE_RESOURCE_ID, sep_media_type_audio, a2dp_sink, TRUE, 0, sizeof(aptx_caps_sink), aptx_caps_sink};
    
#if defined(INCLUDE_APTX_HD) && !defined(INCLUDE_MIRRORING)
const sep_config_type av_aptxhd_snk_sep  = {AV_SEID_APTXHD_SNK,   DECODE_RESOURCE_ID, sep_media_type_audio, a2dp_sink, TRUE, 0, sizeof(aptxhd_caps_sink), aptxhd_caps_sink};
#endif /* defined(INCLUDE_APTX_HD) && !defined(INCLUDE_MIRRORING) */

/*!@} */

/**/
const sep_config_type av_sbc_src_sep     = {AV_SEID_SBC_SRC,      ENCODE_RESOURCE_ID, sep_media_type_audio, a2dp_source, FALSE, 0, sizeof(sbc_caps_src),  sbc_caps_src};

#endif

const avrcp_init_params avrcpConfig =
{
    avrcp_target_and_controller,
    AVRCP_CATEGORY_1,
    AVRCP_CATEGORY_2 | AVRCP_CATEGORY_1,
    AVRCP_VERSION_1_6
};

static const uint8 sink_seids[] = {
#if !(defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER))
                                   AV_SEID_APTX_ADAPTIVE_SNK,
                                   AV_SEID_APTXHD_SNK,
                                   AV_SEID_APTX_SNK,
#endif /* !(defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER)) */

                                   AV_SEID_AAC_SNK,
                                   AV_SEID_SBC_SNK
                                  };

#ifdef USE_SYNERGY
static bool avInterfaceValidateSeps(a2dpSepDataType *seps, uint8 size_seps)
{
    uint8 i, j;

    for (i = 0; i < size_seps; i++)
    {
        /* if no config is supplied then invalid params */
        if (!seps[i].sep_config)
        {
            return FALSE;
        }
        
        /* Valid SEID values are from 0x01..0x3E inclusive */
        if ( !((seps[i].sep_config->seid > 0x00) && (seps[i].sep_config->seid < 0x3F)) )
        {
            return FALSE;
        }
        
        /* Ensure unavailable bit set if SEP initialised as in_use */
        if (seps[i].in_use)
        {
            seps[i].in_use = A2DP_SEP_IS_UNAVAILABLE;
        }
            
        for (j = 0; j < i; ++j)
        {
            /* if any SEIDs are equal then invalid params */
            if (seps[i].sep_config->seid == seps[j].sep_config->seid)
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

static void avInterface_InitSepDataBlock(void *inst)
{
    avInstanceTaskData *av_inst = (avInstanceTaskData *)inst;
    DEBUG_LOG("avInterface_InitSepDataBlock");

    /* Initialize and add the data block with default SEPs */
    appA2dpBlockInit(av_inst);

#ifdef INCLUDE_APTX_ADAPTIVE
    bool enable_adaptive = FeatureVerifyLicense(APTX_ADAPTIVE_DECODE);
#ifdef INCLUDE_MIRRORING
    enable_adaptive |= FeatureVerifyLicense(APTX_ADAPTIVE_MONO_DECODE);
#endif
#endif /* INCLUDE_APTX_ADAPTIVE */

    a2dpSepDataType seps[] = {

#if defined(INCLUDE_APTX_HD) && !defined(INCLUDE_MIRRORING)
        /* Standard sinks */
        { .sep_config = &av_aptxhd_snk_sep,
          .in_use = (FeatureVerifyLicense(APTX_CLASSIC) && appConfigAptxHdEnabled() && AV_CODEC_PS_APTX_HD_ENABLED()) ? 0 : A2DP_SEP_UNAVAILABLE,
        },
#endif /* defined(INCLUDE_APTX_HD) && !defined(INCLUDE_MIRRORING) */

#if !(defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER))
        /* Standard sinks */
        { .sep_config = &av_aptx_snk_sep,
          .in_use     = (FeatureVerifyLicense(APTX_CLASSIC_MONO) && appConfigAptxEnabled() && AV_CODEC_PS_APTX_ENABLED()) ? 0 : A2DP_SEP_IS_UNAVAILABLE,
        },
#endif /* !(defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER)) */

        { .sep_config = &av_aac_snk_sep,
          .in_use     = (appConfigAacEnabled() && AV_CODEC_PS_AAC_ENABLED()) ? 0 : A2DP_SEP_IS_UNAVAILABLE,
        },
        { .sep_config = &av_sbc_snk_sep,
          .in_use = (AV_CODEC_PS_SBC_ENABLED()) ? 0 : A2DP_SEP_UNAVAILABLE,
        },
#ifdef INCLUDE_APTX_ADAPTIVE
        { .sep_config = &av_aptx_adaptive_snk_sep,
          .in_use = (enable_adaptive && appConfigAptxAdaptiveEnabled() && AV_CODEC_PS_APTX_ADAPTIVE_ENABLED()) ? 0 : A2DP_SEP_UNAVAILABLE,
        },
#endif
    };

    if (avInterfaceValidateSeps(seps, ARRAY_DIM(seps)))
    {
        a2dpSepDataType *sep_list = (a2dpSepDataType *)PanicNull(appA2dpBlockAdd(av_inst,
                                                                                 DATA_BLOCK_SEP_LIST,
                                                                                 ARRAY_DIM(seps),
                                                                                 sizeof(a2dpSepDataType)));
        memmove(sep_list, (a2dpSepDataType *)seps, ARRAY_DIM(seps)*sizeof(a2dpSepDataType));
    }
}

static uint8 avInterface_GetA2dpLocalRole(void)
{
    return CSR_BT_AV_SINK;
}

static void avInterface_RegisterMediaPlayer(void)
{
    CsrBtAvrcpMpFeatureMask mpFeatures;
    CsrBtAvrcpNotiMask      notiMask;

    mpFeatures[0] = 0x00000000;
    mpFeatures[1] = CSR_BT_AVRCP_FEATURE_MASK_1_VOLUME_UP | CSR_BT_AVRCP_FEATURE_MASK_1_VOLUME_DOWN;
    mpFeatures[2] = 0x00000000;
    mpFeatures[3] = 0x00000000;

    notiMask = CSR_BT_AVRCP_NOTI_FLAG_VOLUME;

    AvrcpTgMpRegisterReqSend(&(AvGetTaskData()->task),                              /* Player handle */
                                 notiMask,                                          /* Notifications */
                                 CSR_BT_AVRCP_TG_MP_REGISTER_CONFIG_SET_DEFAULT,    /* default configuration */
                                 0,
                                 NULL,
                                 CSR_BT_AVRCP_MP_TYPE_MAJOR_AUDIO,
                                 CSR_BT_AVRCP_MP_TYPE_SUB_AUDIO_BOOK,
                                 mpFeatures,
                                 (CsrUtf8String *)(CsrCharString*)CsrStrDup("CAA Player"));

}
#endif

static void avInterface_InitialiseA2dp(Task client_task)
{
#ifdef USE_SYNERGY
    /* Initialise A2DP role */
    AvActivateReqSend(client_task, CSR_BT_AV_AUDIO_SINK);

#ifdef INCLUDE_APTX_ADAPTIVE
    /* Initialise the structure used by adaptive */
    A2dpProfileAptxAdInitServiceCapability();
#endif
#else /* USE_SYNERGY */
    /* Initialise A2DP role */
    uint16 role = A2DP_INIT_ROLE_SINK;
#ifdef INCLUDE_APTX_ADAPTIVE
     bool enable_adaptive = FeatureVerifyLicense(APTX_ADAPTIVE_DECODE);
#ifdef INCLUDE_MIRRORING
     enable_adaptive |= FeatureVerifyLicense(APTX_ADAPTIVE_MONO_DECODE);
#endif  /* INCLUDE_MIRRORING */
     /* Initialise the structure used by adaptive */
     A2dpProfileAptxAdInitServiceCapability();
#endif  /* INCLUDE_APTX_ADAPTIVE */
     /* Initialise the Stream Endpoints... */
     sep_data_type seps[] = {
     
         /* Standard sinks */
#if defined(INCLUDE_APTX_HD) && !defined(INCLUDE_MIRRORING)
         { .sep_config = &av_aptxhd_snk_sep,
           .in_use = (FeatureVerifyLicense(APTX_CLASSIC) && appConfigAptxHdEnabled() && AV_CODEC_PS_APTX_HD_ENABLED()) ? 0 : A2DP_SEP_UNAVAILABLE,
         },
#endif /* defined(INCLUDE_APTX_HD) && !defined(INCLUDE_MIRRORING) */

         { .sep_config = &av_aptx_snk_sep,
           .in_use = (FeatureVerifyLicense(APTX_CLASSIC_MONO) && appConfigAptxEnabled() && AV_CODEC_PS_APTX_ENABLED()) ? 0 : A2DP_SEP_UNAVAILABLE,
         },
         { .sep_config = &av_aac_snk_sep,
           .in_use = (appConfigAacEnabled() && AV_CODEC_PS_AAC_ENABLED())? 0 : A2DP_SEP_UNAVAILABLE,
         },
         { .sep_config = &av_sbc_snk_sep,
           .in_use = (AV_CODEC_PS_SBC_ENABLED()) ? 0 : A2DP_SEP_UNAVAILABLE,
         },
#ifdef INCLUDE_APTX_ADAPTIVE
         { .sep_config = &av_aptx_adaptive_snk_sep,
           .in_use = (enable_adaptive && appConfigAptxAdaptiveEnabled() && AV_CODEC_PS_APTX_ADAPTIVE_ENABLED()) ? 0 : A2DP_SEP_UNAVAILABLE,
         },
#endif /* INCLUDE_APTX_ADAPTIVE */
     };
     DEBUG_LOG("avInterface_InitialiseA2dp");
     /* Initialise the A2DP Library */
     A2dpInit(client_task, role, 0, ARRAY_DIM(seps), seps, 0);
#endif /* USE_SYNERGY */
}

static void avInterface_Initialise(void)
{
}

static uint16 avInterface_GetMediaChannelSeids(const uint8** seid_list_out)
{
    *seid_list_out = sink_seids;
    return ARRAY_DIM(sink_seids);
}

static uint16 avInterface_GetAvrcpEvents(void)
{
    uint16 events = appAvrcpEventIdToMask(avrcp_event_playback_status_changed);
    return events;
}

static const avrcp_init_params * avInterface_GetAvrcpConfig(void)
{
    return &avrcpConfig;
}

#ifdef USE_SYNERGY
static void avInterface_AvrcpConfigureRole(CsrBtAvrcpRoleDetails* ctFeatures, CsrBtAvrcpRoleDetails* tgFeatures)
{
    AvrcpConfigRoleSupport(tgFeatures,                                              /* Pointer to details */
                           CSR_BT_AVRCP_CONFIG_ROLE_STANDARD,                       /* Role config */
                           CSR_BT_AVRCP_CONFIG_SR_VERSION_16,                       /* AVRCP version */
                           CSR_BT_AVRCP_CONFIG_SR_FEAT_CAT2_MON_AMP,                /* Features */
                           (CsrCharString*)CsrStrDup(AVRCP_CONFIG_PROVIDER_NAME), /* Provider name */
                           (CsrCharString*)CsrStrDup("AVRCP TG"));                /* Service name */
    AvrcpConfigRoleSupport(ctFeatures,
                           CSR_BT_AVRCP_CONFIG_ROLE_STANDARD,
                           CSR_BT_AVRCP_CONFIG_SR_VERSION_16,
                           CSR_BT_AVRCP_CONFIG_SR_FEAT_CAT1_PLAY_REC,
                           (CsrCharString*)CsrStrDup(AVRCP_CONFIG_PROVIDER_NAME),
                           (CsrCharString*)CsrStrDup("AVRCP CT"));
}
#endif /* USE_SYNERGY */

const av_callback_interface_t av_plugin_interface = {
    .Initialise = avInterface_Initialise,
    .InitialiseA2dp = avInterface_InitialiseA2dp,
    .GetMediaChannelSeids = avInterface_GetMediaChannelSeids,
    .OnAvrcpPlay = NULL,
    .OnAvrcpPause = NULL,
    .OnAvrcpForward = NULL,
    .OnAvrcpBackward = NULL,
    .GetAvrcpEvents = avInterface_GetAvrcpEvents,
    .GetAvrcpConfig = avInterface_GetAvrcpConfig,
#ifdef USE_SYNERGY
    .InitialiseA2dpDataBlock = avInterface_InitSepDataBlock,
    .AvrcpConfigureRole = avInterface_AvrcpConfigureRole,
    .GetA2dpLocalRole = avInterface_GetA2dpLocalRole,
    .AvrcpRegisterMediaPlayer = avInterface_RegisterMediaPlayer,
#endif /* USE_SYNERGY */
};

#endif
