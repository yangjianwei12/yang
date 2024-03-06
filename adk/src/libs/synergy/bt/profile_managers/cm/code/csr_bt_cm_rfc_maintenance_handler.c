/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef EXCLUDE_CSR_BT_RFC_MODULE

#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"
#ifndef CSR_TARGET_PRODUCT_VM
#include "csr_hci_sco.h"
#endif

static CsrBool csrBtCmRfcConnInstPendingBtConnIdCompare(cmRfcConnInstType *cmRfcConnInst,
                                                        CsrBtConnId btConnId)
{
    if (cmRfcConnInst->pending && cmRfcConnInst->btConnId == btConnId)
    {
        return TRUE;
    }
    else if (!cmRfcConnInst->pending && btConnId == CSR_BT_CONN_ID_INVALID)
    { /* If connection ID is not pending, the pending ID must be considered an invalid ID */
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static CsrBool csrBtCmRfcConnInstBtConnIdCompare(cmRfcConnInstType *cmRfcConnInst,
                                                 CsrBtConnId btConnId)
{
    if (!cmRfcConnInst->pending && cmRfcConnInst->btConnId == btConnId)
    {
        return TRUE;
    }
    else if (cmRfcConnInst->pending && btConnId == CSR_BT_CONN_ID_INVALID)
    { /* If connection ID is pending, it must be considered an invalid ID */
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

CsrUint16 CsrBtCmRfcDetermineMtu(cmInstanceData_t *cmData, CsrBtDeviceAddr deviceAddr, CsrUint16 proposedMtu)
{
#ifdef CSR_TARGET_PRODUCT_VM
        CSR_UNUSED(cmData);
        CSR_UNUSED(deviceAddr);
        CSR_UNUSED(proposedMtu);

        return (proposedMtu == 0) ? CSR_BT_HCI_BUILD_RFCOMM_NON_EDR_MAX_FRAME_SIZE : proposedMtu;
#else
    if (proposedMtu == 0)
    {
        if (cmData->rfcBuild)
        {
            return CSR_BT_RFC_BUILD_RFCOMM_MAX_FRAME_SIZE;
        }
        else
        {
            if (HCI_FEATURE_IS_SUPPORTED(LMP_FEATURES_5SLOT_MR_BIT, cmData->dmVar.lmpSuppFeatures))
            {
                aclTable *aclConnectionElement = NULL;

                returnAclConnectionElement(cmData, deviceAddr, &aclConnectionElement);

                if (aclConnectionElement &&
                    !HCI_FEATURE_IS_SUPPORTED(LMP_FEATURES_5SLOT_MR_BIT, aclConnectionElement->remoteFeatures))
                {
                    return CSR_BT_HCI_BUILD_RFCOMM_NON_EDR_MAX_FRAME_SIZE;
                }
                return CSR_BT_HCI_BUILD_RFCOMM_EDR_MAX_FRAME_SIZE;
            }
            return CSR_BT_HCI_BUILD_RFCOMM_NON_EDR_MAX_FRAME_SIZE;
        }
    }
    else
    {
        return proposedMtu;
    }
#endif /* CSR_TARGET_PRODUCT_VM */
}

cmConnIdServerType CsrBtCmReturnConnIdServerStruct(CsrBtConnId bt_conn_id, CsrUint8 server)
{
    cmConnIdServerType connIdServer;
    connIdServer.bt_conn_id  = bt_conn_id;
    connIdServer.server = server;
    return (connIdServer);
}

cmConnIdServerContextType CsrBtCmReturnConnIdServerContextStruct(CsrBtConnId bt_conn_id, CsrUint8 server, CsrUint16 context)
{
    cmConnIdServerContextType serverInst;
    serverInst.server  = server;
    serverInst.context  = context;
    serverInst.bt_conn_id = bt_conn_id;
    return (serverInst);
}

cmConnIdLocalRemoteServersType CsrBtCmReturnConnIdLocalRemoteServersStruct(CsrBtConnId bt_conn_id, CsrUint8 localServer, CsrUint8 remoteServer, CsrBtDeviceAddr devAddr)
{
    cmConnIdLocalRemoteServersType connElmt;
    connElmt.bt_conn_id  = bt_conn_id;
    connElmt.lServer = localServer;
    connElmt.rServer = remoteServer;
    connElmt.devAddr = devAddr;

    return (connElmt);
}

cmConnIdLocalRemoteServersContextType CsrBtCmReturnConnIdLocalRemoteServersContextStruct(CsrBtConnId       bt_conn_id,
                                                                                         CsrUint8          localServer,
                                                                                         CsrUint8          remoteServer,
                                                                                         CsrBtDeviceAddr   devAddr,
                                                                                         CsrUint16         context)
{
    cmConnIdLocalRemoteServersContextType connElement;

    connElement.bt_conn_id  = bt_conn_id;
    connElement.lServer     = localServer;
    connElement.rServer     = remoteServer;
    connElement.devAddr     = devAddr;
    connElement.context     = context;

    return connElement;
}

CsrBool CsrBtCmRfcFindRfcConnElementFromIndex(CsrCmnListElm_t *elem, void *value)
{ /* This function will return TRUE if the given index match the elementId  */
    CsrUint8 index                 = *(CsrUint8 *) value;
    cmRfcConnElement * theElement = (cmRfcConnElement *) elem;

    if (index == theElement->elementId)
    {
        if (theElement->cmRfcConnInst)
        {
            return (TRUE);
        }
    }
    return (FALSE);
}

CsrBool CsrBtCmRfcFindRfcConnElementFromBtPendingConnId(CsrCmnListElm_t * elem, void * value)
{
    CsrBtConnId btConnId = *(CsrBtConnId *) value;
    cmRfcConnElement * theElement = (cmRfcConnElement *) elem;

    if (theElement->cmRfcConnInst)
    {
        if (csrBtCmRfcConnInstPendingBtConnIdCompare(theElement->cmRfcConnInst,
                                                     btConnId))
        {
            return TRUE;
        }
    }
    return FALSE;
}

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
CsrBool CsrBtCmRfcFindRfcConnElementFromScoHandle(CsrCmnListElm_t *elem, void *value)
{ /* This function will return TRUE if the given scoHandle match            */
    hci_connection_handle_t scoHandle = *(hci_connection_handle_t *) value;
    cmRfcConnElement * theElement     = (cmRfcConnElement *) elem;

    if (theElement->cmRfcConnInst)
    {
        if (theElement->cmRfcConnInst->eScoParms &&
            theElement->cmRfcConnInst->eScoParms->handle == scoHandle)
        {
            return (TRUE);
        }
    }
    return (FALSE);
}
#endif

CsrBool CsrBtCmRfcFindRfcConnElementFromBtConnId(CsrCmnListElm_t *elem, void *value)
{
    CsrBtConnId btConnId = *(CsrBtConnId *) value;
    cmRfcConnElement * theElement = (cmRfcConnElement *) elem;

    if (theElement->cmRfcConnInst)
    {
        if (csrBtCmRfcConnInstBtConnIdCompare(theElement->cmRfcConnInst,
                                              btConnId))
        {
            return TRUE;
        }
    }
    return FALSE;
}


CsrBool CsrBtCmRfcFindRfcConnElementFromConnIdRemoteServer(CsrCmnListElm_t *elem, void *value)
{ /* This function will return TRUE if the given scoHandle match the scoHandle
     in the element                                                             */
    cmConnIdServerType  connIdServ      = *(cmConnIdServerType *) value;
    cmRfcConnElement * theElement = (cmRfcConnElement *) elem;

    if (theElement->cmRfcConnInst)
    {
        if (theElement->cmRfcConnInst->remoteServerChan == connIdServ.server)
        {
            if (csrBtCmRfcConnInstBtConnIdCompare(theElement->cmRfcConnInst,
                                                  connIdServ.bt_conn_id))
            {
                return TRUE;
            }
        }
    }
    return (FALSE);
}

CsrBool CsrBtCmRfcFindRfcConnElementFromConnIdServer(CsrCmnListElm_t *elem, void *value)
{ /* This function will return TRUE if the given server channel and connection id match the local
     server channel and connection id in the element                  */
    cmConnIdServerType  connIdServ      = *(cmConnIdServerType *) value;
    cmRfcConnElement * theElement = (cmRfcConnElement *) elem;

    if (theElement->cmRfcConnInst)
    {
        if (theElement->cmRfcConnInst->serverChannel == connIdServ.server)
        {
            if (csrBtCmRfcConnInstBtConnIdCompare(theElement->cmRfcConnInst,
                                                  connIdServ.bt_conn_id))
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

CsrBool CsrBtCmRfcFindRfcConnElementFromConnIdLocalRemoteServersContext(CsrCmnListElm_t *elem, void *value)
{
    cmConnIdLocalRemoteServersContextType searchData = *(cmConnIdLocalRemoteServersContextType *) value;
    cmRfcConnElement *theElement = (cmRfcConnElement *) elem;
    cmRfcConnInstType *connInst = theElement->cmRfcConnInst;

    return (connInst &&
            connInst->context == searchData.context &&
            connInst->serverChannel == searchData.lServer &&
            connInst->remoteServerChan == searchData.rServer &&
            csrBtCmRfcConnInstBtConnIdCompare(connInst, searchData.bt_conn_id) &&
            CsrBtBdAddrEq(&connInst->deviceAddr, &searchData.devAddr));
}

CsrBool CsrBtCmRfcFindRfcConnElementFromConnIdLocalRemoteServers(CsrCmnListElm_t *elem, void *value)
{ /* This function will return TRUE if the given local server channel, remote server channel and 
     connection id match the local server channel, remote server channel and connection id in the element */
    cmConnIdLocalRemoteServersType  connIdServ      = *(cmConnIdLocalRemoteServersType *) value;
    cmRfcConnElement *theElement = (cmRfcConnElement *) elem;
    cmRfcConnInstType *connInst = theElement->cmRfcConnInst;

    if (connInst)
    {
        if (connInst->serverChannel == connIdServ.lServer)
        {
            if (connInst->remoteServerChan == connIdServ.rServer)
            {
                if (csrBtCmRfcConnInstBtConnIdCompare(connInst,
                                                      connIdServ.bt_conn_id))
                {
                    if (CsrBtBdAddrEq(&connInst->deviceAddr, &connIdServ.devAddr) != FALSE)
                    {
                        return (TRUE);
                    }
                }
            }
        }
    }
    return (FALSE);
}

CsrBool CsrBtCmRfcFindRfcConnElementFromConnIdServerContext(CsrCmnListElm_t *elem, void *value)
{ /* This function will return TRUE if the given server channel, context and connection id match the 
     local server channel, context and connection id in the element                  */
    cmConnIdServerContextType  serverInst = *(cmConnIdServerContextType *) value;
    cmRfcConnElement * theElement = (cmRfcConnElement *) elem;

    if (theElement->cmRfcConnInst)
    {
        if (theElement->cmRfcConnInst->serverChannel == serverInst.server)
        {
            if (theElement->cmRfcConnInst->context == serverInst.context)
            {
                if (csrBtCmRfcConnInstBtConnIdCompare(theElement->cmRfcConnInst,
                                                      serverInst.bt_conn_id))
                {
                    return (TRUE);
                }
            }
        }
    }
    return (FALSE);
}

cmRfcConnElement * CsrBtCmRfcFindRfcConnElementFromDeviceAddrState(cmInstanceData_t *cmData, CsrBtDeviceAddr *deviceAddr, CsrUint32 state)
{ /* Try to find the cmRfcConnElement that match with the given deviceAddr
     and state. If no match it return NULL                                      */
    cmRfcConnElement *currentElem;

    for (currentElem = CM_RFC_GET_FIRST(cmData->rfcVar.connList); currentElem; currentElem = currentElem->next)
    {
        if (currentElem->cmRfcConnInst)
        {
            if(state == currentElem->cmRfcConnInst->state)
            {
                if(CsrBtBdAddrEq(deviceAddr, &(currentElem->cmRfcConnInst->deviceAddr)))
                {
                    return currentElem;
                }
            }
        }
    }
    return NULL;
}

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
cmRfcConnElement * CsrBtCmRfcFindRfcConnElementFromDeviceAddrScoHandle(cmInstanceData_t *cmData, CsrBtDeviceAddr *deviceAddr, hci_connection_handle_t scoHandle)
{ /* Try to find the cmRfcConnElement that match with the given deviceAddr
     and scoHandle. If no match it return NULL                              */
    cmRfcConnElement *currentElem;

    for (currentElem = CM_RFC_GET_FIRST(cmData->rfcVar.connList); currentElem; currentElem = currentElem->next)
    {
        if (currentElem->cmRfcConnInst)
        {
            if (currentElem->cmRfcConnInst->eScoParms &&
                scoHandle == currentElem->cmRfcConnInst->eScoParms->handle)
            {
                if(CsrBtBdAddrEq(deviceAddr, &(currentElem->cmRfcConnInst->deviceAddr)))
                {
                    return currentElem;
                }
            }
        }
    }
    return NULL;
}
#endif

cmRfcConnElement * CsrBtCmRfcFindRfcConnElementFromServerState(cmInstanceData_t *cmData, CsrUint8 server, CsrUint32 state)
{ /* Try to find the cmRfcConnElement that match with the given server channel
     and state. If no match it return NULL                                    */
    cmRfcConnElement *currentElem;

    for (currentElem = CM_RFC_GET_FIRST(cmData->rfcVar.connList); currentElem; currentElem = currentElem->next)
    {
        if (currentElem->cmRfcConnInst)
        {
            if(server == currentElem->cmRfcConnInst->serverChannel)
            {
                if(state == currentElem->cmRfcConnInst->state)
                {
                    return currentElem;
                }
            }
        }
    }
    return NULL;
}

cmRfcConnElement * CsrBtCmRfcFindRfcConnElementFromDeviceAddrState1OrState2(cmInstanceData_t *cmData,
                                            CsrBtDeviceAddr *deviceAddr, CsrUint32 state1, CsrUint32 state2)
{ /* Try to find the cmRfcConnElement that match with the given  deviceaddr and
     either of the state1 or state2. If no match it return NULL                         */
    cmRfcConnElement *currentElem;

    for (currentElem = CM_RFC_GET_FIRST(cmData->rfcVar.connList); currentElem; currentElem = currentElem->next)
    {
        if (currentElem->cmRfcConnInst)
        {
            if(CsrBtBdAddrEq(deviceAddr, &(currentElem->cmRfcConnInst->deviceAddr)))
            {
                if (state1 == currentElem->cmRfcConnInst->state ||
                    state2 == currentElem->cmRfcConnInst->state)
                {
                    return currentElem;
                }
            }
        }
    }
    return NULL;
}

CsrBool CsrBtCmRfcConnElementIndexCheck(CsrCmnListElm_t *elem, void *value)
{ /* This function will return TRUE if the given index match the elementId  */
    CsrUint8 index                 = *(CsrUint8 *) value;
    cmRfcConnElement * theElement = (cmRfcConnElement *) elem;

    if (index == theElement->elementId)
    {
        return (TRUE);
    }
    return (FALSE);
}

void CsrBtCmRfcEscoParmsFree(cmRfcConnInstType *connInst)
{
#ifndef EXCLUDE_CSR_BT_SCO_MODULE
    if (connInst->eScoParms)
    {
#if !defined (CSR_TARGET_PRODUCT_VM) && !defined (CSR_TARGET_PRODUCT_WEARABLE)
        if (connInst->eScoParms->handle != NO_SCO)
        {
            CsrHciDeRegisterScoHandle(connInst->eScoParms->handle);
        }
#endif
        CsrBtCmScoFreePacketTypeArray(&connInst->eScoParms->negotiateCnt);

        CsrPmemFree(connInst->eScoParms);
        connInst->eScoParms = NULL;
    }
#else
    CSR_UNUSED(connInst);
#endif
}

void cleanUpConnectionTable(cmRfcConnInstType ** theLogicalLink)
{
    cmRfcConnInstType * connInst = *theLogicalLink;

    if (connInst)
    { /* Deallocate priInst and inst                                            */
#ifndef CSR_STREAMS_ENABLE
        CsrUintFast16 i;

        for( i = 0; i < CSR_BT_CM_INIT_CREDIT; i++)
        {
            if(connInst->dataControl.receivedBuffer[i] != NULL)
            { /* To prevent memory leaks, CsrPmemFree data in the receivedbuffer */
                rfc_free_primitive(connInst->dataControl.receivedBuffer[i]);
            }
        }
#endif
        bluestack_msg_free(RFCOMM_PRIM, connInst->controlSignalQueue);
        CsrBtCmRfcEscoParmsFree(connInst);

        CsrPmemFree(connInst);
        *theLogicalLink = NULL;
    }
    else
    { /* Nothing to deallocate                                                  */
        ;
    }
}

CsrBool CsrBtCmIncomingSecRegisterDeregisterRequired(cmInstanceData_t *cmData, CsrUint8 server)
{
    cmRfcConnElement *currentElem;
    CsrUint8 incoming = 0;

    for (currentElem = CM_RFC_GET_FIRST(cmData->rfcVar.connList); currentElem; currentElem = currentElem->next)
    {
        if (currentElem->cmRfcConnInst)
        {
            if(currentElem->cmRfcConnInst->serverChannel == server &&
               currentElem->cmRfcConnInst->remoteServerChan == CSR_BT_NO_SERVER)
            {
               incoming++;
            }
        }
    }
    return((incoming == 1) ? TRUE : FALSE);
}

#endif /* !EXCLUDE_CSR_BT_RFC_MODULE */
