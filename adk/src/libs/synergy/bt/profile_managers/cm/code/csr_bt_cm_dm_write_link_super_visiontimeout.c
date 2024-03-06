/******************************************************************************
 Copyright (c) 2009-2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"

#define CSR_BT_CM_FIRST_ENTRY (0)


static void csrBtCmWriteLinkSuperVisionTimeoutCfmMsgSend(CsrSchedQid phandle, CsrBtResultCode resultCode,
                                           CsrBtSupplier resultSupplier, CsrBtDeviceAddr deviceAddr)
{
    CsrBtCmWriteLinkSupervTimeoutCfm            *prim;

    prim                    = (CsrBtCmWriteLinkSupervTimeoutCfm *)CsrPmemAlloc(sizeof(CsrBtCmWriteLinkSupervTimeoutCfm));
    prim->type              = CSR_BT_CM_WRITE_LINK_SUPERV_TIMEOUT_CFM;
    prim->resultCode        = resultCode;
    prim->resultSupplier    = resultSupplier;
    prim->deviceAddr        = deviceAddr;
    CsrBtCmPutMessage(phandle, prim);
}

#if defined(CSR_BT_INSTALL_CM_WRITE_LINK_SUPERVISION_TIMEOUT) || (CSR_BT_DEFAULT_LINK_SUPERVISION_TIMEOUT != CSR_BT_HCI_DEFAULT_LSTO)
void CsrBtCmWriteDmLinkSuperVisionTimeoutHandler(cmInstanceData_t   *cmData,
                                                 CsrSchedQid             phandle,
                                                 CsrUint16          timeout,
                                                 CsrBtDeviceAddr    deviceAddr)
{
    cmPendingMsg_t *pendingMsg;

    if (!CSR_BT_CM_PENDING_MSG_FIND_TYPE((CsrCmnList_t *) cmData->pendingMsgs,
                                         CSR_BT_CM_PENDING_MSG_LSTO_PARAMS))
    { /* No DM_HCI_WRITE_LINK_SUPERV_TIMEOUT is pending. */
        dm_hci_write_link_superv_timeout(&deviceAddr, timeout, NULL);
    }

    CsrPCmnListAddLast(cmData->pendingMsgs, sizeof(*pendingMsg), pendingMsg);

    pendingMsg->type = CSR_BT_CM_PENDING_MSG_LSTO_PARAMS;
    pendingMsg->arg.lstoParams.appHandle = phandle;
    pendingMsg->arg.lstoParams.timeout = timeout;
    pendingMsg->arg.lstoParams.deviceAddr = deviceAddr;
}
#endif

static CsrBool duplicateCmLstoParamsCommand(CsrCmnListElm_t *elem, void *data)
{
    cmPendingMsg_t *pendingMsg = (cmPendingMsg_t *) elem;
    aclTable *aclConnectionElement = (aclTable *) data;

    if (pendingMsg->type == CSR_BT_CM_PENDING_MSG_LSTO_PARAMS)
    {
        cmLstoParms_t *lsto = (cmLstoParms_t *) &pendingMsg->arg.lstoParams;

        if (lsto->appHandle == CSR_BT_CM_IFACEQUEUE &&
            lsto->timeout == aclConnectionElement->lsto &&
            CsrBtBdAddrEq(&(lsto->deviceAddr), &(aclConnectionElement->deviceAddr)))
        {
            return TRUE;
        }
    }

    return FALSE;
}

void CsrBtCmDmHciWriteLinkSuperVisionTimeoutCompleteHandler(cmInstanceData_t *cmData)
{ /* This event is the confirmation that the host controller was write
     (or try to) the new supervision timeout value to the given device address.
     Build and send CM_LINK_WRITE_SUPERV_TIMEOUT_CFM to the application */
    cmPendingMsg_t *pendingMsg;
    DM_HCI_WRITE_LINK_SUPERV_TIMEOUT_CFM_T *dmPrim = (DM_HCI_WRITE_LINK_SUPERV_TIMEOUT_CFM_T *) cmData->recvMsgP;

    pendingMsg = (cmPendingMsg_t *) CSR_BT_CM_PENDING_MSG_FIND_TYPE((CsrCmnList_t *) cmData->pendingMsgs,
                                                                    CSR_BT_CM_PENDING_MSG_LSTO_PARAMS);

    if (pendingMsg)
    {
        cmLstoParms_t *lsto = (cmLstoParms_t *) &pendingMsg->arg.lstoParams;
        aclTable *aclConnectionElement = NULL;

        if (dmPrim->status == HCI_SUCCESS)
        {
            returnAclConnectionElement(cmData,
                                       dmPrim->bd_addr,
                                       &aclConnectionElement);

            if (aclConnectionElement)
            {
                aclConnectionElement->lsto = lsto->timeout;
            }
        }

        if (lsto->appHandle != CSR_BT_CM_IFACEQUEUE)
        {
            aclConnectionElement = NULL;
            if (dmPrim->status == HCI_SUCCESS)
            {
                csrBtCmWriteLinkSuperVisionTimeoutCfmMsgSend(lsto->appHandle,
                                                             CSR_BT_RESULT_CODE_CM_SUCCESS,
                                                             CSR_BT_SUPPLIER_CM,
                                                             dmPrim->bd_addr);
            }
            else
            {
                csrBtCmWriteLinkSuperVisionTimeoutCfmMsgSend(lsto->appHandle,
                                                             (CsrBtResultCode) dmPrim->status,
                                                             CSR_BT_SUPPLIER_HCI,
                                                             dmPrim->bd_addr);
            }
            CsrPCmnListRemove(cmData->pendingMsgs,
                              (CsrCmnListElm_t *) pendingMsg);
        }

        if (aclConnectionElement)
        { /* Remove duplicate commands initiated by the CM ONLY */
            CsrPCmnListIterateAllowRemove(cmData->pendingMsgs,
                                          duplicateCmLstoParamsCommand,
                                          aclConnectionElement);
        }

        /* Send the next DM_HCI_WRITE_LINK_SUPERV_TIMEOUT */
        pendingMsg = (cmPendingMsg_t *) CSR_BT_CM_PENDING_MSG_FIND_TYPE((CsrCmnList_t *) cmData->pendingMsgs,
                                                                        CSR_BT_CM_PENDING_MSG_LSTO_PARAMS);
        if (pendingMsg)
        {
            lsto = (cmLstoParms_t *) &pendingMsg->arg.lstoParams;
            dm_hci_write_link_superv_timeout(&lsto->deviceAddr,
                                             lsto->timeout,
                                             NULL);
        }
        else
        {
            /* No more messages to send */
        }
    }
}

#ifdef CSR_BT_INSTALL_CM_WRITE_LINK_SUPERVISION_TIMEOUT
void CsrBtCmWriteLinkSuperVTimeoutReqHandler(cmInstanceData_t *cmData)
{ /* This event indicates that the application desired to change link supervision
     timeout setting */
    aclTable           *aclConnectionElement;
    CsrBtCmWriteLinkSupervTimeoutReq *cmPrim = (CsrBtCmWriteLinkSupervTimeoutReq *) cmData->recvMsgP;
    returnAclConnectionElement(cmData, cmPrim->deviceAddr, &aclConnectionElement);

    if (aclConnectionElement)
    {
        CsrBtCmWriteDmLinkSuperVisionTimeoutHandler(cmData,
                                                    cmPrim->phandle,
                                                    cmPrim->timeout,
                                                    cmPrim->deviceAddr);
    }
    else
    {
        csrBtCmWriteLinkSuperVisionTimeoutCfmMsgSend(cmPrim->phandle,
                                                     CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER,
                                                     CSR_BT_SUPPLIER_CM,
                                                     cmPrim->deviceAddr);
    }
}
#endif

void CsrBtCmDmHciLinkSupervisionTimeoutIndHandler(cmInstanceData_t *cmData)
{ /* This event indicates that the application desired to change link supervision
     timeout setting */
    DM_HCI_LINK_SUPERV_TIMEOUT_IND_T *dmPrim;
    aclTable *aclConnectionElement;

    dmPrim    = (DM_HCI_LINK_SUPERV_TIMEOUT_IND_T *) cmData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LSTO_CHANGE
    CsrBtCmPropgateEvent(cmData,
                         CsrBtCmPropgateLstoChangeEvents,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LSTO_CHANGE,
                         HCI_SUCCESS,
                         dmPrim,
                         NULL);
#endif

    returnAclConnectionElement(cmData, dmPrim->bd_addr, &aclConnectionElement);

    if (aclConnectionElement)
    {
        aclConnectionElement->lsto = dmPrim->timeout;
    }
}

