/******************************************************************************
 Copyright (c) 2008-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_l2cap.h"

void CsrBtCmWriteAutoFlushTimeout(cmInstanceData_t *cmData, CsrBtDeviceAddr *deviceAddr)
{
    CsrUint16 smallestFlushTo = returnL2capSmallestFlushTo(cmData, deviceAddr);
    aclTable *acl = returnAclTable(cmData, deviceAddr);
    CsrUint16 flushTo;

    if (acl != NULL && acl->flushTo != smallestFlushTo &&
        cmData->dmVar.lmpVersion >= CSR_BT_BLUETOOTH_VERSION_2P1)
    {
        acl->flushTo = smallestFlushTo;
        flushTo = acl->flushTo == L2CA_FLUSH_TO_DEFAULT ? 0 : MILLI_TO_BB_SLOTS(acl->flushTo);
        CmDmWriteAutoFlushTimeoutReqSend(CSR_BT_CM_IFACEQUEUE, acl->deviceAddr, flushTo);
    }
}

void CsrBtCmDmHciWriteAutoFlushTimeoutCompleteHandler(cmInstanceData_t * cmData)
{
    DM_HCI_WRITE_AUTO_FLUSH_TIMEOUT_CFM_T *dmPrim = (DM_HCI_WRITE_AUTO_FLUSH_TIMEOUT_CFM_T *)cmData->recvMsgP;

    /* For internally initiated request, there is no need to generate response. */
    if (cmData->dmVar.appHandle != CSR_BT_CM_IFACEQUEUE)
    {
        CmDmWriteAutoFlushTimeoutCfm *cfm = (CmDmWriteAutoFlushTimeoutCfm *)CsrPmemZalloc(sizeof(*cfm));

        cfm->type = CM_DM_WRITE_AUTO_FLUSH_TIMEOUT_CFM;
        if (dmPrim->status == HCI_SUCCESS)
        {
            cfm->resultCode     = CSR_BT_RESULT_CODE_CM_SUCCESS;
            cfm->resultSupplier = CSR_BT_SUPPLIER_CM;
        }
        else
        {
            cfm->resultCode     = (CsrBtResultCode) dmPrim->status;
            cfm->resultSupplier = CSR_BT_SUPPLIER_HCI;
        }
        CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    }
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmWriteAutoFlushTimeoutReqHandler(cmInstanceData_t * cmData)
{
    CsrBtCmDmWriteAutoFlushTimeoutReq * cmPrim;

    cmPrim = (CsrBtCmDmWriteAutoFlushTimeoutReq *) cmData->recvMsgP;

    cmData->dmVar.appHandle = cmPrim->appHandle;
    dm_hci_write_auto_flush_timeout(&cmPrim->deviceAddr, cmPrim->flushTo, NULL);
}
#endif /* !EXCLUDE_CSR_BT_L2CA_MODULE */

