/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_RFC_MODULE

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_sdc.h"
#include "csr_bt_cm_util.h"

static void csrBtCmRfcReleaseWhileConnected(cmInstanceData_t *cmData,
                                            cmRfcConnElement *theElement, 
                                            RFC_RESPONSE_T    reason,
                                            CsrBtSupplier     resultSupplier)
{ /* The remote device has release the connection. Inform the
     application and check if another link policy is demand                         */
    cmRfcConnInstType   *theLogicalLink = theElement->cmRfcConnInst;
    CSR_BT_CM_STATE_CHANGE(theLogicalLink->state, CSR_BT_CM_RFC_STATE_IDLE);
    CsrBtCmDmSyncClearPcmSlotFromTable(cmData, theLogicalLink->eScoParms);
    CsrBtCmDisconnectIndMsgCleanupSend(cmData,
                                       theElement,
                                       (CsrBtReasonCode) reason,
                                       resultSupplier,
                                       TRUE,
                                       FALSE);
}

static void csrBtCmRfcReleaseWhileRelease(cmInstanceData_t  *cmData,
                                          cmRfcConnElement  *theElement,
                                          RFC_RESPONSE_T     reason,
                                          CsrBtSupplier      resultSupplier)
{ /* The local device has release the RFC connection.                               */
    cmRfcConnInstType   *theLogicalLink = theElement->cmRfcConnInst;
    CsrUint8    numOfConnection         = CsrBtCmReturnNumOfConnectionsToPeerDevice(cmData, theLogicalLink->deviceAddr);
    CsrBool status = CsrBtCmRfcReleaseStatus((CsrBtReasonCode) reason, resultSupplier);

    CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "Status %d", status));

    if (numOfConnection > 0)
    {
        /* There are more service connections on the link, notify this disconnection to
         * device utility in order to set the correct link policy.*/
        (void)CmDuHandleAutomaticProcedure(cmData,
                                           CM_DU_AUTO_EVENT_SERVICE_DISCONNECTED,
                                           NULL,
                                           &theLogicalLink->deviceAddr);
    }

    CsrBtCmDmSyncClearPcmSlotFromTable(cmData, theLogicalLink->eScoParms);
    CsrBtCmDisconnectIndMsgCleanupSend(cmData,
                                       theElement,
                                       (CsrBtReasonCode) reason,
                                       resultSupplier,
                                       status,
                                       TRUE);
    CsrBtCmServiceManagerLocalQueueHandler(cmData);
}


static void csrBtCmDisconnectIndMsgSend(CsrSchedQid          appHandle,
                                        CsrBtConnId          btConnId,
                                        CsrBtReasonCode      reasonCode,
                                        CsrBtSupplier        reasonSupplier,
                                        CsrBool              status,
                                        CsrBool              localTerminated,
                                        CsrUint16            context)
{/* Send a CSR_BT_CM_DISCONNECT_IND signal to the application                              */
    CsrBtCmDisconnectInd     * cmPrim;
    cmPrim                   = (CsrBtCmDisconnectInd *)CsrPmemAlloc(sizeof(CsrBtCmDisconnectInd));
    cmPrim->type             = CSR_BT_CM_DISCONNECT_IND;
    cmPrim->btConnId         = btConnId;
    cmPrim->reasonCode       = reasonCode;
    cmPrim->reasonSupplier   = reasonSupplier;
    cmPrim->status           = status;
    cmPrim->localTerminated  = localTerminated;
    cmPrim->context          = context;
    CsrBtCmPutMessage(appHandle, cmPrim);
}

CsrBool CsrBtCmRfcReleaseStatus(CsrBtReasonCode     reasonCode,
                               CsrBtSupplier reasonSupplier)
{

    if (reasonSupplier == CSR_BT_SUPPLIER_RFCOMM)
    {
        RFC_RESPONSE_T reason = (RFC_RESPONSE_T) reasonCode;

        if (reason == RFC_RES_ACK_TIMEOUT)
        {
            return FALSE;
        }
    }
    return TRUE;
}

void CsrBtCmDisconnectIndMsgCleanupSend(cmInstanceData_t    *cmData,
                                        cmRfcConnElement    *theElement,
                                        CsrBtReasonCode      reasonCode,
                                        CsrBtSupplier        reasonSupplier,
                                        CsrBool              status,
                                        CsrBool              localTerminated)
{/* Send a CSR_BT_CM_DISCONNECT_IND signal to the application, and clean up the
    connection table                                                                */
    cmRfcConnInstType * theLogicalLink = theElement->cmRfcConnInst;

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    if (!localTerminated)
    {
        /* No need for CM to respond, as application will take care of it. */
        cmData->sendDiscRsp = FALSE;
    }
#endif

    csrBtCmDisconnectIndMsgSend(theLogicalLink->appHandle,
                                theLogicalLink->btConnId,
                                reasonCode,
                                reasonSupplier,
                                status,
                                localTerminated,
                                theLogicalLink->context);

    if (status)
    {
        if(theLogicalLink->remoteServerChan != CSR_BT_NO_SERVER)
        { /* The local device has initiate the connection, deregistered outgoing    */
            dm_sm_unregister_outgoing_req(CSR_BT_CM_IFACEQUEUE,
                                          0, /* context */
                                          &theLogicalLink->deviceAddr,
                                          SEC_PROTOCOL_RFCOMM,
                                          theLogicalLink->serverChannel,
                                          NULL);
        }
        else
        { /* The remote device has initiate the connection, deregistered incoming   */
            if (CsrBtCmIncomingSecRegisterDeregisterRequired(cmData, theLogicalLink->serverChannel))
            {
                CsrBtScDeregisterReqSend(SEC_PROTOCOL_RFCOMM, theLogicalLink->serverChannel);
            }
        }
        cleanUpConnectionTable(&(theElement->cmRfcConnInst));
    }
    else
    {
        if (theLogicalLink->state == CSR_BT_CM_RFC_STATE_RELEASE)
        {
            CSR_BT_CM_STATE_CHANGE(theLogicalLink->state,
                                   CSR_BT_CM_RFC_STATE_CONNECTED);
        }
    }
}

void CsrBtCmRfcReleaseReqHandler(cmInstanceData_t *cmData)
{ /* The application wishes to close down the data link connection                  */
    CsrBtCmDisconnectReq * cmPrim  = (CsrBtCmDisconnectReq *) cmData->recvMsgP;
    cmRfcConnElement * theElement    = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromBtConnId, &(cmPrim->btConnId));

    if (theElement)
    { /* The CM has found a valid index in the connection table                     */
        cmRfcConnInstType *theLogicalLink   = theElement->cmRfcConnInst;

        if (theLogicalLink->state == CSR_BT_CM_RFC_STATE_CONNECTED)
        {
            cmData->rfcVar.activeElemId = theElement->elementId;
            CSR_BT_CM_STATE_CHANGE(theLogicalLink->state, CSR_BT_CM_RFC_STATE_RELEASE);

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
            if (theLogicalLink->eScoParms &&
                theLogicalLink->eScoParms->handle < SCOBUSY_ACCEPT)
            {   /* Before closing down the RFC connection the SCO connection
                has to be released. */
                if (!theElement->cmRfcConnInst->eScoParms->closeSco)
                {
                    cmData->dmVar.rfcConnIndex = theElement->elementId;
                    theElement->cmRfcConnInst->eScoParms->closeSco = TRUE;
                    dm_sync_disconnect_req(theLogicalLink->eScoParms->handle,
                                           HCI_ERROR_OETC_USER);

                    /* Wait for the SCO to disconnect.*/
                    return;
                }
            }
            else
#endif /* !EXCLUDE_CSR_BT_SCO_MODULE */
            { /* There is no SCO connection, just close down the RFC
                 connection. The call is confirmed by RFC_RELEASE_IND. */
                 RfcDisconnectReqSend((CsrUint16)theLogicalLink->btConnId);

                 /* Wait for the RFC to disconnect.*/
                 return;
            }
        }
        else
        { /* The profile try to disconnect a connection that is not up running.
             Build and send CSR_BT_CM_DISCONNECT_IND with ERROR                            */
            csrBtCmDisconnectIndMsgSend(theLogicalLink->appHandle,
                                        cmPrim->btConnId,
                                        (CsrBtReasonCode) CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER,
                                        CSR_BT_SUPPLIER_CM,
                                        TRUE,
                                        TRUE,
                                        cmPrim->context);
        }
    }

    CsrBtCmServiceManagerLocalQueueHandler(cmData);
}

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
void CsrBtCmRfcDisconnectRspHandler(cmInstanceData_t *cmData)
{
    CsrBtCmRfcDisconnectRsp *rsp = (CsrBtCmRfcDisconnectRsp*) cmData->recvMsgP;

#ifdef CSR_STREAMS_ENABLE
    /* A valid sink indicates that the confirmation is required
     * in order to delete the related streams. */
    if (SinkIsValid(StreamRfcommSink(CM_GET_UINT16ID_FROM_BTCONN_ID(rsp->btConnId))))
#endif /* CSR_STREAMS_ENABLE */
    {
        RfcDisconnectRspSend(CM_GET_UINT16ID_FROM_BTCONN_ID(rsp->btConnId));
    }
}
#endif

static void csrBtCmRfcCommonReleaseConnectSHandler(cmInstanceData_t     *cmData,
                                                   cmRfcConnInstType    *theLogicalLink,
                                                   RFC_RESPONSE_T        reason,
                                                   CsrBtSupplier         resultSupplier)
{
    if (cmData->rfcVar.connectState == CM_RFC_SSP_REPAIR)
    {
        CsrBtCmSmCancelSppRepairInd(cmData);
    }

    if (cmData->rfcVar.cancelConnect)
    { /* The rfcomm connection has been release because the application
         has issued a cancel connect                                                */
        cmData->smVar.arg.result.code     = CSR_BT_RESULT_CODE_CM_CANCELLED;
        cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        cmData->smVar.arg.result.code     = (CsrBtResultCode) reason;
        cmData->smVar.arg.result.supplier = resultSupplier;
    }
    CsrBtCmRfcCommonErrorHandler(cmData, theLogicalLink);
}

void CsrBtCmRfcReleaseIndHandler(cmInstanceData_t *cmData)
{ /* This event is an indication that the data link connection referenced
     by the connection id has closed down.                                          */
    RFC_DISCONNECT_IND_T   * rfcPrim = (RFC_DISCONNECT_IND_T *) cmData->recvMsgP;
    cmRfcConnElement * theElement;
    CsrBtSupplier resultSupplier;
    CsrBtConnId        btConnId = CM_CREATE_RFC_CONN_ID(rfcPrim->conn_id);
    theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromBtConnId, &(btConnId));
    /* Keep the result supplier as CSR_BT_SUPPLIER_RFCOMM if reason is RFC_SUCCESS or greater than equal to RFCOMM_ERRORCODE_BASE */
    resultSupplier = (rfcPrim->reason < RFCOMM_ERRORCODE_BASE && rfcPrim->reason != RFC_SUCCESS) ? CSR_BT_SUPPLIER_L2CAP_DISCONNECT : CSR_BT_SUPPLIER_RFCOMM;

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    /* Default behavior is for CM to send response, unless disconnect is
     * notified to application where it takes care of sending response. */
    cmData->sendDiscRsp = TRUE;
#endif

    if(theElement)
    { /* The CM has found a valid index in the connection table                     */
        cmRfcConnInstType *theLogicalLink   = theElement->cmRfcConnInst;

        switch(theLogicalLink->state)
        {
            case CSR_BT_CM_RFC_STATE_CONNECTED:
            { /* The remote device has release the connection. Inform the
                 application and check if another link policy is demand             */
                csrBtCmRfcReleaseWhileConnected(cmData, theElement, rfcPrim->reason, resultSupplier);
                break;
            }

            case CSR_BT_CM_RFC_STATE_RELEASE:
            { /* The local device has release the RFC connection.                   */
                csrBtCmRfcReleaseWhileRelease(cmData, theElement, rfcPrim->reason, resultSupplier);
                break;
            }

            case CSR_BT_CM_RFC_STATE_CONNECT:
            { /* Connection is released while an outgoing attempt was in progress, inform this to caller.*/
                csrBtCmRfcCommonReleaseConnectSHandler(cmData,
                                                       theLogicalLink,
                                                       rfcPrim->reason,
                                                       resultSupplier);
                break;
            }
            case CSR_BT_CM_RFC_STATE_CONNECT_ACCEPT:
            { /* The connection is release while accepting a connection             */
                cmData->smVar.arg.result.code      = (CsrBtResultCode) rfcPrim->reason;
                cmData->smVar.arg.result.supplier  = resultSupplier;
                CmRfcRemoteConnectionStateHandler(cmData, theElement);
                break;
            }

            default:
            {
                break;
            }
        }
    }
    else
    {
        /*This may be because the remote issued RFcomm connect confirmation with status pending and 
        while connection is processing received RFcomm disconnetion from remote device*/
        theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromBtPendingConnId, &(btConnId));
        if (theElement)
        {
            cmRfcConnInstType *theLogicalLink = theElement->cmRfcConnInst;
            switch(theLogicalLink->state)
            {
                case CSR_BT_CM_RFC_STATE_CONNECT:
                { /* Build and send CSR_BT_CM_CONNECT_CFM to the profile.
                  The connection is release while connecting */
                    theLogicalLink->pending = FALSE;
                    csrBtCmRfcCommonReleaseConnectSHandler(cmData, theLogicalLink, rfcPrim->reason, resultSupplier);
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    /* RFC_DISCONNECT_IND requires response which deletes rfcomm streams in bluestack. */
    if (rfcPrim->type == RFC_DISCONNECT_IND)
    {
        /* CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP disables automatic response for
         * disconnect indication from Synergy CM when disconnect indication is sent to
         * application. Check if CM still needs to respond to Disconnect Indication,
         * for the cases when application is not notified. */
        if (cmData->sendDiscRsp)
        {
            RfcDisconnectRspSend(rfcPrim->conn_id);
        }
    }
#endif
}

#endif /* #ifndef EXCLUDE_CSR_BT_RFC_MODULE */
