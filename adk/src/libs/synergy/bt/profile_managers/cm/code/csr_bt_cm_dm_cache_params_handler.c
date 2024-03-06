/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
 ******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_util.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_sdc.h"
#include "csr_bt_cm_dm_sc_ssp_handler.h"
#include "csr_bt_cm_events_handler.h"

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
#include "csr_bt_cm_bnep.h"
#endif

#ifdef CSR_BT_INSTALL_CM_CACHE_PARAMS

/* --- Helper functions --- */
/* CM cache table related functions */
static CsrBool cmCacheParamAddrMatch(CsrCmnListElm_t *elem, void *data)
{
    dmCacheParamEntry *entry = (dmCacheParamEntry *) elem;
    CsrBtDeviceAddr *addr = (CsrBtDeviceAddr *) data;

    return (CsrBtBdAddrEq(addr, &entry->deviceAddr));
}

static void cmCacheParamFlush(CsrCmnListElm_t *elem, void *data)
{
    dmCacheParamEntry *entry = (dmCacheParamEntry *) elem;

    entry->pageScanMode = HCI_PAGE_SCAN_MODE_DEFAULT;
    entry->pageScanRepMode = HCI_PAGE_SCAN_REP_MODE_R2;

    CSR_UNUSED(data);
}

static dmCacheParamEntry *cmGetEntryInCacheParamTable(cmInstanceData_t *cmData, CsrBtDeviceAddr *deviceAddr)
{
    return (dmCacheParamEntry *)CsrCmnListSearch((CsrCmnList_t *) &cmData->dmVar.dmCacheParamTable,
                                                 cmCacheParamAddrMatch,
                                                 deviceAddr);
}

dmCacheParamEntry *CmInsertEntryInCacheParamTable(cmInstanceData_t *cmData, CsrBtDeviceAddr *devAddr)
{
    dmCacheParamEntry *entry = cmGetEntryInCacheParamTable(cmData, devAddr);

    if (!entry)
    {
        entry = (dmCacheParamEntry *) CsrCmnListElementAddLast((CsrCmnList_t *) &cmData->dmVar.dmCacheParamTable,
                                                               sizeof(dmCacheParamEntry));
    }

    return (entry);
}

/* Handler functions for resuming prior activities */
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
static void csrBtCmDmWriteCachedPageModeCfmRfcHandler(cmInstanceData_t *cmData,
                                                      cmRfcConnElement *theRfcElement)
{
    if (theRfcElement && theRfcElement->cmRfcConnInst)
    {
        cmRfcConnInstType *theLogicalLink = theRfcElement->cmRfcConnInst;

        if (!cmData->rfcVar.cancelConnect)
        {
            CsrBtCmRfcStartInitiateConnection(cmData, theLogicalLink);
        }
        else
        {
            /* The application has requested to cancel the connection.*/
            (void)CmDuHandleAutomaticProcedure(cmData,
                                               CM_DU_AUTO_EVENT_SERVICE_DISCONNECTED,
                                               NULL,
                                               &theLogicalLink->deviceAddr);
            CsrBtCmConnectCfmMsgSend(cmData,
                                     CSR_BT_RESULT_CODE_CM_CANCELLED,
                                     CSR_BT_SUPPLIER_CM);
            CsrBtCmServiceManagerLocalQueueHandler(cmData);
        }
    }
    else if (theRfcElement)
    { /* If theRfcElement == NULL then this function has already been called */
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }

    /* Cache param procedure is over, free the DM queue.*/
    CsrBtCmDmLocalQueueHandler();
}
#endif

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
static void csrBtCmDmWriteCachedPageModeCfmL2caHandler(cmInstanceData_t *cmData,
                                                       cmL2caConnElement *theL2caElement)
{
    if (theL2caElement && theL2caElement->cmL2caConnInst)
    {
        if (cmData->l2caVar.cancelConnect)
        {
            CsrBtCmL2caConnectCancelCleanup(cmData, theL2caElement);
            CsrBtCmServiceManagerLocalQueueHandler(cmData);
        }
#ifndef EXCLUDE_CSR_BT_CM_LEGACY_PAIRING_DETACH
        else
        {
            cmL2caConnInstType *l2CaConnection= theL2caElement->cmL2caConnInst;
            /* Check whether we need to apply the legacy
             * pairing ACL detach work around */
            if (!CsrBtCmL2caCheckLegacyDetach(cmData, l2CaConnection))
            {
                CsrBtCml2caAutoConnectSetup(cmData, theL2caElement);
            }
        }
#endif /* !EXCLUDE_CSR_BT_CM_LEGACY_PAIRING_DETACH */
    }
    else
    {
        if (theL2caElement)
        { /* If theL2caElement == NULL then this function has allready been called */
            CsrBtCmServiceManagerLocalQueueHandler(cmData);
        }
    }

    /* DM queue is required only for write cache params procedure, which is completed now. Hence free the DM queue.*/
    CsrBtCmDmLocalQueueHandler();
}
#endif

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
static void csrBtCmDmWriteCachedPageModeCfmBnepHandler(cmInstanceData_t *cmData)
{
    CsrUint8 theIndex;
    bnepTable *bnepConnection;

    theIndex = returnIndexToThisBdAddr(cmData, cmData->dmVar.cacheTargetDev);

    if (theIndex == CM_ERROR)
    {
        /* BNEP Connection instance doesn't exists, no further processing is required, freeup the queues.*/
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
        CsrBtCmDmLocalQueueHandler();
        return;
    }

    bnepConnection = &(cmData->bnepVar.connectVar[theIndex]);

    CSR_BT_CM_STATE_CHANGE(bnepConnection->state, CSR_BT_CM_BNEP_STATE_CONNECT);
    CSR_BT_CM_STATE_CHANGE(cmData->bnepVar.connectState, CM_BNEP_CONNECT);

    if (!cmData->bnepVar.cancelConnect)
    {
        CsrBtBnepConnectReqSend(cmData->bnepVar.connectReqFlags, CsrBtBdAddrToEtherAddr(&bnepConnection->deviceAddr));
        /* SM queue needs to be held till connect process finishes.*/
    }
    else
    {
        CsrBtCmBnepConnectIndMsgSend(cmData,
                                     cmData->bnepVar.appHandle,
                                     ID_EMPTY,
                                     CsrBtBdAddrToEtherAddr(&bnepConnection->deviceAddr),
                                     0,
                                     0,
                                     0,
                                     CSR_BT_RESULT_CODE_CM_CANCELLED,
                                     CSR_BT_SUPPLIER_CM);
        /* Clear the bnep connection details. */
        CsrBtCmBnepClearBnepTableIndex(bnepConnection);
        /* Cancelling the connection is as good as disconnected, inform this to device utility
         * in order to adjust the link policy if required. */
        (void)CmDuHandleAutomaticProcedure(cmData,
                                           CM_DU_AUTO_EVENT_SERVICE_DISCONNECTED,
                                           NULL,
                                           &bnepConnection->deviceAddr);
        CsrBtCmDmUpdateAndClearCachedParamReqSend(bnepConnection->deviceAddr);
        /* Connect service process is over, free SM queue.*/
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }

    CsrBtCmDmLocalQueueHandler();
}
#endif

static void csrBtCmDmWriteCachedPageModeCfmSdcHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSdcStartHandler(cmData, cmData->dmVar.cacheTargetDev);
    CsrBtCmDmLocalQueueHandler();
}

static void cmDmWriteCacheParamsReqSend(CsrUint8 activePlayer,
                                        CsrBtDeviceAddr devAddr,
                                        CsrUint16 clockOffset,
                                        page_scan_mode_t pageScanMode,
                                        page_scan_rep_mode_t pageScanRepMode)
{
    CsrBtCmDmWriteCacheParamsReq *prim;

    prim = (CsrBtCmDmWriteCacheParamsReq *) CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_BT_CM_DM_WRITE_CACHE_PARAMS_REQ;
    prim->activePlayer = activePlayer;
    prim->devAddr = devAddr;
    prim->clockOffset = clockOffset;
    prim->pageScanMode = pageScanMode;
    prim->pageScanRepMode = pageScanRepMode;
    CsrBtCmPutMessage(CSR_BT_CM_IFACEQUEUE, prim);
}

static CsrBool cmDmCacheParamsKnown(cmInstanceData_t *cmData,
                                    CsrBtDeviceAddr devAddr,
                                    CsrUint16 *clockOffset,
                                    page_scan_mode_t *pageScanMode,
                                    page_scan_rep_mode_t *pageScanRepMode)
{
    dmCacheParamEntry * entryCacheTable = cmGetEntryInCacheParamTable(cmData,
                                                                           &devAddr);

    if (entryCacheTable)
    {
        *clockOffset = entryCacheTable->clockOffset;
        *pageScanMode = entryCacheTable->pageScanMode;
        *pageScanRepMode = entryCacheTable->pageScanRepMode;

        return TRUE;
    }

    return FALSE;
}

static void cmDmWriteCacheParamsDirect(cmInstanceData_t *cmData,
                                       CsrBtDeviceAddr devAddr,
                                       CsrUint16 clockOffset,
                                       page_scan_mode_t pageScanMode,
                                       page_scan_rep_mode_t pageScanRepMode)
{
    /* Store the page-mode information */
    cmData->dmVar.pageScanMode = pageScanMode;
    cmData->dmVar.pageScanRepMode = pageScanRepMode;
    cmData->dmVar.cacheTargetDev = devAddr;

    /* start sending parameters directly to DM */
    /* send clock offset now and send page mode information on clock offset cfm */
    dm_write_cached_clock_offset_req(&devAddr, clockOffset, NULL);
}

static void cmFlushCmCacheTimeout(CsrUint16 dummy, void *data)
{
    cmInstanceData_t *cmData = (cmInstanceData_t *) data;

    CsrCmnListIterate((CsrCmnList_t *) &cmData->dmVar.dmCacheParamTable,
                      cmCacheParamFlush,
                      NULL);

    cmData->dmVar.cacheFlushTimerId = 0; /* Timer was fired */
    CSR_UNUSED(dummy);
}

/* --- Externally available functions --- */
#ifdef ENABLE_SHUTDOWN
void CsrBtCmRemoveCacheParamTable(cmInstanceData_t *cmData)
{
    CsrCmnListDeinit((CsrCmnList_t *) &cmData->dmVar.dmCacheParamTable);
}
#endif

void CsrBtCmDmWriteCachedPageModeCfmHandler(cmInstanceData_t *cmData)
{
    switch (cmData->dmVar.activePlayer)
    {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
        case RFC_PLAYER:
            csrBtCmDmWriteCachedPageModeCfmRfcHandler(cmData,
                                                      CM_RFC_ELEMENT_ACTIVE(cmData));
            break;
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
        case L2CAP_PLAYER:
            csrBtCmDmWriteCachedPageModeCfmL2caHandler(cmData,
                                                       CM_L2CA_ELEMENT_ACTIVE(cmData));
            break;
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
        case BNEP_PLAYER:
            csrBtCmDmWriteCachedPageModeCfmBnepHandler(cmData);
            break;
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */

        case SDC_PLAYER:
            csrBtCmDmWriteCachedPageModeCfmSdcHandler(cmData);
            break;

        default:
            CsrBtCmServiceManagerLocalQueueHandler(cmData);
            CsrBtCmDmLocalQueueHandler();
            break;
    }
}

CsrBool CmDmWriteCacheParamsAll(cmInstanceData_t *cmData, CsrBtDeviceAddr *deviceAddr)
{
    CsrUint16 clockOffset;
    page_scan_mode_t pageScanMode;
    page_scan_rep_mode_t pageScanRepMode;

    if (cmDmCacheParamsKnown(cmData,
                             *deviceAddr,
                             &clockOffset,
                             &pageScanMode,
                             &pageScanRepMode))
    {
        /* start sending parameters directly to DM */
        /* send clock offset now and send page mode information on clock offset cfm */
        dm_write_cached_clock_offset_req(deviceAddr, clockOffset, NULL);
        dm_write_cached_page_mode_req(deviceAddr, pageScanMode, pageScanRepMode, NULL);

        return TRUE;
    }
    return FALSE;
}

CsrBool CsrBtCmDmWriteKnownCacheParams(cmInstanceData_t *cmData,
                                       CsrBtDeviceAddr devAddr,
                                       CsrBtCmPlayer player)
{
    CsrUint16 clockOffset;
    page_scan_mode_t pageScanMode;
    page_scan_rep_mode_t pageScanRepMode;

    if (cmDmCacheParamsKnown(cmData,
                         devAddr,
                         &clockOffset,
                         &pageScanMode,
                         &pageScanRepMode))
    {
        /* start writing cache params. via DM queue */
        cmDmWriteCacheParamsReqSend(player,
                                         devAddr,
                                         clockOffset,
                                         pageScanMode,
                                         pageScanRepMode);

        return TRUE;
    }

    return FALSE;
}

CsrBool CmDmWriteKnownCacheParamsDirect(cmInstanceData_t *cmData, CsrBtDeviceAddr devAddr)
{
    CsrUint16 clockOffset;
    page_scan_mode_t pageScanMode;
    page_scan_rep_mode_t pageScanRepMode;

    if (cmDmCacheParamsKnown(cmData,
                         devAddr,
                         &clockOffset,
                         &pageScanMode,
                         &pageScanRepMode))
    {
        /* start writing cache params. via DM queue */
        cmDmWriteCacheParamsDirect(cmData,
                                   devAddr,
                                   clockOffset,
                                   pageScanMode,
                                   pageScanRepMode);
        return TRUE;
    }

    return FALSE;
}

void CsrBtCmDmWriteCacheParamsReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmDmWriteCacheParamsReq *cmPrim = (CsrBtCmDmWriteCacheParamsReq *) cmData->recvMsgP;

    cmData->dmVar.activePlayer = cmPrim->activePlayer;

    cmDmWriteCacheParamsDirect(cmData,
                               cmPrim->devAddr,
                               cmPrim->clockOffset,
                               cmPrim->pageScanMode,
                               cmPrim->pageScanRepMode);
}

void CsrBtCmDmUpdateAndClearCachedParamReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmDmUpdateAndClearCachedParamReq *cmPrim = (CsrBtCmDmUpdateAndClearCachedParamReq *) cmData->recvMsgP;

    CmDmUpdateAndClearCachedParamDirect(cmData, cmPrim->devAddr);
}

void CsrBtCmDmWriteCachedClockOffsetCfmHandler(cmInstanceData_t *cmData)
{
    /* The DM has confirmed that it has received a clock offset */
    /* There is no check to see if it was success or failure */
    /* because in case of failure performance will only degrade */
    /* a bit, hence proceed with the sequence  by sending the */
    /* page scan mode and page scan rep mode */

    dm_write_cached_page_mode_req(&cmData->dmVar.cacheTargetDev,
                                  cmData->dmVar.pageScanMode,
                                  cmData->dmVar.pageScanRepMode,
                                  NULL);
}

void CmDmStoreCacheParams(cmInstanceData_t *cmData,
                          CsrBtDeviceAddr devAddr,
                          CsrUint16 clockOffset,
                          page_scan_mode_t pageScanMode,
                          page_scan_rep_mode_t pageScanRepMode)
{

    dmCacheParamEntry * newEntry = CmInsertEntryInCacheParamTable(cmData,
                                                                       &devAddr);

    newEntry->deviceAddr = devAddr;
    newEntry->clockOffset = clockOffset;
    newEntry->pageScanMode = pageScanMode;
    newEntry->pageScanRepMode = pageScanRepMode;
}

/* Loopback functions to lock the DM queue */

void CsrBtCmDmUpdateAndClearCachedParamReqSend(CsrBtDeviceAddr devAddr)
{
    CsrBtCmDmUpdateAndClearCachedParamReq *prim;

    prim = (CsrBtCmDmUpdateAndClearCachedParamReq *) CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_BT_CM_DM_UPDATE_AND_CLEAR_CACHED_PARAM_REQ;
    prim->devAddr = devAddr;
    CsrBtCmPutMessage(CSR_BT_CM_IFACEQUEUE, prim);
}

void CmDmUpdateAndClearCachedParamDirect(cmInstanceData_t *cmData, CsrBtDeviceAddr devAddr)
{
    CsrBool linkExists = FALSE;
    cmData->dmVar.cacheTargetDev = devAddr;

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    if (CsrBtCmRfcFindRfcConnElementFromDeviceAddrState(cmData,
                                                        &devAddr,
                                                        CSR_BT_CM_RFC_STATE_CONNECTED))
    {
        linkExists = TRUE;
    }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    if (!linkExists)
    {
        if (CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromConnectedSDeviceAddr,
                                 &devAddr))
        {
            linkExists = TRUE;
        }
    }
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
    if (!linkExists)
    {
        CsrUintFast8 i;

        for (i = 0; i < CSR_BT_MAX_NUM_OF_SIMULTANEOUS_BNEP_CONNECTIONS; i++)
        { /* Search through the BNEP connection table */

            if (CsrBtBdAddrEq(&(devAddr),
                              &(cmData->bnepVar.connectVar[i].deviceAddr)))
            {
                if (cmData->bnepVar.connectVar[i].state == CSR_BT_CM_BNEP_STATE_CONNECTED)
                { /* Thie given address is right. */
                    linkExists = TRUE;
                }
            }
        }
    }
#endif

    if (linkExists && cmData->dmVar.lockMsg != CSR_BT_CM_READ_REMOTE_NAME_REQ)
    {
        dm_hci_read_clock_offset(&devAddr, NULL);
    }
    else
    {
        dm_clear_param_cache_req(&devAddr, NULL);
    }
}

void CsrBtCmFlushCmCacheStopTimer(cmInstanceData_t *cmData)
{
    if (cmData->dmVar.cacheFlushTimerId != 0)
    {/* Cancel the timer if it is already running */
        CsrSchedTimerCancel(cmData->dmVar.cacheFlushTimerId, NULL, NULL);
        cmData->dmVar.cacheFlushTimerId = 0;
    }
}

void CsrBtCmFlushCmCacheStartTimer(cmInstanceData_t *cmData)
{
    /* Stop the timer if it is already running - should not be running at this point */
    CsrBtCmFlushCmCacheStopTimer(cmData);

    /* Start the timer */
    cmData->dmVar.cacheFlushTimerId = CsrSchedTimerSet(CM_DM_CACHE_FLUSH_TIMEOUT,
                                                       cmFlushCmCacheTimeout,
                                                       0,
                                                       (void * ) cmData);
}

void CsrBtCmDmHciReadClockOffsetCompleteHandler(cmInstanceData_t *cmData)
{
    DM_HCI_READ_CLOCK_OFFSET_CFM_T *dmPrim;

    dmPrim = (DM_HCI_READ_CLOCK_OFFSET_CFM_T *) cmData->recvMsgP;

    if (dmPrim->status == HCI_SUCCESS)
    {
        dmCacheParamEntry *newEntry = CmInsertEntryInCacheParamTable(cmData, &(dmPrim->bd_addr));

        newEntry->deviceAddr = dmPrim->bd_addr;
        newEntry->clockOffset = dmPrim->clock_offset;
    }
    dm_clear_param_cache_req(&(dmPrim->bd_addr), NULL);
}
#endif /* CSR_BT_INSTALL_CM_CACHE_PARAMS */

