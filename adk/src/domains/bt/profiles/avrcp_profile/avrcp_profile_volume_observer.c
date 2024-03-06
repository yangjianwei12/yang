/*!
\copyright  Copyright (c) 2019 - 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   avrcp_profile
\brief      The audio source observer interface implementation provided by AVRCP
*/

#include "avrcp_profile_volume_observer.h"

#include "audio_sources.h"
#include "av.h"
#include "av_instance.h"
#include "bt_device.h"

#include <logging.h>
#include <panic.h>
#ifdef USE_SYNERGY
#include <avrcp.h>
#endif /* USE_SYNERGY */

static void avrcpProfile_NotifyAbsoluteVolume(audio_source_t source, event_origin_t origin, volume_t volume);

static const audio_source_observer_interface_t avrcp_observer_interface =
{
    .OnVolumeChange = avrcpProfile_NotifyAbsoluteVolume,
};

static bool avrcpProfile_IsAudioSourceValidForThisObserver(audio_source_t source)
{
    return (source == audio_source_a2dp_1) || (source == audio_source_a2dp_2);
}

static void avrcpProfile_NotifyVolumeToPeer(int volume_value)
{
    bdaddr peer_addr;
    if(appDeviceGetPeerBdAddr(&peer_addr))
    {
        avInstanceTaskData* inst = appAvInstanceFindFromBdAddr(&peer_addr);
        if(inst && appAvrcpIsConnected(inst))
        {
            if(appDeviceIsHandsetAvrcpConnected())
            {
                DEBUG_LOG("notifyVolumeToPeer, absolute volume request %u to slave %p", volume_value, inst);
#ifdef USE_SYNERGY
                AvrcpCtSetVolumeReqSend(&(AvGetTaskData()->task), inst->avrcp.connectionId, volume_value);
#else
                AvrcpSetAbsoluteVolumeRequest(inst->avrcp.avrcp, volume_value);
#endif
            }
            else
            {
                DEBUG_LOG("notifyVolumeToPeer, notify volume %u to master %p", volume_value, inst);
                appAvAvrcpVolumeNotification(inst, volume_value);
            }
        }
    }
}

static void avrcpProfile_NotifyVolumeToHandset(audio_source_t source, int volume_value)
{
    avInstanceTaskData* inst = Av_GetInstanceForHandsetSource(source);

    if(inst && appAvrcpIsConnected(inst))
    {
        DEBUG_LOG("notifyVolumeToHandset, notify volume %u to handset %p", volume_value, inst);
        appAvAvrcpVolumeNotification(inst, volume_value);
    }
}

static void avrcpProfile_NotifyAbsoluteVolume(audio_source_t source, event_origin_t origin, volume_t volume)
{
    DEBUG_LOG("AvrcpProfile_notifyAbsoluteVolume, source %u, origin %u, volume %u", source, origin, volume.value);
    if(avrcpProfile_IsAudioSourceValidForThisObserver(source))
    {
        if(origin != event_origin_peer)
        {
            avrcpProfile_NotifyVolumeToPeer(volume.value);
        }
        if(origin != event_origin_external)
        {
            avrcpProfile_NotifyVolumeToHandset(source, volume.value);
        }
    }
    else
    {
        Panic();
    }
}

const audio_source_observer_interface_t * AvrcpProfile_GetObserverInterface(void)
{
    return &avrcp_observer_interface;
}



