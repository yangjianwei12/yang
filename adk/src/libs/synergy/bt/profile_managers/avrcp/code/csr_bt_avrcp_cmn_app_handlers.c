/******************************************************************************
 Copyright (c) 2009-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_AVRCP_MODULE

#include "csr_sched.h"
#include "csr_bt_result.h"
#include "csr_bt_util.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_avrcp_main.h"
#include "csr_bt_avrcp_prim.h"
#include "csr_bt_avrcp_lib.h"
#include "sds_prim.h"
#ifdef CSR_BT_INSTALL_AVRCP_COVER_ART
#include "csr_bt_avrcp_imaging_private_prim.h"
#include "csr_bt_avrcp_imaging_private_lib.h"
#endif
void CsrBtAvrcpMessagePut(CsrSchedQid phandle, void *msg)
{
    CsrSchedMessagePut(phandle, CSR_BT_AVRCP_PRIM, msg);
}

void CsrBtAvrcpConfiqReqIdleState(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpConfigReq *prim = (CsrBtAvrcpConfigReq *) instData->recvMsgP;
    CsrBool UnregisterNotSent = TRUE;
    CsrBool registerAgain = FALSE;

    instData->ctrlHandle            = prim->phandle;
    instData->globalConfig          = prim->globalConfig;
    instData->mtu                   = prim->mtu;
    instData->srAvrcpVersionHighest = CSR_BT_AVRCP_CONFIG_SR_VERSION_10;

    CsrBtAvrcpUtilGo2Busy(instData, CSR_BT_AVRCP_CONFIG_REQ);

    instData->srPending = prim;
    instData->recvMsgP = NULL;

    if (prim->tgDetails.roleSupported || prim->ctDetails.roleSupported)
    {/* New record(s) should be registered - save the pending data */
#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
        if (prim->tgDetails.roleSupported)
        {
            instData->tgLocal.configuration  = prim->tgDetails.roleConfig;
            instData->tgLocal.srFeatures     = prim->tgDetails.srFeatures;
            instData->tgLocal.srAvrcpVersion = prim->tgDetails.srAvrcpVersion;
            instData->tgLocal.uidCount       = prim->uidCount;

#ifdef CSR_BT_INSTALL_AVRCP_TG_13_AND_HIGHER
            if (prim->tgDetails.srAvrcpVersion > instData->srAvrcpVersionHighest)
            {
                instData->srAvrcpVersionHighest = prim->tgDetails.srAvrcpVersion;
            }
#endif
        }
#endif

#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
        if (prim->ctDetails.roleSupported)
        {
            instData->ctLocal.configuration  = prim->ctDetails.roleConfig;
            instData->ctLocal.srFeatures     = prim->ctDetails.srFeatures;
            instData->ctLocal.srAvrcpVersion = prim->ctDetails.srAvrcpVersion;

#ifdef CSR_BT_INSTALL_AVRCP_CT_13_AND_HIGHER
            if (prim->ctDetails.srAvrcpVersion > instData->srAvrcpVersionHighest)
            {
                instData->srAvrcpVersionHighest = prim->ctDetails.srAvrcpVersion;
            }
#endif
        }
#endif
    }

#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
    /* Make sure all existing records are removed */
    if (instData->tgLocal.srHandle != AVRCP_SDP_INVALID_SR_HANDLE)
    {
        UnregisterNotSent = FALSE;
        CsrBtCmSdsUnRegisterReqSend(CSR_BT_AVRCP_IFACEQUEUE, instData->tgLocal.srHandle, CSR_BT_CM_CONTEXT_UNUSED);
    }
#endif

#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
    if (instData->ctLocal.srHandle != AVRCP_SDP_INVALID_SR_HANDLE)
    {
        UnregisterNotSent = FALSE;
        CsrBtCmSdsUnRegisterReqSend(CSR_BT_AVRCP_IFACEQUEUE, instData->ctLocal.srHandle, CSR_BT_CM_CONTEXT_UNUSED);
    }
#endif
    if ((instData->srPending->tgDetails.roleSupported) || (instData->srPending->ctDetails.roleSupported))
    {
        registerAgain = TRUE;
    }

    if (UnregisterNotSent)
    {
        if (!registerAgain)
        {/* No records should be registered and none is registered in advance */
            CsrBtAvrcpUtilFreeConfigReq(&instData->srPending);
            CsrBtAvrcpConfigCfmSend(instData->ctrlHandle,
                CSR_BT_RESULT_CODE_AVRCP_INVALID_PARAMETER,
                CSR_BT_SUPPLIER_AVRCP);
            CsrBtAvrcpUtilGo2Idle(instData);
        }
        else
        {/* Start registration only if no records are unregistered, as it means this is the first registration.
            If records have been unregistered, wait for the unregister_cfm message to start registration */
            CsrBtAvrcpSdpRegisterSR(instData);
        }
    }
}

/***** Activation handling *****/
void CsrBtAvrcpActivateReqIdleState(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpActivateReq *prim = (CsrBtAvrcpActivateReq *)instData->recvMsgP;
    CsrBool ctHandleExists = FALSE;
    CsrBool tgHandleExists = FALSE;

#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
    if (AVRCP_SDP_INVALID_SR_HANDLE != instData->ctLocal.srHandle)
    {
        ctHandleExists = TRUE;
    }
#endif
#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
    if (AVRCP_SDP_INVALID_SR_HANDLE != instData->tgLocal.srHandle)
    {
        tgHandleExists = TRUE;
    }
#endif

    if (ctHandleExists || tgHandleExists)
    {
        if (prim->maxIncoming > 0)
        {
#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
            if (tgHandleExists && (!instData->tgLocal.mpDefault))
            {/* There must be at least one media player registered before activating */
                CsrBtAvrcpActivateCfmSend(instData->ctrlHandle,
                            CSR_BT_RESULT_CODE_AVRCP_COMMAND_DISALLOWED,
                            CSR_BT_SUPPLIER_AVRCP);
            }
            else
#endif
            {
                instData->incomingMaximum = prim->maxIncoming;

                CsrBtAvrcpUtilConnectAccept(instData);
                CsrBtAvrcpActivateCfmSend(instData->ctrlHandle,
                    CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
                    CSR_BT_SUPPLIER_AVRCP);
            }
        }
        else
        {
            CsrBtAvrcpActivateCfmSend(instData->ctrlHandle,
                CSR_BT_RESULT_CODE_AVRCP_INVALID_PARAMETER,
                CSR_BT_SUPPLIER_AVRCP);
        }
    }
    else
    {
        CsrBtAvrcpActivateCfmSend(instData->ctrlHandle,
            CSR_BT_RESULT_CODE_AVRCP_COMMAND_DISALLOWED,
            CSR_BT_SUPPLIER_AVRCP);
    }

    CsrBtAvrcpUtilGo2Idle(instData);
}

/***** Deactivation handling *****/
#ifdef INSTALL_AVRCP_DEACTIVATE
void CsrBtAvrcpDeactivateReqIdleState(AvrcpInstanceData_t *instData)
{
    /* though never activated, the browsing channel may have a pending accept connect due to
       remote device having disconnected browse channel and local device allowing re-connect */
    if ((instData->incomingMaximum > 0) 
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
        || (instData->activateStateBrow == AVRCP_STATE_ACT_ACTIVATED)
#endif
       )
    {
        instData->incomingMaximum = 0;

        if (CsrBtAvrcpUtilConnectAcceptCancel(instData))
        {/* A CM_L2CA_CANCEL_CONNECT_ACCEPT was sent */
            CsrBtAvrcpUtilGo2Busy(instData, CSR_BT_AVRCP_DEACTIVATE_REQ);
        }
        else
        {/* Profile is already deactivated */
            CsrBtAvrcpDeactivateCfmSend(instData->ctrlHandle, CSR_BT_RESULT_CODE_AVRCP_SUCCESS, CSR_BT_SUPPLIER_AVRCP);
            CsrBtAvrcpUtilGo2Idle(instData);
        }
    }
    else
    {
        CsrBtAvrcpDeactivateCfmSend(instData->ctrlHandle, CSR_BT_RESULT_CODE_AVRCP_NOTHING_TO_CANCEL, CSR_BT_SUPPLIER_AVRCP);
        CsrBtAvrcpUtilGo2Idle(instData);
    }
}
#endif /* INSTALL_AVRCP_DEACTIVATE */

/***** Connect handling *****/
void CsrBtAvrcpConnectReqIdleState(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpConnectReq *prim = (CsrBtAvrcpConnectReq *) instData->recvMsgP;
    CsrBool ctHandleExists = FALSE;
    CsrBool tgHandleExists = FALSE;

#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
    if (AVRCP_SDP_INVALID_SR_HANDLE != instData->ctLocal.srHandle)
    {
        ctHandleExists = TRUE;
    }
#endif
#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
    if (AVRCP_SDP_INVALID_SR_HANDLE != instData->tgLocal.srHandle)
    {
        tgHandleExists = TRUE;
    }
#endif

#ifdef CSR_BT_RESTRICT_MAX_PROFILE_CONNECTIONS
    /* Max allowed connections is incomingMaximum */
    if (instData->numActiveAvrcpConns < instData->incomingMaximum &&
        (tgHandleExists || ctHandleExists))
#else
    if (tgHandleExists || ctHandleExists)
#endif
    {/* Connection is allowed */
        if (AVRCP_LIST_CONN_GET_ADDR((CsrCmnList_t *)&instData->connList, &prim->deviceAddr))
        {/* A connection is already present */
            CsrBtAvrcpConnectCfmSend(instData->ctrlHandle, &prim->deviceAddr,
                                     0, CSR_BT_AVRCP_CONNECTION_ID_INVALID, NULL, NULL,
                                     CSR_BT_RESULT_CODE_AVRCP_ALREADY_CONNECTED,
                                     CSR_BT_SUPPLIER_AVRCP,
                                     CSR_BT_CONN_ID_INVALID);
        }
        else
        {/* Establish a new connection */
            if (AVRCP_STATE_SDP_IDLE == instData->sdpState)
            {
                AvrcpConnInstance_t *connInst = CsrBtAvrcpUtilConnAdd(instData, &prim->deviceAddr);
                CsrBtAvrcpSdpSearchStart(instData, connInst);
                AVRCP_CHANGE_STATE_INDEX(connInst->control.state, AVRCP_STATE_CONN_PENDING, connInst->appConnId);
                CsrBtAvrcpUtilGo2Busy(instData, CSR_BT_AVRCP_CONNECT_REQ);
            }
            else
            {
                /* Waiting for SDP to cancel/complete. */
                CsrBtAvrcpUtilSaveMessage(instData);
            }
        }
    }
    else
    {/* No service records have been registered so connection establishment is not allowed */
        CsrBtAvrcpConnectCfmSend(instData->ctrlHandle, &prim->deviceAddr,
                                 AVRCP_MTU_INVALID, CSR_BT_AVRCP_CONNECTION_ID_INVALID, NULL, NULL,
                                 CSR_BT_RESULT_CODE_AVRCP_COMMAND_DISALLOWED,
                                 CSR_BT_SUPPLIER_AVRCP,
                                 CSR_BT_CONN_ID_INVALID);
        CsrBtAvrcpUtilGo2Idle(instData);
    }
}

/***** Cancel connect handling *****/
#ifdef INSTALL_AVRCP_CANCEL_CONNECT
void CsrBtAvrcpCancelConnectReqIdleBusyState(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCancelConnectReq *prim = (CsrBtAvrcpCancelConnectReq *) instData->recvMsgP;
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_ADDR((CsrCmnList_t *)&instData->connList, &prim->deviceAddr);

    if (connInst)
    {
        /* The connection is known */

#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
        if (connInst->ctLocal->ctObexState != AVRCP_CT_OBEX_DISCONNECTED)
        {
            /* Stop the ongoing obex connection */
            CsrBtAvrcpImagingClientCancelConnectReqSend(connInst->address);
        }
#endif
        CsrBtAvrcpUtilDisconnect(connInst);
        if (instData->appState == AVRCP_STATE_APP_IDLE)
        {/* The application has received notification about this; the answer shall be CSR_BT_AVRCP_DISCONNECT_IND*/
            instData->pendingCtrlPrim = CSR_BT_AVRCP_DISCONNECT_REQ;
        }
        else
        {
            instData->pendingCtrlPrim = CSR_BT_AVRCP_CANCEL_CONNECT_REQ;
        }
    }
    else
    {/* Device address is unknown - check if a connection request is present in the save queue */
        if (CsrBtAvrcpUtilCancelSavedMessage(instData, CSR_BT_AVRCP_CONNECT_REQ, &prim->deviceAddr))
        {/* A connection request to the same device was removed from the save queue */
            CsrBtAvrcpConnectCfmSend(instData->ctrlHandle,
                                     &prim->deviceAddr,
                                     AVRCP_MTU_INVALID,
                                     CSR_BT_AVRCP_CONNECTION_ID_INVALID,
                                     NULL,
                                     NULL,
                                     CSR_BT_RESULT_CODE_AVRCP_CONNECT_ATTEMPT_CANCELLED,
                                     CSR_BT_SUPPLIER_AVRCP,
                                     CSR_BT_CONN_ID_INVALID);
        }
        else
        {/* The device might have been disconnected from remote side */
            CsrBtAvrcpConnectCfmSend(instData->ctrlHandle,
                                     &prim->deviceAddr,
                                     AVRCP_MTU_INVALID,
                                     CSR_BT_AVRCP_CONNECTION_ID_INVALID,
                                     NULL,
                                     NULL,
                                     CSR_BT_RESULT_CODE_AVRCP_NOTHING_TO_CANCEL,
                                     CSR_BT_SUPPLIER_AVRCP,
                                     CSR_BT_CONN_ID_INVALID);
        }
    }
}
#endif /* INSTALL_AVRCP_CANCEL_CONNECT */

/***** Disconnect handling *****/
void CsrBtAvrcpDisconnectReqIdleBusyState(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpDisconnectReq *prim = (CsrBtAvrcpDisconnectReq *) instData->recvMsgP;
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, prim->connectionId);

    /* Determine if a connection already is established to the device */
    if (connInst)
    {/* Connection is known */
        if (connInst->control.state == AVRCP_STATE_CONN_CONNECTED)
        {
#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
            if (connInst->ctLocal->ctObexState != AVRCP_CT_OBEX_DISCONNECTED)
            {
                /* Remove OBEX service and transport connection */
                CsrBtAvrcpImagingClientDisconnectReqSend(connInst->appConnId, FALSE);
            }
#endif
            CsrBtAvrcpUtilDisconnect(connInst);
            CsrBtAvrcpUtilGo2Busy(instData, CSR_BT_AVRCP_DISCONNECT_REQ);
        }
        else
        {/* This should not happen - app does not have the correct connId before a connection has been established */
        }
    }
    else
    {/* ConnectionID is unknown - just ignore */
    }
}

#ifdef INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS
/***** Security handling *****/
void CsrBtAvrcpSecurityInReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpSecurityInReq *prim = (CsrBtAvrcpSecurityInReq*)instData->recvMsgP;
    CsrBtResultCode rval = CSR_BT_RESULT_CODE_AVRCP_UNACCEPTABLE_PARAMETER;

    if (CSR_MASK_IS_SET(prim->config, CSR_BT_AVRCP_SECURITY_CONFIG_CONTROL))
    {
        rval = CsrBtScSetSecInLevel(&instData->secIncomingCont, prim->secLevel,
            CSR_BT_AV_RCP_MANDATORY_SECURITY_INCOMING,
            CSR_BT_AV_RCP_DEFAULT_SECURITY_INCOMING,
            CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
            CSR_BT_RESULT_CODE_AVRCP_UNACCEPTABLE_PARAMETER);
    }

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    if (CSR_MASK_IS_SET(prim->config, CSR_BT_AVRCP_SECURITY_CONFIG_BROWSING))
    {
        rval = CsrBtScSetSecInLevel(&instData->secIncomingBrow, prim->secLevel,
            CSR_BT_AV_RCP_MANDATORY_SECURITY_INCOMING,
            CSR_BT_AV_RCP_DEFAULT_SECURITY_INCOMING,
            CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
            CSR_BT_RESULT_CODE_AVRCP_UNACCEPTABLE_PARAMETER);
    }
#endif
    CsrBtAvrcpSecurityInCfmSend(prim->phandle, rval, CSR_BT_SUPPLIER_AVRCP);
}

void CsrBtAvrcpSecurityOutReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpSecurityOutReq *prim = (CsrBtAvrcpSecurityOutReq*)instData->recvMsgP;
    CsrBtResultCode rval = CSR_BT_RESULT_CODE_AVRCP_UNACCEPTABLE_PARAMETER ;

    if (CSR_MASK_IS_SET(prim->config, CSR_BT_AVRCP_SECURITY_CONFIG_CONTROL))
    {
        rval = CsrBtScSetSecOutLevel(&instData->secOutgoingCont, prim->secLevel,
            CSR_BT_AV_RCP_MANDATORY_SECURITY_OUTGOING,
            CSR_BT_AV_RCP_DEFAULT_SECURITY_OUTGOING,
            CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
            CSR_BT_RESULT_CODE_AVRCP_UNACCEPTABLE_PARAMETER);
    }

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    if (CSR_MASK_IS_SET(prim->config, CSR_BT_AVRCP_SECURITY_CONFIG_BROWSING))
    {
        rval = CsrBtScSetSecOutLevel(&instData->secOutgoingBrow, prim->secLevel,
            CSR_BT_AV_RCP_MANDATORY_SECURITY_OUTGOING,
            CSR_BT_AV_RCP_DEFAULT_SECURITY_OUTGOING,
            CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
            CSR_BT_RESULT_CODE_AVRCP_UNACCEPTABLE_PARAMETER);
     }
#endif
    CsrBtAvrcpSecurityOutCfmSend(prim->phandle, rval, CSR_BT_SUPPLIER_AVRCP);
}
#endif /* INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS */

void CsrBtAvrcpConfigCfmSend(CsrSchedQid ctrlHandle, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier)
{
    CsrBtAvrcpConfigCfm *prim = (CsrBtAvrcpConfigCfm *) CsrPmemZalloc(sizeof(*prim));

    prim->type      = CSR_BT_AVRCP_CONFIG_CFM;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;

    CsrBtAvrcpMessagePut(ctrlHandle, prim);
}

void CsrBtAvrcpActivateCfmSend(CsrSchedQid ctrlHandle, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier)
{
    CsrBtAvrcpActivateCfm *prim = (CsrBtAvrcpActivateCfm *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_ACTIVATE_CFM;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;

    CsrBtAvrcpMessagePut(ctrlHandle, prim);
}

#ifdef INSTALL_AVRCP_DEACTIVATE
void CsrBtAvrcpDeactivateCfmSend(CsrSchedQid ctrlHandle, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier)
{
    CsrBtAvrcpDeactivateCfm *prim = (CsrBtAvrcpDeactivateCfm *) CsrPmemZalloc(sizeof(*prim));

    prim->type              = CSR_BT_AVRCP_DEACTIVATE_CFM;
    prim->resultCode        = resultCode;
    prim->resultSupplier    = resultSupplier;

    CsrBtAvrcpMessagePut(ctrlHandle, prim);
}
#endif /* INSTALL_AVRCP_DEACTIVATE */

void CsrBtAvrcpConnectIndSend(CsrSchedQid ctrlHandle,
                              CsrBtDeviceAddr *addr,
                              CsrUint16 frameSize,
                              CsrUint8 connId,
                              CsrBtConnId btConnId)
{
    CsrBtAvrcpConnectInd *prim = (CsrBtAvrcpConnectInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type                  = CSR_BT_AVRCP_CONNECT_IND;
    prim->connectionId          = connId;
    prim->deviceAddr            = *addr;
    prim->btConnId              = btConnId;

    CsrBtAvrcpMessagePut(ctrlHandle, prim);
    CSR_UNUSED(frameSize);
}

void CsrBtAvrcpConnectCfmSend(CsrSchedQid ctrlHandle,
                              CsrBtDeviceAddr *addr,
                              CsrUint16 frameSize,
                              CsrUint8 connId,
                              CsrBtAvrcpRoleDetails *tgDetails,
                              CsrBtAvrcpRoleDetails *ctDetails,
                              CsrBtResultCode resultCode,
                              CsrBtSupplier resultSupplier,
                              CsrBtConnId btConnId)
{
    CsrBtAvrcpConnectCfm *prim = (CsrBtAvrcpConnectCfm *) CsrPmemZalloc(sizeof(*prim));

    prim->type              = CSR_BT_AVRCP_CONNECT_CFM;
    prim->resultCode        = resultCode;
    prim->resultSupplier    = resultSupplier;
    prim->connectionId      = connId;
    prim->deviceAddr        = *addr;
    prim->btConnId          = btConnId;

    if (tgDetails)
    {
        prim->tgFeatures = *tgDetails;
        prim->tgFeatures.providerName = CsrStrDup(tgDetails->providerName);
        prim->tgFeatures.serviceName = CsrStrDup(tgDetails->serviceName);
    }
    else
    {
        CsrBtAvrcpConfigRoleNoSupport(&prim->tgFeatures);
    }

    if (ctDetails)
    {
        prim->ctFeatures = *ctDetails;
        prim->ctFeatures.providerName = CsrStrDup(ctDetails->providerName);
        prim->ctFeatures.serviceName = CsrStrDup(ctDetails->serviceName);
    }
    else
    {
        CsrBtAvrcpConfigRoleNoSupport(&prim->ctFeatures);
    }

    CsrBtAvrcpMessagePut(ctrlHandle, prim);

    CSR_UNUSED(frameSize);
}

void CsrBtAvrcpRemoteFeaturesIndSend(CsrSchedQid phandle, CsrUint8 connId, CsrBtDeviceAddr addr, CsrBtAvrcpRoleDetails *tgFeatures, CsrBtAvrcpRoleDetails *ctFeatures)
{
    CsrBtAvrcpRemoteFeaturesInd *prim = (CsrBtAvrcpRemoteFeaturesInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_REMOTE_FEATURES_IND;
    prim->connectionId  = connId;
    prim->deviceAddr    = addr;
    prim->tgFeatures    = *tgFeatures;
    prim->ctFeatures    = *ctFeatures;
    CsrBtAvrcpMessagePut(phandle, prim);
}

void CsrBtAvrcpDisconnectIndSend(CsrSchedQid ctrlHandle, CsrUint8 connId,
    CsrBtReasonCode reasonCode, CsrBtSupplier reasonSupplier,
    CsrBool localTerminated)
{
    CsrBtAvrcpDisconnectInd *prim = (CsrBtAvrcpDisconnectInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type         = CSR_BT_AVRCP_DISCONNECT_IND;
    prim->connectionId = connId;
    prim->localTerminated = localTerminated;
    prim->reasonCode      = reasonCode;
    prim->reasonSupplier  = reasonSupplier;

    CsrBtAvrcpMessagePut(ctrlHandle, prim);
}

#ifdef INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS
void CsrBtAvrcpSecurityInCfmSend(CsrSchedQid ctrlHandle, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtAvrcpSecurityInCfm *prim = (CsrBtAvrcpSecurityInCfm *) CsrPmemZalloc(sizeof(*prim));

    prim->type           = CSR_BT_AVRCP_SECURITY_IN_CFM;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;
    CsrBtAvrcpMessagePut(ctrlHandle, prim);
}

void CsrBtAvrcpSecurityOutCfmSend(CsrSchedQid ctrlHandle, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtAvrcpSecurityOutCfm *prim = (CsrBtAvrcpSecurityOutCfm *) CsrPmemZalloc(sizeof(*prim));

    prim->type      = CSR_BT_AVRCP_SECURITY_OUT_CFM;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;
    CsrBtAvrcpMessagePut(ctrlHandle, prim);
}
#endif /*INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS */
#endif /* !EXCLUDE_CSR_BT_AVRCP_MODULE */

