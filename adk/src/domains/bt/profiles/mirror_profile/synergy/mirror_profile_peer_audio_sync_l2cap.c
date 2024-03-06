/*!
\copyright  Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       mirror_profile_peer_audio_sync_l2cap.c
\brief      Mirror profile L2cap Channel creation for Audio synchronisation.
*/

#ifdef INCLUDE_MIRRORING

#include <service.h>
#include <bt_device.h>
#include <l2cap_psm.h>
#include "mirror_profile_peer_audio_sync_l2cap.h"
#include "mirror_profile_private.h"
#include "mirror_profile_mdm_prim.h"
#include "sdp.h"

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/

/******************************************************************************
 * Macro Definitions
 ******************************************************************************/
#define MIRROR_SEC_REQ    ((SECL4_IN_SSP | SECL_IN_AUTHENTICATION | SECL_IN_ENCRYPTION) | (SECL4_OUT_SSP | SECL_OUT_AUTHENTICATION | SECL_OUT_ENCRYPTION))


static const CsrBtUuid128 service_uuid_mirror = {UUID_MIRROR_PROFILE_SERVICE};

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/
static void mirrorProfile_ConnectL2cap(const bdaddr *bd_addr);
static void mirrorProfile_SetAudioSyncL2capState(mirror_profile_audio_sync_l2cap_state_t state);
static void mirrorProfile_SendConnectConfirmation(mirror_profile_status_t status);

static uint16 mirrorProfile_ExtractPsm(CmnCsrBtLinkedListStruct *sdpTagList)
{
    uint16 psm = L2CA_PSM_INVALID;
    CsrBtUuid128 *tmpUuid;
    uint16 tmpResult;
    uint16 dummy1, dummy2; /* Currently unused */

    if (CsrBtUtilSdrGetServiceUuid128AndResult(sdpTagList,
                                               0,
                                               &tmpUuid,
                                               &tmpResult,
                                               &dummy1,
                                               &dummy2))
    {
        if (tmpResult == SDR_SDC_SEARCH_SUCCESS &&
            !memcmp(tmpUuid, &service_uuid_mirror, sizeof(service_uuid_mirror)))
        {
            psm = CsrBtUtilSdrGetL2capPsm(sdpTagList, 0);

        }
    }

    return psm;
}

static void mirrorProfile_SdpResultHandler(void *inst,
                                           CmnCsrBtLinkedListStruct *sdpTagList,
                                           CsrBtDeviceAddr deviceAddr,
                                           CsrBtResultCode resultCode,
                                           CsrBtSupplier resultSupplier)
{
    bool retry = FALSE;
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    DEBUG_LOG("mirrorProfile_SdpResultHandler, result:0x%04x supplier:0x%04x audio_sync.l2cap_state %u",
            resultCode,resultSupplier,mirror_inst->audio_sync.l2cap_state);

    switch (mirror_inst->audio_sync.l2cap_state)
    {
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_SDP_SEARCH:
        {
            if (resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                uint16 psm = L2CA_PSM_INVALID;

                if (sdpTagList)
                {
                    psm = mirrorProfile_ExtractPsm(sdpTagList);
                }

                if (psm != L2CA_PSM_INVALID)
                {
                    mirror_inst->audio_sync.remote_psm = psm;
                    DEBUG_LOG("MirrorProfile_HandleClSdpServiceSearchAttributeCfm, peer psm 0x%x",
                              mirror_inst->audio_sync.remote_psm);

                    mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_LOCAL_CONNECTING);
                }
                else
                {
                    /* No PSM found */
                    DEBUG_LOG("mirrorProfile_SdpResultHandler, malformed SDP record");

                    /*! Update the internal L2cap state to none( No L2cap channel exists) */
                    mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
                }
            }
            else
            {
                if (mirror_inst->audio_sync.sdp_search_attempts < MirrorProfile_GetSdpSearchTryLimit())
                {
                    DEBUG_LOG("mirrorProfile_SdpResultHandler, retry attempts %d",
                              mirror_inst->audio_sync.sdp_search_attempts);

                    retry = TRUE;
                    mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_SDP_SEARCH);
                }
                else
                {
                    DEBUG_LOG("mirrorProfile_SdpResultHandler, Max SDP search limit reached ");

                    /*! Update the internal L2cap state to none( No L2cap channel exists) */
                    mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
                }
            }
            break;
        }
        /* SDP search may have been cancelled during mirror profile disconnect and the search
        * confirmation arrived late when already reached the disconnected state, ingore the message. */
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE:
        {
            break;
        }

        default:
        {
            DEBUG_LOG("mirrorProfile_SdpResultHandler, unexpected state 0x%x, l2cap_state %u, status %u",
                      MirrorProfile_GetState(),
                      mirror_inst->audio_sync.l2cap_state,
                      resultCode);
            Panic();
        }
    }

    CsrBtUtilBllFreeLinkedList(&sdpTagList, CsrBtUtilBllPfreeWrapper);
    if (!retry)
    {
        MessageSend(MirrorProfile_GetTask(), MIRROR_INTERNAL_CLOSE_SDP, NULL);
    }

    UNUSED(inst);
    UNUSED(deviceAddr);
    UNUSED(resultSupplier);
}

static void mirrorProfile_ResultHandler(CsrSdcOptCallbackType cbType, void *context)
{
    CsrSdcResultFuncType *params = (CsrSdcResultFuncType *)context;

    if(cbType == CSR_SDC_OPT_CB_SEARCH_RESULT)
    {
        mirrorProfile_SdpResultHandler(params->instData, params->sdpTagList, params->deviceAddr, params->resultCode, params->resultSupplier);
    }
}

/*! \brief Performs operation required while entering the MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_SDP_SEARCH state. */
static void mirrorProfile_EnterSdpSearch(void)
{
    DEBUG_LOG("mirrorProfile_EnterSdpSearch");
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();
    CmnCsrBtLinkedListStruct *sdpTagList = NULL;
    uint16 shIndex;
    CsrBtDeviceAddr addr;


    if (!mirror_inst->sdp_search_data)
    {
        mirror_inst->sdp_search_data = CsrBtUtilSdcInit(mirrorProfile_ResultHandler,
                                                      TrapToOxygenTask(&mirror_inst->task_data));
    }

    sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid128(sdpTagList,
                                                                 &service_uuid_mirror,
                                                                 &shIndex);
    CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList,
                                         shIndex,
                                         CSR_BT_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER,
                                         NULL,
                                         0);

    BdaddrConvertVmToBluestack(&addr, &mirror_inst->audio_sync.peer_addr);

    /* Start the SDP search */
    CsrBtUtilSdcSearchStart((void *) mirror_inst,
                            mirror_inst->sdp_search_data,
                            sdpTagList,
                            addr);

    mirror_inst->audio_sync.sdp_search_attempts++;
}

/*! \brief Send confirmation of a connection to all registered clients.

    \param[in] status   Refer \ref mirror_profile_status_t
*/
static void mirrorProfile_SendConnectConfirmation(mirror_profile_status_t status)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    /* Send MIRROR_PROFILE_CONNECT_CFM to client which made a connect request. */
    if(mirror_inst->audio_sync.connect_task != NULL)
    {
        MESSAGE_MAKE(message, MIRROR_PROFILE_CONNECT_CFM_T);
        message->status = status;
        MessageSend(mirror_inst->audio_sync.connect_task, MIRROR_PROFILE_CONNECT_CFM, message);
        mirror_inst->audio_sync.connect_task = NULL;
    }
}

/*! \brief Send confirmation of a disconnection to all registered clients.

    \param[in] status   Refer \ref mirror_profile_status_t
*/
static void mirrorProfile_SendDisconnectConfirmation(mirror_profile_status_t status)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    /* Send MIRROR_PROFILE_DISCONNECT_CFM to client which made a disconnect request. */
    if(mirror_inst->audio_sync.disconnect_task != NULL)
    {
        MESSAGE_MAKE(message, MIRROR_PROFILE_DISCONNECT_CFM_T);
        message->status = status;
        MessageSend(mirror_inst->audio_sync.disconnect_task, MIRROR_PROFILE_DISCONNECT_CFM, message);
        mirror_inst->audio_sync.disconnect_task = NULL;
    }
}

/*! \brief Initiate L2CAP connection request for Mirror profile to the peer device 

    Extract the remote PSM value from a service record returned by a SDP service search.

    \param[in] bd_addr      BD Address of the peer device

*/
static void mirrorProfile_ConnectL2cap(const bdaddr *bd_addr)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();
    CsrBtDeviceAddr addr;

    static const uint16 l2cap_conftab[] =
    {
        /* Configuration Table must start with a separator. */
        L2CAP_AUTOPT_SEPARATOR,
        /* Flow & Error Control Mode. */
        L2CAP_AUTOPT_FLOW_MODE,
        /* Set to Basic mode with no fallback mode */
        BKV_16_FLOW_MODE( FLOW_MODE_BASIC, 0 ),
        /* Local MTU exact value (incoming). */
        L2CAP_AUTOPT_MTU_IN,
        /*  Exact MTU for this L2CAP connection - 672. */
        MIRROR_PROFILE_L2CAP_MTU_SIZE,
        /* Remote MTU Minumum value (outgoing). */
        L2CAP_AUTOPT_MTU_OUT,
        /*  Minimum MTU accepted from the Remote device. */
        48,
        L2CAP_AUTOPT_FLUSH_IN,
        BKV_UINT32R(MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT,MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT),
        /* Local Flush Timeout  */
        L2CAP_AUTOPT_FLUSH_OUT,
        BKV_UINT32R(MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT,MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT),
        /* Configuration Table must end with a terminator. */
        L2CAP_AUTOPT_TERMINATOR
    };

    DEBUG_LOG("mirrorProfile_ConnectL2cap");
    BdaddrConvertVmToBluestack(&addr, bd_addr);

    CmL2caConnectReqConftabSend(&mirror_inst->task_data,
                                addr,
                                mirror_inst->audio_sync.local_psm,
                                mirror_inst->audio_sync.remote_psm,
                                MIRROR_SEC_REQ,
                                0,
                                (sizeof(l2cap_conftab) / sizeof(uint16)),
                                CsrMemDup(l2cap_conftab,
                                          sizeof(l2cap_conftab)),
                                CSR_BT_SC_DEFAULT_ENC_KEY_SIZE);
    mirror_inst->connect_req_sent = TRUE;
}

/*! \brief Function to set the L2CAP internal state information.

    \param[in] state      Internal state value to be updated.
*/
static void mirrorProfile_SetAudioSyncL2capState(mirror_profile_audio_sync_l2cap_state_t state)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    DEBUG_LOG("mirrorProfile_SetAudioSyncL2capState: exit enum:mirror_profile_audio_sync_l2cap_state_t:%d"
              " enter enum:mirror_profile_audio_sync_l2cap_state_t:%d",
                mirror_inst->audio_sync.l2cap_state, state);

    /*! update the internal state */
    mirror_inst->audio_sync.l2cap_state = state;

    switch (state)
    {
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE:
        {
            MirrorProfile_SetTargetStateFromProfileState();

            /*! Send the connect confirmation back to topology layer */
            mirrorProfile_SendConnectConfirmation(mirror_profile_status_peer_connect_failed);
            mirrorProfile_SendDisconnectConfirmation(mirror_profile_status_peer_disconnected);

            /*! Reset the sdp search attempts */
            mirror_inst->audio_sync.sdp_search_attempts = 0;

            MessageCancelFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_QHS_START_TIMEOUT);

            MirrorProfile_ClearQhsReady();
        }
        break;

        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_SDP_SEARCH:
        {
            /*! Start the SDP Search */
            mirrorProfile_EnterSdpSearch();
        }
        break;

        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_LOCAL_CONNECTING:
        {
            mirror_inst->audio_sync.sdp_search_attempts = 0;
            /*! Establish the L2cap connection for Audio Synchronisation */
            mirrorProfile_ConnectL2cap(&mirror_inst->audio_sync.peer_addr);
        }
        break;

        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_DISCONNECTING:
        {
            if(SinkIsValid(mirror_inst->audio_sync.link_sink))
            {
                /* Initiate the L2cap disconnect request */
                CsrBtConnId btConnId = CM_CREATE_L2CA_CONN_ID(SinkGetL2capCid(mirror_inst->audio_sync.link_sink));

                CmL2caDisconnectReqSend(btConnId, CSR_BT_CM_CONTEXT_UNUSED);
            }
            else if(mirror_inst->connect_req_sent)
            {
                DEBUG_LOG("mirrorProfile_SetAudioSyncL2capState cancelling pending connect request");
                CsrBtDeviceAddr addr;
                BdaddrConvertVmToBluestack(&addr, &mirror_inst->audio_sync.peer_addr);
                CmL2caCancelConnectReqSend(&mirror_inst->task_data, addr, mirror_inst->audio_sync.local_psm);
            }
        }
        break;

        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_REMOTE_CONNECTING:
        break;

        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_CONNECTED:
        {
            if (!MirrorProfile_IsQhsReady())
            {
                MessageSendLater(MirrorProfile_GetTask(), MIRROR_INTERNAL_QHS_START_TIMEOUT, NULL,
                                 mirrorProfileConfig_QhsStartTimeout());
            }
            else
            {
                /*! Set the Mirror profile state */
                MirrorProfile_SetTargetStateFromProfileState();
            }

            /* Send connected status */
            mirrorProfile_SendConnectConfirmation(mirror_profile_status_peer_connected);
        }
        break;

        default:
        break;
    }
}

/*! \brief Handles shut-down request for Mirror-profile.

    Handles L2cap shutdown request by intiating disconnection to peer device based on
    the current machine state.
*/
static void mirrorProfile_HandleShutdown(void)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    switch (mirror_inst->audio_sync.l2cap_state)
    {
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_SDP_SEARCH:
        {
            /*! Cancel the ongoing SDP search */
            CsrBtUtilRfcConCancel(mirror_inst, mirror_inst->sdp_search_data);
            CsrBtUtilSdcRfcDeinit(&mirror_inst->sdp_search_data);

            /*! Update the internal L2cap state to none( No L2cap channel exists) */
            mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
        }
        break;
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_LOCAL_CONNECTING:
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_CONNECTED:
        {
            /*! Move to L2cap disconnecting state */
            mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_DISCONNECTING);
        }
        break;
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE:
        {
            /*! No L2cap channel exists! Just send disconnected status back */
            mirrorProfile_SendDisconnectConfirmation(mirror_profile_status_peer_disconnected);
        }
        break;

        default:
        break;
    }
}

/*! \brief Close the L2cap channel created for audio synchronisation.

    Close the L2CAP connection for Audio synchronisation for Mirror Profile.
*/
void MirrorProfile_CloseAudioSyncL2capChannel(Task task)
{
    /*! Store the disconnect task */
    MirrorProfile_GetAudioSyncL2capState()->disconnect_task = task;
    mirrorProfile_HandleShutdown();
}

/*! \brief Handle the incoming connection request from remote device.
*/
void MirrorProfile_HandleCmL2caConnectAcceptInd(const CsrBtCmL2caConnectAcceptInd *ind)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();
    bool accept = FALSE;
    const uint16 conftab[] =
    {
        /* Configuration Table must start with a separator. */
        L2CAP_AUTOPT_SEPARATOR,
        L2CAP_AUTOPT_FLUSH_IN,
        BKV_UINT32R(MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT,MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT),
        /* Local Flush Timeout  */
        L2CAP_AUTOPT_FLUSH_OUT,
        BKV_UINT32R(MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT,MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT),
        L2CAP_AUTOPT_TERMINATOR
    };

    DEBUG_LOG("MirrorProfile_HandleCmL2caConnectAcceptInd, state %u, psm %u, local_psm %u",
              MirrorProfile_GetState(),
              ind->localPsm,
              mirror_inst->audio_sync.local_psm);

    /* If the PSM doesn't match, panic! */
    PanicFalse(ind->localPsm == mirror_inst->audio_sync.local_psm);

    switch (mirror_inst->audio_sync.l2cap_state)
    {
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE:
        {
            bdaddr bd_addr;
            BdaddrConvertBluestackToVm(&bd_addr, &ind->deviceAddr);

            /* only accept L2cap connections from paired peer device. */
            if (appDeviceIsPeer(&bd_addr))
            {
                DEBUG_LOG("MirrorProfile_HandleCmL2caConnectAcceptInd, accepted");

                /* Move to 'Remote connecting' state */
                mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_REMOTE_CONNECTING);

                MirrorProfile_GetAudioSyncL2capState()->peer_addr = bd_addr;

                /* Accept connection */
                accept = TRUE;
            }
            else
            {
                DEBUG_LOG("MirrorProfile_HandleCmL2caConnectAcceptInd, rejected, unknown peer");
            }
        }
        break;

        default:
        {
            DEBUG_LOG("MirrorProfile_HandleCmL2caConnectAcceptInd, rejected, state %u", MirrorProfile_GetState());
        }
        break;
    }

    /* Send a response accepting or rejecting the connection. */
    CmL2caConnectAcceptRspSend(&mirror_inst->task_data,
                               accept,
                               ind->btConnId,
                               ind->localPsm,
                               ind->deviceAddr,
                               ind->identifier,
                               (sizeof(conftab) / sizeof(uint16)),
                               CsrMemDup(conftab, sizeof(conftab)),
                               CSR_BT_SC_DEFAULT_ENC_KEY_SIZE);
}

static void mirrorProfile_Connected(CsrBtConnId btConnId, CsrBtResultCode result)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();
    Sink sink = StreamL2capSink(CM_GET_UINT16ID_FROM_BTCONN_ID(btConnId));

    DEBUG_LOG("mirrorProfile_Connected, status %u", result);

    switch (mirror_inst->audio_sync.l2cap_state)
    {
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_LOCAL_CONNECTING:
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_REMOTE_CONNECTING:
        {
            /* If connection was succesful, get sink, attempt to enable wallclock and move
             * to connected state */
            if (result == CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                DEBUG_LOG("mirrorProfile_Connected, connected, conn ID %u",
                           btConnId);

                PanicNull(sink);
                mirror_inst->audio_sync.link_sink = sink;
                mirror_inst->audio_sync.link_source = StreamSourceFromSink(sink);

                MessageStreamTaskFromSink(mirror_inst->audio_sync.link_sink,
                                          &mirror_inst->task_data);
                MessageStreamTaskFromSource(mirror_inst->audio_sync.link_source,
                                            &mirror_inst->task_data);

                PanicFalse(SinkConfigure(mirror_inst->audio_sync.link_sink,
                                         VM_SINK_MESSAGES,
                                         VM_MESSAGES_ALL));
                PanicFalse(SourceConfigure(mirror_inst->audio_sync.link_source,
                                           VM_SOURCE_MESSAGES,
                                           VM_MESSAGES_ALL));

                StreamConnectDispose(mirror_inst->audio_sync.link_source);

                mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_CONNECTED);
            }
            else
            {
                DEBUG_LOG("mirrorProfile_Connected, failed, go to disconnected state");

                /*! Move the L2cap state to none */
                mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
            }
            break;
        }

        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_DISCONNECTING:
        {
            /* The L2cap channel is getting closed when we received the l2cap create confirmation message */
            DEBUG_LOG("mirrorProfile_Connected, cancelled");

            if (result == CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                mirror_inst->audio_sync.link_sink = sink;

                /* Re-enter the DISCONNECTING state - this time the L2CAP
                   disconnect request will be sent because link_sink is valid. */
                mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_DISCONNECTING);
            }
            else
            {
                /*! Move the L2cap state to none */
                mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
            }
        }
            break;

        default:
        {
            DEBUG_LOG("mirrorProfile_Connected, failed");
            PanicFalse(result != CSR_BT_RESULT_CODE_CM_SUCCESS);
            break;
        }
    }
}

void MirrorProfile_HandleCmL2caConnectAcceptCfm(const CsrBtCmL2caConnectAcceptCfm *cfm)
{
    DEBUG_LOG("MirrorProfile_HandleCmL2caConnectAcceptCfm, status 0x%04x supplier 0x%04x psm %u bdaddr %04x,%02x,%06lx", 
              cfm->resultCode,
              cfm->resultSupplier,
              cfm->localPsm,
              cfm->deviceAddr.nap,
              cfm->deviceAddr.uap,
              cfm->deviceAddr.lap);

    mirrorProfile_Connected(cfm->btConnId, cfm->resultCode);
}

void MirrorProfile_HandleCmL2caConnectCfm(const CsrBtCmL2caConnectCfm *cfm)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    DEBUG_LOG("MirrorProfile_HandleCmL2caConnectCfm, status 0x%04x supplier 0x%04x psm %u bdaddr %04x,%02x,%06lx", 
              cfm->resultCode,
              cfm->resultSupplier,
              cfm->localPsm,
              cfm->deviceAddr.nap,
              cfm->deviceAddr.uap,
              cfm->deviceAddr.lap);

    mirror_inst->connect_req_sent = FALSE;
    mirrorProfile_Connected(cfm->btConnId, cfm->resultCode);

    if (cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        /* If search data still exists, it needs to be freed, as the connection is completed. */
        if (mirror_inst->sdp_search_data)
        {
            CsrBtUtilRfcConCancel(mirror_inst, mirror_inst->sdp_search_data);
            CsrBtUtilSdcRfcDeinit(&mirror_inst->sdp_search_data);
        }
    }
}

/*! \brief Create the L2cap channel for the Audio Synchronistion.
 */
void MirrorProfile_CreateAudioSyncL2capChannel(Task task, const bdaddr *peer_addr)
{
    mirror_profile_audio_sync_context_t *audio_sync = MirrorProfile_GetAudioSyncL2capState();

    audio_sync->connect_task = task;
    audio_sync->peer_addr = *peer_addr;

    DEBUG_LOG("MirrorProfile_CreateAudioSyncL2capChannel, state %u, bdaddr %04x,%02x,%06lx",
               MirrorProfile_GetState(),
               peer_addr->nap,
               peer_addr->uap,
               peer_addr->lap);

    /* Check if ACL is now up */
    if (ConManagerIsConnected(&audio_sync->peer_addr))
    {
        DEBUG_LOG("MirrorProfile_CreateAudioSyncL2capChannel, ACL connected");
        
        if(L2CAP_PSM_MIRROR_PROFILE == L2CA_PSM_INVALID)
        {
            /* Trigger the SDP Search */
            mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_SDP_SEARCH);
        }
        else
        {
            /* Connect L2CAP straight away */
            MirrorProfile_Get()->audio_sync.remote_psm = L2CAP_PSM_MIRROR_PROFILE;
            mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_LOCAL_CONNECTING);
        }
    }
    else
    {
        /* Send connect failed status since there is no ACL link between the peer devices */
        mirrorProfile_SendConnectConfirmation(mirror_profile_status_peer_connect_failed);
    }
}


/*! \brief Handle a L2CAP disconnect initiated by the remote peer.

    \param[in] ind      Refer \ref CL_L2CAP_DISCONNECT_IND_T, pointer to L2CAP disconnect 
                        indication.

*/
void MirrorProfile_HandleCmL2caDisconnectInd(const CsrBtCmL2caDisconnectInd *ind)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    DEBUG_LOG("MirrorProfile_HandleCmL2caDisconnectInd, reason 0x%04x supplier 0x%04x", ind->reasonCode, ind->reasonSupplier);

    if(!ind->localTerminated)
    {
        Sink sink = StreamL2capSink(CM_GET_UINT16ID_FROM_BTCONN_ID(ind->btConnId));

        /* Response is required only for remote disconnection. */
        ConnectionL2capDisconnectResponse(ind->l2caSignalId, sink);

        /* Only change state if sink matches */
        if (sink == mirror_inst->audio_sync.link_sink)
        {
            /*! Move to L2cap none state */
            mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
        }
    }
    else
    {
        switch (mirror_inst->audio_sync.l2cap_state)
        {
            case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_DISCONNECTING:
            {
                /*! L2cap channel got closed. Move to L2cap none state */
                mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
            }
            default:
            break;
        }
    }
}

void MirrorProfile_HandleCmL2caRegisterCfm(const CsrBtCmL2caRegisterCfm *cfm)
{
    mirror_profile_task_data_t *mirror_task_data = MirrorProfile_Get();

    DEBUG_LOG("MirrorProfile_HandleCmL2caRegisterCfm, status 0x%04x supplier 0x%04x, psm %u",
              cfm->resultCode, cfm->resultSupplier, cfm->localPsm);


    /* We have registered the PSM used for mirror profile link with
       connection manager,        uint8 *record = PanicUnlessMalloc(appSdpGetMirrorProfileServiceRecordSize()); */
    if (CSR_BT_RESULT_CODE_CM_SUCCESS == cfm->resultCode)
    {
        /* Keep a copy of the registered L2CAP PSM, maybe useful later */
        mirror_task_data->audio_sync.local_psm = cfm->localPsm;

        /* Copy and update SDP record */
        uint8 *record = CsrMemDup(appSdpGetMirrorProfileServiceRecord(), appSdpGetMirrorProfileServiceRecordSize());

        /* Keep a copy of the registered L2CAP PSM, maybe useful later */
        mirror_task_data->audio_sync.local_psm = cfm->localPsm;

        /* Write L2CAP PSM into service record */
        appSdpSetMirrorProfilePsm(record, cfm->localPsm);

        /* Register service record */
        ConnectionRegisterServiceRecord(MirrorProfile_GetTask(),
                                        appSdpGetMirrorProfileServiceRecordSize(),
                                        record);
    }
    else
    {
        DEBUG_LOG("MirrorProfile_HandleCmL2caRegisterCfm, failed to register L2CAP PSM");
        Panic();
    }
}

void MirrorProfile_HandleCmSdsRegisterCfm(const CsrBtCmSdsRegisterCfm *cfm)
{
    DEBUG_LOG("MirrorProfile_HandleCmSdsRegisterCfm, status 0x%04x supplier 0x%04x", cfm->resultCode, cfm->resultSupplier);

    if (cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        /* Register with the firmware to receive MESSAGE_BLUESTACK_MDM_PRIM messages */
        MessageMdmTask(MirrorProfile_GetTask());

        /* Register with the SDM service */
        MirrorProfile_MirrorRegisterReq();
    }
    else
    {
        DEBUG_LOG("MirrorProfile_HandleCmSdsRegisterCfm, SDP registration failed");
        Panic();
    }
}

void MirrorProfile_HandleCmModeChangeInd(const CsrBtCmModeChangeInd *ind)
{
    bdaddr bd_addr;

    BdaddrConvertBluestackToVm(&bd_addr, &ind->deviceAddr);

    if (appDeviceIsPeer(&bd_addr))
    {
        DEBUG_LOG("MirrorProfile_HandleCmModeChangeInd state 0x%x, mode 0x%x",
                  MirrorProfile_GetState(),
                  ind->mode);

        if (MirrorProfile_GetState() == MIRROR_PROFILE_STATE_ACL_CONNECTING)
        {
            if (ind->mode == CSR_BT_SNIFF_MODE && MirrorProfile_IsPrimary())
            {
                /* Send MDM prim to create mirror ACL connection */
                MirrorProfile_MirrorConnectReq(LINK_TYPE_ACL);
            }
        }
    }
}

void MirrorProfile_TerminateSdpPrimitive(void)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    DEBUG_LOG("MirrorProfile_TerminateSdpPrimitive, %d", mirror_inst->audio_sync.l2cap_state);

    switch (mirror_inst->audio_sync.l2cap_state)
    {
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_SDP_SEARCH:
            {
                /* Cancel the ongoing SDP search */
                CsrBtUtilRfcConCancel(mirror_inst, mirror_inst->sdp_search_data);
                CsrBtUtilSdcRfcDeinit(&mirror_inst->sdp_search_data);

                /* Update the internal L2cap state to none( No L2cap channel exists) */
                mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
            }
            break;

        default:
            break;
    }

}

#endif /* INCLUDE_MIRRORING */
