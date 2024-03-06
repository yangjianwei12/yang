/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure to enable/disable BREDR connectable (pagescan) for peer connectivity.

            \note Other procedures can block connection at a lower level, for 
            instance disabling Bluetooth completely. These functions have the 
            effect of suspending the ability of a peer to connect.
*/

#include "tws_topology_procedure_enable_connectable_peer.h"
#include "tws_topology_procedures.h"
#include "tws_topology_config.h"
#include "tws_topology_private.h"

#include "le_advertising_manager.h"

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

static void twsTopology_ProcEnableConnectablePeerHandleMessage(Task task, MessageId id, Message message);
static void TwsTopology_ProcedureEnableConnectablePeerStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
static void TwsTopology_ProcedureEnableConnectablePeerCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_enable_connectable_peer_fns = {
    TwsTopology_ProcedureEnableConnectablePeerStart,
    TwsTopology_ProcedureEnableConnectablePeerCancel,
};

typedef struct
{
    TaskData task;
    ENABLE_CONNECTABLE_PEER_PARAMS_T params;
    procedure_complete_func_t complete_fn;
    procedure_cancel_cfm_func_t cancel_fn;
    bool active;
    procedure_result_t complete_status;
} twsTopProcEnableConnectablePeerTaskData;

twsTopProcEnableConnectablePeerTaskData twstop_proc_enable_connectable_peer = {twsTopology_ProcEnableConnectablePeerHandleMessage};

#define TwsTopProcEnableConnectablePeerGetTaskData()     (&twstop_proc_enable_connectable_peer)
#define TwsTopProcEnableConnectablePeerGetTask()         (&twstop_proc_enable_connectable_peer.task)

typedef enum
{
    /*! Timeout when the Secondary Earbud failed to connect ACL. */
    TWS_TOP_PROC_ENABLE_CONNECTABLE_PEER_INTERNAL_CONNECT_TIMEOUT,

} tws_top_proc_enable_connectable_peer_internal_message_t;

static void twsTopology_ProcEnableConnectablePeerReset(void)
{
    twsTopProcEnableConnectablePeerTaskData* td = TwsTopProcEnableConnectablePeerGetTaskData();

    MessageCancelFirst(TwsTopProcEnableConnectablePeerGetTask(), TWS_TOP_PROC_ENABLE_CONNECTABLE_PEER_INTERNAL_CONNECT_TIMEOUT);
    ConManagerUnregisterTpConnectionsObserver(cm_transport_bredr, TwsTopProcEnableConnectablePeerGetTask());
    PeerFindRole_UnregisterTask(TwsTopProcEnableConnectablePeerGetTask());

    td->active = FALSE;
}

/*! Disable page scan on completion of this procedure, unless goal data specified not to do so. */
static void twsTopology_ProcEnableConnectablePeerDisableOnCompletion(void)
{
    twsTopProcEnableConnectablePeerTaskData* td = TwsTopProcEnableConnectablePeerGetTaskData();
    if (td->params.auto_disable)
    {
        BredrScanManager_PageScanRelease(TwsTopProcEnableConnectablePeerGetTask());
    }
}

static void TwsTopology_ProcedureEnableConnectablePeerStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    twsTopProcEnableConnectablePeerTaskData* td = TwsTopProcEnableConnectablePeerGetTaskData();
    ENABLE_CONNECTABLE_PEER_PARAMS_T* params = (ENABLE_CONNECTABLE_PEER_PARAMS_T*)goal_data;

    UNUSED(result_task);

    /* start the procedure */
    td->params = *params;
    td->complete_fn = proc_complete_fn;
    td->cancel_fn = 0;
    td->complete_status = procedure_result_success;
    td->active = TRUE;

    /* Register to be able to receive PEER_FIND_ROLE_CANCELLED */
    PeerFindRole_RegisterTask(TwsTopProcEnableConnectablePeerGetTask());

    /* procedure starts synchronously so return TRUE */
    proc_start_cfm_fn(tws_topology_procedure_enable_connectable_peer, procedure_result_success);

    /* Only wait for the peer to be connected if it is not already connected. There might be rare
     * instances(when we do quick incase/outcase) where the peer would have already connected
     * and the other earbud now switches to become primary from its current standalone primary state. */
    if (td->params.enable && !appDeviceIsPeerConnected())
    {
        DEBUG_LOG("TwsTopology_ProcedureEnableConnectablePeerStart ENABLE");

        BredrScanManager_PageScanRequest(TwsTopProcEnableConnectablePeerGetTask(), td->params.page_scan_type);
        /* register to get notified of connection to peer and start timeout for the secondary to establish the ACL */
        ConManagerRegisterTpConnectionsObserver(cm_transport_bredr, TwsTopProcEnableConnectablePeerGetTask());
        MessageSendLater(TwsTopProcEnableConnectablePeerGetTask(), TWS_TOP_PROC_ENABLE_CONNECTABLE_PEER_INTERNAL_CONNECT_TIMEOUT,
                         NULL, D_SEC(twsTopology_GetProductBehaviour()->timeouts.primary_wait_for_secondary_sec));
    }
    else
    {
        DEBUG_LOG("TwsTopology_ProcedureEnableConnectablePeerStart DISABLE");

        BredrScanManager_PageScanRelease(TwsTopProcEnableConnectablePeerGetTask());

        /* Nothing more to do so set the completion status now. */
        td->complete_status = procedure_result_success;

        /* Cancel any ongoing find role; wait for it to be confirmed */
        PeerFindRole_FindRoleCancel();
    }
}

static void TwsTopology_ProcedureEnableConnectablePeerCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    twsTopProcEnableConnectablePeerTaskData* td = TwsTopProcEnableConnectablePeerGetTaskData();

    DEBUG_LOG("TwsTopology_ProcedureEnableConnectablePeerCancel");

    if (td->params.enable)
    {
        /* procedure was enabling page scan, so disable on cancel requeest */
        twsTopology_ProcEnableConnectablePeerDisableOnCompletion();

        twsTopology_ProcEnableConnectablePeerReset();
        Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_enable_connectable_peer, procedure_result_success);
    }
    else
    {
        /* Need to wait for the PeerFindRoleFindRoleCancel() to complete */
        td->cancel_fn = proc_cancel_cfm_fn;
    }
}

static void twsTopology_ProcEnableConnectablePeerHandleConManagerTpConnectInd(CON_MANAGER_TP_CONNECT_IND_T* ind)
{
    twsTopProcEnableConnectablePeerTaskData* td = TwsTopProcEnableConnectablePeerGetTaskData();
    bdaddr peer_addr;

    appDeviceGetPeerBdAddr(&peer_addr);
    if (BdaddrIsSame(&ind->tpaddr.taddr.addr, &peer_addr))
    {
        DEBUG_LOG("twsTopology_ProcEnableConnectablePeerHandleConManagerTpConnectInd");

        /* link is up to the peer now, so disable page scan */
        twsTopology_ProcEnableConnectablePeerDisableOnCompletion();

        /* Nothing more to do so set the completion status now. */
        td->complete_status = procedure_result_success;

        /* Cancel an ongoing find role - we have connected */
        PeerFindRole_FindRoleCancel();
    }
}

static void twsTopology_ProcEnableConnectablePeerHandleConnectTimeout(void)
{
    twsTopProcEnableConnectablePeerTaskData* td = TwsTopProcEnableConnectablePeerGetTaskData();
    DEBUG_LOG("twsTopology_ProcEnableConnectablePeerHandleConnectTimeout");

    /* only get a timeout for enable operation, so just run disable now */
    twsTopology_ProcEnableConnectablePeerDisableOnCompletion();

    /* Nothing more to do so set the completion status now. */
    td->complete_status = procedure_result_timeout;

    /* Cancel an ongoing find role - new one should be requested */
    PeerFindRole_FindRoleCancel();
}

static void twsTopology_ProcEnableConnectablePeerHandlePeerFindRoleCancelled(void)
{
    twsTopProcEnableConnectablePeerTaskData* td = TwsTopProcEnableConnectablePeerGetTaskData();

    DEBUG_LOG("twsTopology_ProcEnableConnectablePeerHandlePeerFindRoleCancelled");

    if (td->cancel_fn)
    {
        Procedures_DelayedCancelCfmCallback(td->cancel_fn, tws_topology_procedure_enable_connectable_peer, procedure_result_success);
    }
    else
    {
        /* The td->complete_status must have been set by the code that called
           PeerFindRole_FindRoleCancel */
        Procedures_DelayedCompleteCfmCallback(td->complete_fn, tws_topology_procedure_enable_connectable_peer, td->complete_status);
    }

    twsTopology_ProcEnableConnectablePeerReset();
}

static void twsTopology_ProcEnableConnectablePeerHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        case CON_MANAGER_TP_CONNECT_IND:
            twsTopology_ProcEnableConnectablePeerHandleConManagerTpConnectInd((CON_MANAGER_TP_CONNECT_IND_T*)message);
            break;

        case TWS_TOP_PROC_ENABLE_CONNECTABLE_PEER_INTERNAL_CONNECT_TIMEOUT:
            twsTopology_ProcEnableConnectablePeerHandleConnectTimeout();
            break;

        case PEER_FIND_ROLE_CANCELLED:
            twsTopology_ProcEnableConnectablePeerHandlePeerFindRoleCancelled();
            break;

        default:
            DEBUG_LOG("twsTopology_ProcEnableConnectablePeerHandleMessage id MESSAGE:0x%x", id);
            break;
    }
}

#define ENABLE_CONNECTABLE_PEER_SCRIPT(ENTRY) \
    ENTRY(proc_enable_connectable_peer_fns, PROC_ENABLE_CONNECTABLE_PEER_DATA_ENABLE_FAST)

/* Define the enable version of this procedure as a script */
DEFINE_TOPOLOGY_SCRIPT(enable_connectable_peer_fast, ENABLE_CONNECTABLE_PEER_SCRIPT);

