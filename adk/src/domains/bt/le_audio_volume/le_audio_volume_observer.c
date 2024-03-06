/*!
    \copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_audio_volume
    \brief      Audio and voice source volume observers used to update the VCP volume.
*/

#include "le_audio_volume_private.h"
#include "le_audio_volume_observer.h"
#include "volume_renderer_role.h"
#include "audio_sources.h"
#include "bt_device.h"
#include "voice_sources.h"
#include "volume_utils.h"

#include <logging.h>

static void leAudioVolume_NotifyVcpAudioVolume(audio_source_t source, event_origin_t origin, volume_t volume);
static void leAudioVolume_NotifyVcpVoiceVolume(voice_source_t source, event_origin_t origin, volume_t volume);
static void leAudioVolume_NotifyVcpAudioMute(audio_source_t source, event_origin_t origin, bool mute_state);
static void leAudioVolume_NotifyVcpVoiceMute(voice_source_t source, event_origin_t origin, bool mute_state);

static const audio_source_observer_interface_t lea_audio_source_observer_interface =
{
    .OnVolumeChange = leAudioVolume_NotifyVcpAudioVolume,
    .OnMuteChange = leAudioVolume_NotifyVcpAudioMute,
};

static const voice_source_observer_interface_t lea_voice_source_observer_interface =
{
    .OnVolumeChange = leAudioVolume_NotifyVcpVoiceVolume,
    .OnMuteChange = leAudioVolume_NotifyVcpVoiceMute,
};

static void leAudioVolume_NotifyVcpVolume(volume_t volume)
{
    bool changed = FALSE;

    /* The notified volume may be in a different range to the VCP volume.
       Convert it to the VCP range before updating the VCS server. */
    volume_t vcp_volume = LE_AUDIO_VOLUME(0);
    vcp_volume.value = VolumeUtils_ConvertToVolumeConfig(volume, vcp_volume.config);

    volume_t current_vcp_volume = LE_AUDIO_VOLUME(VolumeRenderer_GetVolume());
    int16 converted_vcp_volume = VolumeUtils_ConvertToVolumeConfig(current_vcp_volume, volume.config);

    if (volume.value != converted_vcp_volume)
    {
        changed = VolumeRenderer_SetVolume(vcp_volume.value);
    }

    DEBUG_LOG("leAudioVolume_NotifyVcpVolume volume %d/%d vs %d changed %d",
              volume.value, volume.config.range.max,
              current_vcp_volume.value, changed);
}

static bool leAudioVolume_IsOtherMusicSourceAlsoLeDevice(audio_source_t source)
{
    bool le_device_match = FALSE;
    device_t* le_devices = NULL;
    unsigned number_le_handsets = BtDevice_GetConnectedLeHandsets(&le_devices);
    device_t audio_device = AudioSources_GetAudioSourceDevice(source);

    DEBUG_LOG("leAudioVolume_IsOtherMusicSourceAlsoLeDevice number_le_handsets %d",
              number_le_handsets);

    for (unsigned i=0; i<number_le_handsets; i++)
    {
        DEBUG_LOG("leAudioVolume_IsOtherMusicSourceAlsoLeDevice le_device %p audio_device %p",
                  le_devices[i], audio_device);
        if (le_devices[i] == audio_device)
        {
            le_device_match = TRUE;
            DEBUG_LOG("leAudioVolume_IsOtherMusicSourceAlsoLeDevice device match");
        }
    }

    free(le_devices);

    return le_device_match;
}

static void leAudioVolume_NotifyVcpAudioVolume(audio_source_t source, event_origin_t origin, volume_t volume)
{
    UNUSED(origin);

    DEBUG_LOG("leAudioVolume_NotifyVcpAudioVolume source enum:audio_source_t:%u origin enum:event_origin_t:%u volume %u",
              source, origin, volume.value);

    if (   LeAudioVolume_IsValidLeMusicSource(source)
        || (LeAudioVolume_IsValidOtherMusicSource(source) && leAudioVolume_IsOtherMusicSourceAlsoLeDevice(source)))
    {
        leAudioVolume_NotifyVcpVolume(volume);
    }
}

static void leAudioVolume_NotifyVcpVoiceVolume(voice_source_t source, event_origin_t origin, volume_t volume)
{
    UNUSED(origin);

    DEBUG_LOG("leAudioVolume_NotifyVcpVoiceVolume source enum:audio_source_t:%u origin enum:event_origin_t:%u volume %u",
              source, origin, volume.value);

    if (LeAudioVolume_IsValidLeVoiceSource(source))
    {
        leAudioVolume_NotifyVcpVolume(volume);
    }
}

static void leAudioVolume_NotifyVcpMute(event_origin_t origin, bool mute_state)
{
    if (origin != event_origin_external)
    {
        bool changed = VolumeRenderer_SetMute(mute_state);
        UNUSED(changed);

        DEBUG_LOG("leAudioVolume_NotifyVcpMute mute_state %d changed %d",
                        mute_state, changed);
    }
}

static void leAudioVolume_NotifyVcpAudioMute(audio_source_t source, event_origin_t origin, bool mute_state)
{
    if (   LeAudioVolume_IsValidLeMusicSource(source)
        || (LeAudioVolume_IsValidOtherMusicSource(source) && leAudioVolume_IsOtherMusicSourceAlsoLeDevice(source)))
    {
        leAudioVolume_NotifyVcpMute(origin, mute_state);
    }
}

static void leAudioVolume_NotifyVcpVoiceMute(voice_source_t source, event_origin_t origin, bool mute_state)
{
    if (LeAudioVolume_IsValidLeVoiceSource(source))
    {
        leAudioVolume_NotifyVcpMute(origin, mute_state);
    }
}

static inline const audio_source_observer_interface_t * LeAudioVolume_GetAudioSourceObserverInterface(void)
{
    return &lea_audio_source_observer_interface;
}

static inline const voice_source_observer_interface_t * LeAudioVolume_GetVoiceSourceObserverInterface(void)
{
    return &lea_voice_source_observer_interface;
}

void LeAudioVolume_RegisterVolumeObservers(void)
{
#ifdef INCLUDE_LE_AUDIO_BROADCAST
    AudioSources_RegisterObserver(audio_source_le_audio_broadcast, LeAudioVolume_GetAudioSourceObserverInterface());
#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST
    AudioSources_RegisterObserver(audio_source_le_audio_unicast_1, LeAudioVolume_GetAudioSourceObserverInterface());
    VoiceSources_RegisterObserver(voice_source_le_audio_unicast_1, LeAudioVolume_GetVoiceSourceObserverInterface());
#endif

#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
    AudioSources_RegisterObserver(audio_source_a2dp_1, LeAudioVolume_GetAudioSourceObserverInterface());
    AudioSources_RegisterObserver(audio_source_a2dp_2, LeAudioVolume_GetAudioSourceObserverInterface());
#endif


}
