/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure to enable LE connections to the handset

            \note this allows the connection if one arrives, but does not 
            make any changes to allow a connection
*/

#include "stereo_topology_procedure_setup_le_peer_pair.h"

#include <le_advertising_manager.h>
#include <connection_manager.h>
#include <le_scan_manager_protected.h>

#include <logging.h>

#include <message.h>
#include <panic.h>

#define STEREO_TOP_PROC_SETUP_LE_PEER_PAIR_LE_SCAN         (1 << 0)
#define STEREO_TOP_PROC_SETUP_LE_PEER_PAIR_ADVERTISING     (1 << 1)


/*! Parameter definition for LE connectable enable */
const SETUP_LE_PEER_PAIR_PARAMS_T stereo_topology_procedure_setup_le_peer_pair_enable = { .enable = TRUE };

static void stereoTopology_ProcSetupLePeerPairHandleMessage(Task task, MessageId id, Message message);
static void StereoTopology_ProcSetupLePeerPairStart(Task result_task,
                                                                   procedure_start_cfm_func_t proc_start_cfm_fn,
                                                                   procedure_complete_func_t proc_complete_fn,
                                                                   Message goal_data);

static void StereoTopology_ProcSetupLePeerPairCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);


typedef struct
{
    TaskData task;
    procedure_complete_func_t complete_fn;    
    procedure_cancel_cfm_func_t cancel_fn;
    uint16 pending;     /*!< Activities we are waiting for a response for */
    bool enabling;      /*!< Whether we are enabling or disabling */
} stereotop_proc_setup_le_peer_pair_task_data_t;

const procedure_fns_t stereo_proc_setup_le_peer_pair_fns =
{
    StereoTopology_ProcSetupLePeerPairStart,
    StereoTopology_ProcSetupLePeerPairCancel,
};

stereotop_proc_setup_le_peer_pair_task_data_t stereotop_proc_setup_le_peer_pair = {stereoTopology_ProcSetupLePeerPairHandleMessage};


#define StereoTopProcSetupLEPeerPairGetTaskData()     (&stereotop_proc_setup_le_peer_pair)
#define StereoTopProcSetupLEPeerPairGetTask()         (&stereotop_proc_setup_le_peer_pair.task)

typedef enum
{
     /*! peer pair setup functions have completed */
    STEREO_TOP_PROC_SETUP_LE_PEER_PAIR_INTERNAL_COMPLETED,
} stereo_top_proc_setup_le_peer_pair_internal_message_t;


static void StereoTopology_ProcSetupLePeerPairStart(Task result_task,
                                                                   procedure_start_cfm_func_t proc_start_cfm_fn,
                                                                   procedure_complete_func_t proc_complete_fn,
                                                                   Message goal_data)
{
    SETUP_LE_PEER_PAIR_PARAMS_T* params = (SETUP_LE_PEER_PAIR_PARAMS_T*)goal_data;
    
    stereotop_proc_setup_le_peer_pair_task_data_t *td = StereoTopProcSetupLEPeerPairGetTaskData();

    UNUSED(result_task);

    td->complete_fn = proc_complete_fn;
    td->enabling = params->enable;

    DEBUG_LOG_VERBOSE("StereoTopology_ProcSetupLePeerPairStart ENABLE: %d", params->enable);

    ConManagerAllowConnection(cm_transport_ble, params->enable);

    if (params->enable)
    {
        PanicFalse(LeAdvertisingManager_AllowAdvertising(StereoTopProcSetupLEPeerPairGetTask(), TRUE));
        LeScanManager_Enable(StereoTopProcSetupLEPeerPairGetTask());
        /* only once we are sure we can scan & adv LE, we are done with LE peer pair setup */
        td->pending =   STEREO_TOP_PROC_SETUP_LE_PEER_PAIR_LE_SCAN
                       | STEREO_TOP_PROC_SETUP_LE_PEER_PAIR_ADVERTISING;
    }
    else
    {
        DEBUG_LOG_VERBOSE("Expecting for now just setup of peer-pairing which should enable LE Scan & Adv");
        Panic();
    }
    MessageSendConditionally(StereoTopProcSetupLEPeerPairGetTask(),
                             STEREO_TOP_PROC_SETUP_LE_PEER_PAIR_INTERNAL_COMPLETED, NULL, 
                             &td->pending);

    /* procedure started synchronously so indicate success */
    proc_start_cfm_fn(stero_topology_procedure_setup_le_peer_pair, procedure_result_success);
}

static void StereoTopology_ProcSetupLePeerPairCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    stereotop_proc_setup_le_peer_pair_task_data_t *td = StereoTopProcSetupLEPeerPairGetTaskData();

    DEBUG_LOG_VERBOSE("StereoTopology_ProcSetupLePeerPairCancel");
    td->complete_fn = NULL;
    td->pending = 0;

    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn,
                                  stero_topology_procedure_setup_le_peer_pair,
                                  procedure_result_success);
}

static void stereoTopology_ProcSetupLePeerPairHandleLeAdvManagerAllowCfm(const LE_ADV_MGR_ALLOW_ADVERTISING_CFM_T *cfm)
{
    stereotop_proc_setup_le_peer_pair_task_data_t *td = StereoTopProcSetupLEPeerPairGetTaskData();

    DEBUG_LOG("stereoTopology_ProcSetupLePeerPairHandleLeAdvManagerAllowCfm %u", cfm->status);

    if (le_adv_mgr_status_success == cfm->status)
    {
        if (cfm->allow == td->enabling)
        {
            td->pending &= ~STEREO_TOP_PROC_SETUP_LE_PEER_PAIR_ADVERTISING;
        }
        else
        {
            DEBUG_LOG("stereoTopology_ProcSetupLePeerPairHandleLeAdvManagerAllowCfm Response not for expected operation");
        }
    }
    else
    {
        Panic();
    }
}

static void stereoTopology_ProcSetupLePeerPairHandleLeScanManagerEnableCfm(const LE_SCAN_MANAGER_ENABLE_CFM_T *cfm)
{
    stereotop_proc_setup_le_peer_pair_task_data_t *td = StereoTopProcSetupLEPeerPairGetTaskData();

    DEBUG_LOG("stereoTopology_ProcSetupLePeerPairHandleLeScanManagerEnableCfm %u", cfm->status);

    if (LE_SCAN_MANAGER_RESULT_SUCCESS == cfm->status)
    {
        if (td->enabling)
        {
            td->pending &= ~STEREO_TOP_PROC_SETUP_LE_PEER_PAIR_LE_SCAN;
        }
        else
        {
            DEBUG_LOG("stereoTopology_ProcSetupLePeerPairHandleLeScanManagerEnableCfm Enabled, but waiting for disable !!");
        }
    }
    else
    {
        Panic();
    }
}



static void stereoTopology_ProcSetupLePeerPairHandleMessage(Task task, MessageId id, Message message)
{
    stereotop_proc_setup_le_peer_pair_task_data_t* td = StereoTopProcSetupLEPeerPairGetTaskData();

    UNUSED(task);

    DEBUG_LOG_VERBOSE("stereoTopology_ProcSetupLePeerPairHandleMessage id MESSAGE:0x%x", id);

    if (!td->complete_fn)
    {
        DEBUG_LOG_VERBOSE("delayed response, procedure cancelled, ignore id MESSAGE:0x%x", id);
        return;
    }
    switch (id)
    {
        case STEREO_TOP_PROC_SETUP_LE_PEER_PAIR_INTERNAL_COMPLETED:
            /*! \todo This is naive. Cancel needs to be dealt with */
            td->complete_fn(stero_topology_procedure_setup_le_peer_pair, procedure_result_success);
            td->complete_fn = NULL;
            break;


        case LE_ADV_MGR_ALLOW_ADVERTISING_CFM:
            stereoTopology_ProcSetupLePeerPairHandleLeAdvManagerAllowCfm((const LE_ADV_MGR_ALLOW_ADVERTISING_CFM_T *)message);
            break;

        case LE_SCAN_MANAGER_ENABLE_CFM:
            stereoTopology_ProcSetupLePeerPairHandleLeScanManagerEnableCfm((const LE_SCAN_MANAGER_ENABLE_CFM_T *)message);
            break;

        default:
            break;
    }
}


