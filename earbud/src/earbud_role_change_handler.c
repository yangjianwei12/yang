/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Handles role changes by pausing handset connections.
*/

#include <logging.h>
#include <message.h>
#include <panic.h>

#include <handset_service.h>
#include <tws_topology_role_change_client_if.h>

#include "earbud_sm.h"

Task serverTask;

static void earbudRoleChangeHandler_Initialise(Task server, int32_t reconnect_delay);
static void earbudRoleChangeHandler_RoleChangeIndication(tws_topology_role role);
static void earbudRoleChangeHandler_ProposeRoleChange(void);
static void earbudRoleChangeHandler_ForceRoleChange(void);
static void earbudRoleChangeHandler_PrepareRoleChange(void);
static void earbudRoleChangeHandler_CancelRoleChange(void);

static void earbudRoleChangeHandler_Initialise(Task server, int32_t reconnect_delay)
{
    UNUSED(reconnect_delay);
    serverTask = server;
}

static void earbudRoleChangeHandler_RoleChangeIndication(tws_topology_role role)
{
    DEBUG_LOG_INFO("earbudRoleChangeHandler_RoleChangeIndication");

    if (role == tws_topology_role_primary)
    {
        if (appSmGetReconnectPostHandover())
        {
            PrimaryRules_SetEvent(RULE_EVENT_CONNECT_HANDSET_USER);
            appSmSetReconnectPostHandover(FALSE);
        }
    }
    else
    {
        appSmSetReconnectPostHandover(FALSE);
    }
}

static void earbudRoleChangeHandler_ProposeRoleChange(void)
{
    MAKE_TWS_ROLE_CHANGE_ACCEPTANCE_MESSAGE(TWS_ROLE_CHANGE_ACCEPTANCE_CFM);
    message->role_change_accepted = TRUE;
    MessageSend(serverTask, TWS_ROLE_CHANGE_ACCEPTANCE_CFM, message);
}

static void earbudRoleChangeHandler_ForceRoleChange(void)
{
    DEBUG_LOG_INFO("earbudRoleChangeHandler_ForceRoleChange");

    if (HandsetService_IsConnecting())
    {
        if (HandsetService_IsHandsetInBredrContextPresent(handset_bredr_context_link_loss_reconnecting))
        {
            DEBUG_LOG_VERBOSE("earbudRoleChangeHandler_ForceRoleChange continue link loss reconnection through role change");
            appSmSetContinueLinkLossReconnectPostHandover(TRUE);
        }

        HandsetService_StopReconnect(SmGetTask());
        appSmSetReconnectPostHandover(TRUE);
    }
}

static void earbudRoleChangeHandler_PrepareRoleChange(void)
{
    DEBUG_LOG_INFO("earbudRoleChangeHandler_PrepareRoleChange");
    MessageSend(serverTask, TWS_ROLE_CHANGE_PREPARATION_CFM, NULL);
}

static void earbudRoleChangeHandler_CancelRoleChange(void)
{
    DEBUG_LOG_INFO("earbudRoleChangeHandler_CancelRoleChange");
    if (appSmGetReconnectPostHandover())
    {
        appSmSetReconnectPostHandover(FALSE);
        PrimaryRules_SetEvent(RULE_EVENT_CONNECT_HANDSET_USER);
    }

    if (appSmGetMarshalledState())
    {
        appSmSetMarshalledState(FALSE);
    }
}

TWS_ROLE_CHANGE_CLIENT_REGISTRATION_MAKE(EARBUD_ROLE_CHANGE_HANDLER,
    earbudRoleChangeHandler_Initialise, earbudRoleChangeHandler_RoleChangeIndication,
    earbudRoleChangeHandler_ProposeRoleChange, earbudRoleChangeHandler_ForceRoleChange,
    earbudRoleChangeHandler_PrepareRoleChange, earbudRoleChangeHandler_CancelRoleChange);
