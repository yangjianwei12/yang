/******************************************************************************
 Copyright (c) 2010-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_RFC_MODULE

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_sdc.h"
#include "csr_bt_cm_util.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif
#include "csr_bt_cm_events_handler.h"
#include "csr_bt_cm_l2cap.h"

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_cm_streams_handler.h"
#endif

#define CSR_BT_MAX_TIME     (2000000000)

void CsrBtCmRfcCommonErrorHandler(cmInstanceData_t *cmData, cmRfcConnInstType * theLogicalLink)
{
    CSR_UNUSED(theLogicalLink);

    /* Notify disconnection event to device utility, in order to run automatic procedures (if any).*/
    (void)CmDuHandleAutomaticProcedure(cmData,
                                       CM_DU_AUTO_EVENT_SERVICE_DISCONNECTED,
                                       NULL,
                                       &theLogicalLink->deviceAddr);

    /* Inform RFC connection confirmation with the failure reason to the caller.*/
    CsrBtCmConnectCfmMsgSend(cmData, cmData->smVar.arg.result.code, cmData->smVar.arg.result.supplier);
    CsrBtCmServiceManagerLocalQueueHandler(cmData);
}

static cmRfcConnElement * returnCancelConnectIndex(cmInstanceData_t *cmData, CsrUint8 server, CsrSchedQid phandle, CsrBtDeviceAddr deviceAddr, CsrBtCmPrim type)
{
    if (cmData->smVar.smInProgress && cmData->smVar.smMsgTypeInProgress == type)
    {
        cmRfcConnElement *currentElem;

        for (currentElem = CM_RFC_GET_FIRST(cmData->rfcVar.connList); currentElem; currentElem = currentElem->next)
        {
            if (currentElem->cmRfcConnInst)
            {
                if (server == currentElem->cmRfcConnInst->serverChannel)
                {
                    if (phandle == currentElem->cmRfcConnInst->appHandle)
                    {
                        if(CsrBtBdAddrEq(&(deviceAddr), &(currentElem->cmRfcConnInst->deviceAddr)))
                        {
                            if (currentElem->cmRfcConnInst->state == CSR_BT_CM_RFC_STATE_CONNECT ||
                                currentElem->cmRfcConnInst->state == CSR_BT_CM_RFC_STATE_CONNECT_INIT ||
                                currentElem->cmRfcConnInst->state == CSR_BT_CM_RFC_STATE_ACCESS)
                            {
                                return currentElem;
                            }
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

void CsrBtCmRfcStartInitiateConnection(cmInstanceData_t *cmData, cmRfcConnInstType * theLogicalLink)
{
    CSR_UNUSED(cmData);

    /* Move to outgoing connection state.*/
    CSR_BT_CM_STATE_CHANGE(theLogicalLink->state, CSR_BT_CM_RFC_STATE_CONNECT);

    RfcClientConnectReqSend(CSR_BT_CM_IFACEQUEUE,
                            &theLogicalLink->deviceAddr,
                            theLogicalLink->remoteServerChan,
                            CSR_BT_RFCOMM_PREFERRED_MODE, /* from usr_config */
                            theLogicalLink->context,theLogicalLink->serverChannel,
                            theLogicalLink->profileMaxFrameSize,
                            0, /* priority */
                            CSR_BT_CM_INIT_CREDIT,
                            L2CA_AMP_CONTROLLER_BT,
                            L2CA_AMP_CONTROLLER_BT,
                            0, /* reserved length */
                            NULL, /* reserved */
                            theLogicalLink->modemStatus, 
                            theLogicalLink->signalBreak, 
                            theLogicalLink->mscTimeout); 
}

static CsrBool getSdcRemoteServerCh(cmInstanceData_t * cmData, CsrSchedQid appHandle, CsrBtUuid32 serviceHandle,
                                    CsrBtDeviceAddr deviceAddr, CsrUint8 * server)
{
    sdcResultItem resultItem;

    resultItem.appHandle = appHandle;
    resultItem.deviceAddr = deviceAddr;
    resultItem.serviceHandle = serviceHandle;
    resultItem.serverChannel = CSR_BT_NO_SERVER;

    if (CsrCmnListSearch((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList,
                         CsrBtCmSdcSearchListItemCompare,
                         &resultItem))
    {
        *server = resultItem.serverChannel;
        return TRUE;
    }
    else
    {
        *server = CSR_BT_NO_SERVER;
        return FALSE;
    }
}

static void csrBtCmConnectCfmMsgSend(CsrSchedQid appHandle, CsrBtDeviceAddr deviceAddr, CsrBtConnId btConnId,
                                     CsrUint16 profileMaxFrameSize, CsrBool validPortPar,
                                     RFC_PORTNEG_VALUES_T portPar, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier, CsrUint16 context)
{
    CsrBtCmConnectCfm * prim;

    prim                        = (CsrBtCmConnectCfm *)CsrPmemAlloc(sizeof(CsrBtCmConnectCfm));
    prim->type                  = CSR_BT_CM_CONNECT_CFM;
    prim->btConnId              = btConnId;
    prim->validPortPar          = validPortPar;
    prim->portPar               = portPar;
    prim->profileMaxFrameSize   = profileMaxFrameSize;
    prim->deviceAddr            = deviceAddr;
    prim->resultCode            = resultCode;
    prim->resultSupplier        = resultSupplier;
    prim->context               = context;
    CsrBtCmPutMessage(appHandle, prim);
}

static void csrBtCmCommonRfcConnectHandle(cmInstanceData_t *cmData,
                                          CsrSchedQid appHandle,
                                          CsrUint8 localServer,
                                          CsrUint8 remoteServer,
                                          CsrUint16 profileMaxFrameSize,
                                          CsrBtDeviceAddr devAddr,
                                          dm_security_level_t secLevel,
                                          CsrUint16 context,
                                          CsrUint8 modemStatus,
                                          CsrUint8 breakSignal,
                                          CsrUint8 mscTimeout,
                                          CsrUint8 minEncKeySize)
{
    if (remoteServer != CSR_BT_NO_SERVER && CsrBtCmElementCounterIncrement(cmData))
    {
        aclTable *aclConnectionElement;
        cmRfcConnElement  *theElement = (cmRfcConnElement *) CsrCmnListElementAddLast(&(cmData->rfcVar.connList), sizeof(cmRfcConnElement));
        cmRfcConnInstType *connInst   = theElement->cmRfcConnInst;
        CsrUint8 featIndex = LMP_FEATURES_SIMPLE_PAIRING_BIT / 8;
        CsrUint8 featOffsetBit = LMP_FEATURES_SIMPLE_PAIRING_BIT % 8;

        theElement->elementId         = cmData->elementCounter;
        cmData->rfcVar.activeElemId   = theElement->elementId;
        cmData->rfcVar.cancelConnect  = FALSE;

        CSR_BT_CM_STATE_CHANGE(cmData->rfcVar.connectState, CM_RFC_IDLE);
        CsrBtCmDmSmClearRebondData(cmData);

        connInst->appHandle           = appHandle;
        connInst->deviceAddr          = devAddr;
        connInst->serverChannel       = localServer;
        connInst->btConnId            = CSR_BT_CONN_ID_INVALID;
        connInst->remoteServerChan    = remoteServer;
        connInst->profileMaxFrameSize = CsrBtCmRfcDetermineMtu(cmData, devAddr, profileMaxFrameSize);
        connInst->context             = context;
        connInst->modemStatus         = modemStatus;
        connInst->signalBreak         = breakSignal;
        connInst->mscTimeout          = mscTimeout;
        dm_sm_service_register_outgoing_req(CSR_BT_CM_IFACEQUEUE,
                                    0, /* context */
                                    &devAddr,
                                    SEC_PROTOCOL_RFCOMM,
                                    localServer,
                                    secLevel,
                                    minEncKeySize,
                                    NULL);

        returnAclConnectionElement(cmData, devAddr, &aclConnectionElement);

        if (!aclConnectionElement &&
            CsrBtCmDmWriteKnownCacheParams(cmData, devAddr, RFC_PLAYER))
        {
            CSR_BT_CM_STATE_CHANGE(connInst->state,
                                   CSR_BT_CM_RFC_STATE_CONNECT_INIT);
            /* Wait for application of cached parameters */
        }
        else if ((aclConnectionElement &&
                  !CSR_BIT_IS_SET(aclConnectionElement->remoteFeatures[featIndex], featOffsetBit)) ||
                 !aclConnectionElement)
        {
            CsrBtCmRfcStartInitiateConnection(cmData, connInst);
        }
        else /* if (aclConnectionElement) */
        {
            CSR_BT_CM_STATE_CHANGE(connInst->state,
                                   CSR_BT_CM_RFC_STATE_ACCESS);
            CsrBtCmDmSmAccessReqMsgSend();
        }
    }
    else
    {
        RFC_PORTNEG_VALUES_T portParDefault;
        CsrBtPortParDefault(&portParDefault);

        if (remoteServer == CSR_BT_NO_SERVER)
        {
            csrBtCmConnectCfmMsgSend(appHandle, devAddr, CSR_BT_CONN_ID_INVALID, 0,
                                     FALSE, portParDefault, CSR_BT_RESULT_CODE_CM_COMMAND_DISALLOWED, CSR_BT_SUPPLIER_CM, context);
        }
        else
        {
            csrBtCmConnectCfmMsgSend(appHandle, devAddr, CSR_BT_CONN_ID_INVALID, 0,
                                     FALSE, portParDefault, CSR_BT_RESULT_CODE_CM_INTERNAL_ERROR, CSR_BT_SUPPLIER_CM, context);
        }
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }
}

void CsrBtCmConnectCfmMsgSend(cmInstanceData_t *cmData, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{/* Send a CSR_BT_CM_CONNECT_CFM signal to the application */
    cmRfcConnElement *rfcConnElement = CM_RFC_ELEMENT_ACTIVE(cmData);
    cmRfcConnInstType *theLogicalLink;
    RFC_PORTNEG_VALUES_T portParDefault;

    if (!rfcConnElement || !rfcConnElement->cmRfcConnInst)
    {
        /* This should not happen as the queue is locked and the ACTIVE element is maintained.
         * Raise an exception and tear down gracefully.*/
        CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                CSR_BT_CM_CONNECT_CFM,
                                0,
                                "Either cmRfcConnElement or cmRfcConnInstType is NULL");
        cmData->rfcVar.cancelConnect = FALSE;
        CsrBtCmDmSmClearRebondData(cmData);
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
        return;
    }

    theLogicalLink = rfcConnElement->cmRfcConnInst;

    CsrBtPortParDefault(&portParDefault);

    CsrBtCmDmUpdateAndClearCachedParamReqSend(theLogicalLink->deviceAddr);

    csrBtCmConnectCfmMsgSend(theLogicalLink->appHandle,
                             theLogicalLink->deviceAddr,
                             theLogicalLink->btConnId,
                             theLogicalLink->profileMaxFrameSize,
                             TRUE,
                             portParDefault,
                             resultCode,
                             resultSupplier,
                             theLogicalLink->context);

    if(resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        /* The attempt to create a connection is a SUCCESS */
        CSR_BT_CM_STATE_CHANGE(theLogicalLink->state, CSR_BT_CM_RFC_STATE_CONNECTED);

        if(theLogicalLink->controlSignalQueue != NULL)
        { /* There is control signals on the controlSignalQueue. Restore signal and send it to the application */
            RFC_MODEM_STATUS_IND_T  *prim;

            prim = (RFC_MODEM_STATUS_IND_T *) theLogicalLink->controlSignalQueue;
            CsrBtCmControlIndMsgSend(theLogicalLink, prim->modem_signal, prim->break_signal);
            bluestack_msg_free(RFCOMM_PRIM, prim);
            theLogicalLink->controlSignalQueue = NULL;
        }

#ifndef CSR_STREAMS_ENABLE
        if(theLogicalLink->dataControl.receivedBuffer[theLogicalLink->dataControl.restoreCount] != NULL)
        { /* There is data in the receivebuffer. Restore signal and send the payload to the application */
            CsrBtCmRfcRestoreDataInReceiveBuffer(theLogicalLink);
        }
#endif
    }
    else
    { /* The attempt to create a connection fail. Clean up the RFC connection table */
        dm_sm_unregister_outgoing_req(CSR_BT_CM_IFACEQUEUE,
                                      0, /* context */
                                      &theLogicalLink->deviceAddr,
                                      SEC_PROTOCOL_RFCOMM,
                                      theLogicalLink->serverChannel,
                                      NULL);
        CsrBtCmDmSyncClearPcmSlotFromTable(cmData, theLogicalLink->eScoParms);
        cleanUpConnectionTable(&(CM_RFC_ELEMENT_ACTIVE(cmData)->cmRfcConnInst));
    }
    cmData->rfcVar.cancelConnect = FALSE;

    CsrBtCmDmSmClearRebondData(cmData);
}

#ifdef CSR_BT_INSTALL_CM_PRI_CONNECT_EXT
void CsrBtCmRfcConnectReqExtHandler(cmInstanceData_t *cmData)
{ /* This event is used then the application request to create a new connection. */

    CsrBtCmConnectExtReq    * cmPrim;

    cmPrim                        = (CsrBtCmConnectExtReq *) cmData->recvMsgP;

    csrBtCmCommonRfcConnectHandle(cmData,
                                  cmPrim->appHandle,
                                  cmPrim->localServerCh,
                                  cmPrim->remoteServerCh,
                                  cmPrim->profileMaxFrameSize,
                                  cmPrim->deviceAddr,
                                  cmPrim->secLevel,
                                  CSR_BT_CM_CONTEXT_UNUSED,
                                  cmPrim->modemStatus,
                                  cmPrim->breakSignal,
                                  cmPrim->mscTimeout,
                                  cmPrim->minEncKeySize);
}
#endif

void CsrBtCmRfcConnectReqHandler(cmInstanceData_t *cmData)
{ /* This event is used then the application request to create a new connection. */
    CsrUint8        remoteServerCh;
    CsrBtCmConnectReq    * cmPrim;

    cmPrim    = (CsrBtCmConnectReq *) cmData->recvMsgP;

    getSdcRemoteServerCh(cmData, cmPrim->appHandle, cmPrim->serviceHandle,
                         cmPrim->deviceAddr, &remoteServerCh);

    csrBtCmCommonRfcConnectHandle(cmData,
                                  cmPrim->appHandle,
                                  cmPrim->localServerCh,
                                  remoteServerCh,
                                  cmPrim->profileMaxFrameSize,
                                  cmPrim->deviceAddr,
                                  cmPrim->secLevel,
                                  cmPrim->context,
                                  cmPrim->modemStatus,
                                  cmPrim->breakSignal,
                                  cmPrim->mscTimeout,
                                  cmPrim->minEncKeySize);
}

#ifndef CSR_TARGET_PRODUCT_VM
static CsrBool CsrBtCmReconnect(cmInstanceData_t *cmData, CsrUint16 result, CsrBtDeviceAddr deviceAddr, CsrBool hciErrorType)
{
    CsrBool knownError = FALSE;

    if (hciErrorType)
    {
        hci_return_t errorCode = (hci_return_t)(result & 0x00FF);

        if (errorCode == HCI_ERROR_KEY_MISSING || errorCode == HCI_ERROR_AUTH_FAIL || errorCode == HCI_ERROR_PAIRING_NOT_ALLOWED)
        {
            knownError = TRUE;
        }
    }
    else
    {/* NOTE: l2cap error type occationally received in RFC connect confirm message */
        l2ca_conn_result_t errorCode = (l2ca_conn_result_t)result;
        RFC_RESPONSE_T rfcErrorCode = (RFC_RESPONSE_T)result;
        if (errorCode == L2CA_CONNECT_REJ_SECURITY || errorCode == L2CA_CONNECT_KEY_MISSING
            || rfcErrorCode == RFC_CONNECTION_REJ_SECURITY)
        {
            knownError = TRUE;
        }
    }
    
    if (knownError)
    {
        if (!cmData->rfcVar.cancelConnect && CsrBtCmDmSmRebondNeeded(cmData))
        {
            return TRUE;
        }
        else
        {
            CsrBtCmScRejectedForSecurityReasonMsgSend(cmData,
                                                      deviceAddr, FALSE);
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}
#endif

void CsrBtCmDmSmAccessReqMsgSend(void)
{
    CsrBtCmSmAccessReq * prim;

    prim       = (CsrBtCmSmAccessReq *)CsrPmemAlloc(sizeof(CsrBtCmSmAccessReq));
    prim->type = CSR_BT_CM_SM_ACCESS_REQ;

    CsrBtCmPutMessage(CSR_BT_CM_IFACEQUEUE, prim);
}

void CsrBtCmSmAccessReqHandler(cmInstanceData_t *cmData)
{
    CsrBtTypedAddr tDevAddr;
    cmRfcConnElement *rfcConnElement = CM_RFC_ELEMENT_ACTIVE(cmData);
    cmRfcConnInstType *theLogicalLink;

    if (!rfcConnElement || !rfcConnElement->cmRfcConnInst)
    {
        /* This should not happen as the queue is locked and the ACTIVE element is maintained.
         * Raise an exception and tear down gracefully.*/
        CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                CSR_BT_CM_SM_ACCESS_REQ,
                                0,
                                "Either cmRfcConnElement or cmRfcConnInstType is NULL");
        CsrBtCmDmLocalQueueHandler();
        return;
    }

    theLogicalLink = rfcConnElement->cmRfcConnInst;
    CsrBtAddrCopyFromPublic(&tDevAddr, &theLogicalLink->deviceAddr);
    dm_sm_access_req(CSR_BT_CM_IFACEQUEUE,
                     &tDevAddr,
                     SEC_PROTOCOL_RFCOMM,
                     theLogicalLink->serverChannel,
                     FALSE,
                     CSR_BT_CM_CONTEXT_UNUSED, /* unused: context */
                     NULL);

    /* Restore queue immediately since this can trigger authentication */
    CsrBtCmDmLocalQueueHandler();
}

static void cmRetryDmSmAccessReqOnCollisionFailure(CsrUint8 theIndex, cmInstanceData_t *cmData)
{
    cmRfcConnElement * theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromIndex, &(theIndex));

    if (theElement)
    {
        cmRfcConnInstType *theLogicalLink = theElement->cmRfcConnInst;
        if (cmData->rfcVar.cancelConnect)
        {
            cmData->smVar.arg.result.code     = CSR_BT_RESULT_CODE_CM_CANCELLED;
            cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_CM;
            CsrBtCmRfcCommonErrorHandler(cmData, theLogicalLink);
        }
        else
        {
            CsrBtCmDmSmAccessReqMsgSend();
        }
    }
    else
    {
        CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                CSR_BT_CM_CONNECT_REQ,
                                0,
                                "Did not find any cmRfcConnElement");
    }
}

void CsrBtCmDmSmAccessCfmHandler(cmInstanceData_t *cmData)
{
    DM_SM_ACCESS_CFM_T *dmPrim;
    cmRfcConnInstType *theLogicalLink;
    cmRfcConnElement *connElement = CM_RFC_ELEMENT_ACTIVE(cmData);

    dmPrim = (DM_SM_ACCESS_CFM_T *) cmData->recvMsgP;

    /* In some cases, we may not even have a connection */
    if (connElement == NULL)
    {
        /* FIXME - send error here? */
        return;
    }

    theLogicalLink = connElement->cmRfcConnInst;
    if (cmData->rfcVar.cancelConnect)
    {
        cmData->smVar.arg.result.code     = CSR_BT_RESULT_CODE_CM_CANCELLED;
        cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_CM;
        CsrBtCmRfcCommonErrorHandler(cmData, theLogicalLink);

        return;
    }

    switch(dmPrim->status)
    {
        case HCI_SUCCESS:
        {
            CsrBtCmRfcStartInitiateConnection(cmData, theLogicalLink);
            break;
        }
        case HCI_ERROR_NO_CONNECTION:
        case HCI_ERROR_DIFFERENT_TRANSACTION_COLLISION:
        {
            if (theLogicalLink->state == CSR_BT_CM_RFC_STATE_ACCESS)
            {
                if (dmPrim->status == HCI_ERROR_NO_CONNECTION)
                {
                    CsrBtCmRfcStartInitiateConnection(cmData, theLogicalLink);
                }
                else
                {
                    /* Retry DM SM Access Req after a delay */
                    if (theLogicalLink->dmSmAccessRetry < DM_SM_ACCESS_MAX_RETRY)
                    {
                        CsrSchedTimerSet(DM_SM_ACCESS_REQ_RETRY_DELAY,
                                         (void (*) (CsrUint16, void *)) cmRetryDmSmAccessReqOnCollisionFailure,
                                         (CsrUint16) connElement->elementId,
                                         (void *) cmData);
                        theLogicalLink->dmSmAccessRetry++;
                    }
                    else
                    {
                        cmData->smVar.arg.result.code     = (CsrBtResultCode) dmPrim->status;
                        cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_HCI;
                        CsrBtCmRfcCommonErrorHandler(cmData, theLogicalLink);
                    }                    
                }
            }
            else
            {
                cmData->smVar.arg.result.code     = (CsrBtResultCode) dmPrim->status;
                cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_HCI;
                CsrBtCmRfcCommonErrorHandler(cmData, theLogicalLink);
            }
            break;
        }
        default:
        {
#ifndef CSR_TARGET_PRODUCT_VM
            if (CsrBtCmReconnect(cmData, (CsrUint16)dmPrim->status, theLogicalLink->deviceAddr, TRUE))
            { /* The connect attempt fail because of security reason, e.g the local
                 device had a SSP link key and the remote device did not */
                CsrBtCmSmSppRepairIndSend(cmData, theLogicalLink->deviceAddr);
                CSR_BT_CM_STATE_CHANGE(cmData->rfcVar.connectState, CM_RFC_SSP_REPAIR);
            }
            else
#endif
            {
                cmData->smVar.arg.result.code     = (CsrBtResultCode) dmPrim->status;
                cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_HCI;
                CsrBtCmRfcCommonErrorHandler(cmData, theLogicalLink);
            }
            break;
        }
    }
}

void CsrBtCmCancelConnectReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmCancelConnectReq *prim = (CsrBtCmCancelConnectReq *) cmData->recvMsgP;
    cmRfcConnElement *theElement  = returnCancelConnectIndex(cmData, prim->localServerCh,
                                                             prim->appHandle, prim->deviceAddr, prim->typeToCancel);

    if(theElement == NULL)
    { /* The Connect Request Msg is not placed in the connection table */
        CsrUint8 dummy;
        CsrUint16     context;
        if(cancelServiceManagerMsg(cmData, prim->typeToCancel, prim->appHandle, prim->deviceAddr, prim->localServerCh, NO_LOCAL_PSM, &dummy, &context))
        { /* The Connect request msg is found and removed from the local
             SM queue */
            RFC_PORTNEG_VALUES_T      portPar;
            CsrBtPortParDefault(&portPar);
            csrBtCmConnectCfmMsgSend(prim->appHandle, prim->deviceAddr, (CsrUint32)(prim->localServerCh), 0, FALSE, portPar,
                                     CSR_BT_RESULT_CODE_CM_CANCELLED, CSR_BT_SUPPLIER_CM, context);
        }
        else
        { /* Nothing to cancel just ignore */
            ;
        }
    }
    else
    {
        cmRfcConnInstType *theLogicalLink   = theElement->cmRfcConnInst;
        cmData->rfcVar.cancelConnect        = TRUE;
        CsrBtCmScRejectedForSecurityReasonMsgSend(cmData,
                                                  prim->deviceAddr, TRUE);

        switch (cmData->rfcVar.connectState)
        {
            case CM_RFC_IDLE:
            { /* The CM is about to or is setting an outgoing RFCOMM up             */
                if (CsrBtCmDmCancelPageOrDetach(cmData, prim->deviceAddr))
                { /* The ACL is being detach change state to ensure that
                     it not cancel twice                                            */
                    CSR_BT_CM_STATE_CHANGE(cmData->rfcVar.connectState, CM_RFC_CANCELING);
                }
                else if (theLogicalLink->pending)
                { /* Request RFCOMM to cancel the outgoing connection               */
                    CSR_BT_CM_STATE_CHANGE(cmData->rfcVar.connectState, CM_RFC_CANCELING);
                    RfcDisconnectReqSend((CsrUint16)theLogicalLink->btConnId);
                    theLogicalLink->pending = FALSE;
                }
                else
                { /* Must Wait until CM receives the right Confirm Msg              */
                    ;
                }
                break;
            }
            case CM_RFC_SSP_REPAIR:
            {
                cmData->smVar.arg.result.code     = CSR_BT_RESULT_CODE_CM_CANCELLED;
                cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_CM;
                CsrBtCmSmCancelSppRepairInd(cmData);
                CSR_BT_CM_STATE_CHANGE(cmData->rfcVar.connectState, CM_RFC_CANCELING);
                CsrBtCmRfcCommonErrorHandler(cmData, theLogicalLink);
                break;
            }
            default:
            { /* Must Wait until CM receives the right Confirm Msg */
                break;
            }
        }
    }
}

#ifdef INSTALL_CONTEXT_TRANSFER
static void csrBtCmConnContextSendCfmMsg(cmInstanceData_t *cmData, CsrBool isClient)
{
    cmRfcConnInstType  * theLogicalLink;
    cmRfcConnElement * connElement;
    RFC_PORTNEG_VALUES_T portParDefault;
    RFC_CONNECT_CFM_T *prim = cmData->recvMsgP;


    connElement = CM_RFC_ELEMENT_ACTIVE(cmData);
    if (connElement == NULL || connElement->cmRfcConnInst == NULL)
    {
        CsrBtCmGeneralException(CSR_BT_CM_PRIM,
            CSR_BT_CM_CONNECT_CFM,
            0,
            "Did not find any cmRfcConnElement OR cmRfcConnInstType");
        return;
    }

    theLogicalLink = connElement->cmRfcConnInst;
    CsrBtPortParDefault(&portParDefault);

    if (isClient)
    {
        csrBtCmConnectCfmMsgSend(theLogicalLink->appHandle,
                                 theLogicalLink->deviceAddr,
                                 theLogicalLink->btConnId,
                                 theLogicalLink->profileMaxFrameSize,
                                 TRUE,
                                 portParDefault,
                                 CSR_BT_RESULT_CODE_CM_SUCCESS,
                                 CSR_BT_SUPPLIER_CM,
                                 theLogicalLink->remoteServerChan);
    }
    else
    {
        csrBtCmAcceptConnectCfmMsgSend(theLogicalLink->appHandle,
                                       theLogicalLink->deviceAddr,
                                       theLogicalLink->btConnId,
                                       theLogicalLink->serverChannel,
                                       theLogicalLink->profileMaxFrameSize,
                                       CSR_BT_RESULT_CODE_CM_SUCCESS,
                                       CSR_BT_SUPPLIER_CM,
                                       prim->context);

    }
    CsrBtCmDmSmClearRebondData(cmData);
}

static void csrBtCmCommonRfcConnectHandleExt(cmInstanceData_t *cmData,
                                          CsrUint8 localServer,
                                          CsrUint8 remoteServer,
                                          CsrUint16 profileMaxFrameSize,
                                          CsrBtDeviceAddr devAddr,
                                          CsrUint16 context,
                                          CsrUint8 modemStatus,
                                          CsrUint8 breakSignal,
                                          CsrUint8 mscTimeout,
                                          CsrUint8 minEncKeySize)
{
    CsrBtCmElementCounterIncrement(cmData);

    cmRfcConnElement  *theElement = (cmRfcConnElement *) CsrCmnListElementAddLast(&(cmData->rfcVar.connList), sizeof(cmRfcConnElement));
    cmRfcConnInstType *connInst   = theElement->cmRfcConnInst;

    theElement->elementId         = cmData->elementCounter;
    cmData->rfcVar.activeElemId   = theElement->elementId;
    cmData->rfcVar.cancelConnect  = FALSE;

    CsrBtCmDmSmClearRebondData(cmData);

    connInst->appHandle           = cmData->offloadHandle;
    connInst->deviceAddr          = devAddr;
    connInst->serverChannel       = localServer;
    connInst->btConnId            = CSR_BT_CONN_ID_INVALID;
    connInst->remoteServerChan    = remoteServer;
    connInst->profileMaxFrameSize = CsrBtCmRfcDetermineMtu(cmData, devAddr, profileMaxFrameSize);
    connInst->context             = context;
    connInst->modemStatus         = modemStatus;
    connInst->signalBreak         = breakSignal;
    connInst->mscTimeout          = mscTimeout;
}

static void csrBtCmRfcConnectCfmHandlerExt(cmInstanceData_t *cmData, CsrBool isClient)
{
    RFC_CLIENT_CONNECT_CFM_T *rfcPrim = (RFC_CLIENT_CONNECT_CFM_T *) cmData->recvMsgP;
    cmRfcConnElement * theElement;
    if (isClient)
    {
        /* We do not have a connection Id stored yet, as it is known first now; therefore seach for the server channel
           and the "no-connection-id" field */
        cmConnIdServerType  connIdServ       = CsrBtCmReturnConnIdServerStruct(CSR_BT_CONN_ID_INVALID, rfcPrim->serv_chan);
        theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromConnIdRemoteServer, &(connIdServ));
    }
    else
    {
        cmConnIdLocalRemoteServersType  cmConnEltType;

        /* We do not have a connection Id stored yet, as it is known first now;
         * therefore seach for the server channel and the "no-connection-id" field.
         * Also check if remote server is CSR_BT_NO_SERVER to avoid selecting
         * an in progress outgoing connection */
        cmConnEltType = CsrBtCmReturnConnIdLocalRemoteServersStruct(CSR_BT_CONN_ID_INVALID,
                                                                    rfcPrim->serv_chan,
                                                                    CSR_BT_NO_SERVER,
                                                                    rfcPrim->bd_addr);
        theElement  = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromConnIdLocalRemoteServers, &(cmConnEltType));
    }

    if(theElement)
    {
        cmRfcConnInstType *theLogicalLink   = theElement->cmRfcConnInst;

        if (RFC_SUCCESS == rfcPrim->status)
        {/* Connection succeeded */
            aclTable *aclConnectionElement  = NULL;

            theLogicalLink->btConnId    = CM_CREATE_RFC_CONN_ID(rfcPrim->conn_id);
            theLogicalLink->pending = FALSE;
            theLogicalLink->context     = rfcPrim->context;
            theLogicalLink->profileMaxFrameSize    = rfcPrim->max_payload_size;
            returnAclConnectionElement(cmData, rfcPrim->bd_addr, &aclConnectionElement);

            if (!cmData->rfcVar.cancelConnect)
            {
                CSR_BT_CM_STATE_CHANGE(cmData->rfcVar.connectState, CM_RFC_CONNECTED);
                CSR_BT_CM_STATE_CHANGE(theLogicalLink->state,
                                       CSR_BT_CM_RFC_STATE_CONNECTED);
                csrBtCmConnContextSendCfmMsg(cmData, isClient);
            }
        }
    }
}

void CmRfcConnectCb(bool is_client, RFC_CONNECT_CFM_T *prim)
{
    cmInstanceData_t *cmData = &csrBtCmData;
    CsrUint8 localChannel = is_client ? 0 : prim->serv_chan;
    CsrUint8 remoteChannel = is_client ? prim->serv_chan : CSR_BT_NO_SERVER;

    cmData->recvMsgP = prim;

    CsrBtCmDmAclOpenedSuccessIndHandlerExt(cmData, prim->bd_addr, FALSE, 0);

    csrBtCmCommonRfcConnectHandleExt(cmData, localChannel, remoteChannel, prim->max_payload_size, prim->bd_addr, 0,0,0,0,0);

    csrBtCmRfcConnectCfmHandlerExt(cmData, is_client);

    cmData->recvMsgP = NULL;
}
#endif /* ifdef INSTALL_CONTEXT_TRANSFER */

void CsrBtCmRfcClientConnectCfmHandler(cmInstanceData_t *cmData)
{
    RFC_CLIENT_CONNECT_CFM_T *rfcPrim = (RFC_CLIENT_CONNECT_CFM_T *) cmData->recvMsgP;
    /* We do not have a connection Id stored yet, as it is known first now; therefore seach for the server channel
       and the "no-connection-id" field */
    cmConnIdServerType  connIdServ       = CsrBtCmReturnConnIdServerStruct(CSR_BT_CONN_ID_INVALID, rfcPrim->serv_chan);
    cmRfcConnElement * theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromConnIdRemoteServer, &(connIdServ));

    if ((theElement == NULL) && cmData->rfcVar.cancelConnect)
    {/* This may be because we have issued a disconnect_req already.... */
        CsrBtConnId val = CM_CREATE_RFC_CONN_ID(rfcPrim->conn_id);
        connIdServ      = CsrBtCmReturnConnIdServerStruct(val, rfcPrim->serv_chan);
        theElement      = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromConnIdRemoteServer, &(connIdServ));
    }

    if(theElement)
    {
        cmRfcConnInstType *theLogicalLink   = theElement->cmRfcConnInst;

        if (RFC_SUCCESS == rfcPrim->status)
        {/* Connection succeeded */
            aclTable *aclConnectionElement  = NULL;

            theLogicalLink->btConnId    = CM_CREATE_RFC_CONN_ID(rfcPrim->conn_id);
            theLogicalLink->pending = FALSE;
            theLogicalLink->context     = rfcPrim->context;
            theLogicalLink->profileMaxFrameSize    = rfcPrim->max_payload_size;
            returnAclConnectionElement(cmData, rfcPrim->bd_addr, &aclConnectionElement);

            if (!cmData->rfcVar.cancelConnect)
            {
                cmData->smVar.arg.result.code     = CSR_BT_RESULT_CODE_CM_SUCCESS;
                cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_CM;
                CSR_BT_CM_STATE_CHANGE(cmData->rfcVar.connectState, CM_RFC_CONNECTED);

                CSR_BT_CM_STATE_CHANGE(theLogicalLink->state, CSR_BT_CM_RFC_STATE_CONNECTED);
                CsrBtCmConnectCfmMsgSend(cmData,
                                         cmData->smVar.arg.result.code,
                                         cmData->smVar.arg.result.supplier);
                CsrBtCmServiceManagerLocalQueueHandler(cmData);

                /* Inform local RFC connection completion to device utility.*/
                (void)CmDuHandleAutomaticProcedure(cmData,
                                                   CM_DU_AUTO_EVENT_SERVICE_CONNECTED,
                                                   NULL,
                                                   &theLogicalLink->deviceAddr);
            }
            else
            {
                /* Crossing release and connect cfm */
                CSR_BT_CM_STATE_CHANGE(cmData->rfcVar.connectState, CM_RFC_CANCELING);
                RfcDisconnectReqSend((CsrUint16)theLogicalLink->btConnId);
            }
        }
#ifndef CSR_TARGET_PRODUCT_VM
        else if (CsrBtCmReconnect(cmData, (CsrUint16)rfcPrim->status, theLogicalLink->deviceAddr, FALSE))
        { /* The connect attempt fail because of security reason, e.g the local
             device had a SSP link key and the remote device did not */
            CsrBtCmSmSppRepairIndSend(cmData, theLogicalLink->deviceAddr);
            CSR_BT_CM_STATE_CHANGE(cmData->rfcVar.connectState, CM_RFC_SSP_REPAIR);
        }
#endif
        else
        {
            if (RFC_CONNECTION_PENDING != rfcPrim->status)
            {
                CsrBtResultCode      resultCode;
                RFC_PORTNEG_VALUES_T portPar;
                CsrBtPortParDefault(&portPar);
              
                theLogicalLink->pending = FALSE;
                if (theLogicalLink->remoteServerChan == CSR_BT_NO_SERVER)
                {
                    resultCode = CSR_BT_RESULT_CODE_CM_COMMAND_DISALLOWED;
                }
                else
                {
                    resultCode = CSR_BT_RESULT_CODE_CM_INTERNAL_ERROR;
                }
                CsrBtCmConnectCfmMsgSend(cmData, resultCode, CSR_BT_SUPPLIER_CM);
                CsrBtCmServiceManagerLocalQueueHandler(cmData);
            }
            else
            {/* pending! */
                theLogicalLink->btConnId = CM_CREATE_RFC_CONN_ID(rfcPrim->conn_id);
                theLogicalLink->pending = TRUE;
                
                if (cmData->rfcVar.cancelConnect && 
                    cmData->rfcVar.connectState != CM_RFC_CANCELING)
                { /* Request RFCOMM to cancel the outgoing connection. E.g. 
                     CsrBtCmCancelConnectReq were received before CM receives 
                     this message*/
                    CSR_BT_CM_STATE_CHANGE(cmData->rfcVar.connectState, CM_RFC_CANCELING);
                    RfcDisconnectReqSend((CsrUint16)theLogicalLink->btConnId);
                    theLogicalLink->pending = FALSE;
                }
            }
        }
    }
    else
    { 
        ;
    }
}

static void cmRfcAcceptConnectWithTimeOut(CsrUint8 theIndex, cmInstanceData_t *cmData)
{
    cmRfcConnElement * theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromIndex, &(theIndex));

    if (theElement)
    {
        /* Remove the COD. Check if the Host still need to be connectable and
           send CSR_BT_CM_CONNECT_ACCEPT_CFM with timeout to the application */
        cmRfcConnInstType *theLogicalLink = theElement->cmRfcConnInst;
        CsrBtCmAcceptConnectTimeout *prim = (CsrBtCmAcceptConnectTimeout*) CsrPmemAlloc(sizeof(CsrBtCmAcceptConnectTimeout));

        theLogicalLink->timerId = CSR_SCHED_TID_INVALID;

        prim->type = CSR_BT_CM_ACCEPT_CONNECT_TIMEOUT;
        prim->serverChannel = theLogicalLink->serverChannel;
        CSR_BT_CM_STATE_CHANGE(theLogicalLink->state,
                               CSR_BT_CM_RFC_STATE_CANCEL_TIMER);

        cmData->recvMsgP = prim;
        CsrBtCmServiceManagerProvider(cmData);

        SynergyMessageFree(CSR_BT_CM_PRIM, cmData->recvMsgP);
        cmData->recvMsgP = NULL;
    }
    else
    {
        CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                CSR_BT_CM_ACCEPT_CONNECT_TIMEOUT,
                                0,
                                "Did not find any cmRfcConnElement");
    }
}

static void cmRfcConnectAcceptTimer(cmInstanceData_t *cmData,
                                    cmRfcConnInstType * theLink,
                                    CsrUint16 theTime,
                                    CsrUint8 theIndex)
{
    /* Start a connect accept timer if the timeout is not infinite. In case of a finite timeout value,
     * the caller needs to be informed with CSR_BT_CM_CONNECT_ACCEPT_CFM event with status
     * as CSR_BT_RESULT_CODE_CM_TIMEOUT if the connection doesn't happen in a given timeout value. */
    if(theTime != CSR_BT_INFINITE_TIME)
    {
        if(theTime > (CSR_BT_MAX_TIME / CSR_BT_MICROSEC2SEC))
        {
            theTime = (CSR_BT_MAX_TIME / CSR_BT_MICROSEC2SEC);
        }
        theLink->timerId   = CsrSchedTimerSet(theTime * CSR_BT_MICROSEC2SEC,
                                              (void (*) (CsrUint16, void *)) cmRfcAcceptConnectWithTimeOut,
                                              (CsrUint16) theIndex,
                                              (void *) cmData);
    }
}

/* Handles remote (incoming) connection related states. */
void CmRfcRemoteConnectionStateHandler(cmInstanceData_t    *cmData,
                                       cmRfcConnElement    *connElement)
{
    CsrBool notifyConnectable = TRUE;

    if (connElement && connElement->cmRfcConnInst)
    {
        cmRfcConnInstType *theLogicalLink = connElement->cmRfcConnInst;

        switch (theLogicalLink->state)
        {
            case CSR_BT_CM_RFC_STATE_CONNECTABLE:
            {
                /* Check if application needs to establish incoming connection within a given time limit. */
                cmRfcConnectAcceptTimer(cmData,
                                        theLogicalLink,
                                        cmData->rfcVar.connectAcceptTimeOut,
                                        connElement->elementId);

                /* Initialize the connect accept state machine. */
                CSR_BT_CM_STATE_CHANGE(theLogicalLink->state, CSR_BT_CM_RFC_STATE_IDLE);
                break;
            }

            case CSR_BT_CM_RFC_STATE_CANCEL_CONNECTABLE:
            { /* A connectAble service has been cancel. Restore the RFC queue */

                if (theLogicalLink->timerId != CSR_SCHED_TID_INVALID)
                {
                    CsrSchedTimerCancel(theLogicalLink->timerId, NULL, NULL);
                    theLogicalLink->timerId = CSR_SCHED_TID_INVALID;
                }

                if (CsrBtCmIncomingSecRegisterDeregisterRequired(cmData, theLogicalLink->serverChannel))
                {
                    CsrBtScDeregisterReqSend(SEC_PROTOCOL_RFCOMM, theLogicalLink->serverChannel);
                }
                CsrBtCmDmSyncClearPcmSlotFromTable(cmData,
                                                   theLogicalLink->eScoParms);
                cleanUpConnectionTable(&(connElement->cmRfcConnInst));
                break;
            }

            case CSR_BT_CM_RFC_STATE_CANCEL_TIMER:
            { /* A connectAble service has timeout. Build and send
                 CSR_BT_CM_CONNECT_ACCEPT_CFM to the application. Restore the RFC queue. */
                CsrBtCmConnectAcceptCfmMsgSend(cmData, connElement, CSR_BT_RESULT_CODE_CM_TIMEOUT, CSR_BT_SUPPLIER_CM);
                break;
            }

            case CSR_BT_CM_RFC_STATE_CONNECT_ACCEPT:
            case CSR_BT_CM_RFC_STATE_CONNECT_ACCEPT_FINAL:
            {
                if (cmData->smVar.arg.result.code == CSR_BT_RESULT_CODE_CM_SUCCESS &&
                    cmData->smVar.arg.result.supplier == CSR_BT_SUPPLIER_CM)
                {
                    /* A new RFCOMM connection have just been establish with SUCCESS.
                     * Write lp settings and inform the application.
                     *
                     * Currently there is at least one more connection attach to this
                     * device address. The rfc connection has been accepted with with success.
                     * Inform the application, and restore the local service manager
                     * and DM queue.
                     */
                    CsrBtCmConnectAcceptCfmMsgSend(cmData, connElement, CSR_BT_RESULT_CODE_CM_SUCCESS, CSR_BT_SUPPLIER_CM);
                    /* Inform this to Device utility if its getting used. */
                    (void)CmDuHandleAutomaticProcedure(cmData,
                                                       CM_DU_AUTO_EVENT_SERVICE_CONNECTED,
                                                       NULL,
                                                       &connElement->cmRfcConnInst->deviceAddr);
                    /* Since we have already notified to device utility, no need to do it towards the end of this function. */
                    notifyConnectable = FALSE;
                }
                else
                {
                    /* A new RFCOMM connection have just been establish with FAIL.*/
                    CsrBtCmConnectAcceptCfmMsgSend(cmData,
                                                   connElement,
                                                   cmData->smVar.arg.result.code,
                                                   cmData->smVar.arg.result.supplier);
                }
                break;
            }

            default:
                break;
        }
    }

    if (notifyConnectable)
    {
        /* Kick the automatic scan procedures of device utility. */
        (void)CmDuHandleAutomaticProcedure(cmData,
                                           CM_DU_AUTO_EVENT_CONNECTABLE,
                                           NULL,
                                           NULL);
    }

    if (connElement)
    { /* If connElement == NULL then this function has allready been called */
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }
}

void CsrBtCmRfcServerConnectCfmHandler(cmInstanceData_t *cmData)
{
    RFC_SERVER_CONNECT_CFM_T *rfcPrim = (RFC_SERVER_CONNECT_CFM_T *) cmData->recvMsgP;
    CsrBtConnId val = CM_CREATE_RFC_CONN_ID(rfcPrim->conn_id);
    cmRfcConnElement * rfcElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromBtConnId, &val);

    if(rfcElement)
    {
        cmRfcConnInstType *rfcLogicalLink   = rfcElement->cmRfcConnInst;
        cmData->rfcVar.activeElemId         = rfcElement->elementId;
        rfcLogicalLink->profileMaxFrameSize = rfcPrim->max_payload_size;
        CSR_BT_CM_STATE_CHANGE(rfcLogicalLink->state,
                               CSR_BT_CM_RFC_STATE_CONNECT_ACCEPT_FINAL);

        if (RFC_SUCCESS == rfcPrim->status)
        {/* Connection succeeded */
            cmData->smVar.arg.result.code        = CSR_BT_RESULT_CODE_CM_SUCCESS;
            cmData->smVar.arg.result.supplier    = CSR_BT_SUPPLIER_CM;
        }
        else
        {
            cmData->smVar.arg.result.code   = rfcPrim->status;
            if (rfcPrim->status < RFCOMM_ERRORCODE_BASE)
            {
                cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_L2CAP_CONNECT;
            }
            else
            {
                cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_RFCOMM;
            }
        }
        CmRfcRemoteConnectionStateHandler(cmData, rfcElement);
    }
    else
    { /* No owner, just ignore */
        ;
    }
}

static void csrBtCmConnectAcceptIndSend(CsrSchedQid phandle,
                                        CsrBtConnId btConnId,
                                        CsrBtDeviceAddr deviceAddr,
                                        uint8 localServerChannel)
{
    CsrBtCmRfcConnectAcceptInd *prim = (CsrBtCmRfcConnectAcceptInd *)CsrPmemZalloc(sizeof(*prim));

    prim->type               = CSR_BT_CM_RFC_CONNECT_ACCEPT_IND;
    prim->btConnId           = btConnId;
    prim->deviceAddr         = deviceAddr;
    prim->localServerChannel = localServerChannel;

    CsrBtCmPutMessage(phandle, prim);
}

static void csrBtCmRfcServerConnectRejectHandler(cmInstanceData_t *cmData, CsrUint16 connId)
{
    RfcServerConnectResSend(RFC_FLAGS_UNUSED,
                            connId,
                            RFC_DECLINE_SERVER_CONNECTION,
                            0,
                            0,
                            CSR_BT_CM_INIT_CREDIT,
                            L2CA_AMP_CONTROLLER_BT,
                            L2CA_AMP_CONTROLLER_BT,
                            CSR_BT_DEFAULT_MODEM_STATUS,
                            CSR_BT_DEFAULT_BREAK_SIGNAL,
                            CSR_BT_DEFAULT_MSC_TIMEOUT);

    /* We are not active after rejecting this request */
    cmData->rfcVar.activeElemId = CM_ERROR;

    if (cmData->smVar.popFromSaveQueue)
    { /* The RFC_SERVER_CONNECT_IND message has been restore
         from the SM queue. Restore it and lock it again
         in order to make sure that no message can use
         the SM before the CSR_BT_CM_SM_HOUSE_CLEANING
         is received. */
        cmData->smVar.smInProgress = TRUE;
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }
}

static cmRfcConnElement *cmRfcFindRfcConnElement(cmInstanceData_t *cmData,
                                                 CsrUint8 localServer,
                                                 CsrBtDeviceAddr devAddr)
{
    cmConnIdLocalRemoteServersType cmConnEltType;
    cmRfcConnElement               *rfcElement = NULL;

    cmConnEltType = CsrBtCmReturnConnIdLocalRemoteServersStruct(CSR_BT_CONN_ID_INVALID,
                                                                localServer,
                                                                CSR_BT_NO_SERVER,
                                                                devAddr);
    rfcElement    = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromConnIdLocalRemoteServers,
                                        &(cmConnEltType));

    if (!rfcElement)
    {
        CsrBtDeviceAddr zeroBdAddr = {0, 0, 0};

        /* We do not have a connection Id stored yet, as it is known first now;
         * therefore search for the server channel and the "no-connection-id" field.
         * Also check if remote server is CSR_BT_NO_SERVER to avoid selecting
         * an in progress outgoing connection */
        cmConnEltType = CsrBtCmReturnConnIdLocalRemoteServersStruct(CSR_BT_CONN_ID_INVALID,
                                                                    localServer,
                                                                    CSR_BT_NO_SERVER,
                                                                    zeroBdAddr);
        rfcElement    = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromConnIdLocalRemoteServers,
                                            &(cmConnEltType));
    }

    return (rfcElement);
}

void CsrBtCmRfcServerConnectIndHandler(cmInstanceData_t *cmData)
{
    if (cmData->smVar.smInProgress)
    {
        /* Another service is in progress */
        CsrMessageQueuePush(&cmData->smVar.saveQueue, RFCOMM_PRIM, cmData->recvMsgP);
        cmData->recvMsgP = NULL;
    }
    else
    {
        RFC_SERVER_CONNECT_IND_T       *rfcPrim = (RFC_SERVER_CONNECT_IND_T *) cmData->recvMsgP;

        if (rfcPrim->local_l2cap_control == L2CA_AMP_CONTROLLER_BT)
        {
            if (rfcPrim->context != CSR_BT_CM_CONTEXT_UNUSED &&
                CsrBtCmElementCounterIncrement(cmData))
            {
                /* Context is being used, it means application has registered for
                 * handling connect indication messages, lock the sm queue and indicate
                 * application about incoming connection. */
                CsrBtCmSmLockQueue(cmData);
                csrBtCmConnectAcceptIndSend((CsrSchedQid)rfcPrim->context,
                                            CM_CREATE_RFC_CONN_ID(rfcPrim->conn_id),
                                            rfcPrim->bd_addr,
                                            rfcPrim->loc_serv_chan);
            }
            else
            {
                /* Accept the connection if any RFC connection element is available */
                cmRfcConnElement *rfcElement = cmRfcFindRfcConnElement(cmData,
                                                                       rfcPrim->loc_serv_chan,
                                                                       rfcPrim->bd_addr);

                if (rfcElement)
                {
                    cmRfcConnInstType *rfcLogicalLink = rfcElement->cmRfcConnInst;

                    /* Lock the service manager queue and send response. */
                    CsrBtCmSmLockQueue(cmData);

                    if (rfcLogicalLink->timerId != CSR_SCHED_TID_INVALID)
                    {/* Reset the timer */
                        CsrSchedTimerCancel(rfcLogicalLink->timerId, NULL, NULL);
                        rfcLogicalLink->timerId = CSR_SCHED_TID_INVALID;
                    }

                    rfcLogicalLink->btConnId            = CM_CREATE_RFC_CONN_ID(rfcPrim->conn_id);
                    rfcLogicalLink->deviceAddr          = rfcPrim->bd_addr;
                    rfcLogicalLink->profileMaxFrameSize = CsrBtCmRfcDetermineMtu(cmData,rfcLogicalLink->deviceAddr,0);
                    cmData->rfcVar.activeElemId         = rfcElement->elementId;
                    CSR_BT_CM_STATE_CHANGE(rfcLogicalLink->state, CSR_BT_CM_RFC_STATE_CONNECT_ACCEPT);

                    RfcServerConnectResSend(rfcPrim->flags,
                                            rfcPrim->conn_id,
                                            RFC_ACCEPT_SERVER_CONNECTION,
                                            rfcLogicalLink->profileMaxFrameSize,
                                            0,
                                            CSR_BT_CM_INIT_CREDIT,
                                            L2CA_AMP_CONTROLLER_BT,
                                            L2CA_AMP_CONTROLLER_BT,
                                            rfcLogicalLink->modemStatus,
                                            rfcLogicalLink->signalBreak,
                                            rfcLogicalLink->mscTimeout);
                }
                else
                {
                    /* Neither connect accept done nor application registered for connection handling,
                     * reject the connection. */
                    csrBtCmRfcServerConnectRejectHandler(cmData, rfcPrim->conn_id);
                }
            }
        }
        else
        {
            /* Reject the connection for unknown controller. */
            csrBtCmRfcServerConnectRejectHandler(cmData, rfcPrim->conn_id);
        }
    }
}

void CsrBtCmRfcConnectAcceptRspHandler(cmInstanceData_t *cmData)
{
    CsrBtCmRfcConnectAcceptRsp *prim = (CsrBtCmRfcConnectAcceptRsp *)cmData->recvMsgP;

    if (prim->accept)
    { /* Connection is accepted, find if any RFC conn element is already available */
        cmRfcConnInstType *rfcConnInst;
        cmRfcConnElement  *rfcElement = cmRfcFindRfcConnElement(cmData,
                                                                prim->serverChannel,
                                                                prim->deviceAddr);

        /* RFC element would have been already created if application has sent
         * Connect Accept REQ before connection. Do not create new if one already exists.
         * In such cases, application wants to have connection element upfront but doesn't
         * want CM to take decision on connection (accept/reject).
         */
        if (!rfcElement)
        { /* Continue with adding new connection element to the list */
            rfcElement = (cmRfcConnElement *) CsrCmnListElementAddLast(&(cmData->rfcVar.connList),
                                                                       sizeof(cmRfcConnElement));
            rfcConnInst                  = rfcElement->cmRfcConnInst;
            rfcConnInst->context         = CSR_BT_CM_CONTEXT_UNUSED;
            rfcConnInst->classOfDevice   = 0;
        }
        else
        { /* RFC connection element does exists, start using it */
            rfcConnInst                  = rfcElement->cmRfcConnInst;
        }

        rfcElement->elementId = cmData->rfcVar.activeElemId = cmData->elementCounter;

        rfcConnInst->deviceAddr          = prim->deviceAddr;
        rfcConnInst->serverChannel       = prim->serverChannel;
        rfcConnInst->profileMaxFrameSize = CsrBtCmRfcDetermineMtu(cmData, rfcConnInst->deviceAddr, 0);
        rfcConnInst->btConnId            = prim->btConnId;
        rfcConnInst->appHandle           = prim->appHandle;
        rfcConnInst->modemStatus         = prim->modemStatus;
        rfcConnInst->signalBreak         = prim->breakSignal;
        rfcConnInst->mscTimeout          = prim->mscTimeout;

        CSR_BT_CM_STATE_CHANGE(rfcConnInst->state, CSR_BT_CM_RFC_STATE_CONNECT_ACCEPT);

        /* Accept the connection. */
        RfcServerConnectResSend(RFC_FLAGS_UNUSED,
                                CM_GET_UINT16ID_FROM_BTCONN_ID(prim->btConnId),
                                RFC_ACCEPT_SERVER_CONNECTION,
                                rfcConnInst->profileMaxFrameSize,
                                0,
                                CSR_BT_CM_INIT_CREDIT,
                                L2CA_AMP_CONTROLLER_BT,
                                L2CA_AMP_CONTROLLER_BT,
                                rfcConnInst->modemStatus,
                                rfcConnInst->signalBreak,
                                rfcConnInst->mscTimeout);
    }
    else
    {
        /* Reject the connection. */
        csrBtCmRfcServerConnectRejectHandler(cmData, CM_GET_UINT16ID_FROM_BTCONN_ID(prim->btConnId));

        /* Since we are rejecting the connection, unlock service manager queue which was locked
         * while sending CSR_BT_CM_RFC_CONNECT_ACCEPT_IND */
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }
}
#endif /* #ifndef EXCLUDE_CSR_BT_RFC_MODULE */

