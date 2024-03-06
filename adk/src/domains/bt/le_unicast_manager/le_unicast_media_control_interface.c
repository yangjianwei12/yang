/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_unicast_manager
    \brief      Implementation of the media control interface for LE music sources.
*/
#if defined(INCLUDE_LE_AUDIO_UNICAST)

#include "le_unicast_manager.h"
#include "le_unicast_manager_instance.h"
#include "le_unicast_manager_private.h"
#include "le_unicast_media_control_interface.h"

#include "audio_sources.h"
#include "audio_sources_media_control_interface.h"
#include "multidevice.h"
#include "gatt_pacs_server.h"
#include "bt_device.h"
#include "gatt_connect.h"
#ifdef USE_SYNERGY
#include "media_control_client.h"
#endif
#include "gatt.h"

#include <logging.h>
#include <panic.h>
#include <stdlib.h>

static unsigned leUnicastMusicSource_GetContext(audio_source_t source);

#ifdef USE_SYNERGY

static void leUnicastManagerMediaControl_Play(audio_source_t source);
static void leUnicastManagerMediaControl_Pause(audio_source_t source);
static void leUnicastManagerMediaControl_PlayPause(audio_source_t source);
static void leUnicastManagerMediaControl_Stop(audio_source_t source);
static void leUnicastManagerMediaControl_Forward(audio_source_t source);
static void leUnicastManagerMediaControl_Backward(audio_source_t source);
static void leUnicastManagerMediaControl_FastForward(audio_source_t source, bool state);
static void leUnicastManagerMediaControl_FastRewind(audio_source_t source, bool state);
#define leUnicastManagerMediaControl_Device leUnicastManager_GetBtAudioDevice

#else

#define leUnicastManagerMediaControl_Play               NULL
#define leUnicastManagerMediaControl_Pause              NULL
#define leUnicastManagerMediaControl_PlayPause          NULL
#define leUnicastManagerMediaControl_Stop               NULL
#define leUnicastManagerMediaControl_Forward            NULL
#define leUnicastManagerMediaControl_Backward           NULL
#define leUnicastManagerMediaControl_FastForward        NULL
#define leUnicastManagerMediaControl_FastRewind         NULL
#define leUnicastManagerMediaControl_Device             NULL

#endif

static const media_control_interface_t le_unicast_source_media_control_interface = {
    .Play = leUnicastManagerMediaControl_Play,
    .Pause = leUnicastManagerMediaControl_Pause,
    .PlayPause = leUnicastManagerMediaControl_PlayPause,
    .Stop = leUnicastManagerMediaControl_Stop,
    .Forward = leUnicastManagerMediaControl_Forward,
    .Back = leUnicastManagerMediaControl_Backward,
    .FastForward = leUnicastManagerMediaControl_FastForward,
    .FastRewind = leUnicastManagerMediaControl_FastRewind,
    .NextGroup = NULL,
    .PreviousGroup = NULL,
    .Shuffle = NULL,
    .Repeat = NULL,
    .Context = leUnicastMusicSource_GetContext,
    .Device = leUnicastManagerMediaControl_Device,
};

static bool leUnicastManagerMediaControl_IsStreamingForMedia(audio_source_t source, audio_source_provider_context_t media_context)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByAudioSource(source);

    if (inst)
    {
        if (   media_context != context_audio_is_playing
            && LeUnicastManager_IsContextOfTypeMedia(inst->audio_context)
            && LeUnicastManager_IsAllCisConnected(inst, FALSE))
        {
            /* Media state is not playing but audio context is saying media as well as CISes are established.
               So media is streaming now.
            */
            return TRUE;
        }
    }

    return FALSE;
}

static unsigned leUnicastMusicSource_GetContext(audio_source_t source)
{
    audio_source_provider_context_t context = context_audio_disconnected;
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByAudioSource(source);

    if (inst)
    {
        context = MediaClientControl_GetAudioSourceContext(inst->cid);

        if (leUnicastManagerMediaControl_IsStreamingForMedia(source, context))
        {
            context = context_audio_is_streaming;
        }

    }

    return (unsigned) context;
}

void LeUnicastMediaControlInterface_Init(void)
{
    UNICAST_MANAGER_LOG("LeUnicastMediaControlInterface_Init");
    AudioSources_RegisterMediaControlInterface(audio_source_le_audio_unicast_1, &le_unicast_source_media_control_interface);
}

#ifdef USE_SYNERGY
static void leUnicastManagerMediaControl_Play(audio_source_t source)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByAudioSource(source);

    if (inst)
    {
        MediaControlClient_SendMediaControlOpcode(inst->cid, GATT_MCS_CLIENT_PLAY, 0);
        UNICAST_MANAGER_LOG("leUnicastManagerMediaControl_Play: source=enum:audio_source_t:%d, cid=0x%x", source, inst->cid);
    }
}

static void leUnicastManagerMediaControl_Pause(audio_source_t source)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByAudioSource(source);

    if (inst)
    {
        MediaControlClient_SendMediaControlOpcode(inst->cid, GATT_MCS_CLIENT_PAUSE, 0);
        UNICAST_MANAGER_LOG("leUnicastManagerMediaControl_Pause: source=enum:audio_source_t:%d, cid=0x%x", source, inst->cid);
    }
}

static void leUnicastManagerMediaControl_PlayPause(audio_source_t source)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByAudioSource(source);

    if (inst)
    {
        UNICAST_MANAGER_LOG("leUnicastManagerMediaControl_PlayPause: source=enum:audio_source_t:%d, cid=0x%x", source, inst->cid);
        MediaControlClient_TogglePlayPause(inst->cid);
    }
}

static void leUnicastManagerMediaControl_Stop(audio_source_t source)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByAudioSource(source);

    if (inst)
    {
        MediaControlClient_SendMediaControlOpcode(inst->cid, GATT_MCS_CLIENT_STOP, 0);
        UNICAST_MANAGER_LOG("leUnicastManagerMediaControl_Stop: source=enum:audio_source_t:%d, cid=0x%x", source, inst->cid);
    }
}

static void leUnicastManagerMediaControl_Forward(audio_source_t source)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByAudioSource(source);

    if (inst)
    {
        MediaControlClient_SendMediaControlOpcode(inst->cid, GATT_MCS_CLIENT_NEXT_TRACK, 0);
        UNICAST_MANAGER_LOG("leUnicastManagerMediaControl_Forward: source=enum:audio_source_t:%d, cid=0x%x", source, inst->cid);
    }
}

static void leUnicastManagerMediaControl_Backward(audio_source_t source)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByAudioSource(source);

    if (inst)
    {
        MediaControlClient_SendMediaControlOpcode(inst->cid, GATT_MCS_CLIENT_PREVIOUS_TRACK, 0);
        UNICAST_MANAGER_LOG("leUnicastManagerMediaControl_Backward: source=enum:audio_source_t:%d, cid=0x%x", source, inst->cid);
    }
}

static void leUnicastManagerMediaControl_FastForward(audio_source_t source, bool state)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByAudioSource(source);

    if (inst)
    {
        MediaControlClient_SendMediaControlOpcode(inst->cid, state ? GATT_MCS_CLIENT_FAST_FORWARD : GATT_MCS_CLIENT_STOP, 0);
        UNICAST_MANAGER_LOG("leUnicastManagerMediaControl_FastForward: source=enum:audio_source_t:%d, cid=0x%x state=%u", source, inst->cid, state);
    }
}

static void leUnicastManagerMediaControl_FastRewind(audio_source_t source, bool state)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByAudioSource(source);

    if (inst)
    {
        MediaControlClient_SendMediaControlOpcode(inst->cid, state ? GATT_MCS_CLIENT_FAST_REWIND : GATT_MCS_CLIENT_STOP, 0);
        UNICAST_MANAGER_LOG("leUnicastManagerMediaControl_FastRewind: source=enum:audio_source_t:%d, cid=0x%x state=%u", source, inst->cid, state);
    }
}

#endif /* USE_SYNERGY */

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) */
