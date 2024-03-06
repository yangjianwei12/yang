/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_tasks.h"
#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_hidd_stream.h"
#include "csr_bt_cm_private_lib.h"


static void csrBtHiddStreamsProcessData(HiddInstanceDataType *instData,
                                        Source source)
{
    void *msg = instData->pRecvMsg; /* Save the original message */
    CsrBtCmL2caDataInd *ind = CsrBtCmStreamsDataIndGet(source,
                                                       L2CAP_ID,
                                                       CSR_BT_CM_CONTEXT_UNUSED);

    while (ind)
    {
        instData->pRecvMsg = ind;
        CsrBtHiddCmDataHandler(instData);

        /* CsrPmemFree can be used to deallocate dummy messages */
        CsrPmemFree(instData->pRecvMsg);
        instData->pRecvMsg = NULL;

        ind = CsrBtCmStreamsDataIndGet(source,
                                       L2CAP_ID,
                                       CSR_BT_CM_CONTEXT_UNUSED);
    }

    instData->pRecvMsg = msg; /* Restore the original message */
}

void CsrBtHiddMessageMoreSpaceHandler(HiddInstanceDataType *instData)
{
    CsrBtCmL2caDataCfm *cfm;
    MessageMoreSpace *mms = (MessageMoreSpace *) instData->pRecvMsg;

    cfm = CsrBtCmStreamDataCfmGet(mms->sink,
                                  L2CAP_ID,
                                  CSR_BT_CM_CONTEXT_UNUSED);

    if (cfm)
    {
        instData->pRecvMsg = cfm;
        CsrBtHiddCmDataHandler(instData);
        CsrPmemFree(instData->pRecvMsg);
    }

    /* MessageMoreSpace received from Synergy Service is a duplicate copy,
     * so CsrPmemFree can be used directly */
    CsrPmemFree(mms);
    instData->pRecvMsg = NULL;

}

void CsrBtHiddMessageMoreDataHandler(HiddInstanceDataType *instData)
{
    MessageMoreData *mmd = instData->pRecvMsg;
    csrBtHiddStreamsProcessData(instData, mmd->source);

    /* MessageMoreData received from Synergy Service is a duplicate copy,
     * so CsrPmemFree can be used directly */
    CsrPmemFree(instData->pRecvMsg);
    instData->pRecvMsg = NULL;
}

void CsrBtHiddStreamsRegister(HiddInstanceDataType *instData, CsrBtConnId btConnId)
{
    CsrUint16 cid = CM_GET_UINT16ID_FROM_BTCONN_ID(btConnId);
    Source source = StreamSourceFromSink(StreamL2capSink(cid));

    if (CsrStreamsRegister(cid, L2CAP_ID, CSR_BT_HIDD_IFACEQUEUE))
    {
        CsrStreamsSourceHandoverPolicyConfigure(cid,
                                                L2CAP_ID,
                                                SOURCE_HANDOVER_ALLOW_WITHOUT_DATA);

        /* Ensure that early data (if any) is consumed */
        csrBtHiddStreamsProcessData(instData, source);
    }
}

#endif /* CSR_STREAMS_ENABLE */
