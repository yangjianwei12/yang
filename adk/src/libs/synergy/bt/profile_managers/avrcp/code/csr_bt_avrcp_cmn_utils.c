/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_AVRCP_MODULE

#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_avrcp_main.h"
#include "csr_bt_avrcp_prim.h"
#include "csr_bt_avrcp_lib.h"
#ifdef CSR_BT_INSTALL_AVRCP_COVER_ART
#include "csr_bt_avrcp_imaging_private_prim.h"
#include "csr_bt_avrcp_imaging_private_lib.h"
#endif

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_avrcp_streams.h"
#endif

#ifdef CSR_TARGET_PRODUCT_VM
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif
#endif


/***** Static functions *****/

static CsrUint8 csrBtAvrcpUtilGetAppConnId(AvrcpInstanceData_t *instData)
{
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_FIRST((CsrCmnList_t *)&instData->connList);
    CsrUint8 currConnId = 0;

    while (connInst)
    {
        if (connInst->appConnId == currConnId)
        {/* Connection ID already in use - skip to next number and restart */
            currConnId++;
            connInst = AVRCP_LIST_CONN_GET_FIRST((CsrCmnList_t *)&instData->connList);
        }
        else
        {
            connInst = connInst->next;
        }
    }

    /* Lowest possible connection ID found */
    return currConnId;
}

static void avrcpSendHouseCleaning(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpHouseCleaning *prim = (CsrBtAvrcpHouseCleaning *) CsrPmemZalloc(sizeof(*prim));

    prim->type = CSR_BT_AVRCP_HOUSE_CLEANING;
    instData->restoreFlag = TRUE;
    CsrBtAvrcpMessagePut(CSR_BT_AVRCP_IFACEQUEUE, prim);
}

/***** Public functions *****/
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
L2CA_FLOW_T *CsrBtAvrcpUtilGetFlow(l2ca_mtu_t mtu)
{
    L2CA_FLOW_T *flow = (L2CA_FLOW_T *) CsrPmemZalloc(sizeof(*flow));

    flow->mode              = L2CA_FLOW_MODE_ENHANCED_RETRANS;
    flow->tx_window         = 1;
    flow->max_retransmit    = 0; /* Maximum as specified by spec: infinite */
    flow->retrans_timeout   = CSR_BT_AVRCP_QOS_RETRANSMIT_TIMEOUT;
    flow->monitor_timeout   = CSR_BT_AVRCP_QOS_MONITOR_TIMEOUT;
    flow->maximum_pdu       = mtu;
    return flow;
}
#endif

void CsrBtAvrcpUtilConnect(AvrcpConnInstance_t *connInst)
{
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    if ((connInst->control.state == AVRCP_STATE_CONN_CONNECTED) &&
        ((connInst->browsing.state == AVRCP_STATE_CONN_DISCONNECTED) ||
         (connInst->browsing.state == AVRCP_STATE_CONN_DISC2RECONNECT)))
    {/* Connect browsing */
         dm_security_level_t secOutgoingBrow = 0;
        
#ifndef INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS
        CsrBtScSetSecOutLevel(&secOutgoingBrow, CSR_BT_SEC_DEFAULT,
                                 CSR_BT_AV_RCP_MANDATORY_SECURITY_OUTGOING,
                                 CSR_BT_AV_RCP_DEFAULT_SECURITY_OUTGOING,
                                 CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
                                 CSR_BT_RESULT_CODE_AVRCP_UNACCEPTABLE_PARAMETER);
#else
        secOutgoingBrow = connInst->instData->secOutgoingBrow;
#endif /* INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS */
        
        CsrBtCml2caFecConnectReqSend(CSR_BT_AVRCP_IFACEQUEUE,
                             connInst->address,
                             CSR_BT_AVCTP_BROWSING_PSM,
                             CSR_BT_AVCTP_BROWSING_PSM,
                             connInst->instData->mtu,
                             L2CA_FLUSH_TO_INFINITE,                    /* Flush timeout */
                             NULL,                                      /* QoS */
                             CsrBtAvrcpUtilGetFlow(connInst->instData->mtu), /* Flow */
                             FALSE,                                     /* Fallback basic-mode */
                             secOutgoingBrow,
                             CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                    CSR_BT_AVRCP_DEFAULT_ENC_KEY_SIZE_VAL));

        AVRCP_CHANGE_STATE_INDEX(connInst->browsing.state, AVRCP_STATE_CONN_CONNECTING, connInst->appConnId);
    }
    else if ((connInst->control.state == AVRCP_STATE_CONN_DISCONNECTED) ||
             (connInst->control.state == AVRCP_STATE_CONN_PENDING) ||
             (connInst->control.state == AVRCP_STATE_CONN_DISC2RECONNECT))
#else
    if ((connInst->control.state == AVRCP_STATE_CONN_DISCONNECTED) ||
             (connInst->control.state == AVRCP_STATE_CONN_PENDING) ||
             (connInst->control.state == AVRCP_STATE_CONN_DISC2RECONNECT))
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
    {/* When making outgoing connection, always try basic mode for the control channel. Done to
        avoid IOP problems with existing implementations of AVRCP 1.4 and implementations of older
        versions of AVRCP. */
        dm_security_level_t secOutgoingCont = 0;

#ifndef INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS
        CsrBtScSetSecOutLevel(&secOutgoingCont, CSR_BT_SEC_DEFAULT,
                         CSR_BT_AV_RCP_MANDATORY_SECURITY_OUTGOING,
                         CSR_BT_AV_RCP_DEFAULT_SECURITY_OUTGOING,
                         CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
                         CSR_BT_RESULT_CODE_AVRCP_UNACCEPTABLE_PARAMETER);
#else
        secOutgoingCont = connInst->instData->secOutgoingCont;
#endif /* INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS */

        CsrBtCml2caConnectReqSend(CSR_BT_AVRCP_IFACEQUEUE,
                                  connInst->address,
                                  CSR_BT_AVCTP_PSM,
                                  CSR_BT_AVCTP_PSM,
                                  connInst->instData->mtu,
                                  secOutgoingCont,
                                  CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                         CSR_BT_AVRCP_DEFAULT_ENC_KEY_SIZE_VAL));

        AVRCP_CHANGE_STATE_INDEX(connInst->control.state, AVRCP_STATE_CONN_CONNECTING, connInst->appConnId);
    }
}

void CsrBtAvrcpUtilDisconnect(AvrcpConnInstance_t *connInst)
{
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    CsrBtAvrcpConnDetails *connDetails = &connInst->browsing; /* Browsing must be disconnected first */
#else
    CsrBtAvrcpConnDetails *connDetails = &connInst->control;
#endif

#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
    if ((connInst->tgLocal->obexState != AVRCP_TG_OBEX_SERVER_IDLE) &&
        (connInst->tgLocal->obexState != AVRCP_TG_OBEX_SERVER_DEACTIVATING))
    {
        /* Remove OBEX server instance and transport connection */
        CsrBtAvrcpImagingServerDeactivateReqSend(connInst->instData->tgLocal.obexPsm, connInst->appConnId);
        connInst->tgLocal->obexState = AVRCP_TG_OBEX_SERVER_DEACTIVATING;
    }
#endif

    while (connDetails)
    {
        switch (connDetails->state)
        {
            case AVRCP_STATE_CONN_PENDING:
            {
                AVRCP_CHANGE_STATE_INDEX(connDetails->state, AVRCP_STATE_CONN_DISCONNECTED, connInst->appConnId);
                break;
            }

            case AVRCP_STATE_CONN_CONNECTING:
            { /* Attempt to cancel outgoing connection */
                psm_t psm;

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                psm = (psm_t) (connDetails == &connInst->control ? CSR_BT_AVCTP_PSM : CSR_BT_AVCTP_BROWSING_PSM);
#else
                psm = CSR_BT_AVCTP_PSM;
#endif

                CsrBtCml2caCancelConnectReqSend(CSR_BT_AVRCP_IFACEQUEUE,
                                                connInst->address,
                                                psm);
                AVRCP_CHANGE_STATE_INDEX(connDetails->state, AVRCP_STATE_CONN_CANCELLING, connInst->appConnId);
                break;
            }

            case AVRCP_STATE_CONN_CONNECTED:
            {/* Connection was already established and should be disconnected */
                CsrBtCml2caDisconnectReqSend(connDetails->btConnId);
                connInst->resetAppState = TRUE;
                AVRCP_CHANGE_STATE_INDEX(connDetails->state, AVRCP_STATE_CONN_DISCONNECTING, connInst->appConnId);
                break;
            }

            case AVRCP_STATE_CONN_DISCONNECTING:
            {/* Should not occur - ignore */
                break;
            }

            case AVRCP_STATE_CONN_CANCELLING:
            {/* Should not occur - ignore */
                break;
            }

            default:
            {
                break;
            }
        }

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
        if (connDetails == &connInst->browsing)
        {
            connDetails = &connInst->control;
        }
        else
#endif
        {
            break;
        }
    }

    if (connInst->sdpState == AVRCP_STATE_SDP_ACTIVE)
    {/* Active SDP search should be cancelled */
        CsrBtAvrcpSdpSearchCancel(connInst);
    }
}

void CsrBtAvrcpUtilConnectAccept(AvrcpInstanceData_t *instData)
{

#ifdef CSR_BT_RESTRICT_MAX_PROFILE_CONNECTIONS
    /* Max allowed connections is incomingMaximum */
    if (instData->numActiveAvrcpConns < instData->incomingMaximum)
#else    
    /* Determine if more incoming connections should be accepted */
    if (instData->incomingMaximum > instData->incomingCurrent)
#endif
    {
        CsrBool tgSupport = FALSE;

#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
        if (instData->tgLocal.srHandle)
        {
            tgSupport = TRUE;
        }
#endif
        if (instData->activateStateCont == AVRCP_STATE_ACT_DEACTIVATED)
        {/* Incoming connections on control channel should be accepted
            To avoid IOP problems, accept only basic mode for the control channel */
            dm_security_level_t secIncomingCont = 0;

#ifndef INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS
            CsrBtScSetSecInLevel(&secIncomingCont, CSR_BT_SEC_DEFAULT,
                                 CSR_BT_AV_RCP_MANDATORY_SECURITY_INCOMING,
                                 CSR_BT_AV_RCP_DEFAULT_SECURITY_INCOMING,
                                 CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
                                 CSR_BT_RESULT_CODE_AVRCP_UNACCEPTABLE_PARAMETER);
#else
            secIncomingCont = instData->secIncomingCont;
#endif /* INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS */

            CsrBtCml2caConnectAcceptReqSend(CSR_BT_AVRCP_IFACEQUEUE,
                                            CSR_BT_AVCTP_PSM,
                                            (CsrUint24)(tgSupport ? CSR_BT_CAPTURING_MAJOR_SERVICE_MASK : 0), /* CoD */
                                            secIncomingCont,
                                            instData->mtu,
                                            L2CA_FLUSH_TO_DEFAULT,
                                            NULL,
                                            (uuid16_t)(tgSupport ? CSR_BT_AV_REMOTE_CONTROL_TARGET_UUID : CSR_BT_AV_REMOTE_CONTROL_UUID), /* UUID for use in authorise indications */
                                            CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                                   CSR_BT_AVRCP_DEFAULT_ENC_KEY_SIZE_VAL));

            AVRCP_CHANGE_STATE(instData->activateStateCont, AVRCP_STATE_ACT_ACTIVATED);
        }

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
        /* Only accept connections on browsing channel if AVRCP version 1.4 or higher is supported */
        if ((instData->srAvrcpVersionHighest >= CSR_BT_AVRCP_CONFIG_SR_VERSION_14) &&
            (instData->activateStateBrow == AVRCP_STATE_ACT_DEACTIVATED))
        {/* Incoming connections on browsing channel should be accepted */
            dm_security_level_t secIncomingBrow = 0;
            
#ifndef INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS
            CsrBtScSetSecInLevel(&secIncomingBrow, CSR_BT_SEC_DEFAULT,
                                  CSR_BT_AV_RCP_MANDATORY_SECURITY_INCOMING,
                                  CSR_BT_AV_RCP_DEFAULT_SECURITY_INCOMING,
                                  CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
                                  CSR_BT_RESULT_CODE_AVRCP_UNACCEPTABLE_PARAMETER);
#else
            secIncomingBrow = instData->secIncomingBrow;
#endif /* INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS */

            CsrBtCml2caFecConnectAcceptSecondaryReqSend(CSR_BT_AVRCP_IFACEQUEUE,
                                                        CSR_BT_AVCTP_BROWSING_PSM,
                                                        (CsrUint24)(tgSupport ? CSR_BT_CAPTURING_MAJOR_SERVICE_MASK : 0), /* CoD */
                                                        secIncomingBrow,
                                                        instData->mtu,
                                                        L2CA_FLUSH_TO_INFINITE,            /* Flush timeout */
                                                        NULL,                              /* QoS */
                                                        CsrBtAvrcpUtilGetFlow(instData->mtu),   /* Flow */
                                                        FALSE,                             /* Fallback basic-mode */
                                                        (uuid16_t)(tgSupport ? CSR_BT_AV_REMOTE_CONTROL_TARGET_UUID : CSR_BT_AV_REMOTE_CONTROL_UUID), /* UUID for use in authorise indications */
                                                        CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                                               CSR_BT_AVRCP_DEFAULT_ENC_KEY_SIZE_VAL));

            AVRCP_CHANGE_STATE(instData->activateStateBrow, AVRCP_STATE_ACT_ACTIVATED);
        }
#endif

#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
        /* Only accept connections on Obex channel if AVRCP version 1.6 or higher and cover art is locally supported */
        if ((instData->srAvrcpVersionHighest >= CSR_BT_AVRCP_CONFIG_SR_VERSION_16) &&
            (CSR_MASK_IS_SET(instData->tgLocal.srFeatures, CSR_BT_AVRCP_CONFIG_SR_FEAT_COVER_ART)))
        {/* Incoming connections on Obex channel should be accepted */
            if (instData->activateStateCoverArt == AVRCP_STATE_ACT_DEACTIVATED)
            {
                CsrBtDeviceAddr address;
                CsrBtBdAddrZero(&address);
                CsrBtAvrcpImagingServerActivateReqSend(instData->tgLocal.obexPsm,
                                                       address,
                                                       CSR_BT_AVRCP_CONNECTION_ID_INVALID,
                                                       instData->secIncomingCont,
                                                       CSR_SCHED_TID_INVALID);
                AVRCP_CHANGE_STATE(instData->activateStateCoverArt, AVRCP_STATE_ACT_ACTIVATING);
            }
            else if (instData->activateStateCoverArt != AVRCP_STATE_ACT_ACTIVATED)
            {/* Activation / Deactivation in progress */
                AVRCP_CHANGE_STATE(instData->activateStateCoverArt, AVRCP_STATE_ACT_ACTIVATE_PENDING);
            }
            else
            {/*Already Activated */
            }
        }
#endif
    }
}

CsrBool CsrBtAvrcpUtilConnectAcceptCancel(AvrcpInstanceData_t *instData)
{
    CsrBool cancelSent = FALSE;

#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
    if (instData->activateStateCoverArt == AVRCP_STATE_ACT_ACTIVATED)
    {
        CsrBtAvrcpImagingServerDeactivateReqSend(instData->tgLocal.obexPsm, CSR_BT_AVRCP_CONNECTION_ID_INVALID);
        AVRCP_CHANGE_STATE(instData->activateStateCoverArt, AVRCP_STATE_ACT_DEACTIVATING);
    }
    else if (instData->activateStateCoverArt != AVRCP_STATE_ACT_DEACTIVATED)
    {/* Activation / Deactivation in progress */
        AVRCP_CHANGE_STATE(instData->activateStateCoverArt, AVRCP_STATE_ACT_DEACTIVATE_PENDING);
    }
    else
    {/*Already Deactivated */
    }
#endif
    if (instData->activateStateCont == AVRCP_STATE_ACT_ACTIVATED)
    {
        AVRCP_CHANGE_STATE(instData->activateStateCont, AVRCP_STATE_ACT_DEACTIVATED);
        CsrBtCml2caCancelConnectAcceptReqSend(CSR_BT_AVRCP_IFACEQUEUE, CSR_BT_AVCTP_PSM);
        cancelSent = TRUE;
    }
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    if (instData->activateStateBrow == AVRCP_STATE_ACT_ACTIVATED)
    {
        AVRCP_CHANGE_STATE(instData->activateStateBrow, AVRCP_STATE_ACT_DEACTIVATED);
        CsrBtCml2caCancelConnectAcceptReqSend(CSR_BT_AVRCP_IFACEQUEUE, CSR_BT_AVCTP_BROWSING_PSM);
        cancelSent = TRUE;
    }
#endif
    return cancelSent;
}

void CsrBtAvrcpUtilNewConnEstablished(AvrcpInstanceData_t *instData,
                                      AvrcpConnInstance_t *connInst,
                                      AvrcpConDir conDir)
{
#ifdef CSR_BT_RESTRICT_MAX_PROFILE_CONNECTIONS
    if (instData->activateStateCont == AVRCP_STATE_ACT_ACTIVATED && 
        instData->numActiveAvrcpConns == instData->incomingMaximum)
    {
        CsrBtAvrcpUtilConnectAcceptCancel(instData);
    }
#endif

    if (conDir == INCOMING)
    {
        instData->incomingCurrent++;
        connInst->connDirection = AVRCP_CONN_DIR_INCOMING;
        CsrBtAvrcpConnectIndSend(instData->ctrlHandle, &connInst->address,
                                 connInst->control.mtu, connInst->appConnId,
                                 connInst->control.btConnId);

        /* Start a SDP search */
        CsrBtAvrcpSdpSearchStart(instData, connInst);
    }
    else
    {
        connInst->connDirection = AVRCP_CONN_DIR_OUTGOING;
        if (conDir == COLLISION)
        {
            instData->incomingCurrent++;
            connInst->connDirection = AVRCP_CONN_DIR_INCOMING;
        }
        CsrBtAvrcpConnectCfmSend(instData->ctrlHandle,
                                 &connInst->address,
                                 connInst->control.mtu,
                                 connInst->appConnId,
                                 instData->tgDetails,
                                 instData->ctDetails,
                                 CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
                                 CSR_BT_SUPPLIER_AVRCP,
                                 connInst->control.btConnId);
    }

    connInst->control.dataSendAllowed   = TRUE;
#ifdef CSR_STREAMS_ENABLE
    CsrBtAvrcpStreamsRegister(instData, connInst->control.btConnId);
#endif
}

CsrBool CsrBtAvrcpUtilCancelSavedMessage(AvrcpInstanceData_t *instData, CsrBtAvrcpPrim primType, CsrBtDeviceAddr *addr)
{
    CsrUint16                eventClass;
    void                    *msg;
    CsrBool                  cancelledMsg    = FALSE;
    CsrMessageQueueType    *tempQueue      = NULL;

    while(CsrMessageQueuePop(&instData->saveQueue, &eventClass, &msg))
    {
        if (!cancelledMsg && (CSR_BT_AVRCP_PRIM == eventClass) && (primType == (*((CsrBtCmPrim *) msg))))
        {
            switch (primType)
            {
                case CSR_BT_AVRCP_CONNECT_REQ:
                {
                    CsrBtAvrcpConnectReq * prim = (CsrBtAvrcpConnectReq *) msg;

                    if (CsrBtBdAddrEq(&prim->deviceAddr, addr))
                    {
                        cancelledMsg = TRUE;
                        SynergyMessageFree(CSR_BT_AVRCP_PRIM, msg);
                    }
                    else
                    {
                        CsrMessageQueuePush(&tempQueue, eventClass, msg);
                    }
                    break;
                }
                default:
                {
                    CsrMessageQueuePush(&tempQueue, eventClass, msg);
                    break;
                }
            }
        }
        else
        {
            CsrMessageQueuePush(&tempQueue, eventClass, msg);
        }
    }
    instData->saveQueue = tempQueue;
    return (cancelledMsg);
}

void CsrBtAvrcpUtilGo2Busy(AvrcpInstanceData_t *instData, CsrBtAvrcpPrim primType)
{
    AVRCP_CHANGE_STATE(instData->appState, AVRCP_STATE_APP_BUSY);
    instData->pendingCtrlPrim = primType;
}

void CsrBtAvrcpUtilGo2Idle(AvrcpInstanceData_t *instData)
{
    AVRCP_CHANGE_STATE(instData->appState, AVRCP_STATE_APP_IDLE);

    if (instData->saveQueue)
    {
        instData->restoreFlag = TRUE;
        avrcpSendHouseCleaning(instData);
    }
    CsrBtAvrcpUtilFreeRoleDetails(instData->tgDetails);
    CsrBtAvrcpUtilFreeRoleDetails(instData->ctDetails);
    CsrPmemFree(instData->tgDetails);
    CsrPmemFree(instData->ctDetails);
    instData->tgDetails = NULL;
    instData->ctDetails = NULL;

    instData->pendingCtrlPrim = CSR_BT_AVRCP_HOUSE_CLEANING;
}

void CsrBtAvrcpUtilSaveMessage(AvrcpInstanceData_t *instData)
{
    CsrMessageQueuePush(&instData->saveQueue, CSR_BT_AVRCP_PRIM, instData->recvMsgP);
    instData->recvMsgP = NULL;
}

AvrcpConnInstance_t *CsrBtAvrcpUtilGetConnFromL2caCid(AvrcpInstanceData_t *instData, CsrBtConnId btConnId, CsrBtAvrcpConnDetails **connDetails)
{/* Return the correct connInst and connDetails from L2CAP btConnId */
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_C_CID((CsrCmnList_t *)&instData->connList, btConnId);

    if (connInst)
    {
        *connDetails = &connInst->control;
    }
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    else
    {
        connInst = AVRCP_LIST_CONN_GET_B_CID((CsrCmnList_t *)&instData->connList, btConnId);

        if (connInst)
        {
            *connDetails = &connInst->browsing;
        }
        else
        {
            *connDetails = NULL;
        }
    }
#endif

    return connInst;
}

#if defined(CSR_TARGET_PRODUCT_VM) && defined(CSR_LOG_ENABLE)
CsrUint8 AvrcpUtilGetPduIdFromPacket(CsrUint8* data)
{
    CsrUint8 pduindex;
    switch(AVRCP_DATA_AVC_OPCODE_GET(data))
    {
        case AVRCP_DATA_AVC_OPCODE_VENDOR_DEPENDENT: 
            pduindex = AVRCP_DATA_MD_PDU_ID_INDEX;
            break;
        case AVRCP_DATA_AVC_OPCODE_UNIT_INFO:
        case AVRCP_DATA_AVC_OPCODE_SUBUNIT_INFO:
            pduindex = AVRCP_DATA_AVC_UNIT_RES_UNIT_TYPE_INDEX;
            break;
        case AVRCP_DATA_AVC_OPCODE_PASS_THROUGH:
            if (AVRCP_DATA_PT_OPID_GET(data) == CSR_BT_AVRCP_PT_OP_ID_VENDOR_DEP)
            {
                /*Group Navigation*/
                pduindex = AVRCP_DATA_PT_GN_OPERATION_INDEX;
            }
            else
            {
                /*Normal Passthrough*/
                pduindex = AVRCP_DATA_PT_OPID_INDEX;
            }
            break;
        default:
            /*Should not get here.*/
            pduindex = AVRCP_DATA_AVC_OPCODE_INDEX;             
    }

    return data[pduindex];
}
#endif /* CSR_TARGET_PRODUCT_VM && CSR_LOG_ENABLE*/

AvrcpConnInstance_t *CsrBtAvrcpUtilConnAdd(AvrcpInstanceData_t *instData, CsrBtDeviceAddr *addr)
{
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_ADD_FIRST((CsrCmnList_t *)&instData->connList);

    /* Misc */
    connInst->instData                  = instData;
    connInst->appConnId                 = 0xFF; /* To avoid UMR in next step */
    connInst->appConnId                 = csrBtAvrcpUtilGetAppConnId(instData); /* Assign a unique connection ID */
    connInst->connDirection             = AVRCP_CONN_DIR_INVALID;
    connInst->reconnectTid              = 0;
    connInst->address                   = *addr;
    connInst->remoteFeatures            = 0;
    connInst->pendingNotifReg           = NULL;
    AVRCP_CHANGE_STATE_INDEX(connInst->sdpState, AVRCP_STATE_SDP_PENDING, connInst->appConnId);

    /* Control specific */
    connInst->control.btConnId       = AVRCP_CID_INVALID;
    connInst->control.mtu               = AVRCP_MTU_INVALID;
    connInst->control.ctTLabel          = 0;
    connInst->control.dataSendAllowed   = FALSE;
    connInst->pendingRxDataBufferLen    = 0;
    connInst->pendingRxDataBuffer       = NULL;
    CsrMemSet((void *)&connInst->control.pendingTxDataList, 0, sizeof(connInst->control.pendingTxDataList));
    AVRCP_CHANGE_STATE_INDEX(connInst->control.state, AVRCP_STATE_CONN_DISCONNECTED, connInst->appConnId);

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    /* Browsing specific */
    connInst->browsing.btConnId      = AVRCP_CID_INVALID;
    connInst->browsing.mtu              = AVRCP_MTU_INVALID;
    connInst->browsing.ctTLabel         = 0;
    connInst->browsing.dataSendAllowed  = FALSE;
    CsrMemSet((void *)&connInst->browsing.pendingTxDataList, 0, sizeof(connInst->browsing.pendingTxDataList));
    AVRCP_CHANGE_STATE_INDEX(connInst->browsing.state, AVRCP_STATE_CONN_DISCONNECTED, connInst->appConnId);
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
    /* Target specific */
#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
    CsrBtAvrcpTgUtilInitConnLocal(instData, &connInst->tgLocal);
#endif

    /* Controller specific */
#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
    CsrBtAvrcpCtUtilInitConnLocal(&connInst->ctLocal);
#endif

    connInst->resetAppState = FALSE;
    return connInst;
}

CsrBool CsrBtAvrcpUtilConnRemove(CsrCmnListElm_t *elem, void *data)
{
    AvrcpConnInstance_t *connInst = (AvrcpConnInstance_t *)elem;

    CSR_UNUSED(data);

    CsrCmnListIterateAllowRemove((CsrCmnList_t *)&connInst->control.pendingTxDataList,
                                 CsrBtAvrcpUtilPendingDataRemove,
                                 NULL);
    connInst->control.pendingTxDataList.first = NULL;
    connInst->control.pendingTxDataList.count = 0;

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    CsrCmnListIterateAllowRemove((CsrCmnList_t *)&connInst->browsing.pendingTxDataList,
                                 CsrBtAvrcpUtilPendingDataRemove,
                                 NULL);
    connInst->browsing.pendingTxDataList.first = NULL;
    connInst->browsing.pendingTxDataList.count = 0;

#endif
    CsrPmemFree(connInst->pendingRxDataBuffer);
    CsrSchedTimerCancel(connInst->reconnectTid, NULL, NULL);

    if (connInst->connDirection == AVRCP_CONN_DIR_INCOMING)
    {
        connInst->instData->incomingCurrent--;
    }

#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
    CsrCmnListIterateAllowRemove((CsrCmnList_t *)&connInst->ctLocal->pendingMsgList,
                                 CsrBtAvrcpCtUtilMsgQueueRemove,
                                 NULL);
    connInst->ctLocal->pendingMsgList.first = NULL;
    connInst->ctLocal->pendingMsgList.count = 0;

    /* Make sure that notification parameter pointers don't have stray values left */
    CsrPmemFree(connInst->ctLocal->notiParams.attValPair);
    connInst->ctLocal->notiParams.attValPair = NULL;

    CsrPmemFree(connInst->ctLocal);
    connInst->ctLocal = NULL;
#endif

#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
    CsrCmnListIterateAllowRemove((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList,
                                 CsrBtAvrcpTgUtilMsgQueueRemove,
                                 NULL);
    connInst->tgLocal->pendingMsgList.first = NULL;
    connInst->tgLocal->pendingMsgList.count = 0;

    CsrPmemFree(connInst->tgLocal);
    connInst->tgLocal = NULL;
#endif

    return (TRUE);
}

void CsrBtAvrcpUtilPendingDataAddLast(CsrBtAvrcpConnDetails *connDetails, CsrUint16 dataLen, CsrUint8 *data)
{
    AvrcpPendingData_t *pendingData = AVRCP_LIST_TXDATA_ADD_LAST((CsrCmnList_t *)&connDetails->pendingTxDataList);
    pendingData->dataLen = dataLen;
    pendingData->data    = data;
}

CsrBool CsrBtAvrcpUtilPendingDataRemove(CsrCmnListElm_t *elem, void *data)
{
    CsrPmemFree(((AvrcpPendingData_t *)elem)->data);
    CSR_UNUSED(data);

    return (TRUE);
}

void CsrBtAvrcpUtilPendingDataSend(AvrcpConnInstance_t *connInst, CsrBtConnId connId)
{
    AvrcpPendingData_t *pendingData;
    CsrBtAvrcpConnDetails *connDetails = &connInst->control;

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    if (connDetails->btConnId != connId)
    {/* Make sure that the correct connection is used */
        connDetails = &connInst->browsing;
    }
#else
    CSR_UNUSED(connId);
#endif

    if (!connDetails->dataSendAllowed &&
        (connDetails->state == AVRCP_STATE_CONN_CONNECTED))
    {
        pendingData = AVRCP_LIST_TXDATA_GET_FIRST((CsrCmnList_t *)&connDetails->pendingTxDataList);

        if (pendingData)
        {
#ifdef CSR_STREAMS_ENABLE
            CsrStreamsDataSend(CM_GET_UINT16ID_FROM_BTCONN_ID(connDetails->btConnId),
                               L2CAP_ID,
                               pendingData->dataLen,
                               pendingData->data);
#else
            CsrBtCml2caDataReqSend(connDetails->btConnId,
                                   pendingData->dataLen,
                                   pendingData->data,
                                   CSR_BT_CM_CONTEXT_UNUSED);
#endif
            pendingData->data = NULL;
            AVRCP_LIST_TXDATA_REMOVE((CsrCmnList_t *)&connDetails->pendingTxDataList,
                                     pendingData);
        }
        else
        {/* No more pending messages: allow app data to be sent immediately in the future */
            connDetails->dataSendAllowed = TRUE;
        }
    }
}

void CsrBtAvrcpUtilFreeConfigReq(CsrBtAvrcpConfigReq **prim)
{
    CsrBtAvrcpUtilFreeRoleDetails(&((*prim)->tgDetails));
    CsrBtAvrcpUtilFreeRoleDetails(&((*prim)->ctDetails));
    SynergyMessageFree(CSR_BT_AVRCP_PRIM, *prim);
    *prim = NULL;
}


#endif

