/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       dfu_rules.h
    \addtogroup dfu
    \brief      Rules for the DFU process
    @{
*/

#ifndef DFU_RULES_H_
#define DFU_RULES_H_

#include "dfu_rule_events.h"

#include <rules_engine.h>
#include <domain_message.h>

#include <message.h>

enum dfu_rules_messages_t
{
    /*! Start DFU validation process. */
    DFU_RULES_START_VALIDATION = CONN_RULES_MESSAGE_BASE,
    /*! Start Peer Validation process*/
    DFU_RULES_KICK_PEER_VALIDATION,
    /*! Notify validation complete*/
    DFU_RULES_VALIDATION_COMPLETE,
    /*! Send Data Transfer Complete info to Peer */
    DFU_RULES_RELAY_TRANSFER_COMPLETE,
    /*! Relay Resume Point info to Peer */
    DFU_RULES_RELAY_RESUME_POINT_INFO,
    /*! Revert Upgrade during post reboot*/
    DFU_RULES_REVERT_UPGRADE,
    /*!Start Peer DFU*/
    DFU_RULES_START_PEER_DFU,
    /*!Start Reboot process */
    DFU_RULES_START_REBOOT,

    DFU_RULES_NOP
};

/*! \brief DFU process rules task data. */
typedef struct
{
    rule_set_t      rule_set;
} dfu_rules_task_data_t;

/*! Make the DFU Rules instance visible throughout the component */
extern dfu_rules_task_data_t dfu_rules_task_data;

/*! Get pointer to the dfu rules task data structure. */
#define DfuRulesGetTaskData()           (&dfu_rules_task_data)


/*! \brief Initialise the Dfu rules module. */
bool DfuRules_Init(Task init_task);

/*! \brief Get handle on the rule set, in order to directly set/clear events.
    \return rule_set_t The rule set.
 */
rule_set_t DfuRules_GetRuleSet(void);

/*! \brief Set an event or events
    \param[in] event Events to set that will trigger rules to run
    This function is called to set an event or events that will cause the relevant
    rules in the rules table to run.  Any actions generated will be sent as message
    to the client_task
*/
void DfuRules_SetEvent(rule_events_t event_mask);

/*! \brief Reset/clear an event or events
    \param[in] event Events to clear
    This function is called to clear an event or set of events that was previously
    set. Clear event will reset any rule that was run for event.
*/
void DfuRules_ResetEvent(rule_events_t event);

/*! \brief Get set of active events
    \return The set of active events.
*/
rule_events_t DfuRules_GetEvents(void);

/*! \brief Mark rules as complete from messaage ID
    \param[in] message Message ID that rule(s) generated
    This function is called to mark rules as completed, the message parameter
    is used to determine which rules can be marked as completed.
*/
void DfuRules_SetRuleComplete(MessageId message);

/*! \brief Mark rules as complete from message ID and set of events
    \param[in] message Message ID that rule(s) generated
    \param[in] event Event or set of events that trigger the rule(s)
    This function is called to mark rules as completed, the message and event parameter
    is used to determine which rules can be marked as completed.
*/
void DfuRules_SetRuleWithEventComplete(MessageId message, rule_events_t event);

#endif /* DFU_RULES_H_ */

/*! @} */