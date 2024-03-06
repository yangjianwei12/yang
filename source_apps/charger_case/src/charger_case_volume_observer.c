/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Volume observer that syncs between AVRCP and USB
*/


#include "charger_case_volume_observer.h"
#include "audio_sources.h"
#include <logging.h>
#include <volume_utils.h>
#include <volume_service.h>
#include <volume_messages.h>
#include <avrcp_profile.h>

static void chargerCase_NotifyVolumeChange(audio_source_t source, event_origin_t origin, volume_t volume);

typedef struct {
    avInstanceTaskData *av_instance;
} charger_case_volume_observer_data;

static charger_case_volume_observer_data volume_observer_data;

const volume_config_t avrcp_config = {
    .range = {
        .max = 127,
        .min = 0
    },
    .number_of_steps = 128
};

const volume_config_t usb_config = {
    .range = {
        .max = 15,
        .min = 0
    },
    .number_of_steps = 16
};

static const audio_source_observer_interface_t charger_case_observer_interface =
{
    .OnVolumeChange = chargerCase_NotifyVolumeChange,
};

static void chargerCase_UpdateAvrcpVolume(volume_t volume)
{
    DEBUG_LOG("ChargerCase_UpdateAvrcpVolume");

    if (volume_observer_data.av_instance)
    {
        DEBUG_LOG("ChargerCase_UpdateAvrcpVolume USB VOLUME %d AVRCP VOLUME %d", volume.value, VolumeUtils_ConvertToVolumeConfig(volume, avrcp_config));

        appAvrcpSetAbsoluteVolumeRequest(volume_observer_data.av_instance,
                                         VolumeUtils_ConvertToVolumeConfig(volume, avrcp_config));
    }
}

static void chargerCase_UpdateUsbVolume(volume_t avrcp_volume)
{
    DEBUG_LOG("ChargerCase_UpdateUsbVolume volume.value %d", avrcp_volume.value);
    volume_t usb_volume= AudioSources_GetVolume(audio_source_usb);
    int16 new_usb_volume = VolumeUtils_ConvertToVolumeConfig(avrcp_volume, usb_config);
    if (new_usb_volume == usb_volume.value)
    {
        /* No change required */
        return;
    }
    /* USB does not support absolute volume so have to increment the volume in steps */
    if (new_usb_volume > usb_volume.value)
    {
        Volume_SendAudioSourceVolumeIncrementRequest(audio_source_usb, event_origin_local);
    }
    else
    {
        Volume_SendAudioSourceVolumeDecrementRequest(audio_source_usb, event_origin_local);
    }
}

static void chargerCase_NotifyVolumeChange(audio_source_t source, event_origin_t origin, volume_t volume)
{
    DEBUG_LOG("ChargerCase_NotifyVolumeChange source %d origin %d", source, origin);

    switch(source)
    {
        case audio_source_usb:
            if (origin == event_origin_external)
            {
                chargerCase_UpdateAvrcpVolume(volume);
            }
            break;
        case audio_source_a2dp_1:
            if (origin == event_origin_external)
            {
                chargerCase_UpdateUsbVolume(volume);
            }
            break;
        default:
            DEBUG_LOG("ChargerCase_NotifyVolumeChange Invalid source");
    }
}

static void chargerCase_SynchroniseVolumes(void)
{
    /* Easier to sync to USB as it provides absolute value that can be set via AVRCP */
    volume_t volume = AudioSources_GetVolume(audio_source_usb);
    chargerCase_UpdateAvrcpVolume(volume);
}

void ChargerCase_VolumeObserverInit(void)
{
    DEBUG_LOG_FN_ENTRY("ChargerCase_VolumeObserverInit");
    AudioSources_RegisterObserver(audio_source_usb, &charger_case_observer_interface);
    AudioSources_RegisterObserver(audio_source_a2dp_1, &charger_case_observer_interface);
    volume_observer_data.av_instance = NULL;
}

void ChargerCase_OnAvrcpConnection(avInstanceTaskData *av)
{
    DEBUG_LOG_FN_ENTRY("ChargerCase_OnAvrcpConnection");
    /* Only one connection supported */
    if (!volume_observer_data.av_instance)
    {
        volume_observer_data.av_instance = av;
        chargerCase_SynchroniseVolumes();
    }
}

void ChargerCase_OnAvrcpDisconnection(void)
{
    DEBUG_LOG_FN_ENTRY("ChargerCase_OnAvrcpDisconnection");
    if (volume_observer_data.av_instance)
    {
        volume_observer_data.av_instance = NULL;
    }
}

