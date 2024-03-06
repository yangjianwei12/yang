/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_sdc.h"
#include "csr_bt_cm_util.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_dm_sc_lib.h"
#include "csr_bt_sdc_support.h"
#include "csr_bt_cm_private_lib.h"

#define MAX_RECORDS_TO_RETURN       0x0040
#define MAX_BYTES_TO_RETURN         0x0070

static void removeElementFromSdcList(cmInstanceData_t * cmData);

static void delayedMessage(CsrUint16 appHdl, void *msg)
{
    CsrBtCmPutMessage(appHdl, msg);
}

CsrBool CsrBtCmSdcSearchListAddrCompare(CsrCmnListElm_t *elem, void *value)
{
    sdcSearchElement *searchElem = (sdcSearchElement *) elem;
    CsrBtDeviceAddr *addr = (CsrBtDeviceAddr *) value;

    if (CsrBtBdAddrEq(&searchElem->deviceAddr, addr))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

CsrBool CsrBtCmSdcSearchListIndexItemCompare(CsrCmnListElm_t *elem, void *value)
{
    sdcSearchElement *searchElem = (sdcSearchElement *) elem;
    sdcResultIndexItem *indexItem = (sdcResultIndexItem *) value;

    if (CsrBtBdAddrEq(&searchElem->deviceAddr, &indexItem->deviceAddr) &&
        searchElem->appHandle == indexItem->appHandle)
    {
        return TRUE;
    }
    else
    {
        indexItem->index++;
        return FALSE;
    }
}

CsrBool CsrBtCmSdcSearchListItemCompare(CsrCmnListElm_t *elem, void *value)
{
    sdcSearchElement *searchElem = (sdcSearchElement *) elem;
    sdcResultItem *resultItem = (sdcResultItem *) value;

    if (CsrBtBdAddrEq(&searchElem->deviceAddr, &resultItem->deviceAddr) &&
        searchElem->appHandle == resultItem->appHandle)
    {
        CsrUint8 i;

        for (i = 0; i < searchElem->uuidCount; i++)
        {
            if (searchElem->searchPtrArray[i])
            {
                CsrUint8 j;

                for (j = 0; j < searchElem->searchPtrArray[i]->resultListSize; j++)
                {
                    if (searchElem->searchPtrArray[i]->resultList[j].serviceHandle == resultItem->serviceHandle)
                    {
                        resultItem->serverChannel = searchElem->searchPtrArray[i]->resultList[j].serverChannel;
                        return TRUE;
                    }
                }
            }
        }
    }

    return FALSE;
}

CsrBool CsrBtCmSdcSearchListElemRemove(CsrCmnListElm_t *elem, void *data)
{
    CsrUint8 i;
    sdcSearchElement *searchElem = (sdcSearchElement *) elem;

    for (i = 0; i < searchElem->uuidCount; i++)
    {
        if (searchElem->searchPtrArray[i])
        {
            CsrPmemFree(searchElem->searchPtrArray[i]);
        }

        if (searchElem->attrInfoList && searchElem->attrInfoList[i].attrList)
        {
            CsrPmemFree(searchElem->attrInfoList[i].attrList);
        }
    }
    if (searchElem->attrInfoList)
    {
        CsrPmemFree(searchElem->attrInfoList);
    }

    CsrPmemFree(searchElem->serviceInfoList->serverChannelList);
    CsrPmemFree(searchElem->serviceInfoList->serviceHandleList);

    if (searchElem->uuidType == CSR_BT_CM_SDC_UUID32)
    {
        CsrPmemFree(searchElem->uuid.uuid32);
    }
    else if (searchElem->uuidType == CSR_BT_CM_SDC_UUID128)
    {
        CsrPmemFree(searchElem->uuid.uuid128);
    }
    else
    {
        CsrPmemFree(searchElem->uuid.uuidSet);
    }

    CSR_UNUSED(data);

    return TRUE;
}

void CsrBtCmSdcSearchListDeinit(CsrCmnList_t *searchList)
{
    CsrCmnListIterateAllowRemove(searchList,
                                 CsrBtCmSdcSearchListElemRemove,
                                 NULL);
}

static void CsrBtCmSdcSearchIndMsgSend(cmInstanceData_t *cmData, sdcSearchElement * theElement, CsrUint16 size)
{
    cmData->sdcVar.validSearchResult = TRUE;

    if (theElement->uuidType == CSR_BT_CM_SDC_UUID32)
    {
        CsrUint8 i;
        CsrBtCmSdcSearchInd * prim;

        prim                        = (CsrBtCmSdcSearchInd *) CsrPmemAlloc(sizeof(CsrBtCmSdcSearchInd));
        prim->type                  = CSR_BT_CM_SDC_SEARCH_IND;
        prim->service               = theElement->uuid.uuid32[cmData->sdcVar.uuidIndex];
        prim->serviceHandleListCount         = size;
        prim->localServerChannel    = cmData->sdcVar.localServer;
        prim->deviceAddr            = theElement->deviceAddr;
        prim->serviceHandleList     = (CsrBtUuid32 *) CsrPmemAlloc(sizeof(CsrBtUuid32) * size);

        for (i = 0; i < size; i++)
        {
            prim->serviceHandleList[i] = theElement->searchPtrArray[cmData->sdcVar.currentIndex]->resultList[i].serviceHandle;
        }
        CsrBtCmPutMessage(theElement->appHandle, prim);
    }
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
    else
    {
        CsrUintFast8                 t;
        CsrBtCmSdcUuid128SearchInd * prim;

        prim                    = (CsrBtCmSdcUuid128SearchInd *) CsrPmemAlloc(sizeof(CsrBtCmSdcUuid128SearchInd));
        prim->type              = CSR_BT_CM_SDC_UUID128_SEARCH_IND;

        for (t = 0; t < 16; t++)
        {
            prim->service[t]    = theElement->uuid.uuid128[cmData->sdcVar.uuidIndex][t];
        }
        prim->serviceHandleListCount         = size;
        prim->localServerChannel    = cmData->sdcVar.localServer;
        prim->deviceAddr            = theElement->deviceAddr;
        prim->serviceHandleList     = (CsrBtUuid32 *) CsrPmemAlloc(sizeof(CsrBtUuid32) * size);

        for (t = 0; t < size; t++)
        {
            prim->serviceHandleList[t] = theElement->searchPtrArray[cmData->sdcVar.currentIndex]->resultList[t].serviceHandle;
        }
        CsrBtCmPutMessage(theElement->appHandle, prim);
    }
#endif
}

static void csrBtCmSdcServiceSearchCfmMsgSend(cmInstanceData_t *cmData,
                                              sdcResult_t *resultList,
                                              CsrUint16 resultListLength,
                                              CsrBtResultCode resultCode,
                                              CsrBtSupplier resultSupplier)
{
    CsrBtCmSdcServiceSearchCfm *prim;
    sdcSearchElement *currentElement;

    currentElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t*) &cmData->sdcVar.sdcSearchList,
                                                                 cmData->sdcVar.currentElement);
    prim        = (CsrBtCmSdcServiceSearchCfm *)CsrPmemAlloc(sizeof(CsrBtCmSdcServiceSearchCfm));
    prim->type  = CSR_BT_CM_SDC_SERVICE_SEARCH_CFM;

    if (resultList)
    {
        CsrUint8 i;

        prim->recList = (CsrBtUuid32 *) CsrPmemAlloc(sizeof(CsrBtUuid32) * resultListLength);
        for (i = 0; i < resultListLength; i++)
        {
            prim->recList[i] = resultList[i].serviceHandle;
        }
    }
    else
    {
        prim->recList = NULL;
    }

    prim->recListLength             = resultListLength;
    prim->resultCode                = resultCode;
    prim->resultSupplier            = resultSupplier;

    CsrBtCmPutMessage(currentElement->appHandle, prim);
}

static void CsrBtCmSdcSearchCfmMsgSend(CsrSchedQid appHandle, CsrUint8 server, CsrBtDeviceAddr deviceAddr)
{
    CsrBtCmSdcSearchCfm  * prim;

    prim                        = (CsrBtCmSdcSearchCfm *) CsrPmemAlloc(sizeof(CsrBtCmSdcSearchCfm));
    prim->type                  = CSR_BT_CM_SDC_SEARCH_CFM;
    prim->localServerChannel    = server;
    prim->deviceAddr            = deviceAddr;
    CsrBtCmPutMessage(appHandle, prim);
}

static void csrBtCmSdcAttrCfmMsgSend(cmInstanceData_t *cmData)
{
    CsrBtCmSdcAttributeCfm          * prim;
    SDC_SERVICE_ATTRIBUTE_CFM_T     * sdcPrim;
    sdcSearchElement *currentElement;

    currentElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t*) &cmData->sdcVar.sdcSearchList,
                                                                 cmData->sdcVar.currentElement);

    sdcPrim                     = (SDC_SERVICE_ATTRIBUTE_CFM_T *) cmData->recvMsgP;
    prim                        = (CsrBtCmSdcAttributeCfm *) CsrPmemAlloc(sizeof(CsrBtCmSdcAttributeCfm));
    prim->type                  = CSR_BT_CM_SDC_ATTRIBUTE_CFM;
    prim->attributeList         = sdcPrim->attr_list;
    prim->attributeListSize     = sdcPrim->size_attr_list;
    prim->localServerChannel    = cmData->sdcVar.localServer;
    prim->deviceAddr            = currentElement->deviceAddr;

    sdcPrim->attr_list = NULL;

    if (sdcPrim->response == SDC_RESPONSE_SUCCESS)
    {
        prim->resultCode     = CSR_BT_RESULT_CODE_CM_SUCCESS;
        prim->resultSupplier = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        prim->resultCode     = (CsrBtResultCode) sdcPrim->response;
        prim->resultSupplier = CSR_BT_SUPPLIER_SDP_SDC;
    }
    CsrBtCmPutMessage(currentElement->appHandle, prim);
}

static void cmSdcServiceSearchAttrIndSend(cmInstanceData_t *cmData)
{
    sdcSearchElement *currentElement;
    CmSdcServiceSearchAttrInd          * prim    = CsrPmemAlloc(sizeof(CmSdcServiceSearchAttrInd));
    SDC_SERVICE_SEARCH_ATTRIBUTE_CFM_T * sdcPrim = (SDC_SERVICE_SEARCH_ATTRIBUTE_CFM_T *) cmData->recvMsgP;

    currentElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t*) &cmData->sdcVar.sdcSearchList,
                                                                 cmData->sdcVar.currentElement);

    prim->type               = CM_SDC_SERVICE_SEARCH_ATTR_IND;
    prim->attributeList      = sdcPrim->attr_list;
    prim->attributeListSize  = sdcPrim->size_attr_list;
    prim->serviceIndex       = cmData->sdcVar.uuidIndex;
    prim->localServerChannel = cmData->sdcVar.localServer;
    prim->deviceAddr         = currentElement->deviceAddr;
    prim->serviceHandle      = INVALID_SERVICE_HANDLE;

    if (sdcPrim->response == SDC_RESPONSE_SUCCESS)
    {
        cmData->sdcVar.validSearchResult = TRUE;

        if (currentElement->obtainServerChannels)
        {
            prim->serviceHandle = currentElement->serviceInfoList->serviceHandleList[cmData->sdcVar.currentServiceIndex - 1];
        }
        prim->resultCode     = CSR_BT_RESULT_CODE_CM_SUCCESS;
        prim->resultSupplier = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        prim->resultCode     = (CsrBtResultCode) sdcPrim->response;
        prim->resultSupplier = CSR_BT_SUPPLIER_SDP_SDC;
    }

    sdcPrim->attr_list = NULL;

    CsrBtCmPutMessage(currentElement->appHandle, prim);
}

static void cmSdcServiceSearchAttrCfmMsgSend(cmInstanceData_t *cmData,
                                             CsrSchedQid appHandle,
                                             CsrBtDeviceAddr deviceAddr,
                                             CsrBtResultCode resultCode,
                                             CsrBtSupplier resultSupplier)
{
    CmSdcServiceSearchAttrCfm * prim = CsrPmemZalloc(sizeof(CmSdcServiceSearchAttrCfm));

    prim->type                       = CM_SDC_SERVICE_SEARCH_ATTR_CFM;
    prim->deviceAddr                 = deviceAddr;
    prim->serviceHandle              = INVALID_SERVICE_HANDLE;

    if ((resultSupplier == CSR_BT_SUPPLIER_SDP_SDC) && 
        (resultCode != SDC_RESPONSE_SUCCESS) && 
        (cmData->sdcVar.validSearchResult != FALSE))
    {
        /*If out of all the Service search attribute requests if even 1 has recieved a success response 
          then the sdp search was a success.*/
        prim->resultCode                 = CSR_BT_RESULT_CODE_CM_SUCCESS;
        prim->resultSupplier             = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        prim->resultCode                 = resultCode;
        prim->resultSupplier             = resultSupplier;
    }
    prim->localServerChannel         = cmData->sdcVar.localServer;

    if (resultCode == SDC_RESPONSE_SUCCESS &&
        resultSupplier == CSR_BT_SUPPLIER_SDP_SDC)
    {
        sdcSearchElement *currentElement;
        SDC_SERVICE_SEARCH_ATTRIBUTE_CFM_T * sdcPrim = (SDC_SERVICE_SEARCH_ATTRIBUTE_CFM_T *) cmData->recvMsgP;

        currentElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t*) &cmData->sdcVar.sdcSearchList,
                                                                     cmData->sdcVar.currentElement);

        prim->serviceIndex       = cmData->sdcVar.uuidIndex;
        prim->attributeList      = sdcPrim->attr_list;
        prim->attributeListSize  = sdcPrim->size_attr_list;
        prim->resultCode         = CSR_BT_RESULT_CODE_CM_SUCCESS;
        prim->resultSupplier     = CSR_BT_SUPPLIER_CM;

        if (currentElement->obtainServerChannels)
        {
            /* currentServiceIndex points to the first empty position in the array.
               So decrement the index by 1 to get the actual value */
            prim->serviceHandle  = currentElement->serviceInfoList->serviceHandleList[cmData->sdcVar.currentServiceIndex - 1];
        }

        /* The same attribute list is sent to the application.
           Point it to NULL and let the application free the memory */
        sdcPrim->attr_list       = NULL;
    }

    CsrBtCmPutMessage(appHandle, prim);
}

static void csrBtCmSdcCloseIndMsgSend(CsrSchedQid appHandle,
                                      CsrUint8 server,
                                      CsrBtDeviceAddr deviceAddr,
                                      CsrBtResultCode resultCode,
                                      CsrBtSupplier resultSupplier)
{
    CsrBtCmSdcCloseInd  * prim;
    prim                        = (CsrBtCmSdcCloseInd *) CsrPmemAlloc(sizeof(CsrBtCmSdcCloseInd));
    prim->type                  = CSR_BT_CM_SDC_CLOSE_IND;
    prim->resultCode            = resultCode;
    prim->resultSupplier        = resultSupplier;
    prim->localServerChannel    = server;
    prim->deviceAddr            = deviceAddr;
    CsrBtCmPutMessage(appHandle, prim);
}

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
static CsrBool getSdcElement(cmInstanceData_t *cmData, CsrSchedQid appHandle, CsrBtDeviceAddr deviceAddr)
{
    sdcResultIndexItem indexItem;

    indexItem.appHandle = appHandle;
    indexItem.deviceAddr = deviceAddr;
    indexItem.index = 0;

    if (CsrCmnListSearch((CsrCmnList_t*) &cmData->sdcVar.sdcSearchList,
                         CsrBtCmSdcSearchListIndexItemCompare,
                         &indexItem))
    {
        cmData->sdcVar.currentElement = indexItem.index;
        return TRUE;
    }

    return FALSE;
}

static void csrBtCmSdcReleaseResourcesCfmMsgSend(CsrSchedQid appHandle, CsrUint8 server, CsrBtDeviceAddr deviceAddr)
{
    CsrBtCmSdcReleaseResourcesCfm  * prim;
    prim                        = (CsrBtCmSdcReleaseResourcesCfm *) CsrPmemAlloc(sizeof(CsrBtCmSdcReleaseResourcesCfm));
    prim->type                  = CSR_BT_CM_SDC_RELEASE_RESOURCES_CFM;
    prim->localServerChannel    = server;
    prim->deviceAddr            = deviceAddr;
    CsrBtCmPutMessage(appHandle, prim);
}
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

static void csrBtCmSdcServiceSearchHandler(cmInstanceData_t *cmData)
{
    CsrUint16            i = 0;
    CsrUint8           * searchPtr;
    sdcSearchElement   * theElement;

    theElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t*) &cmData->sdcVar.sdcSearchList,
                                                              cmData->sdcVar.currentElement);

    if (theElement->uuidType == CSR_BT_CM_SDC_UUID32)
    {
        CsrBtUuid32              service;

        service         = theElement->uuid.uuid32[cmData->sdcVar.uuidIndex];

        searchPtr       = (CsrUint8 *) CsrPmemAlloc(7);
        searchPtr[i++]  = 0x35;
        searchPtr[i++]  = 0x05;
        searchPtr[i++]  = 0x1a;
        searchPtr[i++]  = (CsrUint8) ((service    >> 24) & 0xff);
        searchPtr[i++]  = (CsrUint8) ((service    >> 16) & 0xff);
        searchPtr[i++]  = (CsrUint8) ((service    >> 8 ) & 0xff);
        searchPtr[i++]  = (CsrUint8) ((service         ) & 0xff);
    }
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
    else if (theElement->uuidType == CSR_BT_CM_SDC_UUID128)
    {
        CsrUintFast8     t;

        searchPtr       = (CsrUint8 *) CsrPmemAlloc(19);
        searchPtr[i++]  = 0x35;
        searchPtr[i++]  = 0x11;
        searchPtr[i++]  = 0x1c;

        for (t = 0; t < 16; t++)
        {
            searchPtr[i++]  = (CsrUint8) theElement->uuid.uuid128[cmData->sdcVar.uuidIndex][t];
        }
    }
#endif
    else /* if (theElement->uuidSet) */
    {
        searchPtr = CsrMemDup(theElement->uuid.uuidSet, theElement->uuidCount);
        i = theElement->uuidCount;
    }
    sdc_service_search_req(CSR_BT_CM_IFACEQUEUE, &(theElement->deviceAddr), i,
                           searchPtr, MAX_RECORDS_TO_RETURN);
}

static void cmSdcServiceSearchAttrHandler(cmInstanceData_t *cmData)
{
    sdcSearchElement    *curElement;
    CsrUint8             curUuidIndex;
    cmSdcSearchAttrInfo  curAttrInfo;
    CsrUint8            *attrList = NULL, *searchPtr = NULL;
    CsrUintFast16        i = 0, j = 0;

    curElement   = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t*) &cmData->sdcVar.sdcSearchList,
                                                               cmData->sdcVar.currentElement);
    curUuidIndex = cmData->sdcVar.uuidIndex;
    curAttrInfo  = curElement->attrInfoList[curUuidIndex];

    if (curElement->uuidType == CSR_BT_CM_SDC_UUID32)
    {
        CsrBtUuid32 service = curElement->uuid.uuid32[curUuidIndex];

        /* Form the search pattern for the requested 32-bits service UUID */
        searchPtr      = (CsrUint8 *) CsrPmemAlloc(7);
        searchPtr[i++] = 0x35;                 /* type = DataElSeq */
        searchPtr[i++] = 0x05;                 /* size = 5 bytes in DataElSeq */
        searchPtr[i++] = 0x1a;                 /* 32-bit UUID followed by 4 bytes of UUID value */
        searchPtr[i++] = (CsrUint8) ((service    >> 24) & 0xff);
        searchPtr[i++] = (CsrUint8) ((service    >> 16) & 0xff);
        searchPtr[i++] = (CsrUint8) ((service    >> 8 ) & 0xff);
        searchPtr[i++] = (CsrUint8) ((service         ) & 0xff);
    }

#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
    else if (curElement->uuidType == CSR_BT_CM_SDC_UUID128)
    {
        CsrUintFast8     t;

        /* Form the search pattern for the requested 128-bits service UUID */
        searchPtr      = (CsrUint8 *) CsrPmemAlloc(19);
        searchPtr[i++] = 0x35;                 /* type = DataElSeq */
        searchPtr[i++] = 0x11;                 /* size = 17 bytes in DataElSeq */
        searchPtr[i++] = 0x1c;                 /* 128-bit UUID followed by 16 bytes of UUID value*/

        for (t = 0; t < 16; t++)
        {
            searchPtr[i++] = (CsrUint8) curElement->uuid.uuid128[curUuidIndex][t];
        }
    }
#endif

    if (curAttrInfo.attrList && curAttrInfo.noOfAttr > 0)
    {
        /* For each attribute we occupy 3 places in the array, so multiply the number of attributes by 3 */
        CsrUintFast16 attrListSize = curAttrInfo.noOfAttr * 0x03, k;

        /* Add 2 to the attribute size to compensate for the Data element sequence and attribute size */
        attrList = (CsrUint8 *) CsrPmemAlloc(attrListSize + 2);

        /* Form the search pattern for the requested attributes */
        attrList[j++]     = 0x35;       /* type = DataElSeq */
        attrList[j++]     = (CsrUint8) attrListSize;

        for(k=0; j < (attrListSize + 2) && k < (curAttrInfo.noOfAttr); k++)
        {
            attrList[j++] = 0x09;       /* attribute ID followed by 2 bytes of attribute value */
            attrList[j++] = (CsrUint8) ((curAttrInfo.attrList[k] & 0xff00) >> 8);
            attrList[j++] = (CsrUint8) (curAttrInfo.attrList[k] & 0x00ff);
        }
    }
    sdc_service_search_attribute_req(CSR_BT_CM_IFACEQUEUE, &(curElement->deviceAddr), i,
                                     searchPtr, j, attrList, L2CA_MTU_DEFAULT);
}


static void cmSdcServiceSearchAttrComplete(cmInstanceData_t *cmData, sdcSearchElement *theElement)
{
    /* In case of outgoing RFCOMM connection the resources will be released
       only after the connection attempt has been made */
    if (!theElement->obtainServerChannels)
    {
        removeElementFromSdcList(cmData);
    }

    CSR_BT_CM_STATE_CHANGE(cmData->sdcVar.state, CSR_BT_CM_SDC_STATE_IDLE);
    CmSdcManagerLocalQueueHandler(cmData);
}

static void cmSdcServiceSearchAttrContinueOrDoneHandler(cmInstanceData_t *cmData, SDC_SERVICE_SEARCH_ATTRIBUTE_CFM_T *prim)
{
    /* Check whether the service record is not for the last service uuid */
    if (cmData->sdcVar.uuidIndex > 0 || prim->more_to_come)
    {
        /* Send an indication above to store the received data */
        cmSdcServiceSearchAttrIndSend(cmData);

        /* Check if no more data is pending from Bluestack */
        if (!prim->more_to_come)
        {
            /* Increase the indices for the next service search attribute request */
            cmData->sdcVar.currentIndex++;

            cmData->sdcVar.currentServiceIndex = 0;

            /* uuidIndex started from the last uuid so decrease the counter */
            cmData->sdcVar.uuidIndex--;

            /* Now proceed with the service search attribute request
             * for the next service UUID */
            cmSdcServiceSearchAttrHandler(cmData);
        }
    }
    else
    {
        sdcSearchElement *theElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList,
                                                                                   cmData->sdcVar.currentElement);

        SDC_SERVICE_SEARCH_ATTRIBUTE_CFM_T * sdcPrim = (SDC_SERVICE_SEARCH_ATTRIBUTE_CFM_T *) cmData->recvMsgP;

        cmSdcServiceSearchAttrCfmMsgSend(cmData,
                                         theElement->appHandle,
                                         theElement->deviceAddr,
                                         sdcPrim->response,
                                         CSR_BT_SUPPLIER_SDP_SDC);

        cmSdcServiceSearchAttrComplete(cmData, theElement);
    }
}

static void csrBtCmServiceSearchContinueOrDoneHandler(cmInstanceData_t *cmData, sdcSearchElement * theElement)
{
    if (cmData->sdcVar.uuidIndex > 0)
    {
        cmData->sdcVar.uuidIndex--;
        csrBtCmSdcServiceSearchHandler(cmData);
    }
    else
    {
        if (theElement->searchPtrArray[0] && cmData->sdcVar.validSearchResult)
        {
            CSR_BT_CM_STATE_CHANGE(cmData->sdcVar.state,
                                   CSR_BT_CM_SDC_STATE_ATTR);
            CsrBtCmSdcSearchCfmMsgSend(theElement->appHandle, cmData->sdcVar.localServer, theElement->deviceAddr);
        }
        else
        { /* The none of UUID's did match the UUID's defined under the Service
             Class ID List attribute                                            */
            cmData->smVar.arg.result.code     = (CsrBtResultCode) SDC_NO_RESPONSE_DATA;
            cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_SDP_SDC;
            sdc_close_search_req(CSR_BT_CM_IFACEQUEUE);
        }
    }
}

static void csrBtCmSdcOpenCfmMsgSend(CsrSchedQid appHandle, CsrBtDeviceAddr deviceAddr)
{
    CsrBtCmSdcOpenCfm   * prim;

    prim                        = (CsrBtCmSdcOpenCfm *) CsrPmemAlloc(sizeof(CsrBtCmSdcOpenCfm));
    prim->type                  = CSR_BT_CM_SDC_OPEN_CFM;
    prim->deviceAddr            = deviceAddr;
    CsrBtCmPutMessage(appHandle, prim);
}


static void csrBtCmSdcAttrRequestInSearchState(CsrBtDeviceAddr addr, CsrUint16 attrId, CsrBtUuid32 serviceHandle)
{
    CsrUint8 * attrList;

    attrList        = (CsrUint8 *) CsrPmemAlloc(5);
    attrList[0]     = 0x35;
    attrList[1]     = 0x03;
    attrList[2]     = 0x09;
    attrList[3]     = (CsrUint8) ((attrId & 0xff00) >> 8);
    attrList[4]     = (CsrUint8) (attrId & 0x00ff);
    sdc_service_attribute_req(CSR_BT_CM_IFACEQUEUE, &addr, serviceHandle, 5, attrList, MAX_BYTES_TO_RETURN);
}

static void csrBtCmSdcSearchResultHandler(cmInstanceData_t *cmData, sdcSearchElement * theElement)
{
    CsrUintFast8 i;
    sdcList_t *searchList;

    CsrUint8 validResults = 0;
    searchList = theElement->searchPtrArray[cmData->sdcVar.currentIndex];

    for (i = 0; i < searchList->resultListSize; i++)
    {
        if (CSR_BT_CM_SDC_RESULT_IS_VALID(searchList->valid, i))
        {
            if (i != validResults)
            {
                searchList->resultList[validResults].serviceHandle = searchList->resultList[i].serviceHandle;
                searchList->resultList[validResults].serverChannel = searchList->resultList[i].serverChannel;
                CSR_BT_CM_SDC_RESULT_VALID_SET(searchList->valid, validResults);
            }
            validResults++;
        }
    }
    searchList->resultListSize = validResults;

    if (validResults > 0)
    {
        CsrBtCmSdcSearchIndMsgSend(cmData, theElement, validResults);
    }
}


static void csrBtCmRetrieveAttrContinueOrDoneSearchState(cmInstanceData_t *cmData, sdcSearchElement  * theElement)
{
    if ((cmData->sdcVar.currentServiceIndex + 1) < theElement->searchPtrArray[cmData->sdcVar.currentIndex]->resultListSize)
    {
        CsrBtUuid32              serviceHandle;

        cmData->sdcVar.currentServiceIndex++;

        serviceHandle = theElement->searchPtrArray[cmData->sdcVar.currentIndex]->resultList[cmData->sdcVar.currentServiceIndex].serviceHandle;
        cmData->sdcVar.attrId = CSR_BT_SERVICE_CLASS_ID_LIST;
        csrBtCmSdcAttrRequestInSearchState(theElement->deviceAddr,
                                           cmData->sdcVar.attrId,
                                           serviceHandle);
    }
    else
    {
        csrBtCmSdcSearchResultHandler(cmData, theElement);
        cmData->sdcVar.currentIndex++;
        csrBtCmServiceSearchContinueOrDoneHandler(cmData, theElement);
    }
}

static void csrBtCmAttrCfmSuccessHandlerInSdcSearchState(cmInstanceData_t * cmData, SDC_SERVICE_ATTRIBUTE_CFM_T * prim)
{
    sdcSearchElement *theElement;
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    CsrBtUuid32 serviceHandle;
#endif
    theElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t*) &cmData->sdcVar.sdcSearchList,
                                                             cmData->sdcVar.currentElement);
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    serviceHandle = theElement->searchPtrArray[cmData->sdcVar.currentIndex]->resultList[cmData->sdcVar.currentServiceIndex].serviceHandle;
#endif

    if (cmData->sdcVar.attrId == CSR_BT_SERVICE_CLASS_ID_LIST)
    {
        CsrBool result = FALSE;
        if (theElement->uuidType == CSR_BT_CM_SDC_UUID32)
        {
            result = CsrBtSdcFindServiceClassUuid(prim->size_attr_list,
                                                  prim->attr_list,
                                                  theElement->uuid.uuid32[cmData->sdcVar.uuidIndex]);
        }
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
        else
        {
            result = CsrBtSdcFindServiceClass128Uuid(prim->size_attr_list,
                                                     prim->attr_list,
                                                     theElement->uuid.uuid128[cmData->sdcVar.uuidIndex]);
        }
#endif
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
        if (result && theElement->obtainServerChannels)
        {
            if (theElement->extendedProtocolDescriptorList)
            {
                cmData->sdcVar.attrId   = CSR_BT_ADDITIONAL_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER;
            }
            else
            {
                cmData->sdcVar.attrId   = CSR_BT_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER;
            }
            csrBtCmSdcAttrRequestInSearchState(theElement->deviceAddr, cmData->sdcVar.attrId, serviceHandle);
        }
        else
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
        {
            if (cmData->sdcVar.currentServiceIndex < CSR_BT_CM_SDC_RESULT_COUNT_MAX)
            {/* Do not set/unset the bit in the "valid" array if the UUID index is more than the max count */
                if (result)
                {
                    CSR_BT_CM_SDC_RESULT_VALID_SET(theElement->searchPtrArray[cmData->sdcVar.currentIndex]->valid,
                                                   cmData->sdcVar.currentServiceIndex);
                }
                else
                {
                    CSR_BT_CM_SDC_RESULT_VALID_UNSET(theElement->searchPtrArray[cmData->sdcVar.currentIndex]->valid,
                                                     cmData->sdcVar.currentServiceIndex);
                }
            }
            csrBtCmRetrieveAttrContinueOrDoneSearchState(cmData, theElement);
        }
    }
    else
    {
        CsrUint8   server;

        if (theElement->extendedProtocolDescriptorList)
        {
            server          = CsrBtSdcGetExtendedRfcommServerChannel(prim->size_attr_list, prim->attr_list);
        }
        else
        {
            server          = CsrBtSdcGetRfcommServerChannel(prim->size_attr_list, prim->attr_list);
        }
        if (server != CM_ERROR)
        {
            theElement->searchPtrArray[cmData->sdcVar.currentIndex]->resultList[cmData->sdcVar.currentServiceIndex].serverChannel = server;
            if (cmData->sdcVar.currentServiceIndex < CSR_BT_CM_SDC_RESULT_COUNT_MAX)
            {/* Do not set the bit in the "valid" array if the UUID index is more than the max count */
                CSR_BT_CM_SDC_RESULT_VALID_SET(theElement->searchPtrArray[cmData->sdcVar.currentIndex]->valid,
                                               cmData->sdcVar.currentServiceIndex);
            }
        }
        csrBtCmRetrieveAttrContinueOrDoneSearchState(cmData, theElement);
    }
}

static void csrBtCmSdcAttrCfmInSdcSearchState(cmInstanceData_t * cmData, SDC_SERVICE_ATTRIBUTE_CFM_T * prim)
{
    cmData->smVar.arg.result.code     = (CsrBtResultCode) prim->response;
    cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_SDP_SDC;

    switch (prim->response)
    {
        case SDC_RESPONSE_SUCCESS:
        {
            csrBtCmAttrCfmSuccessHandlerInSdcSearchState(cmData, prim);
            break;
        }
        case SDC_CON_DISCONNECTED:
        case SDC_CONNECTION_ERROR_UNKNOWN:
        case SDC_CONFIGURE_ERROR:
        case SDC_RESPONSE_TIMEOUT_ERROR:
        {
            sdc_close_search_req(CSR_BT_CM_IFACEQUEUE);
            break;
        }
        default :
        {
            if (prim->response >= L2CA_RESULT_NOT_READY &&
                prim->response <= L2CA_RESULT_LINK_TRANSFERRED)
            {
                /* All L2CAP failures are considered to be SDC failure */
                sdc_close_search_req(CSR_BT_CM_IFACEQUEUE);
            }
            else
            {
                sdcSearchElement *currentElement;

                currentElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t*) &cmData->sdcVar.sdcSearchList,
                                                                             cmData->sdcVar.currentElement);
                csrBtCmRetrieveAttrContinueOrDoneSearchState(cmData,
                                                             currentElement);
                break;
            }
        }
    }
}

static CsrBool insertSdcElement(cmInstanceData_t *cmData,
                                CsrSchedQid appHandle,
                                CsrBtDeviceAddr deviceAddr,
                                CsrUint8 uuidType,
                                void *uuids,
                                CsrUint8 uuidCount,
                                cmSdcSearchAttrInfo *attrInfoList,
                                CsrBool obtainServerChannel,
                                CsrBool readAttrDirect,
                                CsrBool extendedProtocolList,
                                CsrBool extendedUuidSearch)
{
    if (uuidCount)
    {
        sdcResultIndexItem indexItem;
        CsrSize allocSize = sizeof(sdcSearchElement) + ((uuidCount - 1) * sizeof(sdcList_t*));

        indexItem.appHandle = appHandle;
        indexItem.deviceAddr = deviceAddr;
        indexItem.index = 0;

        if (!CsrCmnListSearch((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList,
                              CsrBtCmSdcSearchListIndexItemCompare,
                              &indexItem))
        {
            sdcSearchElement *theElement = (sdcSearchElement *) CsrCmnListElementAddLast((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList,
                                                                                         allocSize);
            theElement->readAttrDirect = readAttrDirect;
            theElement->dmOpenResult = FALSE;
            theElement->uuidType = uuidType;

            if (uuidType == CSR_BT_CM_SDC_UUID32)
            {
                theElement->uuid.uuid32 = uuids;
            }
            else if (uuidType == CSR_BT_CM_SDC_UUID128)
            {
                theElement->uuid.uuid128 = uuids;
            }
            else
            {
                theElement->uuid.uuidSet = uuids;
            }
            theElement->uuidCount = uuidCount;
            theElement->attrInfoList = attrInfoList;
            if (attrInfoList)
            {
                theElement->serviceInfoList->serverChannelList = (CsrUint8*) CsrPmemAlloc(sizeof (CsrUint8*) * CSR_BT_CM_SDC_RESULT_COUNT_MAX);
                theElement->serviceInfoList->serviceHandleList = (CsrUint32*) CsrPmemAlloc(sizeof (CsrUint32*) * CSR_BT_CM_SDC_RESULT_COUNT_MAX);
            }
            else
            {
                theElement->serviceInfoList->serverChannelList = NULL;
                theElement->serviceInfoList->serviceHandleList = NULL;
            }
            theElement->obtainServerChannels = obtainServerChannel;
            theElement->appHandle = appHandle;
            theElement->deviceAddr = deviceAddr;
            theElement->extendedProtocolDescriptorList = extendedProtocolList;
            theElement->extendedUuidSearch = extendedUuidSearch;
            theElement->assignedServiceHandle = 1;

            cmData->sdcVar.uuidIndex = (CsrUint8) (uuidCount - 1);
            cmData->sdcVar.currentElement = indexItem.index;
            cmData->sdcVar.currentIndex = 0;
            cmData->sdcVar.currentServiceIndex = 0;

            return TRUE;
        }
    }

    return FALSE;
}

static void searchRequestHandler(cmInstanceData_t *cmData,
                                 CsrSchedQid appHandle,
                                 CsrBtDeviceAddr deviceAddr,
                                 CsrUint8 uuidType,
                                 void *uuids,
                                 CsrUint8 uuidCount,
                                 cmSdcSearchAttrInfo *attrInfoList,
                                 CsrBool obtainServerChannel,
                                 CsrBool readAttrDirect,
                                 CsrBool extendedProtocolList,
                                 CsrBool extendedUuidSearch)
{
    if (insertSdcElement(cmData,
                         appHandle,
                         deviceAddr,
                         uuidType,
                         uuids,
                         uuidCount,
                         attrInfoList,
                         obtainServerChannel,
                         readAttrDirect,
                         extendedProtocolList,
                         extendedUuidSearch))
    {
        cmData->sdcVar.validSearchResult = FALSE;
        cmData->sdcVar.cancelPending     = FALSE;

        if (!CsrBtCmDmWriteKnownCacheParams(cmData, deviceAddr, SDC_PLAYER))
        {
            CsrBtCmSdcStartHandler(cmData, deviceAddr);
        }
    }
    else
    {
        if (attrInfoList)
        { /* Enhanced SDP: attrInfoList will be populated by the initiator only 
             in case of service search attribute request */
            CsrUint8 i;
            for (i = 0; i < uuidCount; i++)
            {
                if (attrInfoList[i].attrList)
                {
                    CsrPmemFree(attrInfoList[i].attrList);
                }
            }
            CsrPmemFree(attrInfoList);

            /* SDP OPEN and CLOSE operations are not performed explicitly by Synergy */
            cmSdcServiceSearchAttrCfmMsgSend(cmData,
                                             appHandle,
                                             deviceAddr,
                                             CSR_BT_RESULT_CODE_CM_COMMAND_DISALLOWED,
                                             CSR_BT_SUPPLIER_CM);
        }
        else
        { /* Legacy design */
            csrBtCmSdcCloseIndMsgSend(appHandle,
                                      cmData->sdcVar.localServer,
                                      deviceAddr,
                                      CSR_BT_RESULT_CODE_CM_COMMAND_DISALLOWED,
                                      CSR_BT_SUPPLIER_CM);
        }

        CsrPmemFree(uuids);
        CmSdcManagerLocalQueueHandler(cmData);
    }
}

static void csrBtCmServiceSearchSuccessHandler(cmInstanceData_t *cmData, SDC_SERVICE_SEARCH_CFM_T *prim)
{
    sdcSearchElement *theElement;

    theElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t*) &cmData->sdcVar.sdcSearchList,
                                                             cmData->sdcVar.currentElement);
    if (cmData->sdcVar.currentIndex < theElement->uuidCount)
    {
        CsrUintFast8 i, offset = 0;
        CsrUint8 resultCount = (prim->num_recs_ret < CSR_BT_CM_SDC_RESULT_COUNT_MAX) ?
                        prim->num_recs_ret : CSR_BT_CM_SDC_RESULT_COUNT_MAX;
        CsrUint16 allocSize = sizeof(sdcList_t) + (sizeof(sdcResult_t) * (resultCount - 1));

        theElement->searchPtrArray[cmData->sdcVar.currentIndex] = (sdcList_t *) CsrPmemZalloc(allocSize);
        theElement->searchPtrArray[cmData->sdcVar.currentIndex]->resultListSize = resultCount;

        for (i = 0; i < resultCount; i++)
        {
            theElement->searchPtrArray[cmData->sdcVar.currentIndex]->resultList[i].serviceHandle = CSR_GET_UINT32_FROM_BIG_ENDIAN(&prim->rec_list[offset]);
            offset += 4;
            theElement->searchPtrArray[cmData->sdcVar.currentIndex]->resultList[i].serverChannel = CSR_BT_NO_SERVER;

            if (theElement->extendedUuidSearch)
            {
                CSR_BT_CM_SDC_RESULT_VALID_SET(theElement->searchPtrArray[cmData->sdcVar.currentIndex]->valid,
                                               i);
            }
            else
            {
                CSR_BT_CM_SDC_RESULT_VALID_UNSET(theElement->searchPtrArray[cmData->sdcVar.currentIndex]->valid,
                                                 i);
            }
        }

        if (theElement->uuidType == CSR_BT_CM_SDC_UUID_SET)
        {
            csrBtCmSdcServiceSearchCfmMsgSend(cmData,
                                              theElement->searchPtrArray[cmData->sdcVar.currentIndex]->resultList,
                                              resultCount,
                                              CSR_BT_RESULT_CODE_CM_SUCCESS,
                                              CSR_BT_SUPPLIER_CM);
            cmData->sdcVar.currentIndex++;
            CSR_BT_CM_STATE_CHANGE(cmData->sdcVar.state,
                                   CSR_BT_CM_SDC_STATE_ATTR);
        }
        else
        {
            if (!theElement->extendedUuidSearch)
            { /* The UUID must only be consider for valid, if and only if the
                 UUID is contained under the Service Class ID List attribute    */
                cmData->sdcVar.currentServiceIndex  = 0;
                cmData->sdcVar.attrId               = CSR_BT_SERVICE_CLASS_ID_LIST;
                csrBtCmSdcAttrRequestInSearchState(theElement->deviceAddr,
                                                   cmData->sdcVar.attrId,
                                                   theElement->searchPtrArray[cmData->sdcVar.currentIndex]->resultList[0].serviceHandle);
            }
            else
            { /* The UUID must be consider valid, if the UUID is contained
                 within any of the service record's attribute values            */
                csrBtCmSdcSearchResultHandler(cmData, theElement);
                cmData->sdcVar.currentIndex++;
                csrBtCmServiceSearchContinueOrDoneHandler(cmData, theElement);
            }
        }
    }
    else
    {
        sdc_close_search_req(CSR_BT_CM_IFACEQUEUE);
    }
}

static void cmSdcGetServiceHandleAndServerChannel(cmInstanceData_t* cmData, SDC_SERVICE_SEARCH_ATTRIBUTE_CFM_T* prim)
{
    sdcSearchElement* currentElement;

    currentElement = (sdcSearchElement*)CsrCmnListGetFromIndex((CsrCmnList_t*)&cmData->sdcVar.sdcSearchList,
                      cmData->sdcVar.currentElement);

    if (prim->attr_list &&
        currentElement->obtainServerChannels)
    {
        CsrUint8 server = CM_ERROR;

        if (currentElement->extendedProtocolDescriptorList)
        {
            server = CsrBtSdcGetExtendedRfcommServerChannel(prim->size_attr_list, prim->attr_list);
        }
        else
        {
            server = CsrBtSdcGetRfcommServerChannel(prim->size_attr_list, prim->attr_list);
        }

        if (server != CM_ERROR)
        {
            CsrUint32 shandle = INVALID_SERVICE_HANDLE;
            CsrBool shandlePresent = FALSE;

            /* Obtain the server handle */
            shandlePresent = CsrBtSdcGetUint32Direct(prim->size_attr_list,
                                                     prim->attr_list,
                                                     CSR_BT_SERVICE_RECORD_HANDLE_ATTRIBUTE_IDENTIFIER,
                                                     &shandle);
            if (!shandlePresent)
            {  /* Generate service handle to store/fetch remote server channel for serice level connection,
                  when valid service handle is not available from remote. */
                shandle = ASSIGN_SERVICE_HANDLE(currentElement);
            }

            /* Store the server channel and shandle inside the search element array 
               until we receive all the data from Bluestack */
            currentElement->serviceInfoList->serverChannelList[cmData->sdcVar.currentServiceIndex] = server;
            currentElement->serviceInfoList->serviceHandleList[cmData->sdcVar.currentServiceIndex] = shandle;
            cmData->sdcVar.currentServiceIndex++;

            if (!prim->more_to_come)
            {
                /* We have received the attribute list for all the supported instances.
                   Insert the service handle and server channel inside the result list */
                CsrUint8 i, resultCount;
                CsrUint16 allocSize;
                sdcList_t* currentSearchPtrArray;

                resultCount = (cmData->sdcVar.currentServiceIndex < CSR_BT_CM_SDC_RESULT_COUNT_MAX) ?
                               cmData->sdcVar.currentServiceIndex : CSR_BT_CM_SDC_RESULT_COUNT_MAX;
                allocSize   = sizeof(sdcList_t) + (sizeof(sdcResult_t) * (resultCount - 1));

                currentElement->searchPtrArray[cmData->sdcVar.currentIndex] = (sdcList_t *) CsrPmemZalloc(allocSize);
                currentSearchPtrArray = currentElement->searchPtrArray[cmData->sdcVar.currentIndex];
                currentSearchPtrArray->resultListSize = resultCount;

                /* Copy the server channel and shandle info for all the attribute lists received into
                   the result list array */
                for (i = 0; i < cmData->sdcVar.currentServiceIndex; i++)
                {
                    shandle = currentElement->serviceInfoList->serviceHandleList[i];
                    server = currentElement->serviceInfoList->serverChannelList[i];

                    currentSearchPtrArray->resultList[i].serverChannel = server;
                    currentSearchPtrArray->resultList[i].serviceHandle = shandle;
                }
            }
        }
    }
}

static void cmSdcServiceSearchAttrSuccessHandler(cmInstanceData_t *cmData, SDC_SERVICE_SEARCH_ATTRIBUTE_CFM_T *prim)
{
    /* The required attribute list has been received.
       Check if we need to store the service handle and
       remote server channel */
    cmSdcGetServiceHandleAndServerChannel(cmData, prim);
    cmSdcServiceSearchAttrContinueOrDoneHandler(cmData, prim);
}

void cmSdcServiceSearchAttrFailureHandler(cmInstanceData_t *cmData)
{
    sdcSearchElement *theElement = (sdcSearchElement*)CsrCmnListGetFromIndex((CsrCmnList_t*)&cmData->sdcVar.sdcSearchList,
                                                                             cmData->sdcVar.currentElement);

    if (theElement)
    {
        cmSdcServiceSearchAttrCfmMsgSend(cmData,
                                         theElement->appHandle,
                                         theElement->deviceAddr,
                                         cmData->smVar.arg.result.code,
                                         cmData->smVar.arg.result.supplier);

        cmSdcServiceSearchAttrComplete(cmData, theElement);
    }
}

static void removeElementFromSdcList(cmInstanceData_t * cmData)
{
    sdcSearchElement *theElement;

    theElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList,
                                                             cmData->sdcVar.currentElement);

    if (theElement)
    {
        CsrBtCmSdcSearchListElemRemove((CsrCmnListElm_t *) theElement, NULL);
        CsrCmnListElementRemove((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList,
                                (CsrCmnListElm_t *) theElement);

        cmData->sdcVar.currentElement = CSR_BT_CM_SDC_SEARCH_ELEM_INVALID;
    }
}

static void csrBtCmSdcCancelledHandler(cmInstanceData_t *cmData)
{
    sdcSearchElement *currentElement;

    currentElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList,
                                                                 cmData->sdcVar.currentElement);

    cmData->sdcVar.cancelPending  = FALSE;

    if (currentElement->attrInfoList)
    { /* Enhanced SDP: attrInfoList will be populated by the initiator only 
         in case of service search attribute request */

        /* SDP OPEN and CLOSE operations are not performed explicitly by Synergy */
        cmSdcServiceSearchAttrCfmMsgSend(cmData,
                                         currentElement->appHandle,
                                         currentElement->deviceAddr,
                                         CSR_BT_RESULT_CODE_CM_CANCELLED,
                                         CSR_BT_SUPPLIER_CM);
    }
    else
    { /* Legacy design */
        csrBtCmSdcCloseIndMsgSend(currentElement->appHandle,
                                  cmData->sdcVar.localServer,
                                  currentElement->deviceAddr,
                                  CSR_BT_RESULT_CODE_CM_CANCELLED,
                                  CSR_BT_SUPPLIER_CM);
    }

    removeElementFromSdcList(cmData);
    CSR_BT_CM_STATE_CHANGE(cmData->sdcVar.state, CSR_BT_CM_SDC_STATE_IDLE);
    CmSdcManagerLocalQueueHandler(cmData);
}

void CsrBtCmSdcStartHandler(cmInstanceData_t *cmData, CsrBtDeviceAddr deviceAddr)
{
    sdcSearchElement *theElement;

    theElement = (sdcSearchElement*)CsrCmnListGetFromIndex((CsrCmnList_t*)&cmData->sdcVar.sdcSearchList,
                                                           cmData->sdcVar.currentElement);

    if (!cmData->sdcVar.cancelPending)
    {
        CSR_BT_CM_STATE_CHANGE(cmData->sdcVar.state, CSR_BT_CM_SDC_STATE_SEARCH);
        if (theElement->attrInfoList)
        { /* Enhanced SDP: attrInfoList will be populated by the initiator only 
             in case of service search attribute request */

            /* SDP OPEN and CLOSE operations are not performed explicitly by Synergy */
            cmSdcServiceSearchAttrHandler(cmData);
        }
        else
        { /* Legacy design */
            cmData->sdcVar.sdcMsgPending = TRUE;
            sdc_open_search_req(CSR_BT_CM_IFACEQUEUE, &deviceAddr);
        }
    }
    else
    {
        csrBtCmSdcCancelledHandler(cmData);
    }
}

void CsrBtCmSdcSearchReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSdcSearchReq * prim;

    prim = (CsrBtCmSdcSearchReq *) cmData->recvMsgP;
    cmData->sdcVar.localServer  = CSR_BT_NO_SERVER;

    searchRequestHandler(cmData,
                         prim->appHandle,
                         prim->deviceAddr,
                         CSR_BT_CM_SDC_UUID32,
                         prim->serviceList,
                         prim->serviceListSize,
                         NULL, /* No attributes requsted */
                         FALSE,
                         FALSE,
                         FALSE,
                         prim->extendedUuidSearch);
    prim->serviceList = NULL;
}

void CmSdcServiceSearchAttrReqHandler(cmInstanceData_t *cmData)
{
    CmSdcServiceSearchAttrReq * prim;

    prim = (CmSdcServiceSearchAttrReq *) cmData->recvMsgP;

    cmData->sdcVar.localServer = prim->localServerChannel;

    if (prim->localServerChannel != CSR_BT_NO_SERVER)
    {
        /* Local server channel exists. This is an HF outgoing connection */
        searchRequestHandler(cmData,
                             prim->appHandle,
                             prim->deviceAddr,
                             prim->uuidType,
                             prim->svcSearchAttrInfoList->serviceList,
                             prim->svcSearchAttrInfoList->serviceListSize,
                             prim->svcSearchAttrInfoList->attrInfoList,
                             TRUE, /* Set this to TRUE to obtain the remote server channel required for outgoing connection */
                             FALSE,
                             prim->extendedUuidSearch, /* Flag to search for additional protocol descriptor list attribute */
                             prim->extendedUuidSearch);
    }
    else
    {
        /* No local server sent as part of params, assign no server and
         * proceed with service discovery for incoming connection */
        searchRequestHandler(cmData,
                             prim->appHandle,
                             prim->deviceAddr,
                             prim->uuidType,
                             prim->svcSearchAttrInfoList->serviceList,
                             prim->svcSearchAttrInfoList->serviceListSize,
                             prim->svcSearchAttrInfoList->attrInfoList,
                             FALSE, /* Set this to FALSE as this is an incoming connection and remote server channel is not required */
                             FALSE,
                             FALSE,
                             prim->extendedUuidSearch);
    }
}

#ifdef CSR_BT_INSTALL_CM_PRI_SDC
void CsrBtCmSdcServiceSearchReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSdcServiceSearchReq * prim;

    prim = (CsrBtCmSdcServiceSearchReq *) cmData->recvMsgP;
    cmData->sdcVar.localServer  = CSR_BT_NO_SERVER;

    searchRequestHandler(cmData,
                         prim->appHandle,
                         prim->deviceAddr,
                         CSR_BT_CM_SDC_UUID_SET,
                         prim->uuidSet,
                         (CsrUint8) prim->uuidSetLength,
                         NULL, /* No attributes requsted */
                         FALSE,
                         FALSE,
                         FALSE,
                         FALSE);
    prim->uuidSet = NULL;
}
#endif

#ifdef CSR_BT_INSTALL_CM_PRI_SDC
void CsrBtCmSdcOpenReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSdcOpenReq   * prim;

    prim = (CsrBtCmSdcOpenReq *) cmData->recvMsgP;

    cmData->sdcVar.localServer  = CSR_BT_NO_SERVER;
    searchRequestHandler(cmData,
                         prim->appHandle,
                         prim->deviceAddr,
                         CSR_BT_CM_SDC_UUID_NONE,
                         NULL,
                         1,
                         NULL, /* No attributes requsted */
                         FALSE,
                         TRUE,
                         FALSE,
                         FALSE);

}
#endif
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
void CsrBtCmSdcUuid128SearchReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSdcUuid128SearchReq * prim;

    prim = (CsrBtCmSdcUuid128SearchReq *) cmData->recvMsgP;
    cmData->sdcVar.localServer  = CSR_BT_NO_SERVER;
    searchRequestHandler(cmData,
                         prim->appHandle,
                         prim->deviceAddr,
                         CSR_BT_CM_SDC_UUID128,
                         prim->serviceList,
                         prim->serviceListSize,
                         NULL, /* No attributes requsted */
                         FALSE,
                         FALSE,
                         FALSE,
                         FALSE);
    prim->serviceList = NULL;
}
#endif
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
void CsrBtCmSdcRfcSearchReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSdcRfcSearchReq * prim;

    prim = (CsrBtCmSdcRfcSearchReq *) cmData->recvMsgP;
    cmData->sdcVar.localServer  = prim->localServerChannel;
    searchRequestHandler(cmData,
                         prim->appHandle,
                         prim->deviceAddr,
                         CSR_BT_CM_SDC_UUID32,
                         prim->serviceList,
                         prim->serviceListSize,
                         NULL, /* No attributes requsted */
                         TRUE,
                         FALSE,
                         FALSE,
                         FALSE);
    prim->serviceList = NULL;
}

#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
void CsrBtCmSdcUuid128RfcSearchReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSdcUuid128RfcSearchReq * prim;

    prim = (CsrBtCmSdcUuid128RfcSearchReq *) cmData->recvMsgP;
    cmData->sdcVar.localServer  = prim->localServerChannel;
    searchRequestHandler(cmData,
                         prim->appHandle,
                         prim->deviceAddr,
                         CSR_BT_CM_SDC_UUID128,
                         prim->serviceList,
                         prim->serviceListSize,
                         NULL, /* No attributes requsted */
                         TRUE,
                         FALSE,
                         FALSE,
                         FALSE);
    prim->serviceList = NULL;
}
#endif
void CsrBtCmSdcRfcExtendedSearchReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSdcRfcExtendedSearchReq    * prim;

    prim = (CsrBtCmSdcRfcExtendedSearchReq *) cmData->recvMsgP;
    cmData->sdcVar.localServer  = prim->localServerChannel;
    searchRequestHandler(cmData,
                         prim->appHandle,
                         prim->deviceAddr,
                         CSR_BT_CM_SDC_UUID32,
                         prim->serviceList,
                         prim->serviceListSize,
                         NULL, /* No attributes requsted */
                         TRUE,
                         FALSE,
                         TRUE,
                         FALSE);
    prim->serviceList = NULL;
}


void CsrBtCmSdcReleaseResourcesReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSdcReleaseResourcesReq  * prim;

    prim                        = (CsrBtCmSdcReleaseResourcesReq *) cmData->recvMsgP;

    if (getSdcElement(cmData, prim->appHandle, prim->deviceAddr))
    {
        sdcSearchElement *currentElement;

        currentElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList,
                                                                     cmData->sdcVar.currentElement);

        if (currentElement->dmOpenResult)
        {
            TYPED_BD_ADDR_T ad;
            ad.addr = prim->deviceAddr;
            ad.type = CSR_BT_ADDR_PUBLIC;
            dm_acl_close_req(&ad,
                             0, /* flags */
                             0, /* reason */
                             NULL);
        }
        removeElementFromSdcList(cmData);
    }
    CmSdcManagerLocalQueueHandler(cmData);
    csrBtCmSdcReleaseResourcesCfmMsgSend(prim->appHandle, prim->localServerChannel, prim->deviceAddr);
}
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

static void cmCancelSavedServiceSearchMsg(cmInstanceData_t *cmData, CsrBtCmSdcCancelSearchReq *prim)
{
    CsrUint8  localServer;

    if (CmSdcCancelSdcManagerMsg(cmData, prim, &localServer))
    { /* SDC search request msg is found and removed from local SM queue */
        if (prim->typeToCancel == CM_SDC_SERVICE_SEARCH_ATTR_REQ)
        {
            /* Enhanced SDP case: SDP OPEN and CLOSE are not performed explicitly */
            cmSdcServiceSearchAttrCfmMsgSend(cmData,
                                             prim->appHandle,
                                             prim->deviceAddr,
                                             CSR_BT_RESULT_CODE_CM_CANCELLED,
                                             CSR_BT_SUPPLIER_CM);
        }
        else
        { /* Legacy design */
            csrBtCmSdcCloseIndMsgSend(prim->appHandle,
                                      localServer,
                                      prim->deviceAddr,
                                      CSR_BT_RESULT_CODE_CM_CANCELLED,
                                      CSR_BT_SUPPLIER_CM);
        }
    }
    else
    { /* Nothing to cancel just ignore */
        ;
    }
}

void CsrBtCmSdcCancelSearchReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSdcCancelSearchReq  * prim;

    prim                        = (CsrBtCmSdcCancelSearchReq *) cmData->recvMsgP;

    if (CsrCmnListGetCount((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList) &&
        cmData->sdcVar.lockMsg == prim->typeToCancel)
    {
        if (cmData->sdcVar.state == CSR_BT_CM_SDC_STATE_IDLE)
        {
            /* Intermediate state where current SDC search procedure is
             * either closed/completed or could not start and CM is about
             * to process house cleaning. Cancel the saved messages if any */
            cmCancelSavedServiceSearchMsg(cmData, prim);
        }
        else
        { /* CM is in the middle of a sdc search procedure */
            sdcSearchElement *theElement;

            theElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList,
                                                                     cmData->sdcVar.currentElement);

            if (theElement->appHandle == prim->appHandle && CsrBtBdAddrEq(&(theElement->deviceAddr), &prim->deviceAddr))
            { /* The the ongoing sdc search procedure is the one the
                 application trying to cancel */
                cmData->sdcVar.cancelPending     = TRUE;
                CsrBtCmScRejectedForSecurityReasonMsgSend(cmData,
                                                          prim->deviceAddr, TRUE);

                switch (cmData->sdcVar.state)
                {
                    case CSR_BT_CM_SDC_STATE_SEARCH:
                    {
                        if (cmData->sdcVar.sdcMsgPending)
                        { /* Must be a SDC open req */
                            CsrBtCmDmCancelPageOrDetach(cmData, prim->deviceAddr);
                        }
                        else
                        { /* A ACL connection is establish, must wait until CM
                             receives a SDC_X_CFM */
                            ;
                        }
                        break;
                    }
                    case CSR_BT_CM_SDC_STATE_ATTR:
                    {
                        if (!cmData->sdcVar.sdcMsgPending)
                        {
                            sdc_close_search_req(CSR_BT_CM_IFACEQUEUE);
                        }
                        else
                        { /* A ACL connection is establish, must wait until CM
                             receives a SDC_ATTRIBUTE_CFM */
                            ;
                        }
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            else
            { /* The ongoing sdc search procedure is not the one app wants to cancel */
                 cmCancelSavedServiceSearchMsg(cmData, prim);
            }
        }
    }
    else
    {
        cmCancelSavedServiceSearchMsg(cmData, prim);
    }
}

void CsrBtCmSdcAttrReqHandle(cmInstanceData_t *cmData)
{
    if (CsrCmnListGetCount((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList) &&
        cmData->sdcVar.state == CSR_BT_CM_SDC_STATE_ATTR &&
        !cmData->sdcVar.cancelPending)
    {
        CsrUint16                maxBytesToReturn;
        CsrUint8                 attrListSize;
        CsrUint8                 *attrList;
        CsrUint8                 dataElementSize;
        CsrBtCmSdcAttributeReq   *prim       = (CsrBtCmSdcAttributeReq *) cmData->recvMsgP;
        sdcSearchElement         *theElement;

        theElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList,
                                                                 cmData->sdcVar.currentElement);

#ifndef CSR_TARGET_PRODUCT_VM
        if (cmData->rfcBuild)
        {
            maxBytesToReturn = MAX_BYTES_TO_RETURN;
        }
        else
#endif
        {
            maxBytesToReturn = prim->maxBytesToReturn;
        }

        if (prim->upperRangeAttributeIdentifier == 0)
        {
            attrListSize    = 5;
            dataElementSize = CSR_BT_DATA_ELEMENT_SIZE_2_BYTES;
        }
        else
        {
            attrListSize    = 7;
            dataElementSize = CSR_BT_DATA_ELEMENT_SIZE_4_BYTES;
        }


        cmData->sdcVar.sdcMsgPending   = TRUE;

        attrList    = (CsrUint8 *) CsrPmemAlloc(attrListSize);
        attrList[0] = 0x35;
        attrList[1] = (CsrUint8)(attrListSize - 2);
        attrList[2] = (CsrUint8)(CSR_BT_DATA_ELEMENT_UNSIGNED_INTEGER_TYPE + dataElementSize);
        attrList[3] = (CsrUint8) ((prim->attributeIdentifier & 0xff00) >> 8);
        attrList[4] = (CsrUint8) (prim->attributeIdentifier & 0x00ff);
        if (attrListSize == 7)
        {
            attrList[5] = (CsrUint8) ((prim->upperRangeAttributeIdentifier & 0xff00) >> 8);
            attrList[6] = (CsrUint8) (prim->upperRangeAttributeIdentifier & 0x00ff);
        }
        sdc_service_attribute_req(CSR_BT_CM_IFACEQUEUE,
                                  &theElement->deviceAddr,
                                  prim->serviceHandle,
                                  attrListSize,
                                  attrList,
                                  maxBytesToReturn);/* Limit size if we are running on an RFC build */
    }
}

void CsrBtCmSdcCloseReqHandler(cmInstanceData_t *cmData)
{
    sdcSearchElement *currentElement;

    currentElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t*) &cmData->sdcVar.sdcSearchList,
                                                                 cmData->sdcVar.currentElement);
    if (CsrCmnListGetCount((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList) &&
        (cmData->sdcVar.state == CSR_BT_CM_SDC_STATE_ATTR ||
        (currentElement->attrInfoList && cmData->sdcVar.state == CSR_BT_CM_SDC_STATE_SEARCH)) &&
        !cmData->sdcVar.cancelPending)
    {

        CSR_BT_CM_STATE_CHANGE(cmData->sdcVar.state, CSR_BT_CM_SDC_STATE_CLOSE);
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
        if (currentElement->obtainServerChannels)
        {
            CsrBtTypedAddr ad;

            ad.addr = currentElement->deviceAddr;
            ad.type = CSR_BT_ADDR_PUBLIC;

            cmData->sdcVar.aclOpenRequested = TRUE;
            CsrBtCmAclOpenReqSend(CSR_BT_CM_IFACEQUEUE,
                                  ad,
                                  0);
        }
        else
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
        {
            sdc_close_search_req(CSR_BT_CM_IFACEQUEUE);
        }
    }
}

void CsrBtCmSdcOpenSearchCfmHandler(cmInstanceData_t *cmData)
{
    SDC_OPEN_SEARCH_CFM_T * prim;
    sdcSearchElement *currentElement;

    prim = (SDC_OPEN_SEARCH_CFM_T *) cmData->recvMsgP;
    currentElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList,
                                                                 cmData->sdcVar.currentElement);


    cmData->sdcVar.sdcMsgPending   = FALSE;

    CsrBtCmDmUpdateAndClearCachedParamReqSend(currentElement->deviceAddr);
    if (prim->result == SDC_OPEN_SEARCH_OK)
    {
        if (cmData->sdcVar.state == CSR_BT_CM_SDC_STATE_SEARCH &&
            !cmData->sdcVar.cancelPending)
        {
            if (currentElement->readAttrDirect)
            {
                CSR_BT_CM_STATE_CHANGE(cmData->sdcVar.state,
                                       CSR_BT_CM_SDC_STATE_ATTR);
                csrBtCmSdcOpenCfmMsgSend(currentElement->appHandle,
                                         currentElement->deviceAddr);
            }
            else
            {
                csrBtCmSdcServiceSearchHandler(cmData);
            }
        }
        else
        {
            sdc_close_search_req(CSR_BT_CM_IFACEQUEUE);
        }
    }
    else
    {
        CsrBtDeviceAddr deviceAddr       = currentElement->deviceAddr;
        CSR_BT_CM_STATE_CHANGE(cmData->sdcVar.state, CSR_BT_CM_SDC_STATE_IDLE);
        if (cmData->sdcVar.cancelPending)
        {
            cmData->sdcVar.cancelPending  = FALSE;
            csrBtCmSdcCloseIndMsgSend(currentElement->appHandle,
                                      cmData->sdcVar.localServer,
                                      deviceAddr,
                                      CSR_BT_RESULT_CODE_CM_CANCELLED,
                                      CSR_BT_SUPPLIER_CM);
        }
        else
        {
            csrBtCmSdcCloseIndMsgSend(currentElement->appHandle,
                                      cmData->sdcVar.localServer,
                                      deviceAddr,
                                      (CsrBtResultCode) prim->result,
                                      CSR_BT_SUPPLIER_SDP_SDC_OPEN_SEARCH);
        }
        removeElementFromSdcList(cmData);
        CmSdcManagerLocalQueueHandler(cmData);
    }
}

void CsrBtCmSdcServiceSearchCfmHandler(cmInstanceData_t *cmData)
{ /* This event returns the results of a service search request */
    SDC_SERVICE_SEARCH_CFM_T    * prim;

    prim        = (SDC_SERVICE_SEARCH_CFM_T *) cmData->recvMsgP;

    if (CsrCmnListGetCount((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList))
    {
        if (cmData->sdcVar.cancelPending)
        {
            sdc_close_search_req(CSR_BT_CM_IFACEQUEUE);
        }
        else
        {
            sdcSearchElement *currentElement;

            currentElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList,
                                                                         cmData->sdcVar.currentElement);

            switch (cmData->sdcVar.state)
            {
                case CSR_BT_CM_SDC_STATE_SEARCH:
                    cmData->smVar.arg.result.code = (CsrBtResultCode) prim->response;
                    cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_SDP_SDC;

                    switch (prim->response)
                    {
                        case SDC_RESPONSE_SUCCESS:
                            csrBtCmServiceSearchSuccessHandler(cmData, prim);
                            break;

                        case SDC_CON_DISCONNECTED:
                        case SDC_CONNECTION_ERROR_UNKNOWN:
                        case SDC_CONFIGURE_ERROR:
                        case SDC_RESPONSE_TIMEOUT_ERROR:
                            if (currentElement->uuidType == CSR_BT_CM_SDC_UUID_SET)
                            {
                                csrBtCmSdcServiceSearchCfmMsgSend(cmData,
                                                                  NULL,
                                                                  0,
                                                                  (CsrBtResultCode) prim->response,
                                                                  CSR_BT_SUPPLIER_SDP_SDC);
                            }
                            sdc_close_search_req(CSR_BT_CM_IFACEQUEUE);
                            break;

                        default:
                            if (currentElement->uuidType == CSR_BT_CM_SDC_UUID_SET)
                            {
                                csrBtCmSdcServiceSearchCfmMsgSend(cmData,
                                                                  NULL,
                                                                  0,
                                                                  (CsrBtResultCode) prim->response,
                                                                  CSR_BT_SUPPLIER_SDP_SDC);
                                sdc_close_search_req(CSR_BT_CM_IFACEQUEUE);
                            }
                            else
                            {
                                if (prim->response >= L2CA_RESULT_NOT_READY &&
                                    prim->response <= L2CA_RESULT_LINK_TRANSFERRED)
                                {
                                    /* All L2CAP failures are considered to be SDC failure */
                                    sdc_close_search_req(CSR_BT_CM_IFACEQUEUE);
                                }
                                else
                                {
                                    csrBtCmServiceSearchContinueOrDoneHandler(cmData,
                                                                              currentElement);
                                }
                            }
                            break;
                    }
                    break;

                default:
                    sdc_close_search_req(CSR_BT_CM_IFACEQUEUE);
                    break;
            }
        }
    }
}

void CsrBtCmSdcServiceAttributeCfmHandler(cmInstanceData_t * cmData)
{/* Returns the results of a service attribute request */
    SDC_SERVICE_ATTRIBUTE_CFM_T * prim;

    prim = (SDC_SERVICE_ATTRIBUTE_CFM_T *) cmData->recvMsgP;

    cmData->sdcVar.sdcMsgPending = FALSE;
    if (CsrCmnListGetCount((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList))
    {
        if (cmData->sdcVar.cancelPending)
        {
            sdc_close_search_req(CSR_BT_CM_IFACEQUEUE);
        }
        else
        {
            switch (cmData->sdcVar.state)
            {
                case CSR_BT_CM_SDC_STATE_SEARCH:
                    csrBtCmSdcAttrCfmInSdcSearchState(cmData, prim);
                    break;
                case CSR_BT_CM_SDC_STATE_ATTR:
                    csrBtCmSdcAttrCfmMsgSend(cmData);
                    break;
                default:
                    break;
            }
        }
    }
}

void CmSdcServiceSearchAttributeCfmHandler(cmInstanceData_t *cmData)
{
    SDC_SERVICE_SEARCH_ATTRIBUTE_CFM_T *prim = (SDC_SERVICE_SEARCH_ATTRIBUTE_CFM_T *) cmData->recvMsgP;

    cmData->sdcVar.sdcMsgPending = FALSE;

    if (CsrCmnListGetCount((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList))
    {
        if (cmData->sdcVar.cancelPending)
        {
            csrBtCmSdcCancelledHandler(cmData);
        }
        else
        {
            switch (cmData->sdcVar.state)
            {
                case CSR_BT_CM_SDC_STATE_SEARCH:
                    cmData->smVar.arg.result.code     = (CsrBtResultCode) prim->response;
                    cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_SDP_SDC;

                    switch (prim->response)
                    {
                        case SDC_RESPONSE_SUCCESS:
                            cmSdcServiceSearchAttrSuccessHandler(cmData, prim);
                            break;

                        case SDC_CON_DISCONNECTED:
                        case SDC_CONNECTION_ERROR_UNKNOWN:
                        case SDC_CONFIGURE_ERROR:
                        case SDC_RESPONSE_TIMEOUT_ERROR:
                            /*When the above result codes are recieved
                              then we would consider the search failed.*/
                            cmData->sdcVar.validSearchResult = FALSE;
                            cmSdcServiceSearchAttrFailureHandler(cmData);
                            break;

                        default:
                        {
                            if (prim->response >= L2CA_RESULT_NOT_READY &&
                                prim->response <= L2CA_RESULT_LINK_TRANSFERRED)
                            {
                                /* All L2CAP failures are considered to be SDC failure */
                                /*When the above result codes are recieved
                                  then we would consider the search failed.*/
                                cmData->sdcVar.validSearchResult = FALSE;
                                cmSdcServiceSearchAttrFailureHandler(cmData);
                            }
                            else
                            {
                                cmSdcServiceSearchAttrContinueOrDoneHandler(cmData, prim);
                            }
                        }
                    }
                    break;

                default:
                    cmSdcServiceSearchAttrFailureHandler(cmData);
                    break;
            }
        }
    }
    else
    {
        /* If prim->moreToCome was TRUE for the SDC_SERVICE_SEARCH_ATTRIBUTE_CFM and the SDP was cancelled,
           we will receive more SDC_SERVICE_SEARCH_ATTRIBUTE_CFM after the SDC element has been removed.
           Ignore the messages */
    }
}

void CsrBtCmSdcCloseSearchIndHandler(cmInstanceData_t * cmData)
{
    if (CsrCmnListGetCount((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList))
    {
        sdcSearchElement *theElement;
        CsrBtDeviceAddr deviceAddr;

        theElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList,
                                                                 cmData->sdcVar.currentElement);
        deviceAddr = theElement->deviceAddr;

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
        if (theElement->obtainServerChannels &&
            cmData->sdcVar.state == CSR_BT_CM_SDC_STATE_CLOSE)
        {
            if (cmData->sdcVar.cancelPending)
            {
                if (theElement->dmOpenResult)
                {
                    TYPED_BD_ADDR_T ad;
                    ad.addr = deviceAddr;
                    ad.type = CSR_BT_ADDR_PUBLIC;
                    dm_acl_close_req(&ad,
                                     0, /* flags */
                                     0, /* reason */
                                     NULL);
                }
                cmData->sdcVar.cancelPending = FALSE;
                csrBtCmSdcCloseIndMsgSend(theElement->appHandle,
                                          cmData->sdcVar.localServer,
                                          deviceAddr,
                                          CSR_BT_RESULT_CODE_CM_CANCELLED,
                                          CSR_BT_SUPPLIER_CM);

                removeElementFromSdcList(cmData);
            }
            else
            {
                csrBtCmSdcCloseIndMsgSend(theElement->appHandle,
                                          cmData->sdcVar.localServer,
                                          deviceAddr,
                                          CSR_BT_RESULT_CODE_CM_SUCCESS,
                                          CSR_BT_SUPPLIER_CM);
            }
            CmSdcManagerLocalQueueHandler(cmData);
        }
        else
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
        {
            CsrBtResultCode     resultCode;
            CsrBtSupplier resultSupplier;

            if (cmData->sdcVar.cancelPending)
            {
                cmData->sdcVar.cancelPending = FALSE;
                resultCode                   = CSR_BT_RESULT_CODE_CM_CANCELLED;
                resultSupplier               = CSR_BT_SUPPLIER_CM;
            }
            else
            {
                if (cmData->sdcVar.state == CSR_BT_CM_SDC_STATE_CLOSE)
                {
                    resultCode     = CSR_BT_RESULT_CODE_CM_SUCCESS;
                    resultSupplier = CSR_BT_SUPPLIER_CM;
                }
                else if (cmData->sdcVar.state == CSR_BT_CM_SDC_STATE_SEARCH)
                {
                    if (cmData->smVar.arg.result.code == SDC_RESPONSE_SUCCESS &&
                        cmData->smVar.arg.result.supplier == CSR_BT_SUPPLIER_SDP_SDC)
                    {
                        resultCode     = CSR_BT_RESULT_CODE_CM_SUCCESS;
                        resultSupplier = CSR_BT_SUPPLIER_CM;
                    }
                    else
                    {
                        resultCode      = cmData->smVar.arg.result.code;
                        resultSupplier  = cmData->smVar.arg.result.supplier;
                    }
                }
                else
                {
                    resultCode     = CSR_BT_RESULT_CODE_CM_UNSPECIFIED_ERROR;
                    resultSupplier = CSR_BT_SUPPLIER_CM;
                }
            }
            csrBtCmSdcCloseIndMsgSend(theElement->appHandle,
                                      cmData->sdcVar.localServer,
                                      deviceAddr,
                                      resultCode,
                                      resultSupplier);

            removeElementFromSdcList(cmData);
            CmSdcManagerLocalQueueHandler(cmData);
        }
        CSR_BT_CM_STATE_CHANGE(cmData->sdcVar.state, CSR_BT_CM_SDC_STATE_IDLE);
    }
}

void CsrBtCmSdcDecAclRefCountTo(cmInstanceData_t * cmData, CsrBtDeviceAddr deviceAddr)
{
    sdcSearchElement *theElement = (sdcSearchElement*) CsrCmnListSearch((CsrCmnList_t*) &cmData->sdcVar.sdcSearchList,
                                                                        CsrBtCmSdcSearchListAddrCompare,
                                                                        &deviceAddr);
    if (theElement)
    {
        theElement->dmOpenResult = FALSE;
    }
}

void CsrBtCmDmAclOpenInSdcCloseStateHandler(cmInstanceData_t * cmData,
                                            CsrBtDeviceAddr *deviceAddr,
                                            CsrBool success)
{
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    if (cmData->sdcVar.aclOpenRequested &&
        CsrCmnListGetCount((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList))
    {
        sdcSearchElement *currentElement;

        currentElement = (sdcSearchElement *) CsrCmnListGetFromIndex((CsrCmnList_t *) &cmData->sdcVar.sdcSearchList,
                                                                     cmData->sdcVar.currentElement);
        if (CsrBtBdAddrEq(&currentElement->deviceAddr, deviceAddr))
        {
            cmData->sdcVar.aclOpenRequested = FALSE;
            currentElement->dmOpenResult = success ? TRUE : currentElement->dmOpenResult;
            sdc_close_search_req(CSR_BT_CM_IFACEQUEUE);
        }
    }
#else
    CSR_UNUSED(cmData);
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
}

void CsrBtCmSdsRegisterReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSdsRegisterReq   * prim = (CsrBtCmSdsRegisterReq *) cmData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_AUTO_EIR
    if (cmData->dmVar.localEirData && prim->serviceRecord)
    {
        /* Extract service class UUIDs from service record for use with EIR */
        CsrBtCmEirExtractServicesFromRecord(cmData, prim->serviceRecordSize, prim->serviceRecord);
    }
#endif /* CSR_BT_INSTALL_CM_AUTO_EIR */

    cmData->smVar.appHandle     = prim->appHandle;
    cmData->smVar.arg.reg.context   = prim->context;
    sds_register_req(CSR_BT_CM_IFACEQUEUE, prim->serviceRecord, prim->serviceRecordSize);

    prim->serviceRecord = NULL;
}

void CsrBtCmSdsUnRegisterReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSdsUnregisterReq * prim;

    prim                    = (CsrBtCmSdsUnregisterReq *) cmData->recvMsgP;
    cmData->smVar.appHandle = prim->appHandle;
    cmData->smVar.arg.reg.context = prim->context;
    sds_unregister_req(CSR_BT_CM_IFACEQUEUE, prim->serviceRecHandle);
}

void CsrBtCmSdsRegisterCfmHandler(cmInstanceData_t *cmData)
{
    void                    *message = NULL;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
    SDS_REGISTER_CFM_T      * receivePrim = (SDS_REGISTER_CFM_T *) cmData->recvMsgP;

    if (receivePrim->result == SDS_SUCCESS)
    {
        resultCode     = CSR_BT_RESULT_CODE_CM_SUCCESS;
        resultSupplier = CSR_BT_SUPPLIER_CM;

#ifdef CSR_BT_INSTALL_CM_AUTO_EIR
        if (cmData->dmVar.localEirData)
        {
            /* Registration was a success - update local EIR */
            CsrBtCmEirAddServiceRecord(cmData, receivePrim->svc_rec_hndl);
        }
#endif /* CSR_BT_INSTALL_CM_AUTO_EIR */
    }
    else
    {
        resultCode     = (CsrBtResultCode) receivePrim->result;
        resultSupplier = CSR_BT_SUPPLIER_SDP_SDS;
    }

    if (cmData->smVar.arg.reg.context == CSR_BT_CM_CONTEXT_UNUSED)
    {
        CsrBtCmSdsRegisterCfm   * prim = (CsrBtCmSdsRegisterCfm *) CsrPmemAlloc(sizeof(CsrBtCmSdsRegisterCfm));
        prim->type              = CSR_BT_CM_SDS_REGISTER_CFM;
        prim->serviceRecHandle  = receivePrim->svc_rec_hndl;
        prim->resultCode        = resultCode;
        prim->resultSupplier    = resultSupplier;
        message                 = prim;
    }
    else
    {
        CsrBtCmSdsExtRegisterCfm * prim = (CsrBtCmSdsExtRegisterCfm *) CsrPmemAlloc(sizeof(CsrBtCmSdsExtRegisterCfm));
        prim->type              = CSR_BT_CM_SDS_EXT_REGISTER_CFM;
        prim->serviceRecHandle  = receivePrim->svc_rec_hndl;
        prim->resultCode        = resultCode;
        prim->resultSupplier    = resultSupplier;
        prim->context           = cmData->smVar.arg.reg.context;
        message                 = prim;
    }

    if (receivePrim->result == SDS_SUCCESS)
    {
        CsrBtCmPutMessage(cmData->smVar.appHandle, message);
    }
    else if (receivePrim->result == SDS_PENDING)
    {
        /* Ignore */
        CsrPmemFree(message);
    }
    else
    {
        CsrSchedTimerSet(300000, delayedMessage,cmData->smVar.appHandle, message);
    }
    CsrBtCmServiceManagerLocalQueueHandler(cmData);
}

void CsrBtCmSdsUnRegisterCfmHandler(cmInstanceData_t *cmData)
{
    SDS_UNREGISTER_CFM_T    * receivePrim = (SDS_UNREGISTER_CFM_T *) cmData->recvMsgP;

    if (receivePrim->result == SDS_PENDING)
    {/* Ignore */
        ;
    }
    else
    {
        void                    *message;
        CsrBtResultCode         resultCode;
        CsrBtSupplier           resultSupplier;
        if (receivePrim->result == SDS_SUCCESS)
        {
            resultCode     = CSR_BT_RESULT_CODE_CM_SUCCESS;
            resultSupplier = CSR_BT_SUPPLIER_CM;

#ifdef CSR_BT_INSTALL_CM_AUTO_EIR
            if (cmData->dmVar.localEirData)
            {
                /* Unregistration was a success - update local EIR */
                CsrBtCmEirRemoveServiceRecord(cmData, receivePrim->svc_rec_hndl);
            }
#endif /* CSR_BT_INSTALL_CM_AUTO_EIR */
        }
        else
        {
            resultCode     = (CsrBtResultCode) receivePrim->result;
            resultSupplier = CSR_BT_SUPPLIER_SDP_SDS;
        }

        if (cmData->smVar.arg.reg.context == CSR_BT_CM_CONTEXT_UNUSED)
        {
            CsrBtCmSdsUnregisterCfm * prim  = (CsrBtCmSdsUnregisterCfm *)
                                               CsrPmemAlloc(sizeof(CsrBtCmSdsUnregisterCfm));

            prim->type                      = CSR_BT_CM_SDS_UNREGISTER_CFM;
            prim->serviceRecHandle          = receivePrim->svc_rec_hndl;
            prim->resultCode                = resultCode;
            prim->resultSupplier            = resultSupplier;
            message                         = prim;
        }
        else
        {
            CsrBtCmSdsExtUnregisterCfm * prim = (CsrBtCmSdsExtUnregisterCfm *)
                                                 CsrPmemAlloc(sizeof(CsrBtCmSdsExtUnregisterCfm));

            prim->type                        = CSR_BT_CM_SDS_EXT_UNREGISTER_CFM;
            prim->serviceRecHandle            = receivePrim->svc_rec_hndl;
            prim->resultCode                  = resultCode;
            prim->resultSupplier              = resultSupplier;
            prim->context                     = cmData->smVar.arg.reg.context;
            message                           = prim;
        }

        if (receivePrim->result == SDS_SUCCESS)
        {
            CsrBtCmPutMessage(cmData->smVar.appHandle, message);
        }
        else
        {
            CsrSchedTimerSet(300000, delayedMessage,cmData->smVar.appHandle, message);
        }
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }
}

