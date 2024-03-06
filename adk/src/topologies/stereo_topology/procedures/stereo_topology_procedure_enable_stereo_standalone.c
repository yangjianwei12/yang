/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      Procedure to work as standalone stereo topolgy
            This can lead to the peer link being destroyed.
*/

#include "stereo_topology_procedure_enable_stereo_standalone.h"
#include "stereo_topology_procedure_enable_connectable_peer.h"
#include "stereo_topology_procedure_pri_connect_peer_profiles.h"
#include "stereo_topology_procedure_send_topology_message.h"


#include <bt_device.h>
#include <connection_manager.h>

#include <logging.h>
#include "mirror_profile_peer_audio_sync_l2cap.h"
#include "peer_signalling.h"

STEREOTOP_PROC_SEND_MESSAGE_PARAMS_T stereo_enable_standalone_finished = {PROC_SEND_STEREO_TOPOLOGY_MESSAGE_ENABLE_STANDALONE_FINISHED, (Message)NULL, 0};

static void stereoTopology_ProcEnableStereoStandaloneHandleMessage(Task task, MessageId id, Message message);

void StereoTopology_ProcedureEnableStereoStandaloneStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void StereoTopology_ProcedureReleaseEnableStereoStandaloneCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_enable_stereo_standalone_fns = {
    StereoTopology_ProcedureEnableStereoStandaloneStart,
    StereoTopology_ProcedureReleaseEnableStereoStandaloneCancel,
};

typedef struct
{
    TaskData task;
    procedure_complete_func_t complete_fn;
} stereoTopProcEnableStereoStandaloneTaskData;

stereoTopProcEnableStereoStandaloneTaskData stereotop_proc_enable_stereo_standalone = {stereoTopology_ProcEnableStereoStandaloneHandleMessage};

#define StereoTopProcEnableStereoStandaloneGetTaskData()     (&stereotop_proc_enable_stereo_standalone)
#define StereoTopProcEnableStereoStandaloneGetTask()         (&stereotop_proc_enable_stereo_standalone.task)

void StereoTopology_ProcedureEnableStereoStandaloneStart(Task result_task,
                                           procedure_start_cfm_func_t proc_start_cfm_fn,
                                           procedure_complete_func_t proc_complete_fn,
                                           Message goal_data)
{
    UNUSED(result_task);
    UNUSED(goal_data);

    bdaddr peer_addr;
    stereoTopProcEnableStereoStandaloneTaskData* td = StereoTopProcEnableStereoStandaloneGetTaskData();

    DEBUG_LOG("StereoTopology_ProcedureEnableStereoStandaloneStart");

    if(appDeviceGetPeerBdAddr(&peer_addr))
    {
        appPeerSigTerminateSdpPrimitive();
#ifdef INCLUDE_MIRRORING
        MirrorProfile_TerminateSdpPrimitive();
        /* if there is active a2dp streaming, then we need to ensure MDM link gets disconnected
           before we can claim standalone */
        if(MirrorProfile_IsA2dpActive())
        {
            td->complete_fn = proc_complete_fn;
            MirrorProfile_ClientRegister(StereoTopProcEnableStereoStandaloneGetTask());
        }
#endif

        /* entering standalone speaker, no more require peer ACL */
        ConManagerSendCloseAclRequest(&peer_addr, TRUE);
    }

    proc_start_cfm_fn(stereo_topology_procedure_enable_stereo_standalone, procedure_result_success);
    
    if(!td->complete_fn)
    {
        Procedures_DelayedCompleteCfmCallback(proc_complete_fn,
                                               stereo_topology_procedure_enable_stereo_standalone,
                                               procedure_result_success);
    }
}

void StereoTopology_ProcedureReleaseEnableStereoStandaloneCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("StereoTopology_ProcedureReleaseEnableStereoStandaloneCancel");

    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn,
                                         stereo_topology_procedure_enable_stereo_standalone,
                                         procedure_result_success);
}

static void stereoTopology_ProcEnableStereoStandaloneHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch(id)
    {
#ifdef INCLUDE_MIRRORING
        case MIRROR_PROFILE_DISCONNECT_IND:
        {
            stereoTopProcEnableStereoStandaloneTaskData* td = StereoTopProcEnableStereoStandaloneGetTaskData();
            
            DEBUG_LOG("stereoTopology_ProcEnableStereoStandaloneHandleMessage MIRROR disconnected");
            
            if(td->complete_fn)
            {
                td->complete_fn(stereo_topology_procedure_enable_stereo_standalone, procedure_result_success);
                td->complete_fn = NULL;
            }
        }
        break;
#endif

        default:
        break;
    }
}


#define PROC_SEND_STEREO_TOPOLOGY_MESSAGE_ENABLE_STANDALONE_FINISHED_MESSAGE ((Message)&stereo_enable_standalone_finished)


#define STEREO_STANDALONE_SCRIPT(ENTRY) \
    ENTRY(proc_pri_connect_peer_profiles_fns, PROC_PEER_PROFILE_DISCONNECT), \
    ENTRY(proc_enable_connectable_peer_fns, PROC_ENABLE_CONNECTABLE_PEER_DATA_DISABLE), \
    ENTRY(proc_enable_stereo_standalone_fns, NO_DATA), \
    ENTRY(stereo_proc_send_topology_message_fns, PROC_SEND_STEREO_TOPOLOGY_MESSAGE_ENABLE_STANDALONE_FINISHED_MESSAGE)

/* Define the enable version of this procedure as a script */
DEFINE_TOPOLOGY_SCRIPT(stereo_enable_stereo_standalone, STEREO_STANDALONE_SCRIPT);


