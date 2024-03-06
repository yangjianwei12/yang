/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      TWS Topology component core.
*/

#include "tws_topology.h"
#include "tws_topology_private.h"
#include "tws_topology_role_change_client_notifier.h"
#include "tws_topology_peer_sig.h"
#include "tws_topology_config.h"
#include "tws_topology_sdp.h"
#include "tws_topology_peer_sig.h"
#include "tws_topology_advertising.h"
#include "tws_topology_client_msgs.h"

#include "gatt_connect.h"
#include <hdma.h>
#include <peer_find_role.h>
#include <pairing.h>
#include <bt_device.h>
#include <mirror_profile.h>
#include <peer_signalling.h>
#include <le_advertising_manager.h>

#include <task_list.h>
#include <logging.h>
#include <message.h>

#include <panic.h>
#include <bdaddr.h>

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(tws_topology_message_t)
LOGGING_PRESERVE_MESSAGE_TYPE(tws_topology_internal_message_t)
LOGGING_PRESERVE_MESSAGE_TYPE(tws_topology_client_notifier_message_t)
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(TWS_TOPOLOGY, TWS_TOPOLOGY_MESSAGE_END)
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(TWS_TOPOLOGY_CLIENT_NOTIFIER, TWS_TOPOLOGY_CLIENT_NOTIFIER_MESSAGE_END)


/*! Instance of the TWS Topology. */
twsTopologyTaskData tws_topology = {0};

#ifdef ENABLE_LE_HANDOVER

static void twsTopology_OnGattConnectDisconnect(gatt_cid_t cid);
static void twsTopology_OnEncryptionChanged(gatt_cid_t cid, bool encrypted);

static const gatt_connect_observer_callback_t tws_topology_gatt_callback =
{
    .OnConnection = twsTopology_OnGattConnectDisconnect,
    .OnDisconnection = twsTopology_OnGattConnectDisconnect,
    .OnEncryptionChanged = twsTopology_OnEncryptionChanged
};

/*! \brief Callback function to handle GATT Connect notification */
static void twsTopology_OnGattConnectDisconnect(gatt_cid_t cid)
{
    UNUSED(cid);
}

/*! \brief Callback function to handle encryption change notification for a GATT connection */
static void twsTopology_OnEncryptionChanged(gatt_cid_t cid, bool encrypted)
{
    /* If encryption change indication is from a handset, queue a message to kick the topology sm */
    if (encrypted && BtDevice_IsDeviceHandsetOrLeHandset(GattConnect_GetBtLeDevice(cid)))
    {
        twsTopology_SmEnqueueKick(TwsTopology_GetSm());
    }
}

#endif /* ENABLE_LE_HANDOVER */

static void twsTopology_HandlePeerJoinSignalling(const PEER_SIG_CONNECTION_IND_T *ind)
{
    if(ind->status == peerSigStatusConnected && TwsTopology_IsJoinStatusWaitingTxToPeer())
    {
        TwsTopology_PeerSignalTopologyCmd(peer_joined_topology);
        TwsTopology_ClearJoinStatusWaitingTxToPeer();
    }
}

/*! \brief Handle ACL (dis)connections. */
static void twsTopology_HandleConManagerConnectionInd(const CON_MANAGER_CONNECTION_IND_T* ind)
{
    if(!ind->ble)
    {
        twsTopology_UpdateAdvertisingParams();

        if (appDeviceIsPeer(&ind->bd_addr))
        {
            if (!ind->connected)
            {
                twsTopology_SmHandlePeerLinkDisconnected(TwsTopology_GetSm());
            }
            else
            {
               /* There can be rare instances when we perform quick incase/outcase of the earbuds:
                *
                * - One earbud which was elected Secondary in the earlier peer-find-role stops
                *   advertising/scanning and starts paging.
                * - The other earbud is quickly put in case and then out of case.
                *   It becomes Standalone Primary after a few seconds since it won't be able to find the peer.
                *   It starts page scanning for handset connections.
                * - Now, the Secondary's paging succeeds and it gets connected to this Standalone Primary.
                *
                * In such cases, the earbud which is Standalone Primary should switch
                * to become Primary since it is now connected to the Secondary. */
                if(TwsTopology_GetSm()->state == TWS_TOPOLOGY_STATE_STANDALONE_PRIMARY)
                {
                    twsTopology_SmHandleElectedPrimaryWithPeer(TwsTopology_GetSm());
                }
            }
            twsTopology_SmKick(TwsTopology_GetSm());
        }
    }
}

static void twsTopology_HandlePairingActivity(const PAIRING_ACTIVITY_T *message)
{
    DEBUG_LOG("twsTopology_HandlePairingActivity status=enum:pairingActivityStatus:%d",
                message->status);

    switch(message->status)
    {
        case pairingActivitySuccess:
            /* just completed pairing, kick the SM, because it might need to start HDMA,
            * necessary because the normal checks to start HDMA triggered by
            * CON_MANAGER_CONNECTION_INDs will not succeed immediately after pairing
            * because the link type would not have been known to be a handset */
            twsTopology_SmKick(TwsTopology_GetSm());
            break;

        case pairingActivityInProgress:
        case pairingActivityNotInProgress:
        {
            twsTopology_UpdateAdvertisingParams();
        }
        break;
        default:
            break;
    }
}

#ifdef USE_SYNERGY
static void TwsTopology_HandleCmPrim(Task task, Message message)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    switch (*prim)
    {
        case CSR_BT_CM_SDS_REGISTER_CFM:
        {
            CsrBtCmSdsRegisterCfm *cfm = (CsrBtCmSdsRegisterCfm*)message;

            if (cfm->resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                DEBUG_LOG("TwsTopology_HandleCmPrim CSR_BT_CM_SDS_REGISTER_CFM result:0x%04x supplier:0x%04x",
                          cfm->resultCode, cfm->resultSupplier);
            }

            TwsTopology_HandleSdpRegisterCfm(task,cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS, cfm->serviceRecHandle);
        }
        break;

        case CSR_BT_CM_SDS_UNREGISTER_CFM:
        {
            CsrBtCmSdsUnregisterCfm *cfm = (CsrBtCmSdsUnregisterCfm*)message;

            if (cfm->resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                DEBUG_LOG("TwsTopology_HandleCmPrim CSR_BT_CM_SDS_UNREGISTER_CFM result:0x%04x supplier:0x%04x",
                          cfm->resultCode, cfm->resultSupplier);
            }
            TwsTopology_HandleSdpUnregisterCfm(task,cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS, cfm->serviceRecHandle);
        }
        break;

        default:
            DEBUG_LOG("TwsTopology_HandleCmPrim Unhandled CM Prim 0x%04x", *prim);
            break;
    }

    CmFreeUpstreamMessageContents((void *) message);
}
#endif

/*! \brief TWS Topology message handler.
 */
static void twsTopology_HandleMessage(Task task, MessageId id, Message message)
{
    tws_topology_sm_t *sm = TwsTopology_GetSm();

    UNUSED(task);

    switch (id)
    {
        case PEER_FIND_ROLE_ACTING_PRIMARY:
            twsTopology_SmHandleElectedStandalonePrimary(sm);
            break;
        case PEER_FIND_ROLE_PRIMARY:
            twsTopology_SmHandleElectedPrimaryWithPeer(sm);
            break;
        case PEER_FIND_ROLE_SECONDARY:
            twsTopology_SmHandleElectedSecondary(sm);
            break;

        case PAIRING_ACTIVITY:
            twsTopology_HandlePairingActivity(message);
            break;

        case MIRROR_PROFILE_CONNECT_IND:
            /* This message indicates the mirroring ACL is setup, this may have
            occurred after HDMA has issued a handover decision that was ignored
            by the SM's rules (due to mirroring not being setup yet), so need to
            kick the SM to reevaluate if a handover needs to be started */
            twsTopology_SmKick(sm);
            break;

        case CON_MANAGER_CONNECTION_IND:
            twsTopology_HandleConManagerConnectionInd((CON_MANAGER_CONNECTION_IND_T*)message);
            break;

        case PEER_SIG_CONNECTION_IND:
            twsTopology_HandlePeerJoinSignalling((const PEER_SIG_CONNECTION_IND_T *)message);
        break;

        case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
            TwsTopology_HandleMarshalledMsgChannelRxInd(sm, (PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T*)message);
            break;

        case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
            {
                PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* peer_tx_cfm = (PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T*)message;
                TwsTopology_HandleMarshalledMsgChannelTxCfm(sm, peer_tx_cfm);
                if(TwsTopology_IsLeaveStatusWaitingTxToPeer())
                {
                    TwsTopology_ClearLeaveStatusWaitingTxToPeer();
                    twsTopology_ExecuteLeaveActions(TwsTopology_GetSm());
                }
            }
            break;

        case TWSTOP_INTERNAL_TIMEOUT_TOPOLOGY_STOP:
            twsTopology_SmHandleInternalStopTimeout(sm);
            break;

        case TWSTOP_INTERNAL_KICK_SM:
            twsTopology_SmKick(sm);
            break;

        case TWSTOP_INTERNAL_RETRY_HANDOVER:
            twsTopology_SmHandleInternalRetryHandover(sm);
            break;

        case TWSTOP_INTERNAL_JOIN_REQUEST:
            twsTopology_SmHandleInternalJoinRequest(sm);
            break;

        case TWSTOP_INTERNAL_LEAVE_REQUEST:
            twsTopology_SmHandleInternalLeaveRequest(sm);
            break;

        case TWSTOP_INTERNAL_SWAP_ROLE_REQUEST:
            twsTopology_SmHandleInternalSwapRoleRequest(sm);
            break;

        case TWSTOP_INTERNAL_SWAP_ROLE_AND_DISCONNECT_REQUEST:
            twsTopology_SmHandleInternalSwapRoleAndDisconnectRequest(sm);
            break;

#ifdef USE_SYNERGY
        case CM_PRIM:
            TwsTopology_HandleCmPrim(TwsTopologyGetTask(), message);
            break;
#else
        case CL_SDP_REGISTER_CFM:
        {
            CL_SDP_REGISTER_CFM_T *cfm = (CL_SDP_REGISTER_CFM_T*)message;
            TwsTopology_HandleSdpRegisterCfm(TwsTopologyGetTask(), cfm->status == sds_status_success, cfm->service_handle);
            break;
        }
        case CL_SDP_UNREGISTER_CFM:
        {
            CL_SDP_UNREGISTER_CFM_T * cfm = (CL_SDP_UNREGISTER_CFM_T*)message;
            if(cfm->status != sds_status_pending)
            {
                    TwsTopology_HandleSdpUnregisterCfm(TwsTopologyGetTask(), cfm->status == sds_status_success, cfm->service_handle);
            }
            else
            {
            /* Wait for final confirmation message */
            }
            break;
        }
#endif

        case HDMA_HANDOVER_NOTIFICATION:
            twsTopology_SmHandleHDMARequest(sm, (hdma_handover_decision_t*)message);
            break;

        case HDMA_CANCEL_HANDOVER_NOTIFICATION:
            twsTopology_SmHandleHDMACancelHandover(sm);
            break;

#ifdef ENABLE_SKIP_PFR
        case TWSTOP_INTERNAL_STATIC_HANDOVER_TIMEOUT:
            twsTopology_SmHandlePostIdleStaticHandoverTimeout(sm);
            break;

        case TWSTOP_INTERNAL_SECONDARY_OUT_OF_CASE_TIMEOUT:
            twsTopology_SmKick(sm);
            break;
#endif
        default:
            break;
    }
}

static void twsTopology_RegisterBtParameters(void)
{
    BredrScanManager_PageScanParametersRegister(&page_scan_params);
    BredrScanManager_InquiryScanParametersRegister(&inquiry_scan_params);
    PanicFalse(LeAdvertisingManager_ParametersRegister(&le_adv_params));
}

static void twsTopology_SelectBtParameters(void)
{
    PanicFalse(LeAdvertisingManager_ParametersSelect(LE_ADVERTISING_PARAMS_SET_TYPE_FAST));
}

bool twsTopology_IsRunning(void)
{
    tws_topology_sm_t *sm = TwsTopology_GetSm();

    return (sm->state != TWS_TOPOLOGY_STATE_STOPPED);
}

void twsTopology_TransitionSecondaryToStandalonePrimary(tws_topology_sm_t* sm)
{
    PanicFalse(sm->elected_role == tws_topology_elected_role_secondary);
    twsTopology_ForceIdleState(sm);
    twsTopology_ExecuteJoinActions(sm);
}

void twsTopology_SetRoleSwapSupport(bool enable_role_swap)
{
    DEBUG_LOG_ALWAYS("twsTopology_SetRoleSwapSupport %d", enable_role_swap);
    TwsTopologyGetTaskData()->enable_role_swap = enable_role_swap;
}

const tws_topology_product_behaviour_t* twsTopology_GetProductBehaviour(void)
{
    return TwsTopologyGetTaskData()->product_behaviour;
}

void TwsTopology_RegisterMessageClient(Task client_task)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(TwsTopologyGetMessageClientTasks()), client_task);
}

void TwsTopology_UnRegisterMessageClient(Task client_task)
{
    TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(TwsTopologyGetMessageClientTasks()), client_task);
}

bool TwsTopology_Init(Task init_task)
{
    twsTopologyTaskData *tws_taskdata = TwsTopologyGetTaskData();

    UNUSED(init_task);

    tws_taskdata->enable_role_swap = TRUE;

    tws_taskdata->task.handler = twsTopology_HandleMessage;

    tws_taskdata->advertising_params = LE_ADVERTISING_PARAMS_SET_TYPE_UNSET;

    twsTopology_SmInit(&tws_taskdata->tws_sm);

    PeerFindRole_RegisterTask(TwsTopologyGetTask());

    /* Register for connect / disconnect events from mirror profile */
    MirrorProfile_ClientRegister(TwsTopologyGetTask());

    ConManagerRegisterConnectionsClient(TwsTopologyGetTask());

    Pairing_ActivityClientRegister(TwsTopologyGetTask());

    appPeerSigClientRegister(TwsTopologyGetTask());

    twsTopology_RegisterBtParameters();
    twsTopology_SelectBtParameters();

    TaskList_InitialiseWithCapacity(TwsTopologyGetMessageClientTasks(), MESSAGE_CLIENT_TASK_LIST_INIT_CAPACITY);

    unsigned rc_registrations_array_dim;
    rc_registrations_array_dim = (unsigned)role_change_client_registrations_end -
                              (unsigned)role_change_client_registrations_begin;
    PanicFalse((rc_registrations_array_dim % sizeof(role_change_client_callback_t)) == 0);
    rc_registrations_array_dim /= sizeof(role_change_client_callback_t);

    TwsTopology_RoleChangeClientNotifierInit(role_change_client_registrations_begin, 
                                    rc_registrations_array_dim); 

    TwsTopology_RegisterServiceRecord(TwsTopologyGetTask());

#ifdef ENABLE_LE_HANDOVER
    GattConnect_RegisterObserver(&tws_topology_gatt_callback);
#endif

    appPeerSigMarshalledMsgChannelTaskRegister(TwsTopologyGetTask(),
                                               PEER_SIG_MSG_CHANNEL_TOPOLOGY,
                                               topology_ind_marshal_type_descriptors,
                                               NUMBER_OF_MARSHAL_OBJECT_TYPES);
    return TRUE;
}

void TwsTopology_Start(void)
{
    DEBUG_LOG_FN_ENTRY("TwsTopology_Start");

    /* Product behaviour must have been configured before calling Start */
    PanicFalse(TwsTopologyGetTaskData()->product_behaviour->device_type > topology_device_type_invalid &&
               TwsTopologyGetTaskData()->product_behaviour->device_type < topology_device_type_max &&
               TwsTopologyGetTaskData()->product_behaviour->init &&
               TwsTopologyGetTaskData()->product_behaviour->deinit &&
               TwsTopologyGetTaskData()->product_behaviour->authoriseStartRoleSwap);
    twsTopology_SmStart(TwsTopology_GetSm());
}

void TwsTopology_Stop(void)
{
    DEBUG_LOG_FN_ENTRY("TwsTopology_Stop");
    twsTopology_SmStop(TwsTopology_GetSm());
}

void TwsTopology_ConfigureProductBehaviour(const tws_topology_product_behaviour_t* tws_topology_product_behaviour)
{
    DEBUG_LOG_FN_ENTRY("TwsTopology_ConfigureProductBehaviour");
    PanicFalse(tws_topology_product_behaviour->device_type > topology_device_type_invalid &&
               tws_topology_product_behaviour->device_type < topology_device_type_max &&
               tws_topology_product_behaviour->init &&
               tws_topology_product_behaviour->deinit &&
               tws_topology_product_behaviour->authoriseStartRoleSwap);
    topology_timeouts_t topology_timeouts = {0};
    PanicFalse(memcmp(&tws_topology_product_behaviour->timeouts,
                      &topology_timeouts,
                      sizeof(tws_topology_product_behaviour->timeouts)) != 0);
    if(TwsTopologyGetTaskData()->product_behaviour && TwsTopologyGetTaskData()->product_behaviour->deinit)
    {
        TwsTopologyGetTaskData()->product_behaviour->deinit();
    }
    TwsTopologyGetTaskData()->product_behaviour = tws_topology_product_behaviour;
    TwsTopologyGetTaskData()->product_behaviour->init();
}

bool TwsTopology_IsRolePrimary(void)
{
    return (tws_topology_role_primary == twsTopology_RoleFromState(TwsTopology_GetSm()->state));
}

bool TwsTopology_IsRolePrimaryConnectedToPeer(void)
{
    return TwsTopology_IsRolePrimary() && twsTopology_GetPrimaryPeerConnectStatusFromState(TwsTopology_GetSm()->state);
}

bool TwsTopology_IsRoleStandAlonePrimary(void)
{
    return TwsTopology_IsRolePrimary() && !TwsTopology_IsRolePrimaryConnectedToPeer();
}

bool TwsTopology_IsRoleSecondary(void)
{
    return (tws_topology_role_secondary == twsTopology_RoleFromState(TwsTopology_GetSm()->state));
}

void TwsTopology_Join(void)
{
    DEBUG_LOG_ERROR("TwsTopology_Join");
    MessageCancelFirst(TwsTopologyGetTask(), TWSTOP_INTERNAL_JOIN_REQUEST);
    MessageSend(TwsTopologyGetTask(), TWSTOP_INTERNAL_JOIN_REQUEST, NULL);
}

void TwsTopology_Leave(role_change_t role_change)
{
    DEBUG_LOG_ERROR("TwsTopology_Leave: enum:role_change_t:%d", role_change);
    TwsTopologyGetTaskData()->tws_sm.force_role_relinquish_on_leave = (role_change == role_change_force_reset);

    MessageCancelFirst(TwsTopologyGetTask(), TWSTOP_INTERNAL_LEAVE_REQUEST);
    MessageSend(TwsTopologyGetTask(), TWSTOP_INTERNAL_LEAVE_REQUEST, NULL);
}

void TwsTopology_SwapRole(void)
{
    DEBUG_LOG_ALWAYS("TwsTopology_SwapRole");
    MessageCancelFirst(TwsTopologyGetTask(), TWSTOP_INTERNAL_SWAP_ROLE_REQUEST);
    MessageSend(TwsTopologyGetTask(), TWSTOP_INTERNAL_SWAP_ROLE_REQUEST, NULL);
}

void TwsTopology_SwapRoleAndLeave(void)
{
    DEBUG_LOG_ALWAYS("TwsTopology_SwapRoleAndLeave");
    MessageCancelFirst(TwsTopologyGetTask(), TWSTOP_INTERNAL_SWAP_ROLE_AND_DISCONNECT_REQUEST);
    MessageSend(TwsTopologyGetTask(), TWSTOP_INTERNAL_SWAP_ROLE_AND_DISCONNECT_REQUEST, NULL);
}

void TwsTopology_EnableRoleSwapSupport(void)
{
    DEBUG_LOG_ALWAYS("TwsTopology_EnableRoleSwapSupport");
    twsTopology_SetRoleSwapSupport(TRUE);
    TwsTopology_PeerSignalTopologyCmd(enable_swap_role);
}

void TwsTopology_DisableRoleSwapSupport(void)
{
    DEBUG_LOG_ALWAYS("TwsTopology_DisableRoleSwapSupport");
    twsTopology_SetRoleSwapSupport(FALSE);
    TwsTopology_PeerSignalTopologyCmd(disable_swap_role);
}

bool TwsTopology_IsRoleSwapSupported(void)
{
    DEBUG_LOG_ALWAYS("TwsTopology_IsRoleSwapSupported");
    return TwsTopologyGetTaskData()->enable_role_swap;
}

#ifdef ENABLE_SKIP_PFR
bool TwsToplogy_IsRoleElectedByPeerFindRole(void)
{
    tws_topology_sm_t *sm = TwsTopology_GetSm();
    return sm->role_elected_by_pfr;
}

void TwsTopology_SendSecondaryOutOfCaseMessage(void)
{
    MessageSendLater(TwsTopologyGetTask(), TWSTOP_INTERNAL_SECONDARY_OUT_OF_CASE_TIMEOUT, NULL, D_SEC(twsTopology_GetProductBehaviour()->timeouts.post_idle_secondary_out_of_case_timeout_sec));
}

void TwsTopology_CancelSecondaryOutOfCaseMessage(void)
{
    MessageCancelAll(TwsTopologyGetTask(), TWSTOP_INTERNAL_SECONDARY_OUT_OF_CASE_TIMEOUT);
}
#endif
