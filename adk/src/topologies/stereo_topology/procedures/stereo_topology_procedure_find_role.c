/*!
\copyright  Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#include "stereo_topology_procedure_find_role.h"
#include "stereo_topology_procedures.h"
#include "stereo_topology_config.h"
#include "stereo_topology_advertising.h"
#include "stereo_topology.h"
#include "stereo_topology_procedure_send_topology_message.h"


#include <peer_find_role.h>

#include <logging.h>

#include <message.h>
#include <panic.h>

static void stereoTopology_ProcFindRoleHandleMessage(Task task, MessageId id, Message message);
void StereoTopology_ProcedureFindRoleStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void StereoTopology_ProcedureFindRoleCancel(procedure_cancel_cfm_func_t proc_cancel_fn);

const procedure_fns_t proc_find_role_fns = {
    StereoTopology_ProcedureFindRoleStart,
    StereoTopology_ProcedureFindRoleCancel,
};

typedef struct
{
    TaskData task;
    procedure_complete_func_t complete_fn;
    procedure_cancel_cfm_func_t cancel_fn;
} stereoTopProcFindRoleTaskData;

stereoTopProcFindRoleTaskData stereotop_proc_find_role = {stereoTopology_ProcFindRoleHandleMessage};
STEREOTOP_PROC_SEND_MESSAGE_PARAMS_T stereo_find_role_finished = {PROC_SEND_STEREO_TOPOLOGY_MESSAGE_FIND_ROLE_FINISHED, (Message)NULL, 0};

#define StereoTopProcFindRoleGetTaskData()     (&stereotop_proc_find_role)
#define StereoTopProcFindRoleGetTask()         (&stereotop_proc_find_role.task)


static void stereoTopology_ProcedureFindRoleReset(void)
{
    stereoTopProcFindRoleTaskData* td = StereoTopProcFindRoleGetTaskData();
    PeerFindRole_UnregisterTask(StereoTopProcFindRoleGetTask());
    td->complete_fn = NULL;
    td->cancel_fn = NULL;
}

void StereoTopology_ProcedureFindRoleStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    stereoTopProcFindRoleTaskData* td = StereoTopProcFindRoleGetTaskData();
    UNUSED(goal_data);
    UNUSED(result_task);

    DEBUG_LOG("StereoTopology_ProcedureFindRoleStart timeout %d", StereoTopologyGetTaskData()->find_role_timeout);

    /* Guard against starting PFR when already active. If PFR is already active
    then the intention of starting this goal will be met when it completes, so
    complete this instance of the procedure immediately */
    if (PeerFindRole_IsActive())
    {
        proc_start_cfm_fn(stereo_topology_procedure_find_role, procedure_result_success);
        Procedures_DelayedCompleteCfmCallback(proc_complete_fn, stereo_topology_procedure_find_role, procedure_result_success);
        return;
    }

    td->complete_fn = proc_complete_fn;

    stereoTopology_UpdateAdvertisingParams();
    PanicFalse(LeAdvertisingManager_AllowAdvertising(StereoTopProcFindRoleGetTask(), TRUE));
    PeerFindRole_RegisterTask(StereoTopProcFindRoleGetTask());
    PeerFindRole_FindRole(D_SEC(StereoTopologyGetTaskData()->find_role_timeout));

    /* start is synchronous, use the callback to confirm now */
    proc_start_cfm_fn(stereo_topology_procedure_find_role, procedure_result_success);

    /* if starting in continuous mode indicate the procedure is complete already,
     * this permits the procedure to be used in the middle of a script where we
     * want to continue processing subsequent procedures */
    if (StereoTopologyGetTaskData()->find_role_timeout == 0)
    {
        stereoTopology_ProcedureFindRoleReset();
        Procedures_DelayedCompleteCfmCallback(proc_complete_fn, stereo_topology_procedure_find_role, procedure_result_success);
    }
}

void StereoTopology_ProcedureFindRoleCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    stereoTopProcFindRoleTaskData* td = StereoTopProcFindRoleGetTaskData();

    DEBUG_LOG("StereoTopology_ProcedureFindRoleCancel");

    td->complete_fn = NULL;
    td->cancel_fn = proc_cancel_cfm_fn;

    /* Re-register for pfr notifications so that this procedure will receive
       PEER_FIND_ROLE_CANCELLED to confirm PeerFindRole_FindRoleCancel has
       finished. If already registered this is effectively a nop.

       Note: without re-registering there is a small chance of a race
             condition when:
       * This procedure was started in continuous mode
        (PROC_FIND_ROLE_TIMEOUT_DATA_CONTINUOUS),
         * A delayed complete cfm is sent and this procedure un-registers
           for pfr notifications in twsTopology_ProcedureFindRoleReset.
       * The delayed complete cfm is in flight.
       * This function is called to cancel the procedure.
         * But it may never get PEER_FIND_ROLE_CANCELLED because it is not
           registered for pfr notifications. */
    PeerFindRole_RegisterTask(StereoTopProcFindRoleGetTask());
    PeerFindRole_FindRoleCancel();

    /* cancellation is asynchronous for this procedure, goal handling knows will have to wait for
     * the cancel cfm callback */
}

static void stereTopology_HandleRoleIndication(MessageId id)
{
    stereo_topology_find_role_t *role = PanicUnlessMalloc(sizeof(stereo_topology_find_role_t));

    switch(id)
    {
        case PEER_FIND_ROLE_NO_PEER:
            *role = stereo_find_role_no_peer;
            break;
        case PEER_FIND_ROLE_ACTING_PRIMARY:
            *role = stereo_find_role_acting_primary;
            break;
        case PEER_FIND_ROLE_PRIMARY:
            *role = stereo_find_role_primary;
            break;
        case PEER_FIND_ROLE_SECONDARY:
            *role = stereo_find_role_secondary;
            break;
        default:
            Panic();
    }
    stereo_find_role_finished.message = role;
    stereo_find_role_finished.message_size = sizeof(stereo_topology_find_role_t);
}

static void stereoTopology_ProcFindRoleHandleMessage(Task task, MessageId id, Message message)
{
    stereoTopProcFindRoleTaskData* td = StereoTopProcFindRoleGetTaskData();

    UNUSED(task);
    UNUSED(message);

    if (!td->complete_fn && !td->cancel_fn)
    {
        /* If neither callback is set this procedure is not active so ignore any messages */
        return;
    }

    switch (id)
    {
        /* These messages all indicate the procedure completed, so indicate such
         * to the topology client. The topology client is separately registered
         * to receive role notifications from the find role service, so no need
         * to send result. */
        case PEER_FIND_ROLE_NO_PEER:
        case PEER_FIND_ROLE_ACTING_PRIMARY:
        case PEER_FIND_ROLE_PRIMARY:
        case PEER_FIND_ROLE_SECONDARY:
            /* record the PFR output */
            stereTopology_HandleRoleIndication(id);
            if (td->complete_fn)
            {
                td->complete_fn(stereo_topology_procedure_find_role, procedure_result_success);
            }
            else if (td->cancel_fn)
            {
                /* It may be possible that peer_find_role completed before the
                   cancel was processed - in that situation we should call the
                   cancel_fn instead of the complete_fn. */
                td->cancel_fn(stereo_topology_procedure_find_role, procedure_result_success);
            }

            stereoTopology_ProcedureFindRoleReset();
            break;

        case PEER_FIND_ROLE_CANCELLED:
            td->cancel_fn(stereo_topology_procedure_find_role, procedure_result_success);
            stereoTopology_ProcedureFindRoleReset();
            break;

        case LE_ADV_MGR_ALLOW_ADVERTISING_CFM:
            {
                DEBUG_LOG("stereTopology_HandleRoleIndication: LEAM Adv %d", ((LE_ADV_MGR_ALLOW_ADVERTISING_CFM_T*)message)->allow);
            }
            break;

        default:
            break;
    }
}
        
#define PROC_SEND_STEREO_TOPOLOGY_MESSAGE_FIND_ROLE_FINISHED_MESSAGE ((Message)&stereo_find_role_finished)


#define FIND_ROLE_SCRIPT(ENTRY) \
    ENTRY(proc_find_role_fns, NO_DATA), \
    ENTRY(stereo_proc_send_topology_message_fns, PROC_SEND_STEREO_TOPOLOGY_MESSAGE_FIND_ROLE_FINISHED_MESSAGE)

/* Define the find_role_script with timeout */
DEFINE_TOPOLOGY_SCRIPT(stereo_find_role, FIND_ROLE_SCRIPT);


