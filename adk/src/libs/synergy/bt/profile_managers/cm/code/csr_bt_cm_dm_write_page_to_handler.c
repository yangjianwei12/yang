/******************************************************************************
 Copyright (c) 2008-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"

void CsrBtCmDmHciWritePageToCompleteHandler(cmInstanceData_t * cmData)
{
    DM_HCI_WRITE_PAGE_TIMEOUT_CFM_T * dmPrim;

    dmPrim = (DM_HCI_WRITE_PAGE_TIMEOUT_CFM_T *) cmData->recvMsgP;

    if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
    {
        CmInitSequenceHandler(cmData,
                              CM_INIT_SEQ_WRITE_PAGE_TIMEOUT_CFM,
                              dmPrim->status,
                              CSR_BT_SUPPLIER_HCI);
    }
#ifdef CSR_BT_INSTALL_CM_WRITE_PAGE_TO
    else
    { /* The request came from the application */
        CsrBtCmWritePageToCfm * prim = (CsrBtCmWritePageToCfm *)CsrPmemAlloc(sizeof(CsrBtCmWritePageToCfm));
        prim->type                   = CSR_BT_CM_WRITE_PAGE_TO_CFM;

        if (dmPrim->status == HCI_SUCCESS)
        {
            prim->resultCode         = CSR_BT_RESULT_CODE_CM_SUCCESS;
            prim->resultSupplier     = CSR_BT_SUPPLIER_CM;
        }
        else
        {
            prim->resultCode         = (CsrBtResultCode) dmPrim->status;
            prim->resultSupplier     = CSR_BT_SUPPLIER_HCI;
        }
        CsrBtCmPutMessage(cmData->dmVar.appHandle, prim);
        CsrBtCmDmLocalQueueHandler();
    }
#endif
}

#ifdef CSR_BT_INSTALL_CM_WRITE_PAGE_TO
void CsrBtCmDmWritePageToReqHandler(cmInstanceData_t * cmData)
{
    CsrBtCmWritePageToReq * cmPrim;

    cmPrim = (CsrBtCmWritePageToReq *) cmData->recvMsgP;

    cmData->dmVar.appHandle = cmPrim->appHandle;
    dm_hci_write_page_to(cmPrim->pageTimeout, NULL);
}
#endif
