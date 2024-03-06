/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_sdc.h"

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
#include "csr_bt_cm_bnep.h"
#endif

#include "csr_bt_cm_util.h"
#include "csr_bt_cm_le.h"

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
static CsrBool csrBtCmCmRfcRemoveUnusedRfcConnElement(CsrCmnListElm_t *elem, void *data)
{
    cmRfcConnElement * theElement = (cmRfcConnElement *) elem;

    CSR_UNUSED(data);

    if (theElement->cmRfcConnInst == NULL)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

#endif /* EXCLUDE_CSR_BT_RFC_MODULE  */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
static CsrBool csrBtCmCmL2caRemoveUnusedL2caConnElement(CsrCmnListElm_t *elem, void *data)
{
    cmL2caConnElement * theElement = (cmL2caConnElement *) elem;

    CSR_UNUSED(data);

    if (theElement->cmL2caConnInst == NULL)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE  */

static const SignalHandlerType serviceManagerProvideHandler[CSR_BT_CM_SM_PRIM_DOWNSTREAM_COUNT] =
{
    CsrBtCmRfcRegisterReqHandler,                /* CSR_BT_CM_REGISTER_REQ */
    CsrBtCmRfcConnectReqHandler,                 /* CSR_BT_CM_CONNECT_REQ */
    CsrBtCmRfcAcceptConnectReqHandler,           /* CSR_BT_CM_CONNECT_ACCEPT_REQ */
    CsrBtCmRfcCancelConnectAcceptReqHandler,     /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_REQ */
    CsrBtCmRfcReleaseReqHandler,                 /* CSR_BT_CM_DISCONNECT_REQ */
    CsrBtCmRfcConnectAcceptTimeoutHandler,       /* CSR_BT_CM_ACCEPT_CONNECT_TIMEOUT */
    NULL,                                        /* CSR_BT_CM_RFC_MODE_CHANGE_REQ_DEP */
    CsrBtCmRfcConnectReqExtHandler,              /* CSR_BT_CM_CONNECT_EXT_REQ */
    CsrBtCmL2caRegisterReqHandler,               /* CSR_BT_CM_L2CA_REGISTER_REQ */
    CsrBtCml2caConnectReqHandler,                /* CSR_BT_CM_L2CA_CONNECT_REQ */
    CsrBtCml2caConnectAcceptReqHandler,          /* CSR_BT_CM_L2CA_CONNECT_ACCEPT_REQ */
    CsrBtCml2caCancelAcceptConnectReqHandler,    /* CSR_BT_CM_L2CA_CANCEL_CONNECT_ACCEPT_REQ */
    CsrBtCmL2caDisconnectReqHandler,             /* CSR_BT_CM_L2CA_DISCONNECT_REQ */
    NULL,                                        /* CSR_BT_CM_L2CA_MODE_CHANGE_REQ_DEP */
#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
    CsrBtCmBnepRegisterReqHandler,               /* CSR_BT_CM_BNEP_REGISTER_REQ */
    CsrBtCmBnepConnectReqHandler,                /* CSR_BT_CM_BNEP_CONNECT_REQ */
    CsrBtCmBnepConnectAcceptReqHandler,          /* CSR_BT_CM_BNEP_CONNECT_ACCEPT_REQ */
    CsrBtCmBnepCancelConnectAcceptReqHandler,    /* CSR_BT_CM_BNEP_CANCEL_CONNECT_ACCEPT_REQ */
    NULL,                                        /* CSR_BT_CM_BNEP_MODE_CHANGE_REQ */
    CsrBtCmBnepDisconnectReqHandler,             /* CSR_BT_CM_BNEP_DISCONNECT_REQ */
#else
    NULL,                                        /* CSR_BT_CM_BNEP_REGISTER_REQ */
    NULL,                                        /* CSR_BT_CM_BNEP_CONNECT_REQ */
    NULL,                                        /* CSR_BT_CM_BNEP_CONNECT_ACCEPT_REQ */
    NULL,                                        /* CSR_BT_CM_BNEP_CANCEL_CONNECT_ACCEPT_REQ */
    NULL,                                        /* CSR_BT_CM_BNEP_MODE_CHANGE_REQ */
    NULL,                                        /* CSR_BT_CM_BNEP_DISCONNECT_REQ */
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */
    CsrBtCmSdsRegisterReqHandler,                /* CSR_BT_CM_SDS_REGISTER_REQ */
    CsrBtCmSdsUnRegisterReqHandler,              /* CSR_BT_CM_SDS_UNREGISTER_REQ */
    CsrBtCmModeChangeReqHandler,                 /* CSR_BT_CM_MODE_CHANGE_REQ*/
    CsrBtCmLeLockSmQueueHandler,                 /* CSR_BT_CM_LE_LOCK_SM_QUEUE_REQ */
    CsrBtCmLePhysicalLinkStatusReqHandler,       /* CSR_BT_CM_LE_PHYSICAL_LINK_STATUS_REQ */
    Cml2caTpConnectReqHandler,                   /* CM_L2CA_TP_CONNECT_REQ */
    CmL2caRegisterFixedCidReqHandler,            /* CM_L2CA_REGISTER_FIXED_CID_REQ */
};

static void csrBtCmServiceManagerSignalHandler(cmInstanceData_t *cmData)
{
    CsrBtCmPrim         *primPtr;

    primPtr = (CsrBtCmPrim *) cmData->recvMsgP;

    if(((*primPtr - CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST) < CSR_BT_CM_SM_PRIM_DOWNSTREAM_COUNT) &&
       serviceManagerProvideHandler[*primPtr - CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST] != NULL)
    {
        CsrBtCmSmLockQueue(cmData);
        serviceManagerProvideHandler[*primPtr - CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST](cmData);
    }
    else
    {/* Event ERROR! */
        CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                *primPtr,
                                cmData->globalState,
                                "");
    }
}

void CsrBtCmServiceManagerProvider(cmInstanceData_t *cmData)
{
    if(cmData->smVar.smInProgress)
    {/* Need to save signal, because we are waiting for a Service Manager
        complete signal */
        CsrMessageQueuePush(&cmData->smVar.saveQueue, CSR_BT_CM_PRIM, cmData->recvMsgP);
        cmData->recvMsgP = NULL;
    }
    else
    {/* The Service Manager is ready just proceed */
        csrBtCmServiceManagerSignalHandler(cmData);
    }
}

void CsrBtCmServiceManagerCancelAcceptConnectProvider(cmInstanceData_t *cmData)
{
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    CsrBtResultCode result;
    CsrBtCmCancelAcceptConnectReq  *reqPrim = (CsrBtCmCancelAcceptConnectReq *) cmData->recvMsgP;
    CsrBtCmCancelAcceptConnectCfm  *cfmPrim;
    cmConnIdServerContextType  serverInst = CsrBtCmReturnConnIdServerContextStruct(CSR_BT_CONN_ID_INVALID, reqPrim->serverChannel, reqPrim->context);
    cmRfcConnElement * rfcElement  = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromConnIdServerContext, &serverInst);

    if (rfcElement)
    {
        /* connect_accept_req handled but not yet connected, mark as cancelled and process it */
        rfcElement->cmRfcConnInst->btConnId = CSR_BT_CONN_ID_CANCELLED;

        CsrBtCmServiceManagerProvider(cmData);

        result = CSR_BT_RESULT_CODE_CM_SUCCESS;
    }
    else
    {
        /* either already connected or connect_accept_req not handled yet (on q) */
        if( cmData->smVar.smInProgress
            && CsrBtCmRemoveSavedOutgoingConnectMessage(cmData, CSR_BT_CM_CONNECT_ACCEPT_REQ, reqPrim->serverChannel, reqPrim->context) )
        {
            /* currently busy with something else, found and removed the accept connect req on q */ 
            result = CSR_BT_RESULT_CODE_CM_SUCCESS;
        }
        else
        {
            /* must be already connected or we have never received a connect accept req for the channel, reject */
            result = CSR_BT_RESULT_CODE_CM_NOTHING_TO_CANCEL;
        }
    }
    
    /* return confirmation to app */
    cfmPrim = (CsrBtCmCancelAcceptConnectCfm *)CsrPmemAlloc(sizeof(CsrBtCmCancelAcceptConnectCfm));
    cfmPrim->type            = CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM;
    cfmPrim->serverChannel   = reqPrim->serverChannel;
    cfmPrim->resultCode      = result;
    cfmPrim->resultSupplier  = CSR_BT_SUPPLIER_CM;
    cfmPrim->context         = reqPrim->context;
    CsrBtCmPutMessage(reqPrim->phandle, cfmPrim);
#else
    CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                            CSR_BT_CM_CANCEL_ACCEPT_CONNECT_REQ,
                            cmData->globalState,
                            "");
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
}

void CsrBtCmServiceManagerL2caConnectAcceptProvider(cmInstanceData_t *cmData)
{
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    CsrBtCmL2caConnectAcceptReq  *prim = (CsrBtCmL2caConnectAcceptReq *) cmData->recvMsgP;

    if(prim->primaryAcceptor)
    {
        CsrBtCmServiceManagerProvider(cmData);
    }
    else
    {
        CsrBtCml2caConnectAcceptReqHandler(cmData);

    }
#else
     CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                             CSR_BT_CM_L2CA_CONNECT_ACCEPT_REQ,
                             cmData->globalState,
                             "");
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */
}

void CsrBtCmServiceManagerL2caCancelConnectAcceptProvider(cmInstanceData_t *cmData)
{
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    CsrBtResultCode result;
    CsrBtCancelConnectAcceptContext varPsmContext;
    CsrBtCmL2caCancelConnectAcceptReq  *prim = (CsrBtCmL2caCancelConnectAcceptReq *) cmData->recvMsgP;
    cmL2caConnElement *theElement;

    varPsmContext.context = prim->context;
    varPsmContext.psm = prim->localPsm;
    theElement = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromReserveBtConnIdPsmContext, &varPsmContext);

    if (theElement)
    {   /* connect_accept_req handled but not yet connected, mark as cancelled and process it */
        theElement->cmL2caConnInst->btConnId = CSR_BT_CONN_ID_CANCELLED;

        CsrBtCmServiceManagerProvider(cmData);

        result = CSR_BT_RESULT_CODE_CM_SUCCESS;
    }
    else
    {
        /* either already connected or connect_accept_req not handled yet (on q) */
        if( cmData->smVar.smInProgress
            && CsrBtCmRemoveSavedOutgoingConnectMessage(cmData, CSR_BT_CM_L2CA_CONNECT_ACCEPT_REQ, prim->localPsm, prim->context) )
        {
            /* currently busy with something else, found and removed the accept connect req on q */ 
            result = CSR_BT_RESULT_CODE_CM_SUCCESS;
        }
        else
        {
            /* must be already connected or we have never received a connect accept req for the PSM, reject */
            result = CSR_BT_RESULT_CODE_CM_NOTHING_TO_CANCEL;
        }
    }

    CsrBtCmL2caCancelConnectAcceptCfmMsgSend(cmData, prim->phandle, prim->localPsm, result, CSR_BT_SUPPLIER_CM, prim->context);
                                        
#else
     CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                             CSR_BT_CM_L2CA_CANCEL_CONNECT_ACCEPT_REQ,
                             cmData->globalState,
                             "");
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */
}

void CsrBtCmServiceManagerLocalQueueHandler(cmInstanceData_t *cmData)
{
    CsrBtCmPrim *prim = CsrPmemAlloc(sizeof(CsrBtCmPrim));

    *prim = CSR_BT_CM_SM_HOUSE_CLEANING;
    CsrBtCmPutMessage(CSR_BT_CM_IFACEQUEUE, prim);

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    cmData->rfcVar.activeElemId  = CM_ERROR;
    CsrCmnListIterateAllowRemove(&cmData->rfcVar.connList, csrBtCmCmRfcRemoveUnusedRfcConnElement, NULL);
#endif /* EXCLUDE_CSR_BT_RFC_MODULE     */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    cmData->l2caVar.activeElemId = CM_ERROR;
    CsrCmnListIterateAllowRemove(&cmData->l2caVar.connList, csrBtCmCmL2caRemoveUnusedL2caConnElement, NULL);
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE    */
}

#ifdef CSR_TARGET_PRODUCT_VM
/* Prioratise L2CAP connect request if ACL is established by remote device. */
static CsrBool cmPendingL2CAPConnectMessage(cmInstanceData_t *cmData, void **message)
{
    CsrUint16                eventClass;
    void                    *msg;
    CsrMessageQueueType     *tempQueue = NULL;
    CsrBool                  found = FALSE;

    while (CsrMessageQueuePop(&cmData->smVar.saveQueue, &eventClass, &msg))
    {
        CsrBtCmPrim primType = *((CsrBtCmPrim *) msg);
        if (!found)
        {
            if ((eventClass == CSR_BT_CM_PRIM) && (primType == CSR_BT_CM_L2CA_CONNECT_REQ))
            {
                CsrBtCmL2caConnectReq *prim = (CsrBtCmL2caConnectReq *) msg;
                aclTable *aclConnectionElement;
                returnAclConnectionElement(cmData, prim->addr, &aclConnectionElement);
                if ((aclConnectionElement && aclConnectionElement->incoming) &&
                    (CmReturnNumOfL2CRFCToPeerDevice(cmData, prim->addr) == 0))
                {
                    found = TRUE;
                    *message = msg;
                }
                else
                {
                    CsrMessageQueuePush(&tempQueue, eventClass, msg);
                }
                
            }
            else
            {
                 CsrMessageQueuePush(&tempQueue, eventClass, msg);
            }
        }
        else
        {
            CsrMessageQueuePush(&tempQueue, eventClass, msg);
        }
    }
    cmData->smVar.saveQueue = tempQueue;

    return found;
}
#endif

void CsrBtCmRestoreServiceManagerQueue(cmInstanceData_t *cmData)
{
    CsrUint16          eventClass = 0;
    CsrPrim      *msg = NULL;

    CsrBtCmSmUnlockQueue(cmData);
#ifdef CSR_TARGET_PRODUCT_VM
    /* Check if L2CAP connect needs to be prioratised if ACL is established by remote device. */
    if (cmPendingL2CAPConnectMessage(cmData, (void *) &msg))
    {
        /* Valid l2cap connection pending */
        eventClass = CSR_BT_CM_PRIM;
    }
    else
#endif
    {
        CsrMessageQueuePop(&cmData->smVar.saveQueue, &eventClass, (void *) &msg);
    }

    if (msg)
    {
        SynergyMessageFree(CSR_BT_CM_PRIM, cmData->recvMsgP);
        cmData->recvMsgP = msg;
        switch(eventClass)
        {
            case CSR_BT_CM_PRIM:
                {
                    csrBtCmServiceManagerSignalHandler(cmData);
                    break;
                }
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
            case RFCOMM_PRIM :
                {
                    cmData->smVar.popFromSaveQueue = TRUE;
                    CsrBtCmRfcArrivalHandler(cmData);
                    cmData->smVar.popFromSaveQueue = FALSE;
                    break;
                }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
            case L2CAP_PRIM :
                {
                    cmData->smVar.popFromSaveQueue = TRUE;
                    CsrBtCmL2CaArrivalHandler(cmData);
                    cmData->smVar.popFromSaveQueue = FALSE;
                    break;
                }
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */
#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
            case CSR_BT_BNEP_PRIM :
                {
                    CsrBtCmBnepArrivalHandler(cmData);
                    break;
                }
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */

            default :
                {
                    CsrBtCmGeneralException(eventClass,
                                            *msg,
                                            cmData->globalState,
                                            "");
                    break;
                }
        }
    }
    else
    {
        if ((cmData->dmVar.inquiryAppState == CM_INQUIRY_APP_STATE_RESTARTING) &&
            (cmData->dmVar.inquiryDmState == CM_INQUIRY_DM_STATE_IDLE))
        {
            CsrBtCmStartInquiry(cmData);
        }
    }
}

CsrBool cancelServiceManagerMsg(cmInstanceData_t *cmData,
                               CsrBtCmPrim type,
                               CsrSchedQid phandle,
                               CsrBtDeviceAddr bd_addr,
                               CsrUint8 serverCh,
                               psm_t localPsm,
                               CsrUint8 *sdcServer,
                               CsrUint16     *context)
{
    CsrUint16                eventClass;
    void                    *msg;
    CsrBool                  cancelMsg   = FALSE;
    CsrMessageQueueType    *tempQueue  = NULL;
    *sdcServer                          = CSR_BT_NO_SERVER;

    while (CsrMessageQueuePop(&cmData->smVar.saveQueue, &eventClass, &msg))
    {
        if (!cancelMsg &&
            eventClass == CSR_BT_CM_PRIM &&
            type == *(CsrBtCmPrim *) msg)
        {
            switch (type)
            {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
                case CSR_BT_CM_CONNECT_REQ:
                {
                    CsrBtCmConnectReq * prim = (CsrBtCmConnectReq *) msg;

                    if (phandle == prim->appHandle && serverCh == prim->localServerCh &&
                        CsrBtBdAddrEq(&(prim->deviceAddr), &(bd_addr)))
                    {
                        *context  = prim->context;
                        cancelMsg = TRUE;
                    }

                    break;
                }
                case CSR_BT_CM_CONNECT_EXT_REQ:
                {
                    CsrBtCmConnectExtReq * prim = (CsrBtCmConnectExtReq *) msg;

                    if (phandle == prim->appHandle && serverCh == prim->localServerCh &&
                        CsrBtBdAddrEq(&(prim->deviceAddr), &(bd_addr)))
                    {
                        cancelMsg = TRUE;
                    }
                    break;
                }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
                case CSR_BT_CM_L2CA_CONNECT_REQ:
                {
                    CsrBtCmL2caConnectReq * prim = (CsrBtCmL2caConnectReq *) msg;

                    if (phandle == prim->phandle && localPsm == prim->localPsm &&
                        CsrBtBdAddrEq(&(prim->addr), &(bd_addr)))
                    {
                        cancelMsg = TRUE;
                    }
                    break;
                }
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */
#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
                case CSR_BT_CM_BNEP_CONNECT_REQ:
                {
                    CsrBtDeviceAddr conAddr;
                    CsrBtCmBnepConnectReq * prim = (CsrBtCmBnepConnectReq *) msg;
                    conAddr = CsrBtBdAddrFromEtherAddr(&prim->rem_addr);

                    if (CsrBtBdAddrEq(&(conAddr), &(bd_addr)))
                    {
                        cancelMsg = TRUE;
                    }
                    break;
                }
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */
#ifdef CSR_BT_LE_ENABLE
                case CSR_BT_CM_LE_LOCK_SM_QUEUE_REQ:
                {
                    cancelMsg = TRUE;
                    break;
                }
#endif /* CSR_BT_LE_ENABLE */
                default:
                {
                    break;
                }
            }
            if (cancelMsg)
            {
                CsrBtCmFreeDownstreamMessageContents(CSR_BT_CM_PRIM, msg);
                SynergyMessageFree(CSR_BT_CM_PRIM, msg);
            }
            else
            {
                CsrMessageQueuePush(&tempQueue, eventClass, msg);
            }
        }
        else
        {
            CsrMessageQueuePush(&tempQueue, eventClass, msg);
        }
    }
    cmData->smVar.saveQueue = tempQueue;
    return (cancelMsg);
}

CsrBool CsrBtCmRemoveSavedOutgoingConnectMessage(cmInstanceData_t *cmData, CsrBtCmPrim type, CsrUint32 matchValue, CsrUint16 context)
{
    CsrUint16                eventClass;
    void                    *msg;
    CsrMessageQueueType *tempQueue = NULL;
    CsrBool                  found = FALSE;

    while(CsrMessageQueuePop(&cmData->smVar.saveQueue, &eventClass, &msg))
    {
        if (!found)
        {
            CsrBtCmPrim primType = *((CsrBtCmPrim *) msg);
            
            if ( (eventClass == CSR_BT_CM_PRIM) && (type == primType) )
            {
                if( primType == CSR_BT_CM_CONNECT_ACCEPT_REQ )
                {
                    CsrBtCmConnectAcceptReq * prim = (CsrBtCmConnectAcceptReq *) msg;

                    if ((prim->serverChannel == matchValue) && (prim->context == context))
                    {
                        found = TRUE;
                        SynergyMessageFree(CSR_BT_CM_PRIM, msg);
                    }
                    else
                    {
                        CsrMessageQueuePush(&tempQueue, eventClass, msg);
                    }
                }
                else if (primType == CSR_BT_CM_L2CA_CONNECT_ACCEPT_REQ)
                {
                    CsrBtCmL2caConnectAcceptReq * prim = (CsrBtCmL2caConnectAcceptReq *) msg;

                    if ((prim->localPsm == matchValue) && (prim->context == context))
                    {
                        found = TRUE;
                        CsrPmemFree(prim->conftab);
                        SynergyMessageFree(CSR_BT_CM_PRIM, msg);
                    }
                    else
                    {
                        CsrMessageQueuePush(&tempQueue, eventClass, msg);
                    }
                }
                else
                {
                    CsrMessageQueuePush(&tempQueue, eventClass, msg);
                }
            }
            else
            {
                CsrMessageQueuePush(&tempQueue, eventClass, msg);
            }
        }
        else
        {
            CsrMessageQueuePush(&tempQueue, eventClass, msg);
        }
    }
    cmData->smVar.saveQueue = tempQueue;

    return found;
}

CsrBool CsrBtCmCheckSavedIncomingConnectMessages(cmInstanceData_t *cmData, CsrBtDeviceAddr bd_addr)
{
    CsrUint16                eventClass;
    void                    *msg;
    CsrMessageQueueType *tempQueue = NULL;
    CsrBool                  found = FALSE;

    while(CsrMessageQueuePop(&cmData->smVar.saveQueue, &eventClass, &msg))
    {
        if (!found)
        {
            switch (eventClass)
            {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
                case RFCOMM_PRIM:
                {
                    RFCOMM_UPRIM_T  *rfcPrim = (RFCOMM_UPRIM_T *) msg;

                    if( rfcPrim->type == RFC_SERVER_CONNECT_IND )
                    {
                        RFC_SERVER_CONNECT_IND_T * prim = (RFC_SERVER_CONNECT_IND_T *) msg;

                        if (CsrBtBdAddrEq(&(prim->bd_addr), &(bd_addr)))
                        {
                            found = TRUE;
                        }
                    }
                    break;
                }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
                case L2CAP_PRIM:
                {
                    L2CA_UPRIM_T *l2caPrim = (L2CA_UPRIM_T *) msg;

                    if (l2caPrim->type == L2CA_AUTO_CONNECT_IND)
                    {
                        L2CA_AUTO_CONNECT_IND_T * prim = (L2CA_AUTO_CONNECT_IND_T *) msg;

                        if (CsrBtBdAddrEq(&(prim->bd_addr), &(bd_addr)))
                        {
                            found = TRUE;
                        }
                    }
                    break;
                }
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */
                default:
                {
                    break;
                }
            }
        }
        CsrMessageQueuePush(&tempQueue, eventClass, msg);
    }
    cmData->smVar.saveQueue = tempQueue;

    return found;
}

void CsrBtCmRemoveSavedIncomingConnectMessages(cmInstanceData_t *cmData, CsrBtDeviceAddr bd_addr)
{
    CsrUint16                eventClass;
    void                    *msg;
    CsrMessageQueueType    *tempQueue  = NULL;

    while(CsrMessageQueuePop(&cmData->smVar.saveQueue, &eventClass, &msg))
    {
        switch (eventClass)
        {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
            case RFCOMM_PRIM:
            {
                RFCOMM_UPRIM_T  *rfcPrim = (RFCOMM_UPRIM_T *) msg;

                if( rfcPrim->type == RFC_SERVER_CONNECT_IND )
                {
                    RFC_SERVER_CONNECT_IND_T * prim = (RFC_SERVER_CONNECT_IND_T *) msg;

                    if (CsrBtBdAddrEq(&(prim->bd_addr), &(bd_addr)))
                    {
                        rfc_free_primitive(msg);
                    }
                    else
                    {
                        CsrMessageQueuePush(&tempQueue, eventClass, msg);
                    }
                }
                break;
            }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
            case L2CAP_PRIM:
            {
                L2CA_UPRIM_T *l2caPrim = (L2CA_UPRIM_T *) msg;

                if (l2caPrim->type == L2CA_AUTO_CONNECT_IND)
                {
                    L2CA_AUTO_CONNECT_IND_T * prim = (L2CA_AUTO_CONNECT_IND_T *) msg;

                    if (CsrBtBdAddrEq(&(prim->bd_addr), &(bd_addr)))
                    {
                        L2CA_FreePrimitive(msg);
                    }
                    else
                    {
                        CsrMessageQueuePush(&tempQueue, eventClass, msg);
                    }
                }
                break;
            }
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */
            default:
            {
                CsrMessageQueuePush(&tempQueue, eventClass, msg);
                break;
            }
        }
    }
    cmData->smVar.saveQueue = tempQueue;
}

