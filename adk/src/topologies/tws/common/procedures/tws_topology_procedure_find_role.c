/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#include "tws_topology_procedure_find_role.h"
#include "tws_topology_procedures.h"
#include "tws_topology_config.h"
#include "tws_topology_procedure_allow_connection_over_le.h"
#include "tws_topology_procedure_permit_bt.h"
#include "tws_topology_advertising.h"
#include "tws_topology_private.h"

#include <peer_find_role.h>

#include <logging.h>

#include <message.h>

FIND_ROLE_PARAMS_T proc_find_role_timeout = {0};
const FIND_ROLE_PARAMS_T proc_find_role_continuous = {0};

static void twsTopology_ProcFindRoleHandleMessage(Task task, MessageId id, Message message);
void TwsTopology_ProcedureFindRoleStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void TwsTopology_ProcedureFindRoleCancel(procedure_cancel_cfm_func_t proc_cancel_fn);

const procedure_fns_t proc_find_role_fns = {
    TwsTopology_ProcedureFindRoleStart,
    TwsTopology_ProcedureFindRoleCancel,
};

typedef struct
{
    TaskData task;
    procedure_complete_func_t complete_fn;
    procedure_cancel_cfm_func_t cancel_fn;
} twsTopProcFindRoleTaskData;

twsTopProcFindRoleTaskData twstop_proc_find_role = {.task = {twsTopology_ProcFindRoleHandleMessage}, .complete_fn = NULL, .cancel_fn = NULL};

#define TwsTopProcFindRoleGetTaskData()                     (&twstop_proc_find_role)
#define TwsTopProcFindRoleGetTask()                         (&twstop_proc_find_role.task)
#define STANDALONE_PRIMARY_DEVICE_TYPE_FIND_ROLE_TIMEOUT    (1)

static void twsTopology_ProcedureFindRoleReset(void)
{
    twsTopProcFindRoleTaskData* td = TwsTopProcFindRoleGetTaskData();
    PeerFindRole_UnregisterTask(TwsTopProcFindRoleGetTask());
    td->complete_fn = NULL;
    td->cancel_fn = NULL;
}

void TwsTopology_ProcedureFindRoleStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    twsTopProcFindRoleTaskData* td = TwsTopProcFindRoleGetTaskData();
    FIND_ROLE_PARAMS_T* params = (FIND_ROLE_PARAMS_T*)goal_data;
    if(goal_data == PROC_FIND_ROLE_TIMEOUT_DATA_TIMEOUT)
    {
        tws_topology_product_behaviour_t* tws_topology_product_behaviour = twsTopology_GetProductBehaviour();
        if(tws_topology_product_behaviour->device_type == topology_device_type_standalone)
        {
            params->timeout = STANDALONE_PRIMARY_DEVICE_TYPE_FIND_ROLE_TIMEOUT;
        }
        else
        {
            params->timeout = D_SEC(tws_topology_product_behaviour->timeouts.peer_find_role_sec);
        }
    }

    UNUSED(result_task);

    DEBUG_LOG("TwsTopology_ProcedureFindRoleStart timeout %d", params->timeout);

    /* Guard against starting PFR when already active. If PFR is already active
    then the intention of starting this goal will be met when it completes, so
    complete this instance of the procedure immediately */
    if (PeerFindRole_IsActive())
    {
        proc_start_cfm_fn(tws_topology_procedure_find_role, procedure_result_success);
        Procedures_DelayedCompleteCfmCallback(proc_complete_fn, tws_topology_procedure_find_role, procedure_result_success);
        return;
    }

    td->complete_fn = proc_complete_fn;    

    twsTopology_UpdateAdvertisingParams();
    PeerFindRole_RegisterTask(TwsTopProcFindRoleGetTask());
    PeerFindRole_FindRole(params->timeout);

    /* start is synchronous, use the callback to confirm now */
    proc_start_cfm_fn(tws_topology_procedure_find_role, procedure_result_success);

    /* if starting in continuous mode indicate the procedure is complete already,
     * this permits the procedure to be used in the middle of a script where we
     * want to continue processing subsequent procedures */
    if (goal_data == PROC_FIND_ROLE_TIMEOUT_DATA_CONTINUOUS)
    {
        twsTopology_ProcedureFindRoleReset();
        Procedures_DelayedCompleteCfmCallback(proc_complete_fn, tws_topology_procedure_find_role, procedure_result_success);
    }
}

void TwsTopology_ProcedureFindRoleCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    twsTopProcFindRoleTaskData* td = TwsTopProcFindRoleGetTaskData();

    DEBUG_LOG("TwsTopology_ProcedureFindRoleCancel");

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
    PeerFindRole_RegisterTask(TwsTopProcFindRoleGetTask());
    PeerFindRole_FindRoleCancel();

    /* cancellation is asynchronous for this procedure, goal handling knows will have to wait for
     * the cancel cfm callback */
}

static void twsTopology_ProcFindRoleHandleMessage(Task task, MessageId id, Message message)
{
    twsTopProcFindRoleTaskData* td = TwsTopProcFindRoleGetTaskData();

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
            if (td->complete_fn)
            {
                td->complete_fn(tws_topology_procedure_find_role, procedure_result_success);
            }
            else if (td->cancel_fn)
            {
                /* It may be possible that peer_find_role completed before the
                   cancel was processed - in that situation we should call the
                   cancel_fn instead of the complete_fn. */
                td->cancel_fn(tws_topology_procedure_find_role, procedure_result_success);
            }

            twsTopology_ProcedureFindRoleReset();
            break;

        case PEER_FIND_ROLE_CANCELLED:
            td->cancel_fn(tws_topology_procedure_find_role, procedure_result_success);
            twsTopology_ProcedureFindRoleReset();
            break;

        default:
            break;
    }
}

#define FIND_ROLE_SCRIPT(ENTRY) \
    ENTRY(proc_find_role_fns, PROC_FIND_ROLE_TIMEOUT_DATA_TIMEOUT)

/* Define the find_role_script with timeout */
DEFINE_TOPOLOGY_SCRIPT(find_role_with_timeout, FIND_ROLE_SCRIPT);
