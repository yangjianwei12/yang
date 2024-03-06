/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
******************************************************************************/
#include "csr_synergy.h"
#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_obex_streams.h"
#include "csr_streams.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_obex_util.h"
#include "csr_log_text_2.h"


static void csrBtObexStreamsProcessData(void * inst, CmStreamDataIndHandler hndlr, Source source, CsrUint8 protocol)
{
    CsrBtCmDataInd *ind = (CsrBtCmDataInd *) CsrBtCmStreamsDataIndGet(source,
                                                                      protocol,
                                                                      CSR_BT_CM_CONTEXT_UNUSED);
    while (ind)
    {
        hndlr(inst, &ind);
        /* Note: the data pointer is automatically invalidated if the message is queued in the handler function */
        SynergyMessageFree(CSR_BT_CM_PRIM, ind);
        ind = NULL;
        ind = (CsrBtCmDataInd *) CsrBtCmStreamsDataIndGet(source,
                                                          protocol,
                                                          CSR_BT_CM_CONTEXT_UNUSED);
    }
}

void CsrBtObexMessageMoreSpaceHandler(void * inst, CmStreamDataCfmHandler hndlr, MessageMoreSpace *mms, CsrUint8 protocol)
{
    CsrBtCmDataCfm *cfm;
    cfm = CsrBtCmStreamDataCfmGet(mms->sink,
                                  protocol,
                                  CSR_BT_CM_CONTEXT_UNUSED);    

    if (cfm)
    {
        hndlr(inst, &cfm);
        SynergyMessageFree(CSR_BT_CM_PRIM, cfm);
        cfm = NULL;
    }

    /* MessageMoreSpace received from Synergy Service is a duplicate copy,
     * so CsrPmemFree can be used directly */
    CsrPmemFree(mms);
}

void CsrBtObexMessageMoreDataHandler(void * inst, CmStreamDataIndHandler hndlr, MessageMoreData *mmd, CsrUint8 protocol)
{
    Source source = mmd->source;

    csrBtObexStreamsProcessData(inst, hndlr, source, protocol);
    CsrPmemFree(mmd);
}


void CsrBtObexStreamsRegister(void * inst, CsrBtConnId btConnId, CsrUint8 protocol)
{
    CsrUint16 cid = CM_GET_UINT16ID_FROM_BTCONN_ID(btConnId);    

    if (CsrStreamsRegister(cid, protocol, ObexUtilGetAppHandle(inst)))
    {
        CsrStreamsSourceHandoverPolicyConfigure(cid,
                                                protocol,
                                                SOURCE_HANDOVER_ALLOW_WITHOUT_DATA);
    }
}

#endif /* CSR_STREAMS_ENABLE */
