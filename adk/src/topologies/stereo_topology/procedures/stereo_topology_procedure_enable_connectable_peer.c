/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure to enable/disable BREDR connectable (pagescan) for peer connectivity.

            \note Other procedures can block connection at a lower level, for 
            instance disabling Bluetooth completely. These functions have the 
            effect of suspending the ability of a peer to connect.
*/

#include "stereo_topology_procedure_enable_connectable_peer.h"
#include "stereo_topology_procedure_pri_connect_peer_profiles.h"
#include "stereo_topology_config.h"
#include "le_advertising_manager.h"
#include "stereo_topology_procedure_send_topology_message.h"

#include <bredr_scan_manager.h>
#include <bredr_scan_manager_protected.h>
#include <connection_manager.h>
#include <bt_device.h>
#include <peer_find_role.h>

#include <logging.h>

#include <message.h>

/*! Parameter definition for connectable enable */
const ENABLE_CONNECTABLE_PEER_PARAMS_T proc_enable_connectable_peer_enable_fast =
{
    .enable = TRUE,
    .auto_disable = TRUE,
    .page_scan_type = SCAN_MAN_PARAMS_TYPE_FAST,
};
/*! Parameter definition for connectable enable, but without automatically disabling page scan on procedure completion */
const ENABLE_CONNECTABLE_PEER_PARAMS_T proc_enable_connectable_peer_enable_no_auto_disable_slow =
{
    .enable = TRUE,
    .auto_disable = FALSE,
    .page_scan_type = SCAN_MAN_PARAMS_TYPE_SLOW,
};
/*! Parameter definition for connectable disable */
const ENABLE_CONNECTABLE_PEER_PARAMS_T proc_enable_connectable_peer_disable =
{
    .enable = FALSE,
    .auto_disable = TRUE,
    .page_scan_type =  SCAN_MAN_PARAMS_TYPE_SLOW,
};

static void stereoTopology_ProcEnableConnectablePeerHandleMessage(Task task, MessageId id, Message message);
static void StereoTopology_ProcedureEnableConnectablePeerStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
static void StereoTopology_ProcedureEnableConnectablePeerCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_enable_connectable_peer_fns = {
    StereoTopology_ProcedureEnableConnectablePeerStart,
    StereoTopology_ProcedureEnableConnectablePeerCancel,
};

typedef struct
{
    TaskData task;
    ENABLE_CONNECTABLE_PEER_PARAMS_T params;
    procedure_complete_func_t complete_fn;
    procedure_cancel_cfm_func_t cancel_fn;
    bool active;
    procedure_result_t complete_status;
} stereoTopProcEnableConnectablePeerTaskData;

stereoTopProcEnableConnectablePeerTaskData stereotop_proc_enable_connectable_peer = {stereoTopology_ProcEnableConnectablePeerHandleMessage};

#define StereoTopProcEnableConnectablePeerGetTaskData()     (&stereotop_proc_enable_connectable_peer)
#define StereoTopProcEnableConnectablePeerGetTask()         (&stereotop_proc_enable_connectable_peer.task)

typedef enum
{
    /*! Timeout when the Secondary failed to connect ACL. */
    STEREO_TOP_PROC_ENABLE_CONNECTABLE_PEER_INTERNAL_CONNECT_TIMEOUT,

}stereo_top_proc_enable_connectable_peer_internal_message_t;

static void stereoTopology_ProcEnableConnectablePeerReset(void)
{
    stereoTopProcEnableConnectablePeerTaskData* td = StereoTopProcEnableConnectablePeerGetTaskData();

    MessageCancelFirst(StereoTopProcEnableConnectablePeerGetTask(), STEREO_TOP_PROC_ENABLE_CONNECTABLE_PEER_INTERNAL_CONNECT_TIMEOUT);
    ConManagerUnregisterTpConnectionsObserver(cm_transport_bredr, StereoTopProcEnableConnectablePeerGetTask());
    PeerFindRole_UnregisterTask(StereoTopProcEnableConnectablePeerGetTask());

    td->active = FALSE;
}

/*! Disable page scan on completion of this procedure, unless goal data specified not to do so. */
static void stereoTopology_ProcEnableConnectablePeerDisableOnCompletion(void)
{
    stereoTopProcEnableConnectablePeerTaskData* td = StereoTopProcEnableConnectablePeerGetTaskData();
    if (td->params.auto_disable)
    {
        BredrScanManager_PageScanRelease(StereoTopProcEnableConnectablePeerGetTask());
    }
}

static void StereoTopology_ProcedureEnableConnectablePeerStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    stereoTopProcEnableConnectablePeerTaskData* td = StereoTopProcEnableConnectablePeerGetTaskData();
    ENABLE_CONNECTABLE_PEER_PARAMS_T* params = (ENABLE_CONNECTABLE_PEER_PARAMS_T*)goal_data;

    UNUSED(result_task);

    /* start the procedure */
    td->params = *params;
    td->complete_fn = proc_complete_fn;
    td->cancel_fn = 0;
    td->complete_status = procedure_result_success;
    td->active = TRUE;

    /* Register to be able to receive PEER_FIND_ROLE_CANCELLED */
    PeerFindRole_RegisterTask(StereoTopProcEnableConnectablePeerGetTask());

    /* procedure starts synchronously so return TRUE */
    proc_start_cfm_fn(stereo_topology_procedure_enable_connectable_peer, procedure_result_success);

    /* Only wait for the peer to be connected if it is not already connected. There might be rare
     * instances(when we do quick incase/outcase) where the peer would have already connected
     * and the other earbud now switches to become primary from its current acting primary state. */
    if (td->params.enable && !appDeviceIsPeerConnected())
    {
        DEBUG_LOG("StereoTopology_ProcedureEnableConnectablePeerStart ENABLE");

        BredrScanManager_PageScanRequest(StereoTopProcEnableConnectablePeerGetTask(), td->params.page_scan_type);
        /* register to get notified of connection to peer and start timeout for the secondary to establish the ACL */
        ConManagerRegisterTpConnectionsObserver(cm_transport_bredr, StereoTopProcEnableConnectablePeerGetTask());
        MessageSendLater(StereoTopProcEnableConnectablePeerGetTask(), STEREO_TOP_PROC_ENABLE_CONNECTABLE_PEER_INTERNAL_CONNECT_TIMEOUT,
                         NULL, StereoTopologyConfig_PrimaryPeerConnectTimeoutMs());

    }
    else
    {
        DEBUG_LOG("StereoTopology_ProcedureEnableConnectablePeerStart DISABLE");

        BredrScanManager_PageScanRelease(StereoTopProcEnableConnectablePeerGetTask());

        /* Nothing more to do so set the completion status now. */
        td->complete_status = procedure_result_success;

        /* Cancel any ongoing find role; wait for it to be confirmed */
        PeerFindRole_FindRoleCancel();
    }
}

static void StereoTopology_ProcedureEnableConnectablePeerCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    stereoTopProcEnableConnectablePeerTaskData* td = StereoTopProcEnableConnectablePeerGetTaskData();

    DEBUG_LOG("StereoTopology_ProcedureEnableConnectablePeerCancel");

    if (td->params.enable)
    {
        /* procedure was enabling page scan, so disable on cancel requeest */
        stereoTopology_ProcEnableConnectablePeerDisableOnCompletion();

        stereoTopology_ProcEnableConnectablePeerReset();
        Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, stereo_topology_procedure_enable_connectable_peer, procedure_result_success);
    }
    else
    {
        /* Need to wait for the PeerFindRoleFindRoleCancel() to complete */
        td->cancel_fn = proc_cancel_cfm_fn;
    }
}

static void stereoTopology_ProcEnableConnectablePeerHandleConManagerTpConnectInd(CON_MANAGER_TP_CONNECT_IND_T* ind)
{
    stereoTopProcEnableConnectablePeerTaskData* td = StereoTopProcEnableConnectablePeerGetTaskData();
    bdaddr peer_addr;

    appDeviceGetPeerBdAddr(&peer_addr);
    if (BdaddrIsSame(&ind->tpaddr.taddr.addr, &peer_addr))
    {
        DEBUG_LOG("stereoTopology_ProcEnableConnectablePeerHandleConManagerTpConnectInd");

        /* link is up to the peer now, so disable page scan */
        stereoTopology_ProcEnableConnectablePeerDisableOnCompletion();

        MessageCancelFirst(StereoTopProcEnableConnectablePeerGetTask(), STEREO_TOP_PROC_ENABLE_CONNECTABLE_PEER_INTERNAL_CONNECT_TIMEOUT);

        /* Nothing more to do so set the completion status now. */
        td->complete_status = procedure_result_success;

        /* Cancel an ongoing find role - we have connected */
        PeerFindRole_FindRoleCancel();
    }
}

static void stereoTopology_ProcEnableConnectablePeerHandleConnectTimeout(void)
{
    stereoTopProcEnableConnectablePeerTaskData* td = StereoTopProcEnableConnectablePeerGetTaskData();
    DEBUG_LOG("stereoTopology_ProcEnableConnectablePeerHandleConnectTimeout");

    /* only get a timeout for enable operation, so just run disable now */
    stereoTopology_ProcEnableConnectablePeerDisableOnCompletion();

    /* Nothing more to do so set the completion status now. */
    td->complete_status = procedure_result_timeout;

    /* Cancel an ongoing find role - new one should be requested */
    PeerFindRole_FindRoleCancel();
}

static void stereoTopology_ProcEnableConnectablePeerHandlePeerFindRoleCancelled(void)
{
    stereoTopProcEnableConnectablePeerTaskData* td = StereoTopProcEnableConnectablePeerGetTaskData();

    DEBUG_LOG("stereoTopology_ProcEnableConnectablePeerHandlePeerFindRoleCancelled");

    if (td->cancel_fn)
    {
        Procedures_DelayedCancelCfmCallback(td->cancel_fn, stereo_topology_procedure_enable_connectable_peer, procedure_result_success);
    }
    else
    {
        /* The td->complete_status must have been set by the code that called
           PeerFindRole_FindRoleCancel */
        Procedures_DelayedCompleteCfmCallback(td->complete_fn, stereo_topology_procedure_enable_connectable_peer, td->complete_status);
    }

    stereoTopology_ProcEnableConnectablePeerReset();
}

static void stereoTopology_ProcEnableConnectablePeerHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        case CON_MANAGER_TP_CONNECT_IND:
            stereoTopology_ProcEnableConnectablePeerHandleConManagerTpConnectInd((CON_MANAGER_TP_CONNECT_IND_T*)message);
            break;

        case STEREO_TOP_PROC_ENABLE_CONNECTABLE_PEER_INTERNAL_CONNECT_TIMEOUT:
            stereoTopology_ProcEnableConnectablePeerHandleConnectTimeout();
            break;

        case PEER_FIND_ROLE_CANCELLED:
            stereoTopology_ProcEnableConnectablePeerHandlePeerFindRoleCancelled();
            break;

        default:
            DEBUG_LOG("stereoTopology_ProcEnableConnectablePeerHandleMessage id MESSAGE:0x%x", id);
            break;
    }
}

#define CONNECT_PEER_SCRIPT(ENTRY) \
    ENTRY(proc_enable_connectable_peer_fns, PROC_ENABLE_CONNECTABLE_PEER_DATA_ENABLE_FAST), \
    ENTRY(proc_pri_connect_peer_profiles_fns, PROC_PEER_PROFILE_CONNECT), \
    ENTRY(stereo_proc_send_topology_message_fns, PROC_SEND_STEREO_TOPOLOGY_MESSAGE_PEER_PROFILE_CONN_FINISHED_MESSAGE)
    


/* Define the enable version of this procedure as a script */
DEFINE_TOPOLOGY_SCRIPT(stereo_primary_connect_peer, CONNECT_PEER_SCRIPT);


