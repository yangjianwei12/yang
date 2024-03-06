/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       dfu_rules.c
    \ingroup    dfu
    \brief      Rules for the DFU process
*/
#ifdef INCLUDE_DFU

#include "dfu_rules.h"
#include "rules_engine.h"
#include "dfu.h"
#include "dfu_peer.h"
#include "bt_device.h"
#include <logging.h>

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_ENUM(dfu_rules_messages_t)

/*! \{
    Macros for diagnostic output that can be suppressed. */

#define DFU_RULE_LOG         DEBUG_LOG_INFO
/*! \} */

/* Currently there is no rule set which runs with params. Hence this code is gaurded now.
 * Will be ungaurded once there is any usage
 */
#ifdef RUN_WITH_PARAM

/* Forward declaration for use in RULE_ACTION_RUN_PARAM macro below
*/
static rule_action_t dfuRules_CopyRunParams(const void* param, size_t size_param);

/*! \brief Macro used by rules to return RUN action with parameters to return.

    Copies the parameters/data into the rules instance where the rules engine
    can use it when building the action message.
*/
#define RULE_ACTION_RUN_PARAM(x)   dfuRules_CopyRunParams(&(x), sizeof(x))
#endif

dfu_rules_task_data_t dfu_rules_task_data;

/*! \{
    Rule function prototypes, so we can build the rule tables below. */

DEFINE_RULE(dfuRuleValidationComplete);
DEFINE_RULE(dfuRuleStartValidation);
DEFINE_RULE(dfuRuleRelayDataTransferComplete);
DEFINE_RULE(dfuRuleRelayResumePointInfo);
DEFINE_RULE(dfuRuleSyncPeerPostReboot);
DEFINE_RULE(dfuRuleStartPeerDFU);
DEFINE_RULE(dfuRuleStartReboot);

/*! \} */

/*! \brief DFU process rules deciding behaviour.
*/
const rule_entry_t dfu_rules_set[] =
{
    RULE(DFU_EVENT_VALIDATION_COMPLETE,            dfuRuleValidationComplete,       DFU_RULES_VALIDATION_COMPLETE),
    RULE(DFU_EVENT_DATA_TRANSFER_COMPLETE,         dfuRuleStartValidation,          DFU_RULES_START_VALIDATION),
    RULE(DFU_EVENT_DATA_TRANSFER_COMPLETE,         dfuRuleRelayDataTransferComplete,DFU_RULES_RELAY_TRANSFER_COMPLETE),
    RULE(DFU_EVENT_PEER_END_DATA_TRANSFER,         dfuRuleStartValidation,          DFU_RULES_START_VALIDATION),
    RULE(DFU_EVENT_UPGRADE_PEER_VLDTN_COMPLETE,    dfuRuleValidationComplete,       DFU_RULES_VALIDATION_COMPLETE),
    RULE(DFU_EVENT_RESUME_POINT_SYNC,              dfuRuleRelayResumePointInfo,     DFU_RULES_RELAY_RESUME_POINT_INFO),
    RULE(DFU_EVENT_RESUME_POINT_SYNC,              dfuRuleSyncPeerPostReboot,       DFU_RULES_REVERT_UPGRADE),
    RULE(DFU_EVENT_RESUME_POINT_SYNC_COMPLETED,    dfuRuleStartValidation,          DFU_RULES_START_VALIDATION),
    RULE(DFU_EVENT_RESUME_POINT_SYNC_COMPLETED,    dfuRuleValidationComplete,       DFU_RULES_VALIDATION_COMPLETE),
    RULE(DFU_EVENT_UPGRADE_START_DATA_IND,         dfuRuleStartPeerDFU,             DFU_RULES_START_PEER_DFU),
    RULE(DFU_EVENT_PEER_SIG_CONNECT_IND,           dfuRuleStartPeerDFU,             DFU_RULES_START_PEER_DFU),
    /* This event rule set is for starting the peer dfu during handover scenario once the dfu resumes on new primary. */
    RULE(DFU_EVENT_UPGRADE_RESUME,                 dfuRuleStartPeerDFU,             DFU_RULES_START_PEER_DFU),
    RULE(DFU_EVENT_UPGRADE_RESUME,                 dfuRuleValidationComplete,       DFU_RULES_VALIDATION_COMPLETE),
    RULE(DFU_EVENT_UPGRADE_RESUME,                 dfuRuleStartValidation,          DFU_RULES_START_VALIDATION),
    RULE(DFU_EVENT_UPGRADE_PEER_PROCESS_COMPLETE,  dfuRuleStartReboot,              DFU_RULES_START_REBOOT),
    RULE(DFU_EVENT_UPGRADE_TRANSFER_COMPLETE_RES,  dfuRuleStartReboot,              DFU_RULES_START_REBOOT),
};


/*****************************************************************************
 * RULES FUNCTIONS
 *****************************************************************************/

static rule_action_t dfuRuleStartValidation(void)
{
    if (UpgradeIsResumePoinPreValidate() && UpgradeIsWaitForValidation()
#ifdef INCLUDE_DFU_PEER
        && UPGRADE_PEER_IS_CONNECTED && UpgradePeerIsResumePoinPreValidate()
#endif
        )
    {
        DFU_RULE_LOG("dfuRuleStartValidation, allowed to run");
        return rule_action_run;
    }

    DFU_RULE_LOG("dfuRuleStartValidation, ignore as dfu rules not allowed to run");
    return rule_action_ignore;
}

static rule_action_t dfuRuleValidationComplete(void)
{
    if(UpgradeIsValidationComplete()
#ifdef INCLUDE_DFU_PEER
       && UPGRADE_PEER_IS_CONNECTED && UpgradePeerIsVldtnComplete()
#endif
    )
    {
        DFU_RULE_LOG("dfuRuleValidationComplete, allowed to run");
        return rule_action_run;
    }

    DFU_RULE_LOG("dfuRuleValidationComplete, ignore as dfu rules not allowed to run");
    return rule_action_ignore;

}

static rule_action_t dfuRuleRelayDataTransferComplete(void)
{
#ifdef INCLUDE_DFU_PEER
    if(BtDevice_IsMyAddressPrimary() && UPGRADE_PEER_IS_CONNECTED)
    {
        DFU_RULE_LOG("dfuRuleRelayDataTransferComplete, allowed to run");
        return rule_action_run;
    }
#endif
    DFU_RULE_LOG("dfuRuleRelayDataTransferComplete, ignore as dfu rules not allowed to run");
    return rule_action_ignore;
}

static rule_action_t dfuRuleRelayResumePointInfo(void)
{
#ifdef INCLUDE_DFU_PEER
    if(UPGRADE_PEER_IS_CONNECTED)
    {
        return rule_action_run;
    }
#endif
    DFU_RULE_LOG("dfuRuleRelayResumePointInfo, ignore as dfu rules not allowed to run");
    return rule_action_ignore;
}

static rule_action_t dfuRuleSyncPeerPostReboot(void)
{
#ifdef INCLUDE_DFU_PEER
    if(UPGRADE_PEER_IS_CONNECTED && UpgradeIsResumePointPostReboot() 
        && !UpgradePeerIsResumePointPostReboot())
    {
        DFU_RULE_LOG("dfuRuleSyncPeerPostReboot, allowed to run");
        return rule_action_run;
    }
#endif
    DFU_RULE_LOG("dfuRuleSyncPeerPostReboot, ignore as dfu rules not allowed to run");
    return rule_action_ignore;
}

static rule_action_t dfuRuleStartPeerDFU(void)
{
#ifdef INCLUDE_DFU_PEER
    bool is_primary = BtDevice_IsMyAddressPrimary();
    if(Dfu_IsUpgradeInProgress() && !UPGRADE_PEER_IS_STARTED && is_primary
		&& Dfu_GetPeerSigL2capStatus() == dfu_peer_sig_l2cap_connected)
    {
        DFU_RULE_LOG("dfuRuleStartPeerDFU, allowed to run");
        return rule_action_run;
    }
#endif
    DFU_RULE_LOG("dfuRuleStartPeerDFU, ignore as dfu rules not allowed to run");
    return rule_action_ignore;
}

static rule_action_t dfuRuleStartReboot(void)
{
#ifdef INCLUDE_DFU_PEER
    bool is_primary = BtDevice_IsMyAddressPrimary();
#endif

    /* In case of earbud, for Secondary EB, just check if UPGRADE_HOST_TRANSFER_COMPLETE_RES is received */
    if (UpgradeIsTransferCompleteResponseReceived()
#ifdef INCLUDE_DFU_PEER
        && (!is_primary || (UPGRADE_PEER_IS_CONNECTED && UPGRADE_PEER_IS_PRIMARY && UpgradePeerIsStateProcessComplete()))
#endif
        )
    {
        DFU_RULE_LOG("dfuRuleStartReboot, allowed to run");
        return rule_action_run;
    }

    DFU_RULE_LOG("dfuRuleStartReboot, ignore as dfu rules not allowed to run");
    return rule_action_ignore;
}


/* Currently there is no rule set which runs with params. Hence this code is gaurded now.
 * Will be ungaurded once there is any usage
 */
#ifdef RUN_WITH_PARAM
/*****************************************************************************
 * END RULES FUNCTIONS
 *****************************************************************************/

/*! \brief Copy rule param data for the engine to put into action messages.
    \param param Pointer to data to copy.
    \param size_param Size of the data in bytes.
    \return rule_action_run_with_param to indicate the rule action message needs parameters.
*/ 
static rule_action_t dfuRules_CopyRunParams(const void* param, size_t size_param)
{
    dfu_rules_task_data_t *dfu_rules = DfuRulesGetTaskData();
    RulesEngine_CopyRunParams(dfu_rules->rule_set, param, size_param);

    return rule_action_run_with_param;
}
#endif


/*! \brief Initialise the dfu rules module. */
bool DfuRules_Init(Task result_task)
{
    UNUSED(result_task);

    dfu_rules_task_data_t *dfu_rules = DfuRulesGetTaskData();
    rule_set_init_params_t rule_params;

    memset(&rule_params, 0, sizeof(rule_params));
    rule_params.rules = dfu_rules_set;
    rule_params.rules_count = ARRAY_DIM(dfu_rules_set);
    rule_params.nop_message_id = DFU_RULES_NOP;
    rule_params.event_task = result_task;
    dfu_rules->rule_set = RulesEngine_CreateRuleSet(&rule_params);
    return TRUE;
}

rule_set_t DfuRules_GetRuleSet(void)
{
    dfu_rules_task_data_t *dfu_rules = DfuRulesGetTaskData();
    return dfu_rules->rule_set;
}

void DfuRules_SetEvent(rule_events_t event_mask)
{
    dfu_rules_task_data_t *dfu_rules = DfuRulesGetTaskData();
    RulesEngine_SetEvent(dfu_rules->rule_set, event_mask);
}

void DfuRules_ResetEvent(rule_events_t event)
{
    dfu_rules_task_data_t *dfu_rules = DfuRulesGetTaskData();
    RulesEngine_ResetEvent(dfu_rules->rule_set, event);
}

rule_events_t DfuRules_GetEvents(void)
{
    dfu_rules_task_data_t *dfu_rules = DfuRulesGetTaskData();
    return RulesEngine_GetEvents(dfu_rules->rule_set);
}

void DfuRules_SetRuleComplete(MessageId message)
{
    dfu_rules_task_data_t *dfu_rules = DfuRulesGetTaskData();
    RulesEngine_SetRuleComplete(dfu_rules->rule_set, message);
}

void DfuRules_SetRuleWithEventComplete(MessageId message, rule_events_t event)
{
    dfu_rules_task_data_t *dfu_rules = DfuRulesGetTaskData();
    RulesEngine_SetRuleWithEventComplete(dfu_rules->rule_set, message, event);
}

#endif /*INCLUDE_DFU */
