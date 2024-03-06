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

void CsrBtCmDmExtAdvRegisterAppAdvSetReqHandler(cmInstanceData_t *cmData)
{
    CmExtAdvRegisterAppAdvSetReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;

    dm_ext_adv_register_app_adv_set_req(prim->advHandle, prim->flags, NULL);
}

void CsrBtCmDmExtAdvUnregisterAppAdvSetReqHandler(cmInstanceData_t *cmData)
{
    CmExtAdvUnregisterAppAdvSetReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;
    cmData->advHandle = prim->advHandle;

    dm_ext_adv_unregister_app_adv_set_req(prim->advHandle, NULL);
}

void CsrBtCmDmExtAdvSetParamsReqHandler(cmInstanceData_t *cmData)
{
    CmExtAdvSetParamsReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;
    cmData->advHandle = prim->advHandle;

    dm_ext_adv_set_params_req(prim->advHandle,
                            prim->advEventProperties,
                            prim->primaryAdvIntervalMin,
                            prim->primaryAdvIntervalMax,
                            prim->primaryAdvChannelMap,
                            prim->ownAddrType,
                            &prim->peerAddr,
                            prim->advFilterPolicy,
                            prim->primaryAdvPhy,
                            prim->secondaryAdvMaxSkip,
                            prim->secondaryAdvPhy,
                            prim->advSid,
                            prim->reserved,
                            NULL);
}

void CsrBtCmDmExtAdvSetDataReqHandler(cmInstanceData_t *cmData)
{
    CmExtAdvSetDataReq *prim = cmData->recvMsgP;
    uint8_t *data = NULL;

    if (prim->dataLength)
    {
        uint8_t index, offset, length;
        data = CsrPmemAlloc(prim->dataLength);

        for(offset = 0, index = 0; offset < prim->dataLength;
                               index++, offset += length)
        {
            length = prim->dataLength - offset;
            if(length > CM_EXT_ADV_DATA_BLOCK_SIZE)
                length = CM_EXT_ADV_DATA_BLOCK_SIZE;

            memcpy(&data[offset], prim->data[index], length);
        }
    }

    cmData->dmVar.appHandle = prim->appHandle;
    cmData->advHandle = prim->advHandle;

    dm_hci_ext_adv_set_data_req(prim->advHandle,
                            prim->operation,
                            prim->fragPreference,
                            prim->dataLength,
                            data,
                            NULL);

    if (data)
        CsrPmemFree(data);
}

void CsrBtCmDmExtAdvSetScanRespDataReqHandler(cmInstanceData_t *cmData)
{
    CmExtAdvSetScanRespDataReq *prim = cmData->recvMsgP;
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

    dm_hci_ext_adv_set_scan_resp_data_req(prim->advHandle,
                            prim->operation,
                            prim->fragPreference,
                            prim->dataLength,
                            data,
                            NULL);

    if (data)
        CsrPmemFree(data);
}

void CsrBtCmDmExtAdvEnableReqHandler(cmInstanceData_t *cmData)
{
    CmExtAdvEnableReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;
    cmData->advHandle = prim->advHandle;

    dm_ext_adv_enable_req(prim->advHandle, prim->enable, NULL);
}

void CsrBtCmDmExtAdvReadMaxAdvDataLenReqHandler(cmInstanceData_t *cmData)
{
    CmExtAdvReadMaxAdvDataLenReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;
    cmData->advHandle = prim->advHandle;

    dm_ext_adv_read_max_adv_data_len_req(prim->advHandle, NULL);
}

void CsrBtCmDmExtAdvSetRandomAddrReqHandler(cmInstanceData_t *cmData)
{
    CmExtAdvSetRandomAddrReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;

    dm_ext_adv_set_random_address_req(prim->advHandle,
                                      prim->action,
                                      &prim->randomAddr,
                                      NULL);
}

void CsrBtCmDmExtAdvSetsInfoReqHandler(cmInstanceData_t *cmData)
{
    CmExtAdvSetsInfoReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;

    dm_ext_adv_sets_info_req(NULL);
}

void CsrBtCmDmExtAdvMultiEnableReqHandler(cmInstanceData_t *cmData)
{
    CmExtAdvMultiEnableReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;

    dm_ext_adv_multi_enable_req(prim->enable,
                                prim->numSets,
                                (EA_ENABLE_CONFIG_T *) prim->config,
                                NULL);
}

#ifdef INSTALL_CM_EXT_ADV_SET_PARAM_V2
void CmDmExtAdvSetParamsV2ReqHandler(cmInstanceData_t *cmData)
{
    CmDmExtAdvSetParamsV2Req *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;
    cmData->advHandle = prim->advHandle;

    dm_ext_adv_set_params_v2_req(prim->advHandle,
                            prim->advEventProperties,
                            prim->primaryAdvIntervalMin,
                            prim->primaryAdvIntervalMax,
                            prim->primaryAdvChannelMap,
                            prim->ownAddrType,
                            &prim->peerAddr,
                            prim->advFilterPolicy,
                            prim->primaryAdvPhy,
                            prim->secondaryAdvMaxSkip,
                            prim->secondaryAdvPhy,
                            prim->advSid,
                            prim->advTxPower,
                            prim->scanReqNotifyEnable,
                            prim->primaryAdvPhyOptions,
                            prim->secondaryAdvPhyOptions,
                            NULL);
}
#endif /* INSTALL_CM_EXT_ADV_SET_PARAM_V2 */


void CmDmExtAdvGetAddressReqHandler(cmInstanceData_t *cmData)
{
    CmDmExtAdvGetAddrReq *prim = cmData->recvMsgP;
    cmData->dmVar.appHandle = prim->appHandle;

    dm_ext_adv_get_addr_req(prim->advHandle, NULL);
}

#endif /* End of CSR_BT_INSTALL_EXTENDED_ADVERTISING */
