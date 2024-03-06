/******************************************************************************
 Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_log_text_2.h"
#include "csr_bt_cm_le.h"

#ifdef CSR_BT_INSTALL_PERIODIC_ADVERTISING

void CsrBtCmDmPeriodicAdvSetParamsCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_ADV_SET_PARAMS_CFM_T *dmPrim = msg;
    CmPeriodicAdvSetParamsCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_PERIODIC_ADV_SET_PARAMS_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->advHandle = cmData->advHandle;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmPeriodicAdvSetDataCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_HCI_ULP_PERIODIC_ADV_SET_DATA_CFM_T *dmPrim = msg;
    CmPeriodicAdvSetDataCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_PERIODIC_ADV_SET_DATA_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->advHandle = cmData->advHandle;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmPeriodicAdvReadMaxAdvDataLenCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_ADV_READ_MAX_ADV_DATA_LEN_CFM_T *dmPrim = msg;
    CmPeriodicAdvReadMaxAdvDataLenCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_PERIODIC_ADV_READ_MAX_ADV_DATA_LEN_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->maxAdvData = dmPrim->max_adv_data;
    prim->advHandle = cmData->advHandle;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmPeriodicAdvStartCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_ADV_START_CFM_T *dmPrim = msg;
    CmPeriodicAdvStartCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_PERIODIC_ADV_START_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->advHandle = cmData->advHandle;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmPeriodicAdvStopCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_ADV_STOP_CFM_T *dmPrim = msg;
    CmPeriodicAdvStopCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_PERIODIC_ADV_STOP_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->advHandle = cmData->advHandle;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmPeriodicAdvSetTransferCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_ADV_SET_TRANSFER_CFM_T *dmPrim = msg;
    CmPeriodicAdvSetTransferCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_PERIODIC_ADV_SET_TRANSFER_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->advHandle = cmData->advHandle;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmPeriodicAdvEnableCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_PERIODIC_ADV_ENABLE_CFM_T *dmPrim = msg;
    CmPeriodicAdvEnableCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_PERIODIC_ADV_ENABLE_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->advHandle = cmData->advHandle;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}
#endif /* End of CSR_BT_INSTALL_PERIODIC_ADVERTISING */
