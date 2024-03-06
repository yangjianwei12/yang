/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_sched.h"
#include "csr_pmem.h"
#include "csr_env_prim.h"
#include "csr_bt_result.h"
#include "bluetooth.h"
#include "csr_bt_cm_prim.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif
#include "csr_bt_util.h"
#include "csr_bt_av_main.h"

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_av_streams.h"
#endif

#ifdef CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT
/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvGetL2caChannelInfoReqHandler
 *
 *  DESCRIPTION
 *      Handles get l2cap channel information request
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvGetL2caChannelInfoHandler(av_instData_t *instData, CsrUint16 id, CsrBool isSigChnl)
{
    CsrBtAvGetChannelInfoCfm *avPrim = 
                (CsrBtAvGetChannelInfoCfm *)CsrPmemAlloc(sizeof(CsrBtAvGetChannelInfoCfm));
    CsrUint16 remoteCid = 0;
    CsrUint16 aclHandle = instData->con[id].aclHandle;

    avPrim->type           = CSR_BT_AV_GET_CHANNEL_INFO_CFM;
    avPrim->resultSupplier = CSR_BT_SUPPLIER_AV;
    avPrim->resultCode     = CSR_BT_RESULT_CODE_AV_FAILED;

    if (isSigChnl)
    {/* AV signalling channel */
        remoteCid = instData->con[id].remoteCid;
    }
    else
    {/* AV media channel */
        remoteCid = instData->stream[id].remoteCid;
    }

    if ((0xFFFF != aclHandle) && (0 != remoteCid))
    {/* send cached channel information to application */
        avPrim->remoteCid  = remoteCid;/* attach l2cap channel id */
        avPrim->aclHandle  = aclHandle;
        avPrim->resultCode = CSR_BT_RESULT_CODE_AV_SUCCESS;
    }
    else
    {
#ifdef CSR_LOG_ENABLE
        /* this should not happen! */
        CSR_LOG_TEXT_WARNING
        (
            (CsrBtAvLto,0,
             "AV: CSR_BT_CM_L2CA_GET_CHANNEL_INFO_REQ failed!")
        );
#endif /* CSR_LOG_ENABLE */
    }

    CsrSchedMessagePut(instData->appHandle, CSR_BT_AV_PRIM, avPrim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvGetSigChannelInfoTimeOutHandler
 *
 *  DESCRIPTION
 *      Handles a timeout for retrieving information on L2CAP signalling channel.
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvGetSigChannelInfoTimeOutHandler(CsrUint16 id, void *data)
{
    av_instData_t *instData = data;
    av_connection_info_t *con = &instData->con[id];

    if (con->sigChannelInfoTimerId)
    {
        con->sigChannelInfoTimerId = 0;
        csrBtAvGetL2caChannelInfoHandler(instData, id, TRUE); /* av signalling channel */
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvGetMediaChannelInfoTimeOutHandler
 *
 *  DESCRIPTION
 *      Handles a timeout for retrieving information on L2CAP media channel.
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvGetMediaChannelInfoTimeOutHandler(CsrUint16 id, void *data)
{
    av_instData_t *instData = data;
    av_stream_info_t *stream = &instData->stream[id];

    if (stream->mediaChannelInfoTimerId)
    {
        stream->mediaChannelInfoTimerId = 0;
        csrBtAvGetL2caChannelInfoHandler(instData, id, FALSE); /* av media channel */
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      getMediaChannelInfoTimeOutHandler
 *
 *  DESCRIPTION
 *      Handles a timeout for retrieving information on L2CAP media channel.
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void getMediaChannelInfoTimeOutHandler(CsrUint16 id, void *data)
{
    av_instData_t *instData = data;
    CsrBtAvGetMediaChannelInfoCfm *avPrim = (CsrBtAvGetMediaChannelInfoCfm *) CsrPmemAlloc(sizeof(*avPrim));
    CsrUint16 remoteCid = 0;
    CsrUint8 conId = instData->stream[id].conId;
    CsrUint16 aclHandle = instData->con[conId].aclHandle;
    av_stream_info_t *stream = &instData->stream[id];

    stream->mediaChannelInformationTimerId = 0;

    avPrim->type           = CSR_BT_AV_GET_MEDIA_CHANNEL_INFO_CFM;
    avPrim->resultSupplier = CSR_BT_SUPPLIER_AV;
    avPrim->resultCode     = CSR_BT_RESULT_CODE_AV_FAILED;
    avPrim->shandle        = (CsrUint8) id;
    avPrim->localCid       = (CsrUint16) instData->stream[id].mediaCid;

    remoteCid = instData->stream[id].remoteCid;
    if ((0xFFFF != aclHandle) && (0 != remoteCid))
    {/* send cached channel information to application */
        avPrim->remoteCid  = remoteCid;/* attach l2cap channel id */
        avPrim->aclHandle  = aclHandle;
        avPrim->resultCode = CSR_BT_RESULT_CODE_AV_SUCCESS;
    }
    else
    {
#ifdef CSR_LOG_ENABLE
        /* this should not happen! */
        CSR_LOG_TEXT_WARNING
        (
            (CsrBtAvLto,0,
             "AV: CSR_BT_CM_L2CA_GET_CHANNEL_INFO_REQ failed!")
        );
#endif /* CSR_LOG_ENABLE */
    }

    CsrSchedMessagePut(instData->appHandle, CSR_BT_AV_PRIM, avPrim);
}
#endif /* CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT */

#ifdef INSTALL_AV_CUSTOM_SECURITY_SETTINGS
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSecurityInHandler
 *      CsrBtAvSecurityOutHandler
 *
 *  DESCRIPTION
 *      Set incoming/outgoing security
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrBtAvSecurityInHandler(av_instData_t *instData)
{
    CsrBtResultCode rval;
    CsrBtAvSecurityInReq *prim;
    prim = (CsrBtAvSecurityInReq*)instData->recvMsgP;

    rval = CsrBtScSetSecInLevel(&instData->secIncoming, prim->secLevel,
        CSR_BT_AV_MANDATORY_SECURITY_INCOMING,
        CSR_BT_AV_DEFAULT_SECURITY_INCOMING,
        CSR_BT_RESULT_CODE_AV_SUCCESS,
        CSR_BT_RESULT_CODE_AV_UNACCEPTABLE_PARAMETER);
    CsrBtAvSecurityInCfmSend(prim->appHandle, rval, CSR_BT_SUPPLIER_AV);
}

void CsrBtAvSecurityOutHandler(av_instData_t *instData)
{
    CsrBtResultCode rval;
    CsrBtAvSecurityOutReq *prim;
    prim = (CsrBtAvSecurityOutReq*)instData->recvMsgP;

    rval = CsrBtScSetSecOutLevel(&instData->secOutgoing, prim->secLevel,
        CSR_BT_AV_MANDATORY_SECURITY_OUTGOING,
        CSR_BT_AV_DEFAULT_SECURITY_OUTGOING,
        CSR_BT_RESULT_CODE_AV_SUCCESS,
        CSR_BT_RESULT_CODE_AV_UNACCEPTABLE_PARAMETER);
    CsrBtAvSecurityOutCfmSend(prim->appHandle, rval, CSR_BT_SUPPLIER_AV);
}
#endif /* INSTALL_AV_CUSTOM_SECURITY_SETTINGS */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvConnectReqHandler
 *
 *  DESCRIPTION
 *      Handle a request for an AV connection
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvConnectReqHandler( av_instData_t *instData)
{
    CsrUintFast8 i;
    CsrBtAvConnectReq * prim;

    prim = (CsrBtAvConnectReq *) instData->recvMsgP;

    for (i=0; i<CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        if (CsrBtBdAddrEq(&instData->con[i].remoteDevAddr, &prim->deviceAddr) &&
            instData->con[i].conState != DISCONNECTED_S)
        {
            /* Cross over scenario, incoming connection with remote device 
             * is already connected, reject outgoing connection for 
             * the same remote device.
             */
               
            CsrBtAvConnectCfmSend(prim->phandle, 0,
                                  prim->deviceAddr,
                                  CSR_BT_RESULT_CODE_AV_ALREADY_CONNECTED,
                                  CSR_BT_SUPPLIER_AV,
                                  CSR_BT_CONN_ID_INVALID);
            return;
        }
    }

    for (i=0; i<CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        if(instData->con[i].conState == DISCONNECTED_S)
        {
            break;
        }
    }

    if( i<CSR_BT_AV_MAX_NUM_CONNECTIONS)
    {
        /* Check if no service search is in progress */
#ifdef CSR_TARGET_PRODUCT_VM
        if (!instData->sdpAvSearchData && !instData->searchOngoing)
        {
#endif
            /* found an available connection entry */
            instData->appHandle             = prim->phandle;
            instData->con[i].conState       = CONNECTING_S;
            instData->con[i].remoteDevAddr  = prim->deviceAddr;

            /* Start service search operation for remote version */
            CsrBtAvSdcStartRemoteVersionSearch(instData, prim->deviceAddr);
#ifdef CSR_TARGET_PRODUCT_VM
        }
        else
        {
            /* Waiting for SDP to cancel/complete. */
            CsrBtAvSaveMessage(instData);
        }
#endif
    }
    else
    {
        /* CSRMAX. number of connections reached, reject */
        CsrBtAvConnectCfmSend(prim->phandle, 0,
                              prim->deviceAddr,
                              CSR_BT_RESULT_CODE_AV_MAX_NUM_OF_CONNECTIONS,
                              CSR_BT_SUPPLIER_AV,
                              CSR_BT_CONN_ID_INVALID);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      cancelAvConnectReqFromSaveQueue
 *
 *  DESCRIPTION
 *      Remove an AV connection request that was cancelled from the save queue
 *
 *  RETURNS
 *      TRUE if CSR_BT_AV_CONNECT_REQ was removed from queue, otherwise FALSE
 *
 *---------------------------------------------------------------------------*/
static CsrBool cancelAvConnectReqFromSaveQueue(av_instData_t *instData, CsrBtDeviceAddr addr)
{
    CsrUint16           eventClass;
    void                *msg;
    CsrBool             cancelMsg   = FALSE;
    CsrMessageQueueType *tempQueue  = NULL;

    while(CsrMessageQueuePop(&(instData->saveQueue), &eventClass, &msg))
    {
        CsrBtAvConnectReq * prim = (CsrBtAvConnectReq *) msg;

        if (!cancelMsg && 
            eventClass == CSR_BT_AV_PRIM &&
            (*((CsrBtAvPrim *)msg) == CSR_BT_AV_CONNECT_REQ) &&
            CsrBtBdAddrEq(&prim->deviceAddr, &addr))
        {
            cancelMsg = TRUE;
            SynergyMessageFree(eventClass, msg);
        }
        else
        {
            CsrMessageQueuePush(&tempQueue, eventClass, msg);
        }
    }
    instData->saveQueue = tempQueue;
    return (cancelMsg);
}

#ifdef INSTALL_AV_CANCEL_CONNECT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCancelConnectReqHandler
 *
 *  DESCRIPTION
 *      Handle a request cancelling AV connection establishment
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCancelConnectReqHandler( av_instData_t *instData)
{
    CsrUintFast8 i;
    CsrBtAvCancelConnectReq *prim;

    prim = (CsrBtAvCancelConnectReq *) instData->recvMsgP;

    for(i=0; i<CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        if( CsrBtBdAddrEq(&instData->con[i].remoteDevAddr, &prim->deviceAddr) )
        {
            if(instData->con[i].conState == CONNECTING_S)
            {
                if(CsrBtUtilSdcSearchCancel((void *)instData, instData->sdpAvSearchData) == TRUE)
                {
                    CsrBtCml2caCancelConnectReqSend(CSR_BT_AV_IFACEQUEUE, prim->deviceAddr, CSR_BT_AVDTP_PSM);
                }
            }
            else if (instData->con[i].conState == CONNECTED_S)
            {
                CsrBtCml2caDisconnectReqSend(instData->con[i].signalCid);
            }

            instData->con[i].conState = CANCEL_CONNECTING_S;
            break;
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCancelConnectReqNullStateHandler
 *
 *  DESCRIPTION
 *      Handle a request for cancel an AV connection in the NULL state
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCancelConnectReqNullStateHandler( av_instData_t *instData)
{
    CsrBtAvCancelConnectReq *prim = (CsrBtAvCancelConnectReq *) instData->recvMsgP;

    if (cancelAvConnectReqFromSaveQueue(instData, prim->deviceAddr))
    {
        CsrBtAvConnectCfmSend(instData->appHandle, 0, prim->deviceAddr,
                              CSR_BT_RESULT_CODE_AV_CANCEL_CONNECT_ATTEMPT,
                              CSR_BT_SUPPLIER_AV,
                              CSR_BT_CONN_ID_INVALID);
    }
}
#endif /* INSTALL_AV_CANCEL_CONNECT */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDisconnectReqHandler
 *
 *  DESCRIPTION
 *      Handle a request disconnecting an AV connection
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvDisconnectReqHandler( av_instData_t *instData)
{
    CsrBtAvDisconnectReq    *prim;

    prim = (CsrBtAvDisconnectReq *) instData->recvMsgP;

    if (prim->connectionId < CSR_BT_AV_MAX_NUM_CONNECTIONS &&
        instData->con[prim->connectionId].conState == CONNECTED_S)
    {
        CsrUintFast8 s;

        for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
        {
            if (prim->connectionId == instData->stream[s].conId)
            {
                if ( (instData->stream[s].mediaCid != 0)
                      && (instData->stream[s].streamState >= OPENED_S)
                      && (instData->stream[s].streamState <= PEER_ABORTING_S))
                {
                    CsrBtCml2caDisconnectReqSend(instData->stream[s].mediaCid);
                }
                else if (instData->stream[s].streamState == OPENING_S)
                {
                    instData->stream[s].streamState = TERMINATE_OPENING_S;
                }
            }
        }

        CsrBtCml2caDisconnectReqSend(instData->con[prim->connectionId].signalCid);
        instData->con[prim->connectionId].conState = DISCONNECTING_S;
    }
    else
    {
        CsrBtAvDisconnectIndSend(instData->appHandle, prim->connectionId,
                                 FALSE,
                                 prim->connectionId >= CSR_BT_AV_MAX_NUM_CONNECTIONS ? 
                                 CSR_BT_RESULT_CODE_AV_UNACCEPTABLE_PARAMETER : CSR_BT_RESULT_CODE_AV_NOT_CONNECTED,
                                 CSR_BT_SUPPLIER_AV);
    }
}

#ifdef CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetChannelInfoReqHandler
 *
 *  DESCRIPTION
 *      Handle a CSR_BT_AV_GET_CHANNEL_INFO_REQ
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvGetChannelInfoReqHandler(av_instData_t *instData)
{
    CsrBtAvGetChannelInfoReq *prim = (CsrBtAvGetChannelInfoReq *)instData->recvMsgP;
    CsrBtConnId btConnId = prim->btConnId;
    CsrUintFast8 index = 0;
    CsrUint16 aclHandle = 0xFFFF;
    CsrUint16 remoteCid = 0;

    for (index=0; index<CSR_BT_AV_MAX_NUM_CONNECTIONS; index++)
    {
        if (btConnId == instData->con[index].signalCid)
        {/* attach acl handle to av connection instance */
            aclHandle = instData->con[index].aclHandle;
            remoteCid = instData->con[index].remoteCid;

            if ((0xFFFF == aclHandle) || (0 == remoteCid))
            {/* in the process of retrieving it from l2cap; currently not available, try later */
                if (0 == instData->con[index].sigChannelInfoTimerId)
                {
                    instData->con[index].sigChannelInfoTimerId = 
                        CsrSchedTimerSet
                        (
                            CSR_SCHED_SECOND,
                            csrBtAvGetSigChannelInfoTimeOutHandler,
                            (CsrUint16)index,
                            instData
                        );
                }
                else
                {/* wait for timer expiry - no handling required */
                }
                return;
            }
            break;
        }
    }

    {/* respond to application */
        CsrBtAvGetChannelInfoCfm *avPrim = 
            (CsrBtAvGetChannelInfoCfm *)CsrPmemAlloc(sizeof(CsrBtAvGetChannelInfoCfm));
        avPrim->type = CSR_BT_AV_GET_CHANNEL_INFO_CFM;
        avPrim->resultSupplier = CSR_BT_SUPPLIER_AV;

        if (index < CSR_BT_AV_MAX_NUM_CONNECTIONS)
        {/* found btConnId */
            avPrim->aclHandle  = aclHandle;
            avPrim->remoteCid  = remoteCid;/* attach l2cap signalling channel id */
            avPrim->resultCode = CSR_BT_RESULT_CODE_AV_SUCCESS;
        }
        else
        {
            avPrim->resultCode = CSR_BT_RESULT_CODE_AV_NOT_CONNECTED;
        }

        CsrSchedMessagePut(instData->appHandle, CSR_BT_AV_PRIM, avPrim);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetStreamChannelInfoReqHandler
 *
 *  DESCRIPTION
 *      Handle a CSR_BT_AV_GET_STREAM_CHANNEL_INFO_REQ
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvGetStreamChannelInfoReqHandler(av_instData_t *instData)
{
    CsrBtAvGetStreamChannelInfoReq *prim = (CsrBtAvGetStreamChannelInfoReq *)instData->recvMsgP;
    CsrUint8 shandle = prim->shandle;
    CsrBtResultCode resultCode = CSR_BT_RESULT_CODE_AV_FAILED;
    CsrUint16 remoteCid = 0;
    CsrUint16 aclHandle = 0xFFFF;

    if (shandle >= CSR_BT_AV_MAX_NUM_STREAMS)
    {
        resultCode = CSR_BT_RESULT_CODE_AV_UNACCEPTABLE_PARAMETER;
    }
    else
    {
        CsrUint8 conId = instData->stream[shandle].conId;

        if (instData->con[conId].conState != CONNECTED_S)
        {/* signalling channel */
            resultCode = CSR_BT_RESULT_CODE_AV_NOT_CONNECTED;
        }
        else if ((instData->stream[shandle].streamState < OPENED_S) || 
                        (instData->stream[shandle].streamState > STREAMING_S))
        {/* media channel */
            resultCode = CSR_BT_RESULT_CODE_AV_NOT_CONNECTED;
        }
        else
        {/* alrighy! */
            aclHandle = instData->con[conId].aclHandle;
            remoteCid = instData->stream[shandle].remoteCid;

            if ((0xFFFF == aclHandle) || (0 == remoteCid))
            {/* in the process of retrieving it from l2cap; currently not available, try later */
                if (0 == instData->stream[shandle].mediaChannelInfoTimerId)
                {
                    instData->stream[shandle].mediaChannelInfoTimerId = 
                        CsrSchedTimerSet
                        (
                            CSR_SCHED_SECOND,
                            csrBtAvGetMediaChannelInfoTimeOutHandler,
                            (CsrUint16)shandle,
                            instData
                        );
                }
                else
                {/* wait for timer expiry - no handling required */
                }
                return;
            }
            else
            {
                resultCode = CSR_BT_RESULT_CODE_AV_SUCCESS;
            }
        }
    }

    {/* send result to application */
        CsrBtAvGetChannelInfoCfm *avPrim = 
            (CsrBtAvGetChannelInfoCfm *)CsrPmemAlloc(sizeof(CsrBtAvGetChannelInfoCfm));

        avPrim->type           = CSR_BT_AV_GET_CHANNEL_INFO_CFM;
        avPrim->resultSupplier = CSR_BT_SUPPLIER_AV;
        avPrim->resultCode     = resultCode;
        avPrim->remoteCid      = remoteCid;/* attach l2cap media channel id */
        avPrim->aclHandle      = aclHandle;

        CsrSchedMessagePut(instData->appHandle, CSR_BT_AV_PRIM, avPrim);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetMediaChannelInfoReqHandler
 *
 *  DESCRIPTION
 *      Handle a CSR_BT_AV_GET_MEDIA_CHANNEL_INFO_REQ
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvGetMediaChannelInfoReqHandler(av_instData_t *instData)
{
    CsrBtAvGetMediaChannelInfoReq *prim = (CsrBtAvGetMediaChannelInfoReq *) instData->recvMsgP;
    CsrUint8 shandle = prim->shandle;
    CsrBtResultCode resultCode = CSR_BT_RESULT_CODE_AV_FAILED;
    CsrUint16 remoteCid = 0;
    CsrUint16 aclHandle = 0xFFFF;
    CsrUint16 localCid  = 0;

    if (shandle >= CSR_BT_AV_MAX_NUM_STREAMS)
    {
        resultCode = CSR_BT_RESULT_CODE_AV_UNACCEPTABLE_PARAMETER;
    }
    else
    {
        CsrUint8 conId = instData->stream[shandle].conId;

        if (instData->con[conId].conState != CONNECTED_S)
        {/* signalling channel */
            resultCode = CSR_BT_RESULT_CODE_AV_NOT_CONNECTED;
        }
        else if ((instData->stream[shandle].streamState < OPENED_S) || 
                        (instData->stream[shandle].streamState > STREAMING_S))
        {/* media channel */
            resultCode = CSR_BT_RESULT_CODE_AV_NOT_CONNECTED;
        }
        else
        {/* alrighy! */
            aclHandle = instData->con[conId].aclHandle;
            remoteCid = instData->stream[shandle].remoteCid;
            localCid  = (CsrUint16) instData->stream[shandle].mediaCid;

            if ((0xFFFF == aclHandle) || (0 == remoteCid))
            {/* in the process of retrieving it from l2cap; currently not available, try later */
                if (0 == instData->stream[shandle].mediaChannelInformationTimerId)
                {
                    instData->stream[shandle].mediaChannelInformationTimerId = 
                        CsrSchedTimerSet
                        (
                            CSR_SCHED_SECOND,
                            getMediaChannelInfoTimeOutHandler,
                            (CsrUint16) shandle,
                            instData
                        );
                }
                return;
            }
            else
            {
                resultCode = CSR_BT_RESULT_CODE_AV_SUCCESS;
            }
        }
    }

    {/* send result to application */
        CsrBtAvGetMediaChannelInfoCfm *avPrim = (CsrBtAvGetMediaChannelInfoCfm *) CsrPmemAlloc(sizeof(*avPrim));

        avPrim->type           = CSR_BT_AV_GET_MEDIA_CHANNEL_INFO_CFM;
        avPrim->resultSupplier = CSR_BT_SUPPLIER_AV;
        avPrim->resultCode     = resultCode;
        avPrim->remoteCid      = remoteCid;/* attach l2cap media channel id */
        avPrim->aclHandle      = aclHandle;
        avPrim->shandle        = prim->shandle;
        avPrim->localCid       = localCid;

        CsrSchedMessagePut(instData->appHandle, CSR_BT_AV_PRIM, avPrim);
    }
}
#endif /* CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCmL2caConnectCfmHandler
 *
 *  DESCRIPTION
 *      Handle a CSR_BT_CM_L2CA_CONNECT_CFM
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCmL2caConnectCfmHandler( av_instData_t *instData)
{
    CsrBtCmL2caConnectCfm *prim;
    CsrUintFast8 i, s;
    CsrBool knownDevice = FALSE;
    CsrBtConnId btConnId;

    prim = (CsrBtCmL2caConnectCfm*) instData->recvMsgP;
    btConnId = prim->btConnId;
    /* could be a terminating stream being opened, check it */
    for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
    {
        if( instData->stream[s].streamState == TERMINATE_OPENING_S )
        {
            if (prim->resultSupplier == CSR_BT_SUPPLIER_CM &&
                prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                /* immediately disconnect it, we are supposed to terminate it */
                CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL,prim->deviceAddr,btConnId);
                CsrBtCml2caDisconnectReqSend(btConnId);
                knownDevice = TRUE;
            }
            CsrBtAvClearStream(instData, (CsrUint8)s);
            return;
        }
    }

    for (i=0; i<CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        if( CsrBtBdAddrEq(&instData->con[i].remoteDevAddr, &prim->deviceAddr))
        {
            knownDevice = TRUE;
            if (prim->resultSupplier == CSR_BT_SUPPLIER_CM &&
                prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS )
            {
                /* the creation of an L2CAP channel was a success */
                if(instData->con[i].conState == CONNECTING_S)
                {
                    CsrBtCmL2caGetChannelInfoReqSend(btConnId, CSR_BT_AV_IFACEQUEUE);
                    CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL,prim->deviceAddr,btConnId);
                    /* we are establishing the signalling channel */
                    instData->con[i].signalCid = btConnId;
                    instData->con[i].signalMtu = prim->mtu;
                    instData->con[i].timerId = 0;

                    CsrBtAvConnectCfmSend(instData->appHandle,
                                          (CsrUint8)i,
                                          prim->deviceAddr,
                                          CSR_BT_RESULT_CODE_AV_SUCCESS,
                                          CSR_BT_SUPPLIER_AV,
                                          btConnId);
                    CsrBtAvStatusIndSend(instData, CSR_BT_AV_CONNECT_CFM, 0,(CsrUint8) i);
                    instData->con[i].conState = CONNECTED_S;
#ifdef CSR_STREAMS_ENABLE
                    CsrBtAvStreamsRegister(instData, btConnId);
#endif
                }
                else if (instData->con[i].conState == CONNECTED_S)
                {
                    /* media stream channel */
                    for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
                    {
                        if((instData->stream[s].conId == i) &&
                            (instData->stream[s].streamState == OPENING_S))
                        {
                            CsrBtCmL2caGetChannelInfoReqSend(btConnId, CSR_BT_AV_IFACEQUEUE);
                            CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL,prim->deviceAddr,btConnId);
                            /* we are establishing the media channel */
                            instData->stream[s].streamState = OPENED_S;
                            instData->stream[s].mediaCid = btConnId;
                            instData->stream[s].mediaMtu = prim->mtu;
#ifdef CSR_STREAMS_ENABLE
                            CsrStreamsSourceHandoverPolicyConfigure((CM_GET_UINT16ID_FROM_BTCONN_ID(prim->btConnId)),
                                                                    L2CAP_ID,
                                                                    SOURCE_HANDOVER_ALLOW);
                            StreamConnectDispose(StreamSourceFromSink(StreamL2capSink(CM_GET_UINT16ID_FROM_BTCONN_ID(prim->btConnId))));
#endif
                            CsrBtAvOpenCfmSend(instData, (CsrUint8)s, instData->stream[s].tLabel,
                                CSR_BT_RESULT_CODE_AV_SUCCESS,
                                CSR_BT_SUPPLIER_AV);
                            CsrBtAvStreamMtuSizeIndSend(instData, (CsrUint8)s, btConnId);
                            instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */

                            /* There can be the scenario where remote device has already sent
                             * Close Command to configure different Stream end-point while AV
                             * is in the middle of creating L2CAP Media channel. Send the Close
                             * indication now to application if there is any saved in save queue */
                            AvSendPendingUpstreamMessages(instData, CSR_BT_AV_CLOSE_IND);
                            break;
                        }
                    }
                }
#ifdef INSTALL_AV_CANCEL_CONNECT
                else if (instData->con[i].conState == CANCEL_CONNECTING_S)
                {
                    CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL,prim->deviceAddr,btConnId);
                    /* a connect cancel came in too late, need to disconnect */
                    CsrBtCml2caDisconnectReqSend(btConnId);
                }
#endif /* INSTALL_AV_CANCEL_CONNECT */
            }
            else
            {
                /* the creation of an L2CAP connection failed
                 * so propagate the error up to caller */
#ifdef INSTALL_AV_CANCEL_CONNECT
                if (instData->con[i].conState == CANCEL_CONNECTING_S )
                {
                    /* Except if we cancelled the connection attempt. */
                    CsrBtAvClearConnection(instData, (CsrUint8)i);
                    CsrBtAvConnectCfmSend(instData->appHandle,
                                          0, prim->deviceAddr,
                                          CSR_BT_RESULT_CODE_AV_CANCEL_CONNECT_ATTEMPT,
                                          CSR_BT_SUPPLIER_AV,
                                          CSR_BT_CONN_ID_INVALID);
                }
                else if (instData->con[i].conState != CONNECTED_S)
#else
                if (instData->con[i].conState != CONNECTED_S)
#endif /* INSTALL_AV_CANCEL_CONNECT */
                {
                    /* we are establishing the signalling channel */
                    CsrBtAvClearConnection(instData, (CsrUint8)i);
                    CsrBtAvConnectCfmSend(instData->appHandle,
                                          0,
                                          prim->deviceAddr,
                                          prim->resultCode,
                                          prim->resultSupplier,
                                          btConnId);
                }
                else
                {
                    /* we are establishing the media stream channel */
                    for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
                    {
                        if( (instData->stream[s].conId == i) && (instData->stream[s].streamState == OPENING_S))
                        {
                            instData->stream[s].streamState = CONFIGURED_S;
                            CsrBtAvOpenCfmSend(instData, (CsrUint8)s, instData->stream[s].tLabel,
                                prim->resultCode, prim->resultSupplier);
                            /* Check and process the AV close indication if the remote device has sent it
                             * while AV was in the middle of creating L2CAP Media channel */
                            AvSendPendingUpstreamMessages(instData, CSR_BT_AV_CLOSE_IND);
                            break;
                        }
                    }
                }
            }
            break;
        }
    }

    if (!knownDevice && prim->resultSupplier == CSR_BT_SUPPLIER_CM &&
        prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        /* Disconnect all unknown connections */
        CsrBtCml2caDisconnectReqSend(btConnId);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCmL2caConnectAcceptCfmHandler
 *
 *  DESCRIPTION
 *      Handle a CSR_BT_CM_L2CA_CONNECT_ACCEPT_CFM
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCmL2caConnectAcceptCfmHandler( av_instData_t *instData)
{
    CsrUintFast8 i, s;
    CsrBtCmL2caConnectAcceptCfm *prim = (CsrBtCmL2caConnectAcceptCfm *) instData->recvMsgP;
    CsrBtConnId btConnId = prim->btConnId;

    for (i=0; i<CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        if( (instData->con[i].conState == CONNECTED_S)
            && CsrBtBdAddrEq(&instData->con[i].remoteDevAddr, &prim->deviceAddr) )
        {
            /* apparently a stream being opened, lets find that stream */
            for (s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
            {
                if((instData->stream[s].conId == i) &&
                    ((instData->stream[s].streamState == PEER_OPENING_S) || (instData->stream[s].streamState == PEER_OPENING_TO_STREAM_S)))
                {
                    if( prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
                    {
                        instData->stream[s].mediaCid = btConnId;
                        instData->stream[s].mediaMtu = prim->mtu;
#ifdef CSR_STREAMS_ENABLE
                        CsrStreamsSourceHandoverPolicyConfigure((CM_GET_UINT16ID_FROM_BTCONN_ID(prim->btConnId)),
                                                                L2CAP_ID,
                                                                SOURCE_HANDOVER_ALLOW);
                       StreamConnectDispose(StreamSourceFromSink(StreamL2capSink(CM_GET_UINT16ID_FROM_BTCONN_ID(prim->btConnId))));
#endif
                        CsrBtCmL2caGetChannelInfoReqSend(btConnId, CSR_BT_AV_IFACEQUEUE);

                        CsrBtAvStreamMtuSizeIndSend(instData,(CsrUint8) s, btConnId);

                        if (instData->stream[s].streamState == PEER_OPENING_S)
                        {
                            CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL,prim->deviceAddr,btConnId);
                            instData->stream[s].streamState = OPENED_S;

                            /* It is possible that remote has already sent Start command
                             * while AV was waiting for Connect Accept Cfm from CM. AV
                             * would have saved Start command in the queue if it has received so.
                             * Check and send Start indication to application now.*/
                            AvSendPendingUpstreamMessages(instData, CSR_BT_AV_START_IND);
                        }
                        else
                        {
                            CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_STREAM_CHANNEL,prim->deviceAddr,btConnId);
                            instData->stream[s].streamState = STREAMING_S;
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
                            CsrBtAvStreamStartStopIndSend(instData, s, TRUE);
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */
                        }
                    }
                    else
                    {
                        instData->stream[s].streamState = CONFIGURED_S;
                        /* throw exception ? */
                    }
                    break;
                }
            }


            if (s == CSR_BT_AV_MAX_NUM_STREAMS)
            {/* Stream not found; already connected: reject this connection  */
                CsrBtCml2caDisconnectReqSend(btConnId);
                /* and make sure to accepting new incoming connections */
                CsrBtAvMakeConnectable(instData);
            }
            return;
        }
    }

    /* could be a terminating stream being opened, check it */
    for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
    {
        if( instData->stream[s].streamState == TERMINATE_PEER_OPENING_S )
        {
            /* immediately disconnect it, we are supposed to terminate it */
            CsrBtCml2caDisconnectReqSend(btConnId);
            CsrBtAvClearStream(instData, (CsrUint8)s);
            return;
        }
    }

    /* then lets assume it is a new (device) signalling connection */
    instData->isConnectable = FALSE;
    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        CsrBool collision    = FALSE;
        CsrUint8 freeConnIdx = CSR_BT_AV_MAX_NUM_CONNECTIONS;

        /* check if there is any collision */
        for (i=0; i < CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
        {
            if ((instData->con[i].conState == CONNECTING_S) &&
                 CsrBtBdAddrEq(&instData->con[i].remoteDevAddr, &prim->deviceAddr))
            {
                /* The connection was established from the same device in the process
                 * of being connected to. Attempt to cancel SDP search & outgoing connection request */
                CsrBtCml2caCancelConnectReqSend(CSR_BT_AV_IFACEQUEUE,
                                                prim->deviceAddr,
                                                CSR_BT_AVDTP_PSM);
                freeConnIdx = i;
                collision = TRUE;
                break;
            }
            else if (freeConnIdx == CSR_BT_AV_MAX_NUM_CONNECTIONS &&
                     instData->con[i].conState == DISCONNECTED_S)
            {
                freeConnIdx = i;
            }
        }

        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL, prim->deviceAddr, btConnId);

        if (freeConnIdx < CSR_BT_AV_MAX_NUM_CONNECTIONS)
        {
            instData->con[freeConnIdx].conState      = CONNECTED_S;
            instData->con[freeConnIdx].incoming      = TRUE;
            instData->con[freeConnIdx].remoteDevAddr = prim->deviceAddr;
            instData->con[freeConnIdx].signalCid     = btConnId;
            instData->con[freeConnIdx].signalMtu     = prim->mtu;
            instData->con[freeConnIdx].timerId       = 0;

            CsrBtCmL2caGetChannelInfoReqSend(btConnId, CSR_BT_AV_IFACEQUEUE);

            /* In case of collision, we would have already done SDP search
             * while processing outgoing connection request */
            if (!collision)
            {
                /* get the version of the remote device */
                CsrBtAvSdcStartRemoteVersionSearch(instData, prim->deviceAddr);
            }

            /* Send the connect indication to application now */
            CsrBtAvConnectIndSend(instData->appHandle,
                                  freeConnIdx,
                                  prim->deviceAddr,
                                  btConnId);
            CsrBtAvStatusIndSend(instData, CSR_BT_AV_CONNECT_IND, 0, freeConnIdx);
            /* Inform application about the outgoing connection status due to collision or
               if there is any pending connection request saved */
            if (collision || cancelAvConnectReqFromSaveQueue(instData, prim->deviceAddr))
            {
                CsrBtAvConnectCfmSend(instData->appHandle, 0, prim->deviceAddr,
                                      CSR_BT_RESULT_CODE_AV_ALREADY_CONNECTED,
                                      CSR_BT_SUPPLIER_AV,
                                      CSR_BT_CONN_ID_INVALID);
            }
#ifdef CSR_STREAMS_ENABLE
            CsrBtAvStreamsRegister(instData, btConnId);
#endif
        }
        else
        {
            /* ...out of resources, reject this connection */
            CsrBtCml2caDisconnectReqSend(btConnId);
        }

        if (getNumActivations(instData->roleRegister) > getNumIncomingCon(instData))
        {
            CsrBtAvMakeConnectable(instData);
        }
    }
    else
    {
        /* ...out of connection resources */
        /* throw exception..? */
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCmL2caCancelConnectAcceptCfmHandler
 *
 *  DESCRIPTION
 *      Handle a CSR_BT_CM_L2CA_CANCEL_CONNECT_ACCEPT_CFM
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCmL2caCancelConnectAcceptCfmHandler( av_instData_t *instData)
{
    CsrUintFast8 s;
    CsrBtCmL2caCancelConnectAcceptCfm *prim;

    prim = (CsrBtCmL2caCancelConnectAcceptCfm *) instData->recvMsgP;

    /* first check if any streams are doing cancel accept connect */
    for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
    {
        if(instData->stream[s].streamState == TERMINATE_PEER_OPENING_S)
        {
            if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
            {
                CsrBtAvClearStream(instData,(CsrUint8) s);
            }
            return;
        }
    }

#ifdef INSTALL_AV_DEACTIVATE
    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        if(instData->doDeactivate)
        {
            CsrBtAvDeactivateCfmSend(instData->appHandle, CSR_BT_RESULT_CODE_AV_SUCCESS,
                CSR_BT_SUPPLIER_AV);
            instData->doDeactivate = FALSE;
            instData->isConnectable = FALSE;
        }
    }
    else
    {
        /* an incoming connection must be about to arrive */
        if(instData->doDeactivate)
        {
            CsrBtAvDeactivateCfmSend(instData->appHandle, CSR_BT_RESULT_CODE_AV_CANCEL_CONNECT_ATTEMPT,
                CSR_BT_SUPPLIER_AV);
            instData->doDeactivate = FALSE;
        }
    }
#endif /* INSTALL_AV_DEACTIVATE */
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCmL2caDisconnectIndHandler
 *
 *  DESCRIPTION
 *      Handle a CSR_BT_CM_L2CA_DISCONNECT_IND
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCmL2caDisconnectIndHandler( av_instData_t *instData)
{
    CsrUintFast8 i, s;
    CsrBtCmL2caDisconnectInd *prim;

    prim = (CsrBtCmL2caDisconnectInd*) instData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    if (!prim->localTerminated)
    {
        /* For remote disconnections, profile needs to respond to L2CA_DISCONNECT_IND. */
        CsrBtCmL2caDisconnectRspSend(prim->l2caSignalId, prim->btConnId);
    }
#endif

    for (s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
    {
        if( prim->btConnId == instData->stream[s].mediaCid)
        {
            if( instData->stream[s].streamState == CLOSING_S)
            {
                CsrBtAvCloseCfmSend(instData,(CsrUint8) s, instData->stream[s].tLabel,
                    CSR_BT_RESULT_CODE_AV_SUCCESS,
                    CSR_BT_SUPPLIER_AV);
            }
            else if( instData->stream[s].streamState == ABORTING_S)
            {
                CsrBtAvAbortCfmSend(instData,(CsrUint8) s, instData->stream[s].tLabel);
            }
            else if (instData->stream[s].streamState == ABORT_REQUESTED_S)
            {
                CsrBtCmLogicalChannelTypeReqSend(CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL,
                                                 instData->con[instData->stream[s].conId].remoteDevAddr,
                                                 prim->btConnId);
                instData->stream[s].mediaCid = 0;
                /* Stream to be cleared when remote sends Abort response */
                return;
            }
            else if (!prim->localTerminated                                           &&
                     instData->con[instData->stream[s].conId].conState == CONNECTED_S &&
                     instData->stream[s].streamState != PEER_CLOSING_S)
            {/* Peer disconnected media channel without AV Close, send AV Close indication to application locally(to notify media disconnection) */
                AvCloseIndSend(instData, s, 0xFF /* Invalid tLabel*/);
                CsrBtCmLogicalChannelTypeReqSend(CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL,
                                                 instData->con[instData->stream[s].conId].remoteDevAddr,
                                                 prim->btConnId);
                instData->stream[s].mediaCid = 0;
                /* Reset the stream state(and stream info) on receiving close response */ 
                instData->stream[s].streamState = CLOSING_S;
                return;
            }
            CsrBtCmLogicalChannelTypeReqSend(CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL,
                              instData->con[instData->stream[s].conId].remoteDevAddr,prim->btConnId);

            instData->stream[s].mediaCid = 0;

            CsrBtAvClearStream(instData,(CsrUint8) s);
            return;
        }
    }

    for (i=0; i<CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        if(prim->btConnId == instData->con[i].signalCid)
        {
            CsrBtCmLogicalChannelTypeReqSend(CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL,
                              instData->con[i].remoteDevAddr,prim->btConnId);
#ifdef INSTALL_AV_CANCEL_CONNECT
            if( instData->con[i].conState == CANCEL_CONNECTING_S)
            {
                CsrBtAvDisconnectIndSend(instData->appHandle,(CsrUint8) i,
                    TRUE,
                    CSR_BT_RESULT_CODE_AV_CANCEL_CONNECT_ATTEMPT,
                    CSR_BT_SUPPLIER_AV);
            }
            else
#endif /* INSTALL_AV_CANCEL_CONNECT */
            {
                CsrBtAvDisconnectIndSend(instData->appHandle,(CsrUint8) i,
                    prim->localTerminated,
                    prim->reasonCode, prim->reasonSupplier);
            }
            CsrBtAvStatusIndSend(instData, CSR_BT_AV_DISCONNECT_IND, 0,(CsrUint8) i);

            if( (instData->con[i].incoming == TRUE) && !instData->isConnectable
                && (getNumActivations(instData->roleRegister) > (getNumIncomingCon(instData) - 1)) )
            {
                /* the released connection was an incoming - need to be connectable once again */
                CsrBtAvMakeConnectable(instData);
            }

            CsrBtAvClearConnection(instData,(CsrUint8) i);
            return;
        }
    }

#ifdef INSTALL_AV_CANCEL_CONNECT
    /* the rests of a disconnect caused by a cancel connect arriving too late */
    for (i=0; i<CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        if( instData->con[i].conState == CANCEL_CONNECTING_S)
        {
            CsrBtAvConnectCfmSend(instData->appHandle,
                                  0,
                                  instData->con[i].remoteDevAddr,
                                  CSR_BT_RESULT_CODE_AV_CANCEL_CONNECT_ATTEMPT,
                                  CSR_BT_SUPPLIER_AV,
                                  CSR_BT_CONN_ID_INVALID);
            CsrBtAvClearConnection(instData,(CsrUint8) i);
        }
    }
#endif /* INSTALL_AV_CANCEL_CONNECT */

    /* must have been a rejected connection */
    if( !instData->isConnectable
        && (getNumActivations(instData->roleRegister) > getNumIncomingCon(instData) ) )
    {
        CsrBtAvMakeConnectable(instData);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCmL2caConnectCfmHandlerCleanup
 *
 *  DESCRIPTION
 *      Handle a CSR_BT_CM_L2CA_CONNECT_CFM in clean-up state
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCmL2caConnectCfmHandlerCleanup( av_instData_t *instData)
{
    CsrBtCmL2caConnectCfm *prim;
    CsrUintFast8 i, s;

    prim = (CsrBtCmL2caConnectCfm*) instData->recvMsgP;

    /* could be a terminating stream being opened, check it */
    for (s = 0; s < CSR_BT_AV_MAX_NUM_STREAMS; s++)
    {
        if (instData->stream[s].streamState == TERMINATE_OPENING_S )
        {
            if (prim->resultSupplier == CSR_BT_SUPPLIER_CM &&
                prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_STREAM_CHANNEL,prim->deviceAddr,prim->btConnId);
                /* immediately disconnect it, we are supposed to terminate it */
                CsrBtCml2caDisconnectReqSend(prim->btConnId);
            }
            CsrBtAvClearStream(instData,(CsrUint8) s);
            return;
        }
    }

    for (i = 0; i < CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        if (CsrBtBdAddrEq(&instData->con[i].remoteDevAddr, &prim->deviceAddr))
        {
            if (prim->resultSupplier == CSR_BT_SUPPLIER_CM &&
                prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL,prim->deviceAddr,prim->btConnId);
                /* immediately disconnect it, we are supposed to terminate it */
                CsrBtCml2caDisconnectReqSend(prim->btConnId);
            }
            CsrBtAvClearConnection(instData,(CsrUint8) i);
            break;
        }
    }

    CsrBtAvIsCleanUpFinished(instData);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCmL2caConnectAcceptCfmHandlerCleanup
 *
 *  DESCRIPTION
 *      Handle a CSR_BT_CM_L2CA_CONNECT_ACCEPT_CFM in clean-up state
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCmL2caConnectAcceptCfmHandlerCleanup( av_instData_t *instData)
{
    CsrUintFast8 s;
    CsrBtCmL2caConnectAcceptCfm *prim = (CsrBtCmL2caConnectAcceptCfm *) instData->recvMsgP;

    /* could be a terminating stream being opened, check it */
    for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
    {
        if( instData->stream[s].streamState == TERMINATE_PEER_OPENING_S )
        {
            CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_STREAM_CHANNEL,prim->deviceAddr,prim->btConnId);
            /* immediately disconnect it, we are supposed to terminate it */
            CsrBtCml2caDisconnectReqSend(prim->btConnId);
            CsrBtAvClearStream(instData,(CsrUint8) s);
            break;
        }

        CsrBtAvIsCleanUpFinished(instData);
        if (instData->state == READY_S)
        {
            return;
        }
    }

    /* then lets assume it is a new (device) signalling connection */
    instData->isConnectable = FALSE;
    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL,prim->deviceAddr,prim->btConnId);
        CsrBtCml2caDisconnectReqSend(prim->btConnId);
    }

    CsrBtAvIsCleanUpFinished(instData);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCmL2caCancelConnectAcceptCfmHandlerCleanup
 *
 *  DESCRIPTION
 *      Handle a CSR_BT_CM_L2CA_CANCEL_CONNECT_ACCEPT_CFM in clean-up state
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCmL2caCancelConnectAcceptCfmHandlerCleanup( av_instData_t *instData)
{
    CsrUintFast8 s;
    CsrBtCmL2caCancelConnectAcceptCfm *prim;

    prim = (CsrBtCmL2caCancelConnectAcceptCfm *) instData->recvMsgP;

    /* first check if any streams are doing cancel accept connect */
    for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
    {
        if(instData->stream[s].streamState == TERMINATE_PEER_OPENING_S)
        {
            if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
            {
                CsrBtAvClearStream(instData,(CsrUint8) s);
            }

            CsrBtAvIsCleanUpFinished(instData);
            return;
        }
    }

    instData->isConnectable = FALSE;

    CsrBtAvIsCleanUpFinished(instData);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCmL2caDisconnectIndHandlerCleanup
 *
 *  DESCRIPTION
 *      Handle a CSR_BT_CM_L2CA_DISCONNECT_IND in clean-up state
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCmL2caDisconnectIndHandlerCleanup( av_instData_t *instData)
{
    CsrUintFast8 i, s;
    CsrBtCmL2caDisconnectInd *prim;

    prim = (CsrBtCmL2caDisconnectInd*) instData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    if (!prim->localTerminated)
    {
        /* For remote disconnections, profile needs to respond to L2CA_DISCONNECT_IND. */
        CsrBtCmL2caDisconnectRspSend(prim->l2caSignalId, prim->btConnId);
    }
#endif

    for (s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
    {
        if( prim->btConnId == instData->stream[s].mediaCid)
        {
            CsrBtCmLogicalChannelTypeReqSend(CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL,
                              instData->con[instData->stream[s].conId].remoteDevAddr,prim->btConnId);
            instData->stream[s].mediaCid = 0;

            CsrBtAvClearStream(instData,(CsrUint8) s);
            CsrBtAvIsCleanUpFinished(instData);
            return;
        }
    }

    for (i=0; i<CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        if(prim->btConnId == instData->con[i].signalCid)
        {
            CsrBtCmLogicalChannelTypeReqSend(CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL,
                              instData->con[i].remoteDevAddr,prim->btConnId);
            CsrBtAvStatusIndSend(instData, CSR_BT_AV_DISCONNECT_IND, 0,(CsrUint8) i);
            CsrBtAvClearConnection(instData,(CsrUint8) i);
            break;
        }
    }

    CsrBtAvIsCleanUpFinished(instData);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCmL2caChannelInfoCfmHandler
 *
 *  DESCRIPTION
 *      Handles the response message from CM to a channel info request issued
 *      previously
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCmL2caChannelInfoCfmHandler(av_instData_t *instData)
{
    CsrBtCmL2caGetChannelInfoCfm *prim = (CsrBtCmL2caGetChannelInfoCfm *) instData->recvMsgP;
    CsrBtConnId btConnId = prim->btConnId;
    CsrUintFast8 index;

    if (prim->resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS)
    {/* this should not happen! */
#ifdef CSR_LOG_ENABLE
        CSR_LOG_TEXT_WARNING
        (
            (CsrBtAvLto, 0,
            "AV: CSR_BT_CM_L2CA_GET_CHANNEL_INFO_CFM failed for btConnId (%d)",
            btConnId)
        );
#endif /* CSR_LOG_ENABLE */
        return;
    }

    /* try and match with signalling cid to find the acl handle and remote l2cap cid */
    for (index=0; index<CSR_BT_AV_MAX_NUM_CONNECTIONS; index++)
    {
        if (btConnId == instData->con[index].signalCid)
        {/* attach acl handle to av connection instance */
            instData->con[index].aclHandle = prim->aclHandle;
            instData->con[index].remoteCid = prim->remoteCid;
#ifdef CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT
            if (instData->con[index].sigChannelInfoTimerId != 0)
            {/* cancel the timer and call the timeout handler */
                CsrSchedTimerCancel(instData->con[index].sigChannelInfoTimerId, NULL, NULL);
                csrBtAvGetSigChannelInfoTimeOutHandler((CsrUint16)index, instData);
            }
#endif
            return;
        }
    }

    /* try and match with media cid to find the acl handle and remote l2cap cid */
    for (index=0; index<CSR_BT_AV_MAX_NUM_STREAMS; index++)
    {
        if (btConnId == instData->stream[index].mediaCid)
        {/* attach acl handle to av connection instance */
            CsrUint8 conId = instData->stream[index].conId;

            instData->con[conId].aclHandle = prim->aclHandle;
            instData->stream[index].remoteCid = prim->remoteCid;

#ifdef CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT
            if (instData->stream[index].mediaChannelInfoTimerId != 0)
            {/* cancel the timer and call the timeout handler */
                CsrSchedTimerCancel(instData->stream[index].mediaChannelInfoTimerId, NULL, NULL);
                csrBtAvGetMediaChannelInfoTimeOutHandler((CsrUint16)index, instData);
            }

            if (instData->stream[index].mediaChannelInformationTimerId != 0)
            {/* cancel the timer and call the timeout handler */
                CsrSchedTimerCancel(instData->stream[index].mediaChannelInformationTimerId, NULL, NULL);
                getMediaChannelInfoTimeOutHandler((CsrUint16)index, instData);
            }
#endif /* CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT */

            return;
        }
    }

#ifdef CSR_LOG_ENABLE
    /* this should not happen! */
    CSR_LOG_TEXT_WARNING
    (
        (CsrBtAvLto,0,
         "AV: Could not locate btConnId (%d) reported in CSR_BT_CM_L2CA_GET_CHANNEL_INFO_CFM",
         btConnId)
    );
#endif /* CSR_LOG_ENABLE */
}

