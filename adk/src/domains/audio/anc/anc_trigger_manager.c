/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       anc_trigger_manager.c
\brief      ANC Trigger Manager: Handle various types of triggers (Wind, Noise, Quiet) and manage attack and release with priority.
*/


#ifdef ENABLE_ADAPTIVE_ANC

#include "anc_trigger_manager.h"
#include "aanc_quiet_mode.h"
#include "wind_detect.h"
#include "anc_noise_id.h"
#include "anc_config.h"
#include "anc_state_manager.h"
#include "kymera_anc_common.h"
#include <logging.h>

/***
    Trigger Manager Design

    Input: priority of the capability message received
    Output: Call the trigger function of the capability if none of the preceding capabilities are active
    Prerequisite data: 1) Functions to detect if a capability is active
                       2) Trigger functions for all capabilities
    When the env is Windy or Quiet mode, the noise Id indication will be ignored until the current mode persists.
    Once Windy or Quite mode is exited, the noise Id current category needs to be informed to the Noise Id capability such that it can take action if new category is detected.
    Windy and Quiet mode are mutually exclusive so only one should be active at a time.
***/

/* Based on priority. Index 0 is highest priority */
static bool (*detections[])(void) = {
        #ifdef ENABLE_WIND_DETECT
                    WindDetect_IsEnvWindy,
        #else
                    NULL,
        #endif
                    AancQuietMode_IsQuietModeDetectedOnBothPeer,
                    AncNoiseId_IsModeChangeRequested
                  };

static void (*triggers[])(void) = {
                          NULL, /*WND being highest priority handled in Wind detect module itself*/
                          AancQuietMode_HandleQuietModeEnable,
                          AncNoiseId_TriggerModeChange
                        };

static anc_trigger_priority_t AncTriggerManager_GetPriorityFromTriggerType(anc_trigger_type_t trigger_type)
{
    anc_trigger_priority_t priority=ANC_TRIGGER_PRIORITY_LOW;

    switch(trigger_type)
    {
        case ANC_TRIGGER_TYPE_WIND_ATTACK:
            priority = ANC_TRIGGER_PRIORITY_HIGHEST;
            break;

        case ANC_TRIGGER_TYPE_QUIET_MODE_ENABLE:
            priority = ANC_TRIGGER_PRIORITY_MEDIUM;
            break;

        case ANC_TRIGGER_TYPE_NOISE_ID_CAT_CHANGE:
            priority = ANC_TRIGGER_PRIORITY_LOW;
            break;

        default:
            break;
    }

    DEBUG_LOG("AncTriggerManager_GetPriorityFromTriggerType priority %d", priority);
    return priority;
}

void AncTriggerManager_Invoke(anc_trigger_type_t trigger_type)
{
    uint8 receivedPriority = (uint8) AncTriggerManager_GetPriorityFromTriggerType(trigger_type);
    DEBUG_LOG("AncTriggerManager_Invoke");

    for(uint8 priority = 1; priority < receivedPriority; ++priority)
    {
        if (((detections[priority - 1])!=NULL) && ((*detections[priority - 1])()))
        {
            DEBUG_LOG("AncTriggerManager_Invoke returning for priority %d", priority);
            return;
        }
    }

    DEBUG_LOG("AncTriggerManager_Invoke for receivedPriority %d", receivedPriority);

    if (triggers[receivedPriority - 1]!=NULL)
    {
        (*triggers[receivedPriority - 1])();
    }
}

void AncTriggerManager_ActionPostRelease(void)
{
    /*Set Noise ID category based on current mode*/
    KymeraAncCommon_SetCategoryBasedOnCurrentMode();
}

#endif /*ENABLE_ADAPTIVE_ANC*/

