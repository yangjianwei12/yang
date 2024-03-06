/*!
    \copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_audio_volume
    \brief
*/

#include "le_audio_volume.h"
#include "le_audio_volume_observer.h"
#include "le_audio_volume_private.h"
#include "le_audio_volume_sync.h"

#include "audio_info.h"
#include "audio_sources.h"
#include "bt_device.h"
#include "device.h"
#include "device_db_serialiser.h"
#include "device_list.h"
#include "device_properties.h"
#include "gatt_connect.h"
#ifdef INCLUDE_LE_AUDIO_BROADCAST
#include "le_broadcast_manager.h"
#endif
#ifdef INCLUDE_LE_AUDIO_UNICAST
#include "le_unicast_manager.h"
#endif
#include "volume_messages.h"
#include "volume_mute.h"
#include "volume_renderer_role.h"
#include "volume_utils.h"
#include "voice_sources.h"

#include <logging.h>

#include <panic.h>

#define LE_AUDIO_VOLUME_LOG   DEBUG_LOG


static int vcp_audio_volume = LE_AUDIO_VOLUME_DEFAULT;
static mute_state_t vcp_audio_mute_state = unmute;

static int vcp_voice_volume = LE_AUDIO_VOLUME_DEFAULT;
static mute_state_t vcp_voice_mute_state = unmute;

static volume_t leAudioVolume_GetAudioVolume(audio_source_t source);
static void leAudioVolume_SetAudioVolume(audio_source_t source, volume_t volume);
static mute_state_t leAudioVolume_GetAudioMuteState(audio_source_t source);
static void leAudioVolume_SetAudioMuteState(audio_source_t source, mute_state_t mute_state);

static volume_t leAudioVolume_GetVoiceVolume(voice_source_t source);
static void leAudioVolume_SetVoiceVolume(voice_source_t source, volume_t volume);
static mute_state_t leAudioVolume_GetVoiceMuteState(voice_source_t source);
static void leAudioVolume_SetVoiceMuteState(voice_source_t source, mute_state_t mute_state);

static void leAudioVolume_RendererVolumeChanged(volume_renderer_volume_changed_t * volume);
static void * leAudioVolume_RendererRetrieveClientConfig(gatt_cid_t cid);
static void leAudioVolume_RendererStoreClientConfig(gatt_cid_t cid, void * config, uint8 size);


static const audio_source_volume_interface_t le_audio_volume_interface =
{
    .GetVolume = leAudioVolume_GetAudioVolume,
    .SetVolume = leAudioVolume_SetAudioVolume,
    .GetMuteState = leAudioVolume_GetAudioMuteState,
    .SetMuteState = leAudioVolume_SetAudioMuteState
};

static const voice_source_volume_interface_t le_audio_voice_volume_interface =
{
    .GetVolume = leAudioVolume_GetVoiceVolume,
    .SetVolume = leAudioVolume_SetVoiceVolume,
    .GetMuteState = leAudioVolume_GetVoiceMuteState,
    .SetMuteState = leAudioVolume_SetVoiceMuteState
};


static device_t leAudioVolume_GetDeviceForAudioSource(audio_source_t source)
{
    device_t device = NULL;

    if (audio_source_le_audio_broadcast == source)
    {
        /* Get the device for a routed broadcast source.

           Note: The broadcast source may be from a non-handset type of device,
           e.g. a broadcast in an airport, so although a broadcast source may
           be routed and playing, it may not have a matching device in the
           device database. In this case the volume will not be stored. */
#ifdef INCLUDE_LE_AUDIO_BROADCAST
        device = LeBroadcastManager_GetDeviceForAudioSource(source);
#endif
    }
    else if (audio_source_le_audio_unicast_1 == source)
    {
#ifdef INCLUDE_LE_AUDIO_UNICAST
        device = LeUnicastManager_GetDeviceForAudioSource(source);
#endif
    }

    if(device)
    {
        size_t size = 0;
        bdaddr *addr = NULL;
        Device_GetProperty(device, device_property_bdaddr, (void *)&addr, &size);
        DEBUG_LOG("leAudioVolume_GetDeviceForAudioSource using device bd addr %04x:%02x:%06x", addr->nap, addr->uap, addr->lap);
    }
    else
    {
        DEBUG_LOG("leAudioVolume_GetDeviceForAudioSource, no device identified");
    }
    return device;
}

/*! \brief Read the volume for a LE audio source from persistent storage.

    The volume for all LE audio sources (unicast, broadcast) is stored in the
    same device property as the A2DP volume. Sharing the same property means
    that if the volume is change via AVRCP or VCP, the change will be reflected
    in both profiles.

    The volume will generally only be read from the device property if the
    audio source being changed is currently connected. Note that it does not
    have to be routed by the audio router.

    For broadcast, the broadcast source may be a third device that the earbuds
    are not paired with. In that case there is no device to read the volume
    from. Instead the volume is read from a global variable - see
    vcp_audio_volume.

    \param[in] source Audio source to get the volume for.

    \return The stored volume, in VCP units, for the given source.
*/
static volume_t leAudioVolume_ReadAudioVolume(audio_source_t source)
{
    volume_t volume = LE_AUDIO_VOLUME(vcp_audio_volume);
    device_t device = leAudioVolume_GetDeviceForAudioSource(source);

    if (device)
    {
        /* The per-device audio volume property is used to store the current LE
           audio (unicast or broadcast) volume. This is so that the end user
           will not get a change in volume when switching between A2DP and a LE
           audio source from the same handset. */
        DeviceProperties_GetAudioVolume(device, volume.config, &volume);
    }

    DEBUG_LOG("leAudioVolume_ReadAudioVolume device 0x%x vol %d", device, volume.value);

    return volume;
}

/*! \brief Store the volume for a LE audio source to persistent storage.

    The volume for all LE audio sources (unicast, broadcast) is stored in the
    same device property as the A2DP volume. Sharing the same property means
    that if the volume is change via AVRCP or VCP, the change will be reflected
    in both profiles.

    See leAudioVolume_ReadAudioVolume for details of how the device to store
    the volume is selected.

    \param[in] source Audio source to get the volume for.
    \param[in] volume Volume, in VCP units, to store.
*/
static void leAudioVolume_StoreAudioVolume(audio_source_t source, volume_t volume)
{
    device_t device = leAudioVolume_GetDeviceForAudioSource(source);

    if (device)
    {
        /* The per-device A2DP volume property is used to store the current LE
           audio (unicast or broadcast) volume. This is so that the end user
           will not get a change in volume when switching between A2DP and a LE
           audio source from the same handset. */
        DeviceProperties_SetAudioVolume(device, volume);
    }

    DEBUG_LOG("leAudioVolume_StoreAudioVolume device 0x%x vol %d", device, volume.value);

    vcp_audio_volume = volume.value;
}

static volume_t leAudioVolume_GetAudioVolume(audio_source_t source)
{
    volume_t volume = LE_AUDIO_VOLUME(LE_AUDIO_VOLUME_MIN);

    if(LeAudioVolume_IsValidLeMusicSource(source))
    {
        volume = leAudioVolume_ReadAudioVolume(source);
    }

    return volume;
}

static void leAudioVolume_SetAudioVolume(audio_source_t source, volume_t volume)
{
    if(LeAudioVolume_IsValidLeMusicSource(source))
    {
        DEBUG_LOG("leAudioVolume_SetAudioVolume volume %d", volume.value);

        leAudioVolume_StoreAudioVolume(source, volume);
    }
}

static mute_state_t leAudioVolume_GetAudioMuteState(audio_source_t source)
{
    mute_state_t mute_state = unmute;
    if(LeAudioVolume_IsValidLeMusicSource(source))
    {
        mute_state = vcp_audio_mute_state;
    }
    return mute_state;
}

static void leAudioVolume_SetAudioMuteState(audio_source_t source, mute_state_t mute_state)
{
    if(LeAudioVolume_IsValidLeMusicSource(source))
    {
        DEBUG_LOG("leAudioVolume_SetAudioMuteState mute_state enum:mute_state_t:%d", mute_state);

        vcp_audio_mute_state = mute_state;
    }
}

/*! \brief Read the volume for a LE voice source from persistent storage.

    The volume for all LE voice sources, e.g. unicast, is stored in the
    same device property as the HFP volume. Sharing the same property means
    that if the volume is change via HFP or VCP, the change will be reflected
    in both profiles.

    The volume will generally only be read from the device property if the
    voice source is currently connected. Note that it does not have to be
    routed.

    \param[in] source Voice source to get the volume for.

    \return The stored volume, in VCP units, for the given source.
*/
static volume_t leAudioVolume_ReadVoiceVolume(voice_source_t source)
{
    volume_t volume = LE_AUDIO_VOLUME(vcp_voice_volume);
    device_t device = NULL;
#ifdef INCLUDE_LE_AUDIO_UNICAST
    device = LeUnicastManager_GetDeviceForVoiceSource(source);
#else
    UNUSED(source);
#endif
    if (device)
    {
        /* The per-device voice volume property is used to store the current LE
           voice volume. This is so that the end user will not get a change in
           volume when switching between HFP and a LE voice source from the
           same handset. */
        DeviceProperties_GetVoiceVolume(device, volume.config, &volume);
    }

    return volume;
}

/*! \brief Store the volume for a LE voice source to persistent storage.

    The volume for all LE voice sources, e.g. unicast, is stored in the
    same device property as the HFP volume. Sharing the same property means
    that if the volume is change via HFP or VCP, the change will be reflected
    in both profiles.

    See leAudioVolume_ReadVoiceVolume for details of how the device to store
    the volume is selected.

    \param[in] source Voice source to get the volume for.
    \param[in] volume Volume, in VCP units, to store.
*/
static void leAudioVolume_StoreVoiceVolume(voice_source_t source, volume_t volume)
{
    device_t device = NULL;
#ifdef INCLUDE_LE_AUDIO_UNICAST
    device = LeUnicastManager_GetDeviceForVoiceSource(source);
#else
    UNUSED(source);
#endif
    if (device)
    {
        /* The per-device voice volume property is used to store the current LE
           voice volume. This is so that the end user will not get a change in
           volume when switching between HFP and a LE voice source from the
           same handset. */
        DeviceProperties_SetVoiceVolume(device, volume);
    }

    vcp_voice_volume = volume.value;
}

static volume_t leAudioVolume_GetVoiceVolume(voice_source_t source)
{
    volume_t volume = LE_AUDIO_VOLUME(LE_AUDIO_VOLUME_MIN);

    if (LeAudioVolume_IsValidLeVoiceSource(source))
    {
        volume = leAudioVolume_ReadVoiceVolume(source);
    }
    return volume;
}

static void leAudioVolume_SetVoiceVolume(voice_source_t source, volume_t volume)
{
    if (LeAudioVolume_IsValidLeVoiceSource(source))
    {
        DEBUG_LOG("leAudioVolume_SetVoiceVolume volume %d", volume.value);
        leAudioVolume_StoreVoiceVolume(source, volume);
    }
}

static mute_state_t leAudioVolume_GetVoiceMuteState(voice_source_t source)
{
    mute_state_t mute_state = unmute;
    if(LeAudioVolume_IsValidLeVoiceSource(source))
    {
        mute_state = vcp_voice_mute_state;
    }
    return mute_state;
}

static void leAudioVolume_SetVoiceMuteState(voice_source_t source, mute_state_t mute_state)
{
    if(LeAudioVolume_IsValidLeVoiceSource(source))
    {
        DEBUG_LOG("leAudioVolume_SetVoiceMuteState mute_state enum:mute_state_t:%d", mute_state);

        vcp_voice_mute_state = mute_state;
    }
}


static inline const audio_source_volume_interface_t * LeAudioVolume_GetAudioSourceVolumeInterface(void)
{
    return &le_audio_volume_interface;
}

static inline const voice_source_volume_interface_t * LeAudioVolume_GetVoiceSourceVolumeInterface(void)
{
    return &le_audio_voice_volume_interface;
}

static const volume_renderer_callback_interface_t volume_renderer_interface =
{
    .VolumeRenderer_VolumeChanged = leAudioVolume_RendererVolumeChanged,
    .VolumeRenderer_RetrieveClientConfig = leAudioVolume_RendererRetrieveClientConfig,
    .VolumeRenderer_StoreClientConfig = leAudioVolume_RendererStoreClientConfig
};

static bool leAudioVolume_GetRoutedGenericSource(generic_source_t* source)
{
    *source = AudioInfo_GetRoutedGenericSource();
    return (source->type != source_type_invalid);
}

static bool leAudioVolume_IsLeAudioOrVoiceSource(generic_source_t *source)
{
    bool is_le_source = FALSE;

    if (   source->type == source_type_audio
        && LeAudioVolume_IsValidLeMusicSource(source->u.audio))
    {
        is_le_source = TRUE;
    }
    else if (   source->type == source_type_voice
             && LeAudioVolume_IsValidLeVoiceSource(source->u.voice))
    {
        is_le_source = TRUE;
    }

    return is_le_source;
}

static bool leAudioVolume_IsOtherAudioSource(generic_source_t *source)
{
    bool is_other_source = FALSE;

    if (   source->type == source_type_audio
        && LeAudioVolume_IsValidOtherMusicSource(source->u.audio))
    {
        is_other_source = TRUE;
    }

    return is_other_source;
}

/*! \brief Get the audio source associated with the most recently used handset.

    \return A valid audio source, or audio_source_none if there is no MRU
            device or no audio source associated with it.
*/
static audio_source_t leAudioVolume_GetMruAudioSource(void)
{
    uint8 is_mru_handset = TRUE;
    device_t device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_mru, &is_mru_handset, sizeof(uint8));

    if (device == NULL)
    {
        deviceType type = DEVICE_TYPE_HANDSET;
        device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_type, &type, sizeof(deviceType));
    }

    if (device != NULL)
    {
        return DeviceProperties_GetAudioSource(device);
    }

    return audio_source_none;
}

static audio_source_t leAudioVolume_GetIdleLeAudioSource(void)
{
    return audio_source_le_audio_broadcast;
}

static void leAudioVolume_RendererVolumeChanged(volume_renderer_volume_changed_t * volume)
{
    generic_source_t routed_source = {0};
    device_t vcp_device = GattConnect_GetBtDevice(volume->cid);

    LE_AUDIO_VOLUME_LOG("leAudioVolume_RendererVolumeChanged  cid %d device %p",
                        volume->cid, vcp_device);

    if (LeAudioVolume_GetRoutedSource(&routed_source))
    {
        if (routed_source.type == source_type_audio)
        {
            LE_AUDIO_VOLUME_LOG("leAudioVolume_RendererVolumeChanged  Audio, vol %d mute %u source enum:audio_source_t:%u",
                        volume->volume_setting, volume->mute, routed_source.u.audio);

            if (LeAudioVolume_IsValidLeMusicSource(routed_source.u.audio) ||
                    (vcp_device == AudioSources_GetAudioSourceDevice(routed_source.u.audio)))
            {
                /* Convert the VCP volume to the range of the source it is being sent to. */
                volume_t source_volume = AudioSources_GetVolume(routed_source.u.audio);
                volume_t new_volume = LE_AUDIO_VOLUME(volume->volume_setting);
                source_volume.value = VolumeUtils_ConvertToVolumeConfig(new_volume, source_volume.config);

                LE_AUDIO_VOLUME_LOG("leAudioVolume_RendererVolumeChanged  Valid audio device, updating volume");

                Volume_SendAudioSourceVolumeUpdateRequest(routed_source.u.audio, event_origin_external, source_volume.value);
                Volume_SendAudioSourceMuteRequest(routed_source.u.audio, event_origin_external, volume->mute);
            }
        }
        else if (routed_source.type == source_type_voice)
        {
            LE_AUDIO_VOLUME_LOG("leAudioVolume_RendererVolumeChanged  Voice, vol %d mute %u source enum:voice_source_t:%u",
                        volume->volume_setting, volume->mute, routed_source.u.voice);
            Volume_SendVoiceSourceVolumeUpdateRequest(routed_source.u.voice, event_origin_external, volume->volume_setting);
            Volume_SendVoiceSourceMuteRequest(routed_source.u.voice, event_origin_external, volume->mute);
        }
        else
        {
            Panic();
        }
    }
    else
    {
        /* If no audio or voice source is curently routed then default to
           sending the volume to the audio source of the MRU handset. */
        audio_source_t source = leAudioVolume_GetMruAudioSource();
        if(source == audio_source_none)
        {
            source = leAudioVolume_GetIdleLeAudioSource();
        }

        if (audio_source_none != source)
        {
            LE_AUDIO_VOLUME_LOG("leAudioVolume_RendererVolumeChanged  MRU Audio, vol %d mute %u source enum:audio_source_t:%u",
                        volume->volume_setting, volume->mute, source);

            if (LeAudioVolume_IsValidLeMusicSource(source) ||
                    (vcp_device == AudioSources_GetAudioSourceDevice(source)))
            {
                /* Convert the VCP volume to the range of the source it is being sent to. */
                volume_t source_volume = AudioSources_GetVolume(source);
                volume_t new_volume = LE_AUDIO_VOLUME(volume->volume_setting);
                source_volume.value = VolumeUtils_ConvertToVolumeConfig(new_volume, source_volume.config);

                LE_AUDIO_VOLUME_LOG("leAudioVolume_RendererVolumeChanged  Valid MRU audio device, updating volume");

                Volume_SendAudioSourceVolumeUpdateRequest(source, event_origin_external, source_volume.value);
                Volume_SendAudioSourceMuteRequest(source, event_origin_external, volume->mute);
            }
        }
    }
}

static void * leAudioVolume_RendererRetrieveClientConfig(gatt_cid_t cid)
{
    device_t device = GattConnect_GetBtDevice(cid);
    void * device_config = NULL;
    if (device)
    {
        size_t size;
        if (!Device_GetProperty(device, device_property_le_audio_volume_config, &device_config, &size))
        {
            device_config = NULL;
        }
    }
    return device_config;
}

static void leAudioVolume_RendererStoreClientConfig(gatt_cid_t cid, void * config, uint8 size)
{
    device_t device = GattConnect_GetBtDevice(cid);
    if (device)
    {
        Device_SetProperty(device, device_property_le_audio_volume_config, config, size);
    }
}

static pdd_size_t leAudioVolume_GetDeviceDataLength(device_t device)
{
    void * config = NULL;
    size_t config_size = 0;

    if (Device_GetProperty(device, device_property_le_audio_volume_config, &config, &config_size) == FALSE)
    {
        config_size = 0;
    }
    return config_size;
}

static void leAudioVolume_SerialisetDeviceData(device_t device, void *buf, pdd_size_t offset)
{
    UNUSED(offset);
    void * config = NULL;
    size_t config_size = 0;

    if (Device_GetProperty(device, device_property_le_audio_volume_config, &config, &config_size))
    {
        memcpy(buf, config, config_size);
    }
}

static void leAudioVolume_DeserialisetDeviceData(device_t device, void *buf, pdd_size_t data_length, pdd_size_t offset)
{
    UNUSED(offset);
    Device_SetProperty(device, device_property_le_audio_volume_config, buf, data_length);
}

static void leAudioVolume_RegisterAsPersistentDeviceDataUser(void)
{
    DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        0x04,
        leAudioVolume_GetDeviceDataLength,
        leAudioVolume_SerialisetDeviceData,
        leAudioVolume_DeserialisetDeviceData);
}


bool LeAudioVolume_Init(Task init_task)
{
    volume_renderer_init_t volume_renderer_init;

    LE_AUDIO_VOLUME_LOG("LeAudioVolume_Init");
    UNUSED(init_task);

    volume_renderer_init.volume_setting = vcp_audio_volume;
    volume_renderer_init.mute = (vcp_audio_mute_state == mute) ? 1 : 0;
    volume_renderer_init.step_size = LE_AUDIO_VOLUME_STEP_SIZE;

    VolumeRenderer_Init(&volume_renderer_init, &volume_renderer_interface);

#ifdef INCLUDE_LE_AUDIO_BROADCAST
    AudioSources_RegisterVolume(audio_source_le_audio_broadcast, LeAudioVolume_GetAudioSourceVolumeInterface());
#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST
    AudioSources_RegisterVolume(audio_source_le_audio_unicast_1, LeAudioVolume_GetAudioSourceVolumeInterface());
    VoiceSources_RegisterVolume(voice_source_le_audio_unicast_1, LeAudioVolume_GetVoiceSourceVolumeInterface());
#endif


    LeAudioVolume_RegisterVolumeObservers();

    leAudioVolume_RegisterAsPersistentDeviceDataUser();

    leAudioVolumeSync_Init();

    return TRUE;
}

bool LeAudioVolume_GetRoutedSource(generic_source_t *generic_source)
{
    bool is_routed = FALSE;
    generic_source_t routed_source = {0};

    if (   leAudioVolume_GetRoutedGenericSource(&routed_source)
        && (   leAudioVolume_IsLeAudioOrVoiceSource(&routed_source)
            || leAudioVolume_IsOtherAudioSource(&routed_source)))
    {
        is_routed = TRUE;
        *generic_source = routed_source;
    }

    return is_routed;
}
