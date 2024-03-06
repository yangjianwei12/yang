/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_log_text_2.h"
#include "csr_bt_cm_le.h"

#ifdef CSR_BT_INSTALL_EXTENDED_SCANNING

void CsrBtCmDmExtScanGetGlobalParamsReqHandler(cmInstanceData_t *cmData)
{
    CmExtScanGetGlobalParamsReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;

    dm_ext_scan_get_global_params_req(CSR_BT_CM_IFACEQUEUE, NULL);
}

void CsrBtCmDmExtScanSetGlobalParamsReqHandler(cmInstanceData_t *cmData)
{
    CmExtScanSetGlobalParamsReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;

    dm_ext_scan_set_global_params_req(CSR_BT_CM_IFACEQUEUE, prim->flags, prim->own_address_type, 
        prim->scanning_filter_policy, prim->filter_duplicates, prim->scanning_phy,
        (ES_SCANNING_PHY_T *)prim->phys, NULL);
}

void CsrBtCmDmExtScanRegisterScannerReqHandler(cmInstanceData_t *cmData)
{
    CmExtScanRegisterScannerReq *prim = cmData->recvMsgP;
    CsrUint8 i;

    for(i = 0; i < MAX_EXT_SCAN_APP; i++)
    {
        if(cmData->extScanHandles[i].pHandle ==  CSR_SCHED_QID_INVALID)
        {
            cmData->extScanHandles[i].pHandle = prim->appHandle;
            cmData->extScanHandles[i].pending = TRUE;
            break;
        }
    }

    if (i < MAX_EXT_SCAN_APP)
    {
        cmData->dmVar.appHandle = prim->appHandle;
        dm_ext_scan_register_scanner_req(CSR_BT_CM_IFACEQUEUE, prim->flags, prim->adv_filter, 
            prim->adv_filter_sub_field1, prim->adv_filter_sub_field2, prim->ad_structure_filter, prim->ad_structure_filter_sub_field1,
            prim->ad_structure_filter_sub_field2, prim->num_reg_ad_types,prim->reg_ad_types, NULL);

        return;
    }
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmExtScanUnregisterScannerReqHandler(cmInstanceData_t *cmData)
{
    CmExtScanUnregisterScannerReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;

    dm_ext_scan_unregister_scanner_req(CSR_BT_CM_IFACEQUEUE, prim->scan_handle, NULL);
}

void CsrBtCmDmExtScanConfigureScannerReqHandler(cmInstanceData_t *cmData)
{
    CmExtScanConfigureScannerReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;

    dm_ext_scan_configure_scanner_req(prim->scan_handle, prim->use_only_global_params,
        prim->scanning_phys, (DM_ULP_EXT_SCAN_PHY_T *)prim->phys,NULL);
}

void CsrBtCmDmExtScanEnableScannersReqHandler(cmInstanceData_t *cmData)
{
    CmExtScanEnableScannersReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;

    dm_ext_scan_enable_scanners_req(CSR_BT_CM_IFACEQUEUE, prim->enable, prim->num_of_scanners,
                    (DM_ULP_EXT_SCAN_SCANNERS_T *)prim->scanners, NULL);
}

void CsrBtCmDmExtScanGetCtrlScanInfoReqHandler(cmInstanceData_t *cmData)
{
    CmExtScanGetCtrlScanInfoReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;

    dm_ext_scan_get_ctrl_scan_info_req(CSR_BT_CM_IFACEQUEUE, NULL);
}

#ifdef INSTALL_CM_EXT_SET_CONN_PARAM
void CmDmExtSetConnParamsReqHandler(cmInstanceData_t *cmData)
{
    CmDmExtSetConnParamsReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;

    dm_ext_set_connection_params_req(CSR_BT_CM_IFACEQUEUE, prim->advHandle, prim->subevent,
       prim->connAttemptTimeout, prim->ownAddressType, prim->phyCount,
       prim->initPhys, NULL);
}
#endif /* End of INSTALL_CM_EXT_SET_CONN_PARAM */
#endif /* End of CSR_BT_INSTALL_EXTENDED_SCANNING */
