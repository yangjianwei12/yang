/*!
\copyright  Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#include "stereo_topology_procedure_peer_pair.h"
#include "stereo_topology_procedures.h"
#include "stereo_topology_procedure_setup_le_peer_pair.h"
#include "stereo_topology_procedure_send_topology_message.h"
#include "stereo_topology_client_msgs.h"

#include <peer_pair_le.h>

#include <logging.h>

#include <message.h>
#include <panic.h>

#define MAKE_MESSAGE(TYPE)  TYPE##_T *message = PanicUnlessNew(TYPE##_T);

static void stereoTopology_ProcPeerPairHandleMessage(Task task, MessageId id, Message message);
void StereoTopology_ProcedurePeerPairStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void StereoTopology_ProcedurePeerPairCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t stereo_proc_pair_peer_fns = {
    StereoTopology_ProcedurePeerPairStart,
    StereoTopology_ProcedurePeerPairCancel,
};

typedef struct
{
    TaskData task;
    Task result_task;
    procedure_complete_func_t complete_fn;
} stereoTopProcPeerPairTaskData;

typedef enum
{
     /*! peer pair timeout triggered */
    STEREO_TOP_PROC_PEER_PAIR_TIMEOUT_INTERNAL,
} stereo_top_proc_peer_pair_internal_message_t;


stereoTopProcPeerPairTaskData stereotop_proc_peer_pair = {stereoTopology_ProcPeerPairHandleMessage};

#define StereoTopProcPeerPairGetTaskData()     (&stereotop_proc_peer_pair)
#define StereoTopProcPeerPairGetTask()         (&stereotop_proc_peer_pair.task)

void StereoTopology_ProcedurePeerPairStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    UNUSED(goal_data);

    DEBUG_LOG_INFO("StereoTopology_ProcedurePeerPairStart");

    StereoTopProcPeerPairGetTaskData()->complete_fn = proc_complete_fn;
    StereoTopProcPeerPairGetTaskData()->result_task = result_task;

    if(StereoTopologyGetTaskData()->peer_pair_timeout)
    {
        DEBUG_LOG_INFO("StereoTopology_ProcedurePeerPairStart start with timeout of %d", StereoTopologyGetTaskData()->peer_pair_timeout);
        MessageSendLater(StereoTopProcPeerPairGetTask(), STEREO_TOP_PROC_PEER_PAIR_TIMEOUT_INTERNAL, NULL, D_SEC(StereoTopologyGetTaskData()->peer_pair_timeout));
        PeerPairLe_FindPeer(StereoTopProcPeerPairGetTask());
    }

    /* procedure started synchronously, confirm start now */
    proc_start_cfm_fn(stereo_topology_procedure_peer_pair, procedure_result_success);
}

void StereoTopology_ProcedurePeerPairCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("StereoTopology_ProcedurePeerPairCancel");
    MessageCancelAll(StereoTopProcPeerPairGetTask(), STEREO_TOP_PROC_PEER_PAIR_TIMEOUT_INTERNAL);
    /* Cancel Peer Pair */
    PeerPairLe_StopFindPeer();
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, stereo_topology_procedure_peer_pair, procedure_result_failed);
}

static void stereoTopology_ProcPeerPairHandlePeerPairLePairCfm(const PEER_PAIR_LE_PAIR_CFM_T *cfm)
{
    DEBUG_LOG_INFO("twsTopology_ProcPairPeerHandlePeerPairLePairCfm %u", cfm->status);
    MessageCancelAll(StereoTopProcPeerPairGetTask(), STEREO_TOP_PROC_PEER_PAIR_TIMEOUT_INTERNAL);
    StereoTopProcPeerPairGetTaskData()->complete_fn(stereo_topology_procedure_peer_pair, procedure_result_success);
}

static void stereoTopology_ProcPeerPairHandleTimeout(void)
{
    DEBUG_LOG_INFO("stereoTopology_ProcPeerPairHandleTimeout");
    /* Cancel LE Peer Pair */
    PeerPairLe_StopFindPeer();
    StereoTopProcPeerPairGetTaskData()->complete_fn(stereo_topology_procedure_peer_pair, procedure_result_success);
}

static void stereoTopology_ProcPeerPairHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch (id)
    {
        case PEER_PAIR_LE_PAIR_CFM:
            stereoTopology_ProcPeerPairHandlePeerPairLePairCfm((const PEER_PAIR_LE_PAIR_CFM_T *)message);
            break;

        case STEREO_TOP_PROC_PEER_PAIR_TIMEOUT_INTERNAL:
            stereoTopology_ProcPeerPairHandleTimeout();
            break;

        default:
            break;
    }
}

const STEREOTOP_PROC_SEND_MESSAGE_PARAMS_T stereo_peer_pair_finished = {PROC_SEND_STEREO_TOPOLOGY_MESSAGE_PEER_PAIR_FINISHED, (Message)NULL, 0};
    
#define PROC_SEND_STEREO_TOPOLOGY_MESSAGE_PEER_PAIR_FINISHED_MESSAGE ((Message)&stereo_peer_pair_finished)



#define PEER_PAIR_SCRIPT(ENTRY) \
    ENTRY(stereo_proc_setup_le_peer_pair_fns, PROC_SETUP_LE_PEER_PAIR_DATA_ENABLE), \
    ENTRY(stereo_proc_pair_peer_fns, NO_DATA), \
    ENTRY(stereo_proc_send_topology_message_fns, PROC_SEND_STEREO_TOPOLOGY_MESSAGE_PEER_PAIR_FINISHED_MESSAGE)

/* Define the script pair_peer_script */
DEFINE_TOPOLOGY_SCRIPT(stereo_peer_pair,PEER_PAIR_SCRIPT);

