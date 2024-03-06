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

#ifdef CSR_BT_INSTALL_PERIODIC_SCANNING

#ifdef CSR_STREAMS_ENABLE
extern void CmStreamFlushSource(Source src);
#endif

void CsrBtCmDmPeriodicScanStartFindTrainsReqHandler(cmInstanceData_t *cmData)
{
    CmPeriodicScanStartFindTrainsReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->phandle;

    dm_periodic_scan_start_find_trains_req(CSR_BT_CM_IFACEQUEUE,
                                    prim->flags,
                                    prim->scanForXSeconds,
                                    prim->adStructureFilter,
                                    prim->adStructureFilterSubField1,
                                    prim->adStructureFilterSubField2,
                                    prim->adStructureInfoLen,
                                    prim->adStructureInfo,
                                    NULL);

    if (prim->adStructureInfo)
        CsrPmemFree(prim->adStructureInfo);
}

void CsrBtCmDmPeriodicScanStopFindTrainsReqHandler(cmInstanceData_t *cmData)
{
    CmPeriodicScanStopFindTrainsReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->phandle;

    dm_periodic_scan_stop_find_trains_req(CSR_BT_CM_IFACEQUEUE,
                                    prim->scanHandle,
                                    NULL);
}

void CsrBtCmDmPeriodicScanSyncToTrainReqHandler(cmInstanceData_t *cmData)
{
    CmPeriodicScanSyncToTrainReq *prim = cmData->recvMsgP;
    CsrUint8 i;

    for(i = 0; i < MAX_PERIODIC_SCAN_APP; i++)
    {
        if(cmData->periodicScanHandles[i].pHandle ==  CSR_SCHED_QID_INVALID)
        {
            cmData->periodicScanHandles[i].pHandle = prim->phandle;
            cmData->periodicScanHandles[i].pending = TRUE;
            break;
        }
    }

    if (i < MAX_PERIODIC_SCAN_APP)
    {
        cmData->dmVar.appHandle = prim->phandle;

        dm_periodic_scan_sync_to_train_req(CSR_BT_CM_IFACEQUEUE,
                                        prim->reportPeriodic,
                                        prim->skip,
                                        prim->syncTimeout,
                                        prim->syncCteType,
                                        prim->attemptSyncForXSeconds,
                                        prim->numberOfPeriodicTrains,
                                        (DM_ULP_PERIODIC_SCAN_TRAINS_T *)prim->periodicTrains,
                                        NULL); 
        return;
    }
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmPeriodicScanSyncToTrainCancelReqHandler(cmInstanceData_t *cmData)
{
    CmPeriodicScanSyncToTrainCancelReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->phandle;

    dm_periodic_scan_sync_to_train_cancel_req(CSR_BT_CM_IFACEQUEUE, NULL);
}

void CsrBtCmDmPeriodicScanSyncAdvReportEnableReqHandler(cmInstanceData_t *cmData)
{
    CmPeriodicScanSyncAdvReportEnableReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->phandle;

    dm_periodic_scan_sync_adv_report_enable_req(CSR_BT_CM_IFACEQUEUE,
                                            (uint16_t) prim->syncHandle,
                                            prim->enable,
                                            NULL);
}

void CsrBtCmDmPeriodicScanSyncTerminateReqHandler(cmInstanceData_t *cmData)
{
    CmPeriodicScanSyncTerminateReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->phandle;

    dm_periodic_scan_sync_terminate_req(CSR_BT_CM_IFACEQUEUE,
                                        prim->syncHandle,
                                        NULL);
}

void CsrBtCmDmPeriodicScanSyncLostRspHandler(cmInstanceData_t *cmData)
{
    CmPeriodicScanSyncLostRsp *prim = cmData->recvMsgP;
    CsrUint8 i;

    /* Remove Sync Handle from the list */
    for(i = 0; i < MAX_PERIODIC_SCAN_APP; i++)
    {
        if(cmData->periodicScanHandles[i].syncHandle ==  prim->syncHandle)
        {
#ifdef CSR_STREAMS_ENABLE
            /* If any report is being processed then mark for termination and defer invalidating 'source' */
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

            cmData->periodicScanHandles[i].pending = FALSE;
            break;
        }
    }

    dm_periodic_scan_sync_lost_rsp(prim->syncHandle, NULL);

    CsrBtCmDmLocalQueueHandler();

}


void CsrBtCmDmPeriodicScanSyncTransferReqHandler(cmInstanceData_t *cmData)
{
    CmPeriodicScanSyncTransferReq *prim = cmData->recvMsgP;
    cmData->pastAppHandle = cmData->dmVar.appHandle = prim->phandle;

    dm_periodic_scan_sync_transfer_req(CSR_BT_CM_IFACEQUEUE,
                                        &prim->addrt,
                                        prim->serviceData,
                                        prim->syncHandle,
                                        NULL);
}


void CsrBtCmDmPeriodicScanSyncTransferParamsReqHandler(cmInstanceData_t *cmData)
{
    CmPeriodicScanSyncTransferParamsReq *prim = cmData->recvMsgP;
    cmData->pastAppHandle = cmData->dmVar.appHandle = prim->phandle;

    dm_periodic_scan_sync_transfer_params_req(CSR_BT_CM_IFACEQUEUE,
                                        &prim->addrt,
                                        prim->mode,
                                        prim->skip,
                                        prim->syncTimeout,
                                        prim->cteType,
                                        NULL);
}
#endif /* End of CSR_BT_INSTALL_PERIODIC_SCANNING */
