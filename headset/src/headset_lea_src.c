/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       headset_lea_src.c
\brief      Speaker LE Source audio interface functionality implementation.

*/

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

#include "unexpected_message.h"
#include "audio_sources.h"
#include <kymera_adaptation_audio_protected.h>

#include "headset_lea_src.h"
#include "headset_lea_src_config.h"
#include "headset_sm.h"
#include "le_audio_client.h"
#include "le_audio_client_context.h"


#include <device_types.h>
#include <ui.h>
#include <panic.h>
#include <local_addr.h>
#include <stdio.h>

TaskData headsetLeaSrc_taskData;

/*! Task handler to receiver the LE Audio client messages */
static void headsetLeaSrc_HandleMessage(Task task, MessageId id, Message message);

static void headsetLeaSrc_HandleStreamConnected(LE_AUDIO_CLIENT_STREAM_START_IND_T *msg)
{
    if(msg->status != LE_AUDIO_CLIENT_STATUS_SUCCESS)
    {
        DEBUG_LOG_ERROR("headsetLeaSrc_HandleStreamConnected, stream start failed");
        return;
    }
    Kymera_EnableLeaAudioBroadcasting(TRUE);
}

static void headsetLeaSrc_HandleStreamDisconnected(LE_AUDIO_CLIENT_STREAM_STOP_IND_T *msg)
{
    DEBUG_LOG_FN_ENTRY("headsetLeaSrc_HandleStreamDisconnected");

    UNUSED(msg);
}

static void headsetLeaSrc_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    DEBUG_LOG_FN_ENTRY("headsetLeaSrc_HandleMessage %d", id);

    switch (id)
    {
        case LE_AUDIO_CLIENT_STREAM_START_IND:
            headsetLeaSrc_HandleStreamConnected((LE_AUDIO_CLIENT_STREAM_START_IND_T *) message);
            break;

        case LE_AUDIO_CLIENT_STREAM_STOP_IND:
            headsetLeaSrc_HandleStreamDisconnected((LE_AUDIO_CLIENT_STREAM_STOP_IND_T *) message);
            break;

        default:
            UnexpectedMessage_HandleMessage(id);
            break;

    }
}

/* Used to start broadcast */
void HeadsetLeaSrc_AudioStart(void)
{
    /* Group handle is not required for Broadcasting, just context media is required */
    LeAudioClient_StartStreaming(INVALID_GROUP_HANDLE, CAP_CLIENT_CONTEXT_TYPE_MEDIA);
}

/* Used to stop broadcast */
void HeadsetLeaSrc_AudioStop(void)
{
    Kymera_EnableLeaAudioBroadcasting(FALSE);
    /* Both group handle and removing config is not required for stopping broadcast */
    LeAudioClient_StopStreaming(INVALID_GROUP_HANDLE, FALSE);
}

bool HeadsetLeaSrc_Init(void)
{
    DEBUG_LOG_FN_ENTRY("Headset_LeaInit");

    headsetLeaSrc_taskData.handler = headsetLeaSrc_HandleMessage;
    LeAudioClient_ClientRegister(&headsetLeaSrc_taskData);
    HeadsetLeaSrcConfig_Init();
    LeAudioClient_SetMode(LE_AUDIO_CLIENT_MODE_BROADCAST);
    HeadsetLeaSrcConfig_SetPbpBroadcastmode(FALSE);
    HeadsetLeaSrcConfig_SetLc3CodecParams();

    return TRUE;
}

#endif  /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
