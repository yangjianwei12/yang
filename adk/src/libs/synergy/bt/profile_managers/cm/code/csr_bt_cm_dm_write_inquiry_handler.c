/******************************************************************************
 Copyright (c) 2008-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"

/* Handle inquiry scan setting request */
void CsrBtCmWriteInquiryScanSettingsReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmWriteInquiryscanSettingsReq *prim;
    prim = (CsrBtCmWriteInquiryscanSettingsReq*)cmData->recvMsgP;

    /* Store new settings */
    cmData->dmVar.appHandle           = prim->appHandle;
    cmData->dmVar.inquiryscanInterval = prim->interval;
    cmData->dmVar.inquiryscanWindow   = prim->window;

    /* Send request to core stack */
    dm_hci_write_inquiryscan_activity(cmData->dmVar.inquiryscanInterval,
                                      cmData->dmVar.inquiryscanWindow,
                                      NULL);
}

#ifdef CSR_BT_INSTALL_CM_WRITE_INQUIRY_SCAN_TYPE
/* Handle inquiry scan type request */
void CsrBtCmWriteInquiryScanTypeReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmWriteInquiryscanTypeReq *prim;
    prim = (CsrBtCmWriteInquiryscanTypeReq*)cmData->recvMsgP;

    /* Store new settings */
    cmData->dmVar.appHandle       = prim->appHandle;
    cmData->dmVar.inquiryscanType = prim->scanType;

    /* Send request to core stack */
    dm_hci_write_inquiry_scan_type(prim->scanType,
                                   NULL);
}
#endif

/*************************************************************************************
CsrBtCmDmHciWriteInquiryScanTypeCompleteHandler
*************************************************************************************/
void CsrBtCmDmHciWriteInquiryScanTypeCompleteHandler(cmInstanceData_t * cmData)
{
    DM_HCI_WRITE_INQUIRY_SCAN_TYPE_CFM_T    *dmPrim;
    dmPrim = (DM_HCI_WRITE_INQUIRY_SCAN_TYPE_CFM_T *)cmData->recvMsgP;

    if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
    {
        /* We are currently in the CM initialization process, we must
         * continue setting up the chip */
        CmInitSequenceHandler(cmData,
                              CM_INIT_SEQ_WRITE_INQUIRY_SCAN_TYPE_CFM,
                              dmPrim->status,
                              CSR_BT_SUPPLIER_HCI);
    }
#ifdef CSR_BT_INSTALL_CM_WRITE_INQUIRY_SCAN_TYPE
    else
    {
        /* We are in normal CM operation mode, so handle the confirm from the chip,
         * tell the user app and finally restore the queue */
        CsrBtCmWriteInquiryscanTypeCfm *prim = CsrPmemAlloc(sizeof(CsrBtCmWriteInquiryscanTypeCfm));
        prim->type = CSR_BT_CM_WRITE_INQUIRYSCAN_TYPE_CFM;
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
CsrBtCmDmHciWriteInquiryScanModeCompleteHandler
*************************************************************************************/
void CsrBtCmDmHciWriteInquiryModeCompleteHandler(cmInstanceData_t * cmData)
{
    DM_HCI_WRITE_INQUIRY_MODE_CFM_T     *dmPrim = (DM_HCI_WRITE_INQUIRY_MODE_CFM_T *)cmData->recvMsgP;

    if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
    {
        if (dmPrim->status == HCI_SUCCESS &&
            cmData->dmVar.lmpVersion >= CSR_BT_BLUETOOTH_VERSION_2P1)
        {
            CsrBtCmEirInitData(cmData);
        }

        /* We are currently in CM initialization phase, continue with the sequence. */
        CmInitSequenceHandler(cmData,
                              CM_INIT_SEQ_WRITE_INQUIRY_MODE_CFM,
                              dmPrim->status,
                              CSR_BT_SUPPLIER_HCI);
    }
#ifdef INSTALL_CM_WRITE_INQUIRY_MODE
    else
    {
        CmDmWriteInquiryModeCfm     *cfm    = (CmDmWriteInquiryModeCfm *)CsrPmemZalloc(sizeof(*cfm));
        cfm->type = CM_DM_WRITE_INQUIRY_MODE_CFM;

        if(dmPrim->status == HCI_SUCCESS)
        {
            cfm->resultCode        = CSR_BT_RESULT_CODE_CM_SUCCESS;
            cfm->resultSupplier    = CSR_BT_SUPPLIER_CM;
        }
        else
        {
            cfm->resultCode        = dmPrim->status;
            cfm->resultSupplier    = CSR_BT_SUPPLIER_HCI;
        }
        CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
        CsrBtCmDmLocalQueueHandler();
    }
#endif /* INSTALL_CM_WRITE_INQUIRY_MODE */
}

/*************************************************************************************
 CsrBtCmDmHciWriteInquiryScanActivityCompleteHandler:
************************************************************************************/
void CsrBtCmDmHciWriteInquiryScanActivityCompleteHandler(cmInstanceData_t *cmData)
{
    /* This event is the confirmation from the host controller
     * following a DM_HCI_WRITE_INQUIRYSCAN_ACTIVITY_REQ */
    DM_HCI_WRITE_INQUIRYSCAN_ACTIVITY_CFM_T *dmPrim;
    dmPrim = (DM_HCI_WRITE_INQUIRYSCAN_ACTIVITY_CFM_T *)cmData->recvMsgP;

    if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
    {
        /* We are currently in the CM initialization process, we must
         * continue setting up the chip */
        CmInitSequenceHandler(cmData,
                              CM_INIT_SEQ_WRITE_INQUIRYSCAN_ACTIVITY_CFM,
                              dmPrim->status,
                              CSR_BT_SUPPLIER_HCI);
    }
    else
    {
        /* We are in normal CM operation mode, so handle the confirm from the chip,
         * tell the user app and finally restore the queue */
        if(cmData->dmVar.appHandle != CSR_BT_CM_IFACEQUEUE)
        {
            CsrBtCmWriteInquiryscanSettingsCfm *prim = CsrPmemAlloc(sizeof(CsrBtCmWriteInquiryscanSettingsCfm));
            prim->type = CSR_BT_CM_WRITE_INQUIRYSCAN_SETTINGS_CFM;

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
        }
        CsrBtCmDmLocalQueueHandler();

    }
}

#ifdef INSTALL_CM_WRITE_INQUIRY_MODE
void CmDmWriteInquiryModeReqHandler(cmInstanceData_t *cmData)
{
    CmDmWriteInquiryModeReq    *req = (CmDmWriteInquiryModeReq *)cmData->recvMsgP;
    cmData->dmVar.appHandle         = req->appHandle;
    dm_hci_write_inquiry_mode(req->mode, NULL);
}
#endif /* INSTALL_CM_WRITE_INQUIRY_MODE */

