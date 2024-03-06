/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_sdc.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"

static const SignalHandlerType cmSdcManagerProvider[CM_SDC_PRIM_DOWNSTREAM_COUNT] =
{
    CsrBtCmSdcSearchReqHandler,                  /* CSR_BT_CM_SDC_SEARCH_REQ              */
    CsrBtCmSdcServiceSearchReqHandler,           /* CSR_BT_CM_SDC_SERVICE_SEARCH_REQ      */
    CsrBtCmSdcUuid128SearchReqHandler,           /* CSR_BT_CM_SDC_UUID128_SEARCH_REQ      */
    CsrBtCmSdcOpenReqHandler,                    /* CSR_BT_CM_SDC_OPEN_REQ                */
    CsrBtCmSdcRfcSearchReqHandler,               /* CSR_BT_CM_SDC_RFC_SEARCH_REQ          */
    CsrBtCmSdcUuid128RfcSearchReqHandler,        /* CSR_BT_CM_SDC_UUID128_RFC_SEARCH_REQ  */
    CsrBtCmSdcRfcExtendedSearchReqHandler,       /* CSR_BT_CM_SDC_RFC_EXTENDED_SEARCH_REQ */
    CsrBtCmSdcReleaseResourcesReqHandler,        /* CSR_BT_CM_SDC_RELEASE_RESOURCES_REQ   */
    CmSdcServiceSearchAttrReqHandler,            /* CM_SDC_SERVICE_SEARCH_ATTR_REQ        */
};

static void cmSdcManagerSignalHandler(cmInstanceData_t *cmData)
{
    CsrBtCmPrim *primPtr;

    primPtr = (CsrBtCmPrim *) cmData->recvMsgP;

    if(((*primPtr - CM_SDC_PRIM_DOWNSTREAM_LOWEST) < CM_SDC_PRIM_DOWNSTREAM_COUNT) &&
       cmSdcManagerProvider[*primPtr - CM_SDC_PRIM_DOWNSTREAM_LOWEST] != NULL)
    {
        CmSdcLockQueue(cmData);
        cmSdcManagerProvider[*primPtr - CM_SDC_PRIM_DOWNSTREAM_LOWEST](cmData);
    }
    else
    {/* Event ERROR! */
        CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                *(CsrBtCmPrim *) cmData->recvMsgP,
                                cmData->globalState,
                                "Invalid SDC Event");
    }
}

void CmSdcManagerLocalQueueHandler(cmInstanceData_t *cmData)
{
    CsrBtCmPrim *prim = CsrPmemAlloc(sizeof(CsrBtCmPrim));

    *prim = CM_SDC_HOUSE_CLEANING;
    CsrBtCmPutMessage(CSR_BT_CM_IFACEQUEUE, prim);
    CSR_UNUSED(cmData);
}

void CmSdcRestoreSdcQueueHandler(cmInstanceData_t *cmData)
{
    CsrUint16     eventClass = 0;
    void         *msg = NULL;

    CmSdcUnlockQueue(cmData);

    if(CsrMessageQueuePop(&cmData->sdcVar.saveQueue, &eventClass, &msg))
    {
        SynergyMessageFree(CSR_BT_CM_PRIM, cmData->recvMsgP);
        cmData->recvMsgP = msg;
        cmSdcManagerSignalHandler(cmData);
    }
}

static CsrBool cmSdcCancelSavedMsg(CsrSchedQid primPhandle,
                                   CsrSchedQid savedPhandle,
                                   CsrBtDeviceAddr *primBdAddr,
                                   CsrBtDeviceAddr *savedBdAddr,
                                   CsrUint8 primSdcServer,
                                   CsrUint8 *sdcServer)
{
    CsrBool cancelMsg = FALSE;

    if (primPhandle == savedPhandle && CsrBtBdAddrEq(primBdAddr, savedBdAddr))
    {
        if (primSdcServer != CSR_BT_NO_SERVER)
        {
            *sdcServer  = primSdcServer;
        }
        cancelMsg   = TRUE;
    }

    return (cancelMsg);
}

CsrBool CmSdcCancelSdcManagerMsg(cmInstanceData_t *cmData,
                                 CsrBtCmSdcCancelSearchReq *prim,
                                 CsrUint8 *sdcServer)
{
    CsrUint16                eventClass;
    void                    *msg;
    CsrBool                  cancelMsg  = FALSE;
    CsrMessageQueueType     *tempQueue  = NULL;
    *sdcServer                          = CSR_BT_NO_SERVER;

    while (CsrMessageQueuePop(&cmData->sdcVar.saveQueue, &eventClass, &msg))
    {
        if (!cancelMsg &&
            eventClass == CSR_BT_CM_PRIM &&
            prim->typeToCancel == (*(CsrBtCmPrim *)msg))
        {
            switch (prim->typeToCancel)
            {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
                case CSR_BT_CM_SDC_RFC_SEARCH_REQ:
                {
                    CsrBtCmSdcRfcSearchReq * savedPrim = (CsrBtCmSdcRfcSearchReq *) msg;
                    cancelMsg = cmSdcCancelSavedMsg(prim->appHandle, savedPrim->appHandle, &(prim->deviceAddr), &(savedPrim->deviceAddr), savedPrim->localServerChannel, sdcServer);
                    break;
                }
                case CSR_BT_CM_SDC_RFC_EXTENDED_SEARCH_REQ:
                {
                    CsrBtCmSdcRfcExtendedSearchReq * savedPrim = (CsrBtCmSdcRfcExtendedSearchReq *) msg;
                    cancelMsg = cmSdcCancelSavedMsg(prim->appHandle, savedPrim->appHandle, &(prim->deviceAddr), &(savedPrim->deviceAddr), savedPrim->localServerChannel, sdcServer);
                    break;
                }
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
                case CSR_BT_CM_SDC_UUID128_RFC_SEARCH_REQ:
                {
                    CsrBtCmSdcUuid128RfcSearchReq * savedPrim = (CsrBtCmSdcUuid128RfcSearchReq *) msg;
                    cancelMsg = cmSdcCancelSavedMsg(prim->appHandle, savedPrim->appHandle, &(prim->deviceAddr), &(savedPrim->deviceAddr), savedPrim->localServerChannel, sdcServer);
                    break;
                }
#endif /* CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH */
#endif /* !EXCLUDE_CSR_BT_RFC_MODULE */
                case CM_SDC_SERVICE_SEARCH_ATTR_REQ:
                {
                    CmSdcServiceSearchAttrReq * savedPrim = (CmSdcServiceSearchAttrReq *) msg;
                    cancelMsg = cmSdcCancelSavedMsg(prim->appHandle, savedPrim->appHandle, &(prim->deviceAddr), &(savedPrim->deviceAddr), savedPrim->localServerChannel, sdcServer);
                    break;
                }
                case CSR_BT_CM_SDC_SEARCH_REQ:
                {
                    CsrBtCmSdcSearchReq * savedPrim = (CsrBtCmSdcSearchReq *) msg;
                    cancelMsg = cmSdcCancelSavedMsg(prim->appHandle, savedPrim->appHandle, &(prim->deviceAddr), &(savedPrim->deviceAddr), CSR_BT_NO_SERVER, sdcServer);
                    break;
                }
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
                case CSR_BT_CM_SDC_UUID128_SEARCH_REQ:
                {
                    CsrBtCmSdcUuid128SearchReq * savedPrim = (CsrBtCmSdcUuid128SearchReq *) msg;
                    cancelMsg = cmSdcCancelSavedMsg(prim->appHandle, savedPrim->appHandle, &(prim->deviceAddr), &(savedPrim->deviceAddr), CSR_BT_NO_SERVER, sdcServer);
                    break;
                }
#endif /* CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH */
                case CSR_BT_CM_SDC_OPEN_REQ:
                {
                    CsrBtCmSdcOpenReq * savedPrim = (CsrBtCmSdcOpenReq *) msg;
                    cancelMsg = cmSdcCancelSavedMsg(prim->appHandle, savedPrim->appHandle, &(prim->deviceAddr), &(savedPrim->deviceAddr), CSR_BT_NO_SERVER, sdcServer);
                    break;
                }
#ifdef CSR_BT_INSTALL_CM_PRI_SDC
                case CSR_BT_CM_SDC_SERVICE_SEARCH_REQ:
                {
                    CsrBtCmSdcServiceSearchReq * savedPrim = (CsrBtCmSdcServiceSearchReq *) msg;
                    cancelMsg = cmSdcCancelSavedMsg(prim->appHandle, savedPrim->appHandle, &(prim->deviceAddr), &(savedPrim->deviceAddr), CSR_BT_NO_SERVER, sdcServer);
                    break;
                }
#endif /* CSR_BT_INSTALL_CM_PRI_SDC */
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

void CmSdcManagerProvider(cmInstanceData_t *cmData)
{
    if (CM_SDC_QUEUE_LOCKED(&cmData->sdcVar))
    {/* Save signal, since we are waiting for completion of ongoing service discovery */
        CsrMessageQueuePush(&cmData->sdcVar.saveQueue, CSR_BT_CM_PRIM, cmData->recvMsgP);
        cmData->recvMsgP = NULL;
    }
    else
    {/* SDC Manager is ready just proceed */
        cmSdcManagerSignalHandler(cmData);
    }
}
