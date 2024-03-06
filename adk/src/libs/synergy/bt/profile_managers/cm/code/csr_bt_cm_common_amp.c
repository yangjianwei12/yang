/******************************************************************************
 Copyright (c) 2010-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"

#ifdef CSR_AMP_ENABLE

#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_private_lib.h"

void CsrBtCmAmpMoveChannelCfmSend(cmInstanceData_t *inst,
                                  CsrBtConnId btConnId,
                                  CsrBtAmpController localControl,
                                  CsrBtDeviceAddr addr,
                                  CsrBtResultCode resultCode,
                                  CsrBtSupplier resultSupplier)
{
    CsrBtCmMoveChannelCfm *prim = (CsrBtCmMoveChannelCfm*)CsrPmemAlloc(sizeof(CsrBtCmMoveChannelCfm));

    prim->type = CSR_BT_CM_MOVE_CHANNEL_CFM;
    prim->btConnId = btConnId;
    prim->localControl = localControl;
    prim->deviceAddr = addr;
    prim->resultCode = resultCode;
    prim->resultSupplier = resultSupplier;

    CsrBtCmPutMessage(inst->ampmHandle, prim);
}

void CsrBtCmAmpMoveChannelCmpIndSend(cmInstanceData_t *inst,
                                     CsrBtConnId btConnId,
                                     CsrBtAmpController localControl,
                                     CsrBtDeviceAddr addr,
                                     CsrBtResultCode resultCode,
                                     CsrBtSupplier resultSupplier)
{
    CsrBtCmMoveChannelCmpInd *prim = (CsrBtCmMoveChannelCmpInd*)CsrPmemAlloc(sizeof(CsrBtCmMoveChannelCmpInd));

    prim->type = CSR_BT_CM_MOVE_CHANNEL_CMP_IND;
    prim->btConnId = btConnId;
    prim->localControl = localControl;
    prim->deviceAddr = addr;
    prim->resultCode = resultCode;
    prim->resultSupplier = resultSupplier;

    CsrBtCmPutMessage(inst->ampmHandle, prim);
}

void CsrBtCmAmpMoveChannelIndSend(cmInstanceData_t *inst,
                                  CsrBtConnId btConnId,
                                  CsrBtAmpController localControl,
                                  CsrBtDeviceAddr addr)
{
    CsrBtCmMoveChannelInd *prim = (CsrBtCmMoveChannelInd*) CsrPmemAlloc(sizeof(CsrBtCmMoveChannelInd));

    prim->type = CSR_BT_CM_MOVE_CHANNEL_IND;
    prim->btConnId = btConnId;
    prim->localControl = localControl;
    prim->deviceAddr = addr;

    CsrBtCmPutMessage(inst->ampmHandle, prim);
}

/* Downstream handler for move channel request */
void CsrBtCmAmpMoveChannelReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmMoveChannelReq *cmPrim = (CsrBtCmMoveChannelReq*)cmData->recvMsgP;

    if(cmData->ampmHandle == CSR_SCHED_QID_INVALID)
    {
        /* No AMPM registered - can not handle this request */
        return ;
    }

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    if(CSR_BT_CONN_ID_IS_L2CA(cmPrim->btConnId))
    {
        CsrBtCmL2caMoveChannelReqHandler(cmData);
    }
#endif
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    if(CSR_BT_CONN_ID_IS_RFC(cmPrim->btConnId))
    {
        CsrBtCmRfcAmpMoveChannelReqHandler(cmData);
    }
#endif
}

/* Downstream handler for move channel response */
void CsrBtCmAmpMoveChannelResHandler(cmInstanceData_t *cmData)
{
    CsrBtCmMoveChannelRes *cmPrim = (CsrBtCmMoveChannelRes*)cmData->recvMsgP;

    if(cmData->ampmHandle == CSR_SCHED_QID_INVALID)
    {
        /* No AMPM registered - can not handle this request */
        return ;
    }

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    if(CSR_BT_CONN_ID_IS_L2CA(cmPrim->btConnId))
    {
        CsrBtCmL2caMoveChannelResHandler(cmData);        
    }
#endif
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    if(CSR_BT_CONN_ID_IS_RFC(cmPrim->btConnId))
    {
        CsrBtCmRfcAmpMoveChannelResHandler(cmData);
    }
#endif
}

/* inform the owner of the connection that the connection was
   moved between BR/EDR and AMP */
void CsrBtCmAmpMoveIndProfiles(cmInstanceData_t *cmData,
                               CsrBtConnId btConnId)
{
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    if(btConnId & CSR_BT_CONN_ID_L2CAP_TECH_MASK)
    {
        cmL2caConnElement *l2capElm;
        for (l2capElm = CM_L2CA_GET_FIRST(cmData->l2caVar.connList);
             l2capElm;
             l2capElm = l2capElm->next)
        {
            /* Search through the l2ca list */
            if (l2capElm->cmL2caConnInst)
            {
                if(l2capElm->cmL2caConnInst->btConnId == btConnId)
                {
                    CsrBtCmL2caAmpMoveInd *prim = (CsrBtCmL2caAmpMoveInd*) CsrPmemAlloc(sizeof(CsrBtCmL2caAmpMoveInd));

                    prim->type = CSR_BT_CM_L2CA_AMP_MOVE_IND;
                    prim->btConnId = btConnId;
                    prim->localControl = l2capElm->cmL2caConnInst->controller;
                    prim->context = l2capElm->cmL2caConnInst->context;
    
                    CsrBtCmPutMessage(l2capElm->cmL2caConnInst->appHandle, prim);
                    return;
                }
            }
        }
    }
#endif
}

#endif /* #ifdef CSR_AMP_ENABLE */
