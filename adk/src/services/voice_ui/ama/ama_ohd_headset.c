/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    ama
    \brief      Implementation for AMA On-Head/In-Ear Detection for earbuds.
*/

#if defined(INCLUDE_AMA) && !defined(INCLUDE_TWS)

#include <logging.h>
#include "headset_phy_state.h"
#include "ama_audio.h"
#include "ama_voice_ui_handle.h"
#include "ama_ohd.h"

static void ama_OhdHeadsetMessageHandler(Task task, MessageId id, Message message);
static TaskData ama_ohd_task = {ama_OhdHeadsetMessageHandler};
static bool headset_on_head = FALSE;

static void ama_OhdHeadsetHandleStateChangeEvent(const HEADSET_PHY_STATE_CHANGED_IND_T* message)
{
    DEBUG_LOG("ama_OhdHeadsetHandleStateChangeEvent: new_state enum:headsetPhyState:%u " 
            "event enum:headset_phy_state_event:%u", message->new_state, message->event);

    if (message->new_state == HEADSET_PHY_STATE_ON_HEAD)
    {
        headset_on_head = TRUE;
        if (Ama_IsAmaCurrentSelectedAssistant())
        {
            AmaAudio_StartWakeWordDetection();
        }
    }
    else
    {
        headset_on_head = FALSE;
        if (Ama_IsAmaCurrentSelectedAssistant())
        {
            AmaAudio_StopWakeWordDetection();
        }
    }
}

static void ama_OhdHeadsetMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
        case HEADSET_PHY_STATE_CHANGED_IND:
            ama_OhdHeadsetHandleStateChangeEvent((const HEADSET_PHY_STATE_CHANGED_IND_T *)message);
            break;

        default:
            break;
    }
}

bool AmaOhd_IsAccessoryInEarOrOnHead(void)
{
    return headset_on_head;
}

void AmaOhd_Init(void)
{
    if(appHeadsetPhyStateIsOnHeadDetectionSupported())
    {
        DEBUG_LOG("AmaOhd_Init: registering with headset phy state");
        appHeadsetPhyStateRegisterClient(&ama_ohd_task);
    }
    else
    {
        headset_on_head = TRUE;
    }
}

#endif /* INCLUDE_AMA && INCLUDE_TWS */
