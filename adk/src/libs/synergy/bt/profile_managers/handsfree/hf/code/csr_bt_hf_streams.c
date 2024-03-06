/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_synergy.h"

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_hf_streams.h"
#include "csr_bt_hf_main_sef.h"

static void csrBtHfStreamsProcessData(HfMainInstanceData_t *instData,
                                      Source source)
{
    CsrBtCmDataInd *ind = (CsrBtCmDataInd *) CsrBtCmStreamsDataIndGet(source,
                                                                      RFCOMM_ID,
                                                                      CSR_BT_CM_CONTEXT_UNUSED);
    void *msg = instData->recvMsgP; /* Save the original message */

    while (ind)
    {
        instData->eventClass = CSR_BT_CM_PRIM;
        instData->recvMsgP = ind;

        CsrBtHfActivatedStateCmDataIndHandler(instData);

        SynergyMessageFree(instData->eventClass, instData->recvMsgP);
        instData->recvMsgP = NULL;

        ind = (CsrBtCmDataInd *) CsrBtCmStreamsDataIndGet(source,
                                                          RFCOMM_ID,
                                                          CSR_BT_CM_CONTEXT_UNUSED);
    }

    instData->recvMsgP = msg; /* Restore the original message */
}

void CsrBtHfMessageMoreSpaceHandler(HfMainInstanceData_t *instData)
{
    CsrBtCmDataCfm *cfm;
    MessageMoreSpace *mms = (MessageMoreSpace *) instData->recvMsgP;

    cfm = CsrBtCmStreamDataCfmGet(mms->sink,
                                  RFCOMM_ID,
                                  CSR_BT_CM_CONTEXT_UNUSED);

    if (cfm)
    {
        instData->recvMsgP = cfm;
        instData->eventClass = CSR_BT_CM_PRIM;

        CsrBtHfActivatedStateCmDataCfmHandler(instData);
        SynergyMessageFree(instData->eventClass, instData->recvMsgP);
    }

    /* MessageMoreSpace received from Synergy Service is a duplicate copy,
     * so CsrPmemFree can be used directly */
    CsrPmemFree(mms);
    instData->recvMsgP = NULL;

}

void CsrBtHfMessageMoreDataHandler(HfMainInstanceData_t *instData)
{
    MessageMoreData *mmd = instData->recvMsgP;
    csrBtHfStreamsProcessData(instData, mmd->source);

    /* MessageMoreData received from Synergy Service is a duplicate copy,
     * so CsrPmemFree can be used directly */
    CsrPmemFree(instData->recvMsgP);
    instData->recvMsgP = NULL;
}

void CsrBtHfStreamsRegister(HfMainInstanceData_t *instData, CsrBtConnId btConnId)
{
    CsrUint16 cid = CM_GET_UINT16ID_FROM_BTCONN_ID(btConnId);
    Sink sink = StreamRfcommSink(cid);
    Source source = StreamSourceFromSink(sink);

    if (CsrStreamsRegister(cid, RFCOMM_ID, CSR_BT_HF_IFACEQUEUE))
    {
        CsrStreamsSourceHandoverPolicyConfigure(cid,
                                                RFCOMM_ID,
                                                SOURCE_HANDOVER_ALLOW_WITHOUT_DATA);

        /* Ensure that early data (if any) is consumed */
        csrBtHfStreamsProcessData(instData, source);
    }
}

#endif /* CSR_STREAMS_ENABLE */

