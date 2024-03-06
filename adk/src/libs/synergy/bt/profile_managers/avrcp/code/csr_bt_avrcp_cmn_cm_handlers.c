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
#ifdef CSR_BT_INSTALL_AVRCP_COVER_ART
#include "csr_bt_avrcp_imaging_private_prim.h"
#include "csr_bt_avrcp_imaging_private_lib.h"
#include "csr_bt_avrcp_imaging_main.h"
#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
#include "csr_bt_avrcp_imaging_client_main.h"
#endif
#endif

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_avrcp_streams.h"
#endif

void CsrBtAvrcpCmL2caRegisterCfmHandler(AvrcpInstanceData_t *instData)
{
#if defined (CSR_BT_INSTALL_AVRCP_BROWSING) || defined (CSR_BT_INSTALL_AVRCP_TG_COVER_ART)
    CsrBtCmL2caRegisterCfm *prim = (CsrBtCmL2caRegisterCfm *)(instData->recvMsgP);
#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
    if (prim->context == AVRCP_TG_IMAGING_L2CA_PSM_CONTEXT_ID)
    {
        instData->tgLocal.obexPsm = prim->localPsm;
    }
#endif
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    if (prim->localPsm == CSR_BT_AVCTP_BROWSING_PSM)
#endif /* CSR_BT_INSTALL_ACRCP_BROWSING */
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING || CSR_BT_INSTALL_AVRCP_TG_COVER_ART*/
    {/* Then we assume the the AVCTP PSM has alredy been registered and this is the last one: go to idle */
        CsrBtAvrcpUtilGo2Idle(instData);
    }
}

/***** CM_SDS_X handling *****/
void CsrBtAvrcpCmSdsRegisterCfmHandler(AvrcpInstanceData_t *instData)
{
    CsrBtCmSdsRegisterCfm *prim = (CsrBtCmSdsRegisterCfm *) (instData->recvMsgP);

    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        /* Update correct service record handle */
        if (AVRCP_ROLE_CONTROLLER == instData->srActiveRole)
        {
#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
            instData->ctLocal.srHandle = prim->serviceRecHandle;
#endif
        }
        else if (AVRCP_ROLE_TARGET == instData->srActiveRole)
        {
#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
            instData->tgLocal.srHandle = prim->serviceRecHandle;
#endif
        }

        if (!CsrBtAvrcpSdpRegisterSR(instData))
        {/* No more records to register */
            if (instData->srActiveRole != AVRCP_ROLE_INVALID)
            {
                CsrBtAvrcpConfigCfmSend(instData->ctrlHandle,
                    CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
                    CSR_BT_SUPPLIER_AVRCP);
                AVRCP_CHANGE_STATE(instData->srActiveRole, AVRCP_ROLE_INVALID);
                CsrBtAvrcpUtilGo2Idle(instData);
            }
        }
    }
    else
    {/* Registration failed -- propagate error */
        CsrBtAvrcpUtilFreeConfigReq(&instData->srPending);
        CsrBtAvrcpConfigCfmSend(instData->ctrlHandle,
            prim->resultCode, prim->resultSupplier);

        CsrBtAvrcpUtilGo2Idle(instData);
    }
}

void CsrBtAvrcpCmSdsUnregisterCfmHandler(AvrcpInstanceData_t *instData)
{
    CsrBtCmSdsUnregisterCfm *prim = (CsrBtCmSdsUnregisterCfm *) (instData->recvMsgP);
    CsrBool                  unregComplete = FALSE;

    /* Determine if both records are unregistered and update handles */
    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
        if (prim->serviceRecHandle == instData->tgLocal.srHandle)
        {
            instData->tgLocal.srHandle = AVRCP_SDP_INVALID_SR_HANDLE;
#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
            if (instData->ctLocal.srHandle == AVRCP_SDP_INVALID_SR_HANDLE)
#endif
            {/* The controller record is already unregistered */
                unregComplete = TRUE;
            }
        }
#endif
#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
        if (prim->serviceRecHandle == instData->ctLocal.srHandle)
        {
            instData->ctLocal.srHandle = AVRCP_SDP_INVALID_SR_HANDLE;
#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
            if (instData->tgLocal.srHandle == AVRCP_SDP_INVALID_SR_HANDLE)
#endif
            {
                unregComplete = TRUE;
            }
        }
#endif
        if (unregComplete)
        {/* Determine if new records should be registered */
            if((instData->srPending) && (!instData->srPending->tgDetails.roleSupported) && (!instData->srPending->ctDetails.roleSupported))
            {
                CsrBtAvrcpUtilGo2Idle(instData);
                CsrBtAvrcpUtilFreeConfigReq(&instData->srPending);
                CsrBtAvrcpConfigCfmSend(instData->ctrlHandle,
                    CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
                    CSR_BT_SUPPLIER_AVRCP);
                AVRCP_CHANGE_STATE(instData->srActiveRole, AVRCP_ROLE_INVALID);
            }
            else if(!CsrBtAvrcpSdpRegisterSR(instData))
            {
                if(instData->srActiveRole != AVRCP_ROLE_INVALID)
                {
                    CsrBtAvrcpUtilGo2Idle(instData);
                    CsrBtAvrcpConfigCfmSend(instData->ctrlHandle,
                        CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
                        CSR_BT_SUPPLIER_AVRCP);
                    AVRCP_CHANGE_STATE(instData->srActiveRole, AVRCP_ROLE_INVALID);
                }
            }
        }
    }
    else
    {
        CsrBtAvrcpUtilFreeConfigReq(&instData->srPending);
        CsrBtAvrcpUtilGo2Idle(instData);
        CsrBtAvrcpConfigCfmSend(instData->ctrlHandle,
            prim->resultCode, prim->resultSupplier);
    }
}

static void avrcpUpdateActiveConnRecord(AvrcpInstanceData_t *instData, CsrBool connected)
{
#ifdef CSR_BT_RESTRICT_MAX_PROFILE_CONNECTIONS
    /*  Number of active AVRCP connections is tracked only for control channel */
    if (connected && instData->numActiveAvrcpConns < instData->incomingMaximum)
    {
        instData->numActiveAvrcpConns++;
    }
    else if (!connected && instData->numActiveAvrcpConns)
    {
        instData->numActiveAvrcpConns--;
    }
    else
    {
        CsrPanic(CSR_TECH_BT,
                 CSR_BT_PANIC_MYSTERY,
                 "Invalid instData->numActiveAvrcpConns");
    }
#else
    CSR_UNUSED(instData);
    CSR_UNUSED(connected);
#endif
}

/* Incoming connection established */
void CsrBtAvrcpCmL2caConnectAcceptCfmHandler(AvrcpInstanceData_t *instData)
{
    CsrBtCmL2caConnectAcceptCfm *cmPrim = (CsrBtCmL2caConnectAcceptCfm *)instData->recvMsgP;

    /* A device connected successfully - determine if it is known */
    if (cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        AvrcpConnInstance_t *connInst;
        connInst = AVRCP_LIST_CONN_GET_ADDR((CsrCmnList_t *)(CsrCmnList_t *)&instData->connList, &cmPrim->deviceAddr);

        if (connInst)
        {/* Device is known */
            if (CSR_BT_AVCTP_PSM == cmPrim->localPsm)
            {/* A control channel was established */
                CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL, cmPrim->deviceAddr,cmPrim->btConnId);
                if ((AVRCP_STATE_CONN_CONNECTING == connInst->control.state) &&
                    (instData->incomingMaximum > instData->incomingCurrent))
                {
                    /* The connection was established from a device in the process of being connected to -
                    attempt to cancel outgoing connection */
                    CsrBtCml2caCancelConnectReqSend(CSR_BT_AVRCP_IFACEQUEUE, connInst->address, CSR_BT_AVCTP_PSM);

                    /* initialize and report this connection to the app */
                    connInst->control.btConnId = cmPrim->btConnId;
                    connInst->control.mtu = cmPrim->mtu;

                    avrcpUpdateActiveConnRecord(instData, TRUE /* Connected */);
                    CsrBtAvrcpUtilNewConnEstablished(instData, connInst, COLLISION);
                    AVRCP_CHANGE_STATE_INDEX(connInst->control.state, AVRCP_STATE_CONN_CONNECTED, connInst->appConnId);
                }
                else
                {/* A connection is already present to the device - disconnect new connection */
                    CsrBtCml2caDisconnectReqSend(cmPrim->btConnId); /* Crossing connections */
                }
            }
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
            else
            {/* The browsing channel was established */
                CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_DATA_CHANNEL, cmPrim->deviceAddr,cmPrim->btConnId);

                if (connInst->control.state == AVRCP_STATE_CONN_CONNECTED)
                {
                    AvrcpPendingData_t *pendingData = AVRCP_LIST_TXDATA_GET_FIRST((CsrCmnList_t *)&connInst->browsing.pendingTxDataList);

                    if (connInst->browsing.state == AVRCP_STATE_CONN_CONNECTED)
                    {  /* Crossing browsing connections */
                        CsrBtCml2caDisconnectReqSend(cmPrim->btConnId);
                        CsrBtCml2caDisconnectReqSend(connInst->browsing.btConnId);

                        if (pendingData)
                        { /* The application has requested the local device to send a browsing
                             command. Set browsing state to AVRCP_STATE_CONN_DISC2RECONNECT 
                             in order to reconnect */
                            AVRCP_CHANGE_STATE_INDEX(connInst->browsing.state, AVRCP_STATE_CONN_DISC2RECONNECT, connInst->appConnId);
                        }
                        else
                        { /* The local device do not need to send any browsing command. E.g
                             let the peer device re-establish the connection or wait to 
                             re-establish until it is needed */
                            ;
                        }
                    }
                    else
                    {
                        connInst->browsing.btConnId         = cmPrim->btConnId;
                        connInst->browsing.mtu              = cmPrim->mtu;
                        AVRCP_CHANGE_STATE_INDEX(connInst->browsing.state, AVRCP_STATE_CONN_CONNECTED, connInst->appConnId);
#ifdef CSR_STREAMS_ENABLE
                        CsrBtAvrcpStreamsRegister(instData, cmPrim->btConnId);
#endif

                        if (pendingData)
                        {/* An incoming browsing channel must be crossing with an outgoing one. The outgoing
                            one were being established due to a pending command - send data */
#ifdef CSR_STREAMS_ENABLE
                            CsrStreamsDataSend(CM_GET_UINT16ID_FROM_BTCONN_ID(cmPrim->btConnId),
                                               L2CAP_ID,
                                               pendingData->dataLen,
                                               pendingData->data);
#else
                            CsrBtCml2caDataReqSend(cmPrim->btConnId,
                                                   pendingData->dataLen,
                                                   pendingData->data,
                                                   CSR_BT_CM_CONTEXT_UNUSED);
#endif
                            pendingData->data = NULL;
                            AVRCP_LIST_TXDATA_REMOVE((CsrCmnList_t *)&connInst->browsing.pendingTxDataList,
                                                     pendingData);
                        }
                        else
                        {
                            connInst->browsing.dataSendAllowed  = TRUE;
                        }
                    }
                }
                else
                {/* A browsing channel was established when no control channel is present*/
                    CsrBtCml2caDisconnectReqSend(cmPrim->btConnId);
                }
            }
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
        }
        else
        {/* Device is unknown */
            if ((instData->incomingMaximum > instData->incomingCurrent) && (cmPrim->localPsm == CSR_BT_AVCTP_PSM))
            {/* New incoming connections are allowed - only accept control as first connection */
                CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL, cmPrim->deviceAddr,cmPrim->btConnId);
                connInst = CsrBtAvrcpUtilConnAdd(instData, &cmPrim->deviceAddr);
                AVRCP_CHANGE_STATE_INDEX(connInst->control.state, AVRCP_STATE_CONN_CONNECTED, connInst->appConnId);
                connInst->control.btConnId = cmPrim->btConnId;
                connInst->control.mtu = cmPrim->mtu;
                avrcpUpdateActiveConnRecord(instData, TRUE /* Connected */);
                CsrBtAvrcpUtilNewConnEstablished(instData, connInst, INCOMING);
            }
            else
            {/* Incoming connections are not being accepted or this is a browsing channel */
                CsrBtCml2caDisconnectReqSend(cmPrim->btConnId);
            }
        }
    }
    else
    {/* Error - ignore*/
    }

    /* Update the correct states */
    if (cmPrim->localPsm == CSR_BT_AVCTP_PSM)
    {
        AVRCP_CHANGE_STATE(instData->activateStateCont, AVRCP_STATE_ACT_DEACTIVATED);
    }
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    else if (cmPrim->localPsm == CSR_BT_AVCTP_BROWSING_PSM)
    {
        AVRCP_CHANGE_STATE(instData->activateStateBrow, AVRCP_STATE_ACT_DEACTIVATED);
    }
#endif
    /* Determine if more incoming connections should be accepted */
    CsrBtAvrcpUtilConnectAccept(instData);
}

#ifdef INSTALL_AVRCP_DEACTIVATE
/* Incoming connections rejected */
void CsrBtAvrcpCmL2caCancelConnectAcceptCfmHandler(AvrcpInstanceData_t *instData)
{
    if (instData->pendingCtrlPrim == CSR_BT_AVRCP_DEACTIVATE_REQ)
    {/* ConnectAccept was cancelled due to deactivation */
        CsrBtCmL2caCancelConnectAcceptCfm *cmPrim = (CsrBtCmL2caCancelConnectAcceptCfm *)instData->recvMsgP;

        if (cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
            cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM)
        {
            CsrBtAvrcpDeactivateCfmSend(instData->ctrlHandle,
                CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
                CSR_BT_SUPPLIER_AVRCP);
        }
        else
        {
            CsrBtAvrcpDeactivateCfmSend(instData->ctrlHandle, cmPrim->resultCode, cmPrim->resultSupplier);
        }
        CsrBtAvrcpUtilGo2Idle(instData);
    }
}
#endif /* INSTALL_AVRCP_DEACTIVATE */

static void csrBtAvrcpCompletePendingOp(AvrcpInstanceData_t *instData,
                                        AvrcpConnInstance_t *connInst,
                                        CsrBtReasonCode reasonCode,
                                        CsrBtSupplier reasonSupplier,
                                        CsrBool localTerminated)
{
    switch (instData->pendingCtrlPrim)
    {
        case CSR_BT_AVRCP_CONNECT_REQ:
        {
            CsrBtAvrcpConnectCfmSend(instData->ctrlHandle,
                                     &connInst->address,
                                     AVRCP_MTU_INVALID,
                                     CSR_BT_AVRCP_CONNECTION_ID_INVALID,
                                     NULL,
                                     NULL,
                                     reasonCode,
                                     reasonSupplier,
                                     CSR_BT_CONN_ID_INVALID);

            CsrBtAvrcpUtilGo2Idle(instData);
            break;
        }

#ifdef INSTALL_AVRCP_CANCEL_CONNECT
        case CSR_BT_AVRCP_CANCEL_CONNECT_REQ:
        {
            CsrBtAvrcpConnectCfmSend(instData->ctrlHandle,
                                     &connInst->address,
                                     AVRCP_MTU_INVALID,
                                     CSR_BT_AVRCP_CONNECTION_ID_INVALID,
                                     NULL,
                                     NULL,
                                     CSR_BT_RESULT_CODE_AVRCP_CONNECT_ATTEMPT_CANCELLED,
                                     CSR_BT_SUPPLIER_AVRCP,
                                     CSR_BT_CONN_ID_INVALID);

            CsrBtAvrcpUtilGo2Idle(instData);
            break;
        }
#endif /* INSTALL_AVRCP_CANCEL_CONNECT */

        case CSR_BT_AVRCP_DISCONNECT_REQ:
        {
            CsrBtAvrcpDisconnectIndSend(instData->ctrlHandle,
                connInst->appConnId,
                reasonCode,
                reasonSupplier,
                localTerminated);
            CsrBtAvrcpUtilGo2Idle(instData);
            break;
        }

#ifdef INSTALL_AVRCP_DEACTIVATE
        case CSR_BT_AVRCP_DEACTIVATE_REQ: /* Fall-through */
#endif
        case CSR_BT_AVRCP_CONFIG_REQ: /* Fall-through */
        default:
        {
            CsrBtAvrcpDisconnectIndSend(instData->ctrlHandle,
                connInst->appConnId,
                reasonCode,
                reasonSupplier,
                localTerminated);
            break;
        }
    }
}

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
static CsrBool avrcpIsInitiateBrowsingDisabled(AvrcpInstanceData_t *instData)
{
    CsrBool browsingDisabled = FALSE;

#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
    if (instData->tgLocal.configuration & AVRCP_CONFIG_ROLE_NO_BROWSING_AFTER_CONTROL)
    {
        browsingDisabled = TRUE;
    }
#endif
#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
    if (instData->ctLocal.configuration & AVRCP_CONFIG_ROLE_NO_BROWSING_AFTER_CONTROL)
    {
        browsingDisabled = TRUE;
    }
#endif

    return (browsingDisabled);
}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

/* Outgoing connection established */
void CsrBtAvrcpCmL2caConnectCfmHandler(AvrcpInstanceData_t *instData)
{
    CsrBtCmL2caConnectCfm *cmPrim = (CsrBtCmL2caConnectCfm*)instData->recvMsgP;
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_ADDR((CsrCmnList_t *)&instData->connList, &cmPrim->deviceAddr);

    if (connInst)
    {/* Connection instance is known */
        switch (connInst->control.state)
        {
            case AVRCP_STATE_CONN_CONNECTING:
            {/* Correct connection identified */
                if (cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM &&
                    cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
                    cmPrim->localPsm == CSR_BT_AVCTP_PSM)
                {/* New outgoing control connection established by request from app */
                    connInst->control.btConnId = cmPrim->btConnId;
                    connInst->control.mtu = cmPrim->mtu;
                    connInst->control.dataSendAllowed   = TRUE;
                    AVRCP_CHANGE_STATE_INDEX(connInst->control.state, AVRCP_STATE_CONN_CONNECTED, connInst->appConnId);
                    avrcpUpdateActiveConnRecord(instData, TRUE /* Connected */);
                    CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL, cmPrim->deviceAddr,cmPrim->btConnId);

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                    if (!CSR_MASK_IS_SET(connInst->remoteFeatures, CSR_BT_AVRCP_CONFIG_SR_FEAT_BROWSING) ||
                        avrcpIsInitiateBrowsingDisabled(instData))
#endif
                    {/* Browsing is not supported or Browsing connection not allowed after control channel connection */
#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
                        /* Establish obex connection if PSM is valid */
                        if (connInst->ctLocal->obexPsm != L2CA_PSM_INVALID)
                        {
                            dm_security_level_t secOutgoingCont = 0;

#ifndef INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS
                            CsrBtScSetSecOutLevel(&secOutgoingCont, CSR_BT_SEC_DEFAULT,
                                             CSR_BT_AV_RCP_MANDATORY_SECURITY_OUTGOING,
                                             CSR_BT_AV_RCP_DEFAULT_SECURITY_OUTGOING,
                                             CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
                                             CSR_BT_RESULT_CODE_AVRCP_UNACCEPTABLE_PARAMETER);
#else
                            secOutgoingCont = instData->secOutgoingCont;
#endif /* INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS */

                            connInst->ctLocal->ctObexState = AVRCP_CT_OBEX_CONNECTING_OUT;
                            /* Establish an obex connection */
                            CsrBtAvrcpImagingClientConnectReqSend(connInst->appConnId, 
                                                                  connInst->address, 
                                                                  connInst->ctLocal->obexPsm, 
                                                                  secOutgoingCont);
                        }
                        else
                        {
#endif /* CSR_BT_INSTALL_AVRCP_CT_COVER_ART */
                            CsrBtAvrcpUtilNewConnEstablished(instData, connInst, OUTGOING);
                            CsrBtAvrcpUtilGo2Idle(instData);
#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
                        }
#endif
                    }
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                    else
                    {/* Establish the browsing channel */
                        CsrBtAvrcpUtilConnect(connInst);
                    }
#endif
                }
                else
                {/* Connection failed - report error */
#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
                    if ((connInst->tgLocal->obexState != AVRCP_TG_OBEX_SERVER_IDLE) &&
                        (connInst->tgLocal->obexState != AVRCP_TG_OBEX_SERVER_DEACTIVATING))
                    {
                        /* Remove OBEX server instance since outgoing AVRCP control connection failed */
                        CsrBtAvrcpImagingServerDeactivateReqSend(connInst->instData->tgLocal.obexPsm, connInst->appConnId);
                    }
#endif
                    CsrBtAvrcpConnectCfmSend(instData->ctrlHandle,
                                             &cmPrim->deviceAddr,
                                             AVRCP_MTU_INVALID,
                                             CSR_BT_AVRCP_CONNECTION_ID_INVALID,
                                             NULL,
                                             NULL,
                                             cmPrim->resultCode,
                                             cmPrim->resultSupplier,
                                             CSR_BT_CONN_ID_INVALID);

                    CsrBtAvrcpUtilConnRemove((CsrCmnListElm_t *)connInst, NULL);
                    AVRCP_LIST_CONN_REMOVE((CsrCmnList_t *)&instData->connList, connInst);
                    CsrBtAvrcpUtilGo2Idle(instData);
                }
                break;
            }

            case AVRCP_STATE_CONN_CANCELLING:
            {/* Connection is in the process of being cancelled */
                if (cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM &&
                    cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                {/* Cancel did not succeed - disconnect */
                    connInst->control.btConnId = cmPrim->btConnId;
                    CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL, cmPrim->deviceAddr,cmPrim->btConnId);
                    CsrBtCml2caDisconnectReqSend(cmPrim->btConnId);
                }
                else
                {/* Connection was cancelled or failed - just ignore */
                    CsrBtAvrcpConnectCfmSend(instData->ctrlHandle,
                                             &cmPrim->deviceAddr,
                                             AVRCP_MTU_INVALID,
                                             CSR_BT_AVRCP_CONNECTION_ID_INVALID,
                                             NULL,
                                             NULL,
                                             CSR_BT_RESULT_CODE_AVRCP_CONNECT_ATTEMPT_CANCELLED,
                                             CSR_BT_SUPPLIER_AVRCP,
                                             CSR_BT_CONN_ID_INVALID);

                    CsrBtAvrcpUtilConnRemove((CsrCmnListElm_t *)connInst, NULL);
                    AVRCP_LIST_CONN_REMOVE((CsrCmnList_t *)&instData->connList, connInst);
                    CsrBtAvrcpUtilGo2Idle(instData);
                }
                break;
            }

            case AVRCP_STATE_CONN_DISCONNECTED:
            {/* Control channel is already disconnected.
              * This connect cfm must be for browsing channel */
                if (cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM &&
                    cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                {
                    CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_DATA_CHANNEL,
                                                     cmPrim->deviceAddr,
                                                     cmPrim->btConnId);
                    CsrBtCml2caDisconnectReqSend(cmPrim->btConnId);
                }
                else
                {/* Connection failed - inform application */
                    csrBtAvrcpCompletePendingOp(instData,
                                                connInst,
                                                cmPrim->resultCode,
                                                cmPrim->resultSupplier,
                                                TRUE);
                    CsrBtAvrcpUtilConnRemove((CsrCmnListElm_t *)connInst, NULL);
                    AVRCP_LIST_CONN_REMOVE((CsrCmnList_t *)&instData->connList, connInst);
                }
                break;
            }

            case AVRCP_STATE_CONN_CONNECTED:
            {/* Control channel is already established */
                if (cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM &&
                    cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                {
                    if (cmPrim->localPsm == CSR_BT_AVCTP_PSM)
                    {/* Crossing control connections */
                        AVRCP_CHANGE_STATE_INDEX(connInst->control.state, AVRCP_STATE_CONN_DISC2RECONNECT, connInst->appConnId);
                        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL, cmPrim->deviceAddr,cmPrim->btConnId);
                        CsrBtCml2caDisconnectReqSend(cmPrim->btConnId);
                        CsrBtCml2caDisconnectReqSend(connInst->control.btConnId);
                    }
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                    else
                    { /* The browsing channel has been connected */
                        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_DATA_CHANNEL, cmPrim->deviceAddr,cmPrim->btConnId);
                        if (connInst->browsing.state == AVRCP_STATE_CONN_CONNECTED)
                        {/* Crossing browsing connections. Release both browsing channels */
                            AVRCP_CHANGE_STATE_INDEX(connInst->browsing.state, AVRCP_STATE_CONN_DISC2RECONNECT, connInst->appConnId);
                            CsrBtCml2caDisconnectReqSend(cmPrim->btConnId);
                            CsrBtCml2caDisconnectReqSend(connInst->browsing.btConnId);
                        }
                        else if (connInst->browsing.state == AVRCP_STATE_CONN_CONNECTING)
                        {
                            AvrcpPendingData_t *pendingData = AVRCP_LIST_TXDATA_GET_FIRST((CsrCmnList_t *)&connInst->browsing.pendingTxDataList);
                            connInst->browsing.btConnId = cmPrim->btConnId;
                            connInst->browsing.mtu = cmPrim->mtu;
                            AVRCP_CHANGE_STATE_INDEX(connInst->browsing.state, AVRCP_STATE_CONN_CONNECTED, connInst->appConnId);
#ifdef CSR_STREAMS_ENABLE
                            CsrBtAvrcpStreamsRegister(instData, cmPrim->btConnId);
#endif
                            if (pendingData)
                            {/* Browsing channel was established due to a pending command - send data */
#ifdef CSR_STREAMS_ENABLE
                                CsrStreamsDataSend(CM_GET_UINT16ID_FROM_BTCONN_ID(cmPrim->btConnId),
                                                   L2CAP_ID,
                                                   pendingData->dataLen,
                                                   pendingData->data);
#else
                                CsrBtCml2caDataReqSend(cmPrim->btConnId,
                                                       pendingData->dataLen,
                                                       pendingData->data,
                                                       CSR_BT_CM_CONTEXT_UNUSED);
#endif
                                pendingData->data = NULL;
                                AVRCP_LIST_TXDATA_REMOVE(&connInst->browsing.pendingTxDataList, pendingData);
                            }
                            else
                            { /* A browsing channel were establish right after setting up the control channel */
                                connInst->browsing.dataSendAllowed  = TRUE;
#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
                                if (connInst->ctLocal->obexPsm != L2CA_PSM_INVALID)
                                {
                                    /* Establish an obex connection */
                                    connInst->ctLocal->ctObexState = AVRCP_CT_OBEX_CONNECTING_OUT;
                                    CsrBtAvrcpImagingClientConnectReqSend(connInst->appConnId, 
                                                                          connInst->address, 
                                                                          connInst->ctLocal->obexPsm, 
                                                                          instData->secOutgoingCont);
                                    return;
                                }
                                else
                                {
#endif
                                    CsrBtAvrcpUtilNewConnEstablished(instData, connInst, OUTGOING);
#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
                                }
#endif
                            }
                        }
                    }
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
                }
                else
                {
                    if (cmPrim->localPsm == CSR_BT_AVCTP_PSM)
                    { /* Crossing control connections attempts. In this case the outgoing connection 
                         fails, E.g. AVRCP has only one control connection. Keep it */ 
                        ;
                    }
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                    else 
                    { /* Failed to establish a browsing channel */
                        if (connInst->browsing.state == AVRCP_STATE_CONN_CONNECTED)
                        {/* Crossing browsing connections. In this case the outgoing connection 
                         fails, E.g. AVRCP has only one browsing connection. Keep it */
                            ;
                        }
                        else
                        {
#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
                            if (AVRCP_LIST_TXDATA_GET_FIRST((CsrCmnList_t *)&connInst->browsing.pendingTxDataList))
                            {/* Browsing channel was attempted established due to a pending command (CT only) - find and complete pending msg */
                                CsrBtAvrcpCtUtilPendingMsgCompleteFromPsm(connInst,
                                                                          CSR_BT_AVCTP_BROWSING_PSM,
                                                                          CSR_BT_RESULT_CODE_AVRCP_CHANNEL_NOT_CONNECTED,
                                                                          CSR_BT_SUPPLIER_AVRCP);
                            }
                            else
#endif
                            { /* Browsing channel was attempted established just after control channel were established. 
                                 Send a connect confirm to the application with success, as AVRCP shall not use the 
                                 browsing channel right now, and the application may never use it */
                                 CsrBtAvrcpUtilNewConnEstablished(instData, connInst, OUTGOING);
                            }
                            AVRCP_CHANGE_STATE_INDEX(connInst->browsing.state, AVRCP_STATE_CONN_DISCONNECTED, connInst->appConnId);
                        }
                    }
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
                }
                CsrBtAvrcpUtilGo2Idle(instData);
                break;
            }

            case AVRCP_STATE_CONN_DISCONNECTING:
            {/* Control channel is disconnecting
              * This connect cfm must be for browsing channel */
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                if (cmPrim->localPsm == CSR_BT_AVCTP_BROWSING_PSM)
                {
                    if(cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM &&
                        cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                    {
                        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_DATA_CHANNEL, cmPrim->deviceAddr, cmPrim->btConnId);
                        CsrBtCml2caDisconnectReqSend(cmPrim->btConnId);
                    }
                    else
                    {
                        AVRCP_CHANGE_STATE_INDEX(connInst->browsing.state, AVRCP_STATE_CONN_DISCONNECTED, connInst->appConnId);
                    }
                }
#endif
                break;
            }

            default:
            {/* Connection in invalid state - should not occur */
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                if (cmPrim->localPsm == CSR_BT_AVCTP_BROWSING_PSM &&
                    cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM &&
                    cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                {/* A browsing channel is established before control channel - disconnect */
                    CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_DATA_CHANNEL, cmPrim->deviceAddr,cmPrim->btConnId);
                    CsrBtCml2caDisconnectReqSend(cmPrim->btConnId);
                }
#endif
                break;
            }
        }
    }
    else
    {/* Unknown connection */
        if (cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM &&
            cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
        {/* Make sure the unknown connection is disconnected */
            CsrBtCml2caDisconnectReqSend(cmPrim->btConnId);
        }
        else
        {
            /* Outgoing connection attempt has failed/canceled but no connection instance exists.
             * It means connection was established by remote and got disconnected,
             * while we were also trying to make connection.
             * Let's move the app state to IDLE now if BUSY.
             */
            if (instData->appState == AVRCP_STATE_APP_BUSY)
            {
                CsrBtAvrcpUtilGo2Idle(instData);
            }
        }
    }
}

#ifdef CSR_STREAMS_ENABLE
static void avrcpDisposeStreamData(CsrBtConnId btConnId)
{
    Sink sink = StreamL2capSink(CM_GET_UINT16ID_FROM_BTCONN_ID(btConnId));

    if (SinkIsValid(sink))
    {
        /* Auto discard any data in the source. */
        StreamConnectDispose(StreamSourceFromSink(sink));
    }
}
#endif /* CSR_STREAMS_ENABLE */

/* Connection disconnected */
void CsrBtAvrcpCmL2caDisconnectIndHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpConnDetails       *connDetails;
    CsrBtCmL2caDisconnectInd    *cmPrim = (CsrBtCmL2caDisconnectInd *) instData->recvMsgP;
    AvrcpConnInstance_t         *connInst = CsrBtAvrcpUtilGetConnFromL2caCid(instData, cmPrim->btConnId, &connDetails);

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    if (!cmPrim->localTerminated)
    {
        /* For remote disconnections, profile needs to respond to L2CA_DISCONNECT_IND. */
        CsrBtCmL2caDisconnectRspSend(cmPrim->l2caSignalId, cmPrim->btConnId);
    }
#endif

    if (connInst)
    {
        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL, connInst->address,cmPrim->btConnId);

#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
        if (connDetails == &connInst->browsing)
        {
            CsrBtAvrcpImagingFlushPendingMsgs(instData, connInst, FALSE);
        }
        else
#endif
        {
            CsrBtAvrcpImagingFlushPendingMsgs(instData, connInst, TRUE);
        }

        if ((connDetails == &connInst->control) && 
            (connInst->ctLocal->ctObexState != AVRCP_CT_OBEX_DISCONNECTED))
        {
            CsrBtAvrcpImagingClientDisconnectReqSend(connInst->appConnId, FALSE);
        }
#endif /* CSR_BT_INSTALL_AVRCP_CT_COVER_ART */

        if (((connDetails == &connInst->control) && (connInst->control.state == AVRCP_STATE_CONN_DISC2RECONNECT)) 
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
            || ((connDetails == &connInst->browsing) && (connInst->browsing.state == AVRCP_STATE_CONN_DISC2RECONNECT))
#endif
           )
        {
            CsrBtAvrcpUtilConnect(connInst);
        }
        else
        {
            AVRCP_CHANGE_STATE_INDEX(connDetails->state, AVRCP_STATE_CONN_DISCONNECTED, connInst->appConnId);

#ifdef CSR_BT_RESTRICT_MAX_PROFILE_CONNECTIONS
            if (connDetails == &connInst->control)
            {
                avrcpUpdateActiveConnRecord(instData, FALSE /* Disconnected */);
            }
#endif

#ifdef CSR_STREAMS_ENABLE
            avrcpDisposeStreamData(cmPrim->btConnId);
#endif /* CSR_STREAMS_ENABLE */

            if (connInst->control.state == AVRCP_STATE_CONN_DISCONNECTED)
            {/* Control is disconnected - cleanup */
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                if (connInst->browsing.state == AVRCP_STATE_CONN_DISCONNECTED)
                {
#endif
                    if (cmPrim->localTerminated)
                    {
                        /* Locally initiated disconnection */
                        csrBtAvrcpCompletePendingOp(instData,
                                                    connInst,
                                                    cmPrim->reasonCode,
                                                    cmPrim->reasonSupplier,
                                                    cmPrim->localTerminated);
                        connInst->resetAppState = FALSE;
                    }
                    else
                    {
                        /* Remote initiated disconnection */ 
                        /* In multi-point scenarios, 'pendingCtrlPrim' may hold downstream request for different remote device.
                         * No need to bother about the ongoing operation of other device, 
                         * Send disconnect indication to the application.
                         */  
                        CsrBtAvrcpDisconnectIndSend(instData->ctrlHandle,
                                                    connInst->appConnId,
                                                    cmPrim->reasonCode,
                                                    cmPrim->reasonSupplier,
                                                    cmPrim->localTerminated);

                        if (connInst->resetAppState)
                        {
                            /* In cross over scenario where,
                             * remote initiated disconnection indication is received,
                             * when outgoing disconnection is pending for the same device,
                             * go to idle state.
                             */
                            CsrBtAvrcpUtilGo2Idle(instData);
                            connInst->resetAppState = FALSE;
                        }
                    }
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                }
                else
                {/* Browsing is not disconnected - always disconnect if control is disconnected */
                    CsrBtAvrcpUtilDisconnect(connInst);
                    /* A new disconnect indication will arrive when the browsing channel is disconnected: wait for that and do nothing until then....*/
                }
#endif
                if (connInst->sdpState == AVRCP_STATE_SDP_ACTIVE)
                {/* SDP search is in progress and should be cancelled */
                    CsrBtAvrcpSdpSearchCancel(connInst);
                }

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                if ((connInst->control.state == AVRCP_STATE_CONN_DISCONNECTED) &&
                    (connInst->browsing.state == AVRCP_STATE_CONN_DISCONNECTED))
#else
                if (connInst->control.state == AVRCP_STATE_CONN_DISCONNECTED)
#endif
                {
#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
                    if ((connInst->tgLocal->obexState != AVRCP_TG_OBEX_SERVER_IDLE) &&
                        (connInst->tgLocal->obexState != AVRCP_TG_OBEX_SERVER_DEACTIVATING))
                    {
                        CsrBtAvrcpImagingServerDeactivateReqSend(instData->tgLocal.obexPsm, connInst->appConnId);
                    }
#endif
                    CsrBtAvrcpUtilConnRemove((CsrCmnListElm_t *)connInst, NULL);
                    AVRCP_LIST_CONN_REMOVE((CsrCmnList_t *)&instData->connList, connInst);
                }

                CsrBtAvrcpUtilConnectAccept(instData);
            }
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
            else
            {
                /* do nothing if in the process of disconnecting or if already having a pending
                   accept connect on the browse channel */
                if( (connInst->control.state != AVRCP_STATE_CONN_DISCONNECTING) 
                    && (instData->activateStateBrow == AVRCP_STATE_ACT_DEACTIVATED))
                {
                    CsrBool tgSupport = FALSE;
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

#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
                    if (instData->tgLocal.srHandle)
                    {
                        tgSupport = TRUE;
                    }
#endif 

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
            }
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
        }
    }
}

void CsrBtAvrcpCmL2caDataIndHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpConnDetails  *connDetails;
    CsrBtCmL2caDataInd  *cmPrim = (CsrBtCmL2caDataInd *) instData->recvMsgP;
    AvrcpConnInstance_t *connInst = CsrBtAvrcpUtilGetConnFromL2caCid(instData, cmPrim->btConnId, &connDetails);

#ifndef CSR_STREAMS_ENABLE
    CsrBtCmL2caDataResSend(cmPrim->btConnId);
#endif

    if (connInst && (connDetails->state == AVRCP_STATE_CONN_CONNECTED))
    {/* Connection is valid */
        if (CsrBtAvrcpUtilDataCheckAvctp(cmPrim->length, cmPrim->payload))
        {/* AVCTP header is valid */
            if (
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                        (connDetails == &connInst->browsing) ||
#endif
                        CsrBtAvrcpUtilDataFragRxHandle(connInst, &cmPrim->length, &cmPrim->payload))
            {/* AVCTP packet is not fragmented or a complete frame has been assembled or the data comes through the browsing channel and does not need fragmenting */
                switch (AVRCP_DATA_AVCTP_CR_GET(cmPrim->payload))
                {
#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
                    case AVRCP_DATA_AVCTP_CR_RES:
                    {/* Response */
                        if (connDetails == &connInst->control)
                        {/* AV/C response on control channel */
                            CsrBtAvrcpCtRxControlHandler(connInst, cmPrim->length, &cmPrim->payload);
                        }
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                        else if (connDetails == &connInst->browsing)
                        {/* Response on browsing channel */
                            CsrBtAvrcpCtRxBrowsingHandler(connInst, cmPrim->length, &cmPrim->payload);
                        }
#endif
                        break;
                    }
#endif
#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
                    case AVRCP_DATA_AVCTP_CR_CMD:
                    {/* Command */
                        if (connDetails == &connInst->control)
                        {/* AV/C command on control channel */
                            CsrBtAvrcpTgRxControlHandler(connInst, cmPrim->length, &cmPrim->payload);
                        }
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                        else if (connDetails == &connInst->browsing)
                        {/* Command on browsing channel */
                            CsrBtAvrcpTgRxBrowsingHandler(connInst, cmPrim->length, &cmPrim->payload);
                        }
#endif                        
                        break;
                    }
#endif

                    default:
                    {/* Can not occur - C/R field is only one bit */
                        break;
                    }
                }
            }
        }
        else
        {/* AVCTP packet is invalid */
            CsrUint8 *txData;
            CsrUint8 tLabel;

            /* As per AVCTP spec V1.4 section 7.2, 
               The PID is required to send the response with IPID, PID is 
               present only in the Single or Start of the AVCTP packet. So 
               check if PID is present, if not ignore the received packet */
            if( cmPrim->length < AVRCP_DATA_AVCTP_HEADER_SIZE_SINGLE)
            {
                CsrPmemFree(cmPrim->payload);
                return;
            }
            
            /* Send response with invalid PID */
            txData = CsrPmemAlloc(AVRCP_DATA_AVCTP_SINGLE_MI_INDEX);
            tLabel  = AVRCP_TLABEL_GET(cmPrim->payload);
            /* Invalid PID + command/response info */
            txData[0]  = AVRCP_DATA_AVCTP_IPID_MASK | ((AVRCP_DATA_AVCTP_CR_RES & AVRCP_DATA_AVCTP_CR_MASK) << AVRCP_DATA_AVCTP_CR_SHIFT);
            /* Packet type */
            txData[0] |= (AVRCP_DATA_AVCTP_PACKET_TYPE_SINGLE & AVRCP_DATA_AVCTP_PACKET_TYPE_MASK) << AVRCP_DATA_AVCTP_PACKET_TYPE_SHIFT;
            /* Transaction label as received from remote */
            txData[0] |= (tLabel & AVRCP_DATA_AVCTP_TLABEL_MASK) << AVRCP_DATA_AVCTP_TLABEL_SHIFT;
            /* Incorrect profile id (UUID) as received from remote */
            SynMemCpyS(&txData[AVRCP_DATA_AVCTP_SINGLE_PID_INDEX], sizeof(CsrUint16), &cmPrim->payload[AVRCP_DATA_AVCTP_SINGLE_PID_INDEX], sizeof(CsrUint16));

            CsrBtAvrcpControlDataSend(connInst, AVRCP_DATA_AVCTP_SINGLE_MI_INDEX, txData);
        }
    }

    CsrPmemFree(cmPrim->payload);

}

void CsrBtAvrcpCmL2caDataCfmHandler(AvrcpInstanceData_t *instData)
{
    CsrBtCmL2caDataCfm    *cmPrim = (CsrBtCmL2caDataCfm *) instData->recvMsgP;
    
    
    if (cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {/* Data request successful; check if there are any pending messages to send */
        CsrBtAvrcpConnDetails    *connDetails;
        AvrcpConnInstance_t   *connInst = CsrBtAvrcpUtilGetConnFromL2caCid(instData, cmPrim->btConnId, &connDetails);

        if (connInst)
        {/* Device is known and connected */
            CsrBtAvrcpUtilPendingDataSend(connInst, cmPrim->btConnId);
        }
    }
    /* else what to do???? */
}

#endif

