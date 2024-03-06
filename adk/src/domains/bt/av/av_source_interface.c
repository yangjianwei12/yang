/*!
\copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    av_state_machines
\brief      AV Source callback interface implementation

    Implements the callback interface for the AV source instance type
*/

#ifdef INCLUDE_AV_SOURCE

#include "av_callback_interface.h"
#include "a2dp.h"
#include "macros.h"
#include "av.h"
#include <logging.h>
#include <message.h>
#include <audio_sources.h>
#include "a2dp_profile_caps.h"

#ifdef USE_SYNERGY
#include <panic.h>
#endif /* USE_SYNERGY */

#ifdef INCLUDE_APTX_ADAPTIVE
#include "a2dp_profile_caps_aptx_adaptive.h"
#endif


const uint8 sbc_caps_src[] =
{
    AVDTP_SERVICE_MEDIA_TRANSPORT,
    0,
    AVDTP_SERVICE_MEDIA_CODEC,
    6,
    AVDTP_MEDIA_TYPE_AUDIO<<2,
    AVDTP_MEDIA_CODEC_SBC,

    SBC_SAMPLING_FREQ_48000    |
    SBC_CHANNEL_MODE_JOINT_STEREO   |
    SBC_CHANNEL_MODE_MONO,

    SBC_BLOCK_LENGTH_16         | SBC_SUBBANDS_8             | SBC_ALLOCATION_SNR         | SBC_ALLOCATION_LOUDNESS,

    SBC_BITPOOL_MIN,
    SBC_BITPOOL_HIGH_QUALITY,

    AVDTP_SERVICE_CONTENT_PROTECTION,
    2,
    AVDTP_CP_TYPE_SCMS_LSB,
    AVDTP_CP_TYPE_SCMS_MSB,
};

static const uint8 aptx_classic_src_caps[] =
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

    (A2DP_CSR_APTX_CODEC_ID >> 8) & 0xFF, /* A2DP_CSR_APTX_CODEC_ID is defined backwards (0x0100 for ID 0x01), so write octets in reverse order */
    (A2DP_CSR_APTX_CODEC_ID >> 0) & 0xFF,

	APTX_SAMPLING_FREQ_44100 | APTX_SAMPLING_FREQ_48000 | APTX_CHANNEL_MODE_STEREO,

    AVDTP_SERVICE_CONTENT_PROTECTION,
    2,
    AVDTP_CP_TYPE_SCMS_LSB,
    AVDTP_CP_TYPE_SCMS_MSB,
};

static const uint8 aptxhd_src_caps[] =
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
};

#ifdef USE_SYNERGY
const a2dpSepConfigType av_sbc_src_sep = {AV_SEID_SBC_SRC, CSR_BT_AV_AUDIO, CSR_BT_AV_SOURCE, sizeof(sbc_caps_src),  sbc_caps_src};
const a2dpSepConfigType av_aptx_classic_src_sep     = {AV_SEID_APTX_CLASSIC_SRC, CSR_BT_AV_AUDIO, CSR_BT_AV_SOURCE, sizeof(aptx_classic_src_caps),  aptx_classic_src_caps};
const a2dpSepConfigType av_aptxhd_src_sep = {AV_SEID_APTXHD_SRC, CSR_BT_AV_AUDIO, CSR_BT_AV_SOURCE, sizeof(aptxhd_src_caps),  aptxhd_src_caps};
#else
const sep_config_type av_sbc_src_sep     = {AV_SEID_SBC_SRC,      ENCODE_RESOURCE_ID, sep_media_type_audio, a2dp_source, TRUE, 0, sizeof(sbc_caps_src),  sbc_caps_src};
const sep_config_type av_aptx_classic_src_sep     = {AV_SEID_APTX_CLASSIC_SRC,      ENCODE_RESOURCE_ID, sep_media_type_audio, a2dp_source, TRUE, 0, sizeof(aptx_classic_src_caps),  aptx_classic_src_caps};
const sep_config_type av_aptxhd_src_sep     = {AV_SEID_APTXHD_SRC,      ENCODE_RESOURCE_ID, sep_media_type_audio, a2dp_source, TRUE, 0, sizeof(aptxhd_src_caps),  aptxhd_src_caps};
#endif

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

static void avSourceInterface_InitSepDataBlock(void *inst)
{
    avInstanceTaskData *av_inst = (avInstanceTaskData *)inst;
    DEBUG_LOG("avInterface_InitSepDataBlock");

    /* Initialize and add the data block with default SEPs */
    appA2dpBlockInit(av_inst);

    a2dpSepDataType seps[] = {
        /* Standard sources */
#ifdef INCLUDE_APTX_ADAPTIVE
        { .sep_config = &av_aptx_adaptive_src_sep,
          .in_use = AV_CODEC_PS_APTX_ADAPTIVE_ENABLED() ? 0 : A2DP_SEP_UNAVAILABLE,
        },
#endif /* INCLUDE_APTX_ADAPTIVE */
#ifdef INCLUDE_APTX_HD
        { .sep_config = &av_aptxhd_src_sep,
          .in_use = AV_CODEC_PS_APTX_HD_ENABLED() ? 0 : A2DP_SEP_UNAVAILABLE,
        },
#endif /* INCLUDE_APTX_HD */
        { .sep_config = &av_aptx_classic_src_sep,
          .in_use = AV_CODEC_PS_APTX_ENABLED() ? 0 : A2DP_SEP_UNAVAILABLE,
        },
        { .sep_config = &av_sbc_src_sep,
          .in_use = AV_CODEC_PS_SBC_ENABLED() ? 0 : A2DP_SEP_UNAVAILABLE,
        }
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

static uint8 avSourceInterface_GetA2dpLocalRole(void)
{
    return CSR_BT_AV_SOURCE;
}

static void avSourceInterface_RegisterMediaPlayer(void)
{
    CsrBtAvrcpMpFeatureMask mpFeatures;
    CsrBtAvrcpNotiMask      notiMask;
    
    mpFeatures[0] = 0x00000000;
    mpFeatures[1] = CSR_BT_AVRCP_FEATURE_MASK_1_VOLUME_UP | CSR_BT_AVRCP_FEATURE_MASK_1_VOLUME_DOWN | CSR_BT_AVRCP_FEATURE_MASK_1_PLAY | CSR_BT_AVRCP_FEATURE_MASK_1_PAUSE | CSR_BT_AVRCP_FEATURE_MASK_1_STOP | CSR_BT_AVRCP_FEATURE_MASK_1_FORWARD | CSR_BT_AVRCP_FEATURE_MASK_1_BACKWARD;
    mpFeatures[2] = 0x00000000;
    mpFeatures[3] = 0x00000000;

   notiMask = CSR_BT_AVRCP_NOTI_FLAG_PLAYBACK_STATUS | 
#ifdef INCLUDE_AVRCP_BROWSING
              CSR_BT_AVRCP_NOTI_FLAG_ADDRESSED_PLAYER |
#endif 
#ifdef INCLUDE_AVRCP_METADATA
              CSR_BT_AVRCP_NOTI_FLAG_TRACK |
#endif
              CSR_BT_AVRCP_NOTI_FLAG_VOLUME;

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
#endif /* USE_SYNERGY */

const avrcp_init_params avrcpConfig =
{
    avrcp_target_and_controller,
    AVRCP_CATEGORY_2,
    AVRCP_CATEGORY_2 | AVRCP_CATEGORY_1,
    AVRCP_VERSION_1_6
};

static void avSourceInterface_InitialiseA2dp(Task client_task)
{
#ifndef USE_SYNERGY
    /* Initialise A2DP role */
    uint16 role = A2DP_INIT_ROLE_SOURCE;

    sep_data_type seps[] = {
        /* Standard sinks */
#ifdef INCLUDE_APTX_ADAPTIVE
        { .sep_config = &av_aptx_adaptive_src_sep,
          .in_use = (AV_CODEC_PS_APTX_ADAPTIVE_ENABLED()) ? 0 : A2DP_SEP_UNAVAILABLE,
        },
#endif
#ifdef INCLUDE_APTX_HD
        { .sep_config = &av_aptxhd_src_sep,
          .in_use = (AV_CODEC_PS_APTX_HD_ENABLED()) ? 0 : A2DP_SEP_UNAVAILABLE,
        },
#endif /* INCLUDE_APTX_HD */
        { .sep_config = &av_aptx_classic_src_sep,
          .in_use = (AV_CODEC_PS_APTX_ENABLED()) ? 0 : A2DP_SEP_UNAVAILABLE,
        },
        { .sep_config = &av_sbc_src_sep,
          .in_use = (AV_CODEC_PS_SBC_ENABLED()) ? 0 : A2DP_SEP_UNAVAILABLE,
        }
    };

    DEBUG_LOG("appAvEnterInitialisingA2dp");
    /* Initialise the A2DP Library */
    A2dpInit(client_task, role, 0, ARRAY_DIM(seps), seps, 0);
#else
    /* Initialise A2DP role */
    AvActivateReqSend(client_task, CSR_BT_AV_AUDIO_SOURCE);
#endif
}

static void avSourceInterface_Initialise(void)
{
    DEBUG_LOG_VERBOSE("avSourceInterface_Initialise");
}

static uint16 avSourceInterface_GetMediaChannelSeids(const uint8** seid_list_out)
{
    *seid_list_out = NULL;
    return 0;
}

static bool avSourceInterface_AvrcpPlay(void *av_instance, bool pressed)
{
    if (pressed)
    {
        /* USB does not report play state of media
         * therefore will toggle rather than mapping to
         * AudioSources_Play()
         */
        AudioSources_PlayPause(audio_source_usb);

        avInstanceTaskData * theInst = (avInstanceTaskData *)av_instance;

        /* Make sure someone else hasn't already sent the same notification */
        avrcp_play_status status = theInst->avrcp.play_status;

        if (status != avrcp_play_status_playing)
        {
            appAvAvrcpPlayStatusNotification(theInst, avrcp_play_status_playing);
        }
    }

    return TRUE;
}

static bool avSourceInterface_AvrcpPause(void *av_instance, bool pressed)
{
    if (pressed)
    {
        /* USB does not report play state of media
         * therefore will toggle rather than mapping to
         * AudioSources_Pause()
         */
        AudioSources_PlayPause(audio_source_usb);

        avInstanceTaskData * theInst = (avInstanceTaskData *)av_instance;

        /* Make sure someone else hasn't already sent the same notification */
        avrcp_play_status status = theInst->avrcp.play_status;

        if (status != avrcp_play_status_paused && status != avrcp_play_status_stopped)
        {
            appAvAvrcpPlayStatusNotification(theInst, avrcp_play_status_paused);
        }
    }

    return TRUE;
}

static bool avSourceInterface_AvrcpForward(void *av_instance, bool pressed)
{
    if (pressed)
    {
        AudioSources_Forward(audio_source_usb);
        UNUSED(av_instance);
    }

    return TRUE;
}

static bool avSourceInterface_AvrcpBackward(void *av_instance, bool pressed)
{
    if (pressed)
    {
        AudioSources_Back(audio_source_usb);
        UNUSED(av_instance);
    }

    return TRUE;
}

static uint16 avSourceInterface_GetAvrcpEvents(void)
{
    uint16 events = appAvrcpEventIdToMask(avrcp_event_volume_changed);
    return events;
}

static const avrcp_init_params * avSourceInterface_GetAvrcpConfig(void)
{
    return &avrcpConfig;
}

#ifdef USE_SYNERGY
static void avSourceInterface_AvrcpConfigureRole(CsrBtAvrcpRoleDetails* ctFeatures, CsrBtAvrcpRoleDetails* tgFeatures)
{
    AvrcpConfigRoleSupport(tgFeatures,                                           /* Pointer to details */
                           CSR_BT_AVRCP_CONFIG_ROLE_STANDARD | AVRCP_CONFIG_ROLE_NO_BROWSING_AFTER_CONTROL, /* Role config */
                           CSR_BT_AVRCP_CONFIG_SR_VERSION_16,                     /* AVRCP version */
                           CSR_BT_AVRCP_CONFIG_SR_FEAT_CAT1_PLAY_REC |
#ifdef INCLUDE_AVRCP_BROWSING
                           CSR_BT_AVRCP_CONFIG_SR_FEAT_BROWSING | 
#endif
                           CSR_BT_AVRCP_CONFIG_SR_FEAT_CAT2_MON_AMP,                /* Features */
                           (CsrCharString*)CsrStrDup(AVRCP_CONFIG_PROVIDER_NAME), /* Provider name */
                           (CsrCharString*)CsrStrDup("AVRCP TG"));                /* Service name */
    AvrcpConfigRoleSupport(ctFeatures,
                           CSR_BT_AVRCP_CONFIG_ROLE_STANDARD | AVRCP_CONFIG_ROLE_NO_BROWSING_AFTER_CONTROL,
                           CSR_BT_AVRCP_CONFIG_SR_VERSION_16,
#ifdef INCLUDE_AVRCP_BROWSING
                           CSR_BT_AVRCP_CONFIG_SR_FEAT_BROWSING |
#endif
                           CSR_BT_AVRCP_CONFIG_SR_FEAT_CAT2_MON_AMP,
                           (CsrCharString*)CsrStrDup(AVRCP_CONFIG_PROVIDER_NAME),
                           (CsrCharString*)CsrStrDup("AVRCP CT"));
}
#endif /* USE_SYNERGY */

const av_callback_interface_t av_plugin_interface = {
    .Initialise = avSourceInterface_Initialise,
    .InitialiseA2dp = avSourceInterface_InitialiseA2dp,
    .GetMediaChannelSeids = avSourceInterface_GetMediaChannelSeids,
    .OnAvrcpPlay = avSourceInterface_AvrcpPlay,
    .OnAvrcpPause = avSourceInterface_AvrcpPause,
    .OnAvrcpForward = avSourceInterface_AvrcpForward,
    .OnAvrcpBackward = avSourceInterface_AvrcpBackward,
    .GetAvrcpEvents = avSourceInterface_GetAvrcpEvents,
    .GetAvrcpConfig = avSourceInterface_GetAvrcpConfig,
#ifdef USE_SYNERGY
    .InitialiseA2dpDataBlock = avSourceInterface_InitSepDataBlock,
    .AvrcpConfigureRole = avSourceInterface_AvrcpConfigureRole,
    .GetA2dpLocalRole = avSourceInterface_GetA2dpLocalRole,
    .AvrcpRegisterMediaPlayer = avSourceInterface_RegisterMediaPlayer,
#endif /* USE_SYNERGY */

};

#endif /* INCLUDE_AV_SOURCE */
