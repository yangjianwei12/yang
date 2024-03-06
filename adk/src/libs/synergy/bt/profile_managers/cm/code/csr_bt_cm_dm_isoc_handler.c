/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include <stdio.h>

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_log_text_2.h"
#include "csr_bt_cm_le.h"


#ifdef CSR_LOG_ENABLE
#define CSR_BT_CM_LTSO_ISOC             0
#define CSR_BT_CM_ISOC_LOG_INFO(...)    CSR_LOG_TEXT_INFO((CsrBtCmLto, CSR_BT_CM_LTSO_ISOC, __VA_ARGS__))
#define CSR_BT_CM_ISOC_LOG_WARNING(...) CSR_LOG_TEXT_WARNING((CsrBtCmLto, CSR_BT_CM_LTSO_ISOC, __VA_ARGS__))
#define CSR_BT_CM_ISOC_LOG_ERROR(...)   CSR_LOG_TEXT_ERROR((CsrBtCmLto, CSR_BT_CM_LTSO_ISOC, __VA_ARGS__))
#else
#define CSR_BT_CM_ISOC_LOG_INFO(...)
#define CSR_BT_CM_ISOC_LOG_WARNING(...)
#define CSR_BT_CM_ISOC_LOG_ERROR(...)
#endif

void CsrBtCmDmIsocRegisterReqHandler(cmInstanceData_t *cmData)
{

    CmIsocRegisterReq *prim = cmData->recvMsgP;

    if(prim->isoc_type & CM_ISOC_TYPE_UNICAST)
    {
        cmData->isocUnicastHandle = prim->appHandle;
    }

    if(prim->isoc_type & CM_ISOC_TYPE_BROADCAST)
    {
        cmData->isocHandle = prim->appHandle;
    }

    cmData->pendingIsocHandle = prim->appHandle;
    dm_isoc_register_req(CSR_BT_CM_IFACEQUEUE, prim->isoc_type, 0, NULL);
}

void CsrBtCmDmIsocConfigureCigReqHandler(cmInstanceData_t *cmData)
{
    CmIsocConfigureCigReq *prim = cmData->recvMsgP;
    CsrUint8 i;
    DM_CIS_CONFIG_T *cis[CM_MAX_SUPPORTED_CIS];

    if (prim->cis_count <= 0 || prim->cis_count > CM_MAX_SUPPORTED_CIS)
    {
        CSR_BT_CM_ISOC_LOG_ERROR("CsrBtCmDmIsocConfigureCigReqHandler: Invalid parameters, dropping the message!!");
        return;
    }

    for(i = 0; i < prim->cis_count; i++)
    {
        cis[i] = CsrPmemAlloc(sizeof(DM_CIS_CONFIG_T));

        cis[i]->cis_id = prim->cis_config[i]->cis_id;
        cis[i]->max_sdu_m_to_s = prim->cis_config[i]->max_sdu_m_to_s;
        cis[i]->max_sdu_s_to_m = prim->cis_config[i]->max_sdu_s_to_m;
        cis[i]->phy_m_to_s = prim->cis_config[i]->phy_m_to_s;
        cis[i]->phy_s_to_m = prim->cis_config[i]->phy_s_to_m;
        cis[i]->rtn_m_to_s = prim->cis_config[i]->rtn_m_to_s;
        cis[i]->rtn_s_to_m = prim->cis_config[i]->rtn_s_to_m;

        CsrPmemFree(prim->cis_config[i]);
    }

    cmData->pendingIsocHandle = prim->appHandle;

    dm_isoc_configure_cig_req(CSR_BT_CM_IFACEQUEUE,
                              0,
                              prim->cig_id,
                              prim->sdu_interval_m_to_s,
                              prim->sdu_interval_s_to_m,
                              prim->sca,
                              prim->packing,
                              prim->framing,
                              prim->max_transport_latency_m_to_s,
                              prim->max_transport_latency_s_to_m,
                              prim->cis_count,
                              cis,
                              NULL);
}

void CsrBtCmDmIsocRemoveCigReqHandler(cmInstanceData_t *cmData)
{
    CmIsocRemoveCigReq *prim = cmData->recvMsgP;

    cmData->pendingIsocHandle = prim->appHandle;
    dm_isoc_remove_cig_req(prim->cig_id, NULL);
}

void CsrBtCmDmIsocCisConnectReqHandler(cmInstanceData_t *cmData)
{
    CmIsocCisConnectReq *prim = cmData->recvMsgP;
    CsrUint8 i;
    DM_CIS_CONNECTION_T *cis_conn[CM_MAX_SUPPORTED_CIS];

    if (prim->cis_count <= 0 || prim->cis_count > CM_MAX_SUPPORTED_CIS)
    {
        CSR_BT_CM_ISOC_LOG_ERROR("CsrBtCmDmIsocCisConnectReqHandler: Invalid parameters, dropping the message!!");
        return;
    }

    for(i = 0; i < prim->cis_count; i++)
    {
        cis_conn[i] = CsrPmemAlloc(sizeof(DM_CIS_CONNECTION_T));

        cis_conn[i]->cis_handle = prim->cis_conn[i]->cis_handle;
        CsrBtAddrCopy(&(cis_conn[i]->tp_addrt.addrt), &(prim->cis_conn[i]->tp_addrt.addrt));
        cis_conn[i]->tp_addrt.tp_type = prim->cis_conn[i]->tp_addrt.tp_type;
        CsrPmemFree(prim->cis_conn[i]);
    }

    cmData->pendingIsocHandle = prim->appHandle;
    dm_isoc_cis_connect_req(CSR_BT_CM_IFACEQUEUE, prim->con_context, prim->cis_count, cis_conn, NULL);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmIsocCisConnectRspHandler(cmInstanceData_t *cmData)
{
    CmIsocCisConnectRsp *prim = cmData->recvMsgP;

    /* Ignore response from other than ISOC Unicast manager */
    if ((cmData->isocUnicastHandle == CSR_SCHED_QID_INVALID) ||
        (prim->appHandle != cmData->isocUnicastHandle))
    {
        CsrBtCmDmLocalQueueHandler();
        return;
    }

    cmData->pendingIsocHandle = prim->appHandle;
    dm_isoc_cis_connect_rsp(CSR_BT_CM_IFACEQUEUE, prim->con_context, prim->cis_handle, prim->status, NULL);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmIsocCisDisconnectReqHandler(cmInstanceData_t *cmData)
{
    CmIsocCisDisconnectReq *prim = cmData->recvMsgP;

    cmData->pendingIsocHandle = prim->appHandle;
    dm_isoc_cis_disconnect_req(prim->cis_handle, prim->reason, NULL);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmIsocSetupIsoDataPathReqHandler(cmInstanceData_t *cmData)
{
    CmIsocSetupIsoDataPathReq *prim = cmData->recvMsgP;

    cmData->pendingIsocHandle = prim->appHandle;

    dm_isoc_setup_iso_data_path_req(prim->handle,
                                    prim->data_path_direction,
                                    prim->data_path_id,
                                    prim->codec_id,
                                    prim->controller_delay,
                                    prim->codec_config_length,
                                    prim->codec_config_data,
                                    NULL);
}

void CsrBtCmDmIsocRemoveIsoDataPathReqHandler(cmInstanceData_t *cmData)
{
    CmIsocRemoveIsoDataPathReq *prim = cmData->recvMsgP;

    cmData->pendingIsocHandle = prim->appHandle;
    dm_isoc_remove_iso_data_path_req(prim->handle, prim->data_path_direction, NULL);
}

void CsrBtCmDmIsocCreateBigReqHandler(cmInstanceData_t *cmData)
{
    context_t con_context = 1;
    CmIsocCreateBigReq *prim = cmData->recvMsgP;

    cmData->pendingIsocHandle = prim->appHandle;

    dm_isoc_create_big_req(CSR_BT_CM_IFACEQUEUE, con_context, (DM_BIG_CONFIG_PARAM_T *)&prim->big_config,
        prim->big_handle, prim->adv_handle, prim->num_bis, prim->encryption,
        prim->broadcast_code, NULL);
}

void CsrBtCmDmIsocTerminateBigReqHandler(cmInstanceData_t *cmData)
{
    CmIsocTerminateBigReq *prim = cmData->recvMsgP;

    cmData->pendingIsocHandle = prim->appHandle;
    dm_isoc_terminate_big_req(prim->big_handle, prim->reason, NULL);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmIsocBigCreateSyncReqHandler(cmInstanceData_t *cmData)
{
    context_t con_context = 1;
    CmIsocBigCreateSyncReq *prim = cmData->recvMsgP;

    cmData->pendingIsocHandle = prim->phandle;

    dm_isoc_big_create_sync_req(CSR_BT_CM_IFACEQUEUE, con_context, prim->big_handle,
        prim->sync_handle, prim->encryption, prim->broadcast_code, prim->mse,
        prim->big_sync_timeout, prim->num_bis, prim->bis, NULL);
}

void CsrBtCmDmIsocBigTerminateSyncReqHandler(cmInstanceData_t *cmData)
{
    CmIsocBigTerminateSyncReq *prim = cmData->recvMsgP;

    cmData->pendingIsocHandle = prim->phandle;

    dm_isoc_big_terminate_sync_req(prim->big_handle, NULL);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmIsocConfigureCigTestReqHandler(cmInstanceData_t *cmData)
{
    CmIsocConfigureCigTestReq *prim = cmData->recvMsgP;
    CsrUint8 i;
    DM_CIS_CONFIG_TEST_T *cis[CM_MAX_SUPPORTED_CIS];

    if (prim->cis_count <= 0 || prim->cis_count > CM_MAX_SUPPORTED_CIS)
    {
        CSR_BT_CM_ISOC_LOG_ERROR("CsrBtCmDmIsocConfigureCigTestReqHandler: Invalid parameters, dropping the message!!");
        return;
    }

    for(i = 0; i < prim->cis_count; i++)
    {
        cis[i] = CsrPmemAlloc(sizeof(DM_CIS_CONFIG_TEST_T));

        cis[i]->max_sdu_m_to_s = prim->cis_test_config[i]->max_sdu_m_to_s;
        cis[i]->max_sdu_s_to_m = prim->cis_test_config[i]->max_sdu_s_to_m;
        cis[i]->max_pdu_m_to_s = prim->cis_test_config[i]->max_pdu_m_to_s;
        cis[i]->max_pdu_s_to_m = prim->cis_test_config[i]->max_pdu_s_to_m;
        cis[i]->cis_id = prim->cis_test_config[i]->cis_id;
        cis[i]->nse = prim->cis_test_config[i]->nse;
        cis[i]->phy_m_to_s = prim->cis_test_config[i]->phy_m_to_s;
        cis[i]->phy_s_to_m = prim->cis_test_config[i]->phy_s_to_m;
        cis[i]->bn_m_to_s = prim->cis_test_config[i]->bn_m_to_s;
        cis[i]->bn_s_to_m = prim->cis_test_config[i]->bn_s_to_m;

        CsrPmemFree(prim->cis_test_config[i]);
    }

    cmData->pendingIsocHandle = prim->appHandle;

    dm_isoc_configure_cig_test_req(CSR_BT_CM_IFACEQUEUE,
                                   0,
                                   prim->sdu_interval_m_to_s,
                                   prim->sdu_interval_s_to_m,
                                   prim->iso_interval,
                                   prim->cig_id,
                                   prim->ft_m_to_s,
                                   prim->ft_s_to_m,
                                   prim->sca,
                                   prim->packing,
                                   prim->framing,
                                   prim->cis_count,
                                   cis,
                                   NULL);
}

void CsrBtCmDmIsocCreateBigTestReqHandler(cmInstanceData_t *cmData)
{
    context_t con_context = 1;
    CmIsocCreateBigTestReq *prim = cmData->recvMsgP;

    cmData->pendingIsocHandle = prim->appHandle;

    dm_isoc_create_big_test_req(CSR_BT_CM_IFACEQUEUE, con_context, (DM_BIG_TEST_CONFIG_PARAM_T *)&prim->big_config,
                                prim->big_handle, prim->adv_handle, prim->num_bis, prim->encryption,
                                prim->broadcast_code, NULL);
}

void CmDmIsocReadIsoLinkQualityReqHandler(cmInstanceData_t *cmData)
{
    CmIsocReadIsoLinkQualityReq *prim = cmData->recvMsgP;

    cmData->pendingIsocHandle = prim->appHandle;

    dm_isoc_read_iso_link_quality_req(prim->handle, NULL);
}
