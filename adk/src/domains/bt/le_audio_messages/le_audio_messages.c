/*!
\copyright  Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Functions for generating LE Audio notification messages
*/

#if (defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST))
#include "le_audio_messages.h"

#include <task_list.h>
#include <panic.h>
#include <logging.h>
#include <stdlib.h>


/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_ENUM(le_audio_domain_messages)

static task_list_t client_list;

#define leAudioMessages_GetMessageClients() (&client_list)

#ifdef INCLUDE_LE_AUDIO_BROADCAST

void LeAudioMessages_SendBroadcastConnectStatus(MessageId id, audio_source_t audio_source)
{
    LE_AUDIO_BROADCAST_CONNECTED_T * const message = (LE_AUDIO_BROADCAST_CONNECTED_T *) PanicUnlessMalloc(sizeof(*message));

    message->audio_source = audio_source;
    TaskList_MessageSendWithSize(leAudioMessages_GetMessageClients(), id, message, sizeof(*message));
}

void LeAudioMessages_SendBroadcastMetadata(uint8 metadata_len, uint8 *metadata)
{
    uint16 msg_len = offsetof(LE_AUDIO_BROADCAST_METADATA_PAYLOAD_T, metadata) + metadata_len;
    LE_AUDIO_BROADCAST_METADATA_PAYLOAD_T * const message =
            (LE_AUDIO_BROADCAST_METADATA_PAYLOAD_T * ) malloc(msg_len);

    if (message != NULL)
    {
        message->metadata_len = metadata_len;
        memcpy(&message->metadata, metadata, metadata_len);
        TaskList_MessageSendWithSize(leAudioMessages_GetMessageClients(), LE_AUDIO_BROADCAST_METADATA_PAYLOAD, message, msg_len);
    }
    else
    {
        DEBUG_LOG("LeAudioMessages_SendBroadcastMetadata Memory allocation failed");
    }
}

#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST

void LeAudioMessages_SendUnicastEnabled(multidevice_side_t side)
{
    LE_AUDIO_UNICAST_ENABLED_T * const message = (LE_AUDIO_UNICAST_ENABLED_T *) PanicUnlessMalloc(sizeof(*message));

    message->side = side;
    TaskList_MessageSendWithSize(leAudioMessages_GetMessageClients(), LE_AUDIO_UNICAST_ENABLED, message, sizeof(*message));
}

void LeAudioMessages_SendUnicastVoiceConnectStatus(MessageId id, voice_source_t voice_source)
{
    LE_AUDIO_UNICAST_VOICE_CONNECTED_T * const message = (LE_AUDIO_UNICAST_VOICE_CONNECTED_T *) PanicUnlessMalloc(sizeof(*message));

    message->voice_source = voice_source;
    TaskList_MessageSendWithSize(leAudioMessages_GetMessageClients(), id, message, sizeof(*message));

}

void LeAudioMessages_SendUnicastMediaConnectStatus(MessageId id, audio_source_t audio_source, uint16 audio_context)
{
    LE_AUDIO_UNICAST_MEDIA_CONNECTED_T * const message = (LE_AUDIO_UNICAST_MEDIA_CONNECTED_T *) PanicUnlessMalloc(sizeof(*message));

    message->audio_source = audio_source;
    message->audio_context = audio_context;
    TaskList_MessageSendWithSize(leAudioMessages_GetMessageClients(), id, message, sizeof(*message));
}

void LeAudioMessages_SendUnicastCisConnectStatus(MessageId id, multidevice_side_t side, uint8 cis_id, uint16 cis_handle, uint8 cis_dir)
{
    LE_AUDIO_UNICAST_CIS_CONNECTED_T * const message = (LE_AUDIO_UNICAST_CIS_CONNECTED_T *) PanicUnlessMalloc(sizeof(*message));

    message->side = side;
    message->cis_id = cis_id;
    message->cis_dir = cis_dir;
    message->cis_handle = cis_handle;
    TaskList_MessageSendWithSize(leAudioMessages_GetMessageClients(), id, message, sizeof(*message));
}

#endif /* INCLUDE_LE_AUDIO_UNICAST */

bool LeAudioMessages_Init(Task init_task)
{
    UNUSED(init_task);
    TaskList_Initialise(leAudioMessages_GetMessageClients());
    return TRUE;
}

void LeAudioMessages_ClientRegister(Task task_to_register)
{
    TaskList_AddTask(leAudioMessages_GetMessageClients(), task_to_register);
}

void LeAudioMessages_ClientDeregister(Task task_to_register)
{
    TaskList_RemoveTask(leAudioMessages_GetMessageClients(), task_to_register);
}

static void LeAudioMessages_RegisterMessageGroup(Task task, message_group_t group)
{
    PanicFalse(group == LE_AUDIO_MESSAGE_GROUP);
    TaskList_AddTask(leAudioMessages_GetMessageClients(), task);
}

MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(LE_AUDIO, LeAudioMessages_RegisterMessageGroup, NULL);

#endif /* (defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)) */

