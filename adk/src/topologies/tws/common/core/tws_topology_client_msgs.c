/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      TWS Topology utility functions for sending messages to clients.
*/

#include "tws_topology.h"
#include "tws_topology_private.h"
#include "tws_topology_client_msgs.h"

#include <logging.h>

#include <panic.h>

void TwsTopology_SendStartCompletedMsg(tws_topology_status_t status)
{
    MAKE_TWS_TOPOLOGY_MESSAGE(TWS_TOPOLOGY_START_COMPLETED);
    message->status = status;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(TwsTopologyGetMessageClientTasks()), TWS_TOPOLOGY_START_COMPLETED, message);
}

void TwsTopology_SendInitialIdleCompletedMsg(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(TwsTopologyGetMessageClientTasks()), TWS_TOPOLOGY_INITIAL_IDLE_COMPLETED);
}

void TwsTopology_SendStopCompletedMsg(tws_topology_status_t status)
{
    MAKE_TWS_TOPOLOGY_MESSAGE(TWS_TOPOLOGY_STOP_COMPLETED);
    message->status = status;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(TwsTopologyGetMessageClientTasks()), TWS_TOPOLOGY_STOP_COMPLETED, message);
}

void TwsTopology_SendRoleChangedCompletedMsg(tws_topology_role role, bool error_forced_no_role)
{
    MAKE_TWS_TOPOLOGY_MESSAGE(TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED);

    message->role = role;
    message->error_forced_no_role = error_forced_no_role;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(TwsTopologyGetMessageClientTasks()), TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED, message);
}

void TwsTopology_SendJoinRequestCompletedMsg(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(TwsTopologyGetMessageClientTasks()), TWS_TOPOLOGY_REQUEST_JOIN_COMPLETED);
}

void TwsTopology_SendLeaveRequestCompletedMsg(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(TwsTopologyGetMessageClientTasks()), TWS_TOPOLOGY_REQUEST_LEAVE_COMPLETED);
}
