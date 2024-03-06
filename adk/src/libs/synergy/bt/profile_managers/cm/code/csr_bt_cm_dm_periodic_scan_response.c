/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_log_text_2.h"
#include "csr_bt_cm_le.h"

#ifdef CSR_TARGET_PRODUCT_VM
#include "message.h"
#include "synergy.h"
#endif

#ifdef CSR_BT_INSTALL_PERIODIC_SCANNING
#ifdef CSR_STREAMS_ENABLE
extern Source StreamPeriodicScanSource(uint16 sync_handle);
extern void CmStreamFlushSource(Source src);
#endif /* CSR_STREAMS_ENABLE */

void CsrBtCmDmPeriodicScanStartFindTrainsCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_SCAN_START_FIND_TRAINS_CFM_T *dmPrim = msg;
    CmPeriodicScanStartFindTrainsCfm *prim;
    CsrUint8 i;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_START_FIND_TRAINS_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->scanHandle = dmPrim->scan_handle;

    /* Store periodic scan handle for other Periodic Scanning Operations */
    for(i = 0; i < MAX_EXT_SCAN_APP; i++)
    {
        if (cmData->extScanHandles[i].pHandle == CSR_SCHED_QID_INVALID)
        {
            cmData->extScanHandles[i].pHandle = cmData->dmVar.appHandle;
            cmData->extScanHandles[i].scanHandle = dmPrim->scan_handle;
            break;
        }
    }

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmPeriodicScanStopFindTrainsCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_SCAN_STOP_FIND_TRAINS_CFM_T *dmPrim = msg;
    CmPeriodicScanStopFindTrainsCfm *prim;
    CsrUint8 i;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_STOP_FIND_TRAINS_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;

    /* Remove the Periodic Scan handle from the extScanHandle List */
    for(i = 0; i < MAX_EXT_SCAN_APP; i++)
    {
        if (cmData->extScanHandles[i].pHandle == cmData->dmVar.appHandle)
        {
            cmData->extScanHandles[i].pHandle = CSR_SCHED_QID_INVALID;
            cmData->extScanHandles[i].scanHandle = CSR_BT_EXT_SCAN_HANDLE_INVALID;
            break;
        }
    }

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmPeriodicScanSyncToTrainCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM_T *dmPrim = msg;
    CmPeriodicScanSyncToTrainCfm *prim;
    CsrSchedQid appHandle = CSR_SCHED_QID_INVALID;
    CsrUint8 i;

    for(i = 0; i < MAX_PERIODIC_SCAN_APP; i++)
    {
        if (cmData->periodicScanHandles[i].pending == TRUE)
        {
            prim = CsrPmemAlloc(sizeof(*prim));
            prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM;
            prim->resultCode = (CsrBtResultCode) dmPrim->status;
            prim->syncHandle = dmPrim->sync_handle;
            prim->advSid = dmPrim->adv_sid;
            CsrBtAddrCopy(&(prim->addrt), &(dmPrim->addrt));
            prim->advPhy = dmPrim->adv_phy;
            prim->periodicAdvInterval = dmPrim->periodic_adv_interval;

            appHandle = cmData->periodicScanHandles[i].pHandle;

            /* Store periodic sync handle for sending Periodic Advert Report */
            if (dmPrim->status == CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                cmData->periodicScanHandles[i].syncHandle = dmPrim->sync_handle;
#ifdef CSR_STREAMS_ENABLE
                cmData->periodicScanHandles[i].source = StreamPeriodicScanSource(dmPrim->sync_handle);
                SynergyStreamsSourceRegister(CSR_BT_CM_IFACEQUEUE, cmData->periodicScanHandles[i].source);
                cmData->periodicScanHandles[i].paSyncState &= ~CSR_BT_EA_PA_TERMINATING;
#endif /* CSR_STREAMS_ENABLE */
                cmData->periodicScanHandles[i].pending = FALSE;
            }
            else if (dmPrim->status != CM_ULP_PERIODIC_SCAN_SYNC_TO_TRAIN_PENDING)
            {
                cmData->periodicScanHandles[i].pHandle = CSR_SCHED_QID_INVALID;
                cmData->periodicScanHandles[i].pending = FALSE;
            }

            CsrSchedMessagePut(appHandle, CSR_BT_CM_PRIM, (prim));
            break;
        }
    }
    CsrBtCmDmLocalQueueHandler();
}


void CsrBtCmDmPeriodicScanSyncToTrainCancelCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL_CFM_T *dmPrim = msg;
    CmPeriodicScanSyncToTrainCancelCfm *prim;
    CsrUint8 i;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;

    /* Store periodic sync handle for sending Periodic Advert Report */
    if (dmPrim->status == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        for(i = 0; i < MAX_PERIODIC_SCAN_APP; i++)
        {
            if (cmData->periodicScanHandles[i].pHandle == cmData->dmVar.appHandle)
            {
                cmData->periodicScanHandles[i].pHandle = CSR_SCHED_QID_INVALID;
                cmData->periodicScanHandles[i].syncHandle = CSR_BT_PERIODIC_SCAN_HANDLE_INVALID;
#ifdef CSR_STREAMS_ENABLE
                cmData->periodicScanHandles[i].source = 0;
#endif /* CSR_STREAMS_ENABLE */
                break;
            }
        }
    }

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmPeriodicScanSyncAdvReportEnableCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_SCAN_SYNC_ADV_REPORT_ENABLE_CFM_T *dmPrim = msg;
    CmPeriodicScanSyncAdvReportEnableCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_ENABLE_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmPeriodicScanSyncTerminateCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_SCAN_SYNC_TERMINATE_CFM_T *dmPrim = msg;
    CmPeriodicScanSyncTerminateCfm *prim;
    CsrUint8 i;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_TERMINATE_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->syncHandle = dmPrim->sync_handle;

    /* Store periodic sync handle for sending Periodic Advert Report */
    for(i = 0; i < MAX_PERIODIC_SCAN_APP; i++)
    {
        if (cmData->periodicScanHandles[i].syncHandle == dmPrim->sync_handle)
        {
#ifdef CSR_STREAMS_ENABLE
            if(cmData->periodicScanHandles[i].paSyncState & CSR_BT_EA_PA_REPORT_PROCESSING)
            {
                cmData->periodicScanHandles[i].paSyncState |= CSR_BT_EA_PA_TERMINATING;
            }
            else
#endif /* CSR_STREAMS_ENABLE */
            {
#ifdef CSR_STREAMS_ENABLE
                CmStreamFlushSource(cmData->periodicScanHandles[i].source);
                cmData->periodicScanHandles[i].source = 0;
#endif /* CSR_STREAMS_ENABLE */

                cmData->periodicScanHandles[i].pHandle = CSR_SCHED_QID_INVALID;
                cmData->periodicScanHandles[i].syncHandle = CSR_BT_PERIODIC_SCAN_HANDLE_INVALID;
            }
            break;
        }
    }

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmPeriodicScanSyncAdvReportIndHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_SCAN_SYNC_ADV_REPORT_IND_T *dmPrim = msg;
    CmPeriodicScanSyncAdvReportInd *prim;
#ifdef CSR_STREAMS_ENABLE
    CmPeriodicScanSyncAdvReportDoneInd *donePrim;
    Source source = 0;
    CsrUint16 sourceSize = 0;
    CsrUint8* sourcePtr = NULL;
    CsrUint8 index = 0;
#endif /* CSR_STREAMS_ENABLE */
    CsrSchedQid appHandle = CSR_SCHED_QID_INVALID;
    CsrUint8 i = 0;

    for(i = 0; i < MAX_PERIODIC_SCAN_APP; i++)
    {
        if (cmData->periodicScanHandles[i].syncHandle == dmPrim->sync_handle)
        {
            appHandle = cmData->periodicScanHandles[i].pHandle;
#ifdef CSR_STREAMS_ENABLE
            source = cmData->periodicScanHandles[i].source;
            sourcePtr = (CsrUint8*) &cmData->periodicScanHandles[i].source;
            index = i;
            if (cmData->periodicScanHandles[i].paSyncState & CSR_BT_EA_PA_REPORT_PROCESSING)
            {
                CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "CsrBtCmDmPeriodicScanSyncAdvReportIndHandler Skip processing MESSAGE_MORE_DATA event with syncHandle=0x%x", dmPrim->sync_handle));
                return;
            }
#endif /* CSR_STREAMS_ENABLE */
            break;
        }
    }

    if(appHandle != CSR_SCHED_QID_INVALID)
    {
        prim = CsrPmemAlloc(sizeof(*prim));
        prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_IND;
        prim->syncHandle = dmPrim->sync_handle;
        prim->txPower = dmPrim->tx_power;
        prim->rssi = dmPrim->rssi;
        prim->cteType = dmPrim->cte_type;
        prim->dataLength = 0;
        prim->data = NULL;

#ifdef CSR_STREAMS_ENABLE
        if (source)
        {
            sourceSize = SourceBoundary(source);
            prim->data = (CsrUint8 *) SourceMap(source);
            prim->dataLength = sourceSize;
        }
#else
        if (dmPrim->adv_data)
        {
            /* Get the MBLK data length */
            prim->dataLength = mblk_get_length(dmPrim->adv_data);
            prim->data = (CsrUint8 *) CsrPmemAlloc(prim->dataLength);
            /* Read data from MBLK */
            mblk_read_head(&dmPrim->adv_data, prim->data, prim->dataLength);
        }
#endif /* CSR_STREAMS_ENABLE */

        if (!prim->data || !prim->dataLength)
        {
            CSR_LOG_TEXT_INFO((CsrBtCmLto, 0,  "CsrBtCmDmPeriodicScanSyncAdvReportIndHandler: Skip processing MESSAGE_MORE_DATA event with dataLength=0x%x or data is NULL", prim->dataLength));
            CsrPmemFree(prim);
            return;
        }

#ifdef CSR_STREAMS_ENABLE
        cmData->periodicScanHandles[index].paSyncState |= CSR_BT_EA_PA_REPORT_PROCESSING;
#endif

        CsrSchedMessagePut(appHandle, CSR_BT_CM_PRIM, (prim));

#ifdef CSR_STREAMS_ENABLE
        /* Sending CM_PERIODIC_SCAN_SYNC_ADV_REPORT_DONE_IND message to CM Task
         * to free the data memory once processing of advert report is completed. */
        donePrim = CsrPmemAlloc(sizeof(*donePrim));
        donePrim->type = CM_PERIODIC_SCAN_SYNC_ADV_REPORT_DONE_IND;

        donePrim->data = sourcePtr;
        donePrim->dataLength = sourceSize;
        CsrSchedMessagePut(CSR_BT_CM_IFACEQUEUE, CSR_BT_CM_PRIM, (donePrim));
#endif /* CSR_STREAMS_ENABLE */

    }
}

void CsrBtCmDmPeriodicScanSyncLostIndHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_SCAN_SYNC_LOST_IND_T *dmPrim = msg;
    CmPeriodicScanSyncLostInd *prim;
    CsrSchedQid appHandle = CSR_SCHED_QID_INVALID;
    CsrUint8 i;

    for(i = 0; i < MAX_PERIODIC_SCAN_APP; i++)
    {
        if (cmData->periodicScanHandles[i].syncHandle == dmPrim->sync_handle)
        {
            appHandle = cmData->periodicScanHandles[i].pHandle;
            break;
        }
    }

    if(appHandle != CSR_SCHED_QID_INVALID)
    {
        prim = CsrPmemAlloc(sizeof(*prim));
        prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_LOST_IND;
        prim->syncHandle = dmPrim->sync_handle;

        CsrSchedMessagePut(appHandle, CSR_BT_CM_PRIM, (prim));
    }
}

void CsrBtCmDmPeriodicScanSyncTransferCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_SCAN_SYNC_TRANSFER_CFM_T *dmPrim = msg;
    CmPeriodicScanSyncTransferCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->syncHandle = dmPrim->sync_handle;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmPeriodicScanSyncTransferIndHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_SCAN_SYNC_TRANSFER_IND_T *dmPrim = msg;
    CmPeriodicScanSyncTransferInd *prim;
    CsrUint8 i;

    if (cmData->pastAppHandle != CSR_SCHED_QID_INVALID)
    {
        /* Store periodic sync handle for sending Periodic Advert Report */
        if (dmPrim->status == CSR_BT_RESULT_CODE_CM_SUCCESS)
        {
            for(i = 0; i < MAX_PERIODIC_SCAN_APP; i++)
            {
                if (cmData->periodicScanHandles[i].pHandle == CSR_SCHED_QID_INVALID)
                {
                    cmData->periodicScanHandles[i].pHandle = cmData->pastAppHandle;
                    cmData->periodicScanHandles[i].syncHandle = dmPrim->sync_handle;
#ifdef CSR_STREAMS_ENABLE
                    cmData->periodicScanHandles[i].source = StreamPeriodicScanSource(dmPrim->sync_handle);
                    SynergyStreamsSourceRegister(CSR_BT_CM_IFACEQUEUE, cmData->periodicScanHandles[i].source);
#endif /* CSR_STREAMS_ENABLE */
                    cmData->periodicScanHandles[i].pending = FALSE;
                    break;
                }
            }
        }

        prim = CsrPmemAlloc(sizeof(*prim));
        prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_IND;
        prim->resultCode = (CsrBtResultCode) dmPrim->status;
        prim->advSid = dmPrim->adv_sid;
        prim->syncHandle = dmPrim->sync_handle;
        prim->serviceData = dmPrim->service_data;
        CsrBtAddrCopy(&(prim->addrt), &(dmPrim->adv_addr));

        CsrSchedMessagePut(cmData->pastAppHandle, CSR_BT_CM_PRIM, (prim));
    }
}

void CsrBtCmDmPeriodicScanSyncTransferParamsCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_CFM_T *dmPrim = msg;
    CmPeriodicScanSyncTransferParamsCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    CsrBtAddrCopy(&(prim->addrt), &(dmPrim->addrt));

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}
#endif /* End of CSR_BT_INSTALL_PERIODIC_SCANNING */
