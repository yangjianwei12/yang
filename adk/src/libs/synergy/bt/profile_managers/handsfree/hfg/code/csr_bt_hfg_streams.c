/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_hfg_streams.h"
#include "csr_bt_hfg_proto.h"

static void csrBtHfgStreamsProcessData(HfgMainInstance_t *instData,
                                       Source source)
{
    CsrBtCmDataInd *ind = (CsrBtCmDataInd *) CsrBtCmStreamsDataIndGet(source,
                                                                      RFCOMM_ID,
                                                                      CSR_BT_CM_CONTEXT_UNUSED);
    CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "csrBtHfgStreamsProcessData")); 

                                                                      
    void *msg = instData->msg; /* Save the original message */

    while (ind)
    {
        instData->msgClass = CSR_BT_CM_PRIM;
        instData->msg = ind;

        CsrBtHfgCmDataPrimHandler(instData);
        CsrBtCmFreeUpstreamMessageContents(instData->msgClass, instData->msg);
        SynergyMessageFree(instData->msgClass, instData->msg);
        instData->msg = NULL;

        ind = (CsrBtCmDataInd *) CsrBtCmStreamsDataIndGet(source,
                                                          RFCOMM_ID,
                                                          CSR_BT_CM_CONTEXT_UNUSED);
    }

    instData->msg = msg; /* Restore the original message */
}

void CsrBtHfgMessageMoreSpaceHandler(HfgMainInstance_t *instData)
{
    CsrBtCmDataCfm *cfm;
    MessageMoreSpace *mms = (MessageMoreSpace *) instData->msg;

    CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "CsrBtHfgMessageMoreSpaceHandler")); 

    cfm = CsrBtCmStreamDataCfmGet(mms->sink,
                                  RFCOMM_ID,
                                  CSR_BT_CM_CONTEXT_UNUSED);

    if (cfm)
    {
        instData->msg = cfm;
        instData->msgClass = CSR_BT_CM_PRIM;

        CsrBtHfgCmDataPrimHandler(instData);
        SynergyMessageFree(instData->msgClass, instData->msg);
    }

    /* MessageMoreSpace received from Synergy Service is a duplicate copy,
     * so CsrPmemFree can be used directly */
    CsrPmemFree(mms);
    instData->msg = NULL;

}

void CsrBtHfgMessageMoreDataHandler(HfgMainInstance_t *instData)
{
    MessageMoreData *mmd = instData->msg;
    CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "CsrBtHfgMessageMoreDataHandler")); 

    csrBtHfgStreamsProcessData(instData, mmd->source);

    /* MessageMoreData received from Synergy Service is a duplicate copy,
     * so CsrPmemFree can be used directly */
    CsrPmemFree(instData->msg);
    instData->msg = NULL;
}

void CsrBtHfgStreamsRegister(HfgMainInstance_t *instData, CsrBtConnId btConnId)
{
    CsrUint16 cid = CM_GET_UINT16ID_FROM_BTCONN_ID(btConnId);
    Sink sink = StreamRfcommSink(cid);
    Source source = StreamSourceFromSink(sink);

    CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "CsrBtHfgStreamsRegister")); 

    if (CsrStreamsRegister(cid, RFCOMM_ID, CSR_BT_HFG_IFACEQUEUE))
    {
        /* Ensure that early data (if any) is consumed */
        csrBtHfgStreamsProcessData(instData, source);
    }
}

#endif /* CSR_STREAMS_ENABLE */

