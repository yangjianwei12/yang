/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_bt_panic.h"
#include "csr_bt_util.h"
#include "obex_handover.h"

/* Variables and functions to be used from csr_bt_obex_util.c */
extern ObexUtilInstType obexUtilInstList[OBEX_MAX_NUM_INSTANCES];

static void obexCleanUpAfterHandoverFailure(ObexUtilInstType *priInst)
{
#ifdef CSR_BT_INSTALL_OBEX_GOEP_20
    ObexUtilResetPrivateInstData(priInst, priInst->srmState);
#else
    ObexUtilResetPrivateInstData(priInst, FALSE);
#endif
    /* Clean up remaining data received during handover */
    priInst->maxTransportPacketLength = 0;
    priInst->maxLocalObexPacketLength = 0;
    priInst->remoteServerChannel = 0;
    priInst->cmState = CSR_BT_OBEX_UTIL_BT_RFC_CONNECTED_S; /* This is the default state 0x00 */
#ifdef CSR_BT_INSTALL_OBEX_GOEP_20
    priInst->windowSize = 0;
#endif
    CsrBtBdAddrZero(&priInst->deviceAddr);
    CsrPmemFree(priInst->cliInst);
}

static ObexUtilInstType* obexFetchInstFromInstanceId(CsrSchedQid instanceId)
{
    CsrUint8 instIdx;
    for (instIdx = 0; instIdx < OBEX_MAX_NUM_INSTANCES; instIdx++)
    {
        ObexUtilInstType * priInst  = &obexUtilInstList[instIdx];
        if (priInst)
        {
            if (priInst->phandle == instanceId)
            {
                return priInst;
            }
        }
    }
    return NULL;
}

static CsrBool obexIsInstanceConnected(ObexUtilInstType *priInst)
{
    if (priInst->cmState == CSR_BT_OBEX_UTIL_BT_RFC_CONNECTED_S
#ifdef CSR_BT_INSTALL_OBEX_GOEP_20
        || priInst->cmState == CSR_BT_OBEX_UTIL_BT_L2CA_CONNECTED_S
#endif
       )
    {
        return TRUE;
    }
    return FALSE;
}

static void convObexCliInstData(CsrBtMarshalUtilInst *conv,
                                ObexUtilInstType *priInst)
{
    if (priInst->cliInst == NULL)
    {
        priInst->cliInst = (ObexUtilCliInstType *) CsrPmemZalloc(sizeof(ObexUtilCliInstType));
        ObexUtilResetClientInstData(priInst->cliInst);
    }

    CsrBtMarshalUtilConvertObj(conv, priInst->cliInst->obtainedServer);
}

static void convObexInstData(CsrBtMarshalUtilInst *conv,
                             ObexUtilInstType *priInst)
{
    CsrBtMarshalUtilConvertObj(conv, priInst->processState);
    CsrBtMarshalUtilConvertObj(conv, priInst->cmState);
    CsrBtMarshalUtilConvertObj(conv, priInst->deviceAddr);
    CsrBtMarshalUtilConvertObj(conv, priInst->btConnId);
    CsrBtMarshalUtilConvertObj(conv, priInst->localServerCh);
    CsrBtMarshalUtilConvertObj(conv, priInst->rfcommMtu);
    CsrBtMarshalUtilConvertObj(conv, priInst->maxPeerObexPacketLength);
    CsrBtMarshalUtilConvertObj(conv, priInst->maxTransportPacketLength);
    CsrBtMarshalUtilConvertObj(conv, priInst->maxLocalObexPacketLength);
#if defined(CSR_BT_INSTALL_OBEX_CLI_HEADER_TARGET_WHO_CID) || defined(CSR_BT_INSTALL_OBEX_SRV_HEADER_TARGET_WHO_CID)
    CsrBtMarshalUtilConvertObj(conv, priInst->connectionId);
#endif
    CsrBtMarshalUtilConvertObj(conv, priInst->localPsm);
    CsrBtMarshalUtilConvertObj(conv, priInst->remoteServerChannel);
#ifdef CSR_BT_INSTALL_OBEX_GOEP_20
    CsrBtMarshalUtilConvertObj(conv, priInst->remotePsm);
    CsrBtMarshalUtilConvertObj(conv, priInst->srmState);
    CsrBtMarshalUtilConvertObj(conv, priInst->windowSize);
#endif
    convObexCliInstData(conv, priInst);
}

CsrBool ObexHandoverVeto(CsrSchedQid *connInstIdList,
                         CsrUint8 noOfConnInst)
{
    CsrUint8 connInstIdx;
    bool veto = FALSE;

    for (connInstIdx = 0; connInstIdx < noOfConnInst; connInstIdx++)
    {
        ObexUtilInstType * priInst = obexFetchInstFromInstanceId(connInstIdList[connInstIdx]);
        if (priInst)
        {
            if (priInst->processState    != CSR_BT_OBEX_UTIL_IDLE_P_S &&
                priInst->processState    != CSR_BT_OBEX_UTIL_OBEX_CONNECTED_P_S &&
                priInst->processState    != CSR_BT_OBEX_UTIL_TRANSPORT_CONNECTED_P_S)
            {
                 veto = TRUE;
            }
            if (!obexIsInstanceConnected(priInst) &&
                priInst->cmState != CSR_BT_OBEX_UTIL_BT_ACTIVATED_S)
            {
                veto = TRUE;
            }
            if (priInst->transmittingData == TRUE)
            {
                veto = TRUE;
            }
            if (veto)
            {
                CSR_LOG_TEXT_INFO((CsrBtObexUtilLto, 0, "ObexVetoed for instanceId %d processState->%d, cmState->%d, transmittingData %d", priInst->phandle, priInst->processState, priInst->cmState, priInst->transmittingData));
            }
        }
    }
    return veto;
}

void ObexHandoverSerInstData(CsrBtMarshalUtilInst *conv,
                             CsrSchedQid *connInstIdList,
                             CsrUint8 noOfConnInst)
{
    CsrUint8 connInstIdx;

    if (noOfConnInst > 0)
    {
        for (connInstIdx = 0; connInstIdx < noOfConnInst; connInstIdx++)
        {
            ObexUtilInstType * priInst = obexFetchInstFromInstanceId(connInstIdList[connInstIdx]);
            if (priInst)
            {
                CsrBtMarshalUtilConvertObj(conv, priInst->phandle);
                convObexInstData(conv, priInst);
                break;
            }
        }
    }
}

void ObexHandoverDeserInstData(CsrBtMarshalUtilInst *conv,
                               CsrSchedQid *connInstIdList,
                               CsrUint8 noOfConnInst)
{
    CsrUint8 connInstIdx;
    CsrSchedQid queueId = 0;

    for (connInstIdx = 0; connInstIdx < noOfConnInst; connInstIdx++)
    {
        CsrBtMarshalUtilConvertObj(conv, queueId);
        ObexUtilInstType * priInst = obexFetchInstFromInstanceId(queueId);
        if (priInst)
        {
            convObexInstData(conv, priInst);
            break;
        }
        else
        {
            CsrPanic(CSR_TECH_BT,
                     CSR_BT_PANIC_MYSTERY,
                     "No connected obex instance found for handover");
        }
    }
    CSR_UNUSED(connInstIdList);
}

void ObexHandoverCommit(CsrSchedQid *connInstIdList,
                        CsrUint8 noOfConnInst,
                        ObexUtilAuthenticateIndFuncType authenticateIndHandler,
                        ObexUtilDisconnectIndFuncType disconnectIndHandler)
{
    CsrUint8 connInstIdx;

    for (connInstIdx = 0; connInstIdx < noOfConnInst; connInstIdx++)
    {
        ObexUtilInstType * priInst = obexFetchInstFromInstanceId(connInstIdList[connInstIdx]);
        if (priInst)
        {
            if (priInst->cmState == CSR_BT_OBEX_UTIL_BT_RFC_CONNECTED_S)
            {
                CsrBtObexStreamsRegister(priInst, priInst->btConnId, RFCOMM_ID);
            }
#ifdef CSR_BT_INSTALL_OBEX_GOEP_20
            else if (priInst->cmState == CSR_BT_OBEX_UTIL_BT_L2CA_CONNECTED_S)
            {
                CsrBtObexStreamsRegister(priInst, priInst->btConnId, L2CAP_ID);
            }
            priInst->rxQueueCount = priInst->windowSize;
#endif /* CSR_BT_INSTALL_OBEX_GOEP_20 */
            priInst->cliInst->disconnectResultFunc = disconnectIndHandler;
#ifdef CSR_BT_INSTALL_OBEX_HEADER_AUTH_RESPONSE
            priInst->cliInst->authResultFunc = authenticateIndHandler;
#else
            CSR_UNUSED(authenticateIndHandler);
#endif
        }
    }
}

void ObexHandoverAbort(CsrSchedQid *connInstIdList,
                       CsrUint8 noOfConnInst)
{
    CsrUint8 connInstIdx;

    for (connInstIdx = 0; connInstIdx < noOfConnInst; connInstIdx++)
    {
        ObexUtilInstType * priInst = obexFetchInstFromInstanceId(connInstIdList[connInstIdx]);
        if (priInst)
        {
            obexCleanUpAfterHandoverFailure(priInst);
        }
    }
}
