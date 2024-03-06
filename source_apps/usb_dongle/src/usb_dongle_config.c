/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Application configuration implementation
*/

#include "usb_dongle_config.h"
#include "profile_manager.h"
#include "ps_key_map.h"
#include "logging.h"

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) && defined(INCLUDE_SOURCE_APP_LE_AUDIO)

/*! \brief Transport mode to use for different audio modes */
#define usbDongle_GetDefaultTransportForHighQualityMode()   (SINK_SERVICE_TRANSPORT_BREDR)
#define usbDongle_GetDefaultTransportForGamingMode()        (SINK_SERVICE_TRANSPORT_LE)
#define usbDongle_GetDefaultTransportForBroadcastMode()     (SINK_SERVICE_TRANSPORT_LE)
#define usbDongle_IsInvalidTransportMode(mode)              (mode == usb_dongle_transport_mode_unknown || \
                                                             mode >= usb_dongle_transport_mode_max)
#define usbDongle_GetDefaultTransportMode()                 (usb_dongle_transport_mode_dual)
#elif defined(INCLUDE_SOURCE_APP_BREDR_AUDIO)
#define usbDongle_GetDefaultTransportForHighQualityMode()   (SINK_SERVICE_TRANSPORT_BREDR)
#define usbDongle_GetDefaultTransportForGamingMode()        (SINK_SERVICE_TRANSPORT_BREDR)
#define usbDongle_GetDefaultTransportForBroadcastMode()     (SINK_SERVICE_TRANSPORT_BREDR)
#define usbDongle_IsInvalidTransportMode(mode)              (mode != usb_dongle_transport_mode_bredr)
#define usbDongle_GetDefaultTransportMode()                 (usb_dongle_transport_mode_bredr)
#else
#define usbDongle_GetDefaultTransportForHighQualityMode()   (SINK_SERVICE_TRANSPORT_LE)
#define usbDongle_GetDefaultTransportForGamingMode()        (SINK_SERVICE_TRANSPORT_LE)
#define usbDongle_GetDefaultTransportForBroadcastMode()     (SINK_SERVICE_TRANSPORT_LE)
#define usbDongle_IsInvalidTransportMode(mode)              (mode != usb_dongle_transport_mode_le_audio)
#define usbDongle_GetDefaultTransportMode()                 (usb_dongle_transport_mode_le_audio)
#endif

const rssi_pairing_parameters_t usb_dongle_config_rssi_params =
{
    .inquiry_filter = APP_CONFIG_RSSI_PAIRING_INQUIRY_PARAM_SET,
    .rssi_gap = APP_CONFIG_RSSI_PAIRING_MIN_RSSI_GAP,
    .rssi_threshold = APP_CONFIG_RSSI_PAIRING_RSSI_THRESHOLD,
    .inquiry_count = APP_CONFIG_RSSI_PAIRING_INQUIRY_COUNT
};

const profile_t usb_dongle_config_profile_list [] =
{
    profile_manager_hfp_profile,
    profile_manager_a2dp_profile,
    profile_manager_avrcp_profile,
    profile_manager_max_number_of_profiles,
};

const sink_service_config_t sink_service_config =
{
    .supported_profile_mask = (DEVICE_PROFILE_AVRCP|DEVICE_PROFILE_A2DP|DEVICE_PROFILE_HFP),
    .profile_list = usb_dongle_config_profile_list,
    .profile_list_length = ARRAY_DIM(usb_dongle_config_profile_list),
    .page_timeout = APP_CONFIG_PAGE_TIMEOUT_S,
    .rssi_pairing_params = &usb_dongle_config_rssi_params,
    .max_bredr_connections = SINK_SERVICE_CONFIG_BREDR_CONNECTIONS_MAX,
#ifdef ENABLE_LE_SINK_SERVICE
    .max_le_connections = SINK_SERVICE_CONFIG_LE_CONNECTIONS_MAX,
#endif
};

/*! \brief Lookup table which indicates the transport to use (LE/BREDR) for each audio mode.
           Applicable only for dual mode devices */
const sink_service_transport_t usb_dongle_config_mode_vs_transport_table[] = {
    [usb_dongle_audio_mode_high_quality] = usbDongle_GetDefaultTransportForHighQualityMode(),
    [usb_dongle_audio_mode_gaming] = usbDongle_GetDefaultTransportForGamingMode(),
#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
    [usb_dongle_audio_mode_broadcast] = usbDongle_GetDefaultTransportForBroadcastMode(), /* Broadcast only supports LE */
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
};

usb_dongle_config_t usb_dongle_config;

static uint8 usbDongleConfig_SaveMode(bool save_to_ps, usb_dongle_audio_mode_t audio_mode, usb_dongle_transport_mode_t transport_mode)
{
    uint16 mode_status = USB_DONGLE_MODE_CHANGE_STATUS_NO_CHANGE;

    if (audio_mode == usb_dongle_audio_mode_invalid)
    {
        audio_mode = usbDongleConfig_GetAudioMode();
    }
    else if (audio_mode >= usb_dongle_audio_mode_max)
    {
        return mode_status;
    }

    if (transport_mode == usb_dongle_transport_mode_invalid)
    {
        transport_mode = usbDongleConfig_GetTransportMode();
    }
    else if (usbDongle_IsInvalidTransportMode(transport_mode))
    {
        return mode_status;
    }

    if (usb_dongle_config.audio_mode != audio_mode)
    {
        mode_status |= USB_DONGLE_MODE_CHANGE_STATUS_AUDIO_MODE_UPDATED;

        /* Update the new audio mode */
        usb_dongle_config.new_audio_mode = audio_mode;
    }

    if (usb_dongle_config.transport_mode != transport_mode)
    {
        mode_status |= USB_DONGLE_MODE_CHANGE_STATUS_TRANSPORT_MODE_UPDATED;

        /* Update the new transport mode */
        usb_dongle_config.new_transport_mode = transport_mode;
    }

    if (mode_status && save_to_ps)
    {
#ifndef DISABLE_DONGLE_MODE_FROM_PSKEY
        uint16 mode = ((uint16) transport_mode << 8u) | (uint16) audio_mode;

        PsStore(PS_KEY_USB_DONGLE_MODE, &mode, sizeof(mode) / sizeof(uint16));
#endif

        usb_dongle_config.audio_mode = audio_mode;
        usb_dongle_config.transport_mode = transport_mode;
    }

    DEBUG_LOG_ALWAYS("usbDongleConfig_SaveMode save_to_ps %d, mode_status: %d, audio_mode: enum:usb_dongle_audio_mode_t:%d, transport_mode: enum:usb_dongle_transport_mode_t:%d",
                     save_to_ps, mode_status, audio_mode, transport_mode);

    return mode_status;
}

void UsbDongleConfigInit(void)
{
    uint16 mode = 0xFFFF;

#ifndef DISABLE_DONGLE_MODE_FROM_PSKEY
    uint16 words_read = PsRetrieve(PS_KEY_USB_DONGLE_MODE, &mode, sizeof(mode));

    if (words_read != (sizeof(mode) / sizeof(uint16)))
    {
        mode = 0xFFFF;
    }
#endif

    usb_dongle_config.audio_mode = mode & 0xFFu;
    usb_dongle_config.transport_mode = (mode >> 8u) & 0xFFu;

    if (usb_dongle_config.audio_mode >= usb_dongle_audio_mode_max)
    {
#if !defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) && defined(INCLUDE_SOURCE_APP_LE_AUDIO)
        usb_dongle_config.audio_mode = usb_dongle_audio_mode_gaming;
#else
        usb_dongle_config.audio_mode = usb_dongle_audio_mode_high_quality;
#endif
    }

    if (usbDongle_IsInvalidTransportMode(usb_dongle_config.transport_mode))
    {
        usb_dongle_config.transport_mode = usbDongle_GetDefaultTransportMode();
    }

    DEBUG_LOG_ALWAYS("UsbDongleConfigInit pskey-mode: 0x%04x, selected audio_mode: enum:usb_dongle_audio_mode_t:%d, transport_mode: enum:usb_dongle_transport_mode_t:%d",
                     mode, usb_dongle_config.audio_mode, usb_dongle_config.transport_mode);
    usb_dongle_config.new_audio_mode = usb_dongle_config.audio_mode;
    usb_dongle_config.new_transport_mode = usb_dongle_config.transport_mode;
}

uint8 UsbDongleConfig_SaveMode(usb_dongle_audio_mode_t audio_mode, usb_dongle_transport_mode_t transport_mode)
{
    return usbDongleConfig_SaveMode(TRUE, audio_mode, transport_mode);
}

uint8 UsbDongleConfig_SetNewMode(usb_dongle_audio_mode_t audio_mode, usb_dongle_transport_mode_t transport_mode)
{
    return usbDongleConfig_SaveMode(FALSE, audio_mode, transport_mode);
}

