/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #4 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_profiles.h"
#include "csr_msg_transport.h"
#include "csr_bt_pac_lib.h"
#include "csr_bt_pac_handler.h"

void CsrBtPacMsgTransport(CsrSchedQid instanceId, void* msg)
{
    CsrMsgTransport(instanceId, CSR_BT_PAC_PRIM, msg);
}

#ifdef CSR_TARGET_PRODUCT_VM
CsrBool PacRegisterAppHandle(CsrSchedQid pacInstanceId, CsrSchedQid appHandle)
{
    return PacSetAppHandle(pacInstanceId, appHandle);
}
#endif

void PacPullPbReqSend_struct(CsrSchedQid        instanceId,
                             CsrUcs2String     *ucs2name,
                             CsrBtPacSrcType    src,
                             CsrUint8          *filter,
                             CsrBtPacFormatType format,
                             CsrUint16          maxLstCnt,
                             CsrUint16          listStartOffset,
                             CsrUint8           resetNewMissedCalls,
                             CsrUint8          *vCardSelector,
                             CsrUint8           vCardSelectorOperator,
                             CsrBool            srmpOn)
{
    CsrBtPacPullPbReq *msg = CsrPmemZalloc(sizeof(*msg));

    msg->type = CSR_BT_PAC_PULL_PB_REQ;
    msg->ucs2name = ucs2name;
    msg->src = src;
    if (filter)
    {
        CsrMemCpy(msg->filter, filter, sizeof(msg->filter));
    }
    msg->format = format;
    msg->maxListCnt = maxLstCnt;
    msg->listStartOffset = listStartOffset;
    msg->srmpOn = srmpOn;
    msg->resetNewMissedCalls = resetNewMissedCalls;
    if (vCardSelector)
    {
        CsrMemCpy(msg->vCardSelector, vCardSelector, sizeof(msg->vCardSelector));
        msg->vCardSelectorOperator = vCardSelectorOperator;
    }

    CsrBtPacMsgTransport(instanceId, msg);
}

void PacPullVcardListReqSend_struct(CsrSchedQid       instanceId,
                                    CsrUcs2String    *ucs2name,
                                    CsrBtPacOrderType order,
                                    CsrUint8         *searchVal,
                                    CsrBtPacSearchAtt searchAtt,
                                    CsrUint16         maxListCnt,
                                    CsrUint16         listStartOffset,
                                    CsrUint8          resetNewMissedCalls,
                                    CsrUint8         *vCardSelector,
                                    CsrUint8          vCardSelectorOperator,
                                    CsrBool           srmpOn)
{
    CsrBtPacPullVcardListReq *msg = CsrPmemZalloc(sizeof(CsrBtPacPullVcardListReq));
    msg->type = CSR_BT_PAC_PULL_VCARD_LIST_REQ;
    msg->ucs2name = ucs2name;
    msg->order = order;
    msg->searchVal = searchVal;

    if (searchVal)
    {
        msg->searchValLen = (CsrUint16) (CsrStrLen((char*) searchVal) + 1);
    }

    msg->searchAtt = searchAtt;
    msg->maxListCnt = maxListCnt;
    msg->listStartOffset = listStartOffset;

    msg->resetNewMissedCalls = resetNewMissedCalls;
    if(vCardSelector != NULL)
    {
        CsrMemCpy(msg->vCardSelector,
                  vCardSelector,
                  sizeof(msg->vCardSelector));
        msg->vCardSelectorOperator = vCardSelectorOperator;
    }
    msg->srmpOn = srmpOn;

    CsrBtPacMsgTransport(instanceId, msg);
}

