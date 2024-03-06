/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       headset_rules.h
\brief      Rules for the headset applicaiton
*/

#ifndef HEADSET_RULES_H_
#define HEADSET_RULES_H_

#include "headset_rule_events.h"

#include <rules_engine.h>
#include <handset_service_sm.h>

#include <message.h>

/*! \brief Enum of the different Headset rule message types we may have. */
enum headset_rules_messages
{
    /*! Start handset connection. */
    HS_RULES_CON_HANDSET = CONN_RULES_MESSAGE_BASE,
    /*! Start least recently used handset disconnection. */
    HS_RULES_DISCON_LRU_HANDSET,
    /*! Start disconnecting all the connected handset */
    HS_RULES_DISCON_ALL_HANDSET,
    HS_RULES_NOP
};

typedef struct
{
    bool link_loss;
} HS_RULES_CONNECT_HANDSET_T;

typedef struct
{
    uint32 profiles;
} HS_RULES_DISCONNECT_HANDSET_PROFILES_T;

/*! \brief Headset rules task data. */
typedef struct
{
    rule_set_t      rule_set;
} headset_rules_task_data_t;

/*! Make the HeadsetRules instance visible throughout the component */
extern headset_rules_task_data_t headset_rules_task_data;

/*! Get pointer to the connection rules task data structure. */
#define HeadsetRulesGetTaskData()           (&headset_rules_task_data)

typedef struct
{
    bool link_loss;
} CONNECT_HANDSET_PARAMS_T;

/*! \brief Initialise the connection rules module. */
bool HeadsetRules_Init(Task init_task);

/*! \brief Get handle on the rule set, in order to directly set/clear events.
    \return rule_set_t The rule set.
 */
rule_set_t HeadsetRules_GetRuleSet(void);

/*! \brief Set an event or events
    \param[in] event_mask Events to set that will trigger rules to run
    This function is called to set an event or events that will cause the relevant
    rules in the rules table to run.  Any actions generated will be sent as message
    to the client_task
*/
void HeadsetRules_SetEvent(rule_events_t event_mask);

/*! \brief Reset/clear an event or events
    \param[in] event Events to clear
    This function is called to clear an event or set of events that was previously
    set. Clear event will reset any rule that was run for event.
*/
void HeadsetRules_ResetEvent(rule_events_t event);

/*! \brief Get set of active events
    \return The set of active events.
*/
rule_events_t HeadsetRules_GetEvents(void);

/*! \brief Mark rules as complete from messaage ID
    \param[in] message Message ID that rule(s) generated
    This function is called to mark rules as completed, the message parameter
    is used to determine which rules can be marked as completed.
*/
void HeadsetRules_SetRuleComplete(MessageId message);

/*! \brief Mark rules as complete from message ID and set of events
    \param[in] message Message ID that rule(s) generated
    \param[in] event Event or set of events that trigger the rule(s)
    This function is called to mark rules as completed, the message and event parameter
    is used to determine which rules can be marked as completed.
*/
void HeadsetRules_SetRuleWithEventComplete(MessageId message, rule_events_t event);

#endif /* HEADSET_RULES_H_ */

