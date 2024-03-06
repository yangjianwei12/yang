/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Stereo Topology utility functions for sending messages to clients.
*/

#include "stereo_topology.h"
#include "stereo_topology_private.h"
#include "stereo_topology_client_msgs.h"

#include <logging.h>
#include <task_list.h>
#include <panic.h>

void StereoTopology_SendStopCfm(stereo_topology_status_t status)
{
    stereo_topology_task_data_t *stereo_taskdata = StereoTopologyGetTaskData();
    MAKE_STEREO_TOPOLOGY_MESSAGE(STEREO_TOPOLOGY_STOP_CFM);

    DEBUG_LOG_VERBOSE("StereoTopology_SendStopCfm status %u", status);

    MessageCancelAll(StereoTopologyGetTask(), STEREOTOP_INTERNAL_TIMEOUT_TOPOLOGY_STOP);

    message->status = status;
    MessageSend(stereo_taskdata->app_task, STEREO_TOPOLOGY_STOP_CFM, message);
}

void StereoTopology_SendStoppingCfm(stereo_topology_status_t status)
{
    DEBUG_LOG_VERBOSE("StereoTopology_SendStoppingCfm status %u", status);

    MESSAGE_MAKE(msg, STEREO_TOPOLOGY_STOPPING_CFM_T);
    msg->status = status;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(StereoTopologyGetMessageClientTasks()), STEREO_TOPOLOGY_STOPPING_CFM, msg);
}

void StereoTopology_SendStartedCfm(stereo_topology_status_t status)
{
    DEBUG_LOG_VERBOSE("StereoTopology_SendStartedCfm status %u", status);

    MESSAGE_MAKE(msg, STEREO_TOPOLOGY_STARTED_CFM_T);
    msg->status = status;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(StereoTopologyGetMessageClientTasks()), STEREO_TOPOLOGY_STARTED_CFM, msg);
}

void StereoTopology_SendStartingCfm(stereo_topology_status_t status)
{
    DEBUG_LOG_VERBOSE("StereoTopology_SendStartingCfm status %u", status);

    MESSAGE_MAKE(msg, STEREO_TOPOLOGY_STARTING_CFM_T);
    msg->status = status;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(StereoTopologyGetMessageClientTasks()), STEREO_TOPOLOGY_STARTING_CFM, msg);
}

void StereoTopology_SendPeerPairCfm(stereo_topology_status_t status)
{
    stereo_topology_task_data_t *stereo_taskdata = StereoTopologyGetTaskData();
    MAKE_STEREO_TOPOLOGY_MESSAGE(STEREO_TOPOLOGY_PEER_PAIR_CFM);

    DEBUG_LOG_VERBOSE("StereoTopology_SendPeerPairCfm status %u", status);

    message->status = status;
    MessageSend(stereo_taskdata->app_task, STEREO_TOPOLOGY_PEER_PAIR_CFM, message);
}

#ifdef ENABLE_LE_AUDIO_CSIP
void StereoTopology_SendSirkUpdateCfm(uint8 *key_a, uint8 *key_b)
{
    stereo_topology_task_data_t *stereo_taskdata = StereoTopologyGetTaskData();
    MAKE_STEREO_TOPOLOGY_MESSAGE(STEREO_TOPOLOGY_SIRK_UPDATE_CFM);

    DEBUG_LOG_VERBOSE("StereoTopology_SendSirkUpdateCfm");
    memcpy(message->key_a, key_a, GRKS_KEY_SIZE_128BIT_OCTETS);
    memcpy(message->key_b, key_b, GRKS_KEY_SIZE_128BIT_OCTETS);

    MessageSend(stereo_taskdata->app_task, STEREO_TOPOLOGY_SIRK_UPDATE_CFM, message);
}
#endif /* ENABLE_LE_AUDIO_CSIP */

void StereoTopology_SendFindRoleCfm(const stereo_topology_find_role_t* role)
{
    PanicNull((void*)role);

    MESSAGE_MAKE(msg, STEREO_TOPOLOGY_FIND_ROLE_CFM_T);

    DEBUG_LOG_VERBOSE("StereoTopology_SendFindRoleCfm enum:stereo_topology_find_role_t:%d", *role);

    msg->role = *role;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(StereoTopologyGetMessageClientTasks()), STEREO_TOPOLOGY_FIND_ROLE_CFM, msg);
}

void StereoTopology_SendEnableStandaloneCfm(void)
{

    MESSAGE_MAKE(msg, STEREO_TOPOLOGY_ENABLE_STANDALONE_CFM_T);

    DEBUG_LOG_VERBOSE("StereoTopology_SendEnableStandaloneCfm");

    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(StereoTopologyGetMessageClientTasks()), STEREO_TOPOLOGY_ENABLE_STANDALONE_CFM, msg);
}


void StereoTopology_SendPeerProileConnCfm(stereo_topology_status_t status)
{

    stereo_topology_task_data_t *stereo_taskdata = StereoTopologyGetTaskData();
    MAKE_STEREO_TOPOLOGY_MESSAGE(STEREO_TOPOLOGY_PEER_PROFILE_CONN_CFM);

    DEBUG_LOG_VERBOSE("StereoTopology_SendPeerProileConnCfm status %u", status);

    message->status = status;
    MessageSend(stereo_taskdata->app_task, STEREO_TOPOLOGY_PEER_PROFILE_CONN_CFM, message);
}

