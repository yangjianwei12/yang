/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Procedure to stop LE broadcast.
*/

#include "stereo_topology_procedure_stop_le_broadcast.h"
#include "stereo_topology_procedures.h"
#include "stereo_topology_config.h"
#include "procedures.h"

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
} stereoTopProcStopLeBroadcastTaskData;

static void stereoTopology_ProcStopLeBroadcastHandleMessage(Task task, MessageId id, Message message);
static void stereoTopology_ProcStopLeBroadcastStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
static void stereoTopology_ProcStopLeBroadcastCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t stereo_proc_stop_le_broadcast_fns = {
    stereoTopology_ProcStopLeBroadcastStart,
    stereoTopology_ProcStopLeBroadcastCancel,
};

stereoTopProcStopLeBroadcastTaskData stereotop_proc_stop_le_broadcast = {stereoTopology_ProcStopLeBroadcastHandleMessage};

#define stereoTopProcStopLeBroadcastGetTaskData()     (&stereotop_proc_stop_le_broadcast)
#define stereoTopProcStopLeBroadcastGetTask()         (&stereotop_proc_stop_le_broadcast.task_data)

static void stereoTopology_ProcStopLeBroadcastComplete(stereoTopProcStopLeBroadcastTaskData *td)
{
    if (td->cancel_fn)
    {
        Procedures_DelayedCancelCfmCallback(td->cancel_fn, stereo_topology_proc_stop_le_broadcast, procedure_result_success);
        DEBUG_LOG("stereoTopology_ProcStopLeBroadcastComplete cancel complete");
    }
    else if (td->complete_fn)
    {
        Procedures_DelayedCompleteCfmCallback(td->complete_fn, stereo_topology_proc_stop_le_broadcast, procedure_result_success);
        DEBUG_LOG("stereoTopology_ProcStopLeBroadcastComplete complete");
    }
    td->cancel_fn = NULL;
    td->complete_fn = NULL;
}

static void stereoTopology_ProcStopLeBroadcastStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    stereoTopProcStopLeBroadcastTaskData* td = stereoTopProcStopLeBroadcastGetTaskData();

    UNUSED(result_task);
    UNUSED(goal_data);

    /* start the procedure */
    td->complete_fn = proc_complete_fn;
    td->cancel_fn = NULL;

    /* procedure starts synchronously so return TRUE */
    proc_start_cfm_fn(stereo_topology_proc_stop_le_broadcast, procedure_result_success);

    DEBUG_LOG("stereoTopology_ProcStopLeBroadcastStart");
#ifdef INCLUDE_LE_AUDIO_BROADCAST
    LeBroadcastManager_Stop(stereoTopProcStopLeBroadcastGetTask());
#else
    stereoTopology_ProcStopLeBroadcastComplete(td);
#endif
}

static void stereoTopology_ProcStopLeBroadcastCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    stereoTopProcStopLeBroadcastTaskData* td = stereoTopProcStopLeBroadcastGetTaskData();

    DEBUG_LOG("stereoTopology_ProcStopLeBroadcastCancel");

    /* Need to wait for the response from broadcast manager to complete */
    td->cancel_fn = proc_cancel_cfm_fn;
}

#ifdef INCLUDE_LE_AUDIO_BROADCAST
static void stereoTopology_ProcStopLeBroadcastHandleStopCfm(stereoTopProcStopLeBroadcastTaskData *td)
{
    UNUSED(td);

    LeBroadcastManager_RemoveAllSources(stereoTopProcStopLeBroadcastGetTask());
}

static void stereoTopology_ProcStopLeBroadcastHandleRemoveAllSourcesCfm(stereoTopProcStopLeBroadcastTaskData *td)
{
    stereoTopology_ProcStopLeBroadcastComplete(td);
}
#endif

static void stereoTopology_ProcStopLeBroadcastHandleMessage(Task task, MessageId id, Message message)
{
#ifdef INCLUDE_LE_AUDIO_BROADCAST
    stereoTopProcStopLeBroadcastTaskData *td = STRUCT_FROM_MEMBER(stereoTopProcStopLeBroadcastTaskData, task_data, task);

    UNUSED(message);

    switch (id)
    {
        case LE_BROADCAST_MANAGER_STOP_CFM:
            stereoTopology_ProcStopLeBroadcastHandleStopCfm(td);
            break;

        case LE_BROADCAST_MANAGER_REMOVE_ALL_SOURCES_CFM:
            stereoTopology_ProcStopLeBroadcastHandleRemoveAllSourcesCfm(td);
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
