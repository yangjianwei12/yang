/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Procedure to stop handset reconnections.
*/

#include "stereo_topology_procedure_stop_handset_reconnect.h"

#include <handset_service.h>
#include <logging.h>

#include <message.h>
#include <panic.h>

void StereoTopology_ProcedureStopHandsetReconnectStart(Task result_task,
                                                  procedure_start_cfm_func_t proc_start_cfm_fn,
                                                  procedure_complete_func_t proc_complete_fn,
                                                  Message goal_data);
static void StereoTopology_ProcedureStopHandsetReconnectCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

static void stereoTopology_ProcedureStopHandsetReconnectHandler(Task task, MessageId id, Message message);

const procedure_fns_t stereo_proc_stop_handset_reconnect_fns = {
    StereoTopology_ProcedureStopHandsetReconnectStart,
    StereoTopology_ProcedureStopHandsetReconnectCancel,
};

const TaskData stereotop_stop_handset_reconnect_task = {
    .handler = stereoTopology_ProcedureStopHandsetReconnectHandler
};

static procedure_complete_func_t stereotop_proc_stop_handset_reconnect_complete_fn;

void StereoTopology_ProcedureStopHandsetReconnectStart(Task result_task,
                                                  procedure_start_cfm_func_t proc_start_cfm_fn,
                                                  procedure_complete_func_t proc_complete_fn,
                                                  Message goal_data)
{
    UNUSED(result_task);
    UNUSED(goal_data);

    DEBUG_LOG("StereoTopology_ProcedureStopHandsetReconnectStart");

    HandsetService_StopReconnect((Task)&stereotop_stop_handset_reconnect_task);
    stereotop_proc_stop_handset_reconnect_complete_fn = proc_complete_fn;
    proc_start_cfm_fn(stereo_topology_procedure_stop_handset_reconnect,
                      procedure_result_success);
}

void StereoTopology_ProcedureStopHandsetReconnectCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("StereoTopology_ProcedureStopHandsetReconnectCancel");

    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn,
                                        stereo_topology_procedure_stop_handset_reconnect,
                                        procedure_result_success);
}

static void stereoTopology_ProcedureStopHandsetReconnectHandleConnectStopCfm(const HANDSET_SERVICE_MP_CONNECT_STOP_CFM_T* cfm)
{
    DEBUG_LOG("stereoTopology_ProcConnectHandsetHandleHandsetMpConnectStopCfm status enum:handset_service_status_t:%d", cfm->status);

    Procedures_DelayedCompleteCfmCallback(stereotop_proc_stop_handset_reconnect_complete_fn,
                                          stereo_topology_procedure_stop_handset_reconnect,
                                          procedure_result_success);

}

static void stereoTopology_ProcedureStopHandsetReconnectHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        case HANDSET_SERVICE_MP_CONNECT_STOP_CFM:
            stereoTopology_ProcedureStopHandsetReconnectHandleConnectStopCfm((const HANDSET_SERVICE_MP_CONNECT_STOP_CFM_T *)message);
            break;

        default:
            DEBUG_LOG("stereoTopology_ProcedureStopHandsetReconnectHandler unhandled id MESSAGE:0x%x", id);
            break;
    }
}

