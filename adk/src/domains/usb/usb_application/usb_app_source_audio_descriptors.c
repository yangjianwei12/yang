/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    usb_application
    \brief      USB Audio Descriptors for the USB Source Audio application
*/

#include "usb_audio.h"
#include "usb_audio_class_10_default_descriptors.h"

/* Bit mask indicating different sample rates supported for speaker/MIC */
#define USB_AUDIO_SAMPLE_RATE_8K                                (1u << 0u)
#define USB_AUDIO_SAMPLE_RATE_11K025                            (1u << 1u)  /* Not supported */
#define USB_AUDIO_SAMPLE_RATE_16K                               (1u << 2u)
#define USB_AUDIO_SAMPLE_RATE_22K05                             (1u << 3u)  /* Not supported */
#define USB_AUDIO_SAMPLE_RATE_24K                               (1u << 4u)  /* Not supported */
#define USB_AUDIO_SAMPLE_RATE_32K                               (1u << 5u)
#define USB_AUDIO_SAMPLE_RATE_44K1                              (1u << 6u)
#define USB_AUDIO_SAMPLE_RATE_48K                               (1u << 7u)
#define USB_AUDIO_SAMPLE_RATE_88K2                              (1u << 8u)  /* Not supported */
#define USB_AUDIO_SAMPLE_RATE_96K                               (1u << 9u)
#define USB_AUDIO_SAMPLE_RATE_176K4                             (1u << 10u) /* Not supported */
#define USB_AUDIO_SAMPLE_RATE_192K                              (1u << 11u) /* Not supported */

#ifndef USB_SOURCE_16BIT_AUDIO_SPEAKER_SAMPLE_RATES
#define USB_SOURCE_16BIT_AUDIO_SPEAKER_SAMPLE_RATES             0xC0        /* Sample rate = 48kHz, 44.1kHz */
#endif

#ifndef USB_SOURCE_24BIT_AUDIO_SPEAKER_SAMPLE_RATES
#define USB_SOURCE_24BIT_AUDIO_SPEAKER_SAMPLE_RATES             0x2C0       /* Sample rate = 96kHz, 48kHz, 44.1kHz */
#endif

#ifndef USB_SOURCE_16BIT_MIC_SAMPLE_RATES
#define USB_SOURCE_16BIT_MIC_SAMPLE_RATES                       0xA5        /* Sample rate = 48kHz, 32kHz, 16kHz, 8kHz */
#endif

/* Supported 16 bit USB speaker sample rates */
typedef enum
{
#if (USB_SOURCE_16BIT_AUDIO_SPEAKER_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_44K1)
    USB_AUDIO_16BIT_SUPPORTED_SPKR_SAMPLE_RATE_44K1,
#endif

#if (USB_SOURCE_16BIT_AUDIO_SPEAKER_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_48K)
    USB_AUDIO_16BIT_SUPPORTED_SPKR_SAMPLE_RATE_48K,
#endif

    USB_AUDIO_16BIT_SUPPORTED_SPKR_SAMPLE_RATE_MAX

} USB_AUDIO_16BIT_SUPPORTED_SPKR_SAMPLE_RATE_T;

/* Supported 24 bit USB speaker sample rates */
typedef enum
{
#if (USB_SOURCE_24BIT_AUDIO_SPEAKER_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_44K1)
    USB_AUDIO_24BIT_SUPPORTED_SPKR_SAMPLE_RATE_44K1,
#endif

#if (USB_SOURCE_24BIT_AUDIO_SPEAKER_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_48K)
    USB_AUDIO_24BIT_SUPPORTED_SPKR_SAMPLE_RATE_48K,
#endif

#if (USB_SOURCE_24BIT_AUDIO_SPEAKER_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_96K)
    USB_AUDIO_24BIT_SUPPORTED_SPKR_SAMPLE_RATE_96K,
#endif

    USB_AUDIO_24BIT_SUPPORTED_SPKR_SAMPLE_RATE_MAX

} USB_AUDIO_24BIT_SUPPORTED_SPKR_SAMPLE_RATE_T;

/* Supported 16 bit MIC sample rates */
typedef enum
{
#if (USB_SOURCE_16BIT_MIC_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_8K)
    USB_AUDIO_16BIT_SUPPORTED_MIC_SAMPLE_RATE_8K,
#endif

#if (USB_SOURCE_16BIT_MIC_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_16K)
    USB_AUDIO_16BIT_SUPPORTED_MIC_SAMPLE_RATE_16K,
#endif

#if (USB_SOURCE_16BIT_MIC_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_32K)
    USB_AUDIO_16BIT_SUPPORTED_MIC_SAMPLE_RATE_32K,
#endif

#if (USB_SOURCE_16BIT_MIC_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_48K)
    USB_AUDIO_16BIT_SUPPORTED_MIC_SAMPLE_RATE_48K,
#endif

    USB_AUDIO_16BIT_SUPPORTED_MIC_SAMPLE_RATE_MAX

} USB_AUDIO_16BIT_SUPPORTED_MIC_SAMPLE_RATE_T;

/* Number of sample rates supported for 24 bit speaker */
#define USB_SOURCE_24BIT_AUDIO_SPKR_SUPPORTED_FREQUENCIES   USB_AUDIO_24BIT_SUPPORTED_SPKR_SAMPLE_RATE_MAX

/* Number of sample rates supported for 16 bit speaker */
#define USB_SOURCE_16BIT_AUDIO_SPKR_SUPPORTED_FREQUENCIES   USB_AUDIO_16BIT_SUPPORTED_SPKR_SAMPLE_RATE_MAX

/* Number of sample rates supported for 16 bit MIC */
#define USB_SOURCE_VOICE_MIC_SUPPORTED_FREQUENCIES          USB_AUDIO_16BIT_SUPPORTED_MIC_SAMPLE_RATE_MAX

#ifdef USE_DUAL_MICROPHONE_FOR_LEA_USB_SOURCE
#define USB_SOURCE_MIC_CHANNELS        USB_AUDIO_CHANNELS_STEREO
#else
#define USB_SOURCE_MIC_CHANNELS        USB_AUDIO_CHANNELS_MONO
#endif
#define USB_SOURCE_SPKR_CHANNELS       USB_AUDIO_CHANNELS_STEREO

#define USB_SOURCE_AUDIO_SAMPLE_SIZE   USB_SAMPLE_SIZE_24_BIT

#if USB_SOURCE_MIC_CHANNELS == USB_AUDIO_CHANNELS_STEREO
#define USB_SOURCE_MIC_CHANNEL_CONFIG   0x03
#elif USB_SOURCE_MIC_CHANNELS == USB_AUDIO_CHANNELS_MONO
#define USB_SOURCE_MIC_CHANNEL_CONFIG   0x01
#else
#error NOT_SUPPORTED
#endif

#if USB_SOURCE_SPKR_CHANNELS == USB_AUDIO_CHANNELS_STEREO
#define USB_SOURCE_SPKR_CHANNEL_CONFIG  0x03
#elif USB_SOURCE_SPKR_CHANNELS == USB_AUDIO_CHANNELS_MONO
#define USB_SOURCE_SPKR_CHANNEL_CONFIG  0x01
#else
#error NOT_SUPPORTED
#endif

static const uint8 control_intf_desc_voice_mic[] =
{
    /* Microphone IT */
    UAC_IT_TERM_DESC_SIZE,              /* bLength */
    UAC_CS_DESC_INTERFACE,              /* bDescriptorType = CS_INTERFACE */
    UAC_AC_DESC_INPUT_TERMINAL,         /* bDescriptorSubType = INPUT_TERMINAL */
    UAC1D_MIC_VOICE_IT,                 /* bTerminalID */
    UAC_TRM_BIDI_HEADSET & 0xFF,        /* wTerminalType = Microphone*/
    UAC_TRM_BIDI_HEADSET >> 8,
    0x00,                               /* bAssocTerminal = none */
    USB_SOURCE_MIC_CHANNELS,            /* bNrChannels */
    USB_SOURCE_MIC_CHANNEL_CONFIG,      /* wChannelConfig  */
    USB_SOURCE_MIC_CHANNEL_CONFIG >> 8, /* wChannelConfig	*/
    0x00,                               /* iChannelName = no string */
    0x00,                               /* iTerminal = same as USB product string */

    /* Microphone Features */
    UAC_FU_DESC_SIZE(USB_SOURCE_MIC_CHANNELS, UAC1D_FU_DESC_CONTROL_SIZE),   /*bLength*/
    UAC_CS_DESC_INTERFACE,              /* bDescriptorType = CS_INTERFACE */
    UAC_AC_DESC_FEATURE_UNIT,           /* bDescriptorSubType = FEATURE_UNIT*/
    UAC1D_MIC_VOICE_FU,                 /* bUnitId*/
    UAC1D_MIC_VOICE_IT,                 /* bSourceId - Microphone IT*/
    UAC1D_FU_DESC_CONTROL_SIZE,         /* bControlSize = 1 bytes per control*/
    UAC_FU_CONTROL_MUTE,                /* bmaControls[0] (Mute on Master Channel)*/
    UAC_FU_CONTROL_UNDEFINED,           /* bmaControls[1] (Left Front)*/
#if USB_SOURCE_MIC_CHANNELS == 2
    UAC_FU_CONTROL_UNDEFINED,           /* bmaControls[2] (Right Front)*/
#endif
    0x00,                               /*iFeature = same as USB product string*/

    /* Microphone OT */
    UAC_OT_TERM_DESC_SIZE,              /* bLength */
    UAC_CS_DESC_INTERFACE,              /* bDescriptorType = CS_INTERFACE */
    UAC_AC_DESC_OUTPUT_TERNINAL,        /* bDescriptorSubType = OUTPUT_TERMINAL */
    UAC1D_MIC_VOICE_OT,                 /* bTerminalID */
    UAC_TRM_USB_STREAMING & 0xFF,       /* wTerminalType = USB streaming */
    UAC_TRM_USB_STREAMING >> 8,
    0x00,                               /* bAssocTerminal = none */
    UAC1D_MIC_VOICE_FU,                 /* bSourceID - Microphone Features */
    0x00,                               /* iTerminal = same as USB product string */
};

#if USB_SOURCE_16BIT_MIC_SAMPLE_RATES != 0
/** Default USB streaming interface descriptors for mic */
static const uint8 streaming_intf_desc_voice_mic[] =
{
    /* Class Specific AS interface descriptor */
    UAC_AS_IF_DESC_SIZE,                /* bLength */
    UAC_CS_DESC_INTERFACE,              /* bDescriptorType = CS_INTERFACE */
    UAC_AS_DESC_GENERAL,                /* bDescriptorSubType = AS_GENERAL */
    UAC1D_MIC_VOICE_OT,                 /* bTerminalLink = Microphone OT */
    0x00,                               /* bDelay */
    UAC_DATA_FORMAT_TYPE_I_PCM & 0xFF,  /* wFormatTag = PCM */
    UAC_DATA_FORMAT_TYPE_I_PCM >> 8,

    /* Type 1 format type descriptor */
    UAC_FORMAT_DESC_SIZE(USB_SOURCE_VOICE_MIC_SUPPORTED_FREQUENCIES), /* bLength */
    UAC_CS_DESC_INTERFACE,              /* bDescriptorType = CS_INTERFACE */
    UAC_AS_DESC_FORMAT_TYPE,            /* bDescriptorSubType = FORMAT_TYPE */
    UAC_AS_DESC_FORMAT_TYPE_I,          /* bFormatType = FORMAT_TYPE_I */
    USB_SOURCE_MIC_CHANNELS,            /* bNumberOfChannels */
    USB_SAMPLE_SIZE_16_BIT,             /* bSubframeSize = 2 bytes */
    USB_SAMPLE_SIZE_16_BIT * 8,         /* bBitsResolution */
    USB_SOURCE_VOICE_MIC_SUPPORTED_FREQUENCIES, /* bSampleFreqType */

#if (USB_SOURCE_16BIT_MIC_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_48K)
    SAMPLE_RATE_48K & 0xFF,             /* tSampleFreq = 48000 */
    (SAMPLE_RATE_48K >> 8 ) & 0xFF,
    (SAMPLE_RATE_48K >> 16) & 0xFF,
#endif

#if (USB_SOURCE_16BIT_MIC_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_32K)
    SAMPLE_RATE_32K & 0xFF,             /* tSampleFreq = 32000 */
    (SAMPLE_RATE_32K >> 8 ) & 0xFF,
    (SAMPLE_RATE_32K >> 16) & 0xFF,
#endif

#if (USB_SOURCE_16BIT_MIC_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_16K)
    SAMPLE_RATE_16K & 0xFF,             /* tSampleFreq = 16000 */
    (SAMPLE_RATE_16K >> 8 ) & 0xFF,
    (SAMPLE_RATE_16K >> 16) & 0xFF,
#endif

#if (USB_SOURCE_16BIT_MIC_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_8K)
    SAMPLE_RATE_8K & 0xff,              /* tSampleFreq = 8000 */
    (SAMPLE_RATE_8K >> 8) & 0xFF,
    (SAMPLE_RATE_8K >> 16) & 0xFF,
#endif

    /* Class specific AS isochronous audio data endpoint descriptor */
    UAC_AS_DATA_EP_DESC_SIZE,           /* bLength */
    UAC_CS_DESC_ENDPOINT,               /* bDescriptorType = CS_ENDPOINT */
    UAC_AS_EP_DESC_GENERAL,             /* bDescriptorSubType = AS_GENERAL */
    UAC_EP_CONTROL_SAMPLING_FREQ,       /* bmAttributes = SamplingFrequency contro */
    0x02,                               /* bLockDelayUnits = Decoded PCM samples */
    0x00, 0x00                          /* wLockDelay */
};
#endif /* USB_SOURCE_16BIT_MIC_SAMPLE_RATES */

static const uac_control_config_t usb_dongle_voice_control_mic_desc = {
    control_intf_desc_voice_mic,
    sizeof(control_intf_desc_voice_mic)
};

static const uac_endpoint_config_t usb_dongle_voice_mic_endpoint = {
    .is_to_host = 1,
    .wMaxPacketSize = 0,
    .bInterval = 1
};

static const uac_streaming_config_t usb_dongle_voice_streaming_mic_desc[] = {
    {
        &usb_dongle_voice_mic_endpoint,
        streaming_intf_desc_voice_mic,
        sizeof(streaming_intf_desc_voice_mic)
    }
};

static const uint8 control_intf_desc_audio_spkr[] =
{
    /* ALT_Speaker IT */
    UAC_IT_TERM_DESC_SIZE,              /* bLength */
    UAC_CS_DESC_INTERFACE,              /* bDescriptorType = CS_INTERFACE */
    UAC_AC_DESC_INPUT_TERMINAL,         /* bDescriptorSubType = INPUT_TERMINAL */
    UAC1D_SPKR_AUDIO_IT,                /* bTerminalID */
    UAC_TRM_USB_STREAMING & 0xFF,       /* wTerminalType = USB streaming */
    UAC_TRM_USB_STREAMING >> 8,
    0x00,                               /* bAssocTerminal = none */
    USB_SOURCE_SPKR_CHANNELS,           /* bNrChannels */
    USB_SOURCE_SPKR_CHANNEL_CONFIG,     /* wChannelConfig	*/
    USB_SOURCE_SPKR_CHANNEL_CONFIG >> 8,
    0x00,                               /* iChannelName = no string */
    0x00,                               /* iTerminal = same as USB product string */

    /* ALT_Speaker Features */
    UAC_FU_DESC_SIZE(USB_SOURCE_SPKR_CHANNELS, UAC1D_FU_DESC_CONTROL_SIZE),   /*bLength*/
    UAC_CS_DESC_INTERFACE,              /* bDescriptorType = CS_INTERFACE */
    UAC_AC_DESC_FEATURE_UNIT,           /* bDescriptorSubType = FEATURE_UNIT*/
    UAC1D_SPKR_AUDIO_FU,                /* bUnitId*/
    UAC1D_SPKR_AUDIO_IT,                /* bSourceId - Speaker IT*/
    UAC1D_FU_DESC_CONTROL_SIZE,         /* bControlSize = 1 bytes per control*/
    UAC_FU_CONTROL_MUTE | UAC_FU_CONTROL_VOLUME, /* bmaControls[0] (Mute & Vol on Master Channel)*/
    UAC_FU_CONTROL_UNDEFINED,           /* bmaControls[1] (Left Front)*/
#if USB_SOURCE_SPKR_CHANNELS == 2
    UAC_FU_CONTROL_UNDEFINED,           /* bmaControls[2] (Right Front)*/
#endif
    0x00,                               /* iFeature = same as USB product string*/

    /* ALT_Speaker OT */
    UAC_OT_TERM_DESC_SIZE,              /* bLength */
    UAC_CS_DESC_INTERFACE,              /* bDescriptorType = CS_INTERFACE */
    UAC_AC_DESC_OUTPUT_TERNINAL,        /* bDescriptorSubType = OUTPUT_TERMINAL */
    UAC1D_SPKR_AUDIO_OT,                /* bTerminalID */
    UAC_TRM_BIDI_HEADSET & 0xFF,        /* wTerminalType = Speaker*/
    UAC_TRM_BIDI_HEADSET >> 8,
    0x00,                               /* bAssocTerminal = none */
    UAC1D_SPKR_AUDIO_FU,                /* bSourceID - Speaker Features*/
    0x00                                /* iTerminal = same as USB product string */
};

#if USB_SOURCE_24BIT_AUDIO_SPEAKER_SAMPLE_RATES != 0
/** USB streaming interface descriptors for alt speaker with 24Bit */
static const uint8 streaming_intf_desc_audio_spkr_24b[] =
{
    /* Class Specific AS interface descriptor */
    UAC_AS_IF_DESC_SIZE,                /* bLength */
    UAC_CS_DESC_INTERFACE,              /* bDescriptorType = CS_INTERFACE */
    UAC_AS_DESC_GENERAL,                /* bDescriptorSubType = AS_GENERAL */
    UAC1D_SPKR_AUDIO_IT,                /* bTerminalLink = Speaker IT */
    0x00,                               /* bDelay */
    UAC_DATA_FORMAT_TYPE_I_PCM & 0xFF,  /* wFormatTag = PCM */
    UAC_DATA_FORMAT_TYPE_I_PCM >> 8,

    /* Type 1 format type descriptor */
    UAC_FORMAT_DESC_SIZE(USB_SOURCE_24BIT_AUDIO_SPKR_SUPPORTED_FREQUENCIES),  /* bLength 8+((number of sampling frequencies)*3) */
    UAC_CS_DESC_INTERFACE,              /* bDescriptorType = CS_INTERFACE */
    UAC_AS_DESC_FORMAT_TYPE,            /* bDescriptorSubType = FORMAT_TYPE */
    UAC_AS_DESC_FORMAT_TYPE_I,          /* bFormatType = FORMAT_TYPE_I */
    UAC1D_AUDIO_SPKR_CHANNELS,          /* bNumberOfChannels */
    USB_SAMPLE_SIZE_24_BIT,             /* bSubframeSize = 3 bytes */
    USB_SAMPLE_SIZE_24_BIT * 8,         /* bBitsResolution */
    USB_SOURCE_24BIT_AUDIO_SPKR_SUPPORTED_FREQUENCIES,/* bSampleFreqType = 3 discrete sampling frequencies */

#if (USB_SOURCE_24BIT_AUDIO_SPEAKER_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_96K)
    SAMPLE_RATE_96K & 0xFF,             /* tSampleFreq = 96000*/
    (SAMPLE_RATE_96K >> 8) & 0xFF,
    (SAMPLE_RATE_96K >> 16) & 0xFF,
#endif

#if (USB_SOURCE_24BIT_AUDIO_SPEAKER_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_48K)
    SAMPLE_RATE_48K & 0xFF,             /* tSampleFreq = 48000*/
    (SAMPLE_RATE_48K >> 8) & 0xFF,
    (SAMPLE_RATE_48K >> 16) & 0xFF,
#endif

#if (USB_SOURCE_24BIT_AUDIO_SPEAKER_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_44K1)
    SAMPLE_RATE_44K1 & 0xFF,            /* tSampleFreq = 44100*/
    (SAMPLE_RATE_44K1 >> 8) & 0xFF,
    (SAMPLE_RATE_44K1 >> 16) & 0xFF,
#endif

    /* Class specific AS isochronous audio data endpoint descriptor */
    UAC_AS_DATA_EP_DESC_SIZE,           /* bLength */
    UAC_CS_DESC_ENDPOINT,               /* bDescriptorType = CS_ENDPOINT */
    UAC_AS_EP_DESC_GENERAL,             /* bDescriptorSubType = AS_GENERAL */
    UAC_EP_CONTROL_SAMPLING_FREQ,       /* bmAttributes = SamplingFrequency control */
    0x02,                               /* bLockDelayUnits = Decoded PCM samples */
    0x00, 0x00                          /* wLockDelay */
};
#endif /* USB_SOURCE_24BIT_AUDIO_SPEAKER_SAMPLE_RATES */

/* Audio Speaker interface descriptors */
static const uac_control_config_t usb_source_music_control_spkr_desc = {
    control_intf_desc_audio_spkr,
    sizeof(control_intf_desc_audio_spkr)
};

#if USB_SOURCE_16BIT_AUDIO_SPEAKER_SAMPLE_RATES != 0
/** USB streaming interface descriptors for alt speaker with 16Bit */
static const uint8 streaming_intf_desc_audio_spkr_16b[] =
{
    /* Class Specific AS interface descriptor */
    UAC_AS_IF_DESC_SIZE,                /* bLength */
    UAC_CS_DESC_INTERFACE,              /* bDescriptorType = CS_INTERFACE */
    UAC_AS_DESC_GENERAL,                /* bDescriptorSubType = AS_GENERAL */
    UAC1D_SPKR_AUDIO_IT,                /* bTerminalLink = Speaker IT */
    0x00,                               /* bDelay */
    UAC_DATA_FORMAT_TYPE_I_PCM & 0xFF,  /* wFormatTag = PCM */
    UAC_DATA_FORMAT_TYPE_I_PCM >> 8,

    /* Type 1 format type descriptor */
    UAC_FORMAT_DESC_SIZE(USB_SOURCE_16BIT_AUDIO_SPKR_SUPPORTED_FREQUENCIES),  /* bLength 8+((number of sampling frequencies)*3) */
    UAC_CS_DESC_INTERFACE,              /* bDescriptorType = CS_INTERFACE */
    UAC_AS_DESC_FORMAT_TYPE,            /* bDescriptorSubType = FORMAT_TYPE */
    UAC_AS_DESC_FORMAT_TYPE_I,          /* bFormatType = FORMAT_TYPE_I */
    USB_SOURCE_SPKR_CHANNELS,           /* bNumberOfChannels */
    USB_SAMPLE_SIZE_16_BIT,             /* bSubframeSize = 2 bytes */
    USB_SAMPLE_SIZE_16_BIT * 8,         /* bBitsResolution */
    USB_SOURCE_16BIT_AUDIO_SPKR_SUPPORTED_FREQUENCIES,/* bSampleFreqType = 2 discrete sampling frequencies */

#if (USB_SOURCE_16BIT_AUDIO_SPEAKER_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_48K)
    SAMPLE_RATE_48K & 0xFF,             /* tSampleFreq = 48000*/
    (SAMPLE_RATE_48K >> 8) & 0xFF,
    (SAMPLE_RATE_48K >> 16) & 0xFF,
#endif

#if (USB_SOURCE_16BIT_AUDIO_SPEAKER_SAMPLE_RATES & USB_AUDIO_SAMPLE_RATE_44K1)
    SAMPLE_RATE_44K1 & 0xFF,            /* tSampleFreq = 44100*/
    (SAMPLE_RATE_44K1 >> 8) & 0xFF,
    (SAMPLE_RATE_44K1 >> 16) & 0xFF,
#endif

    /* Class specific AS isochronous audio data endpoint descriptor */
    UAC_AS_DATA_EP_DESC_SIZE,           /* bLength */
    UAC_CS_DESC_ENDPOINT,               /* bDescriptorType = CS_ENDPOINT */
    UAC_AS_EP_DESC_GENERAL,             /* bDescriptorSubType = AS_GENERAL */
    UAC_EP_CONTROL_SAMPLING_FREQ,       /* bmAttributes = SamplingFrequency control */
    0x02,                               /* bLockDelayUnits = Decoded PCM samples */
    0x00, 0x00                          /* wLockDelay */
};
#endif /* USB_SOURCE_16BIT_AUDIO_SPEAKER_SAMPLE_RATES */

static const uac_endpoint_config_t usb_source_music_spkr_endpoint = {
    .is_to_host = 0,
    .wMaxPacketSize = 0,
    .bInterval = 1
};

static const uac_streaming_config_t usb_source_music_streaming_spkr_desc[] = {
#if USB_SOURCE_24BIT_AUDIO_SPEAKER_SAMPLE_RATES != 0
    {
        &usb_source_music_spkr_endpoint,
        streaming_intf_desc_audio_spkr_24b,
        sizeof(streaming_intf_desc_audio_spkr_24b)
    },
#endif /* USB_SOURCE_24BIT_AUDIO_SPEAKER_SAMPLE_RATES */
#if USB_SOURCE_16BIT_AUDIO_SPEAKER_SAMPLE_RATES != 0
    {
        &usb_source_music_spkr_endpoint,
        streaming_intf_desc_audio_spkr_16b,
        sizeof(streaming_intf_desc_audio_spkr_16b)
    }
#endif /* USB_SOURCE_16BIT_AUDIO_SPEAKER_SAMPLE_RATES */
};

static const usb_audio_interface_config_t usb_source_music_voice_intf_list[] =
{
    {
        .type = USB_AUDIO_DEVICE_TYPE_VOICE_MIC,
        .control_desc =   &usb_dongle_voice_control_mic_desc,
        .streaming_config = usb_dongle_voice_streaming_mic_desc,
        .alt_settings_count = ARRAY_DIM(usb_dongle_voice_streaming_mic_desc)
    },
    {
        .type = USB_AUDIO_DEVICE_TYPE_AUDIO_SPEAKER,
        .control_desc =   &usb_source_music_control_spkr_desc,
        .streaming_config = usb_source_music_streaming_spkr_desc,
        .alt_settings_count = ARRAY_DIM(usb_source_music_streaming_spkr_desc)
    }
};

const usb_audio_interface_config_list_t usb_source_music_voice_interfaces =
{
    .intf = usb_source_music_voice_intf_list,
    .num_interfaces = ARRAY_DIM(usb_source_music_voice_intf_list)
};
