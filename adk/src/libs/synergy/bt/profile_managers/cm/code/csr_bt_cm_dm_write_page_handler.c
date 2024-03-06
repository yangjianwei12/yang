/******************************************************************************
 Copyright (c) 2008-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"

#ifdef CSR_BT_INSTALL_CM_WRITE_PAGE_SCAN
/* Handle page scan setting request */
void CsrBtCmWritePageScanSettingsReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmWritePagescanSettingsReq *prim;
    prim = (CsrBtCmWritePagescanSettingsReq*)cmData->recvMsgP;

    /* Store new settings */
    cmData->dmVar.appHandle        = prim->appHandle;
    cmData->dmVar.pagescanInterval = prim->interval;
    cmData->dmVar.pagescanWindow   = prim->window;

    /* Send request to core stack */
    dm_hci_write_pagescan_activity(prim->interval,
                                   prim->window,
                                   NULL);
}

/* Handle page scan type request */
void CsrBtCmWritePageScanTypeReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmWritePagescanTypeReq *prim;
    prim = (CsrBtCmWritePagescanTypeReq*)cmData->recvMsgP;

    /* Store new settings */
    cmData->dmVar.appHandle    = prim->appHandle;
    cmData->dmVar.pagescanType = prim->scanType;

    /* Send request to core stack */
    dm_hci_write_page_scan_type(prim->scanType,
                                NULL);
}
#endif
/*************************************************************************************
 CsrBtCmDmHciWritePageScanActivityCompleteHandler:
************************************************************************************/
void CsrBtCmDmHciWritePageScanActivityCompleteHandler(cmInstanceData_t *cmData)
{
    /* This event is the confirmation from the host controller
     * following a DM_HCI_WRITE_PAGESCAN_ACTIVITY_REQ */
    DM_HCI_WRITE_PAGESCAN_ACTIVITY_CFM_T *dmPrim;
    dmPrim = (DM_HCI_WRITE_PAGESCAN_ACTIVITY_CFM_T *)cmData->recvMsgP;

    if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
    {
        /* We are currently in the CM initialization process, we must
         * continue setting up the chip */
        CmInitSequenceHandler(cmData,
                              CM_INIT_SEQ_WRITE_PAGESCAN_ACTIVITY_CFM,
                              dmPrim->status,
                              CSR_BT_SUPPLIER_HCI);
    }
#ifdef CSR_BT_INSTALL_CM_WRITE_PAGE_SCAN
    else
    {
        /* We are in normal CM operation mode, so handle the confirm from the chip,
         * tell the user app and finally restore the queue */
        CsrBtCmWritePagescanSettingsCfm *prim = CsrPmemAlloc(sizeof(CsrBtCmWritePagescanSettingsCfm));
        prim->type = CSR_BT_CM_WRITE_PAGESCAN_SETTINGS_CFM;

        if (dmPrim->status == HCI_SUCCESS)
        {
            prim->resultCode     = CSR_BT_RESULT_CODE_CM_SUCCESS;
            prim->resultSupplier = CSR_BT_SUPPLIER_CM;
        }
        else
        {
            prim->resultCode     = (CsrBtResultCode) dmPrim->status;
            prim->resultSupplier = CSR_BT_SUPPLIER_HCI;
        }
        CsrBtCmPutMessage(cmData->dmVar.appHandle, prim);
        CsrBtCmDmLocalQueueHandler();
    }
#endif
}

/*************************************************************************************
CsrBtCmDmHciWriteInquiryScanTypeCompleteHandler
*************************************************************************************/
void CsrBtCmDmHciWritePageScanTypeCompleteHandler(cmInstanceData_t * cmData)
{
    DM_HCI_WRITE_PAGE_SCAN_TYPE_CFM_T *dmPrim;
    dmPrim = (DM_HCI_WRITE_PAGE_SCAN_TYPE_CFM_T *)cmData->recvMsgP;

    if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
    {
        /* We are currently in CM initialization phase, continue with the sequence. */
        CmInitSequenceHandler(cmData,
                              CM_INIT_SEQ_WRITE_PAGE_SCAN_TYPE_CFM,
                              dmPrim->status,
                              CSR_BT_SUPPLIER_HCI);
    }
#ifdef CSR_BT_INSTALL_CM_WRITE_PAGE_SCAN
    else
    {
        /* We are in normal CM operation mode, so handle the confirm from the chip,
         * tell the user app and finally restore the queue */
        CsrBtCmWritePagescanTypeCfm *prim = CsrPmemAlloc(sizeof(CsrBtCmWritePagescanTypeCfm));
        prim->type = CSR_BT_CM_WRITE_PAGESCAN_TYPE_CFM;

        if (dmPrim->status == HCI_SUCCESS)
        {
            prim->resultCode     = CSR_BT_RESULT_CODE_CM_SUCCESS;
            prim->resultSupplier = CSR_BT_SUPPLIER_CM;
        }
        else
        {
            prim->resultCode     = (CsrBtResultCode) dmPrim->status;
            prim->resultSupplier = CSR_BT_SUPPLIER_HCI;
        }
        CsrBtCmPutMessage(cmData->dmVar.appHandle, prim);
        CsrBtCmDmLocalQueueHandler();
    }
#endif
}

