/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_log_text_2.h"
#include "csr_bt_cm_le.h"

#ifdef CSR_BT_INSTALL_PERIODIC_ADVERTISING
void CsrBtCmDmPeriodicAdvSetParamsReqHandler(cmInstanceData_t *cmData)
{
    CmPeriodicAdvSetParamsReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;
    cmData->advHandle = prim->advHandle;

    dm_periodic_adv_set_params_req(prim->advHandle,
                                prim->flags,
                                prim->periodicAdvIntervalMin,
                                prim->periodicAdvIntervalMax,
                                prim->periodicAdvProperties,
                                NULL);
}

void CsrBtCmDmPeriodicAdvSetDataReqHandler(cmInstanceData_t *cmData)
{
    CmPeriodicAdvSetDataReq *prim = cmData->recvMsgP;
    uint8_t *data = NULL;

    if (prim->dataLength)
    {
        uint8_t index, offset, length;
        data = CsrPmemAlloc(prim->dataLength);

        for(offset = 0, index = 0; offset < prim->dataLength;
                               index++, offset += length)
        {
            length = prim->dataLength - offset;
            if(length > CM_EXT_ADV_SCAN_RESP_DATA_BLOCK_SIZE)
                length = CM_EXT_ADV_SCAN_RESP_DATA_BLOCK_SIZE;

            memcpy(&data[offset], prim->data[index], length);
        }
    }

    cmData->dmVar.appHandle = prim->appHandle;
    cmData->advHandle = prim->advHandle;
    dm_periodic_adv_set_data_req(prim->advHandle,
                                prim->operation,
                                prim->dataLength,
                                data,
                                NULL);

    if (data)
        CsrPmemFree(data);
}

void CsrBtCmDmPeriodicAdvReadMaxAdvDataLenReqHandler(cmInstanceData_t *cmData)
{
    CmPeriodicAdvReadMaxAdvDataLenReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;
    cmData->advHandle = prim->advHandle;

    dm_periodic_adv_read_max_adv_data_len_req(prim->advHandle, NULL);
}

void CsrBtCmDmPeriodicAdvStartReqHandler(cmInstanceData_t *cmData)
{
    CmPeriodicAdvStartReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;
    cmData->advHandle = prim->advHandle;

    dm_periodic_adv_start_req(prim->advHandle, NULL);
}

void CsrBtCmDmPeriodicAdvStopReqHandler(cmInstanceData_t *cmData)
{
    CmPeriodicAdvStopReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;
    cmData->advHandle = prim->advHandle;

    dm_periodic_adv_stop_req(prim->advHandle, prim->stopAdvertising, NULL);
}

void CsrBtCmDmPeriodicAdvSetTransferReqHandler(cmInstanceData_t *cmData)
{
    CmPeriodicAdvSetTransferReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->phandle;
    cmData->advHandle = prim->advHandle;

    dm_periodic_adv_set_transfer_req(CSR_BT_CM_IFACEQUEUE,
                                    &prim->addrt,
                                    prim->serviceData,
                                    prim->advHandle,
                                    NULL);
}

void CsrBtCmDmPeriodicAdvEnableReqHandler(cmInstanceData_t *cmData)
{
    CmPeriodicAdvEnableReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->phandle;
    cmData->advHandle = prim->adv_handle;

    dm_periodic_adv_enable_req(prim->adv_handle, prim->flags, prim->enable, NULL);
}
#endif /* End of CSR_BT_INSTALL_PERIODIC_ADVERTISING */
