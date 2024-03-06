/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    media_player
    \brief      Implementation of the media player service.
*/

#include "media_player.h"
#include "media_player_private.h"

#include <logging.h>
#include <panic.h>

#include "ui.h"
#include "ui_user_config.h"
#include "av.h"
#include "audio_sources.h"
#include "kymera_adaptation.h"
#if (defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST))
#include "le_audio_messages.h"
#endif
#include "wired_audio_source.h"
#include "usb_audio.h"
#include "audio_router.h"
#include "audio_router_observer.h"
#include "bt_device.h"
#include "device_properties.h"

#include <focus_audio_source.h>
#include <focus_device.h>
#include <focus_select.h>
#ifdef INCLUDE_FAST_PAIR
#include <sass.h>
#endif

static void mediaPlayer_HandleMessage(Task task, MessageId id, Message message);
static void mediaPlayer_HandleMediaMessage(Task task, MessageId id, Message message);

static void mediaPlayer_NotifyAudioRouted(audio_source_t source, source_routing_change_t change);

static void mediaPlayer_OnAudioSourceStateChange(generic_source_t source, source_state_t state);

media_player_task_data_t media_player_task_data;

/* Ui Inputs in which media player service is interested*/
static const message_group_t ui_inputs[] =
{
    UI_INPUTS_MEDIA_PLAYER_MESSAGE_GROUP
};

#ifdef INCLUDE_UI_USER_CONFIG
static const ui_user_config_context_id_map_t media_player_map[] =
{
    { .context_id=ui_context_media_streaming, .context=context_media_player_streaming },
    { .context_id=ui_context_media_idle, .context=context_media_player_idle }
};
#endif

static const audio_source_observer_interface_t media_player_audio_observer_interface =
{
    .OnVolumeChange = NULL,
    .OnAudioRoutingChange = mediaPlayer_NotifyAudioRouted,
    .OnMuteChange = NULL,
};

static const audio_router_observer_interface_t media_player_observer =
{
    .OnSourceStateChange = mediaPlayer_OnAudioSourceStateChange,
};

static inline generic_source_t mediaPlayer_GenericAudioSource(audio_source_t source)
{
    generic_source_t generic_source =
    {
        .type = source_type_audio,
        .u.audio = source
    };

    return generic_source;
}

static unsigned mediaPlayer_ConvertAudioSourceToMediaPlayerContext(audio_source_provider_context_t audio_context)
{
    unsigned media_context = BAD_CONTEXT;
    switch (audio_context)
    {
    case context_audio_disconnected:
        media_context = context_media_player_no_media;
        break;
    case context_audio_connected:
    case context_audio_is_paused:
        media_context = context_media_player_idle;
        break;
    case context_audio_is_streaming:
    case context_audio_is_playing:
    case context_audio_is_va_response:
    case context_audio_is_high_priority:
    case context_audio_is_broadcast:
        media_context = context_media_player_streaming;
        break;
    default:
        Panic();
        break;
    }
    return media_context;
}

static void mediaPlayer_NotifyAudioRouted(audio_source_t source, source_routing_change_t change)
{
    UNUSED(change);
    UNUSED(source);
    audio_source_t focused_source = audio_source_none;
    DEBUG_LOG_FN_ENTRY("mediaPlayer_NotifyAudioRouted");

    /* Received notification from AudioSources that there is change in audio routing activity
       Update UI based on any source is streaming or disconnected */
    if(Focus_GetAudioSourceForContext(&focused_source))
    {
        audio_source_provider_context_t context = AudioSources_GetSourceContext(focused_source);
        if (context >= BAD_CONTEXT)
        {
            context = context_audio_disconnected;
        }
        Ui_InformContextChange(ui_provider_media_player, mediaPlayer_ConvertAudioSourceToMediaPlayerContext(context));
    }
}

/* Start and stop the media player routing timeouts*/
static void mediaPlayer_StartUnroutedMediaPauseTimer(audio_source_t source)
{
    media_player_internal_messages msgId = mediaPlayerConvertAudioSourceToInternalMessageId(source, MEDIA_PLAYER_INTERNAL_UNROUTED_PAUSE_BASE);
    
    DEBUG_LOG("mediaPlayer_StartUnroutedMediaPauseTimer enum:audio_source_t:%d id=%u", source, msgId);
    
    MessageCancelAll(mediaPlayer_MediaTask(), msgId);
    MessageSendLater(mediaPlayer_MediaTask(), msgId, NULL, MEDIA_PLAYER_DELAY_BEFORE_PAUSING_UNROUTED_SOURCE);
}

static void mediaPlayer_CancelUnroutedMediaPauseTimer(audio_source_t source)
{
    media_player_internal_messages msgId = mediaPlayerConvertAudioSourceToInternalMessageId(source, MEDIA_PLAYER_INTERNAL_UNROUTED_PAUSE_BASE);
    DEBUG_LOG("mediaPlayer_CancelUnroutedMediaPauseTimer enum:audio_source_t:%d enum:media_player_internal_messages:%d", source, msgId);
    MessageCancelAll(mediaPlayer_MediaTask(), msgId);
}

static void mediaPlayer_HandleUnroutedMediaPauseTimeout(media_player_internal_messages id)
{
    audio_source_t source = mediaPlayerConvertInternalMessageIdToAudioSource(id, MEDIA_PLAYER_INTERNAL_UNROUTED_PAUSE_BASE);
    source_state_t state = AudioSources_GetState(source);
    bool is_audio_available = AudioSources_IsAudioAvailable(source);

    DEBUG_LOG("mediaPlayer_HandleUnroutedMediaPauseTimeout enum:audio_source_t:%d enum:source_state_t:%d is_audio_available=%d", source, state, is_audio_available);

    /* Media player shall send Pause request only if the audio is available in the source */
    if(is_audio_available)
    {
        switch(state)
        {
            case source_state_connecting:
                mediaPlayer_StartUnroutedMediaPauseTimer(source);
                break;

        case source_state_disconnecting:
        case source_state_disconnected:
        {
            if (Focus_GetFocusForAudioSource(source) != focus_none)
            {
                /* Postpone the unrouted Pause, for as long as the source has any sort of focus.
                 * This is to account for silenced calls, and avoid assuming responsibility
                 * for resuming the music stream whenever the call ends or is rejected. */
                mediaPlayer_StartUnroutedMediaPauseTimer(source);
            }
            else
            {
                AudioSources_Pause(source);
            }
        }
        break;

            default:
                DEBUG_LOG("mediaPlayer_HandleUnroutedMediaPauseTimeout enum:audio_source_t:%d unhandled state enum:source_state_t:%d", source, state);
                break;
        }
    }
}

static void mediaPlayer_OnAudioSourceStateChange(generic_source_t source, source_state_t state)
{
    DEBUG_LOG_FN_ENTRY("mediaPlayer_OnAudioSourceStateChange enum:source_type_t:%d enum:source_state_t:%d", source.type, state);

    if(source.type == source_type_audio)
    {
        if(state == source_state_connected)
        {
            mediaPlayer_CancelUnroutedMediaPauseTimer(source.u.audio);
        }
        else if(state == source_state_disconnecting)
        {
            mediaPlayer_StartUnroutedMediaPauseTimer(source.u.audio);
        }
    }
}

#ifdef INCLUDE_FAST_PAIR
static void mediaPlayer_HandleSwitchAudioSourceUiInput(void)
{
    bdaddr* new_mru_device_bd_addr = NULL;
    device_t new_mru_device = NULL;

    DEBUG_LOG("mediaPlayer_HandleSwitchAudioSourceUiInput");

    new_mru_device_bd_addr = Sass_GetSwitchToDeviceBdAddr();

    if(new_mru_device_bd_addr == NULL)
    {
        DEBUG_LOG("mediaPlayer_HandleSwitchAudioSourceUiInput: bd addr of new mru device is NULL");
        return;
    }
    new_mru_device = BtDevice_GetDeviceForBdAddr(new_mru_device_bd_addr);

    if(!BdaddrIsZero(new_mru_device_bd_addr))
    {
        /* Resume audio streaming on switch to device. As per sass spec, Seeker will send switch active audio source message
           with Bit 1 set (resume playing) if user may tap on 'Switch Back' notification to resume playing on 
           switched away device (previous active audio source device). As per sass spec, Resume playing means Provider sends
           a play notification to Seeker through AVRCP profile */
        if(Sass_ResumePlayingOnSwitchToDevice())
        {
            sass_audio_status_t new_mru_device_audio_status = Sass_GetAudioStatus(new_mru_device);

            if(new_mru_device_audio_status != audio_status_disconnected || new_mru_device_audio_status != audio_status_playing)
            {
                avInstanceTaskData *av_instance = Av_InstanceFindFromDevice(new_mru_device);
                A2dpProfile_ResumeMedia(av_instance);
            }
            Sass_ResetResumePlayingFlag();
        }

        DEBUG_LOG("mediaPlayer_HandleSwitchAudioSourceUiInput. Kick the audio router to update routing.");
        AudioRouter_Update();
    }
}
#endif

static void mediaPlayer_HandleUiInput(MessageId ui_input)
{
    audio_source_t routed_source = audio_source_none;

    if(ui_input == ui_input_pause_all)
    {
        AudioSources_PauseAll();
        return;
    }

#ifdef INCLUDE_FAST_PAIR
    if(ui_input == ui_input_switch_active_audio_source)
    {
        mediaPlayer_HandleSwitchAudioSourceUiInput();
        return;
    }
#endif

    if (Focus_GetAudioSourceForUiInput(ui_input, &routed_source))
    {
        switch(ui_input)
        {
            case ui_input_toggle_play_pause:
                AudioSources_PlayPause(routed_source);
                break;

            case ui_input_play:
                AudioSources_Play(routed_source);
                break;

            case ui_input_pause:
                AudioSources_Pause(routed_source);
                break;

            case ui_input_stop_av_connection:
                AudioSources_Stop(routed_source);
                break;

            case ui_input_av_forward:
                AudioSources_Forward(routed_source);
                break;

            case ui_input_av_backward:
                AudioSources_Back(routed_source);
                break;

            case ui_input_av_fast_forward_start:
                AudioSources_FastForward(routed_source, TRUE);
                break;

            case ui_input_fast_forward_stop:
                AudioSources_FastForward(routed_source, FALSE);
                break;

            case ui_input_av_rewind_start:
                AudioSources_FastRewind(routed_source, TRUE);
                break;

            case ui_input_rewind_stop:
                AudioSources_FastRewind(routed_source, FALSE);
                break;

            default:
                break;
        }
    }
}

static void mediaPlayer_ConnectAudio(audio_source_t source)
{
    bool is_audio_available = AudioSources_IsAudioAvailable(source);

    DEBUG_LOG("mediaPlayer_ConnectAudio enum:audio_source_t:%d enum:audio_source_provider_context_t:%d is_audio_available=%d",
              source, AudioSources_GetSourceContext(source), is_audio_available);

    AudioRouter_AddSource(mediaPlayer_GenericAudioSource(source));

    /* Start UnroutedMediaPauseTimer if the source is not routed but audio is available */
    if(is_audio_available && AudioSources_GetState(source) != source_state_connected)
    {
        mediaPlayer_StartUnroutedMediaPauseTimer(source);
    }
}

static void mediaPlayer_DisconnectAudio(audio_source_t source)
{
    DEBUG_LOG("mediaPlayer_DisconnectAudio enum:audio_source_t:%d", source);
    AudioRouter_RemoveSource(mediaPlayer_GenericAudioSource(source));
}

static void mediaPlayer_HandleA2dpConnectInd(AV_A2DP_CONNECTED_IND_T * ind)
{
    audio_source_t source = Av_GetSourceForInstance(ind->av_instance);
    DEBUG_LOG("mediaPlayer_HandleA2dpConnectInd %p enum:audio_source_t:%d", ind->av_instance, source);
    
    if (source != audio_source_none)
    {
        mediaPlayer_ConnectAudio(source);
    }
}

static void mediaPlayer_HandleA2dpDisconnectInd(AV_A2DP_DISCONNECTED_IND_T * ind)
{
    audio_source_t source = Av_GetSourceForInstance(ind->av_instance);
    
    DEBUG_LOG_VERBOSE("mediaPlayer_HandleA2dpDisconnectInd %p enum:audio_source_t:%d enum:avA2dpDisconnectReason:%d",
                      ind->av_instance, source, ind->reason);

    if (source != audio_source_none)
    {
        if (ind->reason == AV_A2DP_DISCONNECT_LINK_TRANSFERRED && AudioSources_IsAudioRouted(source))
        {
            return;
        }

        mediaPlayer_DisconnectAudio(source);
    }
}

static void mediaPlayer_HandleA2dpAudioConnectInd(AV_A2DP_AUDIO_CONNECT_MESSAGE_T * ind)
{
    DEBUG_LOG("mediaPlayer_HandleA2dpAudioConnectInd enum:audio_source_t:%d", ind->audio_source);
    if (ind->audio_source != audio_source_none)
    {
        mediaPlayer_ConnectAudio(ind->audio_source);
    }
}

static void mediaPlayer_HandleA2dpAudioDisconnectInd(AV_A2DP_AUDIO_DISCONNECT_MESSAGE_T * ind)
{
    DEBUG_LOG("mediaPlayer_HandleA2dpAudioDisconnectInd enum:audio_source_t:%d", ind->audio_source);
    if (ind->audio_source != audio_source_none)
    {
        if(AudioSources_GetSourceContext(ind->audio_source) == context_audio_disconnected)
        {
            mediaPlayer_DisconnectAudio(ind->audio_source);
        }
        else
        {
            AudioRouter_Update();
        }
    }
}

static void mediaPlayer_HandleAvrcpPlayStatusPlayingMessage(const AV_AVRCP_PLAY_STATUS_PLAYING_IND_T *message)
{
    DEBUG_LOG_FN_ENTRY("mediaPlayer_HandleAvrcpPlayStatusPlayingMessage");

    mediaPlayer_StartUnroutedMediaPauseTimer(Av_GetSourceForInstance(message->av_instance));

    appDeviceUpdateMruDevice(&message->av_instance->bd_addr);
    AudioRouter_Update();
}

static void mediaPlayer_HandleAvrcpPlayStatusNotPlayingMessage(const AV_AVRCP_PLAY_STATUS_NOT_PLAYING_IND_T *message)
{
    DEBUG_LOG_FN_ENTRY("mediaPlayer_HandleAvrcpPlayStatusNotPlayingMessage");
    
    mediaPlayer_CancelUnroutedMediaPauseTimer(Av_GetSourceForInstance(message->av_instance));
    
    AudioRouter_Update();
}

#ifdef INCLUDE_LE_AUDIO_UNICAST

static void mediaPlayer_HandleLeAudioUnicastMediaDataPathReady(void)
{
    AudioRouter_Update();
}

static void mediaPlayer_HandleLeAudioUnicastMediaDisconnected(const LE_AUDIO_UNICAST_MEDIA_DISCONNECTED_T *msg)
{
    bool interrupted = AudioRouter_SourceIsInterrupted(mediaPlayer_GenericAudioSource(msg->audio_source));

    DEBUG_LOG_FN_ENTRY("mediaPlayer_HandleLeAudioUnicastMediaDisconnected: source=enum:audio_source_t:%d interrupted=%u", msg->audio_source, interrupted);

    if (!interrupted)
    {
        mediaPlayer_DisconnectAudio(msg->audio_source);
    }
    else
    {
        AudioRouter_Update();
    }
}
#endif /* INCLUDE_LE_AUDIO_UNICAST */

static void mediaPlayer_HandleMediaMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    DEBUG_LOG("mediaPlayer_HandleMediaMessage message id, MESSAGE:0x%x", id);

    if(id >= MEDIA_PLAYER_INTERNAL_UNROUTED_PAUSE_BASE && id < MEDIA_PLAYER_INTERNAL_MESSAGES_MAX_SOURCES)
    {
        mediaPlayer_HandleUnroutedMediaPauseTimeout((media_player_internal_messages)id);
    }

    switch(id)
    {
        case AV_A2DP_AUDIO_CONNECTING:
        case AV_A2DP_AUDIO_CONNECTED:
            mediaPlayer_HandleA2dpAudioConnectInd((AV_A2DP_AUDIO_CONNECT_MESSAGE_T*)message);
            break;
            
        case AV_A2DP_AUDIO_DISCONNECTED:
            mediaPlayer_HandleA2dpAudioDisconnectInd((AV_A2DP_AUDIO_DISCONNECT_MESSAGE_T*)message);
            break;

        case AV_A2DP_CONNECTED_IND:
            mediaPlayer_HandleA2dpConnectInd((AV_A2DP_CONNECTED_IND_T *)message);
            break;

        case AV_A2DP_DISCONNECTED_IND:
            mediaPlayer_HandleA2dpDisconnectInd((AV_A2DP_DISCONNECTED_IND_T *)message);
            break;

        case AV_AVRCP_PLAY_STATUS_PLAYING_IND:
            mediaPlayer_HandleAvrcpPlayStatusPlayingMessage((const AV_AVRCP_PLAY_STATUS_PLAYING_IND_T*)message);
            break;

        case AV_AVRCP_PLAY_STATUS_NOT_PLAYING_IND:
            mediaPlayer_HandleAvrcpPlayStatusNotPlayingMessage((const AV_AVRCP_PLAY_STATUS_NOT_PLAYING_IND_T*)message);
            break;

        case WIRED_AUDIO_DEVICE_CONNECT_IND:
            mediaPlayer_ConnectAudio(((WIRED_AUDIO_DEVICE_CONNECT_IND_T*)message)->audio_source);
            break;

        case WIRED_AUDIO_DEVICE_DISCONNECT_IND:
            mediaPlayer_DisconnectAudio(((WIRED_AUDIO_DEVICE_DISCONNECT_IND_T*)message)->audio_source);
            break;

#ifdef INCLUDE_LE_AUDIO_BROADCAST
        case LE_AUDIO_BROADCAST_CONNECTED:
            mediaPlayer_ConnectAudio(((LE_AUDIO_BROADCAST_CONNECTED_T*)message)->audio_source);
            break;

        case LE_AUDIO_BROADCAST_DISCONNECTED:
            mediaPlayer_DisconnectAudio(((LE_AUDIO_BROADCAST_DISCONNECTED_T*)message)->audio_source);
            break;
#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST
        case LE_AUDIO_UNICAST_MEDIA_CONNECTED:
            mediaPlayer_ConnectAudio(((LE_AUDIO_UNICAST_MEDIA_CONNECTED_T*)message)->audio_source);
            break;

        case LE_AUDIO_UNICAST_MEDIA_DISCONNECTED:
            mediaPlayer_HandleLeAudioUnicastMediaDisconnected((LE_AUDIO_UNICAST_MEDIA_DISCONNECTED_T*) message);
            break;

        case LE_AUDIO_UNICAST_MEDIA_DATA_PATH_READY:
            mediaPlayer_HandleLeAudioUnicastMediaDataPathReady();
            break;

        case LE_AUDIO_UNICAST_VOICE_CONNECTED:
        case LE_AUDIO_UNICAST_VOICE_DISCONNECTED:
        case LE_AUDIO_UNICAST_CIS_CONNECTED:
        case LE_AUDIO_UNICAST_CIS_DISCONNECTED:
        case LE_AUDIO_UNICAST_ENABLED:
            /* Message ignored */
            break;
#endif


        case USB_AUDIO_CONNECTED_IND:
            mediaPlayer_ConnectAudio(((USB_AUDIO_CONNECT_MESSAGE_T *)message)->audio_source);
            break;

        case USB_AUDIO_DISCONNECTED_IND:
            mediaPlayer_DisconnectAudio(((USB_AUDIO_DISCONNECT_MESSAGE_T *)message)->audio_source);
            break;

        default:
            DEBUG_LOG("mediaPlayer_HandleMediaMessage unknown message id, MESSAGE:0x%x", id);
            break;
    }
}

static unsigned mediaPlayer_GetFocusedContext(void)
{
    audio_source_provider_context_t context = context_audio_disconnected;
    audio_source_t focused_source = audio_source_none;

    DEBUG_LOG_FN_ENTRY("mediaPlayer_GetFocusedContext");

    if (Focus_GetAudioSourceForContext(&focused_source))
    {
        context = AudioSources_GetSourceContext(focused_source);
        if (context >= BAD_CONTEXT)
        {
            context = context_audio_disconnected;
        }
    }

    DEBUG_LOG_INFO("mediaPlayer_GetFocusedContext source=%d context=%d", focused_source, context);

    return mediaPlayer_ConvertAudioSourceToMediaPlayerContext(context);
}

static void mediaPlayer_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    if (isMessageUiInput(id))
    {
        mediaPlayer_HandleUiInput(id);
    }
}

/*! \brief Initialise the media player service
*/
bool MediaPlayer_Init(Task init_task)
{
    UNUSED(init_task);
    audio_source_t source = audio_source_none;

    DEBUG_LOG_FN_ENTRY("MediaPlayer_Init");
    
    memset(mediaPlayer_MediaTaskData(), 0, sizeof(media_player_task_data_t));
    mediaPlayer_MediaTask()->handler = mediaPlayer_HandleMediaMessage;
    mediaPlayer_UiTask()->handler = mediaPlayer_HandleMessage;

    /* Register av task call back as ui provider*/
    Ui_RegisterUiProvider(ui_provider_media_player, mediaPlayer_GetFocusedContext);

    Ui_RegisterUiInputConsumer(mediaPlayer_UiTask(), ui_inputs, ARRAY_DIM(ui_inputs));

#ifdef INCLUDE_UI_USER_CONFIG
    UiUserConfig_RegisterContextIdMap(ui_provider_media_player, media_player_map, ARRAY_DIM(media_player_map));
#endif

    appAvStatusClientRegister(mediaPlayer_MediaTask());

    WiredAudioSource_ClientRegister(mediaPlayer_MediaTask());
#if (defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST))
    LeAudioMessages_ClientRegister(mediaPlayer_MediaTask());
#endif
    UsbAudio_ClientRegister(mediaPlayer_MediaTask(),
                             USB_AUDIO_REGISTERED_CLIENT_MEDIA);

    while(++source < max_audio_sources)
    {
        AudioSources_RegisterObserver(source, &media_player_audio_observer_interface);
    }

    AudioRouterObserver_RegisterAudioObserver(&media_player_observer);

    return TRUE;
}
