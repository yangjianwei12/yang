/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE

#include "csr_synergy.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_dm.h"

#ifdef CSR_STREAMS_ENABLE
#define csrBtCmL2caCleanDataSaveQueue(_l2capConn)
#else
static void csrBtCmL2caCleanDataSaveQueue(cmL2caConnInstType *l2capConn)
{
    CsrUint16 eventClass;
    void *msg;
    while(CsrMessageQueuePop(&l2capConn->rxQueue, &eventClass, &msg))
    {
        L2CA_FreePrimitive((L2CA_UPRIM_T *)msg);
    }
}
#endif /* !CSR_STREAMS_ENABLE */

CsrBool CsrBtCmL2caConnElementIndexCheck(CsrCmnListElm_t *elem, void *value)
{ /* This function will return TRUE if the given index match the elementId  */
    CsrUint8 index                  = *(CsrUint8 *) value;
    cmL2caConnElement * theElement = (cmL2caConnElement *) elem;

    if (index == theElement->elementId)
    {
        return (TRUE);
    }
    return (FALSE);
}

CsrBool CsrBtCmL2caFindL2caConnElementFromBtConnId(CsrCmnListElm_t *elem, void *value)
{ /* This function will return TRUE if the given cid match                          */
    CsrBtConnId btConnId          = *((CsrBtConnId *) value);
    cmL2caConnElement *theElement = (cmL2caConnElement *) elem;

    if (theElement->cmL2caConnInst)
    {
        if (theElement->cmL2caConnInst->btConnId == btConnId)
        {
            return (TRUE);
        }
    }
    return (FALSE);
}

CsrBool CsrBtCmL2caFindL2caConnElementFromReserveBtConnIdPsm(CsrCmnListElm_t *elem, void *value)
{ /* This function will return TRUE if the cid is reserved and the given psm match  */
    psm_t psm                     = *((psm_t *) value);
    cmL2caConnElement *theElement = (cmL2caConnElement *) elem;

    if (theElement->cmL2caConnInst)
    {
        if (theElement->cmL2caConnInst->btConnId == BTCONN_ID_RESERVED)
        {
            if (theElement->cmL2caConnInst->psm == psm)
            {
                return (TRUE);
            }
        }
    }
    return (FALSE);
}

CsrBool CsrBtCmL2caFindL2caConnElementFromReserveBtConnIdPsmBdaddr(CsrCmnListElm_t *elem, void *value)
{
    cmL2caConnEltType connElt = *(cmL2caConnEltType *)value;
    cmL2caConnElement *theElement = (cmL2caConnElement *) elem;

    if (theElement->cmL2caConnInst)
    {
        if (theElement->cmL2caConnInst->btConnId == BTCONN_ID_RESERVED)
        {
            if (theElement->cmL2caConnInst->psm == connElt.psm)
            {
                if (CsrBtBdAddrEq(&theElement->cmL2caConnInst->deviceAddr, &connElt.devAddr) != FALSE)
                {
                    return (TRUE);
                }
            }
        }
    }
    return (FALSE);
}

CsrBool CsrBtCmL2caFindL2caConnElementFromReserveBtConnIdPsmContext(CsrCmnListElm_t *elem, void *value)
{ /* This function will return TRUE if the cid is reserved and the given psm and context match  */
    psm_t psm                     = (*(CsrBtCancelConnectAcceptContext *) value).psm;
    CsrUint16 context             = (*(CsrBtCancelConnectAcceptContext *) value).context;
    cmL2caConnElement *theElement = (cmL2caConnElement *) elem;

    if (theElement->cmL2caConnInst)
    {
        if (theElement->cmL2caConnInst->btConnId == BTCONN_ID_RESERVED)
        {
            if (theElement->cmL2caConnInst->psm == psm)
            {
                if (theElement->cmL2caConnInst->context == context)
                {
                    return (TRUE);
                }
            }
        }
    }
    return (FALSE);
}

CsrBool CsrBtCmL2caFindL2caConnElementFromCancelledBtConnIdPsm(CsrCmnListElm_t *elem, void *value)
{ /* This function will return TRUE if the cid is cancelled and the given psm match  */
    psm_t psm                     = *((psm_t *) value);
    cmL2caConnElement *theElement = (cmL2caConnElement *) elem;

    if (theElement->cmL2caConnInst)
    {
        if (theElement->cmL2caConnInst->btConnId == CSR_BT_CONN_ID_CANCELLED)
        {
            if (theElement->cmL2caConnInst->psm == psm)
            {
                return (TRUE);
            }
        }
    }
    return (FALSE);
}

#ifdef CSR_BT_INSTALL_L2CAP_CONNLESS_SUPPORT
CsrBool CsrBtCmL2caFindL2caConnElementFromPsm(CsrCmnListElm_t *elem, void *value)
{
    /* Find connection block based on PSM */
    psm_t psm = *((psm_t *) value);
    cmL2caConnElement *theElement = (cmL2caConnElement*)elem;

    if (theElement->cmL2caConnInst)
    {
        if (theElement->cmL2caConnInst->psm == psm)
        {
            return TRUE;
        }
    }
    return FALSE;
}
#endif

CsrBool CsrBtCmL2caFindL2caPriorityDataChannel(CsrCmnListElm_t *elem, void *value)
{
    /* This function will return TRUE if a high data priority channel exists                      */
    CsrBtDeviceAddr   deviceAddr  = *((CsrBtDeviceAddr *) value);
    cmL2caConnElement *theElement = (cmL2caConnElement *) elem;

    if (theElement->cmL2caConnInst)
    {
        if(CsrBtBdAddrEq(&(deviceAddr), &(theElement->cmL2caConnInst->deviceAddr)))
        {
            if (theElement->cmL2caConnInst->dataPriority != CSR_BT_CM_PRIORITY_NORMAL)
            {
                return (TRUE);
            }
        }
    }
    return (FALSE);
}

CsrBool CsrBtCmL2caFindL2caConnElementFromConnectedSDeviceAddr(CsrCmnListElm_t *elem, void *value)
{ /* This function will return TRUE if the state is CSR_BT_CM_L2CAP_STATE_CONNECTED and the given deviceAddr match  */
    CsrBtDeviceAddr      deviceAddr  = *((CsrBtDeviceAddr *) value);
    cmL2caConnElement *theElement = (cmL2caConnElement *) elem;

    if (theElement->cmL2caConnInst)
    {
        if (theElement->cmL2caConnInst->state == CSR_BT_CM_L2CAP_STATE_CONNECTED)
        {
            if(CsrBtBdAddrEq(&(deviceAddr), &(theElement->cmL2caConnInst->deviceAddr)))
            {
                return (TRUE);
            }
        }
    }
    return (FALSE);
}

CsrBool CsrBtCmL2caFindL2caConnElementFromIndex(CsrCmnListElm_t *elem, void *value)
{ /* This function will return TRUE if the given index match the elementId  */
    CsrUint8 index                  = *((CsrUint8 *) value);
    cmL2caConnElement * theElement = (cmL2caConnElement *) elem;

    if (index == theElement->elementId)
    {
        if (theElement->cmL2caConnInst)
        {
            return (TRUE);
        }
    }
    return (FALSE);
}

void numberOfSecurityRegister(cmInstanceData_t *cmData, psm_t localPsm, CsrBtDeviceAddr deviceAddr, CsrUint8 *numOfOutgoing, CsrUint8 *numOfIncoming)
{
    cmL2caConnElement *currentElem;
    CsrUint8 outgoing = 0;
    CsrUint8 incoming = 0;

    for (currentElem = CM_L2CA_GET_FIRST(cmData->l2caVar.connList); currentElem; currentElem = currentElem->next)
    { /* Search through the L2CAP connection list                     */
        if (currentElem->cmL2caConnInst)
        {
            if(currentElem->cmL2caConnInst->psm == localPsm)
            {
                if (currentElem->cmL2caConnInst->remotePsm != NO_REMOTE_PSM)
                { /* The local device has initiate the connection */
                    if(CsrBtBdAddrEq(&(deviceAddr), &(currentElem->cmL2caConnInst->deviceAddr)))
                    {
                        outgoing++;
                    }
                    else
                    {
                        ;
                    }
                }
                else
                {
                    incoming++;
                }
            }
        }
    }
    *numOfOutgoing = outgoing;
    *numOfIncoming = incoming;
}

CsrUint16 returnL2capSmallestFlushTo(cmInstanceData_t *cmData, CsrBtDeviceAddr *deviceAddr)
{
    cmL2caConnElement *currentElem;
    CsrUint16 flushTo = L2CA_FLUSH_TO_DEFAULT;

    for (currentElem = CM_L2CA_GET_FIRST(cmData->l2caVar.connList); currentElem; currentElem = currentElem->next)
    { /* Search through the L2CAP connection list                     */
        if (currentElem->cmL2caConnInst)
        {
            if(currentElem->cmL2caConnInst->outgoingFlush < flushTo &&
               CsrBtBdAddrEq(&(currentElem->cmL2caConnInst->deviceAddr), deviceAddr))
            {
                flushTo = currentElem->cmL2caConnInst->outgoingFlush;
            }
        }
    }
    return flushTo;
}

aclTable* returnAclTable(cmInstanceData_t *cmData, CsrBtDeviceAddr *deviceAddr)
{
    CsrIntFast32 i;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; ++i)
    {
        if(CsrBtBdAddrEq(&cmData->roleVar.aclVar[i].deviceAddr, deviceAddr))
        {
            return &cmData->roleVar.aclVar[i];
        }
    }
    return NULL;
}

void CsrBtCleanUpCmL2caConnInst(cmL2caConnInstType **theLink)
{
    cmL2caConnInstType * connInst = *theLink;

    if (connInst)
    {
#ifndef CSR_STREAMS_ENABLE
        if(connInst->combine != NULL)
        {
            CsrMblkDestroy(connInst->combine);
        }
#endif
        csrBtCmL2caCleanDataSaveQueue(connInst);

        CsrPmemFree(connInst->conftabIter.block);
        CsrPmemFree(connInst);
        *theLink = NULL;
    }
    else
    {
        /* Nothing to deallocate                                                  */
        ;
    }
}

void CsrBtCmL2capClearL2capTableIndex(cmInstanceData_t *cmData, cmL2caConnInstType **theLink)
{
    CSR_UNUSED(cmData);
    csrBtCmL2caCleanDataSaveQueue(*theLink);
    CsrBtCleanUpCmL2caConnInst(theLink);
}

void CsrBtCmL2capAcceptFailClearing(cmInstanceData_t *cmData, cmL2caConnInstType *theLink)
{
    CSR_UNUSED(cmData);
    CSR_BT_CM_STATE_CHANGE(theLink->state, CSR_BT_CM_L2CAP_STATE_IDLE);

    CsrBtBdAddrZero(&theLink->deviceAddr);
    theLink->btConnId               = BTCONN_ID_RESERVED;
    theLink->remotePsm              = NO_REMOTE_PSM;
    theLink->outgoingFlush          = L2CA_FLUSH_TO_DEFAULT;
    CSR_BT_CM_L2CAP_TX_MAX_COUNT_SET(theLink, 0);

    csrBtCmL2caCleanDataSaveQueue(theLink);
}

void CsrBtCmL2caTimeoutIndHandler(cmInstanceData_t *cmData)
{
    L2CA_TIMEOUT_IND_T  * prim      = (L2CA_TIMEOUT_IND_T *) cmData->recvMsgP;
    CsrBtConnId btConnId = CM_CREATE_L2CA_CONN_ID(prim->cid);
    cmL2caConnElement * theElement  = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromBtConnId, &(btConnId));

    if (theElement)
    {
        cmL2caConnInstType *l2CaConnection = theElement->cmL2caConnInst;

        switch(l2CaConnection->state)
        {
            case CSR_BT_CM_L2CAP_STATE_CONNECT:
            case CSR_BT_CM_L2CAP_STATE_CONNECT_ACCEPT:
            {
                L2CA_DisconnectReq(prim->cid);
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

CsrBool CmL2caFindL2caConnElementWithConnectedPsmBdaddr(CsrCmnListElm_t *elem, void *value)
{
    cmL2caConnEltType connElt     = *(cmL2caConnEltType *)value;
    cmL2caConnElement *theElement = (cmL2caConnElement *) elem;

    if (theElement->cmL2caConnInst)
    {
        if (theElement->cmL2caConnInst->state == CSR_BT_CM_L2CAP_STATE_CONNECTED)
        {
            if (theElement->cmL2caConnInst->psm == connElt.psm)
            {
                if (CsrBtBdAddrEq(&theElement->cmL2caConnInst->deviceAddr, &connElt.devAddr) != FALSE)
                {
                    return (TRUE);
                }
            }
        }
    }
    return (FALSE);
}

#endif /* !EXCLUDE_CSR_BT_L2CA_MODULE */
