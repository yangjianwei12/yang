/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief      LE Broadcast Manager Link Policy
*/

#if defined(INCLUDE_LE_AUDIO_BROADCAST) && defined(INCLUDE_LEA_LINK_POLICY)

#include "le_broadcast_manager_link_policy.h"

#include "le_broadcast_manager.h"

#include "link_policy.h"
#include "logging.h"

/*! \brief Number of LE intervals in 7.5ms. 7.5ms is a common multiple used for Bluetooth connections */
#define BROADCAST_SOURCE_LE_INTERVAL_7p5MS  6
/*! \brief Number of Bluetooth slots in 7.5ms. 7.5ms is a common multiple used for Bluetooth connections */
#define BROADCAST_SOURCE_BREDR_INTERVAL_7p5MS 12


/* Suggested requirement for non colocated streaming is to adjust LE link parameters to
   be a multiple of 7.5ms for general compatibility. And value selected so that it
   is not clashing with the non-colocated ISO interval */
static bool LeBroadcastSource_CheckLeParams(const tp_bdaddr *tpaddr,
                                            uint16 *min_interval,
                                            uint16 *max_interval)
{
    uint16 highest = *max_interval;
    uint16 iso_to_skip;

    UNUSED(tpaddr);

    if (LeBroadcastSource_IsActiveSourceNonColocated())
    {
        iso_to_skip = LeBroadcastSource_GetActiveSourceIsoInterval();

        /* Start with highest value that meets the 7.5ms criteria */
        uint16 value_being_checked = highest - (highest % BROADCAST_SOURCE_LE_INTERVAL_7p5MS);

        DEBUG_LOG("LeBroadcastSource_CheckLeParams Range:%d-%d First check %d against interval of %d",
                  *min_interval, *max_interval, value_being_checked, iso_to_skip);

        if (iso_to_skip != 0)
        {
            while (value_being_checked >= *min_interval)
            {
                /* Determine clashes by modulo of values against each other */
                if (value_being_checked != 0)
                {
                    if ((value_being_checked % iso_to_skip) && (iso_to_skip % value_being_checked))
                    {
                        *min_interval = *max_interval = value_being_checked;
                        return TRUE;
                    }
                }
                value_being_checked -= BROADCAST_SOURCE_LE_INTERVAL_7p5MS;
            }
        }
    }

    return FALSE;
}

static bool LeBroadcastSource_CheckBredrParams(const tp_bdaddr *tpaddr,
                                               uint16 *min_interval,
                                               uint16 *max_interval)
{
    uint16 iso_interval;
    uint16 value_being_checked = *max_interval - (*max_interval % BROADCAST_SOURCE_BREDR_INTERVAL_7p5MS);

    UNUSED(tpaddr);

    if (LeBroadcastSource_IsActiveSourceNonColocated())
    {
        iso_interval = LeBroadcastSource_GetActiveSourceIsoInterval();
        uint16 iso_interval_slots = iso_interval * 2;

        DEBUG_LOG("LeBroadcastSource_CheckBredrParams Range:%d-%d First check %d against interval of %d",
                  *min_interval, *max_interval, value_being_checked, iso_interval_slots);

        while (value_being_checked >= *min_interval)
        {
            /* Determine clashes by modulo of values against each other */
            if (   (value_being_checked % iso_interval_slots) 
                && (iso_interval_slots % value_being_checked))
            {
                *max_interval = *min_interval = value_being_checked;
                return TRUE;
            }
            value_being_checked -= BROADCAST_SOURCE_BREDR_INTERVAL_7p5MS;
        }
    }

    return FALSE;
}

static link_policy_parameter_callbacks_t LeBroadcastSource_LinkParamCallbacks = 
                { .LeParams = LeBroadcastSource_CheckLeParams,
                  .BredrParams = LeBroadcastSource_CheckBredrParams };
                  

void LeBroadcastManager_LinkPolicyInit(void)
{
    LinkPolicy_SetParameterCallbacks(&LeBroadcastSource_LinkParamCallbacks);
}

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST) && defined(INCLUDE_LEA_LINK_POLICY) */
