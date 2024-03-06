/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#include "stereo_topology_rules.h"
#include "stereo_topology_goals.h"
#include "stereo_topology_client_msgs.h"

#include <bt_device.h>
#include <device_properties.h>
#include <device_list.h>
#include <rules_engine.h>
#include <connection_manager.h>
#include <handset_service_sm.h>
#include <av.h>
#include <multidevice.h>

#include <logging.h>

#pragma unitsuppress Unused

/*! \{
    Macros for diagnostic output that can be suppressed. */

#define STEREOTOP_RULE_LOG         DEBUG_LOG
/*! \} */

/* Forward declaration for use in RULE_ACTION_RUN_PARAM macro below */
static rule_action_t stereoTopologyRules_CopyRunParams(const void* param, size_t size_param);

/*! \brief Macro used by rules to return RUN action with parameters to return.
 
    Copies the parameters/data into the rules instance where the rules engine 
    can use it when building the action message.
*/
#define RULE_ACTION_RUN_PARAM(x)   stereoTopologyRules_CopyRunParams(&(x), sizeof(x))

/*! Get pointer to the connection rules task data structure. */
#define StereoTopologyRulesGetTaskData()  (&stereo_topology_rules_task_data)

stereo_topology_rules_task_data_t stereo_topology_rules_task_data;

/*! \{
    Rule function prototypes, so we can build the rule tables below. */

DEFINE_RULE(ruleStereoTopEnableConnectableHandset);
DEFINE_RULE(ruleStereoTopAllowHandsetConnect);
DEFINE_RULE(ruleStereoTopAllowLEConnection);
DEFINE_RULE(ruleStereoTopStop);
DEFINE_RULE(ruleStereoTopPeerPair);
DEFINE_RULE(ruleStereoTopPeerFindRole);
DEFINE_RULE(ruleStereoTopPrimaryConnPeer);
DEFINE_RULE(ruleStereoTopSecConnPeer);
DEFINE_RULE(ruleStereoTopEnableStereoStandalone);

/*! \} */

/*! \brief STEREO Topology rules deciding behaviour.
*/
const rule_entry_t stereotop_rules_set[] =
{
    /*! When we are shutting down, disconnect everything. */
    RULE(STEREOTOP_RULE_EVENT_STOP,                       ruleStereoTopStop,                     STEREOTOP_GOAL_SYSTEM_STOP),
    /* Upon start of day of topology, Allow LE connection, make the handset connectable and connect handset if PDL is not empty */
    RULE(STEREOTOP_RULE_EVENT_START,                      ruleStereoTopAllowLEConnection,        STEREOTOP_GOAL_ALLOW_LE_CONNECTION),
    RULE(STEREOTOP_RULE_EVENT_START,                      ruleStereoTopEnableConnectableHandset, STEREOTOP_GOAL_CONNECTABLE_HANDSET),
    RULE(STEREOTOP_RULE_EVENT_START,                      ruleStereoTopAllowHandsetConnect,      STEREOTOP_GOAL_ALLOW_HANDSET_CONNECT),
    /* TWM rules for speaker supporting TWM configuration */
    RULE(STEREOTOP_RULE_EVENT_PEER_PAIR,                  ruleStereoTopPeerPair,                 STEREOTOP_GOAL_PEER_PAIR),
    RULE(STEREOTOP_RULE_EVENT_PEER_FIND_ROLE,             ruleStereoTopPeerFindRole,             STEREOTOP_GOAL_PEER_FIND_ROLE),
    RULE(STEREOTOP_RULE_EVENT_PRIMARY_CONN_PEER,          ruleStereoTopPrimaryConnPeer,          STEREOTOP_GOAL_PRIMARY_CONNECT_PEER),
    RULE(STEREOTOP_RULE_EVENT_SECONDARY_CONN_PEER,        ruleStereoTopSecConnPeer,              STEREOTOP_GOAL_SECONDARY_CONNECT_PEER),
    RULE(STEREOTOP_RULE_EVENT_ENABLE_STEREO_STANDALONE,   ruleStereoTopEnableStereoStandalone,   STEREOTOP_GOAL_ENABLE_STEREO_STANDALONE),
};

/*! Types of event that can initiate a connection rule decision. */
typedef enum
{
    /*! Auto connect to MRU device on stereo power on. */
    rule_auto_connect = 1 << 0,
    /*! Link loss with handset. */
    rule_connect_linkloss = 1 << 1,
    /*! Topology user requests for connection. */
    rule_connect_user = 1 << 2,
} rule_connect_reason_t;


/*****************************************************************************
 * RULES FUNCTIONS
 *****************************************************************************/
static rule_action_t ruleStereoTopEnableStereoStandalone(void)
{
    /* blindly allow */
    return rule_action_run;
}

static rule_action_t ruleStereoTopEnableConnectableHandset(void)
{
    bdaddr handset_addr;
    const STEREOTOP_GOAL_CONNECTABLE_HANDSET_T enable_connectable = {.enable = TRUE};

    /* Ignore the rule if no devices is PDL */
    if (!appDeviceGetHandsetBdAddr(&handset_addr))
    {
        STEREOTOP_RULE_LOG("ruleStereoTopEnableConnectableHandset, ignore as not paired with handset");
        return rule_action_ignore;
    }

    /* Ignore the rule if already connected with handset */
    if (ConManagerIsConnected(&handset_addr))
    {
        STEREOTOP_RULE_LOG("ruleStereoTopEnableConnectableHandset, ignore as connected to handset");
        return rule_action_ignore;
    }

    /* Ignore the rule if we are in shutdown mode */
    if(StereoTopologyGetTaskData()->shutdown_in_progress)
    {
        STEREOTOP_RULE_LOG("ruleStereoTopEnableConnectableHandset, ignore as we are in shutdown mode");
        return rule_action_ignore;
    }

    STEREOTOP_RULE_LOG("ruleStereoTopEnableConnectableHandset, run as stereo not connected to handset");

    return RULE_ACTION_RUN_PARAM(enable_connectable);
}


/*! Decide whether to allow handset BR/EDR connections */
static rule_action_t ruleStereoTopAllowHandsetConnect(void)
{
    const bool allow_connect = TRUE;

    /* Ignore the rule if we are in shutdown mode */
    if(StereoTopologyGetTaskData()->shutdown_in_progress)
    {
        STEREOTOP_RULE_LOG("ruleStereoTopAllowHandsetConnect, ignore as we are in shutdown mode");
        return rule_action_ignore;
    }
    STEREOTOP_RULE_LOG("ruleStereoTopAllowHandsetConnect, run ");

    return RULE_ACTION_RUN_PARAM(allow_connect);
}

/*! Decide whether to allow handset LE connections */
static rule_action_t ruleStereoTopAllowLEConnection(void)
{
    const STEREOTOP_GOAL_ALLOW_LE_CONNECTION_T allow_connect = {.allow = TRUE};

    STEREOTOP_RULE_LOG("ruleStereoTopAllowLEConnection, run ");
    return RULE_ACTION_RUN_PARAM(allow_connect);
}

static rule_action_t ruleStereoTopStop(void)
{
    STEREOTOP_RULE_LOG("ruleStereoTopStop");

    return rule_action_run;
}

static rule_action_t ruleStereoTopPeerPair(void)
{
    /* only if peer is required */
    if(Multidevice_IsPair())
    {
        bdaddr peer_addr;

        /* Get peer bd address if paired */
        if(appDeviceGetPeerBdAddr(&peer_addr))
        {
            stereo_topology_task_data_t *stereo_top = StereoTopologyGetTaskData();
            DEBUG_LOG("ruleStereoTopPeerPair, ignore as already paired with peer");
            /* We are done with peer pair, update the requesting task */
            StereoTopology_SendPeerPairCfm(stereo_topology_status_success);
            return rule_action_ignore;
        }
        
        STEREOTOP_RULE_LOG("ruleStereoTopPeerPair, is enabled for CSIP LR Speaker. just allow");
        return rule_action_run;
    }
    return rule_action_ignore;
}

static rule_action_t ruleStereoTopPeerFindRole(void)
{
    stereo_topology_find_role_t role = stereo_find_role_no_peer;
    /* only if peer is required */
    if(Multidevice_IsPair())
    {
        bdaddr bd_addr_secondary;

        /* Secondary BD_ADDR will be set if paired with peer */
        if(!appDeviceGetPeerBdAddr(&bd_addr_secondary))
        {
            DEBUG_LOG("ruleStereoTopPeerFindRole, ignore peer is not paired, firt pair peer");
            StereoTopology_SendFindRoleCfm(&role);
            return rule_action_ignore;
        }
        
        STEREOTOP_RULE_LOG("ruleStereoTopPeerFindRole, run");
        return rule_action_run;
    }
    DEBUG_LOG("ruleStereoTopPeerFindRole, ignore as this is standalone and PFR not required");
    StereoTopology_SendFindRoleCfm(&role);
    return rule_action_ignore;
}

static rule_action_t ruleStereoTopPrimaryConnPeer(void)
{
    /* only if peer is required */
    if(Multidevice_IsPair())
    {
        bdaddr bd_addr_secondary;

        /* Secondary BD_ADDR will be set if paired with peer */
        if(!appDeviceGetPeerBdAddr(&bd_addr_secondary))
        {
            DEBUG_LOG("ruleStereoTopPrimaryConnPeer, ignore peer is not paired, firt pair peer");
            return rule_action_ignore;
        }
        
        STEREOTOP_RULE_LOG("ruleStereoTopPrimaryConnPeer, run");
        return rule_action_run;
    }
    DEBUG_LOG("ruleStereoTopPrimaryConnPeer, ignore as this is standalone and not required");
    return rule_action_ignore;
}

static rule_action_t ruleStereoTopSecConnPeer(void)
{
    /* only if peer is required */
    if(Multidevice_IsPair())
    {
        bdaddr bd_addr_primary;

        /* Secondary BD_ADDR will be set if paired with peer */
        if(!appDeviceGetPeerBdAddr(&bd_addr_primary))
        {
            DEBUG_LOG("ruleStereoTopSecConnPeer, ignore peer is not paired, firt pair peer");
            return rule_action_ignore;
        }

        if(BtDevice_IsMyAddressPrimary())
        {
            DEBUG_LOG("ruleStereoTopSecConnPeer, ignore I am only primary");
            return rule_action_ignore;
        }
        
        STEREOTOP_RULE_LOG("ruleStereoTopSecConnPeer, run");
        return rule_action_run;
    }
    DEBUG_LOG("ruleStereoTopSecConnPeer, ignore as this is standalone and not required");
    return rule_action_ignore;
}




/*****************************************************************************
 * END RULES FUNCTIONS
 *****************************************************************************/

/*! \brief Initialise the stereo rules module. */
bool StereoTopologyRules_Init(Task result_task)
{
    stereo_topology_rules_task_data_t *stereo_rules = StereoTopologyRulesGetTaskData();
    rule_set_init_params_t rule_params;

    memset(&rule_params, 0, sizeof(rule_params));
    rule_params.rules = stereotop_rules_set;
    rule_params.rules_count = ARRAY_DIM(stereotop_rules_set);
    rule_params.nop_message_id = STEREOTOP_GOAL_NOP;
    rule_params.event_task = result_task;
    stereo_rules->rule_set = RulesEngine_CreateRuleSet(&rule_params);

    return TRUE;
}

rule_set_t StereoTopologyRules_GetRuleSet(void)
{
    stereo_topology_rules_task_data_t *stereo_rules = StereoTopologyRulesGetTaskData();
    return stereo_rules->rule_set;
}

void StereoTopologyRules_SetEvent(rule_events_t event_mask)
{
    stereo_topology_rules_task_data_t *stereo_rules = StereoTopologyRulesGetTaskData();
    RulesEngine_SetEvent(stereo_rules->rule_set, event_mask);
}

void StereoTopologyRules_ResetEvent(rule_events_t event)
{
    stereo_topology_rules_task_data_t *stereo_rules = StereoTopologyRulesGetTaskData();
    RulesEngine_ResetEvent(stereo_rules->rule_set, event);
}

rule_events_t StereoTopologyRules_GetEvents(void)
{
    stereo_topology_rules_task_data_t *stereo_rules = StereoTopologyRulesGetTaskData();
    return RulesEngine_GetEvents(stereo_rules->rule_set);
}

void StereoTopologyRules_SetRuleComplete(MessageId message)
{
    stereo_topology_rules_task_data_t *stereo_rules = StereoTopologyRulesGetTaskData();
    RulesEngine_SetRuleComplete(stereo_rules->rule_set, message);
}

void StereoTopologyRules_SetRuleWithEventComplete(MessageId message, rule_events_t event)
{
    stereo_topology_rules_task_data_t *stereo_rules = StereoTopologyRulesGetTaskData();
    RulesEngine_SetRuleWithEventComplete(stereo_rules->rule_set, message, event);
}

/*! \brief Copy rule param data for the engine to put into action messages.
    \param param Pointer to data to copy.
    \param size_param Size of the data in bytes.
    \return rule_action_run_with_param to indicate the rule action message needs parameters.
 */
static rule_action_t stereoTopologyRules_CopyRunParams(const void* param, size_t size_param)
{
    stereo_topology_rules_task_data_t *stereo_rules = StereoTopologyRulesGetTaskData();
    RulesEngine_CopyRunParams(stereo_rules->rule_set, param, size_param);

    return rule_action_run_with_param;
}
