/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_cm_streams_handler.h"
#include "csr_bt_cm_dm.h"

#ifdef CSR_BT_LE_ENABLE

static void *csrBtCmStreamPrimHeaderGet(Source source,
                                        CsrUint8 primSize)
{
    CsrUint16 primLength = SourceSizeHeader(source);
    const CsrUint8 *primValue = SourceMapHeader(source);

    if (primValue && primLength)
    {
        void *dmPrim = CsrPmemZalloc(primSize);
        SynMemMoveS(dmPrim, primSize, primValue, primLength);
        return dmPrim;
    }

    return NULL;
}


static void csrBtCmStreamTypeProcessData(cmInstanceData_t *cmData,
                                         CsrUint8 streamType,
                                         Source source)
{
    CsrUint8 primSize;
    void *savedMsg = cmData->recvMsgP; /* Save original message */

    if (streamType == CSR_BT_EXT_SCAN_STREAM)
    {
        primSize = sizeof(DM_ULP_EXT_SCAN_FILTERED_ADV_REPORT_IND_T);
    }
    else
    {
        primSize = sizeof(DM_ULP_PERIODIC_SCAN_SYNC_ADV_REPORT_IND_T);
    }

    cmData->recvMsgP = csrBtCmStreamPrimHeaderGet(source, primSize);
    if(cmData->recvMsgP)
    {
        CsrBtCmDmArrivalHandler(cmData);
    }

    cmData->recvMsgP = savedMsg; /* Restore original message */
}

void CmStreamFlushSource(Source src)
{
    CsrUint16 sourceSize = 0;

#ifdef CSR_TARGET_PRODUCT_VM
    CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "CmStreamFlushSource src=%p", src));
#endif

    SourceMap(src);

    while ((sourceSize = SourceBoundary(src)) != 0)
    {
        SourceDrop(src, sourceSize);
        CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "CmStreamFlushSource dropping=0x%x src=%p, total=0x%x ",
                            sourceSize, src, SourceSize(src)));
    }
}

void CsrBtCmStreamProcessData(cmInstanceData_t *cmData, Source source)
{
    CsrUint8 streamType = CSR_BT_UNIDENTIFIED_STREAM;

    /* Find the source stream type */
    if (source == cmData->extScanSource)
    {
        streamType = CSR_BT_EXT_SCAN_STREAM;
    }
    else
    {
        CsrUint8 i;

        for (i = 0; i < MAX_PERIODIC_SCAN_APP; i++)
        {
            if (cmData->periodicScanHandles[i].source == source)
            {
                streamType = CSR_BT_PER_SCAN_STREAM;
                break;
            }
        }
    }

    if (streamType != CSR_BT_UNIDENTIFIED_STREAM)
    {
        csrBtCmStreamTypeProcessData(cmData, streamType, source);
    }
    else
    {
        CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "CsrBtCmStreamProcessData invalid Source=%p", source));
    }
}


void CsrBtCmMessageMoreDataHandler(cmInstanceData_t *cmData)
{
    MessageMoreData *mmd = ((MessageMoreData *) cmData->recvMsgP);

    CsrBtCmStreamProcessData(cmData, mmd->source);

    /* Reassigning the prim pointer back */
    cmData->recvMsgP = mmd;
}
#endif /* CSR_BT_LE_ENABLE */

#endif /* CSR_STREAMS_ENABLE */

