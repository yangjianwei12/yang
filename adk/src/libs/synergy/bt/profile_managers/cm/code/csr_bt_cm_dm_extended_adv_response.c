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

#ifdef CSR_BT_INSTALL_EXTENDED_ADVERTISING

void CsrBtCmDmExtAdvRegisterAppAdvSetCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_ADV_REGISTER_APP_ADV_SET_CFM_T *dmPrim = msg;
    CmExtAdvRegisterAppAdvSetCfm *prim;

    if(dmPrim->status == HCI_SUCCESS)
    {
        cmData->extAdvAppHandle[dmPrim->adv_handle] = cmData->dmVar.appHandle;
    }

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_ADV_REGISTER_APP_ADV_SET_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->advHandle = dmPrim->adv_handle;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));	
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmExtAdvUnregisterAppAdvSetCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM_T *dmPrim = msg;
    CmExtAdvUnregisterAppAdvSetCfm *prim;

    if((dmPrim->status == HCI_SUCCESS) &&
       (cmData->advHandle < MAX_EXT_ADV_APP))
    {
        cmData->extAdvAppHandle[cmData->advHandle] = CSR_SCHED_QID_INVALID;
    }

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->advHandle = cmData->advHandle;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmExtAdvSetParamsCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_ADV_SET_PARAMS_CFM_T *dmPrim = msg;
    CmExtAdvSetParamsCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_ADV_SET_PARAMS_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->advSid = dmPrim->adv_sid;
    prim->advHandle = cmData->advHandle;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmExtAdvSetDataCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_HCI_ULP_EXT_ADV_SET_DATA_CFM_T *dmPrim = msg;
    CmExtAdvSetDataCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_ADV_SET_DATA_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->advHandle = cmData->advHandle;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmExtAdvSetScanRespDataCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_CFM_T *dmPrim = msg;
    CmExtAdvSetScanRespDataCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->advHandle = cmData->advHandle;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmExtAdvEnableCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_ADV_ENABLE_CFM_T *dmPrim = msg;
    CmExtAdvEnableCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_ADV_ENABLE_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->advHandle = cmData->advHandle;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmExtAdvReadMaxAdvDataLenCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM_T *dmPrim = msg;
    CmExtAdvReadMaxAdvDataLenCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->maxAdvData = dmPrim->max_adv_data;
    prim->maxScanRespData = dmPrim->max_scan_resp_data;
    prim->advHandle = cmData->advHandle;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmExtAdvTerminatedIndHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_ADV_TERMINATED_IND_T *dmPrim = msg;
    CmExtAdvTerminatedInd *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_ADV_TERMINATED_IND;
    prim->advHandle = dmPrim->adv_handle;
    prim->reason = dmPrim->reason;
    CsrBtAddrCopy(&(prim->addrt), &(dmPrim->addrt));
    prim->eaEvents = dmPrim->ea_events;
    prim->maxAdvSets = dmPrim->max_adv_sets;
    prim->advBits = dmPrim->adv_bits;

    if(dmPrim->adv_handle < MAX_EXT_ADV_APP)
    {
        CSR_LOG_TEXT_WARNING((CsrBtCmLto, 0, "CsrBtCmDmExtAdvTerminatedIndHandler id:%d",
                                cmData->extAdvAppHandle[dmPrim->adv_handle]));
        if(cmData->extAdvAppHandle[dmPrim->adv_handle] != CSR_SCHED_QID_INVALID)
        {
            CsrSchedMessagePut(cmData->extAdvAppHandle[dmPrim->adv_handle], CSR_BT_CM_PRIM, (prim));

            /* Clear the App handle for legacy adverts.
             * For extended adverts, the APP handle gets cleared on Unregister call success. But since
             * there is no unregister call for legacy adverts, we will need to clear the APP handle here.
             */
            if(dmPrim->adv_handle == ADV_HANDLE_FOR_LEGACY_API)
            {
                cmData->extAdvAppHandle[dmPrim->adv_handle] = CSR_SCHED_QID_INVALID;
            }
        }
    }

}

void CsrBtCmDmExtAdvSetRandomAddrCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_ADV_SET_RANDOM_ADDR_CFM_T *dmPrim = msg;
    CmExtAdvSetRandomAddrCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->advHandle = dmPrim->adv_handle;
    CsrBtAddrCopy(&(prim->randomAddr), &(dmPrim->random_addr));

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmExtAdvSetsInfoCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_ADV_SETS_INFO_CFM_T *dmPrim = msg;
    CmExtAdvSetsInfoCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_ADV_SETS_INFO_CFM;
    prim->flags = dmPrim->flags;
    prim->numAdvSets = dmPrim->num_adv_sets;
    SynMemCpyS(prim->advSets, sizeof(CmExtAdvSetInfo) * CM_EXT_ADV_MAX_REPORTED_ADV_SETS, 
               dmPrim->adv_sets, sizeof(CmExtAdvSetInfo) * CM_EXT_ADV_MAX_REPORTED_ADV_SETS);

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmExtAdvMultiEnableCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_ADV_MULTI_ENABLE_CFM_T *dmPrim = msg;
    CmExtAdvMultiEnableCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_EXT_ADV_MULTI_ENABLE_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->maxAdvSets = dmPrim->max_adv_sets;
    prim->advBits = dmPrim->adv_bits;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

#ifdef INSTALL_CM_EXT_ADV_SET_PARAM_V2
void CmDmExtAdvSetParamsV2CfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_ADV_SET_PARAMS_V2_CFM_T *dmPrim = msg;
    CmDmExtAdvSetParamsV2Cfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CM_DM_EXT_ADV_SET_PARAMS_V2_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->advSid = dmPrim->adv_sid;
    prim->advHandle = cmData->advHandle;
    prim->selectedTxPower = dmPrim->selected_tx_pwr;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}
#endif

void CmDmExtAdvGetAddressCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_EXT_ADV_GET_ADDR_CFM_T *dmPrim = msg;
    CmDmExtAdvGetAddrCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CM_DM_EXT_ADV_GET_ADDR_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->advHandle = dmPrim->adv_handle;
    prim->ownAddr = dmPrim->addrt;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

#endif /* End of CSR_BT_INSTALL_EXTENDED_ADVERTISING */
