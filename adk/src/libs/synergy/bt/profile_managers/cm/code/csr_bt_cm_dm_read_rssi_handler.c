/******************************************************************************
 Copyright (c) 2008-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"
#ifdef CSR_BT_INSTALL_CM_READ_RSSI
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"

void CsrBtCmDmHciReadRssiCompleteHandler(cmInstanceData_t *cmData)
{
    DM_HCI_READ_RSSI_CFM_T * dmPrim = (DM_HCI_READ_RSSI_CFM_T *) cmData->recvMsgP;
    CsrBtCmReadRssiCfm     * cmPrim = (CsrBtCmReadRssiCfm *)CsrPmemAlloc(sizeof(CsrBtCmReadRssiCfm));

    cmPrim->type          = CSR_BT_CM_READ_RSSI_CFM;
    cmPrim->deviceAddr    = dmPrim->tp_addrt.addrt.addr;
    cmPrim->rssi          = dmPrim->rssi;
    cmPrim->addressType   = dmPrim->tp_addrt.addrt.type;
    cmPrim->transportType = dmPrim->tp_addrt.tp_type;

    if (dmPrim->status == HCI_SUCCESS)
    {
        cmPrim->resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
        cmPrim->resultSupplier  = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        cmPrim->resultCode      = (CsrBtResultCode) dmPrim->status;
        cmPrim->resultSupplier  = CSR_BT_SUPPLIER_HCI;
    }

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cmPrim);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmReadRssiReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmReadRssiReq * cmPrim = (CsrBtCmReadRssiReq *) cmData->recvMsgP;
    TP_BD_ADDR_T tpAddr;

    cmData->dmVar.appHandle = cmPrim->appHandle;
    tpAddr.addrt.addr       = cmPrim->deviceAddr;
    tpAddr.addrt.type       = cmPrim->addressType;
    tpAddr.tp_type          = cmPrim->transportType;

    dm_hci_read_rssi(&tpAddr, NULL);
}
#endif
