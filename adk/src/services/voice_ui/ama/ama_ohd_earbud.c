/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                            All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    ama
    \brief      Implementation for AMA On-Head/In-Ear Detection for earbuds.
*/

#if defined(INCLUDE_AMA) && defined(INCLUDE_TWS)

#include <logging.h>
#include "state_proxy.h"
#include "ama_audio.h"
#include "ama_voice_ui_handle.h"
#include "ama_ohd.h"

static void ama_OhdEarbudMessageHandler(Task task, MessageId id, Message message);
static TaskData ama_ohd_task = {ama_OhdEarbudMessageHandler};

static void ama_OhdEarbudHandleStateProxyEvent(const STATE_PROXY_EVENT_T* sp_event)
{
    DEBUG_LOG("ama_OhdEarbudHandleStateProxyEvent source: enum:state_proxy_source:%u"
              " type: enum:state_proxy_event_type:%u", sp_event->source, sp_event->type);
    switch(sp_event->type)
    {
        case state_proxy_event_type_phystate:
            DEBUG_LOG("ama_OhdEarbudHandleStateProxyEvent new_state: enum:phyState:%u"
                      " phy_state_event enum:phy_state_event:%u",
                      sp_event->event.phystate.new_state, sp_event->event.phystate.event);

            if (Ama_IsAmaCurrentSelectedAssistant())
            {
                if (StateProxy_IsInEar())
                {
                    AmaAudio_StartWakeWordDetection();
                }
                else
                {
                    AmaAudio_StopWakeWordDetection();
                }
            }
            break;

        default:
            break;
    }
}

static void ama_OhdEarbudMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
        case STATE_PROXY_EVENT:
            ama_OhdEarbudHandleStateProxyEvent((const STATE_PROXY_EVENT_T *)message);
            break;

        default:
            break;
    }
}

bool AmaOhd_IsAccessoryInEarOrOnHead(void)
{
    return StateProxy_IsInEar();
}

void AmaOhd_Init(void)
{
    StateProxy_EventRegisterClient(&ama_ohd_task, state_proxy_event_type_phystate);
}

#endif /* INCLUDE_AMA && INCLUDE_TWS */
