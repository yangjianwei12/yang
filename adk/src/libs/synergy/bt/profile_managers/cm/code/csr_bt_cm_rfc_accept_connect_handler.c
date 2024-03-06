/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_RFC_MODULE

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif


void csrBtCmAcceptConnectCfmMsgSend(CsrSchedQid                  appHandle,
                                           BD_ADDR_T            deviceAddr,
                                           CsrBtConnId           btConnId,
                                           CsrUint8              localServerChannel,
                                           CsrUint16             profileMaxFrameSize,
                                           CsrBtResultCode      resultCode,
                                           CsrBtSupplier  resultSupplier,
                                           CsrUint16     context)
{
    CsrBtCmConnectAcceptCfm        *prim;

    prim                        = (CsrBtCmConnectAcceptCfm *)CsrPmemAlloc(sizeof(CsrBtCmConnectAcceptCfm));
    prim->type                  = CSR_BT_CM_CONNECT_ACCEPT_CFM;
    prim->deviceAddr            = deviceAddr;
    prim->btConnId              = btConnId;
    prim->serverChannel         = localServerChannel;
    prim->profileMaxFrameSize   = profileMaxFrameSize;
    prim->resultCode            = resultCode;
    prim->resultSupplier        = resultSupplier;
    prim->context               = context;
    CsrBtCmPutMessage(appHandle, prim);
}

static void csrBtCmAcceptConnectErrorHandler(cmInstanceData_t     *cmData,
                                             CsrSchedQid               appHandle,
                                             CsrUint8        theServer,
                                             CsrBtResultCode      resultCode,
                                             CsrBtSupplier        resultSupplier,
                                             CsrUint16            context)
{
    CsrBtDeviceAddr        deviceAddr;

    CsrBtBdAddrZero(&deviceAddr);
    csrBtCmAcceptConnectCfmMsgSend(appHandle, deviceAddr, CSR_BT_CONN_ID_INVALID, theServer, 0, resultCode, resultSupplier, context);
    CsrBtCmServiceManagerLocalQueueHandler(cmData);
}

void CsrBtCmConnectAcceptCfmMsgSend(cmInstanceData_t    *cmData,
                                    cmRfcConnElement    *theElement,
                                    CsrBtResultCode     resultCode,
                                    CsrBtSupplier resultSupplier)
{/* Send a Connect accept cfm signal to the application                            */
    cmRfcConnInstType * theLogicalLink = theElement->cmRfcConnInst;

    csrBtCmAcceptConnectCfmMsgSend(theLogicalLink->appHandle,
                                   theLogicalLink->deviceAddr,
                                   theLogicalLink->btConnId,
                                   theLogicalLink->serverChannel,
                                   theLogicalLink->profileMaxFrameSize,
                                   resultCode,
                                   resultSupplier,
                                   theLogicalLink->context);

    if(resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        /* A new connection is establish with success */
        CSR_BT_CM_STATE_CHANGE(theLogicalLink->state, CSR_BT_CM_RFC_STATE_CONNECTED);

        if(theLogicalLink->controlSignalQueue != NULL)
        { /* There is control signals on the controlSignalQueue. Restore
             signal and send it to the application                                */
            RFC_MODEM_STATUS_IND_T  *prim;

            prim        = (RFC_MODEM_STATUS_IND_T *) theLogicalLink->controlSignalQueue;
            CsrBtCmControlIndMsgSend(theLogicalLink, prim->modem_signal, prim->break_signal);
            bluestack_msg_free(RFCOMM_PRIM, prim);
            theLogicalLink->controlSignalQueue = NULL;
        }

#ifndef CSR_STREAMS_ENABLE
        if(theLogicalLink->dataControl.receivedBuffer[theLogicalLink->dataControl.restoreCount] != NULL)
        { /* There is data in the receivebuffer. Restore signal and send the
             payload to the application                                            */
            CsrBtCmRfcRestoreDataInReceiveBuffer(theLogicalLink);
        }
#endif
    }
    else
    { /* The attempt of creating a new connection fail, clean up the
         connection table                                                        */

        /* Connection elements which got created because of response from application, 
         * doesn't require deregistering as the security was never registered. */
        if (!theElement->app_controlled &&
            CsrBtCmIncomingSecRegisterDeregisterRequired(cmData, theLogicalLink->serverChannel))
        {
            CsrBtScDeregisterReqSend(SEC_PROTOCOL_RFCOMM, theLogicalLink->serverChannel);
        }

        CsrBtCmDmSyncClearPcmSlotFromTable(cmData, theLogicalLink->eScoParms);
        cleanUpConnectionTable(&(theElement->cmRfcConnInst));
    }
}

void CsrBtCmRfcAcceptConnectReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmConnectAcceptReq *cmPrim = (CsrBtCmConnectAcceptReq *) cmData->recvMsgP;
    cmConnIdServerContextType  serverInst = CsrBtCmReturnConnIdServerContextStruct(CSR_BT_CONN_ID_INVALID, cmPrim->serverChannel, cmPrim->context);
    cmRfcConnElement * theElement   = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromConnIdServerContext, &(serverInst));
    cmRfcConnInstType *connInst;

    if(theElement == NULL && CsrBtCmElementCounterIncrement(cmData))
    { /* Make sure that the server instance (<server channel, context>) can accept
         only one connection. Store received parameters in the connection table,
         and start the connectable routine                                        */
        theElement                          = (cmRfcConnElement *) CsrCmnListElementAddLast(&(cmData->rfcVar.connList), sizeof(cmRfcConnElement));
        theElement->app_controlled          = FALSE;
        theElement->elementId               = cmData->elementCounter;
        connInst                            = theElement->cmRfcConnInst;

        cmData->rfcVar.activeElemId         = theElement->elementId;
        cmData->rfcVar.connectAcceptTimeOut = cmPrim->timeout;

        connInst->serverChannel             = cmPrim->serverChannel;
        connInst->profileMaxFrameSize       = cmPrim->profileMaxFrameSize;
        connInst->btConnId                  = CSR_BT_CONN_ID_INVALID;
        connInst->appHandle                 = cmPrim->appHandle;
        connInst->classOfDevice             = cmPrim->classOfDevice;
        connInst->context                   = cmPrim->context;
        connInst->modemStatus               = cmPrim->modemStatus;
        connInst->signalBreak               = cmPrim->breakSignal;
        connInst->mscTimeout                = cmPrim->mscTimeout;
        connInst->deviceAddr                = cmPrim->deviceAddr;

        CSR_BT_CM_STATE_CHANGE(connInst->state, CSR_BT_CM_RFC_STATE_CONNECTABLE);
        CmRfcRemoteConnectionStateHandler(cmData, theElement);

        /*
        Send a register request to SC for registering in the SM database.
        This is for incoming connection, i.e. when the device is server
        and activates a service. Use the profile uuid16_t and the allocated
        server channel                                                          */
        if (CsrBtCmIncomingSecRegisterDeregisterRequired(cmData, connInst->serverChannel))
        {
            CsrBtScRegisterReqSend(cmPrim->profileUuid,
                                   connInst->serverChannel,
                                   FALSE, /* Also applies for outgoing connections */
                                   SEC_PROTOCOL_RFCOMM,
                                   cmPrim->secLevel,
                                   cmPrim->minEncKeySize);
        }
    }
    else
    {
        if(theElement)
        { /* Already accepting a connection at this server instance (<server channel, context>) */
            csrBtCmAcceptConnectErrorHandler(cmData,
                                             cmPrim->appHandle,
                                             cmPrim->serverChannel,
                                             CSR_BT_RESULT_CODE_CM_ALREADY_CONNECTING,
                                             CSR_BT_SUPPLIER_CM, cmPrim->context);
        }
        else
        { /* Connection list full */
            csrBtCmAcceptConnectErrorHandler(cmData,
                                             cmPrim->appHandle,
                                             cmPrim->serverChannel,
                                             CSR_BT_RESULT_CODE_CM_INTERNAL_ERROR,
                                             CSR_BT_SUPPLIER_CM,
                                             cmPrim->context);
        }
    }
}

void CsrBtCmRfcConnectAcceptTimeoutHandler(cmInstanceData_t *cmData)
{
    CsrBtCmAcceptConnectTimeout *cmPrim = (CsrBtCmAcceptConnectTimeout *) cmData->recvMsgP;
    cmRfcConnElement *theElement = CsrBtCmRfcFindRfcConnElementFromServerState(cmData,
                                                                               cmPrim->serverChannel,
                                                                               CSR_BT_CM_RFC_STATE_CANCEL_TIMER);

    if (theElement)
    {
        cmData->rfcVar.activeElemId = theElement->elementId;
        theElement->cmRfcConnInst->btConnId = CSR_BT_CONN_ID_INVALID;
        theElement->cmRfcConnInst->pending = FALSE;
        CmRfcRemoteConnectionStateHandler(cmData, theElement);
    }
    else
    {/* Restore RFC queue                                                    */
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }
}

void CsrBtCmRfcCancelConnectAcceptReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmCancelAcceptConnectReq * cmPrim = (CsrBtCmCancelAcceptConnectReq *) cmData->recvMsgP;
    cmConnIdServerContextType  serverInst       = CsrBtCmReturnConnIdServerContextStruct(CSR_BT_CONN_ID_CANCELLED, cmPrim->serverChannel, cmPrim->context);
    cmRfcConnElement * theElement  = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromConnIdServerContext, &(serverInst));

    if(theElement)
    { /* The device is connectable. Remove COD and cancel the service */
        cmRfcConnInstType *theLogicalLink   = theElement->cmRfcConnInst;
        cmData->rfcVar.activeElemId         = theElement->elementId;
        theLogicalLink->btConnId            = CSR_BT_CONN_ID_INVALID;
        CSR_BT_CM_STATE_CHANGE(theLogicalLink->state,
                               CSR_BT_CM_RFC_STATE_CANCEL_CONNECTABLE);
        CmRfcRemoteConnectionStateHandler(cmData, theElement);
    }
    else
    { /* The CM can not cancel the service.Restore the RFC queue. */
        
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }
}
#endif /* #ifndef EXCLUDE_CSR_BT_RFC_MODULE */
