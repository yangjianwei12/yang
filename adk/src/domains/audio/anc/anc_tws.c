/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       anc_tws.c
\brief      For ANC use cases where the primary EB will support the use case capability e.g Noise ID
            This file provides TWS support for ANC use cases to identify the primary
*/

#ifdef ENABLE_ADAPTIVE_ANC

#include "anc_tws.h"
#include "anc_noise_id.h"
#include "bt_device.h"
#include "tws_topology_role_change_client_if.h"

#include <logging.h>
#include <message.h>
#include <panic.h>
#include <stdio.h>

typedef struct
{
    Task serverTask;
    int32 min_reconnection_delay;
    tws_topology_role current_role;
} anc_tws_data_t;

static anc_tws_data_t anc_tws_data =
{
    .serverTask = NULL,
    .min_reconnection_delay = D_SEC(2),
    .current_role = tws_topology_role_none
};

static void ancTws_SendRoleChangePrepareResponse(void)
{
    DEBUG_LOG("ancTws_SendRoleChangePrepareResponse");
    MessageSend(PanicNull(anc_tws_data.serverTask), TWS_ROLE_CHANGE_PREPARATION_CFM, NULL);
}

static void ancTws_Enable(void)
{
    DEBUG_LOG("ancTws_Enable current_role=enum:tws_topology_role:%d", anc_tws_data.current_role);
    if ((anc_tws_data.current_role == tws_topology_role_primary) &&
        AncNoiseId_IsFeatureEnabled())
    {
        AncNoiseId_Enable(TRUE);
    }
}

static void ancTws_Disable(void)
{
    DEBUG_LOG("ancTws_Disable current_role=enum:tws_topology_role:%d", anc_tws_data.current_role);
    AncNoiseId_Enable(FALSE);
}

static void ancTws_SendRoleChangeRequestResponse(void)
{
    DEBUG_LOG("ancTws_SendRoleChangeRequestResponse");
    MAKE_TWS_ROLE_CHANGE_ACCEPTANCE_MESSAGE(TWS_ROLE_CHANGE_ACCEPTANCE_CFM);
    message->role_change_accepted = TRUE;

    MessageSend(PanicNull(anc_tws_data.serverTask), TWS_ROLE_CHANGE_ACCEPTANCE_CFM, message);
}

static void ancTws_Initialise(Task server, int32_t reconnect_delay)
{
    DEBUG_LOG("ancTws_Initialise server, reconnect delay =%d",reconnect_delay);
    anc_tws_data.min_reconnection_delay = reconnect_delay;
    anc_tws_data.serverTask = server;
}

static void ancTws_RoleChangeIndication(tws_topology_role new_role)
{
    tws_topology_role old_role = anc_tws_data.current_role;
    anc_tws_data.current_role = new_role;
    
    DEBUG_LOG("ancTws_RoleChangeIndication role=enum:tws_topology_role:%d", new_role);

    if(old_role == tws_topology_role_primary && new_role != tws_topology_role_primary)
    {
        ancTws_Disable();
    }
    else if(old_role != tws_topology_role_primary && new_role == tws_topology_role_primary)
    {
        ancTws_Enable();
    }
}

static void ancTws_ProposeRoleChange(void)
{
    DEBUG_LOG("ancTws_ProposeRoleChange");
    ancTws_SendRoleChangeRequestResponse();
}

static void ancTws_ForceRoleChange(void)
{
    DEBUG_LOG("ancTws_ForceRoleChange");
}

static void ancTws_PrepareRoleChange(void)
{
    DEBUG_LOG("ancTws_PrepareRoleChange");
    ancTws_SendRoleChangePrepareResponse();
}

static void ancTws_CancelRoleChange(void)
{
    DEBUG_LOG("ancTws_CancelRoleChange");
}

bool AncTws_IsPrimary(void)
{
    return (anc_tws_data.current_role == tws_topology_role_primary);
}

TWS_ROLE_CHANGE_CLIENT_REGISTRATION_MAKE(ancTws,
                                         ancTws_Initialise,
                                         ancTws_RoleChangeIndication,
                                         ancTws_ProposeRoleChange,
                                         ancTws_ForceRoleChange,
                                         ancTws_PrepareRoleChange,
                                         ancTws_CancelRoleChange);

#endif /*ENABLE_ADAPTIVE_ANC*/

