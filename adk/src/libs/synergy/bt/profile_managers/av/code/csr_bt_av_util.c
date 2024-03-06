/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_pmem.h"
#include "sdc_prim.h"
#include "csr_bt_result.h"
#include "csr_bt_util.h"
#include "csr_bt_av_main.h"
#include "csr_bt_av_prim.h"
#include "csr_bt_av_lib.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cmn_sdc_rfc_util.h"
#include "csr_bt_cmn_sdr_tagbased_lib.h"
#include "csr_bt_cmn_sdp_lib.h"
#include "csr_bt_sdc_support.h"
#ifdef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_cm_private_lib.h"
#else
#include "csr_bt_sc_private_lib.h"
#endif

#ifdef CSR_LOG_ENABLE
#include "csr_log_text_2.h"
#endif /* CSR_LOG_ENABLE */

void CsrBtAvMessagePut(CsrSchedQid phandle, void *msg)
{
    CsrSchedMessagePut(phandle, CSR_BT_AV_PRIM, msg);
}

static void csrBtAvSdpDeInit(av_instData_t *instData)
{
    if (instData->sdpAvSearchData)
    {
        CsrBtUtilSdcRfcDeinit(&(instData->sdpAvSearchData));
        instData->sdpAvSearchData = NULL;
        /* Restore any previously queued SDC search request as the current one is finished.
         * The queuing is done in cases when there is an av connect request (which sends SDC search request)
         * at the time of an already ongoing SDC search */
        CsrBtAvSendHouseCleaning(instData);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvClearTxSignallingQ
 *
 *  DESCRIPTION
 *      Free memory hanging on Tx signalling queue
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void CsrBtAvClearTxSignallingQ(av_connection_info_t *con)
{
    while(con->qFirst != NULL)
    {
        q_element_t *tmp = con->qFirst;
        con->qFirst = con->qFirst->next;

        CsrPmemFree(tmp->data);
        CsrPmemFree(tmp);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvClearConnection
 *
 *  DESCRIPTION
 *      Clear/reset connection information
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvClearConnection(av_instData_t *instData, CsrUint8 conId)
{
    CsrUintFast8 s;

    if(instData->con[conId].timerId != 0)
    {
        CsrSchedTimerCancel(instData->con[conId].timerId, NULL, NULL);
        instData->con[conId].timerId = 0;
    }

#ifdef CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT
    if (instData->con[conId].sigChannelInfoTimerId != 0)
    {
        CsrSchedTimerCancel(instData->con[conId].sigChannelInfoTimerId, NULL, NULL);
        instData->con[conId].sigChannelInfoTimerId = 0;
    }
#endif /* CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT */

    /* streams depending on this connection needs cleaning too */
    for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
    {
        if(instData->stream[s].conId == conId)
        {
            CsrBtAvClearStream(instData, (CsrUint8)s);
        }
    }

    /* free any rx signalling message fragments stored internally */
    CsrBtAvFreeFragments( &instData->fragmentHeadPtr, instData->con[conId].signalCid);

    CsrBtAvClearTxSignallingQ( &instData->con[conId]);

    CsrMemSet(&instData->con[conId], 0, sizeof(av_connection_info_t));
    instData->con[conId].conState = DISCONNECTED_S;
    instData->con[conId].aclHandle = 0xFFFF;/* invalidate acl handle */
    instData->con[conId].remoteAVDTPVersion = DEFAULT_AVDTP_VERSION;
#ifdef CSR_TARGET_PRODUCT_VM
    instData->con[conId].useLargeMtu = FALSE;
#endif
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvClearStreamQ
 *
 *  DESCRIPTION
 *      Free memory hanging on stream queue
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
#ifndef CSR_STREAMS_ENABLE
void CsrBtAvClearStreamQ(av_stream_info_t *stream)
{
    CsrUintFast8 i;

    stream->sentBufFullInd = FALSE;

    for( i=0; i<CSR_BT_AV_MEDIA_BUFFER_SIZE; i++)
    {
        if( stream->fifoQ[i] != NULL)
        {
            CsrMblkDestroy(((CsrBtCmL2caDataReq *) stream->fifoQ[i])->payload);
            CsrPmemFree(stream->fifoQ[i]);
            stream->fifoQ[i] = NULL;
        }
    }
        stream->fifoQIn = 0;
        stream->fifoQOut = 0;
}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvStopStreamTimer
 *
 *  DESCRIPTION
 *      Stop signalling timer running for the stream

 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
CsrBool CsrBtAvStopStreamTimer(av_instData_t *instData, CsrUint8 s)
{
    if(instData->stream[s].timerId != 0)
    {
        CsrSchedTimerCancel(instData->stream[s].timerId, NULL, NULL);
        instData->stream[s].timerId = 0;
        return TRUE;
    }
    return FALSE;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvClearStream
 *
 *  DESCRIPTION
 *      Clear/reset stream information, disconnect/cancel connect accept
 *      depending on state
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvClearStream(av_instData_t *instData, CsrUint8 s)
{
    CsrBtAvStopStreamTimer(instData, s);
#ifdef CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT
    if (instData->stream[s].mediaChannelInfoTimerId != 0)
    {
        CsrSchedTimerCancel(instData->stream[s].mediaChannelInfoTimerId, NULL, NULL);
        instData->stream[s].mediaChannelInfoTimerId = 0;
    }

    if (instData->stream[s].mediaChannelInformationTimerId != 0)
    {
        CsrSchedTimerCancel(instData->stream[s].mediaChannelInformationTimerId, NULL, NULL);
        instData->stream[s].mediaChannelInformationTimerId = 0;
    }
#endif /* CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT */

#ifndef CSR_STREAMS_ENABLE
    CsrBtAvClearStreamQ(&instData->stream[s]);
#endif

    if( instData->stream[s].mediaCid != 0 )
    {
        if( instData->stream[s].streamState != PEER_ABORTING_S)
        {   /* Do not disconnect if remote is aborting */
            CsrBtCml2caDisconnectReqSend(instData->stream[s].mediaCid);
        }
    }
    else
    {
        if (instData->stream[s].streamState == PEER_OPENING_S)
        {
            CsrBtCmContextl2caCancelConnectAcceptReqSend(CSR_BT_AV_IFACEQUEUE,
                                                         CSR_BT_AVDTP_PSM,
                                                         CSR_BT_AV_STREAM_CONTEXT(s));
            instData->stream[s].streamState = TERMINATE_PEER_OPENING_S; /* Use this state even when handling an ABORT CMD */
        }
        else if ( instData->stream[s].streamState == OPENING_S)
        {
            CsrBtCml2caCancelConnectReqSend(CSR_BT_AV_IFACEQUEUE,
                                            instData->con[instData->stream[s].conId].remoteDevAddr,
                                            CSR_BT_AVDTP_PSM);
            
            instData->stream[s].streamState = TERMINATE_OPENING_S;
        }
        else
        {
            /* time to recycle the stream info */
            if (CSR_BT_A2DP_BIT_RATE_STREAM_CLOSED != instData->stream[s].bitRate)
            {
                CsrBtCmA2dpBitRateReqSend(instData->con[instData->stream[s].conId].remoteDevAddr,s,CSR_BT_A2DP_BIT_RATE_STREAM_CLOSED);
            }
            
            CsrMemSet(&instData->stream[s], 0, sizeof(av_stream_info_t));
            instData->stream[s].tLabel                  = 0xFF;
            instData->stream[s].streamState             = IDLE_S;
            instData->stream[s].bitRate  = CSR_BT_A2DP_BIT_RATE_STREAM_CLOSED;
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvIsCleanUpFinished
 *
 *  DESCRIPTION
 *      Check if a cleanup has been completed, enter READY state when done
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvIsCleanUpFinished(av_instData_t *instData)
{
    CsrUintFast8 i;

    for( i=0; i<4; i++)
    {
        if( instData->serviceRecordHandles[i] != 0 )
        {
            return;
        }
    }

    for(i=0; i<CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        if(instData->con[i].conState != DISCONNECTED_S)
        {
            return;
        }
    }

    /* if reaching here, then everything should have been cleaned,
       - start providing service again */
    instData->state = READY_S;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSendHouseCleaning
 *
 *  DESCRIPTION
 *      Send a 'house cleaning' message to AV itself
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvSendHouseCleaning(av_instData_t *instData)
{
    CsrBtAvHouseCleaning    *prim;

    prim = (CsrBtAvHouseCleaning *) CsrPmemAlloc(sizeof(CsrBtAvHouseCleaning));
    prim->type = CSR_BT_AV_HOUSE_CLEANING;
    instData->restoreFlag = TRUE;
    CsrBtAvMessagePut(CSR_BT_AV_IFACEQUEUE, prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSaveMessage
 *
 *  DESCRIPTION
 *      Put a signal on the save queue
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvSaveMessage(av_instData_t *instData)
{
    CsrMessageQueuePush(&instData->saveQueue, CSR_BT_AV_PRIM, instData->recvMsgP);
    instData->recvMsgP = NULL;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      AvSendPendingUpstreamMessages
 *
 *  DESCRIPTION
 *      Pop the required stored upstream message from the save queue and
 *      send it to the application
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void AvSendPendingUpstreamMessages(av_instData_t *instData, CsrBtAvPrim primType)
{
    CsrUint16                eventClass;
    void                    *pMsg;
    CsrMessageQueueType     *tempQueue  = NULL;

    while(CsrMessageQueuePop(&instData->saveQueue, &eventClass, &pMsg))
    {
        CsrBtAvPrim *msgPrimType = (CsrBtAvPrim *)pMsg;

        if (*msgPrimType == primType)
        {
            switch(primType)
            {
                case CSR_BT_AV_CLOSE_IND:
                {
                    CsrBtAvCloseInd *prim = (CsrBtAvCloseInd*) pMsg;
                    PUT_PRIM_TO_APP(prim);
                    break;
                }
                case CSR_BT_AV_START_IND:
                {
                    CsrBtAvStartInd *prim = (CsrBtAvStartInd*) pMsg;
                    PUT_PRIM_TO_APP(prim);
                    break;
                }
                default:
                {
                    CsrMessageQueuePush(&tempQueue, eventClass, pMsg);
                    break;
                }
            }
        }
        else
        {
            CsrMessageQueuePush(&tempQueue, eventClass, pMsg);
        }
    }

    instData->saveQueue = tempQueue;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      getNumActivations
 *
 *  DESCRIPTION
 *      Find the total number of current activations
 *
 *  RETURNS
 *      Number of successful activations made by the application
 *
 *---------------------------------------------------------------------------*/
CsrUint8 getNumActivations(CsrUint8 *roleRegister)
{
    return (roleRegister[0] + roleRegister[1] + roleRegister[2] + roleRegister[3]);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      getNumIncomingCon
 *
 *  DESCRIPTION
 *      Finds current number of incoming connections(initiated by peer device)
 *
 *  RETURNS
 *      Number of current incoming connections
 *
 *---------------------------------------------------------------------------*/
CsrUint8 getNumIncomingCon(av_instData_t *instData)
{
    CsrUintFast8 i, t;

    t = 0;

    for(i=0; i<CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        if( (instData->con[i].incoming == TRUE) && (instData->con[i].conState != DISCONNECTED_S))
        {
            t++;
        }
    }

    return (CsrUint8)t;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvMakeConnectable
 *
 *  DESCRIPTION
 *      Send a CSR_BT_CM_L2CA_CONNECT_ACCEPT_REQ based on registered services, making
 *      AV connectable
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvMakeConnectable(av_instData_t *instData)
{
    CsrBtClassOfDevice cod = 0;
    uuid16_t        uuid;
    dm_security_level_t secIncoming;

    if (instData->serviceRecordHandles[CSR_BT_AV_AUDIO_SOURCE] || instData->serviceRecordHandles[CSR_BT_AV_VIDEO_SOURCE])
    {
        cod |= CSR_BT_CAPTURING_MAJOR_SERVICE_MASK;
    }

    if (instData->serviceRecordHandles[CSR_BT_AV_AUDIO_SINK] || instData->serviceRecordHandles[CSR_BT_AV_VIDEO_SINK])
    {
        cod |= CSR_BT_RENDERING_MAJOR_SERVICE_MASK;
    }

    if (instData->serviceRecordHandles[CSR_BT_AV_AUDIO_SOURCE] || instData->serviceRecordHandles[CSR_BT_AV_AUDIO_SINK])
    {
        uuid = CSR_BT_ADVANCED_AUDIO_PROFILE_UUID;
    }
    else
    {/* Only VDP is supported */
        uuid = CSR_BT_VIDEO_DISTRIBUTION_UUID;
    }

#ifndef INSTALL_AV_CUSTOM_SECURITY_SETTINGS
    CsrBtScSetSecInLevel(&secIncoming,
                         CSR_BT_SEC_DEFAULT,
                         CSR_BT_AV_MANDATORY_SECURITY_INCOMING,
                         CSR_BT_AV_DEFAULT_SECURITY_INCOMING,
                         CSR_BT_RESULT_CODE_AV_SUCCESS,
                         CSR_BT_RESULT_CODE_AV_UNACCEPTABLE_PARAMETER);
#else
    secIncoming = instData->secIncoming;
#endif /* INSTALL_AV_CUSTOM_SECURITY_SETTINGS */

    CsrBtCml2caConnectAcceptReqSend(CSR_BT_AV_IFACEQUEUE,
                                    CSR_BT_AVDTP_PSM,
                                    cod,
                                    secIncoming,
                                    CSR_BT_AV_PROFILE_DEFAULT_MTU_SIZE,
                                    L2CA_FLUSH_TO_DEFAULT,
                                    NULL,
                                    uuid,
                                    CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                           CSR_BT_AV_DEFAULT_ENC_KEY_SIZE_VAL));

    instData->isConnectable = TRUE;
}

#ifdef INSTALL_AV_STREAM_DATA_APP_SUPPORT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvRegisterStreamHandleCfmSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvRegisterStreamHandleCfmSend(CsrSchedQid handle)
{
    CsrBtAvRegisterStreamHandleCfm *prim;

    prim        = (CsrBtAvRegisterStreamHandleCfm*)CsrPmemAlloc(sizeof(CsrBtAvRegisterStreamHandleCfm));
    prim->type  = CSR_BT_AV_REGISTER_STREAM_HANDLE_CFM;
    CsrBtAvMessagePut(handle, prim);
}
#endif /* INSTALL_AV_STREAM_DATA_APP_SUPPORT */


#ifdef INSTALL_AV_CUSTOM_SECURITY_SETTINGS
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSecurityInCfmSend
 *
 *  DESCRIPTION
 *      Send confirm on security changes
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrBtAvSecurityInCfmSend(CsrSchedQid appHandle, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier)
{
    CsrBtAvSecurityInCfm *prim;
    prim = (CsrBtAvSecurityInCfm*)CsrPmemAlloc(sizeof(CsrBtAvSecurityInCfm));
    prim->type = CSR_BT_AV_SECURITY_IN_CFM;
    prim->resultCode = resultCode;
    prim->resultSupplier = resultSupplier;
    CsrBtAvMessagePut(appHandle, prim);
}

void CsrBtAvSecurityOutCfmSend(CsrSchedQid appHandle, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier)
{
    CsrBtAvSecurityOutCfm *prim;
    prim = (CsrBtAvSecurityOutCfm*)CsrPmemAlloc(sizeof(CsrBtAvSecurityOutCfm));
    prim->type = CSR_BT_AV_SECURITY_OUT_CFM;
    prim->resultCode = resultCode;
    prim->resultSupplier = resultSupplier;
    CsrBtAvMessagePut(appHandle, prim);
}
#endif /* INSTALL_AV_CUSTOM_SECURITY_SETTINGS */
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvActivateCfmSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvActivateCfmSend(CsrSchedQid handle,
    CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier)
{
    CsrBtAvActivateCfm *prim;

    prim            = (CsrBtAvActivateCfm *) CsrPmemAlloc(sizeof(CsrBtAvActivateCfm));
    prim->type      = CSR_BT_AV_ACTIVATE_CFM;
    prim->avResultCode     = resultCode;
    prim->avResultSupplier = resultSupplier;

    CsrBtAvMessagePut(handle, prim);
}

#ifdef INSTALL_AV_DEACTIVATE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDeactivateCfmSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvDeactivateCfmSend(CsrSchedQid handle, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier)
{
    CsrBtAvDeactivateCfm        *prim;

    prim            = (CsrBtAvDeactivateCfm *) CsrPmemAlloc(sizeof(CsrBtAvDeactivateCfm));
    prim->type      = CSR_BT_AV_DEACTIVATE_CFM;
    prim->avResultCode     = resultCode;
    prim->avResultSupplier = resultSupplier;
    CsrBtAvMessagePut(handle, prim);
}
#endif /* INSTALL_AV_DEACTIVATE */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvStatusIndSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *----------------------------------------------------------------------------*/
void CsrBtAvStatusIndSend(av_instData_t *instData, CsrBtAvPrim signalId, CsrBtAvRole role, CsrUint8 conId)
{
#ifdef INSTALL_AV_STREAM_DATA_APP_SUPPORT
    if((instData->streamAppHandle != CSR_SCHED_QID_INVALID) &&
       (instData->streamAppHandle != instData->appHandle))
    {
        CsrBtAvStatusInd *prim;

        prim                = (CsrBtAvStatusInd *) CsrPmemAlloc(sizeof(CsrBtAvStatusInd));
        prim->type          = CSR_BT_AV_STATUS_IND;
        prim->connectionId  = conId;
        prim->statusType    = signalId;
        prim->roleType      = role;
        prim->appHandle     = instData->appHandle;

        CsrBtAvMessagePut(instData->streamAppHandle, prim);
    }
#else
    CSR_UNUSED(instData);
    CSR_UNUSED(signalId);
    CSR_UNUSED(role);
    CSR_UNUSED(conId);
#endif /* INSTALL_AV_STREAM_DATA_APP_SUPPORT */
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvConnectCfmSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvConnectCfmSend(CsrSchedQid handle,
                           CsrUint8 connectionId,
                           CsrBtDeviceAddr devAddr,
                           CsrBtResultCode resultCode,
                           CsrBtSupplier resultSupplier,
                           CsrBtConnId btConnId)
{
    CsrBtAvConnectCfm *prim;

    prim                = (CsrBtAvConnectCfm *) CsrPmemAlloc(sizeof(CsrBtAvConnectCfm));
    prim->type          = CSR_BT_AV_CONNECT_CFM;
    prim->avResultCode     = resultCode;
    prim->avResultSupplier = resultSupplier;
    prim->connectionId  = connectionId;
    prim->deviceAddr    = devAddr;
    prim->btConnId      = btConnId;

    CsrBtAvMessagePut(handle, prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvConnectIndSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvConnectIndSend(CsrSchedQid handle,
                           CsrUint8 connectionId,
                           CsrBtDeviceAddr devAddr,
                           CsrBtConnId btConnId)
{
    CsrBtAvConnectInd *prim;

    prim                = (CsrBtAvConnectInd *) CsrPmemAlloc(sizeof(CsrBtAvConnectInd));
    prim->type          = CSR_BT_AV_CONNECT_IND;
    prim->connectionId  = connectionId;
    prim->deviceAddr    = devAddr;
    prim->btConnId      = btConnId;

    CsrBtAvMessagePut(handle, prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDisconnectIndSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvDisconnectIndSend(CsrSchedQid handle, CsrUint8 connectionId,
    CsrBool localTerminated,
    CsrBtReasonCode reasonCode,
    CsrBtSupplier reasonSupplier)
{
    CsrBtAvDisconnectInd *prim;

    prim                = (CsrBtAvDisconnectInd *) CsrPmemAlloc(sizeof(CsrBtAvDisconnectInd));
    prim->type          = CSR_BT_AV_DISCONNECT_IND;
    prim->connectionId  = connectionId;
    prim->localTerminated = localTerminated;
    prim->reasonCode      = reasonCode;
    prim->reasonSupplier  = reasonSupplier;

    CsrBtAvMessagePut(handle, prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvStreamMtuSizeIndSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvStreamMtuSizeIndSend(av_instData_t *instData, CsrUint8 shandle, CsrBtConnId btConnId)
{
    CsrBtAvStreamMtuSizeInd *prim;

    prim                = (CsrBtAvStreamMtuSizeInd *) CsrPmemAlloc(sizeof(CsrBtAvStreamMtuSizeInd));
    prim->type          = CSR_BT_AV_STREAM_MTU_SIZE_IND;
    prim->shandle       = shandle;
    prim->remoteMtuSize = instData->stream[shandle].mediaMtu;
    prim->btConnId      = btConnId;

    PUT_PRIM_TO_APP(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDiscoverCfmSendReject
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvDiscoverCfmSendReject(av_instData_t *instData, CsrUint8 conId, CsrUint8 label,
    CsrBtResultCode avResultCode, CsrBtSupplier avResultSupplier)
{
    CsrBtAvDiscoverCfm *prim;

    prim                = (CsrBtAvDiscoverCfm *) CsrPmemAlloc(sizeof(CsrBtAvDiscoverCfm));
    prim->type          = CSR_BT_AV_DISCOVER_CFM;
    prim->connectionId  = conId;
    prim->tLabel        = label;
    prim->avResultCode     = avResultCode;
    prim->avResultSupplier = avResultSupplier;
    prim->seidInfoCount = 0;
    prim->seidInfo      = NULL;

    PUT_PRIM_TO_APP(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetCapabilitiesCfmSendReject
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvGetCapabilitiesCfmSendReject(av_instData_t *instData, CsrUint8 conId, CsrUint8 label,
    CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtAvGetCapabilitiesCfm *prim;

    prim                = (CsrBtAvGetCapabilitiesCfm *) CsrPmemAlloc(sizeof(CsrBtAvGetCapabilitiesCfm));
    prim->type          = CSR_BT_AV_GET_CAPABILITIES_CFM;
    prim->connectionId  = conId;
    prim->tLabel        = label;
    prim->avResultCode     = resultCode;
    prim->avResultSupplier = resultSupplier;
    prim->servCapLen    = 0;
    prim->servCapData   = NULL;

    PUT_PRIM_TO_APP(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSetConfigurationCfmSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvSetConfigurationCfmSend(av_instData_t *instData,
                               CsrUint8 conId,
                               CsrUint8 label,
                               CsrUint8 shandle,
                               CsrUint8 servCategory,
                               CsrBtResultCode resultCode,
                               CsrBtSupplier resultSupplier)
{
    CsrBtAvSetConfigurationCfm *prim;

    prim                = (CsrBtAvSetConfigurationCfm *) CsrPmemAlloc(sizeof(CsrBtAvSetConfigurationCfm));
    prim->type          = CSR_BT_AV_SET_CONFIGURATION_CFM;
    prim->connectionId  = conId;
    prim->tLabel        = label;
    prim->avResultCode     = resultCode;
    prim->avResultSupplier = resultSupplier;
    prim->shandle       = shandle;
    prim->servCategory  = servCategory;

    PUT_PRIM_TO_APP(prim);
}

#ifdef INSTALL_AV_GET_CONFIGURATION
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetConfigurationCfmSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvGetConfigurationCfmSend(av_instData_t *instData, CsrUint8 label, CsrUint8 shandle, CsrUint16 servCapLen, CsrUint8 *servCapData,
    CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtAvGetConfigurationCfm *prim;

    prim                = (CsrBtAvGetConfigurationCfm *) CsrPmemAlloc(sizeof(CsrBtAvGetConfigurationCfm));
    prim->type          = CSR_BT_AV_GET_CONFIGURATION_CFM;
    prim->shandle       = shandle;
    prim->tLabel        = label;
    prim->servCapLen    = servCapLen;
    prim->servCapData   = servCapData;
    prim->avResultCode     = resultCode;
    prim->avResultSupplier = resultSupplier;

    PUT_PRIM_TO_APP(prim);
}
#endif /* INSTALL_AV_GET_CONFIGURATION */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvReconfigureCfmSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvReconfigureCfmSend(av_instData_t *instData, CsrUint8 label, CsrUint8 shandle, CsrUint8 servCategory,
    CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier)
{
    CsrBtAvReconfigureCfm *prim;

    prim                = (CsrBtAvReconfigureCfm *) CsrPmemAlloc(sizeof(CsrBtAvReconfigureCfm));
    prim->type          = CSR_BT_AV_RECONFIGURE_CFM;
    prim->shandle       = shandle;
    prim->tLabel        = label;
    prim->avResultCode     = resultCode;
    prim->avResultSupplier = resultSupplier;
    prim->servCategory  = servCategory;

    PUT_PRIM_TO_APP(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvOpenCfmSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvOpenCfmSend(av_instData_t *instData, CsrUint8 shandle, CsrUint8 label,
    CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtAvOpenCfm *prim;

    prim                = (CsrBtAvOpenCfm *) CsrPmemAlloc(sizeof(CsrBtAvOpenCfm));
    prim->type          = CSR_BT_AV_OPEN_CFM;
    prim->shandle       = shandle;
    prim->tLabel        = label;
    prim->avResultCode     = resultCode;
    prim->avResultSupplier = resultSupplier;

    PUT_PRIM_TO_APP(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvStartCfmSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvStartCfmSend(av_instData_t *instData,
                    CsrUint8 shandle,
                    CsrUint8 label,
                    CsrBtResultCode resultCode,
                    CsrBtSupplier resultSupplier)
{
    CsrBtAvStartCfm *prim;

    prim                   = (CsrBtAvStartCfm *) CsrPmemAlloc(sizeof(CsrBtAvStartCfm));
    prim->type             = CSR_BT_AV_START_CFM;
    if (shandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        prim->connectionId = instData->stream[shandle].conId;
    }
    else
    {
        prim->connectionId = 0;
    }
    prim->tLabel           = label;
    prim->avResultCode     = resultCode;
    prim->avResultSupplier = resultSupplier;
    prim->reject_shandle = shandle;

    PUT_PRIM_TO_APP(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCloseCfmSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCloseCfmSend(av_instData_t *instData,
                    CsrUint8 shandle,
                    CsrUint8 label,
                    CsrBtResultCode resultCode,
                    CsrBtSupplier resultSupplier)
{
    CsrBtAvCloseCfm *prim;

    prim                = (CsrBtAvCloseCfm *) CsrPmemAlloc(sizeof(CsrBtAvCloseCfm));
    prim->type          = CSR_BT_AV_CLOSE_CFM;
    prim->tLabel        = label;
    prim->avResultCode     = resultCode;
    prim->avResultSupplier = resultSupplier;
    prim->shandle       = shandle;

    PUT_PRIM_TO_APP(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      AvCloseIndSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void AvCloseIndSend(av_instData_t *instData, CsrUint8 shandle, CsrUint8 label)
{
    CsrBtAvCloseInd *prim;

    prim                = (CsrBtAvCloseInd *) CsrPmemAlloc(sizeof(CsrBtAvCloseInd));
    prim->type          = CSR_BT_AV_CLOSE_IND;
    prim->shandle       = shandle;
    prim->tLabel        = label;
    PUT_PRIM_TO_APP(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSuspendCfmSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvSuspendCfmSend(av_instData_t *instData,
                      CsrUint8 shandle,
                      CsrUint8 label,
                      CsrBtResultCode resultCode,
                      CsrBtSupplier resultSupplier)
{
    CsrBtAvSuspendCfm *prim;

    prim                    = (CsrBtAvSuspendCfm *) CsrPmemAlloc(sizeof(CsrBtAvSuspendCfm));
    prim->type              = CSR_BT_AV_SUSPEND_CFM;
    if (shandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        prim->connectionId   = instData->stream[shandle].conId;
    }
    else
    {
        prim->connectionId   = 0;
    }
    prim->tLabel         = label;
    prim->reject_shandle = shandle;
    prim->avResultCode   = resultCode;
    prim->avResultSupplier = resultSupplier;

    PUT_PRIM_TO_APP(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvAbortCfmSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvAbortCfmSend(av_instData_t *instData, CsrUint8 shandle, CsrUint8 label)
{
    CsrBtAvAbortCfm *prim;

    prim                = (CsrBtAvAbortCfm *) CsrPmemAlloc(sizeof(CsrBtAvAbortCfm));
    prim->type          = CSR_BT_AV_ABORT_CFM;
    prim->shandle       = shandle;
    prim->tLabel        = label;

    PUT_PRIM_TO_APP(prim);
}

#ifdef INSTALL_AV_SECURITY_CONTROL
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSecurityControlCfmSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvSecurityControlCfmSend(av_instData_t *instData,
                              CsrUint8 shandle,
                              CsrUint8 label,
                              CsrUint16 length,
                              CsrUint8 *data,
                              CsrBtResultCode resultCode,
                              CsrBtSupplier resultSupplier)
{
    CsrBtAvSecurityControlCfm *prim;

    prim                = (CsrBtAvSecurityControlCfm *) CsrPmemAlloc(sizeof(CsrBtAvSecurityControlCfm));
    prim->type          = CSR_BT_AV_SECURITY_CONTROL_CFM;
    prim->tLabel        = label;
    prim->avResultCode     = resultCode;
    prim->avResultSupplier = resultSupplier;
    prim->shandle       = shandle;
    prim->contProtMethodLen = length;
    prim->contProtMethodData = data;

    PUT_PRIM_TO_APP(prim);
}
#endif /* INSTALL_AV_SECURITY_CONTROL */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDelayReportCfmSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvDelayReportCfmSend(av_instData_t *instData,
                                CsrUint8 shandle,
                                CsrUint8 label,
                                CsrBtResultCode resultCode,
                                CsrBtSupplier resultSupplier)
{
    CsrBtAvDelayReportCfm *prim;

    prim                = (CsrBtAvDelayReportCfm *) CsrPmemAlloc(sizeof(CsrBtAvDelayReportCfm));
    prim->type          = CSR_BT_AV_DELAY_REPORT_CFM;
    prim->tLabel        = label;
    prim->avResultCode     = resultCode;
    prim->avResultSupplier = resultSupplier;
    prim->shandle       = shandle;

    PUT_PRIM_TO_APP(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvQosIndSend
 *
 *  DESCRIPTION
 *      Build and send primitive
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
#ifndef CSR_STREAMS_ENABLE
void CsrBtAvQosIndSend(av_instData_t *instData, CsrUint8 shandle, CsrUint16 bufferStatus)
{
    CsrBtAvQosInd * prim;

    prim = (CsrBtAvQosInd *) CsrPmemAlloc(sizeof(CsrBtAvQosInd));
    prim->type = CSR_BT_AV_QOS_IND;
    prim->shandle = shandle;
    prim->bufferStatus = bufferStatus;

    PUT_PRIM_TO_APP(prim);
}
#endif

#ifdef CSR_BT_INSTALL_AV_SET_QOS_INTERVAL
void CsrBtAvSetQosIntervalReqHandler(av_instData_t *instData)
{
    CsrBtAvSetQosIntervalReq *prim;

    prim = (CsrBtAvSetQosIntervalReq *) instData->recvMsgP;
    instData->qosInterval = prim->qosInterval;
}
#endif

static CsrBool csrBtAvSdpGetRemoteAvdtpVersion( CmnCsrBtLinkedListStruct *sdpTagList,
                                                CsrUint16  serviceHandleIndex,
                                                CsrUint16  *version)
{
    CsrUint16    tmp;
    CsrUint32    uuid, value, protocol;
    CsrUint16    dummy1, dummy2;

    if (CsrBtUtilSdrGetServiceUuid32AndResult(sdpTagList,
            serviceHandleIndex,
            &uuid,
            &tmp,
            &dummy1,
            &dummy2))
    {
        if (CSR_BT_RESULT_CODE_CM_SUCCESS == tmp)
        {
            CsrUint16 attDataLen, emptyAttSize, consumedBytes, index = 0;
            CsrUint8 *att_p = CsrBtUtilSdrGetAttributePointer(sdpTagList, serviceHandleIndex, 0, &dummy1);

            if (att_p)
            {
                CsrBtUtilSdrGetEmptyAttributeSize(&emptyAttSize);
                SynMemCpyS(&tmp, SDR_ENTRY_SIZE_SERVICE_UINT16, att_p, SDR_ENTRY_SIZE_SERVICE_UINT16);
                attDataLen = tmp - emptyAttSize + SDR_ENTRY_SIZE_TAG_LENGTH;

                if ( CsrBtUtilSdpExtractUint(att_p + SDR_ENTRY_INDEX_ATTRIBUTE_DATA + index,
                     attDataLen,
                     &value,
                     &consumedBytes,
                     TRUE))
                {
                    attDataLen -= consumedBytes;
                    index += consumedBytes;

                    while( attDataLen > 0 )
                    {
                        if ( CsrBtUtilSdpExtractUint(att_p + SDR_ENTRY_INDEX_ATTRIBUTE_DATA + index,
                             attDataLen,
                             &protocol,
                             &consumedBytes,
                             TRUE))
                        {
                            attDataLen -= consumedBytes;
                            index += consumedBytes;

                            if( CsrBtUtilSdpExtractUint(att_p + SDR_ENTRY_INDEX_ATTRIBUTE_DATA + index,
                                    attDataLen,
                                    &value,
                                    &consumedBytes,
                                    TRUE))
                            {
                                attDataLen -= consumedBytes;
                                index += consumedBytes;

                                /* Now find the value if it is AVDTP protocol UUID */
                                if ( protocol == CSR_BT_PROTOCOL_AVDTP_UUID )
                                {
                                     *version = (CsrUint16)value;
                                     return TRUE;
                                }
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
            else
            {
                return FALSE;
            }
        }
    }
    return FALSE;
}

static void csrBtAvSearchResultHandler(CsrSdcOptCallbackType cbType, void *context)
{
    if(cbType == CSR_SDC_OPT_CB_SEARCH_RESULT)
    {
        CsrSdcResultFuncType *params = (CsrSdcResultFuncType *)context;

        CsrBtAvSdcResultHandler(params->instData,
                                params->sdpTagList,
                                params->deviceAddr,
                                params->resultCode,
                                params->resultSupplier);
    }
}


void CsrBtAvSdcStartRemoteVersionSearch(av_instData_t *instData, CsrBtDeviceAddr deviceAddr)
{
    CmnCsrBtLinkedListStruct *sdpTagList = NULL;
    CsrUint16 shIndex;

    if( instData->roleRegister[CSR_BT_AV_AUDIO_SOURCE] )
    {
        sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid32(sdpTagList, CSR_BT_AUDIO_SINK_UUID, &shIndex);
        CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER, NULL, 0);
    }
    if( instData->roleRegister[CSR_BT_AV_AUDIO_SINK] )
    {
        sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid32(sdpTagList, CSR_BT_AUDIO_SOURCE_UUID, &shIndex);
        CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER, NULL, 0);
    }
    if( instData->roleRegister[CSR_BT_AV_VIDEO_SOURCE] )
    {
        sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid32(sdpTagList, CSR_BT_VIDEO_SINK_UUID, &shIndex);
        CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER, NULL, 0);
    }
    if( instData->roleRegister[CSR_BT_AV_VIDEO_SINK] )
    {
        sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid32(sdpTagList, CSR_BT_VIDEO_SOURCE_UUID, &shIndex);
        CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER, NULL, 0);
    }

    if(NULL == instData->sdpAvSearchData)
    {
        instData->sdpAvSearchData = CsrBtUtilSdcInit(csrBtAvSearchResultHandler, CSR_BT_AV_IFACEQUEUE);
    }

    instData->searchOngoing = TRUE;
    CsrBtUtilSdcSearchStart((void *)instData, instData->sdpAvSearchData, sdpTagList, deviceAddr);
}

void CsrBtAvSdcResultHandler(void                     * inst,
                             CmnCsrBtLinkedListStruct * sdpTagList,
                             CsrBtDeviceAddr          deviceAddr,
                             CsrBtResultCode          resultCode,
                             CsrBtSupplier            resultSupplier)
{
    av_instData_t *instData = (av_instData_t *) inst;
    av_connection_info_t    *con = NULL;
    CsrUint8  i = 0;
    CsrBool found = FALSE;

    instData->searchOngoing = FALSE;

    for (i = 0; i < CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        con = &(instData->con[i]);
        if (!CsrMemCmp(&con->remoteDevAddr,&deviceAddr, sizeof(CsrBtDeviceAddr)))
        {/* Found*/
            found = TRUE;
            break;
        }
    }

    if (found)
    {
        CsrBtResultCode result = CSR_BT_RESULT_CODE_AV_FAILED;
        CsrBool confirm, clear;

        confirm = clear = FALSE;
#ifdef INSTALL_AV_CANCEL_CONNECT
        if (instData->con[i].conState == CANCEL_CONNECTING_S)
        {/* Except if we cancelled the connection attempt. */
            clear = confirm = TRUE;
            result = CSR_BT_RESULT_CODE_AV_CANCEL_CONNECT_ATTEMPT;
        }
        else if(instData->con[i].conState == CONNECTING_S)
#else
        if (instData->con[i].conState == CONNECTING_S)
#endif /* INSTALL_AV_CANCEL_CONNECT */
        {/* local initiated connection, proceed with connection establishment */
            if (resultCode     == CSR_BT_RESULT_CODE_CM_SUCCESS &&
                resultSupplier == CSR_BT_SUPPLIER_CM)
            {
                if (sdpTagList != NULL)
                {
                    dm_security_level_t secOutgoing;

#ifndef INSTALL_AV_CUSTOM_SECURITY_SETTINGS
                    CsrBtScSetSecOutLevel(&secOutgoing,
                                          CSR_BT_SEC_DEFAULT,
                                          CSR_BT_AV_MANDATORY_SECURITY_OUTGOING,
                                          CSR_BT_AV_DEFAULT_SECURITY_OUTGOING,
                                          CSR_BT_RESULT_CODE_AV_SUCCESS,
                                          CSR_BT_RESULT_CODE_AV_UNACCEPTABLE_PARAMETER);
#else
                    secOutgoing = instData->secOutgoing;
#endif /* INSTALL_AV_CUSTOM_SECURITY_SETTINGS */

                    CsrBtCml2caConnectReqSend(CSR_BT_AV_IFACEQUEUE,
                                         deviceAddr,
                                         CSR_BT_AVDTP_PSM,
                                         CSR_BT_AVDTP_PSM,
                                         CSR_BT_AV_PROFILE_DEFAULT_MTU_SIZE,
                                         secOutgoing,
                                         CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                                CSR_BT_AV_DEFAULT_ENC_KEY_SIZE_VAL));
                }
                else
                { /* SDP was successful but no valid SDP record found */
                    clear = confirm = TRUE;
                    result = CSR_BT_RESULT_CODE_AV_SDC_SEARCH_FAILED;
                }
            }
            else
            {
                clear = confirm = TRUE;

                if (resultCode == SDC_NO_RESPONSE_DATA && (resultSupplier == CSR_BT_SUPPLIER_SDP_SDC || resultSupplier == CSR_BT_SUPPLIER_SDP_SDC_OPEN_SEARCH))
                {
                    result = CSR_BT_RESULT_CODE_AV_SDC_SEARCH_FAILED;
                }
                else
                {
                    result = CSR_BT_RESULT_CODE_AV_FAILED;
                }
            }
        }

        if (resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
            resultSupplier == CSR_BT_SUPPLIER_CM  &&
            sdpTagList != NULL)
        {
            CsrUintFast16 numOfSdpRecords = CsrBtUtilBllGetNofEntriesEx(sdpTagList);
            CsrUintFast16 sdpRecordIndex;
            CsrUint16 tmpResult;
            CsrUint16 dummy1, dummy2; /* Currently CSR_UNUSED */
            CsrUint16 version = 0;
            CsrBtUuid32 tmpUuid = 0;

            for (sdpRecordIndex = 0; sdpRecordIndex < numOfSdpRecords; sdpRecordIndex++)
            {
                if (CsrBtUtilSdrGetServiceUuid32AndResult(sdpTagList,
                                                sdpRecordIndex,
                                                &tmpUuid,
                                                &tmpResult,
                                                &dummy1,
                                                &dummy2))
                {
                    if (tmpResult == SDR_SDC_SEARCH_SUCCESS)
                    {
                        if ((tmpUuid == CSR_BT_AUDIO_SOURCE_UUID) || (tmpUuid == CSR_BT_AUDIO_SINK_UUID) ||
                            (tmpUuid == CSR_BT_VIDEO_SOURCE_UUID) || (tmpUuid == CSR_BT_VIDEO_SINK_UUID))
                        { 
                            if (TRUE == csrBtAvSdpGetRemoteAvdtpVersion(sdpTagList, (CsrUint16)sdpRecordIndex, &version))
                            {
                                con->remoteAVDTPVersion = version;

                                /* workaround to avoid IOP issues with a larger range of SE handsets
                                   incorrectly reporting AVDTP version 10.0 instead of 1.0
                                   (K800x, K610x, K618x, V630x, K790x, W712x, Z712x, W710x, Z710x,
                                   K610im, Z610x, W850x, W830x, W880x, W888x, K550x, W610, W660x,
                                   K810x, K818x, K530x, S500x, W580x, T650x, T658x, K770x) */
                                if ( version == 0x1000 )
                                {
                                    con->remoteAVDTPVersion = 0x0100;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (clear)
        {/* clear/reset connection information */
            CsrBtAvClearConnection(instData, (CsrUint8)i);
        }

        if (confirm)
        {/* send confirmation to application */
            CsrBtAvConnectCfmSend(instData->appHandle,
                                  0, deviceAddr,
                                  result,
                                  CSR_BT_SUPPLIER_AV,
                                  CSR_BT_CONN_ID_INVALID);
        }
    }
    else
    {
#ifdef CSR_LOG_ENABLE
        CSR_LOG_TEXT_WARNING((CsrBtAvLto, 0, "AV: Remote device not found!"));
#endif /* CSR_LOG_ENABLE */
    }

    csrBtAvSdpDeInit(instData);
    CsrBtUtilBllFreeLinkedList(&sdpTagList, CsrBtUtilBllPfreeWrapper);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvCalculateStreamBitRate
 *
 *  DESCRIPTION
 *      calculates the bit rate to use for the stream connection and returns
 *      the result or the "Unknown" value if not possible to calculate.
 *      Indicates the bit rate to the CM that will propagate the value to
 *      any application that subscribes to that information.
 *
 *  RETURNS
 *      CsrUint32 bit rate
 *
 *---------------------------------------------------------------------------*/
CsrUint32 csrBtAvCalculateStreamBitRate(av_instData_t *instData, CsrUint8 *servCap, CsrUint16 servCapLen, CsrUint8 strIdx)
{
    CsrUint32 bitRate = CSR_BT_A2DP_BIT_RATE_UNKNOWN;
    CsrBool found = FALSE;
    CsrUint16 index = 0;
    CsrUint8 audioCodec = CSR_BT_AV_NON_A2DP_CODEC;
    CsrUint32 sampling_freq = 0; /* In Hz */

    servCap = CsrBtAvGetServiceCap(CSR_BT_AV_SC_MEDIA_CODEC,
                                   servCap,
                                   servCapLen,
                                   &index);

    if (servCap && servCap[CSR_BT_AV_MEDIA_TYPE_INDEX] == CSR_BT_AV_AUDIO_MEDIA_TYPE)
    {/* Media type data found */
        audioCodec = servCap[CSR_BT_AV_CODEC_TYPE_INDEX]>>4;
        found = TRUE;
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
        instData->stream[strIdx].codecBR = 0;
        instData->stream[strIdx].codecType = CSR_BT_A2DP_CODEC_TYPE_UNKNOWN;
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

        switch (audioCodec)
        {
            case CSR_BT_AV_AUDIO_SBC_CODEC: /* 0 */
            {
                /*
                            bit rate = 8 * frame_length * sampling freq / no. subbands / no. blocks
                            If (Mono or Dual channel channel mode)
                                frame length = 4 + (4 * no. subbands * no. channels) / 8 + [no. blocks * no. channels * bitpool / 8]
                            Else (--> stereo and joint stereo)
                                frame length = 4 + (4 * no. subbands * no. channels) / 8 + [(join * no. subbands + no. blocks * bitpool) / 8]
                                (join = 1 if joint stereo channel mode; otherwise 0).
                            */
                CsrUint32 frame_length = 0;  /* In bytes */
                CsrUint8 subBands = 0, blocks = 0, channels = 0, bitpool = 0, channel_mode = 0;

                /* First get the data out of the capabilities received */
                /* sampling frequency */
                switch (servCap[CSR_BT_AV_SFREQ_AND_CMODE_INDEX] & CSR_BT_AV_HIGH_NIBBLE_MASK)
                {
                    case 0x10:
                        sampling_freq = 48000;
                    break;

                    case 0x20:
                        sampling_freq = 44100;
                    break;

                    case 0x40:
                        sampling_freq = 32000;
                    break;

                    case 0x80:
                    default:
                        sampling_freq = 16000;
                    break;
                }

                /* channel mode */
                channel_mode = (servCap[CSR_BT_AV_SFREQ_AND_CMODE_INDEX] & CSR_BT_AV_LOW_NIBBLE_MASK);
                if (channel_mode == CSR_BT_AV_MONO_CMODE)
                {
                    channels = 1;
                }
                else
                {
                    channels = 2;
                }

                /* no. blocks */
                switch (servCap[CSR_BT_AV_BLOCKS_SUBBAND_METHOD_INDEX] & CSR_BT_AV_HIGH_NIBBLE_MASK)
                {
                    case 0x10:
                        blocks = 16;
                    break;
                    case 0x20:
                        blocks = 12;
                    break;
                    case 0x40:
                        blocks = 8;
                    break;
                    case 0x80:
                    default:
                        blocks = 4;
                    break;
                }

                /* no. of sub-bands */
                if ((servCap[CSR_BT_AV_BLOCKS_SUBBAND_METHOD_INDEX] & CSR_BT_AV_SUBBANDS_MASK) == CSR_BT_AV_4_SUBBANDS)
                {
                    subBands = 4;
                }
                else
                {
                    subBands = 8;
                }

                /* max bitpool */
                bitpool = servCap[CSR_BT_AV_MAX_BITPOOL_INDEX];
                /* Now calculate frame length */
                if ((channel_mode == CSR_BT_AV_MONO_CMODE) || (channel_mode == CSR_BT_AV_DUAL_CMODE))
                {
                    frame_length = (CsrUint32) (4 + ((4 * subBands * channels) / 8) + ((blocks * channels * bitpool) / 8) );
                }
                else
                {
                    CsrUint8 join = 0;
                    if (channel_mode == CSR_BT_AV_JOINT_STEREO_CMODE)
                    {
                        join = 1;
                    }

                    frame_length = (CsrUint32) (4 + ((4 * subBands * channels) / 8) + (((join * subBands) + (blocks * bitpool)) / 8) );
                }
                /* Finally, calculate the max bit rate */
                bitRate = (CsrUint32)(((8 * frame_length * sampling_freq) / subBands) / blocks);
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
                instData->stream[strIdx].codecType = CSR_BT_A2DP_CODEC_TYPE_SBC;
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */
                break;
            }

            case CSR_BT_AV_AUDIO_MPEG_2_4_AAC_CODEC: /* 2 */
            {
                bitRate = (CsrUint32)(servCap[CSR_BT_AV_BIT_RATE_HIGH_IDX] & 0x7F);
                bitRate = (CsrUint32)((bitRate << 8) | (CsrUint32)(servCap[CSR_BT_AV_BIT_RATE_MED_IDX]));
                bitRate = (CsrUint32)((bitRate << 8) | (CsrUint32)(servCap[CSR_BT_AV_BIT_RATE_LOW_IDX]));
                break;
            }

            case AV_AUDIO_MPEG_D_USAC_CODEC: /* 3 */
            {
                bitRate = (CsrUint32)(servCap[AV_MPEG_D_USAC_BIT_RATE_HIGH_IDX] & 0x7F);
                bitRate = (CsrUint32)((bitRate << 8) | (CsrUint32)(servCap[AV_MPEG_D_USAC_BIT_RATE_MED_IDX]));
                bitRate = (CsrUint32)((bitRate << 8) | (CsrUint32)(servCap[AV_MPEG_D_USAC_BIT_RATE_LOW_IDX]));
                break;
            }

            case CSR_BT_AV_NON_A2DP_CODEC:
            {
                CsrUint32 vendorId = (CsrUint32)((servCap[CSR_BT_AV_APTX_VENDOR_ID_LSB_INDEX+3] << 24) | (servCap[CSR_BT_AV_APTX_VENDOR_ID_LSB_INDEX+2] << 16) | (servCap[CSR_BT_AV_APTX_VENDOR_ID_LSB_INDEX+1] << 8) | servCap[CSR_BT_AV_APTX_VENDOR_ID_LSB_INDEX]);
                CsrUint16 codecId  = (CsrUint16)((servCap[CSR_BT_AV_APTX_CODEC_ID_LSB_INDEX+1] << 8) | servCap[CSR_BT_AV_APTX_CODEC_ID_LSB_INDEX]);
                /* Find out whether this is an AptX codec and if so, calculate the bitrate; if not, just ignore. */
                if (((vendorId == CSR_BT_AV_APTX_VENDOR_ID1) || (vendorId == CSR_BT_AV_APTX_VENDOR_ID2)) && 
                    ((codecId  == CSR_BT_AV_APTX_CODEC_ID1)  || (codecId  == CSR_BT_AV_APTX_CODEC_ID2)))
                {/* Aptx codec: calculate the bitrate. The bitrate is fixed, compression ratio 1:4 (f.ex. 352.8 kbps at 44.1 Khz)  */
                    
                    /* extract the sampling frequency out of the capabilities received */
                    switch (servCap[CSR_BT_AV_APTX_SFREQ_AND_CMODE_INDEX] & CSR_BT_AV_HIGH_NIBBLE_MASK)
                    {
                        case 0x10:
                            sampling_freq = 48000;
                        break;

                        case 0x20:
                            sampling_freq = 44100;
                        break;

                        case 0x40:
                            sampling_freq = 32000;
                        break;

                        case 0x80:
                        default:
                            sampling_freq = 16000;
                        break;

                    }
                    /* calculate the bit rate */
                    bitRate = (CsrUint32)(8 * sampling_freq);
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
                    instData->stream[strIdx].codecType = CSR_BT_A2DP_CODEC_TYPE_APTX;
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */
                }
                break;
            }
            case CSR_BT_AV_AUDIO_MPEG_1_2_CODEC: /* 1 */
            case CSR_BT_AV_AUDIO_ATRAC_CODEC: /* 4 */
            default:
                break;
        }

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
        /* update sampling frequency */
        if (48000 == sampling_freq)
        {
            instData->stream[strIdx].samplingFreq = CSR_BT_A2DP_SAMPLING_FREQ_48000;
        }
        else if (44100 == sampling_freq)
        {
            instData->stream[strIdx].samplingFreq = CSR_BT_A2DP_SAMPLING_FREQ_44100;
        }
        else if (32000 == sampling_freq)
        {
            instData->stream[strIdx].samplingFreq = CSR_BT_A2DP_SAMPLING_FREQ_32000;
        }
        else
        {
            instData->stream[strIdx].samplingFreq = CSR_BT_A2DP_SAMPLING_FREQ_16000;
        }

        /* update bitrate in kbps*/
        if (bitRate)
        {
            instData->stream[strIdx].codecBR = bitRate/1000; /* bitrate in kbps */
        }
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */
    }
    if ((bitRate != instData->stream[strIdx].bitRate) && found)
    {/* Indicate the change! */
        CsrBtCmA2dpBitRateReqSend(instData->con[instData->stream[strIdx].conId].remoteDevAddr,strIdx,bitRate);
    }
    return bitRate;
}

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvStreamStartStopIndSend
 *
 *  DESCRIPTION
 *      Send AV start/stop indication to CM
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvStreamStartStopIndSend(av_instData_t* instData, CsrUint8 sHandle, CsrBool start)
{
    av_stream_info_t stream = instData->stream[sHandle];
    CsrUint8 conId = stream.conId;
    av_connection_info_t conn = instData->con[conId];

    if (start)
    {/*send start stream indication */
        if (!stream.startSent)
        {/*send start stream indication once, per stream*/
            instData->stream[sHandle].stopSent = FALSE;
            CsrBtCmSetAvStreamInfoReqSend
            (
                sHandle, TRUE, conn.aclHandle, stream.mediaCid, stream.codecBR, stream.mediaMtu, stream.period, 
                (CsrUint8)instData->role, stream.samplingFreq, stream.codecType, stream.codecLocation
            );
            instData->stream[sHandle].startSent = TRUE;
        }
    }
    else
    {/*stop stream indication */
        if (stream.startSent && !stream.stopSent)
        {/*send stop stream indication once, per stream */
            instData->stream[sHandle].startSent = FALSE;
            CsrBtCmSetAvStreamInfoReqSend
            (
                sHandle, FALSE, conn.aclHandle, stream.mediaCid, 
                (CsrUint16)CSR_BT_A2DP_BIT_RATE_UNKNOWN,  /*don't care */
                0, /*don't care */
                0, /*don't care */ 
                0, /*don't care */
                CSR_BT_A2DP_SAMPLING_FREQ_UNKNOWN, /*don't care */
                CSR_BT_A2DP_CODEC_TYPE_UNKNOWN, /*don't care */
                CSR_BT_A2DP_CODEC_LOCATION_UNKNOWN /*don't care */
            );
            instData->stream[sHandle].stopSent = TRUE;
        }
    }
}
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

CsrUint16 CsrBtAvGetMtuSize(CsrUint8 conId)
{
#ifdef CSR_TARGET_PRODUCT_VM
    return (csrBtAvInstData.con[conId].useLargeMtu ? CSR_BT_AV_PROFILE_LARGE_MTU_SIZE : CSR_BT_AV_PROFILE_DEFAULT_MTU_SIZE);
#else
    return (CSR_BT_AV_PROFILE_DEFAULT_MTU_SIZE);
#endif /* CSR_TARGET_PRODUCT_VM */
}

