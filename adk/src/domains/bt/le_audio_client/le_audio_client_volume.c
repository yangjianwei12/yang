/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief   LE Audio and voice source volume interfaces
*/

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
#include "audio_sources.h"
#include "voice_sources.h"

#include "le_audio_client_volume.h"
#include "le_audio_client_context.h"
#include <device_properties.h>

#include <logging.h>

/* Default LE Audio/Voice volume */
#define LE_AUDIO_CLIENT_VOLUME_DEFAULT     130

/* Minimum volume level for LE audio */
#define LE_AUDIO_CLIENT_VOLUME_MIN         0

/* Maximum volume level for LE audio */
#define LE_AUDIO_CLIENT_VOLUME_MAX         255

/* Number of steps between minimum and maximum volume */
#define LE_AUDIO_CLIENT_VOLUME_STEPS       256

/* Config values for a LE audio volume_t */
#define LE_AUDIO_CLIENT_VOLUME_CONFIG      { .range = { .min = LE_AUDIO_CLIENT_VOLUME_MIN, .max = LE_AUDIO_CLIENT_VOLUME_MAX }, .number_of_steps = LE_AUDIO_CLIENT_VOLUME_STEPS }

/* Macro to initialise a volume_t struct to the given raw volume value */
#define LE_AUDIO_CLIENT_VOLUME(step)       { .config = LE_AUDIO_CLIENT_VOLUME_CONFIG, .value = step }

static volume_t leAudioClientVolume_GetAudioVolume(audio_source_t audio_source);
static void leAudioClientVolume_SetAudioVolume(audio_source_t audio_source, volume_t volume);
static volume_t leAudioClientVolume_GetVoiceVolume(voice_source_t voice_source);
static void leAudioClientVolume_SetVoiceVolume(voice_source_t voice_source, volume_t volume);

static const audio_source_volume_interface_t le_audio_client_volume_interface =
{
    .GetVolume = leAudioClientVolume_GetAudioVolume,
    .SetVolume = leAudioClientVolume_SetAudioVolume,
    .GetMuteState = NULL,
    .SetMuteState = NULL,
};

static const voice_source_volume_interface_t le_audio_client_voice_volume_interface =
{
    .GetVolume = leAudioClientVolume_GetVoiceVolume,
    .SetVolume = leAudioClientVolume_SetVoiceVolume,
    .GetMuteState = NULL,
    .SetMuteState = NULL,
};

static void leAudioClientVolume_SetAudioVolume(audio_source_t audio_source, volume_t volume)
{
    device_t device;
    generic_source_t source;

    if (audio_source == audio_source_le_audio_unicast_sender)
    {
        source.type = source_type_audio;
        source.u.audio = audio_source;
        device = leAudioClient_GetDeviceForSource(source);

        DEBUG_LOG("leAudioClientVolume_SetAudioVolume enum:audio_source_t:%d volume:%d device:%p",
                   audio_source, volume.value, device);

        DeviceProperties_SetAudioVolume(device, volume);
    }
}

static void leAudioClientVolume_SetVoiceVolume(voice_source_t voice_source, volume_t volume)
{
    device_t device;
    generic_source_t source;

    if (voice_source == voice_source_le_audio_unicast_1)
    {
        source.type = source_type_voice;
        source.u.voice = voice_source;
        device = leAudioClient_GetDeviceForSource(source);

        DEBUG_LOG("leAudioClientVolume_SetVoiceVolume enum:voice_source_t:%d volume:%d device:%p",
                   voice_source, volume.value, device);

        DeviceProperties_SetVoiceVolume(device, volume);
    }
}

static volume_t leAudioClientVolume_GetDefaultVolume(void)
{
    volume_t volume = LE_AUDIO_CLIENT_VOLUME(LE_AUDIO_CLIENT_VOLUME_DEFAULT);
    
    return volume;
}

static volume_t leAudioClientVolume_GetAudioVolume(audio_source_t audio_source)
{
    device_t device;
    generic_source_t source;
    volume_t volume = LE_AUDIO_CLIENT_VOLUME(LE_AUDIO_CLIENT_VOLUME_MIN);

    if(audio_source == audio_source_le_audio_unicast_sender)
    {
        source.type = source_type_audio;
        source.u.audio = audio_source;
        device = leAudioClient_GetDeviceForSource(source);

        if (!DeviceProperties_GetAudioVolume(device, volume.config, &volume))
        {
            /* If the audio volume couldn't be read from the device properties
               fallback to the default volume. */
            volume = leAudioClientVolume_GetDefaultVolume();
            DEBUG_LOG("leAudioClientVolume_GetAudioVolume Using default volume. device:%p volume:%d",
                       device, volume.value);
        }
        else
        {
            DEBUG_LOG("leAudioClientVolume_GetAudioVolume from device:%p volume:%d", device, volume.value);
        }
    }

    return volume;
}

static volume_t leAudioClientVolume_GetVoiceVolume(voice_source_t voice_source)
{
    device_t device;
    generic_source_t source;
    volume_t volume = LE_AUDIO_CLIENT_VOLUME(LE_AUDIO_CLIENT_VOLUME_MIN);

    if(voice_source == voice_source_le_audio_unicast_1)
    {
        source.type = source_type_voice;
        source.u.voice = voice_source;
        device = leAudioClient_GetDeviceForSource(source);
        PanicNull(device);

        if (!DeviceProperties_GetVoiceVolume(device, volume.config, &volume))
        {
            /* If the voice volume couldn't be read from the device properties
               fallback to the default voice volume. */
            volume = leAudioClientVolume_GetDefaultVolume();
            DEBUG_LOG("leAudioClientVolume_GetVoiceVolume Using default volume. device:%p volume:%d",
                       device, volume.value);
        }
        else
        {
            DEBUG_LOG("leAudioClientVolume_GetVoiceVolume from device:%p volume:%d", device, volume.value);
        }
    }

    return volume;
}

static inline const audio_source_volume_interface_t * LeAudioVolumeClient_GetAudioSourceVolumeInterface(void)
{
    return &le_audio_client_volume_interface;
}

static inline const voice_source_volume_interface_t * LeAudioVolumeClient_GetVoiceSourceVolumeInterface(void)
{
    return &le_audio_client_voice_volume_interface;
}

void LeAudioClientVolume_Init(void)
{
    DEBUG_LOG("LeAudioClientVolume_Init");

    AudioSources_RegisterVolume(audio_source_le_audio_unicast_sender, LeAudioVolumeClient_GetAudioSourceVolumeInterface());
    VoiceSources_RegisterVolume(voice_source_le_audio_unicast_1, LeAudioVolumeClient_GetVoiceSourceVolumeInterface());
}

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
