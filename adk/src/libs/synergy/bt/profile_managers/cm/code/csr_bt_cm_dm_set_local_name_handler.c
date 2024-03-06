/******************************************************************************
 Copyright (c) 2009-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"

static void csrBtCmSetLocalNameCfmMsgSend(CsrSchedQid appHandle, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    /* Build and send CSR_BT_CM_SET_LOCAL_NAME_CFM to the application */
    CsrBtCmSetLocalNameCfm *prim = (CsrBtCmSetLocalNameCfm *)CsrPmemAlloc(sizeof(CsrBtCmSetLocalNameCfm));
    prim->type                   = CSR_BT_CM_SET_LOCAL_NAME_CFM;
    prim->resultCode             = resultCode;
    prim->resultSupplier         = resultSupplier;
    CsrBtCmPutMessage(appHandle, prim);
}

void CsrBtCmSetLocalNameReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSetLocalNameReq *cmPrim = (CsrBtCmSetLocalNameReq *) cmData->recvMsgP;

    /* Request the host controller to write the given name as it local device name */
    cmData->dmVar.appHandle = cmPrim->phandle;

    /* The primitive is returned, not sent. */
    dm_hci_change_local_name((CsrUint8 *) cmPrim->friendlyName, NULL);

#ifdef CSR_BT_INSTALL_CM_AUTO_EIR
    {
        localEirData_t *localEirData = cmData->dmVar.localEirData;

        /* Store the name for use with the local EIR */
        if (localEirData)
        {
            CsrPmemFree(localEirData->requestedName);
            localEirData->requestedName = (CsrUtf8String *) CsrStrDup((CsrCharString *) cmPrim->friendlyName);
        }
    }
#endif /* CSR_BT_INSTALL_CM_AUTO_EIR */

    CsrPmemFree(cmData->dmVar.localName);
    cmData->dmVar.localName = cmPrim->friendlyName;
    cmPrim->friendlyName = NULL;
}

void CsrBtCmDmHciChangeLocalNameCompleteHandler(cmInstanceData_t *cmData)
{
    /* This event is the confirmation that the host controller was
     * written (or tried to write) the local device name */
    DM_HCI_CHANGE_LOCAL_NAME_CFM_T * dmPrim = (DM_HCI_CHANGE_LOCAL_NAME_CFM_T *) cmData->recvMsgP;

    if (dmPrim->status == HCI_SUCCESS)
    {
        csrBtCmSetLocalNameCfmMsgSend(cmData->dmVar.appHandle, CSR_BT_RESULT_CODE_CM_SUCCESS, CSR_BT_SUPPLIER_CM);

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOCAL_NAME_CHANGE
        /* Propagate name change to subscribers */
        CsrBtCmPropgateEvent(cmData,
                             CsrBtCmPropagateLocalNameChangeEvent,
                             CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOCAL_NAME_CHANGE,
                             HCI_SUCCESS,
                             cmData->dmVar.localName,
                             NULL);
#endif
    }
    else
    {
        csrBtCmSetLocalNameCfmMsgSend(cmData->dmVar.appHandle, (CsrBtResultCode) dmPrim->status, CSR_BT_SUPPLIER_HCI);
    }

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1
#ifdef CSR_BT_INSTALL_CM_AUTO_EIR
    {
        localEirData_t *localEirData = cmData->dmVar.localEirData;

        if (dmPrim->status == HCI_SUCCESS)
        {
            if (localEirData)
            {
                /* Only update the local EIR if the name was successfully updated */
                CsrBtCmEirUpdateName(cmData);
            }
            else
            {
                /* Restore the DM-queue if EIR is not supported */
                CsrBtCmDmLocalQueueHandler();
            }
        }
        else
        {
            if (localEirData)
            {
                /* If not success and if EIR is supported, free requested name and restore queue */
                CsrPmemFree(localEirData->requestedName);
                localEirData->requestedName = NULL;
            }
            CsrBtCmDmLocalQueueHandler();
        }
    }
#else /* CSR_BT_INSTALL_CM_AUTO_EIR */
    CsrBtCmDmLocalQueueHandler();
#endif /* !CSR_BT_INSTALL_CM_AUTO_EIR */
#else
    CsrBtCmDmLocalQueueHandler();
#endif
}

