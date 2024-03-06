/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface for sending messages from LE audio client to registered modules
*/

#include "le_audio_client_messages.h"
#include "le_audio_client_context.h"

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

/*! Returns TRUE if the context is of type media/gaming */
#define LeAudioClient_IsContextOfTypeMedia(context) (context != CAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL && \
                                                     context != CAP_CLIENT_CONTEXT_TYPE_PROHIBITED)

void leAudioClientMessages_SendConnectInd(ServiceHandle group_handle, le_audio_client_status_t status,
                                          uint8 total_devices, uint8 connected_devices)
{
    MESSAGE_MAKE(message, LE_AUDIO_CLIENT_CONNECT_IND_T);
    message->group_handle = group_handle;
    message->status = status;
    message->total_devices = total_devices;
    message->connected_devices = connected_devices;
    TaskList_MessageSend(le_audio_client_context.client_tasks, LE_AUDIO_CLIENT_CONNECT_IND, message);
}

void leAudioClientMessages_SendDeviceAddedInd(gatt_cid_t cid, bool device_added, bool more_devices_needed)
{
    MESSAGE_MAKE(message, LE_AUDIO_CLIENT_DEVICE_ADDED_IND_T);
    message->cid = cid;
    message->status =  device_added ? LE_AUDIO_CLIENT_STATUS_SUCCESS : LE_AUDIO_CLIENT_STATUS_FAILED;
    message->more_devices_needed =  more_devices_needed;
    TaskList_MessageSend(le_audio_client_context.client_tasks, LE_AUDIO_CLIENT_DEVICE_ADDED_IND, message);
}

void leAudioClientMessages_SendDeviceRemovedInd(gatt_cid_t cid, bool device_removed, bool more_devices_present)
{
    MESSAGE_MAKE(message, LE_AUDIO_CLIENT_DEVICE_REMOVED_IND_T);
    message->cid = cid;
    message->status =  device_removed ? LE_AUDIO_CLIENT_STATUS_SUCCESS : LE_AUDIO_CLIENT_STATUS_FAILED;
    message->more_devices_present =  more_devices_present;
    TaskList_MessageSend(le_audio_client_context.client_tasks, LE_AUDIO_CLIENT_DEVICE_REMOVED_IND, message);
}

void leAudioClientMessages_SendDisconnectInd(ServiceHandle group_handle, bool disconnected)
{
    MESSAGE_MAKE(message, LE_AUDIO_CLIENT_DISCONNECT_IND_T);
    message->group_handle = group_handle;
    message->status = disconnected ? LE_AUDIO_CLIENT_STATUS_SUCCESS : LE_AUDIO_CLIENT_STATUS_FAILED;
    TaskList_MessageSend(le_audio_client_context.client_tasks, LE_AUDIO_CLIENT_DISCONNECT_IND, message);
}

void leAudioClientMessages_SendStreamStartInd(ServiceHandle group_handle,
                                              bool stream_started,
                                              uint16 audio_context,
                                              bool is_source_broadcast)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    MESSAGE_MAKE(message, LE_AUDIO_CLIENT_STREAM_START_IND_T);
    message->group_handle = group_handle;
    message->status = stream_started ? LE_AUDIO_CLIENT_STATUS_SUCCESS : LE_AUDIO_CLIENT_STATUS_FAILED;
    message->audio_context = audio_context;

    if (LeAudioClient_IsContextOfTypeMedia(audio_context))
    {
        message->source.type = source_type_audio;
        message->source.u.audio = is_source_broadcast ? audio_source_le_audio_broadcast_sender :
                                                        audio_source_le_audio_unicast_sender;
    }
    else
    {
        PanicFalse(!is_source_broadcast);
        message->source.type = source_type_voice;
        message->source.u.voice = voice_source_le_audio_unicast_1;
    }

    TaskList_MessageSend(client_ctxt->client_tasks, LE_AUDIO_CLIENT_STREAM_START_IND, message);
}

void leAudioClientMessages_SendStreamStartCancelCompleteInd(ServiceHandle group_handle, bool cancelled)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    MESSAGE_MAKE(message, LE_AUDIO_CLIENT_STREAM_START_CANCEL_COMPLETE_IND_T);
    message->group_handle = group_handle;
    message->status = cancelled ? LE_AUDIO_CLIENT_STATUS_SUCCESS : LE_AUDIO_CLIENT_STATUS_FAILED;

    TaskList_MessageSend(client_ctxt->client_tasks, LE_AUDIO_CLIENT_STREAM_START_CANCEL_COMPLETE_IND, message);
}

void leAudioClientMessages_SendStreamStopInd(ServiceHandle group_handle,
                                             bool stream_stopped,
                                             uint16 audio_context,
                                             bool is_source_broadcast)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    MESSAGE_MAKE(message, LE_AUDIO_CLIENT_STREAM_STOP_IND_T);
    message->group_handle = group_handle;
    message->status = stream_stopped ? LE_AUDIO_CLIENT_STATUS_SUCCESS : LE_AUDIO_CLIENT_STATUS_FAILED;

    if (LeAudioClient_IsContextOfTypeMedia(audio_context))
    {
        message->source.type = source_type_audio;
        message->source.u.audio = is_source_broadcast ? audio_source_le_audio_broadcast_sender :
                                                        audio_source_le_audio_unicast_sender;
    }
    else
    {
        PanicFalse(!is_source_broadcast);
        message->source.type = source_type_voice;
        message->source.u.voice = voice_source_le_audio_unicast_1;
    }

    TaskList_MessageSend(client_ctxt->client_tasks, LE_AUDIO_CLIENT_STREAM_STOP_IND, message);
}

#ifdef ENABLE_ACK_FOR_PA_TRANSMITTED
void leAudioClientMessages_SendPATransmittedInd(void)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();
    TaskList_MessageSendId(client_ctxt->client_tasks, LE_AUDIO_CLIENT_PA_TRANSMITTED_IND);
}
#endif /* ENABLE_ACK_FOR_PA_TRANSMITTED */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
void leAudioClientMessages_HandleRemoteCallControlAccept(void)
{
    TaskList_MessageSendId(le_audio_client_context.client_tasks, LE_AUDIO_CLIENT_REMOTE_CALL_CONTROL_CALL_ACCEPT);
}

void leAudioClientMessages_HandleRemoteCallControlTerminate(void)
{
    TaskList_MessageSendId(le_audio_client_context.client_tasks, LE_AUDIO_CLIENT_REMOTE_CALL_CONTROL_CALL_TERMINATE);
}
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */
