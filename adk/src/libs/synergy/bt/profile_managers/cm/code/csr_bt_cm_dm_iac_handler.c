/******************************************************************************
 Copyright (c) 2009-2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"
#ifdef CSR_BT_INSTALL_CM_PRI_IAC

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_prim.h"
#include "dm_prim.h"
#include "dmlib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_cm_util.h"

#ifdef CSR_BT_INSTALL_CM_PRI_IAC_READ
static void sendCmReadIacCfm(CsrSchedQid appHandle,
                             CsrUint24 iac,
                             CsrBtResultCode resultCode,
                             CsrBtSupplier resultSupplier)
{
    CsrBtCmReadIacCfm *msg = (CsrBtCmReadIacCfm*) CsrPmemAlloc(sizeof(*msg));
    msg->type = CSR_BT_CM_READ_IAC_CFM;
    msg->iac = iac;
    msg->resultCode = resultCode;
    msg->resultSupplier = resultSupplier;
    CsrBtCmPutMessage(appHandle, msg);
}

void CsrBtCmReadIacReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmReadIacReq *cmPrim = (CsrBtCmReadIacReq *) cmData->recvMsgP;

    cmData->dmVar.appHandle = cmPrim->appHandle;
    dm_hci_read_current_iac_lap(NULL);
}

void CsrBtCmDmHciReadIacCompleteHandler(cmInstanceData_t *cmData)
{
    DM_HCI_READ_CURRENT_IAC_LAP_CFM_T *dmPrim = (DM_HCI_READ_CURRENT_IAC_LAP_CFM_T *) cmData->recvMsgP;

    if (dmPrim->status == HCI_SUCCESS)
    {
        CsrUint24 iac = 0;

        if (dmPrim->num_current_iac == 1)
        {
            iac = dmPrim->iac_lap[0][0];
        }
        else if (dmPrim->num_current_iac)
        {
            if (dmPrim->iac_lap[0][0] == HCI_INQ_CODE_GIAC)
            {
                iac = dmPrim->iac_lap[0][1];
            }
            else
            {
                iac = dmPrim->iac_lap[0][1];
            }
        }

        sendCmReadIacCfm(cmData->dmVar.appHandle,
                         iac,
                         CSR_BT_RESULT_CODE_CM_SUCCESS,
                         CSR_BT_SUPPLIER_CM);
    }
    else
    {
        sendCmReadIacCfm(cmData->dmVar.appHandle,
                         0,
                         (CsrBtResultCode) dmPrim->status,
                         CSR_BT_SUPPLIER_HCI);
    }
    CsrBtCmDmLocalQueueHandler();
}
#endif /* CSR_BT_INSTALL_CM_PRI_IAC_READ */

static void sendCmWriteIacInd(CsrSchedQid appHandle,
                              CsrBtResultCode resultCode,
                              CsrBtSupplier resultSupplier)
{
    CsrBtCmWriteIacInd *msg = (CsrBtCmWriteIacInd*) CsrPmemAlloc(sizeof(*msg));
    msg->type = CSR_BT_CM_WRITE_IAC_IND;
    msg->resultCode = resultCode;
    msg->resultSupplier = resultSupplier;
    CsrBtCmPutMessage(appHandle, msg);
}

void CsrBtCmWriteIacReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmWriteIacReq *cmPrim = (CsrBtCmWriteIacReq *) cmData->recvMsgP;

    if (cmPrim->iac >= HCI_IAC_LAP_MIN &&
        cmPrim->iac <= HCI_IAC_LAP_MAX)
    {
        uint24_t iac_list[2];
        CsrUint8 num_iac;

        cmData->dmVar.appHandle = cmPrim->appHandle;

        iac_list[0] = HCI_INQ_CODE_GIAC;

        if (cmPrim->iac == HCI_INQ_CODE_GIAC)
        {
            num_iac = 1;
        }
        else
        {
            num_iac = 2;
            iac_list[1] = cmPrim->iac;
        }

        dm_hci_write_current_iac_lap(num_iac, (uint24_t *) &iac_list, NULL);
    }
    else
    {
        sendCmWriteIacInd(cmPrim->appHandle,
                          CSR_BT_RESULT_CODE_CM_UNACCEPTABLE_PARAMETER,
                          CSR_BT_SUPPLIER_CM);
        CsrBtCmDmLocalQueueHandler();
    }
}

void CsrBtCmDmHciWriteIacCompleteHandler(cmInstanceData_t *cmData)
{
    CsrBtResultCode result;
    CsrBtSupplier supplier;
    DM_HCI_WRITE_CURRENT_IAC_LAP_CFM_T *dmPrim = (DM_HCI_WRITE_CURRENT_IAC_LAP_CFM_T *) cmData->recvMsgP;

    if (dmPrim->status == HCI_SUCCESS)
    {
        result = CSR_BT_RESULT_CODE_CM_SUCCESS;
        supplier = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        result = (CsrBtResultCode) dmPrim->status;
        supplier = CSR_BT_SUPPLIER_HCI;
    }

    sendCmWriteIacInd(cmData->dmVar.appHandle,
                      result,
                      supplier);
    CsrBtCmDmLocalQueueHandler();
}

#endif /* CSR_BT_INSTALL_CM_PRI_IAC */

