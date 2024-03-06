/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       usb_dongle_lea_config.c
\brief      UsbDongle lea configuration module
*/

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

#include "usb_dongle_lea_config.h"
#include "ps_key_map.h"
#include "sink_service.h"

#include <local_addr.h>
#include <stdio.h>
#include <logging.h>

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

#define USB_LEA_DEF_BROADCAST_CODE_PREFIX_LEN                       4u

#define USB_LEA_DEF_BROADCAST_NAME                                  "Broadcast%04x"
#define USB_LEA_DEF_BROADCAST_NAME_LEN                              14u
#define USB_LEA_DEF_BROADCAST_ID_DONT_CARE                          ((uint32)0xFFFFFFFFu)
#define USB_LEA_BROADCAST_ADV_SETTING_LEN                           (sizeof(CapClientBcastSrcAdvParams))
#define USB_LEA_BROADCAST_AUDIO_CONFIG_LEN                          (32u)
#define USB_LEA_BROADCAST_ID_LEN                                    (4)

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
} usb_lea_broadcast_audio_config_t;

/*! \brief USB Dongle LEA config data instance */
static usb_dongle_lea_config_data_t usb_dongle_lea_config_data;

static void usbDongle_LeaConfigGetBroadcastConfig(LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *broadcast_audio_config);
static void usbDongle_LeaConfigGetBroadcastNameAndCode(LE_AUDIO_CLIENT_BROADCAST_NAME_CODE_T *broadcast_name_code);
static void usbDongle_LeaConfigGetBroadcastAdvParams(CapClientBcastSrcAdvParams **broadcast_adv_param);
static uint32 usbDongle_LeaConfigGetBroadcastId(void);
static bool usbDongle_LeaConfigIsPublicBroadcast(void);

static const le_audio_client_config_interface_t usb_dongle_lea_config_interfaces =
{
    .GetBroadcastAudioConfig = usbDongle_LeaConfigGetBroadcastConfig,
    .GetBroadcastNameAndEncryptionCode = usbDongle_LeaConfigGetBroadcastNameAndCode,
    .GetBroadcastAdvParams = usbDongle_LeaConfigGetBroadcastAdvParams,
    .GetBroadcastId = usbDongle_LeaConfigGetBroadcastId,
};

static void usbDongle_LeaConfigGetBroadcastConfig(LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *broadcast_audio_config)
{
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *cfg = usbDongle_LeaConfigIsPublicBroadcast() ?
            &usb_dongle_lea_config_data.pbp_broadcast_audio_config : &usb_dongle_lea_config_data.tmap_broadcast_audio_config;

    DEBUG_LOG_FN_ENTRY("usbDongle_LeaConfigGetBroadcastConfig isPBP: %d", usbDongle_LeaConfigIsPublicBroadcast());

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

static void usbDongle_LeaConfigGetBroadcastNameAndCode(LE_AUDIO_CLIENT_BROADCAST_NAME_CODE_T *broadcast_name_code)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_LeaConfigGetBroadcastNameAndCode");

    broadcast_name_code->broadcast_source_name_len = usb_dongle_lea_config_data.broadcast_source_name_len;
    broadcast_name_code->broadcast_source_name = usb_dongle_lea_config_data.broadcast_source_name;
    broadcast_name_code->broadcast_code = usb_dongle_lea_config_data.is_encrypted_broadcast ? usb_dongle_lea_config_data.broadcast_code : NULL;
}

static void usbDongle_LeaConfigGetBroadcastAdvParams(CapClientBcastSrcAdvParams **broadcast_adv_param)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_LeaConfigGetBroadcastAdvParams");

    *broadcast_adv_param = &usb_dongle_lea_config_data.broadcast_adv_param;
}

static uint32 usbDongle_LeaConfigGetBroadcastId(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_LeaConfigGetBroadcastId ID 0x%4x", usb_dongle_lea_config_data.broadcast_id);

    return usb_dongle_lea_config_data.broadcast_id;
}

static bool usbDongle_LeaConfigIsPublicBroadcast(void)
{
    return usb_dongle_lea_config_data.public_broadcast_mode;
}

static void usbDongle_LeaConfigSetDefaultBroadcastConfig(void)
{
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *pbp_cfg = &usb_dongle_lea_config_data.pbp_broadcast_audio_config;
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *tmap_cfg = &usb_dongle_lea_config_data.tmap_broadcast_audio_config;

    pbp_cfg->qhs_required = FALSE;
    pbp_cfg->target_latency = CAP_CLIENT_TARGET_BALANCE_LATENCY_AND_RELIABILITY;
    pbp_cfg->audio_context = TMAP_CLIENT_CONTEXT_TYPE_MEDIA;
    pbp_cfg->number_of_bis = USB_LEA_CONFIG_BROADCAST_DEFAULT_NUM_BIS;
    pbp_cfg->no_of_audio_channels_per_bis = USB_LEA_CONFIG_BROADCAST_DEFAULT_AUDIO_CHANNELS_PER_BIS;
    pbp_cfg->rtn = USB_LEA_CONFIG_BROADCAST_DEFAULT_NUM_OF_RETRANSMISSION;
    pbp_cfg->max_codec_frames_per_sdu = USB_LEA_CONFIG_BROADCAST_DEFAULT_MAX_CODEC_PER_SDU;
    pbp_cfg->sdu_size = USB_LEA_CONFIG_BROADCAST_DEFAULT_SDU_SIZE;
    pbp_cfg->max_latency = USB_LEA_CONFIG_BROADCAST_DEFAULT_MAX_LATENCY;
    pbp_cfg->phy = BAP_LE_2M_PHY;
    pbp_cfg->sdu_interval = USB_LEA_CONFIG_BROADCAST_DEFAULT_SDU_INTERVAL;
    pbp_cfg->presentation_delay = USB_LEA_CONFIG_BROADCAST_DEFAULT_PRESENTATION_DELAY;
    *tmap_cfg = *pbp_cfg;

    pbp_cfg->broadcast_type = CAP_CLIENT_HQ_PUBLIC_BROADCAST;
    pbp_cfg->broadcast_stream_capability = TMAP_CLIENT_STREAM_CAPABILITY_48_2;

    tmap_cfg->broadcast_type = CAP_CLIENT_TMAP_BROADCAST;
    tmap_cfg->broadcast_stream_capability = TMAP_CLIENT_STREAM_CAPABILITY_48_2;
}

static void UsbDongle_LeaConfigloadDefaultAdvSettings(void)
{
    CapClientBcastSrcAdvParams *bcast_adv_param = &usb_dongle_lea_config_data.broadcast_adv_param;

    bcast_adv_param->advEventProperties = 0;
    bcast_adv_param->advIntervalMin = USB_LEA_ADV_INTERVAL_MIN_DEFAULT;
    bcast_adv_param->advIntervalMax = USB_LEA_ADV_INTERVAL_MAX_DEFAULT;
    bcast_adv_param->primaryAdvPhy = BAP_LE_1M_PHY;
    bcast_adv_param->primaryAdvChannelMap = HCI_ULP_ADVERT_CHANNEL_DEFAULT;
    bcast_adv_param->secondaryAdvMaxSkip = 0;
    bcast_adv_param->secondaryAdvPhy = BAP_LE_1M_PHY;
    bcast_adv_param->advSid = CM_EXT_ADV_SID_ASSIGNED_BY_STACK;
    bcast_adv_param->periodicAdvIntervalMin = USB_LEA_PA_INTERVAL_MIN_DEFAULT;
    bcast_adv_param->periodicAdvIntervalMax = USB_LEA_PA_INTERVAL_MAX_DEFAULT;
    bcast_adv_param->advertisingTransmitPower = USB_LEA_ADVERTISEMENT_TX_POWER;
}

static void UsbDongle_LeaConfigLoadNameFromPs(void)
{
    uint16 name_length = PsRetrieve(PS_KEY_BROADCAST_SRC_NAME, NULL, 0);
    uint16 name_length_u8 = SIZE_FROM_UINT16_TO_UINT8(name_length);

    if (name_length_u8 == 0 || name_length_u8 > USB_LEA_DEF_BROADCAST_SRC_NAME_MAX_LEN)
    {
        bdaddr bd_addr;

        /* Invalid name length, set it to default name */
        LocalAddr_GetProgrammedBtAddress(&bd_addr);
        sprintf(usb_dongle_lea_config_data.broadcast_source_name, USB_LEA_DEF_BROADCAST_NAME, (uint16) bd_addr.lap & 0xffff);
        usb_dongle_lea_config_data.broadcast_source_name_len = USB_LEA_DEF_BROADCAST_NAME_LEN;
        LeAudioClientBroadcast_BroadcastConfigChanged();
    }
    else
    {
        PsRetrieve(PS_KEY_BROADCAST_SRC_NAME, usb_dongle_lea_config_data.broadcast_source_name, name_length);
        usb_dongle_lea_config_data.broadcast_source_name_len = name_length_u8;
    }
}

static void UsbDongle_LeaConfigStoreNameToPs(void)
{
    uint16 written_words;
    uint16 length = SIZE_FROM_UINT8_TO_UINT16(usb_dongle_lea_config_data.broadcast_source_name_len);

    written_words = PsStore(PS_KEY_BROADCAST_SRC_NAME,
                            usb_dongle_lea_config_data.broadcast_source_name,
                            length);
    DEBUG_LOG_ALWAYS("UsbDongle_LeaConfigStoreNameToPs Requested-Size: %d, Written-Size: %d",
                     length, written_words);

}

static void UsbDongle_LeaConfigLoadCodeFromPs(void)
{
    uint16 code_length = PsRetrieve(PS_KEY_BROADCAST_SRC_CODE, NULL, 0);
    uint8 code_length_u8 = SIZE_FROM_UINT16_TO_UINT8(code_length);

    if (code_length_u8 > USB_LEA_BROADCAST_CODE_MAX_LEN || code_length_u8 < USB_LEA_BROADCAST_CODE_MIN_LEN)
    {
        bdaddr bd_addr;

        usb_dongle_lea_config_data.is_encrypted_broadcast = FALSE;

        LocalAddr_GetProgrammedBtAddress(&bd_addr);

        /* Invalid code length, set it to default code */
        sprintf((char*) usb_dongle_lea_config_data.broadcast_code, "%04x", (uint16) bd_addr.lap & 0xffff);

        /* Appends 0 to remaining 12 digits of broadcast code */
        memset(&usb_dongle_lea_config_data.broadcast_code[USB_LEA_DEF_BROADCAST_CODE_PREFIX_LEN], 0,
               USB_LEA_BROADCAST_CODE_MAX_LEN - USB_LEA_DEF_BROADCAST_CODE_PREFIX_LEN);
    }
    else
    {
        usb_dongle_lea_config_data.is_encrypted_broadcast = TRUE;
        PsRetrieve(PS_KEY_BROADCAST_SRC_CODE, usb_dongle_lea_config_data.broadcast_code, code_length);
    }
}

static void UsbDongle_LeaConfigStoreCodeToPs(uint8 code_length)
{
    uint16 written_words;
    uint16 length = SIZE_FROM_UINT8_TO_UINT16(code_length);

    written_words = PsStore(PS_KEY_BROADCAST_SRC_CODE, usb_dongle_lea_config_data.broadcast_code, length);
    DEBUG_LOG_ALWAYS("UsbDongle_LeaConfigStoreCodeToPs Requested-Size: %d, Written-Size: %d",
                     length, written_words);
}

static void UsbDongle_LeaConfigLoadBcastAdvFromPs(void)
{
    uint16 key_data[SIZE_FROM_UINT8_TO_UINT16(USB_LEA_BROADCAST_ADV_SETTING_LEN)];
    uint16 adv_length = PsRetrieve(PS_KEY_BROADCAST_SRC_ADV_CONFIG, key_data, sizeof(key_data));

    if (SIZE_FROM_UINT16_TO_UINT8(adv_length) != USB_LEA_BROADCAST_ADV_SETTING_LEN)
    {
        /* Invalid advertisement settings length or no key exists, set it to default advertising settings */
        UsbDongle_LeaConfigloadDefaultAdvSettings();
    }
    else
    {
        memcpy(&usb_dongle_lea_config_data.broadcast_adv_param, key_data, USB_LEA_BROADCAST_ADV_SETTING_LEN);
    }
}

static void usbDongle_UpdateAudioConfig(const usb_lea_broadcast_audio_config_t *audio_config)
{
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *pbp_cfg = &usb_dongle_lea_config_data.pbp_broadcast_audio_config;
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *tmap_cfg = &usb_dongle_lea_config_data.tmap_broadcast_audio_config;

    if (audio_config->broadcast_type == CAP_CLIENT_TMAP_BROADCAST)
    {
        UsbDongle_LeaConfigSetPbpBroadcastmode(FALSE);

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
        UsbDongle_LeaConfigSetPbpBroadcastmode(TRUE);

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

static void UsbDongle_LeaConfigLoadBcastAudioConfigFromPs(void)
{
    uint16 key_data[SIZE_FROM_UINT8_TO_UINT16(USB_LEA_BROADCAST_AUDIO_CONFIG_LEN)];
    uint16 adv_length = PsRetrieve(PS_KEY_BROADCAST_SRC_AUDIO_CONFIG, key_data, sizeof(key_data));

    usbDongle_LeaConfigSetDefaultBroadcastConfig();

    if (SIZE_FROM_UINT16_TO_UINT8(adv_length) != USB_LEA_BROADCAST_AUDIO_CONFIG_LEN)
    {
        /* TMAP broadcast shall use default 48_2, unencrypted
            PBP broadcast shall use default 48_2, HQ (High quality), unencrypted
         */
        UsbDongle_LeaConfigSetPbpBroadcastmode(TRUE);
    }
    else
    {
        usb_lea_broadcast_audio_config_t audio_config;

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
        usbDongle_UpdateAudioConfig(&audio_config);
    }
}

static void UsbDongle_LeaConfigLoadBcastIdFromPs(void)
{
    uint8 broadcast_id[USB_LEA_BROADCAST_ID_LEN];
    uint16 id_length = PsRetrieve(PS_KEY_BROADCAST_ID, broadcast_id, USB_LEA_BROADCAST_ID_LEN);

    if (SIZE_FROM_UINT16_TO_UINT8(id_length) == USB_LEA_BROADCAST_ID_LEN)
    {
        usb_dongle_lea_config_data.broadcast_id = CONVERT_TO_UINT32(broadcast_id);
    }
    else
    {
        /* Invalid or no broadcast ID, in which case broadcast ID will be randomly generated */
        usb_dongle_lea_config_data.broadcast_id = USB_LEA_DEF_BROADCAST_ID_DONT_CARE;
    }
}

bool UsbDongle_LeaConfigSetBroadcastAudioConfig(const uint8 *bcast_audio_config, uint8 len)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_LeaConfigSetBroadcastAudioConfig ADV Length : %d", len);

    if (len == USB_LEA_BROADCAST_AUDIO_CONFIG_LEN)
    {
        usb_lea_broadcast_audio_config_t audio_config;
        uint16 written_words;
        uint16 len_in_u16 = SIZE_FROM_UINT8_TO_UINT16(len);

        audio_config.rtn = bcast_audio_config[0];
        audio_config.max_codec_frames_per_sdu = bcast_audio_config[1];
        audio_config.sdu_size = CONVERT_TO_UINT16(&bcast_audio_config[2]);
        audio_config.max_latency = CONVERT_TO_UINT16(&bcast_audio_config[4]);
        audio_config.phy = CONVERT_TO_UINT16(&bcast_audio_config[6]);
        audio_config.number_of_bis = CONVERT_TO_UINT16(&bcast_audio_config[8]);
        audio_config.no_of_audio_channels_per_bis = CONVERT_TO_UINT16(&bcast_audio_config[10]);
        audio_config.audio_context = CONVERT_TO_UINT32(&bcast_audio_config[12]);
        audio_config.broadcast_type = CONVERT_TO_UINT32(&bcast_audio_config[16]);
        audio_config.stream_capability = CONVERT_TO_UINT32(&bcast_audio_config[20]);
        audio_config.sdu_interval = CONVERT_TO_UINT32(&bcast_audio_config[24]);
        audio_config.presentation_delay = CONVERT_TO_UINT32(&bcast_audio_config[28]);

        usbDongle_UpdateAudioConfig(&audio_config);

        written_words = PsStore(PS_KEY_BROADCAST_SRC_AUDIO_CONFIG, &audio_config, len_in_u16);
        DEBUG_LOG_ALWAYS("UsbDongle_LeaConfigSetBroadcastAudioConfig Requested-Size: %d, Written-Size: %d",
                         len_in_u16, written_words);

        return TRUE;
    }

    return FALSE;
}

bool UsbDongle_LeaConfigSetBroadcastStreamCapability(uint32 broadcast_stream_capability, bool is_public_broadcast)
{
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *pbp_cfg = &usb_dongle_lea_config_data.pbp_broadcast_audio_config;
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *tmap_cfg = &usb_dongle_lea_config_data.tmap_broadcast_audio_config;

    DEBUG_LOG_FN_ENTRY("UsbDongle_LeaConfigSetBroadcastStreamCapability 0x%x, is_public_broadcast: %d",
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

bool UsbDongle_LeaConfigSetBroadcastBisConfig(uint16 number_of_bis, uint16 audio_channels_per_bis)
{
    bool status = FALSE;
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *pbp_cfg = &usb_dongle_lea_config_data.pbp_broadcast_audio_config;
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *tmap_cfg = &usb_dongle_lea_config_data.tmap_broadcast_audio_config;

    DEBUG_LOG_FN_ENTRY("UsbDongle_LeaConfigSetBroadcastBisConfig number_of_bis %d, audio_channels_per_bis %d", number_of_bis, audio_channels_per_bis);

    if (number_of_bis <= 2 && audio_channels_per_bis <= 2)
    {
        tmap_cfg->sdu_size = USB_LEA_CONFIG_BROADCAST_DEFAULT_SDU_SIZE * audio_channels_per_bis;
        tmap_cfg->number_of_bis = number_of_bis;
        tmap_cfg->no_of_audio_channels_per_bis = audio_channels_per_bis;

        pbp_cfg->sdu_size = USB_LEA_CONFIG_BROADCAST_DEFAULT_SDU_SIZE * audio_channels_per_bis;
        pbp_cfg->number_of_bis = number_of_bis;
        pbp_cfg->no_of_audio_channels_per_bis = audio_channels_per_bis;

        status = TRUE;
    }

    return status;
}

void UsbDongle_LeaConfigSetLeaBroadcastCode(const uint8 *code, uint8 len)
{
    if (len <= USB_LEA_BROADCAST_CODE_MAX_LEN)
    {
        bool encrypted = FALSE;

        DEBUG_LOG_FN_ENTRY("UsbDongle_LeaConfigSetLeaBroadcastCode");

        memset(&usb_dongle_lea_config_data.broadcast_code[0], 0, USB_LEA_BROADCAST_CODE_MAX_LEN);
        if (len > 0 && code != NULL)
        {
            memcpy(usb_dongle_lea_config_data.broadcast_code, code, len);
            encrypted = TRUE;
        }

            UsbDongle_LeaConfigStoreCodeToPs(len);

        if (usb_dongle_lea_config_data.is_encrypted_broadcast != encrypted)
        {
            LeAudioClientBroadcast_BroadcastConfigChanged();
            usb_dongle_lea_config_data.is_encrypted_broadcast = encrypted;
        }
    }
}

bool UsbDongle_LeaConfigSetLeaBroadcastSourceName(const char *name, uint8 len)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_LeaConfigSetLeaBroadcastSourceName");

    if (len > 0 && len <= USB_LEA_DEF_BROADCAST_SRC_NAME_MAX_LEN && name != NULL)
    {
        if(len != usb_dongle_lea_config_data.broadcast_source_name_len ||
           memcmp(usb_dongle_lea_config_data.broadcast_source_name, name, len))
        {
            usb_dongle_lea_config_data.broadcast_source_name_len = len;
            memcpy(usb_dongle_lea_config_data.broadcast_source_name, name, len);
            LeAudioClientBroadcast_BroadcastConfigChanged();
        }
        UsbDongle_LeaConfigStoreNameToPs();

        return TRUE;
    }

    DEBUG_LOG_ERROR("UsbDongle_LeaConfigSetLeaBroadcastSourceName Invalid broadcast source name length %d (expected b/w 1 and %d)",
                    SIZE_FROM_UINT8_TO_UINT16(len), USB_LEA_DEF_BROADCAST_SRC_NAME_MAX_LEN);

    return FALSE;
}

void UsbDongle_LeaConfigSetPbpBroadcastmode(bool enable)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_LeaConfigSetPbpBroadcastmode() Current PBP Mode : %d, new PBP mode: %d",
                       usb_dongle_lea_config_data.public_broadcast_mode, enable);

    usb_dongle_lea_config_data.public_broadcast_mode = enable;
    LeAudioClientBroadcast_SetPbpMode(enable);
}

bool UsbDongle_LeaConfigSetLeaBroadcastAdvSettings(const uint8 *adv_setting, uint8 len)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_LeaConfigSetLeaBroadcastAdvSettings() ADV Length : %d", len);

    if (len == USB_LEA_BROADCAST_ADV_SETTING_LEN)
    {
        BapBroadcastSrcAdvParams *adv_params = &usb_dongle_lea_config_data.broadcast_adv_param;
        uint16 written_words;
        uint16 len_in_u16 = SIZE_FROM_UINT8_TO_UINT16(len);

        adv_params->advEventProperties = CONVERT_TO_UINT16(adv_setting);
        adv_setting += 2;
        adv_params->advIntervalMin = CONVERT_TO_UINT16(adv_setting);
        adv_setting += 2;
        adv_params->advIntervalMax = CONVERT_TO_UINT16(adv_setting);
        adv_setting += 2;
        adv_params->primaryAdvPhy = CONVERT_TO_UINT16(adv_setting);
        adv_setting += 2;
        adv_params->primaryAdvChannelMap = adv_setting[0];
        adv_params->secondaryAdvMaxSkip = adv_setting[1];
        adv_setting += 2;
        adv_params->secondaryAdvPhy = CONVERT_TO_UINT16(adv_setting);
        adv_setting += 2;
        adv_params->advSid = CONVERT_TO_UINT16(adv_setting);
        adv_setting += 2;
        adv_params->periodicAdvIntervalMin = CONVERT_TO_UINT16(adv_setting);
        adv_setting += 2;
        adv_params->periodicAdvIntervalMax = CONVERT_TO_UINT16(adv_setting);
        adv_setting += 2;
        adv_params->advertisingTransmitPower = adv_setting[0];

        written_words = PsStore(PS_KEY_BROADCAST_SRC_ADV_CONFIG, adv_params, len_in_u16);
        DEBUG_LOG_ALWAYS("UsbDongle_LeaConfigSetLeaBroadcastAdvSettings Requested-Size: %d, Written-Size: %d",
                         len_in_u16, written_words);

        LeAudioClientBroadcast_BroadcastConfigChanged();

        return TRUE;
    }

    return FALSE;
}

bool UsbDongle_LeaConfigSetLeaBroadcastID(const uint8 *bcast_id, uint8 len)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_LeaConfigSetLeaBroadcastID() Broadcast ID Length: %d", len);

    if (len == USB_LEA_BROADCAST_ID_LEN)
    {
        uint32 broadcast_id = CONVERT_TO_UINT32(bcast_id);
        uint16 written_words;
        uint16 len_in_u16 = SIZE_FROM_UINT8_TO_UINT16(len);

        usb_dongle_lea_config_data.broadcast_id = broadcast_id;

        written_words = PsStore(PS_KEY_BROADCAST_ID, bcast_id, len_in_u16);
        DEBUG_LOG_ALWAYS("UsbDongle_LeaConfigSetLeaBroadcastID Requested-Size: %d, Written-Size: %d",
                         len_in_u16, written_words);

        LeAudioClientBroadcast_BroadcastConfigChanged();

        return TRUE;
    }

    return FALSE;
}

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

/*! \brief Set default UUID filters to use for RSSI pairing (CAS & ASCS) */
static void usbDongle_LeaConfigSetDefaultUuidAdvFilter(void)
{
    uint16 uuid_list[] = { USB_LEA_CAS_UUID,
                           USB_LEA_ASCS_UUID,
                           USB_LEA_TMAS_UUID,
                           USB_LEA_GMCS_UUID,
                           USB_LEA_GTBS_UUID,
                           USB_LEA_VCS_UUID,
                           USB_LEA_CSIS_UUID
                         };

    SinkService_SetUuidAdvFilter(ARRAY_DIM(uuid_list), uuid_list);
}

void UsbDongle_LeaConfigInit(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_LeaConfigInit");

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
    memset(&usb_dongle_lea_config_data, 0, sizeof(usb_dongle_lea_config_data));

    UsbDongle_LeaConfigLoadNameFromPs();
    UsbDongle_LeaConfigLoadCodeFromPs();
    UsbDongle_LeaConfigLoadBcastAdvFromPs();
    UsbDongle_LeaConfigLoadBcastAudioConfigFromPs();
    UsbDongle_LeaConfigLoadBcastIdFromPs();

    LeAudioClient_RegisterConfigInterface(&usb_dongle_lea_config_interfaces);
#endif

    usbDongle_LeaConfigSetDefaultUuidAdvFilter();
}

#endif  /* INCLUDE_SOURCE_APP_LE_AUDIO */
