/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_synergy.h"

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_av_streams.h"

static void csrBtAvStreamsProcessData(av_instData_t *instData, Source source)
{
    void *msg = instData->recvMsgP; /* Save the original message */
    CsrBtCmL2caDataInd *ind = CsrBtCmStreamsDataIndGet(source,
                                                       L2CAP_ID,
                                                       CSR_BT_CM_CONTEXT_UNUSED);

    while (ind)
    {
        instData->recvMsgP = ind;
        CsrBtAvCmL2caDataIndHandler(instData);

        /* CsrPmemFree can be used to deallocate dummy messages */
        CsrPmemFree(instData->recvMsgP);
        instData->recvMsgP = NULL;

        ind = CsrBtCmStreamsDataIndGet(source,
                                       L2CAP_ID,
                                       CSR_BT_CM_CONTEXT_UNUSED);
    }

    instData->recvMsgP = msg; /* Restore the original message */
}

void CsrBtAvMessageMoreSpaceHandler(av_instData_t *instData)
{
    CsrBtCmL2caDataCfm *cfm;
    MessageMoreSpace *mms = (MessageMoreSpace *) instData->recvMsgP;

    cfm = CsrBtCmStreamDataCfmGet(mms->sink,
                                  L2CAP_ID,
                                  CSR_BT_CM_CONTEXT_UNUSED);

    if (cfm)
    {
        instData->recvMsgP = cfm;
        CsrBtAvCmL2caDataCfmHandler(instData);
        CsrPmemFree(instData->recvMsgP);
    }

    /* MessageMoreSpace received from Synergy Service is a duplicate copy,
     * so CsrPmemFree can be used directly */
    CsrPmemFree(mms);
    instData->recvMsgP = NULL;

}

void CsrBtAvMessageMoreDataHandler(av_instData_t *instData)
{
    MessageMoreData *mmd = instData->recvMsgP;
    csrBtAvStreamsProcessData(instData, mmd->source);

    /* MessageMoreData received from Synergy Service is a duplicate copy,
     * so CsrPmemFree can be used directly */
    CsrPmemFree(instData->recvMsgP);
    instData->recvMsgP = NULL;
}

void CsrBtAvStreamsRegister(av_instData_t *instData, CsrBtConnId btConnId)
{
    CsrUint16 cid = CM_GET_UINT16ID_FROM_BTCONN_ID(btConnId);
    Source source = StreamSourceFromSink(StreamL2capSink(cid));

    if (CsrStreamsRegister(cid, L2CAP_ID, CSR_BT_AV_IFACEQUEUE))
    {
        CsrStreamsSourceHandoverPolicyConfigure(cid,
                                                L2CAP_ID,
                                                SOURCE_HANDOVER_ALLOW_WITHOUT_DATA);

        /* Ensure that early data (if any) is consumed */
        csrBtAvStreamsProcessData(instData, source);
    }
}

#endif /* CSR_STREAMS_ENABLE */

