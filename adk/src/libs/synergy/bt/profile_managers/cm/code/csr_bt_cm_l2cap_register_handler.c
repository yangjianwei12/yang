/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE

#include "csr_synergy.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_util.h"

static void CsrBtCmL2caRegisterCfmSend(CsrSchedQid appHandle, psm_t psm, CsrUint16 modeMask,
                                       CsrBtResultCode resultCode, CsrBtSupplier resultSupplier,
                                       CsrUint16 context)
{
    CsrBtCmL2caRegisterCfm * prim   = (CsrBtCmL2caRegisterCfm *)CsrPmemAlloc(sizeof(CsrBtCmL2caRegisterCfm));
    prim->type                      = CSR_BT_CM_L2CA_REGISTER_CFM;
    prim->localPsm                  = psm;
    prim->mode_mask                 = modeMask;
    prim->resultCode                = resultCode;
    prim->resultSupplier            = resultSupplier;
    prim->context                   = context;
    CsrBtCmPutMessage(appHandle, prim);
}

void CsrBtCmL2caRegisterReqHandler(cmInstanceData_t *cmData)
{ /* Request to register a PSM channel in the L2CAP layer, so it can multiplex
     between protocols. */
    CsrBtCmL2caRegisterReq  * prim  = (CsrBtCmL2caRegisterReq *) cmData->recvMsgP;
    /* if connection handling from application is supported the context is used to store the application task. */
    CsrSchedQid phandle = (prim->optionsMask & CM_L2CA_REGISTER_OPTION_APP_CONNECTION_HANDLING) ?
                          prim->phandle : 0;

    cmData->smVar.appHandle = prim->phandle;
    cmData->smVar.arg.reg.context   = prim->context;

    /* Register a PSM to l2cap */
    L2CA_RegisterReq(prim->localPsm,
                     CSR_BT_CM_IFACEQUEUE,
                     prim->mode_mask,
                     prim->flags,
                     (context_t)phandle);
}

static void CmL2caRegisterFixedCidCfmSend(CsrSchedQid appHandle, CsrUint16 fixedCid,
                                       CsrBtResultCode resultCode, CsrBtSupplier resultSupplier,
                                       CsrUint16 context)
{
    CmL2caRegisterFixedCidCfm * prim   = (CmL2caRegisterFixedCidCfm *)CsrPmemAlloc(sizeof(CmL2caRegisterFixedCidCfm));
    prim->type                      = CM_L2CA_REGISTER_FIXED_CID_CFM;
    prim->fixedCid                  = fixedCid;
    prim->resultCode                = resultCode;
    prim->resultSupplier            = resultSupplier;
    prim->context                   = context;
    CsrBtCmPutMessage(appHandle, prim);
}

void CmL2caRegisterFixedCidReqHandler(cmInstanceData_t *cmData)
{
    CmL2caRegisterFixedCidReq  * prim  = (CmL2caRegisterFixedCidReq *) cmData->recvMsgP;

    cmData->smVar.appHandle = prim->phandle;
    cmData->smVar.arg.reg.context   = prim->context;

    /* Register Fixed CID to l2cap */
    L2CA_RegisterFixedCidReq(CSR_BT_CM_IFACEQUEUE,
                     prim->fixedCid,
                     &prim->config,
                     (context_t)prim->phandle);
}

#ifdef CSR_BT_INSTALL_CM_PRI_L2CA_UNREGISTER
static void cmL2caUnregisterCfmSend(CsrSchedQid appHandle,
                                    psm_t localPsm,
                                    CsrBtResultCode resultCode,
                                    CsrBtSupplier resultSupplier)
{
    CmL2caUnregisterCfm * prim   = (CmL2caUnregisterCfm *)CsrPmemAlloc(sizeof(*prim));

    prim->type                      = CM_L2CA_UNREGISTER_CFM;
    prim->localPsm                  = localPsm;
    prim->resultCode                = resultCode;
    prim->resultSupplier            = resultSupplier;

    CsrBtCmPutMessage(appHandle, prim);
}

static void cmL2caUnregisterMsgAdd(cmInstanceData_t *cmData, CsrSchedQid appHandle, CsrUint16 localPsm)
{
    cmPendingMsg_t *pendingMsg;

    CsrPCmnListAddLast(cmData->pendingMsgs, sizeof(*pendingMsg), pendingMsg);
    pendingMsg->type = CM_PENDING_MSG_L2CA_UNREGISTER_PARAMS;
    pendingMsg->arg.l2caUnregisterParams.appHandle = appHandle;
    pendingMsg->arg.l2caUnregisterParams.localPsm  = localPsm;
}

static CsrBool cmL2caUnregisterPendingMsgCompare(CsrCmnListElm_t *elem, void *data)
{
    cmPendingMsg_t *pendingMsg = (cmPendingMsg_t*) elem;
    cmL2caUnregisterParam *params = (cmL2caUnregisterParam*) data;

    if (pendingMsg->type == CM_PENDING_MSG_L2CA_UNREGISTER_PARAMS)
    {
        if (pendingMsg->arg.l2caUnregisterParams.localPsm == params->localPsm)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void CsrBtCmL2caUnRegisterReqHandler(cmInstanceData_t *cmData)
{ /* Request to unregister a dynamic PSM channel in the L2CAP layer                 */
    CsrBtCmL2caUnregisterReq  * prim = (CsrBtCmL2caUnregisterReq *) cmData->recvMsgP;

    cmL2caUnregisterMsgAdd(cmData, prim->phandle, prim->localPsm);
    L2CA_UnRegisterReq(prim->localPsm, CSR_BT_CM_IFACEQUEUE);
}

void CsrBtCmL2caUnRegisterCfmHandler(cmInstanceData_t *cmData)
{ /* Confirmation event to previous PSM register request.                           */
    cmPendingMsg_t *pendingMsg;
    cmL2caUnregisterParam l2caUnregisterParam;
    L2CA_UNREGISTER_CFM_T * prim = (L2CA_UNREGISTER_CFM_T *) cmData->recvMsgP;

    l2caUnregisterParam.localPsm = prim->psm_local;

    pendingMsg = (cmPendingMsg_t *) CsrCmnListSearch((CsrCmnList_t *) cmData->pendingMsgs,
                                                     cmL2caUnregisterPendingMsgCompare,
                                                     &l2caUnregisterParam);

    if (pendingMsg)
    {
        if ((L2CA_RESULT_T)prim->result == L2CA_RESULT_SUCCESS)
        {
            cmL2caUnregisterCfmSend(pendingMsg->arg.l2caUnregisterParams.appHandle,
                                    prim->psm_local,
                                    CSR_BT_RESULT_CODE_CM_SUCCESS,
                                    CSR_BT_SUPPLIER_CM);
        }
        else
        {
            cmL2caUnregisterCfmSend(pendingMsg->arg.l2caUnregisterParams.appHandle,
                                    prim->psm_local,
                                    (CsrBtResultCode) prim->result,
                                    CSR_BT_SUPPLIER_L2CAP_MISC);
        }
        CsrPCmnListRemove(cmData->pendingMsgs, (CsrCmnListElm_t* ) pendingMsg);
    }
}
#endif /* CSR_BT_INSTALL_CM_PRI_L2CA_UNREGISTER */

void CsrBtCmL2caRegisterCfmHandler(cmInstanceData_t *cmData)
{ /* Confirmation event to previous PSM register request. Send
     CSR_BT_CM_L2CA_REGISTER_CFM to the application and restore the local service manager
     queue                                                                          */
    L2CA_REGISTER_CFM_T * prim = (L2CA_REGISTER_CFM_T *) cmData->recvMsgP;

    if ((L2CA_RESULT_T)prim->result == L2CA_RESULT_SUCCESS)
    {
        CsrBtCmL2caRegisterCfmSend(cmData->smVar.appHandle,
                                   prim->psm_local, prim->mode_mask,
                                   CSR_BT_RESULT_CODE_CM_SUCCESS, CSR_BT_SUPPLIER_CM,
                                   cmData->smVar.arg.reg.context);
    }
    else
    {
        CsrBtCmL2caRegisterCfmSend(cmData->smVar.appHandle,
                                   prim->psm_local, prim->mode_mask,
                                   (CsrBtResultCode) prim->result, CSR_BT_SUPPLIER_L2CAP_MISC,
                                   cmData->smVar.arg.reg.context);
    }
    CsrBtCmServiceManagerLocalQueueHandler(cmData);
}

void CmL2caRegisterFixedCidCfmHandler(cmInstanceData_t *cmData)
{
    L2CA_REGISTER_FIXED_CID_CFM_T * prim = (L2CA_REGISTER_FIXED_CID_CFM_T*) cmData->recvMsgP;

    if ((L2CA_RESULT_T)prim->result == L2CA_RESULT_SUCCESS)
    {
        cmData->l2caSigAppHandle = cmData->smVar.appHandle;
        CmL2caRegisterFixedCidCfmSend(cmData->smVar.appHandle,
                                   prim->fixed_cid,
                                   CSR_BT_RESULT_CODE_CM_SUCCESS, CSR_BT_SUPPLIER_CM,
                                   cmData->smVar.arg.reg.context);
    }
    else
    {
        CmL2caRegisterFixedCidCfmSend(cmData->smVar.appHandle,
                                   prim->fixed_cid,
                                   (CsrBtResultCode) prim->result, CSR_BT_SUPPLIER_L2CAP_MISC,
                                   cmData->smVar.arg.reg.context);
    }
    CsrBtCmServiceManagerLocalQueueHandler(cmData);
}
#endif /* !EXCLUDE_CSR_BT_L2CA_MODULE */
