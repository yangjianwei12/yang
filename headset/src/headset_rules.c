/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       headset_rules.c
\brief      Rules for the headset applicaiton
*/

#include "headset_rules.h"
#include "rules_engine.h"
#include "headset_sm.h"

#include <logging.h>

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_ENUM(headset_rules_messages)

/*! \{
    Macros for diagnostic output that can be suppressed. */

#define HEADSET_RULE_LOG         DEBUG_LOG
/*! \} */

/* Forward declaration for use in RULE_ACTION_RUN_PARAM macro below */
static rule_action_t headsetRules_CopyRunParams(const void* param, size_t size_param);

/*! \brief Macro used by rules to return RUN action with parameters to return.

    Copies the parameters/data into the rules instance where the rules engine
    can use it when building the action message.
*/
#define RULE_ACTION_RUN_PARAM(x)   headsetRules_CopyRunParams(&(x), sizeof(x))

headset_rules_task_data_t headset_rules_task_data;

/*! \{
    Rule function prototypes, so we can build the rule tables below. */

DEFINE_RULE(headsetRuleConnectHandsetLinkLoss);
DEFINE_RULE(headsetRuleConnectHandsetUser);
DEFINE_RULE(headsetRuleConnectHandsetAuto);
DEFINE_RULE(headsetRuleDisconnectAllHandset);
DEFINE_RULE(headsetRuleDisconnectLruHandset);

/*! \} */

/*! \brief Headset Application rules deciding behaviour.
*/
const rule_entry_t headset_rules_set[] =
{
    RULE(HS_EVENT_AUTO_CON_HANDSET,         headsetRuleConnectHandsetAuto,                 HS_RULES_CON_HANDSET),
    RULE(HS_EVENT_LINK_LOSS,                headsetRuleConnectHandsetLinkLoss,             HS_RULES_CON_HANDSET),
    RULE(HS_EVENT_USER_CON_HANDSET,         headsetRuleConnectHandsetUser,                 HS_RULES_CON_HANDSET),
    RULE(HS_EVENT_DISCON_ALL_HANDSET,       headsetRuleDisconnectAllHandset,        HS_RULES_DISCON_ALL_HANDSET),
    RULE(HS_EVENT_DISCON_LRU_HANDSET,       headsetRuleDisconnectLruHandset,        HS_RULES_DISCON_LRU_HANDSET),
};

/*! Types of event that can initiate a connection rule decision. */
typedef enum
{
    /*! Auto connect to MRU device on power on. */
    rule_auto_connect = 1 << 0,
    /*! Link loss with handset. */
    rule_connect_linkloss = 1 << 1,
    /*! Topology user requests for connection. */
    rule_connect_user = 1 << 2,
} rule_connect_reason_t;

/*****************************************************************************
 * RULES FUNCTIONS
 *****************************************************************************/

static rule_action_t headsetRuleConnectHandset(rule_connect_reason_t reason)
{
    bdaddr handset_addr;
    const HS_RULES_CONNECT_HANDSET_T reconnect_link_loss = {.link_loss = TRUE};
    const HS_RULES_CONNECT_HANDSET_T reconnect_normal = {.link_loss = FALSE};

    HEADSET_RULE_LOG("headsetRuleConnectHandset, reason %u", reason);

    /* Ignore the rule if not allowed to run headset rules - Only allowed in STEREO_TOPOLOGY_STARTED_STATE */
    if (!headsetSmIsAllowedToRunHeadsetRules())
    {
        HEADSET_RULE_LOG("headsetRuleConnectHandset, ignore as headset rules not allowed to run");
        return rule_action_ignore;
    }

    /* Ignore the rule if no devices is PDL */
    if (!appDeviceGetHandsetBdAddr(&handset_addr))
    {
        HEADSET_RULE_LOG("headsetRuleConnectHandset, ignore as not paired with handset");
        return rule_action_ignore;
    }

    if (reason == rule_connect_linkloss)
    {
        HEADSET_RULE_LOG("headsetRuleConnectHandset, run as handset had a link loss");
        RULE_ACTION_RUN_PARAM(reconnect_link_loss);
    }
    else
    {
         HEADSET_RULE_LOG("headsetRuleConnectHandset, run as handset we were connected to before");
         RULE_ACTION_RUN_PARAM(reconnect_normal);
    }
    return rule_action_run_with_param;
}

static rule_action_t headsetRuleConnectHandsetAuto(void)
{
    HEADSET_RULE_LOG("headsetRuleConnectHandsetAuto");

    return headsetRuleConnectHandset(rule_auto_connect);
}

static rule_action_t headsetRuleConnectHandsetLinkLoss(void)
{
    HEADSET_RULE_LOG("headsetRuleConnectHandsetLinkLoss");

    return headsetRuleConnectHandset(rule_connect_linkloss);
}

static rule_action_t headsetRuleConnectHandsetUser(void)
{
    HEADSET_RULE_LOG("headsetRuleConnectHandsetUser");

    return headsetRuleConnectHandset(rule_connect_user);
}

static rule_action_t headsetRuleDisconnectAllHandset(void)
{
    HEADSET_RULE_LOG("headsetRuleDisconnectAllHandset");

    /* Ignore the rule if not allowed to run headset rules */
    if (!headsetSmIsAllowedToRunHeadsetRules())
    {
        HEADSET_RULE_LOG("headsetRuleDisconnectAllHandset, ignore as headset rules not allowed to run");
        return rule_action_ignore;
    }

    return rule_action_run;
}

static rule_action_t headsetRuleDisconnectLruHandset(void)
{
    HEADSET_RULE_LOG("headsetRuleDisconnectLruHandset");

    /* Ignore the rule if not allowed to run headset rules */
    if (!headsetSmIsAllowedToRunHeadsetRules())
    {
        HEADSET_RULE_LOG("headsetRuleDisconnectLruHandset, ignore as headset rules not allowed to run");
        return rule_action_ignore;
    }

    if(!HandsetService_IsAnyDeviceConnected())
    {
        HEADSET_RULE_LOG("headsetRuleDisconnectLruHandset, ignore as there is no BR/EDR connection");
        return rule_action_ignore;
    }

    HEADSET_RULE_LOG("headsetRuleDisconnectLruHandset, run");
    return rule_action_run;
}

/*****************************************************************************
 * END RULES FUNCTIONS
 *****************************************************************************/

/*! \brief Copy rule param data for the engine to put into action messages.
    \param param Pointer to data to copy.
    \param size_param Size of the data in bytes.
    \return rule_action_run_with_param to indicate the rule action message needs parameters.
 */
static rule_action_t headsetRules_CopyRunParams(const void* param, size_t size_param)
{
    headset_rules_task_data_t *headset_rules = HeadsetRulesGetTaskData();
    RulesEngine_CopyRunParams(headset_rules->rule_set, param, size_param);

    return rule_action_run_with_param;
}

/*! \brief Initialise the headset rules module. */
bool HeadsetRules_Init(Task result_task)
{
    UNUSED(result_task);

    headset_rules_task_data_t *headset_rules = HeadsetRulesGetTaskData();
    rule_set_init_params_t rule_params;

    memset(&rule_params, 0, sizeof(rule_params));
    rule_params.rules = headset_rules_set;
    rule_params.rules_count = ARRAY_DIM(headset_rules_set);
    rule_params.nop_message_id = HS_RULES_NOP;
    rule_params.event_task = headsetSmGetTask();
    headset_rules->rule_set = RulesEngine_CreateRuleSet(&rule_params);
    return TRUE;
}

rule_set_t HeadsetRules_GetRuleSet(void)
{
    headset_rules_task_data_t *headset_rules = HeadsetRulesGetTaskData();
    return headset_rules->rule_set;
}

void HeadsetRules_SetEvent(rule_events_t event_mask)
{
    headset_rules_task_data_t *headset_rules = HeadsetRulesGetTaskData();
    RulesEngine_SetEvent(headset_rules->rule_set, event_mask);
}

void HeadsetRules_ResetEvent(rule_events_t event)
{
    headset_rules_task_data_t *headset_rules = HeadsetRulesGetTaskData();
    RulesEngine_ResetEvent(headset_rules->rule_set, event);
}

rule_events_t HeadsetRules_GetEvents(void)
{
    headset_rules_task_data_t *headset_rules = HeadsetRulesGetTaskData();
    return RulesEngine_GetEvents(headset_rules->rule_set);
}

void HeadsetRules_SetRuleComplete(MessageId message)
{
    headset_rules_task_data_t *headset_rules = HeadsetRulesGetTaskData();
    RulesEngine_SetRuleComplete(headset_rules->rule_set, message);
}

void HeadsetRules_SetRuleWithEventComplete(MessageId message, rule_events_t event)
{
    headset_rules_task_data_t *headset_rules = HeadsetRulesGetTaskData();
    RulesEngine_SetRuleWithEventComplete(headset_rules->rule_set, message, event);
}
