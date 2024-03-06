/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Procedure to stop LE broadcast.
*/

#include "tws_topology_procedure_stop_le_broadcast.h"
#include "tws_topology_procedures.h"
#include "tws_topology_config.h"
#ifdef INCLUDE_LE_AUDIO_BROADCAST
#include "le_broadcast_manager.h"
#endif

#include <logging.h>
#include <message.h>

typedef struct
{
    TaskData task_data;
    procedure_complete_func_t complete_fn;
    procedure_cancel_cfm_func_t cancel_fn;
} twsTopProcStopLeBroadcastTaskData;

static void twsTopology_ProcStopLeBroadcastHandleMessage(Task task, MessageId id, Message message);
static void twsTopology_ProcStopLeBroadcastStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
static void twsTopology_ProcStopLeBroadcastCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_stop_le_broadcast_fns = {
    twsTopology_ProcStopLeBroadcastStart,
    twsTopology_ProcStopLeBroadcastCancel,
};

twsTopProcStopLeBroadcastTaskData twstop_proc_stop_le_broadcast = {.task_data.handler = twsTopology_ProcStopLeBroadcastHandleMessage};

#define TwsTopProcStopLeBroadcastGetTaskData()     (&twstop_proc_stop_le_broadcast)
#define TwsTopProcStopLeBroadcastGetTask()         (&twstop_proc_stop_le_broadcast.task_data)

static void twsTopology_ProcStopLeBroadcastComplete(twsTopProcStopLeBroadcastTaskData *td)
{
    if (td->cancel_fn)
    {
        Procedures_DelayedCancelCfmCallback(td->cancel_fn, tws_topology_proc_stop_le_broadcast, procedure_result_success);
        DEBUG_LOG("twsTopology_ProcStopLeBroadcastComplete cancel complete");
    }
    else if (td->complete_fn)
    {
        Procedures_DelayedCompleteCfmCallback(td->complete_fn, tws_topology_proc_stop_le_broadcast, procedure_result_success);
        DEBUG_LOG("twsTopology_ProcStopLeBroadcastComplete complete");
    }
    td->cancel_fn = NULL;
    td->complete_fn = NULL;
}

static void twsTopology_ProcStopLeBroadcastStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    twsTopProcStopLeBroadcastTaskData* td = TwsTopProcStopLeBroadcastGetTaskData();

    UNUSED(result_task);
    UNUSED(goal_data);

    /* start the procedure */
    td->complete_fn = proc_complete_fn;
    td->cancel_fn = NULL;

    /* procedure starts synchronously so return TRUE */
    proc_start_cfm_fn(tws_topology_proc_stop_le_broadcast, procedure_result_success);

    DEBUG_LOG("twsTopology_ProcStopLeBroadcastStart");
#ifdef INCLUDE_LE_AUDIO_BROADCAST
    LeBroadcastManager_Stop(TwsTopProcStopLeBroadcastGetTask());
#else
    twsTopology_ProcStopLeBroadcastComplete(td);
#endif
}

static void twsTopology_ProcStopLeBroadcastCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    twsTopProcStopLeBroadcastTaskData* td = TwsTopProcStopLeBroadcastGetTaskData();

    DEBUG_LOG("twsTopology_ProcStopLeBroadcastCancel");

    /* Need to wait for the response from broadcast manager to complete */
    td->cancel_fn = proc_cancel_cfm_fn;
}

#ifdef INCLUDE_LE_AUDIO_BROADCAST
static void twsTopology_ProcStopLeBroadcastHandleStopCfm(twsTopProcStopLeBroadcastTaskData *td)
{
    UNUSED(td);

    LeBroadcastManager_RemoveAllSources(TwsTopProcStopLeBroadcastGetTask());
}

static void twsTopology_ProcStopLeBroadcastHandleRemoveAllSourcesCfm(twsTopProcStopLeBroadcastTaskData *td)
{
#ifdef INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN
    UNUSED(td);
    LeBroadcastManager_SelfScanStopAll(TwsTopProcStopLeBroadcastGetTask());
#else
    twsTopology_ProcStopLeBroadcastComplete(td);
#endif
}

static void twsTopology_ProcStopLeBroadcastHandleSelfScanStopAllCfm(twsTopProcStopLeBroadcastTaskData *td)
{
    twsTopology_ProcStopLeBroadcastComplete(td);
}
#endif

static void twsTopology_ProcStopLeBroadcastHandleMessage(Task task, MessageId id, Message message)
{
#ifdef INCLUDE_LE_AUDIO_BROADCAST
    twsTopProcStopLeBroadcastTaskData *td = STRUCT_FROM_MEMBER(twsTopProcStopLeBroadcastTaskData, task_data, task);

    UNUSED(message);
    DEBUG_LOG("twsTopology_ProcStopLeBroadcastHandleMessage: id=0x%04x", id);

    switch (id)
    {
        case LE_BROADCAST_MANAGER_STOP_CFM:
            twsTopology_ProcStopLeBroadcastHandleStopCfm(td);
            break;

        case LE_BROADCAST_MANAGER_REMOVE_ALL_SOURCES_CFM:
            twsTopology_ProcStopLeBroadcastHandleRemoveAllSourcesCfm(td);
            break;

        case LE_BROADCAST_MANAGER_SELF_SCAN_STOP_ALL_CFM:
            twsTopology_ProcStopLeBroadcastHandleSelfScanStopAllCfm(td);
            break;

        default:
            break;
    }
#else
    UNUSED(task);
    UNUSED(id);
    UNUSED(message);
#endif
}
