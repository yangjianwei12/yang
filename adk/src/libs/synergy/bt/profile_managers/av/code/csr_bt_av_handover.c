/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "csr_synergy.h"

#include "csr_bt_handover_if.h"
#include "csr_bt_marshal_util.h"
#include "csr_pmem.h"
#include "csr_bt_panic.h"
#include "csr_bt_av_main.h"
#include "csr_bt_av_streams.h"


#ifdef CSR_LOG_ENABLE
#define CSR_BT_AV_LTSO_HANDOVER             0
#define CSR_BT_AV_HANDOVER_LOG_INFO(...)    CSR_LOG_TEXT_INFO((CsrBtAvLto, CSR_BT_AV_LTSO_HANDOVER, __VA_ARGS__))
#define CSR_BT_AV_HANDOVER_LOG_WARNING(...) CSR_LOG_TEXT_WARNING((CsrBtAvLto, CSR_BT_AV_LTSO_HANDOVER, __VA_ARGS__))
#define CSR_BT_AV_HANDOVER_LOG_ERROR(...)   CSR_LOG_TEXT_ERROR((CsrBtAvLto, CSR_BT_AV_LTSO_HANDOVER, __VA_ARGS__))
#else
#define CSR_BT_AV_HANDOVER_LOG_INFO(...)
#define CSR_BT_AV_HANDOVER_LOG_WARNING(...)
#define CSR_BT_AV_HANDOVER_LOG_ERROR(...)
#endif

typedef struct
{
    CsrUint8 pendingSigProc :4;
    av_con_state_t conState :4;
    CsrBool mediaConnInfoPresent :1;
    CsrBool incoming :1;
    CsrBool sending :1;
} av_connection_info_bitfields_t;

static CsrBtMarshalUtilInst *avConverter;

static CsrUint8 getEmptyAvConnId(void)
{
    CsrUint8 i;

    for (i = 0; i < CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        if (CsrBtBdAddrEqZero(&csrBtAvInstData.con[i].remoteDevAddr))
        {
            break;
        }
    }

    return i;
}

static CsrUint8 getAvConnId(const BD_ADDR_T *deviceAddr)
{
    CsrUint8 i;

    for (i = 0; i < CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        if (CsrBtBdAddrEq(&csrBtAvInstData.con[i].remoteDevAddr, deviceAddr))
        {
            break;
        }
    }

    return i;
}

static CsrUint8 getAvStreamByConnId(CsrUint8 connId)
{
    CsrUint8 s;

    for (s = 0; s < CSR_BT_AV_MAX_NUM_STREAMS; s++)
    {
        if (csrBtAvInstData.stream[s].conId == connId)
        {
            if (csrBtAvInstData.stream[s].streamState >= CONFIGURED_S)
            {/* Stream End Point (SEP) is configured. Return this stream index
                to marshal SEP details from the media connection instance.  */
                break;
            }
        }
    }

    return s;
}

static void getConnIdAndStreamId(const CsrBtDeviceAddr *addr, CsrBool *signalingConnInfoPresent, CsrUint8 *connId, CsrBool *mediaConnInfoPresent, CsrUint8 *streamId)
{
    *connId = getAvConnId(addr);
    *signalingConnInfoPresent = (*connId < CSR_BT_AV_MAX_NUM_CONNECTIONS) ? TRUE : FALSE;

    if (*signalingConnInfoPresent)
    {
        *streamId = getAvStreamByConnId(*connId);
        *mediaConnInfoPresent = (*streamId < CSR_BT_AV_MAX_NUM_STREAMS) ? TRUE : FALSE;
    }
}

static void convAvStreamInfo(CsrBtMarshalUtilInst *conv, av_stream_info_t *avStreamInfo)
{
    CsrBtMarshalUtilConvertObj(conv, avStreamInfo->mediaCid);
    CsrBtMarshalUtilConvertObj(conv, avStreamInfo->bitRate);
    CsrBtMarshalUtilConvertObj(conv, avStreamInfo->mediaMtu);
    CsrBtMarshalUtilConvertObj(conv, avStreamInfo->delayValue);
    CsrBtMarshalUtilConvertObj(conv, avStreamInfo->remoteCid);
    CsrBtMarshalUtilConvertObj(conv, avStreamInfo->remoteSeid);
    CsrBtMarshalUtilConvertObj(conv, avStreamInfo->localSeid);
}

static void deserAvStreamInfo(CsrBtMarshalUtilInst *conv, av_stream_info_t *avStreamInfo)
{
    av_stream_state_t streamState;

    convAvStreamInfo(conv, avStreamInfo);

    CsrBtMarshalUtilConvertObj(conv, streamState);

    avStreamInfo->streamState = streamState;
}

static void serAvStreamInfo(CsrBtMarshalUtilInst *conv, av_stream_info_t *avStreamInfo)
{
    av_stream_state_t streamState = avStreamInfo->streamState;

    convAvStreamInfo(conv, avStreamInfo);

    CsrBtMarshalUtilConvertObj(conv, streamState);
}

static void convAvConnectionInfo(CsrBtMarshalUtilInst *conv, av_connection_info_t *avConnInst)
{
    CsrBtMarshalUtilConvertObj(conv, avConnInst->signalCid);
    CsrBtMarshalUtilConvertObj(conv, avConnInst->signalMtu);
    CsrBtMarshalUtilConvertObj(conv, avConnInst->remoteAVDTPVersion);
    CsrBtMarshalUtilConvertObj(conv, avConnInst->aclHandle);
    CsrBtMarshalUtilConvertObj(conv, avConnInst->remoteCid);
}

static void deserAvConnectionInfo(CsrBtMarshalUtilInst *conv, av_connection_info_t *avConnInst, CsrBool *mediaConnInfoPresent)
{
    av_connection_info_bitfields_t bitfields = { 0 };

    /* In case bitfields are already read before an unmarshal resume */
    bitfields.pendingSigProc = avConnInst->pendingSigProc;
    bitfields.conState = avConnInst->conState;
    bitfields.incoming = avConnInst->incoming;
    bitfields.sending = avConnInst->sending;

    convAvConnectionInfo(conv, avConnInst);

    CsrBtMarshalUtilConvertObj(conv, bitfields);

    avConnInst->pendingSigProc = bitfields.pendingSigProc;
    avConnInst->conState = bitfields.conState;
    *mediaConnInfoPresent = bitfields.mediaConnInfoPresent;
    avConnInst->incoming = bitfields.incoming;
    avConnInst->sending = bitfields.sending;
}

static void serAvConnectionInfo(CsrBtMarshalUtilInst *conv, av_connection_info_t *avConnInst, CsrBool mediaConnInfoPresent)
{
    av_connection_info_bitfields_t bitfields = { 0 };

    bitfields.pendingSigProc = avConnInst->pendingSigProc;
    bitfields.conState = avConnInst->conState;
    bitfields.mediaConnInfoPresent = mediaConnInfoPresent;
    bitfields.incoming = avConnInst->incoming;
    bitfields.sending = avConnInst->sending;

    convAvConnectionInfo(conv, avConnInst);

    CsrBtMarshalUtilConvertObj(conv, bitfields);
}

static void deserAvInstData(CsrBtMarshalUtilInst *conv,
                            av_instData_t *avInst,
                            const CsrBtDeviceAddr *addr)
{
    CsrBool  signalingConnInfoPresent, mediaConnInfoPresent = FALSE;
    CsrUint8 connId, streamId = CSR_BT_AV_MAX_NUM_STREAMS;

    /* In case connId is already read and conId in stream data
     * is already set before an unmarshal resume */
    getConnIdAndStreamId(addr, &signalingConnInfoPresent, &connId, &mediaConnInfoPresent, &streamId);

    /* Get an empty slot to place unmarshalled connection instance */
    if (connId >= CSR_BT_AV_MAX_NUM_CONNECTIONS)
    {
        connId = getEmptyAvConnId();
    }

    if (connId >= CSR_BT_AV_MAX_NUM_CONNECTIONS)
    {
        /* Max number of AV connections are already connected.
         * Panic as it is not an expected state */
        CsrPanic(CSR_TECH_BT,
                 CSR_BT_PANIC_MYSTERY,
                 "Max number of AV connections are already connected");
    }

    avInst->con[connId].remoteDevAddr = *addr;
    deserAvConnectionInfo(conv, &avInst->con[connId], &mediaConnInfoPresent);
    if (mediaConnInfoPresent)
    {
        if (streamId >= CSR_BT_AV_MAX_NUM_STREAMS)
        {/* Not an unmarshal resume. Use the connId to place
            the unmarshalled media connection instance */
            streamId = connId;
        }
        avInst->stream[streamId].conId = connId;
        deserAvStreamInfo(conv, &avInst->stream[streamId]);
    }
}

static void serAvInstData(CsrBtMarshalUtilInst *conv,
                          av_instData_t *avInst,
                          const CsrBtDeviceAddr *addr)
{
    CsrBool  signalingConnInfoPresent, mediaConnInfoPresent = FALSE;
    CsrUint8 connId, streamId = CSR_BT_AV_MAX_NUM_STREAMS;

    getConnIdAndStreamId(addr, &signalingConnInfoPresent, &connId, &mediaConnInfoPresent, &streamId);

    if (signalingConnInfoPresent)
    {
        serAvConnectionInfo(conv, &avInst->con[connId], mediaConnInfoPresent);
        if (mediaConnInfoPresent)
        {
            serAvStreamInfo(conv, &avInst->stream[streamId]);
        }
    }
}

static bool csrBtAvVeto(void)
{
    bool veto = FALSE;

    if (csrBtAvInstData.state == READY_S)
    {
        CsrUint8 i;

        for (i = 0; i < CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
        {
            switch (csrBtAvInstData.con[i].conState)
            {
                case DISCONNECTED_S:
                    break;

                case CONNECTED_S:
                    if (csrBtAvInstData.stream[i].streamState != IDLE_S &&
                        csrBtAvInstData.stream[i].streamState != CONFIGURED_S &&
                        csrBtAvInstData.stream[i].streamState != OPENED_S &&
                        csrBtAvInstData.stream[i].streamState != STREAMING_S)
                    {
                        veto = TRUE;
                    }
                    break;

                case CONNECTING_S:
#ifdef INSTALL_AV_CANCEL_CONNECT
                case CANCEL_CONNECTING_S:
#endif
                case DISCONNECTING_S:
                default:
                    veto = TRUE;
                    break;
            }

            if (veto)
            {
                break;
            }
        }
    }
    else
    {
        veto = TRUE;
    }

    if (!veto)
    {
        if (csrBtAvInstData.saveQueue ||
            SynergySchedMessagesPendingForTask(CSR_BT_AV_IFACEQUEUE, NULL) != 0)
        {
            /* If there are pending messages in either the synergy queue or savequeue of AV, veto handover. */
            veto = TRUE;
        }
    }

    CSR_BT_AV_HANDOVER_LOG_INFO("csrBtAvVeto %d", veto);

    return veto;
}

static bool csrBtAvMarshal(const tp_bdaddr *vmTpAddrt,
                           CsrUint8 *buf,
                           CsrUint16 length,
                           CsrUint16 *written)
{
    CsrBtTpdAddrT tpAddrt = { 0 };

    CSR_BT_AV_HANDOVER_LOG_INFO("csrBtAvMarshal");

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);
    if (!avConverter)
    {
        avConverter = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_SERIALIZER);
    }

    CsrBtMarshalUtilResetBuffer(avConverter, length, buf, TRUE);

    serAvInstData(avConverter, &csrBtAvInstData, &tpAddrt.addrt.addr);

    *written = length - CsrBtMarshalUtilRemainingLengthGet(avConverter);

    return CsrBtMarshalUtilStatus(avConverter);
}

static bool csrBtAvUnmarshal(const tp_bdaddr *vmTpAddrt,
                             const CsrUint8 *buf,
                             CsrUint16 length,
                             CsrUint16 *written)
{
    CsrBtTpdAddrT tpAddrt = { 0 };

    CSR_BT_AV_HANDOVER_LOG_INFO("csrBtAvUnmarshal");

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    if (!avConverter)
    {
        avConverter = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_DESERIALIZER);
    }

    CsrBtMarshalUtilResetBuffer(avConverter, length, (void *) buf, TRUE);

    deserAvInstData(avConverter, &csrBtAvInstData, &tpAddrt.addrt.addr);

    *written = length - CsrBtMarshalUtilRemainingLengthGet(avConverter);

    return CsrBtMarshalUtilStatus(avConverter);
}

static void csrBtAvHandoverCommit(const tp_bdaddr *vmTpAddrt,
                                  const bool newPrimary)
{
    CsrBtTpdAddrT tpAddrt = { 0 };
    CsrUint8 connId;

    CSR_BT_AV_HANDOVER_LOG_INFO("csrBtAvHandoverCommit");

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    connId = getAvConnId(&tpAddrt.addrt.addr);

    if (connId < CSR_BT_AV_MAX_NUM_CONNECTIONS)
    {
        if (newPrimary)
        {
            CsrBtConnId signalConnId = csrBtAvInstData.con[connId].signalCid;

            if (signalConnId != CSR_BT_CONN_ID_INVALID)
            {
                CsrBtConnId mediaConnId = csrBtAvInstData.stream[connId].mediaCid;

#ifdef CSR_BT_RESTRICT_MAX_PROFILE_CONNECTIONS
                {
                    av_instData_t* avInst = &csrBtAvInstData;
                    if (getNumActivations(avInst->roleRegister) >  getNumIncomingCon(avInst))
                    {
                        CsrBtAvMakeConnectable(avInst);
                    }
                    else
                    {
                        avInst->isConnectable = FALSE;
                    }
                }
#endif

                CsrStreamsRegister(CM_GET_UINT16ID_FROM_BTCONN_ID(signalConnId),
                                   L2CAP_ID,
                                   CSR_BT_AV_IFACEQUEUE);

                CsrStreamsSourceHandoverPolicyConfigure(CM_GET_UINT16ID_FROM_BTCONN_ID(signalConnId),
                                                        L2CAP_ID,
                                                        SOURCE_HANDOVER_ALLOW_WITHOUT_DATA);

                if (mediaConnId != CSR_BT_CONN_ID_INVALID)
                {
                    CsrUint16 mediaCid = CM_GET_UINT16ID_FROM_BTCONN_ID(mediaConnId);
                    Source source = StreamSourceFromSink(StreamL2capSink(mediaCid));

                    CsrStreamsSourceHandoverPolicyConfigure(mediaCid,
                                                            L2CAP_ID,
                                                            SOURCE_HANDOVER_ALLOW);

                    /* If handover is performed when media is streaming, the
                       source will already exist and will be connected via a
                       transform (typically to audio subsystem). If handover is
                       performed when media is not streaming the source will be
                       created during the handover. In this latter case, the
                       source should be connected to a dispose transform in the
                       same way as the AVDTP L2CAP source is disposed when
                       initially connected. */
                    if (!TransformFromSource(source))
                    {
                        StreamConnectDispose(source);
                    }
                }
            }
        }
        else
        { /* Remove connection instance from new secondary */
#if 0
            csrBtAvInstData.con[connId].conState = DISCONNECTED_S;
            CsrBtBdAddrZero(&csrBtAvInstData.con[connId].remoteDevAddr);
#endif
        }
    }
}

static void csrBtAvHandoverComplete(const bool newPrimary)
{
    CSR_BT_AV_HANDOVER_LOG_INFO("csrBtAvHandoverComplete");

    if (avConverter)
    {
        CsrBtMarshalUtilDestroy(avConverter);
        avConverter = NULL;
    }

    CSR_UNUSED(newPrimary);
}

static void csrBtAvHandoverAbort(void)
{
    CSR_BT_AV_HANDOVER_LOG_INFO("csrBtAvHandoverAbort");
    if (avConverter && CsrBtMarshalUtilTypeGet(avConverter) == CSR_BT_MARSHAL_UTIL_DESERIALIZER)
    {
        CsrUint8 i;
        for (i = 0; i < CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
        {
            csrBtAvInstData.con[i].conState = DISCONNECTED_S;
            csrBtAvInstData.stream[i].streamState = IDLE_S;
            CsrBtBdAddrZero(&csrBtAvInstData.con[i].remoteDevAddr);
        }
    }
    csrBtAvHandoverComplete(FALSE);
}

const handover_interface csr_bt_av_handover_if =
        MAKE_BREDR_HANDOVER_IF(&csrBtAvVeto,
                               &csrBtAvMarshal,
                               &csrBtAvUnmarshal,
                               &csrBtAvHandoverCommit,
                               &csrBtAvHandoverComplete,
                               &csrBtAvHandoverAbort);

