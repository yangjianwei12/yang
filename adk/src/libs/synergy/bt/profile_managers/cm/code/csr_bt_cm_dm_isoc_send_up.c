/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
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

void CsrBtCmDmIsocRegisterCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_REGISTER_CFM_T *dmPrim = msg;
    CmIsocRegisterCfm *prim;
    CsrBtResultCode   resultCode;

    if (dmPrim->status == HCI_SUCCESS)
    {
        resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
    }
    else
    {
        resultCode      = (CsrBtResultCode) dmPrim->status;
    }
    prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_BT_CM_ISOC_REGISTER_CFM;
    prim->isoc_type = dmPrim->isoc_type;
    prim->resultCode = resultCode;

    CsrSchedMessagePut(cmData->pendingIsocHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmIsocConfigureCigCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_CONFIGURE_CIG_CFM_T *dmPrim = msg;
    CmIsocConfigureCigCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_ISOC_CONFIGURE_CIG_CFM;
    prim->cig_id = dmPrim->cig_id;
    prim->cis_count = dmPrim->cis_count;

    if(dmPrim->status == HCI_SUCCESS)
    {
        CsrUint8 i;

        prim->resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;

        for (i = 0; i < dmPrim->cis_count; i++)
            prim->cis_handles[i] = dmPrim->cis_handles[i];
    }
    else
    {
        prim->resultCode = (CsrBtResultCode) dmPrim->status;
    }

    CsrSchedMessagePut(cmData->pendingIsocHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmIsocRemoveCigCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_REMOVE_CIG_CFM_T *dmPrim = msg;
    CmIsocRemoveCigCfm *prim;
    CsrBtResultCode   resultCode;

    if (dmPrim->status == HCI_SUCCESS)
    {
        resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
    }
    else
    {
        resultCode      = (CsrBtResultCode) dmPrim->status;
    }

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_ISOC_REMOVE_CIG_CFM;
    prim->cig_id = dmPrim->cig_id;
    prim->resultCode = resultCode;

    CsrSchedMessagePut(cmData->pendingIsocHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmIsocCisConnectIndHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_CIS_CONNECT_IND_T *dmPrim = msg;
    CmIsocCisConnectInd *prim;

    if (cmData->isocUnicastHandle != CSR_SCHED_QID_INVALID)
    {
        prim = CsrPmemAlloc(sizeof(*prim));
        prim->type = CSR_BT_CM_ISOC_CIS_CONNECT_IND;
        prim->cig_id = dmPrim->cig_id;
        prim->cis_handle = dmPrim->cis_handle;
        prim->cis_id = dmPrim->cis_id;
        prim->tp_addrt.tp_type = dmPrim->tp_addrt.tp_type;
        CsrBtAddrCopy(&(prim->tp_addrt.addrt), &(dmPrim->tp_addrt.addrt));

        CsrSchedMessagePut(cmData->isocUnicastHandle, CSR_BT_CM_PRIM, (prim));
    }
}

void CsrBtCmDmIsocCisConnectCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_CIS_CONNECT_CFM_T *dmPrim = msg;
    CmIsocCisConnectCfm *prim;
    CsrBtResultCode   resultCode;

    if (dmPrim->status == HCI_SUCCESS)
    {
        resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
    }
    else
    {
        resultCode      = (CsrBtResultCode) dmPrim->status;
    }

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_ISOC_CIS_CONNECT_CFM;
    prim->cis_handle = dmPrim->cis_handle;
    prim->tp_addr.tp_type = dmPrim->tp_addr.tp_type;
    CsrBtAddrCopy(&(prim->tp_addr.addrt), &(dmPrim->tp_addr.addrt));
    prim->resultCode = resultCode;
    SynMemCpyS(&prim->cis_params, sizeof(CmCisParam), &dmPrim->cis_params, sizeof(CmCisParam));

    CsrSchedMessagePut(cmData->pendingIsocHandle, CSR_BT_CM_PRIM, (prim));
}

void CsrBtCmDmIsocCisDisconnectCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_CIS_DISCONNECT_CFM_T *dmPrim = msg;
    CmIsocCisDisconnectCfm *prim;
    CsrBtResultCode   resultCode;

     if (dmPrim->status == HCI_SUCCESS)
     {
         resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
     }
     else
     {
         resultCode      = (CsrBtResultCode) dmPrim->status;
     }

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_ISOC_CIS_DISCONNECT_CFM;
    prim->cis_handle = dmPrim->cis_handle;
    prim->resultCode = resultCode;

    CsrSchedMessagePut(cmData->pendingIsocHandle, CSR_BT_CM_PRIM, (prim));
}

void CsrBtCmDmIsocCisDisconnectIndHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_CIS_DISCONNECT_IND_T *dmPrim = msg;
    CmIsocCisDisconnectInd *prim;

    if (cmData->isocUnicastHandle != CSR_SCHED_QID_INVALID)
    {
        prim = CsrPmemAlloc(sizeof(*prim));
        prim->type = CSR_BT_CM_ISOC_CIS_DISCONNECT_IND;
        prim->cis_handle = dmPrim->cis_handle;
        prim->reason = dmPrim->reason;

        CsrSchedMessagePut(cmData->isocUnicastHandle, CSR_BT_CM_PRIM, (prim));
    }
}

void CsrBtCmDmIsocSetupIsoDataPathCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_SETUP_ISO_DATA_PATH_CFM_T *dmPrim = msg;
    CmIsocSetupIsoDataPathCfm *prim;
    CsrBtResultCode   resultCode;

     if (dmPrim->status == HCI_SUCCESS)
     {
         resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
     }
     else
     {
         resultCode      = (CsrBtResultCode) dmPrim->status;
     }

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_ISOC_SETUP_ISO_DATA_PATH_CFM;
    prim->handle = dmPrim->handle;
    prim->resultCode = resultCode;

    CsrSchedMessagePut(cmData->pendingIsocHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmIsocRemoveIsoDataPathCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_REMOVE_ISO_DATA_PATH_CFM_T *dmPrim = msg;
    CmIsocRemoveIsoDataPathCfm *prim;
    CsrBtResultCode   resultCode;

     if (dmPrim->status == HCI_SUCCESS)
     {
         resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
     }
     else
     {
         resultCode      = (CsrBtResultCode) dmPrim->status;
     }

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_ISOC_REMOVE_ISO_DATA_PATH_CFM;
    prim->handle = dmPrim->handle;
    prim->resultCode = resultCode;

    CsrSchedMessagePut(cmData->pendingIsocHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmIsocCreateBigCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_CREATE_BIG_CFM_T *dmPrim = msg;
    CmIsocCreateBigCfm *prim;
    CsrBtResultCode   resultCode;

    if (dmPrim->status == HCI_SUCCESS)
    {
        resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
    }
    else
    {
        resultCode      = (CsrBtResultCode) dmPrim->status;
    }

    prim = CsrPmemZalloc(sizeof(*prim));
    prim->type = CSR_BT_CM_ISOC_CREATE_BIG_CFM;
    prim->big_sync_delay = dmPrim->big_sync_delay;
    SynMemCpyS(&prim->big_params, sizeof(CmBigParam), &dmPrim->big_params, sizeof(CmBigParam));
    prim->big_handle = dmPrim->big_handle;
    prim->num_bis = dmPrim->num_bis;
    prim->resultCode = resultCode;
    if(prim->num_bis && dmPrim->bis_handles)
    {
        prim->bis_handles = CsrPmemZalloc(prim->num_bis*sizeof(hci_connection_handle_t));
        SynMemCpyS(prim->bis_handles, (prim->num_bis*sizeof(hci_connection_handle_t)),
                   dmPrim->bis_handles, (prim->num_bis*sizeof(hci_connection_handle_t)));
    }

    CsrSchedMessagePut(cmData->pendingIsocHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmIsocTerminateBigCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_TERMINATE_BIG_CFM_T *dmPrim = msg;
    CmIsocTerminateBigCfm *prim;
    CsrBtResultCode   resultCode;

    if (dmPrim->status_or_reason == HCI_SUCCESS)
    {
        resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
    }
    else
    {
        resultCode      = (CsrBtResultCode) dmPrim->status_or_reason;
    }

    prim = CsrPmemZalloc(sizeof(*prim));
    prim->type = CSR_BT_CM_ISOC_TERMINATE_BIG_CFM;
    prim->big_handle = dmPrim->big_handle;
    prim->resultCode = resultCode;

    CsrSchedMessagePut(cmData->pendingIsocHandle, CSR_BT_CM_PRIM, (prim));
}


void CsrBtCmDmIsocBigCreateSyncCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_BIG_CREATE_SYNC_CFM_T *dmPrim = msg;
    CmIsocBigCreateSyncCfm *prim;
    CsrBtResultCode   resultCode;

    if (dmPrim->status == HCI_SUCCESS)
    {
        resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
    }
    else
    {
        resultCode      = (CsrBtResultCode) dmPrim->status;
    }

    prim = CsrPmemZalloc(sizeof(*prim));
    prim->type = CSR_BT_CM_ISOC_BIG_CREATE_SYNC_CFM;
    SynMemCpyS(&prim->big_params, sizeof(CmBigParam), &dmPrim->big_params, sizeof(CmBigParam));
    prim->big_handle = dmPrim->big_handle;
    prim->num_bis = dmPrim->num_bis;
    prim->resultCode = resultCode;
    if(prim->num_bis && dmPrim->bis_handles)
    {
        prim->bis_handles = CsrPmemZalloc(prim->num_bis*sizeof(hci_connection_handle_t));
        SynMemCpyS(prim->bis_handles, (prim->num_bis*sizeof(hci_connection_handle_t)),
                   dmPrim->bis_handles, (prim->num_bis*sizeof(hci_connection_handle_t)));
    }

    CsrSchedMessagePut(cmData->pendingIsocHandle, CSR_BT_CM_PRIM, (prim));
    cmData->pendingIsocHandle = cmData->isocHandle;
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmIsocBigTerminateSyncIndHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_BIG_TERMINATE_SYNC_IND_T *dmPrim = msg;
    CmIsocBigTerminateSyncInd *prim;

    if (cmData->isocHandle != CSR_SCHED_QID_INVALID)
    {
        prim = CsrPmemZalloc(sizeof(*prim));
        prim->type = CSR_BT_CM_ISOC_BIG_TERMINATE_SYNC_IND;
        prim->big_handle = dmPrim->big_handle;
        prim->resultCode = (CsrBtResultCode) dmPrim->status_or_reason;

        CsrSchedMessagePut(cmData->isocHandle, CSR_BT_CM_PRIM, (prim));
    }
}

void CsrBtCmDmBleBigInfoAdvReportIndHandler(cmInstanceData_t* cmData, void* msg)
{
    DM_HCI_ULP_BIGINFO_ADV_REPORT_IND_T* dmPrim = msg;
    CmBleBigInfoAdvReportInd* prim;

    if (cmData->isocHandle != CSR_SCHED_QID_INVALID)
    {
        prim = CsrPmemAlloc(sizeof(*prim));
        prim->type = CSR_BT_CM_BLE_BIGINFO_ADV_REPORT_IND;
        SynMemCpyS(&prim->bigParams, sizeof(CmBigParam), &dmPrim->big_params, sizeof(CmBigParam));
        prim->sduInterval = dmPrim->sdu_interval;
        prim->syncHandle = dmPrim->sync_handle;
        prim->maxSdu = dmPrim->max_sdu;
        prim->numBis = dmPrim->num_bis;
        prim->framing = dmPrim->framing;
        prim->encryption = dmPrim->encryption;

        CsrSchedMessagePut(cmData->isocHandle, CSR_BT_CM_PRIM, (prim));
    }
}

void CsrBtCmDmIsocConfigureCigTestCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_CONFIGURE_CIG_TEST_CFM_T *dmPrim = msg;
    CmIsocConfigureCigTestCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_ISOC_CONFIGURE_CIG_TEST_CFM;
    prim->cig_id = dmPrim->cig_id;
    prim->cis_count = dmPrim->cis_count;

    if(dmPrim->status == HCI_SUCCESS)
    {
        CsrUint8 i;

        prim->resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;

        for (i = 0; i < dmPrim->cis_count; i++)
            prim->cis_handles[i] = dmPrim->cis_handles[i];
    }
    else
    {
        prim->resultCode = (CsrBtResultCode) dmPrim->status;
    }

    CsrSchedMessagePut(cmData->pendingIsocHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmIsocCreateBigTestCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_CREATE_BIG_TEST_CFM_T *dmPrim = msg;
    CmIsocCreateBigTestCfm *prim;
    CsrBtResultCode   resultCode;

    if (dmPrim->status == HCI_SUCCESS)
    {
        resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
    }
    else
    {
        resultCode      = (CsrBtResultCode) dmPrim->status;
    }

    prim = CsrPmemZalloc(sizeof(*prim));
    prim->type = CSR_BT_CM_ISOC_CREATE_BIG_TEST_CFM;
    prim->big_sync_delay = dmPrim->big_sync_delay;
    SynMemCpyS(&prim->big_params, sizeof(CmBigParam), &dmPrim->big_params, sizeof(CmBigParam));
    prim->big_handle = dmPrim->big_handle;
    prim->num_bis = dmPrim->num_bis;
    prim->resultCode = resultCode;
    if(prim->num_bis && dmPrim->bis_handles)
    {
        prim->bis_handles = CsrPmemZalloc(prim->num_bis*sizeof(hci_connection_handle_t));
        SynMemCpyS(prim->bis_handles, (prim->num_bis*sizeof(hci_connection_handle_t)),
                   dmPrim->bis_handles, (prim->num_bis*sizeof(hci_connection_handle_t)));
    }

    CsrSchedMessagePut(cmData->pendingIsocHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CmDmIsocReadIsoLinkQualityCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ISOC_READ_ISO_LINK_QUALITY_CFM_T *dmPrim = msg;
    CmIsocReadIsoLinkQualityCfm *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CM_ISOC_READ_ISO_LINK_QUALITY_CFM;
    prim->handle = dmPrim->handle;
    prim->tx_unacked_packets = dmPrim->tx_unacked_packets;       
    prim->tx_flushed_packets = dmPrim->tx_flushed_packets;
    prim->tx_last_subevent_packets = dmPrim->tx_last_subevent_packets;
    prim->retransmitted_packets = dmPrim->retransmitted_packets;
    prim->crc_error_packets = dmPrim->crc_error_packets;
    prim->rx_unreceived_packets = dmPrim->rx_unreceived_packets;
    prim->duplicate_packets = dmPrim->duplicate_packets;
    
    if (dmPrim->status == HCI_SUCCESS)
    {
        prim->resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
        prim->resultSupplier  = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        prim->resultCode      = (CsrBtResultCode) dmPrim->status;
        prim->resultSupplier  = CSR_BT_SUPPLIER_HCI;
    }

    CsrBtCmPutMessage(cmData->pendingIsocHandle, prim);
    CsrBtCmDmLocalQueueHandler();
}

