/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       headset_lea_src_config.c
\brief      Speaker lea source configuration module
*/

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

#include "headset_lea_src_config.h"
#include "ps_key_map.h"

#include <local_addr.h>
#include <stdio.h>
#include <logging.h>


#define HEADSET_LEA_DEF_BROADCAST_CODE_PREFIX_LEN                       4u

#define HEADSET_LEA_DEF_BROADCAST_NAME                                  "Broadcast%04x"
#define HEADSET_LEA_DEF_BROADCAST_NAME_LEN                              14u
#define HEADSET_LEA_DEF_BROADCAST_ID_DONT_CARE                          ((uint32)0xFFFFFFFFu)
#define HEADSET_LEA_BROADCAST_ADV_SETTING_LEN                           (sizeof(CapClientBcastSrcAdvParams))
#define HEADSET_LEA_BROADCAST_AUDIO_CONFIG_LEN                          (32u)
#define HEADSET_LEA_BROADCAST_ID_LEN                                    (4)

#define SIZE_FROM_UINT16_TO_UINT8(size)                             (size * sizeof(uint16))
#define SIZE_FROM_UINT8_TO_UINT16(size)                             (size / sizeof(uint16))
#define GET_UINT32_FROM_UINT16_BE_AND_INCR(dest, p_src) \
    dest = (uint32) p_src[0] | ((uint32) p_src[1] << 16u), p_src += 2


typedef struct
{
    uint8 rtn;
    uint8 max_codec_frames_per_sdu;
    uint16 sdu_size;
    uint16 max_latency;
    uint16 phy;
    uint16 number_of_bis;
    uint16 no_of_audio_channels_per_bis;
    uint32 audio_context;
    uint32 broadcast_type;
    uint32 stream_capability;
    uint32 sdu_interval;
    uint32 presentation_delay;
} headset_lea_broadcast_audio_config_t;

/*! \brief Speaker LEA source audio config instance */
static headset_lea_config_data_t headset_lea_config_data;

/*! \brief Speaker LEA Broadcast params instance */
static le_media_config_t lea_broadcast_params;

static void headsetLeaSrcConfig_GetBroadcastConfig(LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *broadcast_audio_config);
static void headsetLeaSrcConfig_GetBroadcastNameAndCode(LE_AUDIO_CLIENT_BROADCAST_NAME_CODE_T *broadcast_name_code);
static void headsetLeaSrcConfig_GetBroadcastAdvParams(CapClientBcastSrcAdvParams **broadcast_adv_param);
static uint32 headsetLeaSrcConfig_GetBroadcastId(void);
static bool headsetLeaSrcConfig_IsPublicBroadcast(void);

static const le_audio_client_config_interface_t headset_lea_config_interfaces =
{
    .GetBroadcastAudioConfig = headsetLeaSrcConfig_GetBroadcastConfig,
    .GetBroadcastNameAndEncryptionCode = headsetLeaSrcConfig_GetBroadcastNameAndCode,
    .GetBroadcastAdvParams = headsetLeaSrcConfig_GetBroadcastAdvParams,
    .GetBroadcastId = headsetLeaSrcConfig_GetBroadcastId
};

static void headsetLeaSrcConfig_GetBroadcastConfig(LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *broadcast_audio_config)
{
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *cfg = headsetLeaSrcConfig_IsPublicBroadcast() ?
            &headset_lea_config_data.pbp_broadcast_audio_config : &headset_lea_config_data.tmap_broadcast_audio_config;

    DEBUG_LOG_FN_ENTRY("headsetLeaSrcConfig_GetBroadcastConfig isPBP: %d", headsetLeaSrcConfig_IsPublicBroadcast());

    broadcast_audio_config->qhs_required = cfg->qhs_required;
    broadcast_audio_config->target_latency = cfg->target_latency;
    broadcast_audio_config->audio_context = cfg->audio_context;
    broadcast_audio_config->broadcast_type = cfg->broadcast_type;
    broadcast_audio_config->number_of_bis = cfg->number_of_bis;
    broadcast_audio_config->no_of_audio_channels_per_bis = cfg->no_of_audio_channels_per_bis;
    broadcast_audio_config->broadcast_stream_capability = cfg->broadcast_stream_capability;
    broadcast_audio_config->rtn = cfg->rtn;
    broadcast_audio_config->max_codec_frames_per_sdu = cfg->max_codec_frames_per_sdu;
    broadcast_audio_config->sdu_size = cfg->sdu_size;
    broadcast_audio_config->max_latency = cfg->max_latency;
    broadcast_audio_config->phy = cfg->phy;
    broadcast_audio_config->sdu_interval = cfg->sdu_interval;
    broadcast_audio_config->number_of_bis = cfg->number_of_bis;
    broadcast_audio_config->no_of_audio_channels_per_bis = cfg->no_of_audio_channels_per_bis;
    broadcast_audio_config->presentation_delay = cfg->presentation_delay;
    broadcast_audio_config->audio_context = cfg->audio_context;
}

static void headsetLeaSrcConfig_GetBroadcastNameAndCode(LE_AUDIO_CLIENT_BROADCAST_NAME_CODE_T *broadcast_name_code)
{
    DEBUG_LOG_FN_ENTRY("headsetLeaSrcConfig_GetBroadcastNameAndCode");

    broadcast_name_code->broadcast_source_name_len = headset_lea_config_data.broadcast_source_name_len;
    broadcast_name_code->broadcast_source_name = headset_lea_config_data.broadcast_source_name;
    broadcast_name_code->broadcast_code = headset_lea_config_data.is_encrypted_broadcast ? headset_lea_config_data.broadcast_code : NULL;
}

static void headsetLeaSrcConfig_GetBroadcastAdvParams(CapClientBcastSrcAdvParams **broadcast_adv_param)
{
    DEBUG_LOG_FN_ENTRY("headsetLeaSrcConfig_GetBroadcastAdvParams");

    *broadcast_adv_param = &headset_lea_config_data.broadcast_adv_param;
}

static uint32 headsetLeaSrcConfig_GetBroadcastId(void)
{
    DEBUG_LOG_FN_ENTRY("headsetLeaSrcConfig_GetBroadcastId ID 0x%4x", headset_lea_config_data.broadcast_id);

    return headset_lea_config_data.broadcast_id;
}


static bool headsetLeaSrcConfig_IsPublicBroadcast(void)
{
    return headset_lea_config_data.public_broadcast_mode;
}

static void headsetLeaSrcConfig_SetDefaultBroadcastConfig(void)
{
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *pbp_cfg = &headset_lea_config_data.pbp_broadcast_audio_config;
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *tmap_cfg = &headset_lea_config_data.tmap_broadcast_audio_config;

    pbp_cfg->qhs_required = FALSE;
    pbp_cfg->target_latency = CAP_CLIENT_TARGET_BALANCE_LATENCY_AND_RELIABILITY;
    pbp_cfg->audio_context = TMAP_CLIENT_CONTEXT_TYPE_MEDIA;
    pbp_cfg->number_of_bis = HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_NUM_BIS;
    pbp_cfg->no_of_audio_channels_per_bis = HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_AUDIO_CHANNELS_PER_BIS;
    pbp_cfg->rtn = HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_NUM_OF_RETRANSMISSION;
    pbp_cfg->max_codec_frames_per_sdu = HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_MAX_CODEC_PER_SDU;
    pbp_cfg->sdu_size = HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_SDU_SIZE;
    pbp_cfg->max_latency = HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_MAX_LATENCY;
    pbp_cfg->phy = BAP_LE_2M_PHY;
    pbp_cfg->sdu_interval = HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_SDU_INTERVAL;
    pbp_cfg->presentation_delay = HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_PRESENTATION_DELAY;
    *tmap_cfg = *pbp_cfg;

    pbp_cfg->broadcast_type = CAP_CLIENT_HQ_PUBLIC_BROADCAST;
    pbp_cfg->broadcast_stream_capability = TMAP_CLIENT_STREAM_CAPABILITY_48_2;

    tmap_cfg->broadcast_type = CAP_CLIENT_TMAP_BROADCAST;
    tmap_cfg->broadcast_stream_capability = TMAP_CLIENT_STREAM_CAPABILITY_48_2;
}

static void headsetLeaSrcConfig_loadDefaultAdvSettings(void)
{
    CapClientBcastSrcAdvParams *bcast_adv_param = &headset_lea_config_data.broadcast_adv_param;

    bcast_adv_param->advEventProperties = 0;
    bcast_adv_param->advIntervalMin = HEADSET_LEA_ADV_INTERVAL_MIN_DEFAULT;
    bcast_adv_param->advIntervalMax = HEADSET_LEA_ADV_INTERVAL_MAX_DEFAULT;
    bcast_adv_param->primaryAdvPhy = BAP_LE_1M_PHY;
    bcast_adv_param->primaryAdvChannelMap = HCI_ULP_ADVERT_CHANNEL_DEFAULT;
    bcast_adv_param->secondaryAdvMaxSkip = 0;
    bcast_adv_param->secondaryAdvPhy = BAP_LE_1M_PHY;
    bcast_adv_param->advSid = CM_EXT_ADV_SID_ASSIGNED_BY_STACK;
    bcast_adv_param->periodicAdvIntervalMin = HEADSET_LEA_PA_INTERVAL_MIN_DEFAULT;
    bcast_adv_param->periodicAdvIntervalMax = HEADSET_LEA_PA_INTERVAL_MAX_DEFAULT;
    bcast_adv_param->advertisingTransmitPower = HEADSET_LEA_ADVERTISEMENT_TX_POWER;
}

static void headsetLeaSrcConfig_LoadNameFromPs(void)
{
    uint16 name_length = PsRetrieve(PS_KEY_BROADCAST_SRC_NAME, NULL, 0);
    uint16 name_length_u8 = SIZE_FROM_UINT16_TO_UINT8(name_length);

    if (name_length_u8 == 0 || name_length_u8 > HEADSET_LEA_DEF_BROADCAST_SRC_NAME_MAX_LEN)
    {
        bdaddr bd_addr;

        /* Invalid name length, set it to default name */
        LocalAddr_GetProgrammedBtAddress(&bd_addr);
        sprintf(headset_lea_config_data.broadcast_source_name, HEADSET_LEA_DEF_BROADCAST_NAME, (uint16) bd_addr.lap & 0xffff);
        headset_lea_config_data.broadcast_source_name_len = HEADSET_LEA_DEF_BROADCAST_NAME_LEN;
        LeAudioClientBroadcast_BroadcastConfigChanged();
    }
    else
    {
        PsRetrieve(PS_KEY_BROADCAST_SRC_NAME, headset_lea_config_data.broadcast_source_name, name_length);
        headset_lea_config_data.broadcast_source_name_len = name_length_u8;
    }
}

static void headsetLeaSrcConfig_LoadCodeFromPs(void)
{
    uint16 code_length = PsRetrieve(PS_KEY_BROADCAST_SRC_CODE, NULL, 0);
    uint8 code_length_u8 = SIZE_FROM_UINT16_TO_UINT8(code_length);

    if (code_length_u8 > HEADSET_LEA_BROADCAST_CODE_MAX_LEN || code_length_u8 < HEADSET_LEA_BROADCAST_CODE_MIN_LEN)
    {
        bdaddr bd_addr;

        headset_lea_config_data.is_encrypted_broadcast = FALSE;

        LocalAddr_GetProgrammedBtAddress(&bd_addr);

        /* Invalid code length, set it to default code */
        sprintf((char*) headset_lea_config_data.broadcast_code, "%04x", (uint16) bd_addr.lap & 0xffff);

        /* Appends 0 to remaining 12 digits of broadcast code */
        memset(&headset_lea_config_data.broadcast_code[HEADSET_LEA_DEF_BROADCAST_CODE_PREFIX_LEN], 0,
               HEADSET_LEA_BROADCAST_CODE_MAX_LEN - HEADSET_LEA_DEF_BROADCAST_CODE_PREFIX_LEN);
    }
    else
    {
        headset_lea_config_data.is_encrypted_broadcast = TRUE;
        PsRetrieve(PS_KEY_BROADCAST_SRC_CODE, headset_lea_config_data.broadcast_code, code_length);
    }
}

static void headsetLeaSrcConfig_LoadBcastAdvFromPs(void)
{
    uint16 key_data[SIZE_FROM_UINT8_TO_UINT16(HEADSET_LEA_BROADCAST_ADV_SETTING_LEN)];
    uint16 adv_length = PsRetrieve(PS_KEY_BROADCAST_SRC_ADV_CONFIG, key_data, sizeof(key_data));

    if (SIZE_FROM_UINT16_TO_UINT8(adv_length) != HEADSET_LEA_BROADCAST_ADV_SETTING_LEN)
    {
        /* Invalid advertisement settings length or no key exists, set it to default advertising settings */
        headsetLeaSrcConfig_loadDefaultAdvSettings();
    }
    else
    {
        memcpy(&headset_lea_config_data.broadcast_adv_param, key_data, HEADSET_LEA_BROADCAST_ADV_SETTING_LEN);
    }
}

static void headsetLeaSrcConfig_LoadBcastIdFromPs(void)
{
    uint16 key_data[SIZE_FROM_UINT8_TO_UINT16(HEADSET_LEA_BROADCAST_ID_LEN)];
    uint16 id_length = PsRetrieve(PS_KEY_BROADCAST_ID, key_data, sizeof(key_data));

    if (SIZE_FROM_UINT16_TO_UINT8(id_length) == HEADSET_LEA_BROADCAST_ID_LEN)
    {
        headset_lea_config_data.broadcast_id = (uint32) key_data[0] | ((uint32) key_data[1] << 16);
    }
    else
    {
        /* Invalid or no broadcast ID, in which case broadcast ID will be randomly generated */
        headset_lea_config_data.broadcast_id = HEADSET_LEA_DEF_BROADCAST_ID_DONT_CARE;
    }
}


static void headsetLeaSrcConfig_UpdateAudioConfig(const headset_lea_broadcast_audio_config_t *audio_config)
{
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *pbp_cfg = &headset_lea_config_data.pbp_broadcast_audio_config;
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *tmap_cfg = &headset_lea_config_data.tmap_broadcast_audio_config;

    if (audio_config->broadcast_type == CAP_CLIENT_TMAP_BROADCAST)
    {
        HeadsetLeaSrcConfig_SetPbpBroadcastmode(FALSE);

        tmap_cfg->broadcast_type = audio_config->broadcast_type;
        tmap_cfg->broadcast_stream_capability = audio_config->stream_capability;
        tmap_cfg->rtn = audio_config->rtn;
        tmap_cfg->max_codec_frames_per_sdu = audio_config->max_codec_frames_per_sdu;
        tmap_cfg->sdu_size = audio_config->sdu_size;
        tmap_cfg->max_latency = audio_config->max_latency;
        tmap_cfg->phy = audio_config->phy;
        tmap_cfg->sdu_interval = audio_config->sdu_interval;
        tmap_cfg->number_of_bis = audio_config->number_of_bis;
        tmap_cfg->no_of_audio_channels_per_bis = audio_config->no_of_audio_channels_per_bis;
        tmap_cfg->presentation_delay = audio_config->presentation_delay;
        tmap_cfg->audio_context = (uint16) audio_config->audio_context;
    }
    else
    {
        HeadsetLeaSrcConfig_SetPbpBroadcastmode(TRUE);

        pbp_cfg->broadcast_type = audio_config->broadcast_type;
        pbp_cfg->broadcast_stream_capability = audio_config->stream_capability;
        pbp_cfg->rtn = audio_config->rtn;
        pbp_cfg->max_codec_frames_per_sdu = audio_config->max_codec_frames_per_sdu;
        pbp_cfg->sdu_size = audio_config->sdu_size;
        pbp_cfg->max_latency = audio_config->max_latency;
        pbp_cfg->phy = audio_config->phy;
        pbp_cfg->sdu_interval = audio_config->sdu_interval;
        pbp_cfg->number_of_bis = audio_config->number_of_bis;
        pbp_cfg->no_of_audio_channels_per_bis = audio_config->no_of_audio_channels_per_bis;
        pbp_cfg->presentation_delay = audio_config->presentation_delay;
        pbp_cfg->audio_context = (uint16) audio_config->audio_context;
    }

    LeAudioClientBroadcast_BroadcastConfigChanged();
}

static void headsetLeaSrcConfig_LoadBcastAudioConfigFromPs(void)
{
    uint16 key_data[SIZE_FROM_UINT8_TO_UINT16(HEADSET_LEA_BROADCAST_AUDIO_CONFIG_LEN)];
    uint16 adv_length = PsRetrieve(PS_KEY_BROADCAST_SRC_AUDIO_CONFIG, key_data, sizeof(key_data));

    headsetLeaSrcConfig_SetDefaultBroadcastConfig();

    if (SIZE_FROM_UINT16_TO_UINT8(adv_length) != HEADSET_LEA_BROADCAST_AUDIO_CONFIG_LEN)
    {
        /* TMAP broadcast shall use default 48_2, unencrypted
            PBP broadcast shall use default 48_2, HQ (High quality), unencrypted
         */
        HeadsetLeaSrcConfig_SetPbpBroadcastmode(TRUE);
    }
    else
    {
        headset_lea_broadcast_audio_config_t audio_config;

        uint16 *p_key_data = key_data;

        audio_config.rtn = (uint8) ((*p_key_data & 0x00FF));
        audio_config.max_codec_frames_per_sdu =  (uint8) ((*p_key_data & 0xFF00) >> 8);
        p_key_data++;
        audio_config.sdu_size = *p_key_data;
        p_key_data++;
        audio_config.max_latency = *p_key_data;
        p_key_data++;
        audio_config.phy = *p_key_data;
        p_key_data++;
        audio_config.number_of_bis = *p_key_data;
        p_key_data++;
        audio_config.no_of_audio_channels_per_bis = *p_key_data;
        p_key_data++;
        GET_UINT32_FROM_UINT16_BE_AND_INCR(audio_config.audio_context, p_key_data);
        GET_UINT32_FROM_UINT16_BE_AND_INCR(audio_config.broadcast_type, p_key_data);
        GET_UINT32_FROM_UINT16_BE_AND_INCR(audio_config.stream_capability, p_key_data);
        GET_UINT32_FROM_UINT16_BE_AND_INCR(audio_config.sdu_interval, p_key_data);
        GET_UINT32_FROM_UINT16_BE_AND_INCR(audio_config.presentation_delay, p_key_data);
        headsetLeaSrcConfig_UpdateAudioConfig(&audio_config);
    }
}

static uint32 headsetLeaSrcConfig_GetSampleRate(uint16 stream_capability)
{
    uint32 sample_rate = 0;

    switch(stream_capability)
    {
        case TMAP_CLIENT_STREAM_CAPABILITY_48_1 :
        case TMAP_CLIENT_STREAM_CAPABILITY_48_2 :
        case TMAP_CLIENT_STREAM_CAPABILITY_48_3 :
        case TMAP_CLIENT_STREAM_CAPABILITY_48_4 :
        case TMAP_CLIENT_STREAM_CAPABILITY_48_5 :
        case TMAP_CLIENT_STREAM_CAPABILITY_48_6 :
            sample_rate = 48000;
            break;

        default:
            DEBUG_LOG_INFO("headsetLeaSrcConfig_GetSampleRate Not Found");
            break;
    }

    return sample_rate;
}

static appKymeraLeStreamType headsetLeaSrcConfig_GetStreamType(uint16 number_of_bis, uint16 no_of_audio_channels_per_bis)
{
    appKymeraLeStreamType stream_type;

    if (number_of_bis == 2)
    {
        /* Dual BIS configuration */
        stream_type = KYMERA_LE_STREAM_DUAL_MONO;
    }
    else
    {
        if (no_of_audio_channels_per_bis == 2)
        {
            /* Stereo BIS configuration */
            stream_type = KYMERA_LE_STREAM_STEREO_USE_BOTH;
        }
        else
        {
            /* Mono BIS configuration */
            stream_type = KYMERA_LE_STREAM_MONO;
        }
    }
    return stream_type;
}

void HeadsetLeaSrcConfig_SetPbpBroadcastmode(bool enable)
{
    DEBUG_LOG_FN_ENTRY("HeadsetLeaSrcConfig_SetPbpBroadcastmode() Current PBP Mode : %d, new PBP mode: %d",
                       headset_lea_config_data.public_broadcast_mode, enable);

    headset_lea_config_data.public_broadcast_mode = enable;
    LeAudioClientBroadcast_SetPbpMode(enable);
}

void HeadsetLeaSrcConfig_Init(void)
{
    DEBUG_LOG_FN_ENTRY("HeadsetLeaSrcConfig_Init");

    memset(&headset_lea_config_data, 0, sizeof(headset_lea_config_data));

    headsetLeaSrcConfig_LoadNameFromPs();
    headsetLeaSrcConfig_LoadCodeFromPs();
    headsetLeaSrcConfig_LoadBcastAdvFromPs();
    headsetLeaSrcConfig_LoadBcastAudioConfigFromPs();
    headsetLeaSrcConfig_LoadBcastIdFromPs();

    LeAudioClient_RegisterConfigInterface(&headset_lea_config_interfaces);

}

void HeadsetLeaSrcConfig_SetLc3CodecParams(void)
{
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T broadcast_config;
    headsetLeaSrcConfig_GetBroadcastConfig(&broadcast_config);

    /* Set LEA broadcast params */
    lea_broadcast_params.sample_rate = headsetLeaSrcConfig_GetSampleRate(broadcast_config.broadcast_stream_capability);
    lea_broadcast_params.frame_length = broadcast_config.sdu_size;
    lea_broadcast_params.frame_duration = broadcast_config.sdu_interval;
    lea_broadcast_params.stream_type = headsetLeaSrcConfig_GetStreamType(broadcast_config.number_of_bis, broadcast_config.no_of_audio_channels_per_bis);
    lea_broadcast_params.presentation_delay = HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_PRESENTATION_DELAY;
    lea_broadcast_params.codec_type = KYMERA_LE_AUDIO_CODEC_LC3;
    lea_broadcast_params.codec_frame_blocks_per_sdu = broadcast_config.max_codec_frames_per_sdu;
    lea_broadcast_params.transport_latency_big = HEADSET_LEA_CONFIG_BROADCAST_TRANSPORT_LATENCY;
    Kymera_SetLeaBroadcastParams(&lea_broadcast_params);
}

bool HeadsetLeaSrcConfig_SetLeaBroadcastStreamCapability(uint32 broadcast_stream_capability, bool is_public_broadcast)
{
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *pbp_cfg = &headset_lea_config_data.pbp_broadcast_audio_config;
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *tmap_cfg = &headset_lea_config_data.tmap_broadcast_audio_config;

    DEBUG_LOG_FN_ENTRY("HeadsetLeaSrcConfig_SetLeaBroadcastStreamCapability StreamCapability 0x%x, is_public_broadcast: %d",
                       broadcast_stream_capability, is_public_broadcast);

    if (is_public_broadcast)
    {
        TmapClientStreamCapability config = broadcast_stream_capability;

        if (config & TMAP_CLIENT_CODEC_ID_MASK)
        {
            config &= ~TMAP_CLIENT_CODEC_ID_MASK;
        }

        if (!((config == TMAP_CLIENT_STREAM_CAPABILITY_48_1) ||
              (config == TMAP_CLIENT_STREAM_CAPABILITY_48_2) ||
              (config == TMAP_CLIENT_STREAM_CAPABILITY_48_3) ||
              (config == TMAP_CLIENT_STREAM_CAPABILITY_48_4) ||
              (config == TMAP_CLIENT_STREAM_CAPABILITY_48_5) ||
              (config == TMAP_CLIENT_STREAM_CAPABILITY_48_6)))
        {
            pbp_cfg->broadcast_type = CAP_CLIENT_SQ_PUBLIC_BROADCAST;
        }

        pbp_cfg->broadcast_stream_capability = broadcast_stream_capability;
    }
    else
    {
        tmap_cfg->broadcast_stream_capability = broadcast_stream_capability;
    }

    return TRUE;
}

void HeadsetLeaSrcConfig_SetLeaBroadcastCode(const uint8 *code, uint8 length)
{
    if (length <= HEADSET_LEA_BROADCAST_CODE_MAX_LEN)
    {
        bool encrypted = FALSE;

        DEBUG_LOG_FN_ENTRY("HeadsetLeaSrcConfig_SetLeaBroadcastCode");

        memset(&headset_lea_config_data.broadcast_code[0], 0, HEADSET_LEA_BROADCAST_CODE_MAX_LEN);
        if (length > 0 && code != NULL)
        {
            memcpy(headset_lea_config_data.broadcast_code, code, length);
            encrypted = TRUE;
        }

        if (headset_lea_config_data.is_encrypted_broadcast != encrypted)
        {
            LeAudioClientBroadcast_BroadcastConfigChanged();
            headset_lea_config_data.is_encrypted_broadcast = encrypted;
        }
    }
}

void HeadsetLeaSrcConfig_SetLeaBroadcastAudioConfig(uint32 sdu_interval, uint16 sdu_size, uint16 max_transport_latency, uint8 rtn)
{
    DEBUG_LOG_FN_ENTRY("HeadsetLeaSrcConfig_SetLeaBroadcastAudioConfig, sdu_interval = 0x%x, sdu_size = 0x%x, max_transport_latency = 0x%x, rtn = %d",
                       sdu_interval, sdu_size, max_transport_latency, rtn);
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *broadcast_cfg = headsetLeaSrcConfig_IsPublicBroadcast() ?
                &headset_lea_config_data.pbp_broadcast_audio_config : &headset_lea_config_data.tmap_broadcast_audio_config;
    broadcast_cfg->sdu_interval = sdu_interval;
    broadcast_cfg->sdu_size = sdu_size;
    broadcast_cfg->max_latency = max_transport_latency;
    broadcast_cfg->rtn = rtn;
    LeAudioClientBroadcast_BroadcastConfigChanged();
}

bool HeadsetLeaSrcConfig_SetLeaBroadcastID(const uint8 *bcast_id, uint8 length)
{
    DEBUG_LOG_FN_ENTRY("HeadsetLeaSrcConfig_SetLeaBroadcastID, Broadcast ID Length: %d", length);

    if (length == HEADSET_LEA_BROADCAST_ID_LEN)
    {
        uint32 broadcast_id = CONVERT_TO_UINT32(bcast_id);
        headset_lea_config_data.broadcast_id = broadcast_id;
        LeAudioClientBroadcast_BroadcastConfigChanged();
        return TRUE;
    }
    return FALSE;
}

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
