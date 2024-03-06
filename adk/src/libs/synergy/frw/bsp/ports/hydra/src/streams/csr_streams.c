/******************************************************************************
 Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#include "csr_streams.h"
#include "csr_pmem.h"

CsrBool CsrStreamsRegister(CsrUint16 connId, CsrUint8 protocol, CsrSchedQid qId)
{
    Sink sink;

    if (protocol == RFCOMM_ID)
    {
        sink = StreamRfcommSink(connId);
    }
    else
    {
        sink = StreamL2capSink(connId);
    }

    if (sink)
    {    
        SourceConfigure(StreamSourceFromSink(sink),
                        VM_SOURCE_MESSAGES,
                        VM_MESSAGES_ALL);

        SynergyStreamsSinkRegister((CsrUint8) qId, sink);
        return TRUE;
    }

    return FALSE;
}

void CsrStreamsUnregister(CsrUint16 connId, CsrUint8 protocol)
{
    Sink sink;

    if (protocol == RFCOMM_ID)
    {
        sink = StreamRfcommSink(connId);
    }
    else
    {
        sink = StreamL2capSink(connId);
    }

    if (!sink)
    {
        CsrPanic(CSR_TECH_FW,
                 CSR_PANIC_FW_UNEXPECTED_VALUE,
                 "Invalid Sink");
    }

    StreamConnectDispose(StreamSourceFromSink(sink));

    SynergyStreamsSinkRegister(0xFF, sink);
}

void CsrStreamsMessageMoreSpaceHandler(Sink sink)
{
    CSR_UNUSED(sink);
}

CsrUint16 CsrStreamsMessageMoreDataHandler(Source source,
                                           CsrUint8 **payload)
{
    CsrUint16 payloadLength = 0;

    if (source && SourceIsValid(source))
    {
        payloadLength = SourceBoundary(source);

        if (payloadLength)
        {
            if (payload)
            {
                const CsrUint8 *dataPtr = SourceMap(source);

                *payload = (CsrUint8 *) CsrPmemAlloc(payloadLength);
                CsrMemMove(*payload, dataPtr, payloadLength);
            }
            else
            {
                /* App just wants to drop the data */
            }

            SourceDrop(source, payloadLength);
        }
    }

    return payloadLength;
}

CsrBool CsrStreamsDataSend(CsrUint16 connId,
                           CsrUint8 protocol,
                           CsrUint16 payloadLen,
                           CsrUint8 *payload)
{
    Sink sink;
    CsrBool sent = FALSE;

    if (protocol == RFCOMM_ID)
    {
        sink = StreamRfcommSink(connId);
    }
    else
    {
        sink = StreamL2capSink(connId);
    }

    if (sink)
    {
        /* Get the space available */
        CsrUint16 sinkSlack = SinkSlack(sink);

        if (sinkSlack >= payloadLen)
        {
            CsrUint16 sinkOffset = SinkClaim(sink, payloadLen);

            /* Check for valid offset */
            if (sinkOffset != INVALID_SINK_OFFSET)
            {
                CsrUint8 *dataPtr = SinkMap(sink);

                if (dataPtr)
                {
                    /* Copy data to buffer */
                    CsrMemMove(dataPtr + sinkOffset, payload, payloadLen);
                    sent = SinkFlush(sink, payloadLen);
                }
            }
        }
    }
    CsrPmemFree(payload);

    return sent;
}

void CsrStreamsSourceHandoverPolicyConfigure(CsrUint16 connId,
                                             CsrUint8 protocol,
                                             CsrUint8 handoverPolicy)
{
    Sink sink;

    if (protocol == RFCOMM_ID)
    {
        sink = StreamRfcommSink(connId);
    }
    else
    {
        sink = StreamL2capSink(connId);
    }

    if (sink)
    {
        SourceConfigure(StreamSourceFromSink(sink),
                        STREAM_SOURCE_HANDOVER_POLICY,
                        handoverPolicy);
    }
}

