/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE

#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif

static void cmPendingL2caMsgFree(void *msg)
{
     L2CA_FreePrimitive(msg);
}

static CsrBool cmRejectPendingL2caAutoConnectInd(cmInstanceData_t *cmData, void *msg, void* pContext)
{
    CsrBool processed = FALSE;

    if (pContext)
    {
        CsrBtConnId btConnId;
        cmL2caConnElement* theElement;
        L2CA_AUTO_CONNECT_IND_T* prim;
        l2ca_cid_t cid = 0; /* cid is passed in context */

        cid        = *((l2ca_cid_t*)pContext);
        prim       = (L2CA_AUTO_CONNECT_IND_T*) msg;
        btConnId   = CM_CREATE_L2CA_CONN_ID(cid); 
        theElement = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromBtConnId, &(btConnId)); 

        if (CsrBtBdAddrEq(&prim->bd_addr, &theElement->cmL2caConnInst->deviceAddr) && 
            (prim->psm_local == theElement->cmL2caConnInst->psm))
        {
            L2CA_AutoConnectRsp(prim->identifier,
                                prim->cid,
                                L2CA_CONNECT_REJ_RESOURCES,
                                CM_L2CA_INCOMING_CONNECT_REJECTED_CTX,
                                0, /*conftab_length*/
                                NULL); /*conftab*/
            processed = TRUE;
        }
    }
    return(processed);
}

static void cmL2capDisconnectIndMsgSend(cmInstanceData_t     *cmData,
                                        cmL2caConnElement    *theElement,
                                        CsrBtReasonCode       reasonCode,
                                        CsrBtSupplier         reasonSupplier,
                                        CsrBool               localTerminated,
                                        l2ca_identifier_t     signalId)
{
    
    cmL2caConnInstType *l2CaConnection = theElement->cmL2caConnInst;
    CsrBtCmL2caDisconnectInd *prim  = (CsrBtCmL2caDisconnectInd *) CsrPmemAlloc(sizeof(CsrBtCmL2caDisconnectInd));

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    /* No need for CM to respond, as application will take care of it. */
    cmData->sendDiscRsp = FALSE;
#endif

    prim->type              = CSR_BT_CM_L2CA_DISCONNECT_IND;
    prim->btConnId          = l2CaConnection->btConnId;
    prim->reasonCode        = reasonCode;
    prim->reasonSupplier    = reasonSupplier;
    prim->localTerminated   = localTerminated;
    prim->context           = l2CaConnection->context;
    prim->l2caSignalId      = signalId;
    CsrBtCmPutMessage(l2CaConnection->appHandle, prim);

    if (reasonCode == CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER &&
        reasonSupplier == CSR_BT_SUPPLIER_CM)
    {
        ;
    }
    else
    {
        CsrUint8        numOfOutgoing;
        CsrUint8        numOfIncoming;
        CsrBtDeviceAddr deviceAddr;

        numberOfSecurityRegister(cmData, l2CaConnection->psm, l2CaConnection->deviceAddr, &numOfOutgoing, &numOfIncoming);

        if(l2CaConnection->remotePsm != NO_REMOTE_PSM)
        { /* The local device has initiate the connection, deregistered outgoing */
            if (numOfOutgoing == 1)
            { /* Unregister The Outgoing security setting */
                dm_sm_unregister_outgoing_req(CSR_BT_CM_IFACEQUEUE,
                                              0, /* context */
                                              &l2CaConnection->deviceAddr,
                                              (l2CaConnection->transportType == BREDR_ACL) ? SEC_PROTOCOL_L2CAP : SEC_PROTOCOL_LE_L2CAP,
                                              l2CaConnection->psm,
                                              NULL);
            }
            else
            { /* There is more that need this security setting */
                ;
            }
        }
        else
        { /* The remote device has initiate the connection, deregistered incoming */
            if (numOfIncoming == 1)
            { /* Unregister The Incoming security setting */
                CsrBtScDeregisterReqSend(
                    (l2CaConnection->transportType == BREDR_ACL) ? SEC_PROTOCOL_L2CAP : SEC_PROTOCOL_LE_L2CAP,
                    l2CaConnection->psm);
            }
            else
            { /* There is more that need this security setting */
                ;
            }
        }
        if (l2CaConnection->dataPriority != CSR_BT_CM_PRIORITY_NORMAL)
        {
            deviceAddr = l2CaConnection->deviceAddr;
        }
        else
        {
            CsrBtBdAddrZero(&deviceAddr);
        }
        CsrBtCmL2capClearL2capTableIndex(cmData, &(theElement->cmL2caConnInst));
    }
}

static void csrBtCmL2caDisconnectHandler(cmInstanceData_t   *cmData,
                                         CsrBool            localTerminated,
                                         l2ca_cid_t         cid,
                                         l2ca_identifier_t  signalId,
                                         CsrBtReasonCode    reason)
{
    CsrBtConnId btConnId            = CM_CREATE_L2CA_CONN_ID(cid);
    cmL2caConnElement * theElement  = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromBtConnId, &(btConnId));

    if (theElement)
    {
        cmL2caConnInstType *l2CaConnection = theElement->cmL2caConnInst;
        CsrBtDeviceAddr    remoteAddr      = l2CaConnection->deviceAddr;

        switch(l2CaConnection->state)
        {
            case CSR_BT_CM_L2CAP_STATE_CONNECTED:
            {
                /* The peer device has requested that the L2CAP is release or the CM
                 * has release it direct. Accept this request, clear the l2cap 
                 * connection table and notify the application, Note always 
                 * consider as a remote disconnect */
                CSR_BT_CM_STATE_CHANGE(l2CaConnection->state,
                                       CSR_BT_CM_L2CAP_STATE_IDLE);
                /* If a remote device is disconnecting A2DP L2CAP signaling connection,
                 * reject any pending A2DP media L2CAP connection from the same device */
                if (!localTerminated && (l2CaConnection->psm == CSR_BT_AVDTP_PSM))
                {
                    CmHandlePendingPrim(cmData,
                                        L2CAP_PRIM,
                                        L2CA_AUTO_CONNECT_IND,
                                        (void*)&cid, /* pContext */
                                        cmRejectPendingL2caAutoConnectInd,
                                        cmPendingL2caMsgFree);
                }
                cmL2capDisconnectIndMsgSend(cmData,
                                            theElement,
                                            reason,
                                            CSR_BT_SUPPLIER_L2CAP_DISCONNECT,
                                            FALSE,
                                            signalId);
                CsrBtCmWriteAutoFlushTimeout(cmData, &remoteAddr);
                break;
            }

            case CSR_BT_CM_L2CAP_STATE_RELEASE:
            {
                aclTable *aclConnectionElement;

                if (theElement->cmL2caConnInst->transportType == CSR_BT_TRANSPORT_BREDR &&
                    returnAclConnectionElement(cmData, theElement->cmL2caConnInst->deviceAddr, &aclConnectionElement) != CM_ERROR)
                {
                    /* For BREDR transport, respective ACL Connection still exists. Notify l2cap disconnection to
                     * device utility to set up required link policy. */
                    (void)CmDuHandleAutomaticProcedure(cmData,
                                                       CM_DU_AUTO_EVENT_SERVICE_DISCONNECTED,
                                                       NULL,
                                                       &aclConnectionElement->deviceAddr);
                }

                cmL2capDisconnectIndMsgSend(cmData,
                                            theElement,
                                            reason,
                                            CSR_BT_SUPPLIER_L2CAP_DISCONNECT,
                                            localTerminated,
                                            signalId);

                CsrBtCmServiceManagerLocalQueueHandler(cmData);

                if (localTerminated)
                {
                    CsrBtCmWriteAutoFlushTimeout(cmData, &remoteAddr);
                }
                break;
            }

            case CSR_BT_CM_L2CAP_STATE_CONNECT:
            {
                /* The  L2CAP is release while trying to establish it. Make sure that
                 * the local device accept SNIFF before notifying the
                 * application and cleaning up the l2cap connection table */
                aclTable *aclConnectionElement;

                if (theElement->cmL2caConnInst->transportType == CSR_BT_TRANSPORT_BREDR &&
                    returnAclConnectionElement(cmData, theElement->cmL2caConnInst->deviceAddr, &aclConnectionElement) != CM_ERROR)
                {
                    /* For BREDR transport, respective ACL Connection still exists. Notify l2cap disconnection to
                     * device utility to set up required link policy. */
                    (void)CmDuHandleAutomaticProcedure(cmData,
                                                       CM_DU_AUTO_EVENT_SERVICE_DISCONNECTED,
                                                       NULL,
                                                       &aclConnectionElement->deviceAddr);
                }
                CsrBtCmL2caConnectCfmMsgHandler(cmData,
                                                theElement,
                                                (cmData->l2caVar.cancelConnect ? CSR_BT_RESULT_CODE_CM_CANCELLED: reason),
                                                (cmData->l2caVar.cancelConnect ? CSR_BT_SUPPLIER_CM: CSR_BT_SUPPLIER_L2CAP_DISCONNECT));
                CsrBtCmServiceManagerLocalQueueHandler(cmData);
                break;
            }

            case CSR_BT_CM_L2CAP_STATE_CONNECT_ACCEPT:
            { /* The L2CAP connection is release while accepting a connection. 
                 Keep the link discoverable and restore the local service manager 
                 and DM queue */
                CsrBtCmL2capAcceptFailClearing(cmData, l2CaConnection);
                if (localTerminated)
                {
                    CsrBtCmWriteAutoFlushTimeout(cmData, &remoteAddr);
                }
                CsrBtCmServiceManagerLocalQueueHandler(cmData);
                break;
            }

            default:
                break;
        }
    }
}

void CsrBtCmL2caDisconnectReqHandler(cmInstanceData_t *cmData)
{ /* Request to disconnect L2CAP channel */
    CsrBtCmL2caDisconnectReq   *cmPrim = (CsrBtCmL2caDisconnectReq *) cmData->recvMsgP;
    cmL2caConnElement * l2caConnElement = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromBtConnId, &(cmPrim->btConnId));

    if (l2caConnElement)
    {
        cmL2caConnInstType *l2caConnection = l2caConnElement->cmL2caConnInst;
        cmData->l2caVar.activeElemId       = l2caConnElement->elementId;

        if (l2caConnection->state == CSR_BT_CM_L2CAP_STATE_CONNECTED)
        {
            /* This means the mode change is not required and hence we can conitnue with the disconnection process. */
            CSR_BT_CM_STATE_CHANGE(l2caConnection->state, CSR_BT_CM_L2CAP_STATE_RELEASE);
            /* Release the l2cap connection on restoring the local DM queue */
            L2CA_DisconnectReq(CM_GET_UINT16ID_FROM_BTCONN_ID(l2caConnection->btConnId));
            /* Since we are moving ahead with the disconnection process, SM will be unlocked once the process is over. */
            return;
        }
        else
        { /* The profile try to disconnect a connection that is not up and running.
             Build and send CSR_BT_CM_L2CA_DISCONNECT_IND with ERROR, and restore the
             local service manager queue */
            l2caConnElement->cmL2caConnInst->context = cmPrim->context;
            cmL2capDisconnectIndMsgSend(cmData,
                                        l2caConnElement,
                                        (CsrBtReasonCode) CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER,
                                        CSR_BT_SUPPLIER_CM,
                                        TRUE,
                                        0); /* Signal ID is only used for sending response */
        }
    }

    CsrBtCmServiceManagerLocalQueueHandler(cmData);
}

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
void CsrBtCmL2caDisconnectRspHandler(cmInstanceData_t *cmData)
{
    CsrBtCmL2caDisconnectRsp *rsp = (CsrBtCmL2caDisconnectRsp*) cmData->recvMsgP;

#ifdef CSR_STREAMS_ENABLE
    /* A valid sink indicates that the confirmation is required
     * in order to delete the related streams. */
    if (SinkIsValid(StreamL2capSink(CM_GET_UINT16ID_FROM_BTCONN_ID(rsp->btConnId))))
#endif /* CSR_STREAMS_ENABLE */
    {
        L2CA_DisconnectRsp(rsp->identifier, CM_GET_UINT16ID_FROM_BTCONN_ID(rsp->btConnId));
    }
}
#endif

void CsrBtCmL2caDisconnectCfmHandler(cmInstanceData_t *cmData)
{ /* Confirmation of request to disconnect L2CAP channel, and restore the local
     L2CAP queue */
    L2CA_DISCONNECT_CFM_T   * prim = (L2CA_DISCONNECT_CFM_T *) cmData->recvMsgP;
    csrBtCmL2caDisconnectHandler(cmData,
                                 TRUE,
                                 prim->cid,
                                 0,
                                 (CsrBtReasonCode) prim->result);

#ifdef CSR_TARGET_PRODUCT_VM
    if (prim->result == L2CA_DISCONNECT_LINK_TRANSFERRED && cmData->recvMsgP)
    {
        /* ULCONV_TODO: This is a dummy indication formulated by CM while handling DM_BAD_MESSAGE_IND.
         * This must be released as normal Synergy primitive rather than as Bluestack primitive. */
        SynergyMessageFree(L2CAP_PRIM, cmData->recvMsgP);
        cmData->recvMsgP = NULL;
    }
#endif
}

void CsrBtCmL2caDisconnectIndHandler(cmInstanceData_t *cmData)
{ /* Indication of request to disconnect L2CAP channel. Send a Respond back
     to accept this demand, and inform the application */
    L2CA_DISCONNECT_IND_T * prim = (L2CA_DISCONNECT_IND_T *) cmData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    /* Default behavior is for CM to send response, unless disconnect is
     * notified to application where it takes care of sending response. */
    cmData->sendDiscRsp = TRUE;
#endif
    csrBtCmL2caDisconnectHandler(cmData,
                                 FALSE,
                                 prim->cid,
                                 prim->identifier,
                                 (CsrBtReasonCode) prim->reason);

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    /* CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP disables automatic response for
     * disconnect indication from Synergy CM when disconnect indication is sent to
     * application. Check if CM still needs to respond to Disconnect Indication,
     * for the cases when application is not notified. */
    if (cmData->sendDiscRsp)
#endif
    {
        if (cmData->recvMsgP)
        { /* The CM has not save the L2CA_DISCONNECT_IND message. Accept the release
            request initiated from a peer device right away                            */
            L2CA_DisconnectRsp(prim->identifier, prim->cid);
        }
        else
        { /* The CM has save L2CA_DISCONNECT_IND. Wait to send the response message     */
            ;
        }
    }
}

#endif /* #ifndef EXCLUDE_CSR_BT_L2CA_MODULE */
