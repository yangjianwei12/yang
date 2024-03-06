/*!
    \copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_audio_volume
    \brief      Synchronise the LE audio volume from the Primary earbud to the Secondary earbud.
*/

#if (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)) && defined(INCLUDE_MIRRORING)

#include "le_audio_volume.h"
#include "le_audio_volume_sync.h"
#include "le_audio_volume_sync_marshal_typedef.h"
#include "le_audio_volume_sync_typedef.h"

#include "bt_device.h"
#include "peer_signalling.h"
#include "voice_sources.h"
#include "volume_messages.h"

#include <logging.h>
#include <panic.h>


/*
    This module is used to forward the current LEA volume from the Primary
    earbud to the Secondary earbud when the secondary has no direct link to
    the handset device.

    The Audio and Voice Source Volume Observer interfaces are used to monitor
    changes in the volume on the Primary device. Any changes are forwarded
    to the Secondary over a peer_signalling channel.

    If the volume is changed on the Seoncdary by a user input, e.g. a button
    press, the UI event shall be forwarded to the Primary and then the updated
    volume applied from there, instead of the Secondary forwarding the new
    LEA volume directly.

    Use of the volume observer interface:

    OnVolumeChange : Whether the volume is changed by the handset or a local
                     UI event, this callback will be called. The new volume
                     will be forwarded on to the Secondary.

    OnAudioRoutingChange : (audio sources only) When an audio source is routed
                     this will be callback called. This is used to forward the
                     current LEA volume to the Secondary so that when the audio
                     chain is created on the secondary it will have the latest
                     volume to match the Primary.

`   OnMuteChange : If the mute state is changed by a non-peer event the new
                   mute state will be forwarded on to the Secondary.

    The volume observer interfaces are only registered when the Secondary
    earbud is connected to the Primary because otherwise there is no need to
    try to forward the volume.
*/

#define leAudioVolumeSync_IsLeAudioSource(source) \
    (   (audio_source_le_audio_broadcast == (source)) \
     || (audio_source_le_audio_unicast_1 == (source)) )


#define leAudioVolumeSync_IsOtherAudioSource(source) \
    (   (audio_source_a2dp_1 == (source)) \
     || (audio_source_a2dp_2 == (source)))

#define leAudioVolumeSync_IsLeVoiceSource(source) \
    (   (voice_source_le_audio_unicast_1 == (source)) )


le_audio_volume_sync_data_t le_audio_volume_sync_data;


static void leAudioVolumeSync_OnAudioVolumeChange(audio_source_t source, event_origin_t origin, volume_t volume);
static void leAudioVolumeSync_OnAudioRoutingChange(audio_source_t source, source_routing_change_t change);
static void leAudioVolumeSync_OnAudioMuteChange(audio_source_t source, event_origin_t origin, bool mute_state);

#ifdef INCLUDE_LE_AUDIO_UNICAST
static void leAudioVolumeSync_OnVoiceVolumeChange(voice_source_t source, event_origin_t origin, volume_t volume);
static void leAudioVolumeSync_OnVoiceMuteChange(voice_source_t source, event_origin_t origin, bool mute_state);
#endif

static const audio_source_observer_interface_t leavs_audio_observer_interface =
{
    .OnVolumeChange = leAudioVolumeSync_OnAudioVolumeChange,
    .OnAudioRoutingChange = leAudioVolumeSync_OnAudioRoutingChange,
    .OnMuteChange = leAudioVolumeSync_OnAudioMuteChange,
};

#ifdef INCLUDE_LE_AUDIO_UNICAST
static const voice_source_observer_interface_t leavs_voice_observer_interface =
{
    .OnVolumeChange = leAudioVolumeSync_OnVoiceVolumeChange,
    .OnMuteChange = leAudioVolumeSync_OnVoiceMuteChange,
};
#endif

static void leAudioVolumeSync_SendLeaVolumeToSecondary(const generic_source_t *source, uint8 volume, uint8 mute)
{
    le_audio_volume_sync_update_t* msg = PanicUnlessMalloc(sizeof(*msg));
    msg->source_type = source->type;
    msg->source = source->u.audio ;
    msg->volume = volume;
    msg->mute = mute;

    DEBUG_LOG("leAudioVolumeSync_SendLeaVolumeToSecondary type enum:source_type_t:%u source %u vol %u mute %u",
              source->type, source->u.audio, volume, mute);

    appPeerSigMarshalledMsgChannelTxCancelAll(leAudioVolumeSync_GetTask(),
                                              PEER_SIG_MSG_CHANNEL_LE_AUDIO_VOLUME,
                                              MARSHAL_TYPE(le_audio_volume_sync_update_t));
    appPeerSigMarshalledMsgChannelTx(leAudioVolumeSync_GetTask(),
                                     PEER_SIG_MSG_CHANNEL_LE_AUDIO_VOLUME,
                                     (msg), MARSHAL_TYPE(le_audio_volume_sync_update_t));
}

static void leAudioVolumeSync_OnAudioVolumeChange(audio_source_t source, event_origin_t origin, volume_t volume)
{
    if (   (   leAudioVolumeSync_IsLeAudioSource(source)
            || leAudioVolumeSync_IsOtherAudioSource(source))
        && (event_origin_external == origin || event_origin_local == origin))
    {
        generic_source_t generic_source = {
                .type = source_type_audio,
                .u.audio = source
            };
        bool mute_state = AudioSources_GetMuteState(source);

        DEBUG_LOG("leAudioVolumeSync_OnVolumeChange source enum:audio_source_t:%u origin enum:event_origin_t:%u volume %d",
                  source, origin, volume.value);

        leAudioVolumeSync_SendLeaVolumeToSecondary(&generic_source, volume.value, mute_state);
    }
}

static void leAudioVolumeSync_OnAudioRoutingChange(audio_source_t source, source_routing_change_t change)
{
    DEBUG_LOG("leAudioVolumeSync_OnAudioRoutingChange source enum:audio_source_t:%u change enum:source_routing_change_t:%u",
              source, change);

    if (   (   leAudioVolumeSync_IsLeAudioSource(source)
            || leAudioVolumeSync_IsOtherAudioSource(source))
        && (source_routed == change))
    {
        generic_source_t generic_source = {
            .type = source_type_audio,
            .u.audio = source
        };
        volume_t volume = AudioSources_GetVolume(source);
        bool mute_state = AudioSources_GetMuteState(source);

        leAudioVolumeSync_SendLeaVolumeToSecondary(&generic_source, volume.value, mute_state);
    }
}

static void leAudioVolumeSync_OnAudioMuteChange(audio_source_t source, event_origin_t origin, bool mute_state)
{
    if (   (   leAudioVolumeSync_IsLeAudioSource(source)
            || leAudioVolumeSync_IsOtherAudioSource(source))
        &&  (event_origin_external == origin || event_origin_local == origin))
    {
        generic_source_t generic_source = {
                .type = source_type_audio,
                .u.audio = source
            };
        volume_t volume = AudioSources_GetVolume(source);

        DEBUG_LOG("leAudioVolumeSync_OnAudioMuteChange source enum:audio_source_t:%u origin enum:event_origin_t:%u mute %d",
                  source, origin, mute_state);

        leAudioVolumeSync_SendLeaVolumeToSecondary(&generic_source, volume.value, mute_state);
    }
}

#ifdef INCLUDE_LE_AUDIO_UNICAST
static void leAudioVolumeSync_OnVoiceVolumeChange(voice_source_t source, event_origin_t origin, volume_t volume)
{
    if (    leAudioVolumeSync_IsLeVoiceSource(source)
        &&  (event_origin_external == origin || event_origin_local == origin))
    {
        generic_source_t generic_source = {
                .type = source_type_voice,
                .u.voice = source
            };
        bool mute_state = VoiceSources_GetMuteState(source);

        DEBUG_LOG("leAudioVolumeSync_OnVoiceVolumeChange source enum:voice_source_t:%u origin enum:event_origin_t:%u volume %d",
                  source, origin, volume.value);

        leAudioVolumeSync_SendLeaVolumeToSecondary(&generic_source, volume.value, mute_state);
    }
}

static void leAudioVolumeSync_OnVoiceMuteChange(voice_source_t source, event_origin_t origin, bool mute_state)
{
    if (    leAudioVolumeSync_IsLeVoiceSource(source)
        &&  (event_origin_external == origin || event_origin_local == origin))
    {
        generic_source_t generic_source = {
                .type = source_type_voice,
                .u.voice = source
            };
        volume_t volume = VoiceSources_GetVolume(source);

        DEBUG_LOG("leAudioVolumeSync_OnVoiceMuteChange source enum:voice_source_t:%u origin enum:event_origin_t:%u mute %d",
                  source, origin, mute_state);

        leAudioVolumeSync_SendLeaVolumeToSecondary(&generic_source, volume.value, mute_state);
    }
}
#endif

static void leAudioVolumeSync_RegisterVolumeObservers(void)
{
#ifdef INCLUDE_LE_AUDIO_BROADCAST
        AudioSources_RegisterObserver(audio_source_le_audio_broadcast, &leavs_audio_observer_interface);
#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST
        AudioSources_RegisterObserver(audio_source_le_audio_unicast_1, &leavs_audio_observer_interface);
        VoiceSources_RegisterObserver(voice_source_le_audio_unicast_1, &leavs_voice_observer_interface);
#endif

#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
        AudioSources_RegisterObserver(audio_source_a2dp_1, &leavs_audio_observer_interface);
        AudioSources_RegisterObserver(audio_source_a2dp_2, &leavs_audio_observer_interface);
#endif

}

static void leAudioVolumeSync_UnregisterVolumeObservers(void)
{
#ifdef INCLUDE_LE_AUDIO_BROADCAST
        AudioSources_UnregisterObserver(audio_source_le_audio_broadcast, &leavs_audio_observer_interface);
#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST
        AudioSources_UnregisterObserver(audio_source_le_audio_unicast_1, &leavs_audio_observer_interface);
        VoiceSources_DeregisterObserver(voice_source_le_audio_unicast_1, &leavs_voice_observer_interface);
#endif

#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
        AudioSources_UnregisterObserver(audio_source_a2dp_1, &leavs_audio_observer_interface);
        AudioSources_UnregisterObserver(audio_source_a2dp_2, &leavs_audio_observer_interface);
#endif

}

static void leAudioVolumeSync_HandleLeaVolumeUpdate(const le_audio_volume_sync_update_t *update)
{
    UNUSED(update);

    DEBUG_LOG("leAudioVolumeSync_HandleLeaVolumeUpdate type enum:source_type_t:%u source %u vol %u mute %u",
              update->source_type, update->source, update->volume, update->mute);

    if (update->source_type == source_type_audio)
    {
        Volume_SendAudioSourceVolumeUpdateRequest(update->source, event_origin_peer, update->volume);
        if (update->mute != AudioSources_GetMuteState(update->source))
        {
            Volume_SendAudioSourceMuteRequest(update->source, event_origin_peer, update->mute);
        }
    }
    else if (update->source_type == source_type_voice)
    {
        Volume_SendVoiceSourceVolumeUpdateRequest(update->source, event_origin_peer, update->volume);
        if (update->mute != VoiceSources_GetMuteState(update->source))
        {
            Volume_SendVoiceSourceMuteRequest(update->source, event_origin_peer, update->mute);
        }
    }
    else
    {
        Panic();
    }
}

static void leAudioVolumeSync_SendCurrentVolumeToPeer(void)
{
    generic_source_t routed_source = {0};

    if (LeAudioVolume_GetRoutedSource(&routed_source))
    {
        volume_t volume = {0};
        bool mute_state = FALSE;

        if (routed_source.type == source_type_audio)
        {
            volume = AudioSources_GetVolume(routed_source.u.audio);
            mute_state = AudioSources_GetMuteState((routed_source.u.audio));
        }
        else if (routed_source.type == source_type_voice)
        {
            volume = VoiceSources_GetVolume(routed_source.u.voice);
            mute_state = VoiceSources_GetMuteState(routed_source.u.voice);
        }
        else
        {
            Panic();
        }

        leAudioVolumeSync_SendLeaVolumeToSecondary(&routed_source, volume.value, mute_state);
    }
}

static void leAudioVolumeSync_HandlePeerSigConnectionInd(const PEER_SIG_CONNECTION_IND_T *ind)
{
    le_audio_volume_sync_data_t *leavs = leAudioVolumeSync_Get();

    DEBUG_LOG("leAudioVolumeSync_HandlePeerSigConnectionInd status enum:peerSigStatus:%u is_primary %d",
              ind->status, leavs->is_primary);

    switch (ind->status)
    {
        case peerSigStatusConnected:
            /* Only the primary earbud should send volume updates to the
               secondary. So only register volume observers on the primary
               earbud. */
            if (leavs->is_primary)
            {
                leAudioVolumeSync_RegisterVolumeObservers();
                leAudioVolumeSync_SendCurrentVolumeToPeer();
            }
            break;

        case peerSigStatusDisconnected:
        case peerSigStatusLinkLoss:
            if (leavs->is_primary)
            {
                leAudioVolumeSync_UnregisterVolumeObservers();
            }
            break;

        default:
            /* Ignore */
            break;
    }
}

static void leAudioVolumeSync_HandleMarshalMsgTxCfm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T *cfm)
{
    UNUSED(cfm);
}

static void leAudioVolumeSync_HandleMarshalMsgRxInd(const PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T *ind)
{
    switch (ind->type)
    {
    case MARSHAL_TYPE(le_audio_volume_sync_update_t):
        {
            const le_audio_volume_sync_update_t *update = (const le_audio_volume_sync_update_t *)ind->msg;
            leAudioVolumeSync_HandleLeaVolumeUpdate(update);
        }
        break;

    default:
        DEBUG_LOG("leAudioVolumeSync_HandleMarshalMsgRxInd unhandled type 0x%x", ind->type);
        break;
    }

    /* free unmarshalled msg */
    free(ind->msg);
}

static void leAudioVolumeSync_MessageHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);

    switch (id)
    {
        case PEER_SIG_CONNECTION_IND:
            leAudioVolumeSync_HandlePeerSigConnectionInd(msg);
            break;

        case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
            leAudioVolumeSync_HandleMarshalMsgTxCfm(msg);
            break;

        case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
            leAudioVolumeSync_HandleMarshalMsgRxInd(msg);
            break;

        default:
            break;
    }
}

void leAudioVolumeSync_Init(void)
{
    memset(&le_audio_volume_sync_data, 0, sizeof(le_audio_volume_sync_data));
    le_audio_volume_sync_data.task.handler = leAudioVolumeSync_MessageHandler;

    appPeerSigClientRegister(leAudioVolumeSync_GetTask());

    appPeerSigMarshalledMsgChannelTaskRegister(leAudioVolumeSync_GetTask(),
                                               PEER_SIG_MSG_CHANNEL_LE_AUDIO_VOLUME,
                                               le_audio_volume_sync_marshal_type_descriptors,
                                               NUMBER_OF_LE_AUDIO_VOLUME_SYNC_MARSHAL_TYPES);
}

void LeAudioVolumeSync_SetRole(bool primary)
{
    le_audio_volume_sync_data_t *leavs = leAudioVolumeSync_Get();

    if (leavs->is_primary != primary)
    {
        if (primary && appPeerSigIsConnected())
        {
            leAudioVolumeSync_RegisterVolumeObservers();
        }
        else
        {
            leAudioVolumeSync_UnregisterVolumeObservers();
        }

        leavs->is_primary = primary;
    }

    DEBUG_LOG("leAudioVolumeSync_SetRole primary %u", leavs->is_primary);
}

#endif /* #if (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)) && defined(INCLUDE_MIRRORING) */
