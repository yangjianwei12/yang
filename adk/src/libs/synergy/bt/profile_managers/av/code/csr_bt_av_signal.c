/******************************************************************************
 Copyright (c) 2010-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_bt_av_main.h"
#include "csr_pmem.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_util.h"
#include "sds_prim.h"
#include "csr_bt_av_prim.h"
#include "csr_bt_av_lib.h"
#include "csr_bt_av_signal.h"
#ifdef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_cm_private_lib.h"
#else
#include "csr_bt_sc_private_lib.h"
#endif

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_av_streams.h"
#endif

/* static function prototypes */
static void csrBtAvDiscoverCmdHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvGetCapabilitiesCmdHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvSetConfigurationCmdHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvGetConfigurationCmdHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvReconfigureCmdHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvOpenCmdHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvStartCmdHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvCloseCmdHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvSuspendCmdHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvAbortCmdHandler(av_instData_t * instData, CsrUint8 conId);
#ifdef INSTALL_AV_SECURITY_CONTROL
static void csrBtAvSecurityControlCmdHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvSecurityControlAcpHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvSecurityControlRejHandler(av_instData_t * instData, CsrUint8 conId);
#else
#define csrBtAvSecurityControlCmdHandler    NULL
#define csrBtAvSecurityControlAcpHandler    NULL
#define csrBtAvSecurityControlRejHandler    NULL
#endif /* INSTALL_AV_SECURITY_CONTROL */
static void csrBtAvGetAllCapabilitiesCmdHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvDelayReportCmdHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvDiscoverAcpHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvGetCapabilitiesAcpHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvSetConfigurationAcpHandler(av_instData_t * instData, CsrUint8 conId);
#ifdef INSTALL_AV_GET_CONFIGURATION
static void csrBtAvGetConfigurationAcpHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvGetConfigurationRejHandler(av_instData_t * instData, CsrUint8 conId);
#else
#define csrBtAvGetConfigurationAcpHandler    NULL
#define csrBtAvGetConfigurationRejHandler    NULL
#endif /* INSTALL_AV_GET_CONFIGURATION */
static void csrBtAvReconfigureAcpHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvOpenAcpHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvStartAcpHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvCloseAcpHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvSuspendAcpHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvAbortAcpHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvDelayReportAcpHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvDiscoverRejHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvGetCapabilitiesRejHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvSetConfigurationRejHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvReconfigureRejHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvOpenRejHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvStartRejHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvCloseRejHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvSuspendRejHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvAbortRejHandler(av_instData_t * instData, CsrUint8 conId);
static void csrBtAvDelayReportRejHandler(av_instData_t * instData, CsrUint8 conId);

/* jump tables for message handler functions */
static avSignalHandler_t avSignalCmdHandler[AVDTP_DELAYREPORT_SID+1] =
{
    NULL,
    csrBtAvDiscoverCmdHandler,
    csrBtAvGetCapabilitiesCmdHandler,
    csrBtAvSetConfigurationCmdHandler,
    csrBtAvGetConfigurationCmdHandler,
    csrBtAvReconfigureCmdHandler,
    csrBtAvOpenCmdHandler,
    csrBtAvStartCmdHandler,
    csrBtAvCloseCmdHandler,
    csrBtAvSuspendCmdHandler,
    csrBtAvAbortCmdHandler,
    csrBtAvSecurityControlCmdHandler,
    csrBtAvGetAllCapabilitiesCmdHandler,
    csrBtAvDelayReportCmdHandler
};

static avSignalHandler_t avSignalAcpHandler[AVDTP_DELAYREPORT_SID+1] =
{
    NULL,
    csrBtAvDiscoverAcpHandler,
    csrBtAvGetCapabilitiesAcpHandler,
    csrBtAvSetConfigurationAcpHandler,
    csrBtAvGetConfigurationAcpHandler,
    csrBtAvReconfigureAcpHandler,
    csrBtAvOpenAcpHandler,
    csrBtAvStartAcpHandler,
    csrBtAvCloseAcpHandler,
    csrBtAvSuspendAcpHandler,
    csrBtAvAbortAcpHandler,
    csrBtAvSecurityControlAcpHandler,
    csrBtAvGetCapabilitiesAcpHandler,
    csrBtAvDelayReportAcpHandler
};

static avSignalHandler_t avSignalRejHandler[AVDTP_DELAYREPORT_SID+1] =
{
    NULL,
    csrBtAvDiscoverRejHandler,
    csrBtAvGetCapabilitiesRejHandler,
    csrBtAvSetConfigurationRejHandler,
    csrBtAvGetConfigurationRejHandler,
    csrBtAvReconfigureRejHandler,
    csrBtAvOpenRejHandler,
    csrBtAvStartRejHandler,
    csrBtAvCloseRejHandler,
    csrBtAvSuspendRejHandler,
    csrBtAvAbortRejHandler,
    csrBtAvSecurityControlRejHandler,
    csrBtAvGetCapabilitiesRejHandler,
    csrBtAvDelayReportRejHandler
};

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvCmL2caDataReqSend
 *
 *  DESCRIPTION
 *      Transmit or queue a signalling message
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvCmL2caDataReqSend(av_connection_info_t *con, CsrUint16 msg_size, CsrUint8 *msg)
{
    if(!con->sending)
    {
#ifdef CSR_STREAMS_ENABLE
        CsrStreamsDataSend(CM_GET_UINT16ID_FROM_BTCONN_ID(con->signalCid),
                           L2CAP_ID,
                           msg_size,
                           msg);
#else
        CsrBtCml2caDataReqSend(con->signalCid,
                               msg_size,
                               msg,
                               CSR_BT_CM_CONTEXT_UNUSED);
#endif
        con->sending = TRUE;
    }
    else
    {
        /* q the data */
        q_element_t    *newElement, *temp;

        newElement = CsrPmemAlloc(sizeof(q_element_t));
        newElement->length = msg_size;
        newElement->data = msg;
        newElement->next = NULL;

        temp = con->qFirst;

        if( con->qFirst == NULL)
        {
            con->qFirst = newElement;
        }
        else
        {
            while(temp->next != NULL)
            {
                temp = temp->next;
            }

            temp->next = newElement;
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvSignalSend
 *
 *  DESCRIPTION
 *      Send a signalling message to peer device, fragment if needed.
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvSignalSend(av_instData_t * instData, CsrUint16 conId, CsrUint16 msg_size, CsrUint8 *msg)
{
    av_connection_info_t *con;

    con = &(instData->con[conId]);

    if (con->conState == CONNECTED_S)
    {
        if( msg_size <= con->signalMtu)
        {
#ifdef CSR_TARGET_PRODUCT_VM
            CSR_LOG_TEXT_INFO((CsrBtAvLto, 0, "csrBtAvSignalSend TLable:0x%0x, MsgType:0x%0x, MESSAGE:AvdtpSignalId:0x%0x, Connection Index:%d, BtConnId:0x%08x",
                               (msg[0] & TLABLE_MASK) >> 4,
                               msg[0] & MSG_TYPE_MASK, 
                               msg[1] & 0x3F,
                               conId,
                               con->signalCid));
#endif
            csrBtAvCmL2caDataReqSend(con, msg_size, msg);
        }
        else
        {
            int frag_size, count;
            CsrUint8       *fragment;
            CsrBool        firstFrag = TRUE;

            count = 0;

            while( msg_size > 0)
            {
                if( firstFrag )
                {
                    firstFrag = FALSE;
                    fragment = (CsrUint8 *) CsrPmemAlloc(con->signalMtu);
                    fragment[0] = (CsrUint8) ((msg[0] & ~PKT_TYPE_MASK) | PKT_TYPE_START);
                    fragment[1] = (CsrUint8) ((msg_size / (con->signalMtu - 1)) + 1);

                    SynMemCpyS(&fragment[2], con->signalMtu - 2, &msg[1], con->signalMtu - 2);
                    frag_size = con->signalMtu;
                    count +=  con->signalMtu - 1;
                    msg_size = (CsrUint16) (msg_size - con->signalMtu + 1);
                }
                else
                {
                    if(msg_size < con->signalMtu)
                    {
                        fragment = (CsrUint8 *) CsrPmemAlloc(msg_size + 1);
                        fragment[0] = (CsrUint8) ((msg[0] & ~PKT_TYPE_MASK) | PKT_TYPE_END);
                        SynMemCpyS(&fragment[1], msg_size, &msg[count], msg_size);
                        frag_size = (CsrUint16) (msg_size + 1);
                        msg_size = 0;
                    }
                    else
                    {
                        fragment = (CsrUint8 *) CsrPmemAlloc(con->signalMtu);
                        fragment[0] = (CsrUint8) ((msg[0] & ~PKT_TYPE_MASK) | PKT_TYPE_CONTINUE);
                        SynMemCpyS(&fragment[1], con->signalMtu - 1, &msg[count], con->signalMtu - 1);
                        frag_size = con->signalMtu;
                        count += (CsrUint16) (con->signalMtu - 1);
                        msg_size = (CsrUint16) (msg_size - con->signalMtu + 1);
                    }
                }

                csrBtAvCmL2caDataReqSend(con, (CsrUint16) frag_size, fragment);
            }

            CsrPmemFree(msg);
        }
    }
    else
    {
        CsrPmemFree(msg);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvCommonSignalReject
 *
 *  DESCRIPTION
 *      Common function for rejecting a message cmd from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvCommonSignalReject(av_instData_t *instData, CsrUint8 conId, CsrUint8 errorCode, CsrUint8 extraByte)
{
    CsrBtCmL2caDataInd *prim;
    CsrUint8 *msg;
    CsrUint8 index = 2;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    msg = (void *) CsrPmemAlloc(4);
    BUILD_MSG_HDR(msg, MSG_TYPE_REJ, (CsrUint8)((prim->payload[0] & TLABLE_MASK)>>4), (CsrUint8)(prim->payload[1] & SIGNAL_ID_MASK));

    if( ((prim->payload[1] & SIGNAL_ID_MASK) == AVDTP_SET_CONFIGURATION_SID)
        || ((prim->payload[1] & SIGNAL_ID_MASK) == AVDTP_RECONFIGURE_SID)
        || ((prim->payload[1] & SIGNAL_ID_MASK) == AVDTP_START_SID)
        || ((prim->payload[1] & SIGNAL_ID_MASK) == AVDTP_SUSPEND_SID) )
    {
        *(msg + index) = extraByte;
        index++;
    }

    *(msg + index) = errorCode;

    csrBtAvSignalSend( instData, conId, (CsrUint16)(index + 1), msg);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      findStreamFromLocalSeid
 *
 *  DESCRIPTION
 *      Find index to stream information based on local stream end-point
 *
 *  RETURNS
 *      The index number if found, CSR_BT_AV_MAX_NUM_STREAMS if not found
 *
 *---------------------------------------------------------------------------*/
static CsrUint8 findStreamFromLocalSeid(av_instData_t *instData, CsrUint8 localSeid, CsrUint16 conId)
{
    CsrUintFast8 s;

    for (s = 0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
    {
        if ( (conId == instData->stream[s].conId) && (localSeid == instData->stream[s].localSeid))
            break;
    }

    return (CsrUint8)s;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      findStreamFromTLabel
 *
 *  DESCRIPTION
 *      Find index to stream information based on transaction label
 *
 *  RETURNS
 *      The index number if found, CSR_BT_AV_MAX_NUM_STREAMS if not found
 *
 *---------------------------------------------------------------------------*/
static CsrUint8 findStreamFromTLabel(av_instData_t *instData, CsrUint8 conId, CsrUint8 tLabel)
{
    CsrUintFast8 s;

    for (s = 0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
    {
        if ( (conId == instData->stream[s].conId) && (tLabel == instData->stream[s].tLabel))
            break;
    }

    return (CsrUint8)s;
}
#ifndef CSR_STREAMS_ENABLE

/*----------------------------------------------------------------------------*
 *  NAME
 *      findStreamFromCid
 *
 *  DESCRIPTION
 *      Find index to stream information based on transaction label
 *
 *  RETURNS
 *      The index number if found, CSR_BT_AV_MAX_NUM_STREAMS if not found
 *
 *---------------------------------------------------------------------------*/
static CsrUint8 findStreamFromCid(av_instData_t *instData, CsrBtConnId btConnId)
{
    CsrUintFast8 s;

    for (s = 0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
    {
        if ( btConnId == instData->stream[s].mediaCid )
            break;
    }

    return (CsrUint8)s;
}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      findConFromCid
 *
 *  DESCRIPTION
 *      Find index to connection information based on L2CAP btConnId
 *
 *  RETURNS
 *      The index number if found, CSR_BT_AV_MAX_NUM_CONNECTIONS if not found
 *
 *---------------------------------------------------------------------------*/
static CsrUint8 findConFromCid(av_instData_t *instData, CsrBtConnId btConnId)
{
    CsrUintFast8 i;

    for (i = 0; i<CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        if ( btConnId == instData->con[i].signalCid )
            break;
    }

    return (CsrUint8)i;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      timeOutHandler
 *
 *  DESCRIPTION
 *      Handles a timeout of a message cmd, peer does not respond to cmd
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void timeOutHandler(CsrUint16 info, av_instData_t *instData)
{
    CsrUint8 sid, tLabel, s;

    s       = (CsrUint8) (info & 0x000F);
    tLabel  = (CsrUint8) ((info>>4) & 0x000F);
    sid     = (CsrUint8) ((info>>8) & 0x00FF);

    switch (sid)
    {
        case AVDTP_DISCOVER_SID:
            {
                CsrBtAvDiscoverCfmSendReject(instData, s, tLabel,
                    CSR_BT_RESULT_CODE_AV_SIGNAL_TIMEOUT,
                    CSR_BT_SUPPLIER_AV);
                instData->con[s].timerId = 0x00;
                instData->con[s].pendingSigProc = 0;
                break;
            }
        case AVDTP_GET_CAPABILITIES_SID:
            {
                CsrBtAvGetCapabilitiesCfmSendReject(instData, s, tLabel,
                    CSR_BT_RESULT_CODE_AV_SIGNAL_TIMEOUT,
                    CSR_BT_SUPPLIER_AV);
                instData->con[s].timerId = 0x00;
                instData->con[s].pendingSigProc = 0;
                break;
            }
        case AVDTP_SET_CONFIGURATION_SID:
            {
                CsrBtAvSetConfigurationCfmSend(instData, instData->stream[s].conId, tLabel, s, 0,
                    CSR_BT_RESULT_CODE_AV_SIGNAL_TIMEOUT,
                    CSR_BT_SUPPLIER_AV);
                instData->stream[s].timerId = 0; 
                CsrBtAvClearStream(instData, s);
                break;
            }
#ifdef INSTALL_AV_GET_CONFIGURATION
        case AVDTP_GET_CONFIGURATION_SID:
            {
                CsrBtAvGetConfigurationCfmSend(instData, tLabel, s, 0, NULL,
                    CSR_BT_RESULT_CODE_AV_SIGNAL_TIMEOUT,
                    CSR_BT_SUPPLIER_AV);
                instData->stream[s].timerId = 0;
                break;
            }
#endif /* INSTALL_AV_GET_CONFIGURATION */
        case AVDTP_RECONFIGURE_SID:
            {
                CsrBtAvReconfigureCfmSend(instData, tLabel, s, 0,
                    CSR_BT_RESULT_CODE_AV_SIGNAL_TIMEOUT,
                    CSR_BT_SUPPLIER_AV);
                instData->stream[s].timerId = 0;
                break;
            }
        case AVDTP_OPEN_SID:
            {
                CsrBtAvOpenCfmSend(instData, s, tLabel,
                    CSR_BT_RESULT_CODE_AV_SIGNAL_TIMEOUT,
                    CSR_BT_SUPPLIER_AV);
                instData->stream[s].timerId = 0;
                break;
            }
        case AVDTP_START_SID:
            {
                CsrUintFast16 i;
                CsrUint16 conId = instData->stream[s].conId;
                
                CsrBtAvStartCfmSend(instData, s, tLabel,
                    CSR_BT_RESULT_CODE_AV_SIGNAL_TIMEOUT,
                    CSR_BT_SUPPLIER_AV);
                instData->stream[s].timerId = 0;

                for( i=0; i<CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
                {
                    if((instData->stream[i].tLabel == tLabel)
                        && (instData->stream[i].conId == conId)
                        && (instData->stream[i].streamState == STARTING_S))
                    {
                        instData->stream[i].streamState = OPENED_S;
                        instData->stream[i].tLabel = 0xFF;
                    }
                }
                break;
            }
        case AVDTP_CLOSE_SID:
            {
                CsrBtAvCloseCfmSend(instData, s, tLabel,
                    CSR_BT_RESULT_CODE_AV_SIGNAL_TIMEOUT,
                    CSR_BT_SUPPLIER_AV);
                instData->stream[s].timerId = 0;
                break;
            }
        case AVDTP_SUSPEND_SID:
            {
                CsrUintFast16 i;
                CsrUint16 conId = instData->stream[s].conId;

                CsrBtAvSuspendCfmSend(instData, s, tLabel,
                    CSR_BT_RESULT_CODE_AV_SIGNAL_TIMEOUT,
                    CSR_BT_SUPPLIER_AV);
                instData->stream[s].timerId = 0;
                
                for( i=0; i<CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
                {
                    if((instData->stream[i].tLabel == tLabel)
                        && (instData->stream[i].conId == conId)
                        && (instData->stream[i].streamState == STREAMING_S))
                    {
                        instData->stream[i].tLabel = 0xFF;
                    }
                }                
                break;
            }
            
        case AVDTP_ABORT_SID:
            {
                if( instData->stream[s].mediaCid != 0)
                {
                    CsrBtCml2caDisconnectReqSend(instData->stream[s].mediaCid);
                    instData->stream[s].streamState = ABORTING_S;
                }
                else
                {
                    CsrBtAvAbortCfmSend(instData, s, tLabel);
                }
                instData->stream[s].timerId = 0;
                break;
            }
#ifdef INSTALL_AV_SECURITY_CONTROL
        case AVDTP_SECURITY_CONTROL_SID:
            {
                CsrBtAvSecurityControlCfmSend(instData, s, tLabel, 0, NULL,
                    CSR_BT_RESULT_CODE_AV_SIGNAL_TIMEOUT,
                    CSR_BT_SUPPLIER_AV);
                instData->stream[s].timerId = 0;
                break;
            }
#endif /* INSTALL_AV_SECURITY_CONTROL */
        case AVDTP_GET_ALL_CAPABILITIES_SID:
            {
                CsrBtAvGetCapabilitiesCfmSendReject(instData, s, tLabel,
                    CSR_BT_RESULT_CODE_AV_SIGNAL_TIMEOUT,
                    CSR_BT_SUPPLIER_AV);
                instData->con[s].timerId = 0x00;
                instData->con[s].pendingSigProc = 0;
                break;
            }
        case AVDTP_DELAYREPORT_SID:
            {
                CsrBtAvDelayReportCfmSend(instData, s, tLabel,
                    CSR_BT_RESULT_CODE_AV_SIGNAL_TIMEOUT,
                    CSR_BT_SUPPLIER_AV);
                instData->stream[s].timerId = 0;
                break;
            }
        default:
            break;
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvDelayReportSend
 *
 *  DESCRIPTION
 *      Function for sending Delay Report to peer SRC device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvDelayReportSend(av_instData_t *instData,
                                   CsrUint8 shandle,
                                   CsrUint8 tLabel,
                                   CsrUint16 delay)
{
    CsrUint8 *msg = (CsrUint8 *) CsrPmemZalloc(AVDTP_DELAYREPORT_SID_LEN);

    BUILD_MSG_HDR(msg,
                  MSG_TYPE_CMD,
                  tLabel,
                  AVDTP_DELAYREPORT_SID);
    msg[2] = (CsrUint8) (instData->stream[shandle].remoteSeid << 2);
    msg[3] = CSR_MSB16(delay);
    msg[4] = CSR_LSB16(delay);

    /* Send Delay Report to SRC */
    csrBtAvSignalSend(instData,
                      instData->stream[shandle].conId,
                      AVDTP_DELAYREPORT_SID_LEN,
                      msg);

    /* Start signal timer */
    instData->stream[shandle].timerId = AV_START_SIGNAL_TIMER(AVDTP_DELAYREPORT_SID,
                                                              tLabel,
                                                              shandle);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDiscoverReqHandler
 *
 *  DESCRIPTION
 *      Handle a request for performing discover procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvDiscoverReqHandler(av_instData_t *instData)
{
    CsrBtAvDiscoverReq *prim;

    prim = (CsrBtAvDiscoverReq *) instData->recvMsgP;

    if( (prim->connectionId < CSR_BT_AV_MAX_NUM_CONNECTIONS)
        && (instData->con[prim->connectionId].conState == CONNECTED_S))
    {
        CsrUint8 *msg = (void *) CsrPmemAlloc(AVDTP_DISCOVER_SID_LEN);
        BUILD_MSG_HDR(msg, MSG_TYPE_CMD, prim->tLabel, AVDTP_DISCOVER_SID);

        csrBtAvSignalSend( instData, prim->connectionId, AVDTP_DISCOVER_SID_LEN, msg);

        instData->con[prim->connectionId].pendingSigProc = AVDTP_DISCOVER_SID; 
        instData->con[prim->connectionId].timerId = AV_START_SIGNAL_TIMER(AVDTP_DISCOVER_SID, prim->tLabel, prim->connectionId);
    }
    else
    {/* Answer back */
        CsrBtAvDiscoverCfmSendReject(instData, prim->connectionId, prim->tLabel,
                    CSR_BT_RESULT_CODE_AV_NOT_CONNECTED,
                    CSR_BT_SUPPLIER_AV);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetCapabilitiesReqHandler
 *
 *  DESCRIPTION
 *      Handle a request for performing get capabilities procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvGetCapabilitiesReqHandler(av_instData_t *instData)
{
    CsrBtAvGetCapabilitiesReq *prim;

    prim = (CsrBtAvGetCapabilitiesReq *) instData->recvMsgP;

    if( (prim->connectionId < CSR_BT_AV_MAX_NUM_CONNECTIONS)
        && (instData->con[prim->connectionId].conState == CONNECTED_S))
    {
        CsrUint8 *msg = (void *) CsrPmemAlloc(AVDTP_GET_CAPABILITIES_SID_LEN);

        if ((instData->con[prim->connectionId].remoteAVDTPVersion >= AVDTP_1_3_VERSION) &&
            (instData->localVersion >= AVDTP_1_3_VERSION))
        {
            BUILD_MSG_HDR(msg, MSG_TYPE_CMD, prim->tLabel, AVDTP_GET_ALL_CAPABILITIES_SID);
            instData->con[prim->connectionId].pendingSigProc = AVDTP_GET_ALL_CAPABILITIES_SID;
        }
        else
        {
            BUILD_MSG_HDR(msg, MSG_TYPE_CMD, prim->tLabel, AVDTP_GET_CAPABILITIES_SID);
            instData->con[prim->connectionId].pendingSigProc = AVDTP_GET_CAPABILITIES_SID;
        }
        msg[2] = (CsrUint8)(prim->acpSeid << 2);

        csrBtAvSignalSend( instData, prim->connectionId, AVDTP_GET_CAPABILITIES_SID_LEN, msg);

        instData->con[prim->connectionId].timerId = AV_START_SIGNAL_TIMER(AVDTP_GET_CAPABILITIES_SID, prim->tLabel, prim->connectionId);
    }
    else
    {/* Answer back */
        CsrBtAvGetCapabilitiesCfmSendReject(instData, prim->connectionId, prim->tLabel,
                    CSR_BT_RESULT_CODE_AV_NOT_CONNECTED,
                    CSR_BT_SUPPLIER_AV);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSetConfigurationReqHandler
 *
 *  DESCRIPTION
 *      Handle a request for performing set configuration procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvSetConfigurationReqHandler(av_instData_t *instData)
{
    CsrBtAvSetConfigurationReq *prim;

    prim = (CsrBtAvSetConfigurationReq *) instData->recvMsgP;

    if ( (prim->connectionId < CSR_BT_AV_MAX_NUM_CONNECTIONS)
        && (instData->con[prim->connectionId].conState == CONNECTED_S))
    {
        CsrUintFast8 s;

        for( s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
        {
            if( instData->stream[s].streamState == IDLE_S)
            {
                break;
            }
        }

        if( s<CSR_BT_AV_MAX_NUM_STREAMS)
        {
            CsrUint8 *msg = (void *) CsrPmemAlloc(AVDTP_SET_CONFIGURATION_SID_MIN_LEN + prim->appServCapLen);

            BUILD_MSG_HDR(msg, MSG_TYPE_CMD, prim->tLabel, AVDTP_SET_CONFIGURATION_SID);
            msg[2] = (CsrUint8)(prim->acpSeid << 2);
            msg[3] = (CsrUint8)(prim->intSeid << 2);
#ifndef CSR_TARGET_PRODUCT_VM
            msg[4] = CSR_BT_AV_SC_MEDIA_TRANSPORT;
            msg[5] = 0;
            SynMemCpyS(&msg[6], prim->appServCapLen, prim->appServCapData, prim->appServCapLen);
#else
            /* In-case of Product VM, App takes care of setting Media Transport field */
            SynMemCpyS(&msg[4], prim->appServCapLen, prim->appServCapData, prim->appServCapLen);
#endif

            csrBtAvSignalSend( instData, prim->connectionId, (CsrUint16) (AVDTP_SET_CONFIGURATION_SID_MIN_LEN + prim->appServCapLen), msg);

            /* Now calculate the bit rate if possible */
            instData->stream[s].bitRate = csrBtAvCalculateStreamBitRate(instData, prim->appServCapData,prim->appServCapLen,(CsrUint8)s);

            instData->stream[s].streamState = CONFIGURING_S;
            instData->stream[s].localSeid = prim->intSeid;
            instData->stream[s].remoteSeid = prim->acpSeid;
            instData->stream[s].conId = prim->connectionId;
            instData->stream[s].tLabel = prim->tLabel;

            instData->stream[s].timerId = AV_START_SIGNAL_TIMER(AVDTP_SET_CONFIGURATION_SID, prim->tLabel, s);
        }
        else
        {
            /* no stream resources left */
            CsrBtAvSetConfigurationCfmSend(instData, prim->connectionId, prim->tLabel, 0, 0,
                CSR_BT_RESULT_CODE_AV_MAX_NUM_OF_CONNECTIONS,
                CSR_BT_SUPPLIER_AV);
        }

        CsrPmemFree(prim->appServCapData);
    }
    else
    {
        CsrBtAvSetConfigurationCfmSend(instData, prim->connectionId, prim->tLabel, 0, 0,
                    CSR_BT_RESULT_CODE_AV_NOT_CONNECTED,
                    CSR_BT_SUPPLIER_AV);
        CsrPmemFree(prim->appServCapData);
    }
}

#ifdef INSTALL_AV_GET_CONFIGURATION
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetConfigurationReqHandler
 *
 *  DESCRIPTION
 *      Handle a request for performing get configuration procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvGetConfigurationReqHandler(av_instData_t *instData)
{
    CsrBtAvGetConfigurationReq *prim;

    prim = (CsrBtAvGetConfigurationReq *) instData->recvMsgP;

    if( prim->shandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if( (instData->stream[prim->shandle].conId < CSR_BT_AV_MAX_NUM_CONNECTIONS)
            && (instData->con[instData->stream[prim->shandle].conId].conState == CONNECTED_S))
        {
            CsrUint8 *msg = (void *) CsrPmemAlloc( AVDTP_GET_CONFIGURATION_SID_LEN);

            BUILD_MSG_HDR(msg, MSG_TYPE_CMD, prim->tLabel, AVDTP_GET_CONFIGURATION_SID);
            msg[2] = (CsrUint8)(instData->stream[prim->shandle].remoteSeid << 2);

            csrBtAvSignalSend( instData, instData->stream[prim->shandle].conId, AVDTP_GET_CONFIGURATION_SID_LEN, msg);

            instData->stream[prim->shandle].tLabel = prim->tLabel;

            instData->stream[prim->shandle].timerId =
                AV_START_SIGNAL_TIMER(AVDTP_GET_CONFIGURATION_SID, prim->tLabel, prim->shandle);
        }
        else
        {
            CsrBtAvGetConfigurationCfmSend(instData, prim->tLabel, prim->shandle, 0, NULL,
                    CSR_BT_RESULT_CODE_AV_NOT_CONNECTED,
                    CSR_BT_SUPPLIER_AV);
        }
    }
    else
    {
        CsrBtAvGetConfigurationCfmSend(instData, prim->tLabel, prim->shandle, 0, NULL,
                    CSR_BT_RESULT_CODE_AV_UNACCEPTABLE_PARAMETER,
                    CSR_BT_SUPPLIER_AV);
    }
}
#endif /* INSTALL_AV_GET_CONFIGURATION */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvReconfigureReqHandler
 *
 *  DESCRIPTION
 *      Handle a request for performing reconfigure procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvReconfigureReqHandler(av_instData_t *instData)
{
    CsrBtAvReconfigureReq *prim;

    prim = (CsrBtAvReconfigureReq *) instData->recvMsgP;
    if( prim->shandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if( (instData->stream[prim->shandle].conId < CSR_BT_AV_MAX_NUM_CONNECTIONS)
            && (instData->con[instData->stream[prim->shandle].conId].conState == CONNECTED_S))
        {
            CsrUint8 *msg = (void *) CsrPmemAlloc( AVDTP_RECONFIGURE_SID_MIN_LEN + prim->servCapLen);

            BUILD_MSG_HDR(msg, MSG_TYPE_CMD, prim->tLabel, AVDTP_RECONFIGURE_SID);
            msg[2] = (CsrUint8)(instData->stream[prim->shandle].remoteSeid << 2);
            SynMemCpyS(&msg[3], prim->servCapLen, prim->servCapData, prim->servCapLen);

            csrBtAvSignalSend( instData, instData->stream[prim->shandle].conId, (CsrUint16) (AVDTP_RECONFIGURE_SID_MIN_LEN + prim->servCapLen), msg);
            /* Now calculate the bit rate if possible */
            instData->stream[prim->shandle].bitRate = csrBtAvCalculateStreamBitRate(instData, prim->servCapData,prim->servCapLen,prim->shandle);
            instData->stream[prim->shandle].tLabel = prim->tLabel;

            instData->stream[prim->shandle].timerId = AV_START_SIGNAL_TIMER(AVDTP_RECONFIGURE_SID, prim->tLabel, prim->shandle);
                CsrPmemFree(prim->servCapData);
        }
        else
        {
        CsrPmemFree(prim->servCapData);
            CsrBtAvReconfigureCfmSend(instData, prim->tLabel, prim->shandle, 0,
                    CSR_BT_RESULT_CODE_AV_NOT_CONNECTED,
                    CSR_BT_SUPPLIER_AV);
        }
    }
    else
    {
        CsrPmemFree(prim->servCapData);
        CsrBtAvReconfigureCfmSend(instData, prim->tLabel, prim->shandle, 0,
                    CSR_BT_RESULT_CODE_AV_UNACCEPTABLE_PARAMETER,
                    CSR_BT_SUPPLIER_AV);
    }

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvOpenReqHandler
 *
 *  DESCRIPTION
 *      Handle a request for performing open procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvOpenReqHandler(av_instData_t *instData)
{
    CsrBtAvOpenReq *prim;

    prim = (CsrBtAvOpenReq *) instData->recvMsgP;

    if( prim->shandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        CsrUint8 idx;

        idx = instData->stream[prim->shandle].conId;
        if( (idx < CSR_BT_AV_MAX_NUM_CONNECTIONS)
            && (instData->con[idx].conState == CONNECTED_S))
        {
            CsrUint8 *msg = (CsrUint8 *)CsrPmemAlloc(AVDTP_OPEN_SID_LEN);

            BUILD_MSG_HDR(msg, MSG_TYPE_CMD, prim->tLabel, AVDTP_OPEN_SID);
            msg[2] = (CsrUint8)(instData->stream[prim->shandle].remoteSeid << 2);

                csrBtAvSignalSend( instData, idx, AVDTP_OPEN_SID_LEN, msg);

            instData->stream[prim->shandle].tLabel = prim->tLabel;

            instData->stream[prim->shandle].timerId = AV_START_SIGNAL_TIMER(AVDTP_OPEN_SID, prim->tLabel, prim->shandle);
        }
        else
        {
            CsrBtAvOpenCfmSend(instData, prim->shandle, prim->tLabel,
                                CSR_BT_RESULT_CODE_AV_NOT_CONNECTED,
                                CSR_BT_SUPPLIER_AV);
        }
    }
    else
    {
        CsrBtAvOpenCfmSend(instData, prim->shandle, prim->tLabel,
                                CSR_BT_RESULT_CODE_AV_UNACCEPTABLE_PARAMETER,
                                CSR_BT_SUPPLIER_AV);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvStartReqHandler
 *
 *  DESCRIPTION
 *      Handle a request for performing start procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvStartReqHandler(av_instData_t *instData)
{
    CsrBtAvStartReq *prim;
    CsrUint8 sHandle, idx;
    prim = (CsrBtAvStartReq *) instData->recvMsgP;

    sHandle = prim->list[0];

    if( sHandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        idx = instData->stream[sHandle].conId;

        if( (idx < CSR_BT_AV_MAX_NUM_CONNECTIONS)
            && (instData->con[idx].conState == CONNECTED_S))
        {
            CsrUintFast8 i;
            CsrUint8 *msg = (void *) CsrPmemAlloc( AVDTP_START_SID_MIN_LEN + prim->listLength);

            BUILD_MSG_HDR(msg, MSG_TYPE_CMD, prim->tLabel, AVDTP_START_SID);

            for(i=0; i<prim->listLength; i++)
            {
                msg[2+i] = instData->stream[prim->list[i]].remoteSeid << 2;
                instData->stream[prim->list[i]].tLabel = prim->tLabel;
                instData->stream[prim->list[i]].streamState = STARTING_S;
            }

            csrBtAvSignalSend(instData,
                              idx,
                              (CsrUint16) (AVDTP_START_SID_MIN_LEN + prim->listLength),
                              msg);

            instData->stream[sHandle].timerId = AV_START_SIGNAL_TIMER(AVDTP_START_SID,
                                                                        prim->tLabel,
                                                                            sHandle);

            CsrPmemFree(prim->list);
        }
        else
        {
        CsrPmemFree(prim->list);
            CsrBtAvStartCfmSend(instData, sHandle, prim->tLabel,
                                CSR_BT_RESULT_CODE_AV_NOT_CONNECTED,
                                CSR_BT_SUPPLIER_AV);
        }
    }
    else
    {
        CsrPmemFree(prim->list);
        CsrBtAvStartCfmSend(instData, sHandle, prim->tLabel,
                            CSR_BT_RESULT_CODE_AV_UNACCEPTABLE_PARAMETER,
                            CSR_BT_SUPPLIER_AV);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCloseReqHandler
 *
 *  DESCRIPTION
 *      Handle a request for performing close procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCloseReqHandler(av_instData_t *instData)
{
    CsrBtAvCloseReq *prim;
    CsrUint8 shandle,idx;

    prim = (CsrBtAvCloseReq *) instData->recvMsgP;
    shandle = prim->shandle;
    idx = instData->stream[shandle].conId;
    if( prim->shandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if ( (idx < CSR_BT_AV_MAX_NUM_CONNECTIONS)
            && (instData->con[idx].conState == CONNECTED_S))
        {
            CsrUint8 *msg = (void *) CsrPmemAlloc( AVDTP_CLOSE_SID_LEN);

            BUILD_MSG_HDR(msg, MSG_TYPE_CMD, prim->tLabel, AVDTP_CLOSE_SID);
            msg[2] = (CsrUint8)(instData->stream[shandle].remoteSeid << 2);

            csrBtAvSignalSend( instData, idx, AVDTP_CLOSE_SID_LEN, msg);

            instData->stream[shandle].tLabel = prim->tLabel;

            instData->stream[shandle].timerId = AV_START_SIGNAL_TIMER(AVDTP_CLOSE_SID, prim->tLabel, prim->shandle);
        }
        else
        {
            CsrBtAvCloseCfmSend(instData, shandle, prim->tLabel,
                        CSR_BT_RESULT_CODE_AV_NOT_CONNECTED,
                        CSR_BT_SUPPLIER_AV);
        }
    }
    else
    {
        CsrBtAvCloseCfmSend(instData, shandle, prim->tLabel,
                        CSR_BT_RESULT_CODE_AV_UNACCEPTABLE_PARAMETER,
                        CSR_BT_SUPPLIER_AV);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvIgnoreReqHandler
 *
 *  DESCRIPTION
 *      Ignore a request from the application if it should not be handled 
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvIgnoreReqHandler(av_instData_t *instData)
{
    /* just do nothing */
    CSR_UNUSED(instData);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSuspendReqHandler
 *
 *  DESCRIPTION
 *      Handle a request for performing suspend procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvSuspendReqHandler(av_instData_t *instData)
{
    CsrBtAvSuspendReq *prim;
    CsrUint8 sHandle, idx;
    prim = (CsrBtAvSuspendReq *) instData->recvMsgP;

    sHandle = prim->list[0];

    if( sHandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        idx = instData->stream[sHandle].conId;
        if( (idx < CSR_BT_AV_MAX_NUM_CONNECTIONS)
            && (instData->con[idx].conState == CONNECTED_S))
        {
            CsrUintFast8 i;
            CsrUint8 *msg = (void *) CsrPmemAlloc( AVDTP_SUSPEND_SID_MIN_LEN + prim->listLength);

            BUILD_MSG_HDR(msg, MSG_TYPE_CMD, prim->tLabel, AVDTP_SUSPEND_SID);

            for(i=0; i<prim->listLength; i++)
            {
                msg[2+i] = instData->stream[prim->list[i]].remoteSeid << 2;
                instData->stream[prim->list[i]].tLabel = prim->tLabel;
            }

            csrBtAvSignalSend(instData,
                              idx,
                              (CsrUint16) (AVDTP_SUSPEND_SID_MIN_LEN + prim->listLength),
                              msg);

            instData->stream[prim->list[0]].timerId = AV_START_SIGNAL_TIMER(AVDTP_SUSPEND_SID,
                                                                            prim->tLabel,
                                                                                sHandle);

            CsrPmemFree(prim->list);
        }
        else
        {
            CsrPmemFree(prim->list);
            CsrBtAvSuspendCfmSend(instData, sHandle, prim->tLabel,
                    CSR_BT_RESULT_CODE_AV_NOT_CONNECTED,
                    CSR_BT_SUPPLIER_AV);
        }
    }
    else
    {
        CsrPmemFree(prim->list);
        CsrBtAvSuspendCfmSend(instData, sHandle, prim->tLabel,
                    CSR_BT_RESULT_CODE_AV_UNACCEPTABLE_PARAMETER,
                    CSR_BT_SUPPLIER_AV);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvAbortReqHandler
 *
 *  DESCRIPTION
 *      Handle a request for performing abort procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvAbortReqHandler(av_instData_t *instData)
{
    CsrBtAvAbortReq *prim;
    CsrUint8 sHandle;

    prim = (CsrBtAvAbortReq *) instData->recvMsgP;
    sHandle = prim->shandle;

    if(sHandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        CsrUint8 idx;

        idx = instData->stream[sHandle].conId;

        if ((idx < CSR_BT_AV_MAX_NUM_CONNECTIONS)  && 
           (instData->con[idx].conState == CONNECTED_S) &&
           (instData->stream[sHandle].streamState != ABORTING_S))
        {
            CsrUint8 *msg = (void *) CsrPmemAlloc( AVDTP_ABORT_SID_LEN);

            BUILD_MSG_HDR(msg, MSG_TYPE_CMD, prim->tLabel, AVDTP_ABORT_SID);
            msg[2] = (CsrUint8)(instData->stream[sHandle].remoteSeid << 2);

            csrBtAvSignalSend( instData, idx, AVDTP_ABORT_SID_LEN, msg);

            instData->stream[sHandle].tLabel = prim->tLabel;

            instData->stream[sHandle].timerId = AV_START_SIGNAL_TIMER(AVDTP_ABORT_SID, prim->tLabel, sHandle);

            if (instData->stream[sHandle].mediaCid != 0)
            {
                instData->stream[sHandle].streamState = ABORT_REQUESTED_S;
            }
        }
    }
    else
    {
        CsrBtAvAbortCfmSend(instData, sHandle, prim->tLabel);
    }

}

#ifdef INSTALL_AV_SECURITY_CONTROL
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSecurityReqHandler
 *
 *  DESCRIPTION
 *      Handle a request for performing security control procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvSecurityReqHandler(av_instData_t *instData)
{
    CsrBtAvSecurityControlReq *prim;
    CsrUint8 sHandle, idx;

    prim = (CsrBtAvSecurityControlReq *) instData->recvMsgP;
    sHandle = prim->shandle;
    idx = instData->stream[prim->shandle].conId;

    if(( sHandle < CSR_BT_AV_MAX_NUM_STREAMS) &&
       (idx < CSR_BT_AV_MAX_NUM_CONNECTIONS)  && 
       (instData->con[idx].conState == CONNECTED_S))
    {
        CsrUint8 *msg = (void *) CsrPmemAlloc( AVDTP_SECURITY_CONTROL_SID_MIN_LEN + prim->contProtMethodLen);

        BUILD_MSG_HDR(msg, MSG_TYPE_CMD, prim->tLabel, AVDTP_SECURITY_CONTROL_SID);
        msg[2] = (CsrUint8)(instData->stream[prim->shandle].remoteSeid << 2);
        SynMemCpyS( &msg[3], prim->contProtMethodLen, prim->contProtMethodData, prim->contProtMethodLen);

        csrBtAvSignalSend( instData,
                      instData->stream[prim->shandle].conId,
                      (CsrUint16) (AVDTP_SECURITY_CONTROL_SID_MIN_LEN + prim->contProtMethodLen),
                      msg);

        instData->stream[prim->shandle].tLabel = prim->tLabel;

        instData->stream[prim->shandle].timerId = AV_START_SIGNAL_TIMER(AVDTP_SECURITY_CONTROL_SID, prim->tLabel, prim->shandle);

        CsrPmemFree(prim->contProtMethodData);
    }
    else
    {
        CsrPmemFree(prim->contProtMethodData);
        CsrBtAvSecurityControlCfmSend(instData, sHandle, prim->tLabel, 0, NULL,
                        CSR_BT_RESULT_CODE_AV_NOT_CONNECTED,
                        CSR_BT_SUPPLIER_AV);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSecurityReqFree
 *
 *  DESCRIPTION
 *      Free signal data, ignore request
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvSecurityReqFree(av_instData_t *instData)
{
    CsrBtAvSecurityControlReq *prim;

    prim = (CsrBtAvSecurityControlReq *) instData->recvMsgP;

    CsrPmemFree(prim->contProtMethodData);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSecurityResHandler
 *
 *  DESCRIPTION
 *      Handle a response to security control procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvSecurityResHandler(av_instData_t *instData)
{
    CsrUint8 *msg;
    CsrUint16 msg_len;

    CsrBtAvSecurityControlRes *prim;

    prim = (CsrBtAvSecurityControlRes *) instData->recvMsgP;

    if (prim->shandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if (prim->avResponse == CSR_BT_AV_ACCEPT)
        {
            msg_len = AVDTP_SIGNALLING_HDR_LEN + prim->contProtMethodLen;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_ACP, prim->tLabel, AVDTP_SECURITY_CONTROL_SID);
            SynMemCpyS(&msg[2], prim->contProtMethodLen, prim->contProtMethodData, prim->contProtMethodLen);

            CsrPmemFree(prim->contProtMethodData);
        }
        else
        {
            msg_len = AVDTP_SIGNALLING_HDR_LEN + 1;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_REJ, prim->tLabel, AVDTP_SECURITY_CONTROL_SID);
            msg[2] = prim->avResponse;
        }
        csrBtAvSignalSend( instData, instData->stream[prim->shandle].conId, msg_len, msg);
    }
}
#endif /* INSTALL_AV_SECURITY_CONTROL */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvStreamDataReqHandler
 *
 *  DESCRIPTION
 *      Stream data to peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
#ifndef CSR_STREAMS_ENABLE
void CsrBtAvStreamDataReqHandler(av_instData_t *instData)
{
    CsrBtAvStreamDataReq *prim;
    CsrUint8 tmpShandle, idx;

    prim = (CsrBtAvStreamDataReq*) instData->recvMsgP;

    tmpShandle = prim->shandle;
    idx = instData->stream[tmpShandle].conId;

    if(( tmpShandle < CSR_BT_AV_MAX_NUM_STREAMS) &&
       (idx < CSR_BT_AV_MAX_NUM_CONNECTIONS)  && 
       (instData->con[idx].conState == CONNECTED_S))
    {
        av_stream_info_t *stream = &instData->stream[tmpShandle];
            
        if(stream->streamState == STREAMING_S)
        {
            CsrBtCmL2caDataReq * cmPrim;

            if( prim->hdr_type == CSR_BT_AV_MEDIA_PACKET_HDR_TYPE_RTP )
            {   /* assign sequence number to the RTP media packet header */
                prim->data[3] = (CsrUint8) stream->seqNo;
                prim->data[2] = (CsrUint8) (stream->seqNo>>8);
            }
            stream->seqNo++;

            cmPrim = CsrBtCml2caDataReq_struct(stream->mediaCid,
                                              prim->length,
                                              prim->data,
                                              instData->l2capCtx++);
            prim->data = NULL;

            if (!stream->sending)
            {
                CsrSchedMessagePut(CSR_BT_CM_IFACEQUEUE, CSR_BT_CM_PRIM, cmPrim);
                stream->sending = TRUE;
            }
            else
            {
                if( stream->fifoQ[stream->fifoQIn] != NULL)
                {
                    /* buffer is full - discarding the oldest packet */
                    CsrMblkDestroy(((CsrBtCmL2caDataReq *) stream->fifoQ[stream->fifoQIn])->payload);
                    CsrPmemFree(stream->fifoQ[stream->fifoQIn]);
                    stream->fifoQOut = (stream->fifoQOut + 1) % CSR_BT_AV_MEDIA_BUFFER_SIZE;
                }
                stream->fifoQ[stream->fifoQIn] = (CsrBtCmPrim *) cmPrim;
                stream->fifoQIn = (stream->fifoQIn + 1) % CSR_BT_AV_MEDIA_BUFFER_SIZE;
            }
            
            if (instData->qosInterval > 0)
            {
                if( (stream->qos_replyCounter++) >= instData->qosInterval)
                {
                    CsrUint8 bufSize = 0;
                    if(stream->fifoQIn == stream->fifoQOut)
                    {
                        CsrUint8 previous;
                        if (stream->fifoQIn == 0)
                        {
                            previous = CSR_BT_AV_MEDIA_BUFFER_SIZE - 1;
                        }
                        else
                        {
                            previous = stream->fifoQIn-1;
                        }

                        if(stream->fifoQ[previous] != NULL)
                        {
                            /*buffer is full */
                            bufSize = (CsrUint8)(CSR_BT_AV_MEDIA_BUFFER_SIZE);
                        }
                        /* else the buffer is empty; bufSize is already 0 */
                    }
                    else
                    {
                        if(stream->fifoQIn > stream->fifoQOut)
                        {
                            /* In after Out*/
                            bufSize = (CsrUint8)(stream->fifoQIn - stream->fifoQOut);
                        }
                        else
                        {
                            /* Out after in */
                            bufSize = (CsrUint8)(CSR_BT_AV_MEDIA_BUFFER_SIZE - stream->fifoQOut + stream->fifoQIn);
                        }
                    }

                    CsrBtAvQosIndSend(instData, tmpShandle, (CsrUint16)((bufSize * AV_QOS_MAX_INDICATION)/CSR_BT_AV_MEDIA_BUFFER_SIZE));

                    stream->qos_replyCounter = 0;
                }
            }
            else
            {
                if( (stream->fifoQ[stream->fifoQIn] != NULL) && !stream->sentBufFullInd)
                {
                    /* buffer is completely filled, notify application */
                    CsrBtAvQosIndSend(instData, tmpShandle, AV_QOS_MAX_INDICATION);
                    stream->sentBufFullInd = TRUE;
                }
            }
        }
        else
        {/* Not streaming */
            CsrPmemFree(prim->data);
        }
    }
    else
    {/* Invalid connection ID or invalid state */
        CsrPmemFree(prim->data);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvStreamDataReqFree
 *
 *  DESCRIPTION
 *      Free signal data, ignore request
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvStreamDataReqFree(av_instData_t *instData)
{
    CsrBtAvStreamDataReq *prim;

    prim = (CsrBtAvStreamDataReq*) instData->recvMsgP;

    CsrPmemFree(prim->data);
}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDelayReportReqHandler
 *
 *  DESCRIPTION
 *      Handle delay report request
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvDelayReportReqHandler(av_instData_t *instData)
{
    CsrBtResultCode result = CSR_BT_RESULT_CODE_AV_SUCCESS;
    CsrBtAvDelayReportReq *prim = (CsrBtAvDelayReportReq *) instData->recvMsgP;

    if (prim->shandle < CSR_BT_AV_MAX_NUM_STREAMS)
    { /* Valid stream handle */
        av_connection_info_t *con = &instData->con[instData->stream[prim->shandle].conId];

        if ((instData->localVersion >= AVDTP_1_3_VERSION)
            && (con->remoteAVDTPVersion >= AVDTP_1_3_VERSION))
        { /* Delay reporting is supported */
            av_stream_info_t *stream = &instData->stream[prim->shandle];

            if ((stream->streamState == STREAMING_S
                 && stream->delayValue != prim->delay) /* Delay changed while streaming, OR */
                || (stream->streamState >= CONFIGURED_S
                    && stream->streamState < STREAMING_S)) /* stream is being configured */
            {
                /* Send Delay Report to SRC */
                csrBtAvDelayReportSend(instData,
                                       prim->shandle,
                                       prim->tLabel,
                                       prim->delay);

                /* Save the delay and transaction label */
                stream->delayValue = prim->delay;
                stream->tLabel = prim->tLabel;
            }
            else
            {
                result = CSR_BT_RESULT_CODE_AV_UNEXPECTED_REQUEST;
            }
        }
        else
        {
            result = CSR_BT_RESULT_CODE_AV_NOT_SUPPORTED;
        }
    }
    else
    {
        result = CSR_BT_RESULT_CODE_AV_UNACCEPTABLE_PARAMETER;
    }

    /* Respond to application if Delay Report was not sent */
    if (result != CSR_BT_RESULT_CODE_AV_SUCCESS)
    {
        CsrBtAvDelayReportCfmSend(instData,
                                  prim->shandle,
                                  prim->tLabel,
                                  result,
                                  CSR_BT_SUPPLIER_AV);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDiscoverResHandler
 *
 *  DESCRIPTION
 *      Handle a response to discover procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvDiscoverResHandler(av_instData_t *instData)
{
    CsrBtAvDiscoverRes *prim;
    CsrUint16 msg_len;
    CsrUint8 *msg;

    prim = (CsrBtAvDiscoverRes *) instData->recvMsgP;

    if (prim->avResponse == CSR_BT_AV_ACCEPT)
    {
        CsrUintFast8 i;

        msg_len = (prim->seidInfoCount * 2) + AVDTP_SIGNALLING_HDR_LEN;
        msg = (CsrUint8*)CsrPmemAlloc(msg_len);

        BUILD_MSG_HDR(msg,
                      MSG_TYPE_ACP,
                      prim->tLabel,
                      AVDTP_DISCOVER_SID);

        for(i=0; i<prim->seidInfoCount; i++)
        {
            msg[2 + (i<<1)] = (CsrUint8)((prim->seidInfo[i].acpSeid << 2) | (prim->seidInfo[i].inUse << 1));
            msg[3 + (i<<1)] = (CsrUint8)((prim->seidInfo[i].mediaType << 4) | (prim->seidInfo[i].sepType << 3));
        }
    }
    else
    { /* application has rejected the request */
        msg_len = AVDTP_SIGNALLING_HDR_LEN + 1;
        msg = (CsrUint8*) CsrPmemAlloc(msg_len);
        BUILD_MSG_HDR(msg, MSG_TYPE_REJ, prim->tLabel, AVDTP_DISCOVER_SID);
        msg[2] = prim->avResponse;
    }

    csrBtAvSignalSend( instData, prim->connectionId, msg_len, msg);
    CsrPmemFree(prim->seidInfo);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetCapabilitiesResHandler
 *
 *  DESCRIPTION
 *      Handle a response to get capabilities procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvGetCapabilitiesResHandler(av_instData_t *instData)
{
    CsrBtAvGetCapabilitiesRes *prim;
    CsrUint8 *msg;
    CsrUint16 msg_len;
    CsrUint8 cmdId = AVDTP_GET_CAPABILITIES_SID;

    prim = (CsrBtAvGetCapabilitiesRes * ) instData->recvMsgP;

    if (prim->type == CSR_BT_AV_GET_ALL_CAPABILITIES_RES)
    {
        cmdId = AVDTP_GET_ALL_CAPABILITIES_SID;
    }
    
    if (prim->avResponse == CSR_BT_AV_ACCEPT)
    {
        msg_len    = (CsrUint16)(AVDTP_SIGNALLING_HDR_LEN + prim->servCapLen);
        msg = (CsrUint8*) CsrPmemAlloc(msg_len);

        BUILD_MSG_HDR(msg, MSG_TYPE_ACP, prim->tLabel, cmdId);

        SynMemCpyS( &msg[2], prim->servCapLen, prim->servCapData, prim->servCapLen);
    }
    else
    {
        msg_len = AVDTP_SIGNALLING_HDR_LEN + 1;
        msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
        BUILD_MSG_HDR(msg, MSG_TYPE_REJ, prim->tLabel, cmdId);
        msg[2] = prim->avResponse;
    }

    csrBtAvSignalSend( instData, prim->connectionId, msg_len, msg);

    CsrPmemFree(prim->servCapData);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetCapabilitiesResFree
 *
 *  DESCRIPTION
 *      Free signal data, ignore response
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvGetCapabilitiesResFree(av_instData_t *instData)
{
    CsrBtAvGetCapabilitiesRes *prim;

    prim = (CsrBtAvGetCapabilitiesRes * ) instData->recvMsgP;

    CsrPmemFree(prim->servCapData);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSetConfigurationResHandler
 *
 *  DESCRIPTION
 *      Handle a response to set configuration procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvSetConfigurationResHandler(av_instData_t *instData)
{
    CsrUint8 *msg;
    CsrUint16 msg_len;

    CsrBtAvSetConfigurationRes *prim;

    prim = (CsrBtAvSetConfigurationRes *) instData->recvMsgP;

    if (prim->shandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if (prim->avResponse == CSR_BT_AV_ACCEPT)
        {
            msg_len = AVDTP_SIGNALLING_HDR_LEN;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_ACP, prim->tLabel, AVDTP_SET_CONFIGURATION_SID);
            instData->stream[prim->shandle].streamState = CONFIGURED_S;
            csrBtAvSignalSend( instData, instData->stream[prim->shandle].conId, msg_len, msg);
        }
        else
        {
            msg_len = AVDTP_SIGNALLING_HDR_LEN + 2;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_REJ, prim->tLabel, AVDTP_SET_CONFIGURATION_SID);
            msg[2] = prim->servCategory;
            msg[3] = prim->avResponse;
            csrBtAvSignalSend( instData, instData->stream[prim->shandle].conId, msg_len, msg);
            CsrBtAvClearStream(instData, prim->shandle);
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetConfigurationResHandler
 *
 *  DESCRIPTION
 *      Handle a response to get configuration procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvGetConfigurationResHandler(av_instData_t *instData)
{
    CsrUint8 *msg;
    CsrUint16 msg_len;

    CsrBtAvGetConfigurationRes *prim;

    prim = (CsrBtAvGetConfigurationRes *) instData->recvMsgP;

    if (prim->shandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if (prim->avResponse == CSR_BT_AV_ACCEPT)
        {
            msg_len = AVDTP_SIGNALLING_HDR_LEN + prim->servCapLen;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_ACP, prim->tLabel, AVDTP_GET_CONFIGURATION_SID);
            SynMemCpyS(&msg[2], prim->servCapLen, prim->servCapData, prim->servCapLen);
        }
        else
        {
            msg_len = AVDTP_SIGNALLING_HDR_LEN + 1;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_REJ, prim->tLabel, AVDTP_GET_CONFIGURATION_SID);
            msg[2] = prim->avResponse;
        }
        csrBtAvSignalSend( instData, instData->stream[prim->shandle].conId, msg_len, msg);
    }

    CsrPmemFree(prim->servCapData);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvGetConfigurationResFree
 *
 *  DESCRIPTION
 *      Free signal data, ignore request
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvGetConfigurationResFree(av_instData_t *instData)
{
    CsrBtAvGetConfigurationRes *prim;

    prim = (CsrBtAvGetConfigurationRes *) instData->recvMsgP;

    CsrPmemFree(prim->servCapData);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvReconfigureResHandler
 *
 *  DESCRIPTION
 *      Handle a response to reconfigure procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvReconfigureResHandler(av_instData_t *instData)
{
    CsrUint8 *msg;
    CsrUint16 msg_len;

    CsrBtAvReconfigureRes *prim;

    prim = (CsrBtAvReconfigureRes *) instData->recvMsgP;

    if (prim->shandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if (prim->avResponse == CSR_BT_AV_ACCEPT)
        {
            msg_len = AVDTP_SIGNALLING_HDR_LEN;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_ACP, prim->tLabel, AVDTP_RECONFIGURE_SID);
        }
        else
        {
            msg_len = AVDTP_SIGNALLING_HDR_LEN + 2;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_REJ, prim->tLabel, AVDTP_RECONFIGURE_SID);
            msg[2] = prim->servCategory;
            msg[3] = prim->avResponse;
        }
        csrBtAvSignalSend( instData, instData->stream[prim->shandle].conId, msg_len, msg);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvOpenResHandler
 *
 *  DESCRIPTION
 *      Handle a response to open procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvOpenResHandler(av_instData_t *instData)
{
    CsrUint8 *msg;
    CsrUint16 msg_len;
    CsrBtAvOpenRes *prim;

    prim = (CsrBtAvOpenRes *) instData->recvMsgP;

    if (prim->shandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        CsrUint8 connId = instData->stream[prim->shandle].conId;

        if (prim->avResponse == CSR_BT_AV_ACCEPT)
        {
            uuid16_t uuid;
            dm_security_level_t secIncoming;

            msg_len = AVDTP_SIGNALLING_HDR_LEN;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_ACP, prim->tLabel, AVDTP_OPEN_SID);

            if (instData->serviceRecordHandles[CSR_BT_AV_AUDIO_SOURCE] || instData->serviceRecordHandles[CSR_BT_AV_AUDIO_SINK])
            {
                uuid = CSR_BT_ADVANCED_AUDIO_PROFILE_UUID;
            }
            else
            {/* Only VDP is supported */
                uuid = CSR_BT_VIDEO_DISTRIBUTION_UUID;
            }

#ifndef INSTALL_AV_CUSTOM_SECURITY_SETTINGS
            CsrBtScSetSecInLevel(&secIncoming, CSR_BT_SEC_DEFAULT,
                CSR_BT_AV_MANDATORY_SECURITY_INCOMING,
                CSR_BT_AV_DEFAULT_SECURITY_INCOMING,
                CSR_BT_RESULT_CODE_AV_SUCCESS,
                CSR_BT_RESULT_CODE_AV_UNACCEPTABLE_PARAMETER);
#else
            secIncoming = instData->secIncoming;
#endif /* !INSTALL_AV_CUSTOM_SECURITY_SETTINGS */

            /* await that the remote device establishes the media stream channel */
            CsrBtCmL2caConnectAcceptReqExSend(CSR_BT_AV_IFACEQUEUE,
                                              CSR_BT_AVDTP_PSM,
                                              secIncoming,
                                              uuid,
                                              CsrBtAvGetMtuSize(connId),
                                              0,
                                              instData->con[connId].remoteDevAddr,
                                              CSR_BT_AV_STREAM_CONTEXT(prim->shandle),
                                              CSR_BT_CM_PRIORITY_HIGH,
                                              CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                                     CSR_BT_AV_DEFAULT_ENC_KEY_SIZE_VAL));

            instData->stream[prim->shandle].streamState = PEER_OPENING_S;
        }
        else
        {
            msg_len = AVDTP_SIGNALLING_HDR_LEN + 1;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_REJ, prim->tLabel, AVDTP_OPEN_SID);
            msg[2] = prim->avResponse;
        }
        csrBtAvSignalSend( instData, connId, msg_len, msg);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvStartResHandler
 *
 *  DESCRIPTION
 *      Handle a response to start procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvStartResHandler(av_instData_t *instData)
{
    CsrUint8 *msg;
    CsrUint16 msg_len;

    CsrBtAvStartRes *prim;

    prim = (CsrBtAvStartRes *) instData->recvMsgP;

    if(prim->list[0] < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        CsrUint8 s;

        if (prim->avResponse == CSR_BT_AV_ACCEPT)
        {
            msg_len = AVDTP_SIGNALLING_HDR_LEN;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_ACP, prim->tLabel, AVDTP_START_SID);
        }
        else
        {
            msg_len = AVDTP_SIGNALLING_HDR_LEN + 2;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_REJ, prim->tLabel, AVDTP_START_SID);
            msg[2] = (CsrUint8 ) (instData->stream[prim->reject_shandle].localSeid<<2);
            msg[3] = prim->avResponse;
        }
        csrBtAvSignalSend( instData, instData->stream[prim->list[0]].conId, msg_len, msg);

        s = 0;
        while(s < prim->listLength)
        {
            if (prim->avResponse != CSR_BT_AV_ACCEPT &&
                prim->list[s] == prim->reject_shandle)
            {
                break;
            }

            CsrBtCmA2dpBitRateReqSend(instData->con[instData->stream[prim->list[s]].conId].remoteDevAddr,s,instData->stream[s].bitRate);
            
            if (instData->stream[prim->list[s]].streamState == PEER_OPENING_S)
            {/* The streaming channel is not fully established yet */
                instData->stream[prim->list[s]].streamState = PEER_OPENING_TO_STREAM_S;
            }
            else
            {
                CsrUint8 idx = prim->list[s];
                CsrUint8 conId = instData->stream[idx].conId;
                
                CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_STREAM_CHANNEL,instData->con[conId].remoteDevAddr,instData->stream[idx].mediaCid);
                instData->stream[idx].streamState = STREAMING_S;
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
                CsrBtAvStreamStartStopIndSend(instData, idx, TRUE);
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */
            }
            s++;
        }
    }
    CsrPmemFree(prim->list);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCloseResHandler
 *
 *  DESCRIPTION
 *      Handle a response to close procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCloseResHandler(av_instData_t *instData)
{
    CsrUint8 *msg;
    CsrUint16 msg_len;
    CsrUint8 sHandle;
    CsrBtAvCloseRes *prim;

    prim = (CsrBtAvCloseRes *) instData->recvMsgP;

    sHandle = prim->shandle;
    if (sHandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if (instData->stream[sHandle].streamState == CLOSING_S)
        {/* Reset stream info as it is internally generated AV Close indication,
            sent due to disconnect indication received from remote without AV Close */
            CsrBtAvClearStream(instData, sHandle);
        }
        else
        {
            if (prim->avResponse == CSR_BT_AV_ACCEPT)
            {
                msg_len = AVDTP_SIGNALLING_HDR_LEN;
                msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
                BUILD_MSG_HDR(msg, MSG_TYPE_ACP, prim->tLabel, AVDTP_CLOSE_SID);
                /* In the scenario where L2CAP disconnect indication is handled before AV close response, refrain from changing
                 * the stream state as the stream data is already cleared */
                if (instData->stream[sHandle].streamState != IDLE_S)
                {
                    instData->stream[sHandle].streamState = PEER_CLOSING_S;
                }
#ifndef CSR_STREAMS_ENABLE
                CsrBtAvClearStreamQ(&instData->stream[sHandle]);
#endif
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
                CsrBtAvStreamStartStopIndSend(instData, sHandle, FALSE);
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */
            }
            else
            {
                msg_len = AVDTP_SIGNALLING_HDR_LEN + 1;
                msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
                BUILD_MSG_HDR(msg, MSG_TYPE_REJ, prim->tLabel, AVDTP_CLOSE_SID);
                msg[2] = prim->avResponse;
            }
            csrBtAvSignalSend(instData, instData->stream[prim->shandle].conId, msg_len, msg);
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSuspendResHandler
 *
 *  DESCRIPTION
 *      Handle a response suspend procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvSuspendResHandler(av_instData_t *instData)
{
    CsrUint8 *msg;
    CsrUint16 msg_len;

    CsrBtAvSuspendRes *prim;

    prim = (CsrBtAvSuspendRes *) instData->recvMsgP;

    if(prim->list[0] < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        CsrUint8 s;

        if(prim->avResponse == CSR_BT_AV_ACCEPT)
        {
            msg_len = AVDTP_SIGNALLING_HDR_LEN;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_ACP, prim->tLabel, AVDTP_SUSPEND_SID);
        }
        else
        {
            msg_len = AVDTP_SIGNALLING_HDR_LEN + 2;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_REJ, prim->tLabel, AVDTP_SUSPEND_SID);
            msg[2] = (CsrUint8 ) (instData->stream[prim->reject_shandle].localSeid<<2);
            msg[3] = prim->avResponse;
        }
        csrBtAvSignalSend( instData, instData->stream[prim->list[0]].conId, msg_len, msg);

        s = 0;
        while (s < prim->listLength)
        {
            CsrUint8 idx = prim->list[s];
            if( (prim->avResponse != CSR_BT_AV_ACCEPT) && (prim->list[s] == prim->reject_shandle))
            {
                break;
            }
            if (instData->stream[idx].streamState != CLOSING_S)
            {/* If the stream is in closing state, just wait for disconnect indication and do not change the 
                stream data */
                instData->stream[idx].streamState = OPENED_S;
                /* Suspended but yet open.... let the CM know about the logical channel */
                CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL,
                     instData->con[instData->stream[idx].conId].remoteDevAddr,instData->stream[idx].mediaCid);
                CsrBtCmA2dpBitRateReqSend(instData->con[instData->stream[idx].conId].remoteDevAddr,idx,CSR_BT_A2DP_BIT_RATE_STREAM_SUSPENDED);
#ifndef CSR_STREAMS_ENABLE
                CsrBtAvClearStreamQ(&instData->stream[idx]);
#endif
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
                CsrBtAvStreamStartStopIndSend(instData, idx, FALSE);
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */
            }
            s++;
        }
    }
    CsrPmemFree(prim->list);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvAbortResHandler
 *
 *  DESCRIPTION
 *      Handle a response to abort procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvAbortResHandler(av_instData_t *instData)
{
    CsrBtAvAbortRes *prim;
    CsrUint8         sHandle;
    prim = (CsrBtAvAbortRes *) instData->recvMsgP;

    sHandle = prim->shandle;
    if (sHandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        CsrUint8 *msg = (CsrUint8 *) CsrPmemAlloc(AVDTP_SIGNALLING_HDR_LEN);
        BUILD_MSG_HDR(msg, MSG_TYPE_ACP, prim->tLabel, AVDTP_ABORT_SID);

        csrBtAvSignalSend( instData, instData->stream[prim->shandle].conId, AVDTP_SIGNALLING_HDR_LEN, msg);

        /* Before clearing the stream, check whether there are any commands pending... */
        if(instData->stream[prim->shandle].timerId != 0)
        {
            CsrUint16 pendingInfo = 0;
            
            CsrSchedTimerCancel(instData->stream[prim->shandle].timerId, &pendingInfo, NULL);
            timeOutHandler(pendingInfo ,instData);
            instData->stream[prim->shandle].timerId = 0;
        }
        else if (instData->stream[prim->shandle].streamState == CLOSING_S)
        {/* The close command timer was stopped, the disconnect request has been sent but the answer to that is pending.
            In the mean time, the remote has sent an ABORT and the application has accepted it: before loosing all information
            about the stream, make sure to send a close_cfm message back to the application! */
            CsrBtAvCloseCfmSend(instData, prim->shandle, instData->stream[prim->shandle].tLabel, 
                    CSR_BT_RESULT_CODE_AV_SUCCESS,
                    CSR_BT_SUPPLIER_AV);
        }

        if( (instData->stream[prim->shandle].streamState == OPENED_S)
            || (instData->stream[prim->shandle].streamState == STREAMING_S))
        {
            /* a stream channel is existing and no procedures are running (otherwise need
               to handle abort in context of the active procedure, i.e. no state change) */
            instData->stream[prim->shandle].streamState = PEER_ABORTING_S;
        }
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
        /* av stop indication */
        CsrBtAvStreamStartStopIndSend(instData, sHandle, FALSE);
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */
        CsrBtAvClearStream(instData, sHandle); /* Stream should be disconnected and cleared entirely */
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvDelayReportResHandler
 *
 *  DESCRIPTION
 *      Handle a response to delay report procedure
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvDelayReportResHandler(av_instData_t *instData)
{
    CsrUint8 *msg;
    CsrUint16 msg_len;

    CsrBtAvDelayReportRes *prim;

    prim = (CsrBtAvDelayReportRes *) instData->recvMsgP;

    if (prim->shandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if (prim->avResponse == CSR_BT_AV_ACCEPT)
        {
            msg_len = AVDTP_SIGNALLING_HDR_LEN;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_ACP, prim->tLabel, AVDTP_DELAYREPORT_SID);
        }
        else
        {
            msg_len = AVDTP_SIGNALLING_HDR_LEN + 1;
            msg = (CsrUint8 *) CsrPmemAlloc(msg_len);
            BUILD_MSG_HDR(msg, MSG_TYPE_REJ, prim->tLabel, AVDTP_DELAYREPORT_SID);
            msg[2] = prim->avResponse;
        }
        csrBtAvSignalSend( instData, instData->stream[prim->shandle].conId, msg_len, msg);
    }
}

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSetStreamInfoReqHandler
 *
 *  DESCRIPTION
 *      Set information related to AV stream
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvSetStreamInfoReqHandler(av_instData_t *instData)
{
    CsrBtAvSetStreamInfoReq* prim = (CsrBtAvSetStreamInfoReq *) instData->recvMsgP;
    CsrUint8 sHandle = prim->shandle;

    if (sHandle < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        CsrUint8 idx = instData->stream[sHandle].conId;
        if ((idx < CSR_BT_AV_MAX_NUM_CONNECTIONS)
                && (instData->con[idx].conState == CONNECTED_S))
        {
            if (instData->stream[sHandle].streamState > IDLE_S &&
                     instData->stream[sHandle].streamState < CLOSING_S)
            {
                instData->stream[sHandle].codecLocation = prim->sInfo.codecLocation;
            }
        }
    }
}
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvSecurityResFree
 *
 *  DESCRIPTION
 *      Free signal data, ignore request
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvSecurityResFree(av_instData_t *instData)
{
    CsrBtAvSecurityControlRes *prim;

    prim = (CsrBtAvSecurityControlRes *) instData->recvMsgP;

    CsrPmemFree(prim->contProtMethodData);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCmL2caDataIndHandler
 *
 *  DESCRIPTION
 *      Handle a CSR_BT_CM_L2CA_DATA_IND
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCmL2caDataIndHandler(av_instData_t *instData)
{
    CsrBtCmL2caDataInd *prim;
    CsrUintFast8 i;
    CsrUint8 sid, packet_type, msg_type;
#ifndef CSR_STREAMS_ENABLE
        CsrUint8 s;
#endif

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;
#ifndef CSR_STREAMS_ENABLE
    CsrBtCmL2caDataResSend(prim->btConnId);
#endif

    /* find out if it is stream data */
#ifndef CSR_STREAMS_ENABLE
    if( (s = findStreamFromCid(instData, prim->btConnId)) < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        CsrBtAvStreamDataInd *avPrim;
        avPrim = CsrPmemZalloc( sizeof(CsrBtAvStreamDataInd) );

        avPrim->type    = CSR_BT_AV_STREAM_DATA_IND;
        avPrim->shandle = (CsrUint8)s;
        avPrim->length  = prim->length;
        avPrim->data    = prim->payload;
        PUT_PRIM_TO_APP(avPrim);

        instData->stream[s].seqNo++;
        return;
    }
#endif
    if( prim->length < AVDTP_SIGNALLING_HEADER_MIN_LEN)
    {
        /* The command and response Signaling message for Continue and End 
           packets Minimum size is 1 byte, looks like received an Invalid 
           packet ignore it */
        return;
    }

    /* then try to find a signalling btConnId */
    for( i=0; i<CSR_BT_AV_MAX_NUM_CONNECTIONS; i++)
    {
        if(instData->con[i].signalCid == prim->btConnId)
        {
            break;
        }
    }

    if(i == CSR_BT_AV_MAX_NUM_CONNECTIONS)
    {
        /* ...data from unknown btConnId */
        CsrPmemFree(prim->payload);
        return;
    }

    /* ...signalling data to process */
    packet_type = (CsrUint8)(prim->payload[0] & PKT_TYPE_MASK);

    if( packet_type != PKT_TYPE_SINGLE)
    {
        /* packet is fragmented - defragment */
        switch(packet_type)
        {
            case PKT_TYPE_START:
                {
                    if( instData->fragmentHeadPtr != NULL)
                    {
                        /* discard the already stored fragment(s) */
                        CsrBtAvFreeFragments(&instData->fragmentHeadPtr, prim->btConnId);
                    }
                    CsrBtAvAllocFragmentHead( &instData->fragmentHeadPtr, prim->btConnId);
                    CsrBtAvAddFragment( instData->fragmentHeadPtr, prim->btConnId, prim->length, prim->payload);
                    return;
                }
            case PKT_TYPE_CONTINUE:
                {
                    if( instData->fragmentHeadPtr == NULL)
                    {
                        /* discard */
                        CsrPmemFree(prim->payload);
                        return;
                    }

                    CsrBtAvAddFragment( instData->fragmentHeadPtr, prim->btConnId, prim->length, prim->payload);
                    return;
                }
            case PKT_TYPE_END:
                {
                    if( instData->fragmentHeadPtr == NULL)
                    {
                        /* discard */
                        CsrPmemFree(prim->payload);
                        return;
                    }

                    prim->payload = CsrBtAvDefragment( &instData->fragmentHeadPtr, prim->btConnId, &prim->length, prim->payload);
                    if (prim->payload == NULL)
                    {
                        return;
                    }
                    break;
                }
        }
    }


    if( prim->length < AVDTP_SIGNALLING_HDR_LEN )
    {
        /* discard - too short to be a valid message */
        /* AVDTP header is invalid, ignore the message */
        CsrPmemFree(prim->payload);
        return;
    }

    msg_type = prim->payload[0] & MSG_TYPE_MASK;
    sid = prim->payload[1] & SIGNAL_ID_MASK;
#ifdef CSR_TARGET_PRODUCT_VM
    CSR_LOG_TEXT_INFO((CsrBtAvLto, 0, "CsrBtAvCmL2caDataIndHandler TLabel:0x%0x, MsgType:0x%0x, MESSAGE:AvdtpSignalId:0x%x, Connection Index:%d, BtConnId:0x%08x", 
                       (prim->payload[0] & TLABLE_MASK) >>  4,
                       prim->payload[0] & MSG_TYPE_MASK, 
                       sid,
                       i,
                       prim->btConnId));
#endif

    if(msg_type == MSG_TYPE_CMD)
    {
        if( (sid < AVDTP_DISCOVER_SID) || (sid > AVDTP_DELAYREPORT_SID))
        {
            /* invalid signal id, return general reject */
            CsrUint8 *msg;

            msg = (CsrUint8 *) CsrPmemAlloc(2);

            msg[0] = (CsrUint8)((prim->payload[0] & TLABLE_MASK) | MSG_TYPE_GENERAL_REJ);
            msg[1] = sid;
            
            csrBtAvSignalSend( instData,(CsrUint8) i, 2, msg);
        }
        else
        {
            avSignalCmdHandler[sid](instData,(CsrUint8) i);
        }
    }
    else if(msg_type == MSG_TYPE_ACP)
    {
        if( (sid >= AVDTP_DISCOVER_SID) && (sid <= AVDTP_DELAYREPORT_SID))
        {
            avSignalAcpHandler[sid](instData,(CsrUint8) i);
        }
    }
    else if(msg_type == MSG_TYPE_REJ)
    {
        if( (sid >= AVDTP_DISCOVER_SID) && (sid <= AVDTP_DELAYREPORT_SID))
        {
            avSignalRejHandler[sid](instData,(CsrUint8) i);
        }
    }
    /* else: general reject from peer, ignore and let the procedure time out */
    
    CsrPmemFree(prim->payload);

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCmL2caDataIndHandlerCleanup
 *
 *  DESCRIPTION
 *      Handle a CSR_BT_CM_L2CA_DATA_IND in clean-up state, free data
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCmL2caDataIndHandlerCleanup(av_instData_t *instData)
{
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;
#ifndef CSR_STREAMS_ENABLE
    CsrBtCmL2caDataResSend(prim->btConnId);
#endif
    CsrPmemFree(prim->payload);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvCmL2caDataCfmHandler
 *
 *  DESCRIPTION
 *      Handle a CSR_BT_CM_L2CA_DATA_CFM
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
void CsrBtAvCmL2caDataCfmHandler(av_instData_t *instData)
{
    CsrBtCmL2caDataCfm *prim;
    CsrUint8 conId;
#ifndef CSR_STREAMS_ENABLE
    CsrUint8 s;
#endif

    prim = (CsrBtCmL2caDataCfm *) instData->recvMsgP;
#ifndef CSR_STREAMS_ENABLE
    if( (s = findStreamFromCid(instData, prim->btConnId)) < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        av_stream_info_t *stream = &instData->stream[s];

        stream->sending = FALSE;

        if( stream->fifoQ[stream->fifoQOut] != NULL)
        {
            if (stream->streamState == STREAMING_S)
            {/* Only stream data if streaming; do not stream if configuring or suspended */
                CsrSchedMessagePut(CSR_BT_CM_IFACEQUEUE, CSR_BT_CM_PRIM, stream->fifoQ[stream->fifoQOut]);
                stream->sending = TRUE;
                stream->fifoQ[stream->fifoQOut] = NULL;
                stream->fifoQOut = (stream->fifoQOut + 1) % CSR_BT_AV_MEDIA_BUFFER_SIZE;
            }
        }
        else if (instData->qosInterval == 0)
        {
            if(stream->sentBufFullInd)
            {
                /* buffer empty after having been full */
                CsrBtAvQosIndSend(instData, s, 0);
                stream->sentBufFullInd = FALSE;
            }
        }
        return;
    }
#endif
    if((conId = findConFromCid(instData, prim->btConnId)) < CSR_BT_AV_MAX_NUM_CONNECTIONS)
    {
        av_connection_info_t *con;

        con = &instData->con[conId];

        con->sending = FALSE;

        if(con->qFirst != NULL)
        {
            q_element_t *tmp = con->qFirst;
            con->qFirst = con->qFirst->next;

#ifdef CSR_STREAMS_ENABLE
            CsrStreamsDataSend(CM_GET_UINT16ID_FROM_BTCONN_ID(con->signalCid),
                               L2CAP_ID,
                               tmp->length,
                               tmp->data);
#else
            CsrBtCml2caDataReqSend(con->signalCid,
                                   tmp->length,
                                   tmp->data,
                                   CSR_BT_CM_CONTEXT_UNUSED);
#endif
            con->sending = TRUE;

            CsrPmemFree(tmp);
        }
    }
}

/* handle CMD messages from peer */
/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvDiscoverCmdHandler
 *
 *  DESCRIPTION
 *      Handle cmd from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvDiscoverCmdHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrBtAvDiscoverInd *avPrim;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    avPrim = (CsrBtAvDiscoverInd *) CsrPmemAlloc(sizeof(CsrBtAvDiscoverInd));
    avPrim->type         = CSR_BT_AV_DISCOVER_IND;
    avPrim->connectionId = conId;
    avPrim->tLabel       = prim->payload[0]>>4;
    PUT_PRIM_TO_APP(avPrim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvGetCapabilitiesCmdHandler
 *
 *  DESCRIPTION
 *      Handle cmd from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvGetCapabilitiesCmdHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    if( prim->length != 3 )
    {
        /* invalid message length, return reject */
        csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_LENGTH, 0);
    }
    else
    {
        CsrBtAvGetCapabilitiesInd *avPrim;

        avPrim = (CsrBtAvGetCapabilitiesInd *) CsrPmemAlloc(sizeof(CsrBtAvGetCapabilitiesInd));
        avPrim->type         = CSR_BT_AV_GET_CAPABILITIES_IND;
        avPrim->connectionId = conId;
        avPrim->tLabel       = prim->payload[0]>>4;
        avPrim->acpSeid      = prim->payload[2]>>2;
        PUT_PRIM_TO_APP(avPrim);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvSetConfigurationCmdHandler
 *
 *  DESCRIPTION
 *      Handle cmd from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvSetConfigurationCmdHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    if( prim->length < 4 )
    {
        /* invalid message length, return reject */
        csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_LENGTH, 0);
    }
    else
    {
        CsrUintFast8 s;
        for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
        {
            if(instData->stream[s].streamState == IDLE_S)
            {
                CsrUint8 *servCap;
                CsrBtAvSetConfigurationInd *avPrim;

                instData->stream[s].streamState = CONFIGURING_S;
                instData->stream[s].conId = conId;
                instData->stream[s].localSeid = prim->payload[2]>>2;
                instData->stream[s].remoteSeid = prim->payload[3]>>2;

                avPrim = (CsrBtAvSetConfigurationInd *) CsrPmemAlloc(sizeof(CsrBtAvSetConfigurationInd));
                avPrim->type            = CSR_BT_AV_SET_CONFIGURATION_IND;
                avPrim->connectionId    = conId;
                avPrim->tLabel          = prim->payload[0]>>4;
                avPrim->shandle         = (CsrUint8)s;
                avPrim->acpSeid         = instData->stream[s].localSeid;
                avPrim->intSeid         = instData->stream[s].remoteSeid;
                avPrim->servCapLen      = prim->length - 4;

                servCap = CsrPmemAlloc(avPrim->servCapLen);
                SynMemCpyS(servCap, avPrim->servCapLen, &(prim->payload[4]), avPrim->servCapLen);
                
                /* Now calculate the bit rate if possible */
                instData->stream[s].bitRate = csrBtAvCalculateStreamBitRate(instData,servCap,avPrim->servCapLen,(CsrUint8)s);
                
                avPrim->servCapData     = servCap;

                PUT_PRIM_TO_APP(avPrim);
                break;
            }
        }

        if( s == CSR_BT_AV_MAX_NUM_STREAMS)
        {
            csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_INSUFFICIENT_RESOURCES, 1);
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvGetConfigurationCmdHandler
 *
 *  DESCRIPTION
 *      Handle cmd from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvGetConfigurationCmdHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    if( prim->length != 3 )
    {
        /* invalid message length, return reject */
        csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_LENGTH, 0);
    }
    else
    {
        CsrUint8 s;

        if( (s = findStreamFromLocalSeid(instData, (CsrUint8) (prim->payload[2]>>2), conId)) < CSR_BT_AV_MAX_NUM_STREAMS)
        {
            if( instData->stream[s].streamState >= CONFIGURED_S)
            {
                CsrBtAvGetConfigurationInd *avPrim;
                avPrim = (CsrBtAvGetConfigurationInd *) CsrPmemAlloc(sizeof(CsrBtAvGetConfigurationInd));
                avPrim->type         = CSR_BT_AV_GET_CONFIGURATION_IND;
                avPrim->shandle      = s;
                avPrim->tLabel       = prim->payload[0]>>4;
                PUT_PRIM_TO_APP(avPrim);
            }
            else
            {
                csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_STATE, 0);
            }
        }
        else
        {
            csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_ACP_SEID, 0);
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvReconfigureCmdHandler
 *
 *  DESCRIPTION
 *      Handle cmd from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvReconfigureCmdHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    if( prim->length < 3 )
    {
        /* invalid message length, return reject */
        csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_LENGTH, 0);
    }
    else
    {
        CsrUint8 s;

        if( (s = findStreamFromLocalSeid(instData, (CsrUint8) (prim->payload[2]>>2), conId)) < CSR_BT_AV_MAX_NUM_STREAMS)
        {
            if( instData->stream[s].streamState == OPENED_S)
            {
                CsrUint8 *servCap;
                CsrBtAvReconfigureInd *avPrim;
                avPrim = (CsrBtAvReconfigureInd *) CsrPmemAlloc(sizeof(CsrBtAvReconfigureInd));
                avPrim->type         = CSR_BT_AV_RECONFIGURE_IND;
                avPrim->shandle      = s;
                avPrim->tLabel       = prim->payload[0]>>4;
                avPrim->servCapLen   = prim->length - 3;

                servCap = CsrPmemAlloc(avPrim->servCapLen);
                SynMemCpyS(servCap, avPrim->servCapLen, &(prim->payload[3]), avPrim->servCapLen);
                /* Now calculate the bit rate if possible */
                instData->stream[s].bitRate = csrBtAvCalculateStreamBitRate(instData,servCap,avPrim->servCapLen,s);
                avPrim->servCapData     = servCap;

                PUT_PRIM_TO_APP(avPrim);
            }
            else
            {
                csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_STATE, 0);
            }
        }
        else
        {
            csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_ACP_SEID, 0);
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvOpenCmdHandler
 *
 *  DESCRIPTION
 *      Handle cmd from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvOpenCmdHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    if( prim->length != 3 )
    {
        /* invalid message length, return reject */
        csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_LENGTH, 0);
    }
    else
    {
        CsrUint8 s;

        if( (s = findStreamFromLocalSeid(instData, (CsrUint8) (prim->payload[2]>>2), conId)) < CSR_BT_AV_MAX_NUM_STREAMS)
        {
            if( instData->stream[s].streamState == CONFIGURED_S)
            {
                CsrBtAvOpenInd *avPrim;
                avPrim = (CsrBtAvOpenInd *) CsrPmemAlloc(sizeof(CsrBtAvOpenInd));
                avPrim->type         = CSR_BT_AV_OPEN_IND;
                avPrim->shandle      = s;
                avPrim->tLabel       = prim->payload[0]>>4;
                PUT_PRIM_TO_APP(avPrim);
                return;
            }
        }

        csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_STATE, 0);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvStartCmdHandler
 *
 *  DESCRIPTION
 *      Handle cmd from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvStartCmdHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    if (prim->length < 3)
    {
        /* invalid message length, return reject */
        csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_LENGTH, 0);
    }
    else
    {
        CsrUint8 s;

        if ((s = findStreamFromLocalSeid(instData, (CsrUint8) (prim->payload[2]>>2), conId)) < CSR_BT_AV_MAX_NUM_STREAMS)
        {
            if ((instData->stream[s].streamState == OPENED_S) ||
                (instData->stream[s].streamState == PEER_OPENING_S) ||
                (instData->stream[s].streamState == STARTING_S))
            {
                CsrUintFast8 i;
                CsrBtAvStartInd *avPrim;

                avPrim = (CsrBtAvStartInd *) CsrPmemAlloc(sizeof(CsrBtAvStartInd));
                avPrim->type            = CSR_BT_AV_START_IND;
                avPrim->tLabel          = (CsrUint8)(*prim->payload >> 4);
                avPrim->listLength      = (CsrUint8)(prim->length - 2); /* Length - header */
                avPrim->list            = CsrPmemAlloc(avPrim->listLength);

                for(i=0; i < avPrim->listLength; i++)
                {
                    avPrim->list[i] = findStreamFromLocalSeid(instData,
                                                              (CsrUint8)(prim->payload[2 + i] >> 2),
                                                              conId);
                }

                if (instData->stream[s].streamState == OPENED_S || instData->stream[s].streamState == STARTING_S)
                {
                    PUT_PRIM_TO_APP(avPrim);
                }
                else
                {
                    /* Media channel is still not created. Defer sending CSR_BT_AV_START_IND
                     * to application. Send it once AV receives Connect Accept Cfm from CM i.e.
                     * when streamState moves to OPENED_S */
                    instData->recvMsgP = avPrim;

                    /* Store the Start indication in the save queue and send it later using
                     * AvSendPendingUpstreamMessages() */
                    CsrBtAvSaveMessage(instData);

                    /* Point the recvMsgP back to the old prim msg */
                    instData->recvMsgP = prim;
                }
            }
            else
            {
                csrBtAvCommonSignalReject(instData,
                                          conId,
                                          CSR_BT_RESULT_CODE_A2DP_BAD_STATE,
                                          prim->payload[2]);
            }
        }
        else
        {
            csrBtAvCommonSignalReject(instData,
                                      conId,
                                      CSR_BT_RESULT_CODE_A2DP_BAD_ACP_SEID,
                                      prim->payload[2]);
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvCloseCmdHandler
 *
 *  DESCRIPTION
 *      Handle cmd from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvCloseCmdHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    if( prim->length != 3 )
    {
        /* invalid message length, return reject */
        csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_LENGTH, 0);
    }
    else
    {
        CsrUint8 s;

        if( (s = findStreamFromLocalSeid(instData, (CsrUint8) (prim->payload[2]>>2), conId)) < CSR_BT_AV_MAX_NUM_STREAMS)
        {
            CsrBtAvCloseInd *avPrim;
            avPrim = (CsrBtAvCloseInd *) CsrPmemAlloc(sizeof(CsrBtAvCloseInd));
            avPrim->type         = CSR_BT_AV_CLOSE_IND;
            avPrim->shandle      = s;
            avPrim->tLabel       = prim->payload[0]>>4;

            if (instData->stream[s].streamState == OPENING_S)
            {
                /* Received AV Close indication before the library has informed the application
                 * of the AV OPEN_CFM. Defer the close indication for later */
                instData->recvMsgP = avPrim;

                /* Store the close indication in the save queue and send it later using
                 * AvSendPendingUpstreamMessages() */
                CsrBtAvSaveMessage(instData);

                /* Point the recvMsgP back to the old prim msg */
                instData->recvMsgP = prim;
            }
            else
            {
                PUT_PRIM_TO_APP(avPrim);
            }
        }
        else
        {
            csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_ACP_SEID, 0);
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvSuspendCmdHandler
 *
 *  DESCRIPTION
 *      Handle cmd from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvSuspendCmdHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    if( prim->length < 3 )
    {
        /* invalid message length, return reject */
        csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_LENGTH, 0);
    }
    else
    {
        CsrUint8 s;

        if( (s = findStreamFromLocalSeid(instData, (CsrUint8) (prim->payload[2]>>2), conId)) < CSR_BT_AV_MAX_NUM_STREAMS)
        {
            if( instData->stream[s].streamState == STREAMING_S)
            {
                CsrUintFast8 i;
                CsrBtAvSuspendInd *avPrim;

                avPrim = (CsrBtAvSuspendInd *) CsrPmemAlloc(sizeof(CsrBtAvSuspendInd) + ((prim->length - 2) + 2));
                avPrim->type            = CSR_BT_AV_SUSPEND_IND;
                avPrim->tLabel          = (CsrUint8)(*prim->payload >> 4);
                avPrim->listLength      = (CsrUint8)(prim->length - 2);    /* Length - header */
                avPrim->list            = CsrPmemAlloc(avPrim->listLength);

                for(i=0; i < avPrim->listLength; i++)
                {
                    avPrim->list[i] = findStreamFromLocalSeid(instData,
                                                              (CsrUint8)(prim->payload[2 + i] >> 2),
                                                              conId);
                }

                PUT_PRIM_TO_APP(avPrim);
                return;
            }
             else if (instData->stream[s].streamState == CLOSING_S)
            {
                CsrBtAvCloseCfmSend(instData, s, instData->stream[s].tLabel, 
                    CSR_BT_RESULT_CODE_AV_SUCCESS,
                    CSR_BT_SUPPLIER_AV);
            }
        }

        csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_STATE, prim->payload[2]);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvAbortCmdHandler
 *
 *  DESCRIPTION
 *      Handle cmd from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvAbortCmdHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    if( prim->length != 3 )
    {
        /* invalid message length, return reject */
        csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_LENGTH, 0);
    }
    else
    {
        CsrUint8 s;

        if( (s = findStreamFromLocalSeid(instData, (CsrUint8) (prim->payload[2]>>2), conId)) < CSR_BT_AV_MAX_NUM_STREAMS)
        {
            CsrBtAvAbortInd *avPrim;
            avPrim = (CsrBtAvAbortInd *) CsrPmemAlloc(sizeof(CsrBtAvAbortInd));
            avPrim->type         = CSR_BT_AV_ABORT_IND;
            avPrim->shandle      = s;
            avPrim->tLabel       = prim->payload[0]>>4;
            PUT_PRIM_TO_APP(avPrim);
        }
        /* ...else: ignore the abort cmd when the seid is invalid */
    }
}

#ifdef INSTALL_AV_SECURITY_CONTROL
/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvSecurityControlCmdHandler
 *
 *  DESCRIPTION
 *      Handle cmd from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvSecurityControlCmdHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    if( prim->length < 3 )
    {
        /* invalid message length, return reject */
        csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_LENGTH, 0);
    }
    else
    {
        CsrUint8 s;

        if( (s = findStreamFromLocalSeid(instData, (CsrUint8) (prim->payload[2]>>2), conId)) < CSR_BT_AV_MAX_NUM_STREAMS)
        {
            CsrBtAvSecurityControlInd *avPrim;
            avPrim = (CsrBtAvSecurityControlInd *) CsrPmemAlloc(sizeof(CsrBtAvSecurityControlInd));
            avPrim->type         = CSR_BT_AV_SECURITY_CONTROL_IND;
            avPrim->shandle      = s;
            avPrim->tLabel       = prim->payload[0]>>4;
            avPrim->contProtMethodData = NULL;
            avPrim->contProtMethodLen   = prim->length - 3;

            if( avPrim->contProtMethodLen != 0 )
            {
                CsrUint8 *contProtMethodData;

                contProtMethodData = CsrPmemAlloc(avPrim->contProtMethodLen);
                SynMemCpyS(contProtMethodData, avPrim->contProtMethodLen, &(prim->payload[3]), avPrim->contProtMethodLen);

                avPrim->contProtMethodData = contProtMethodData;
            }

            PUT_PRIM_TO_APP(avPrim);
        }
        else
        {
            csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_ACP_SEID, 0);
        }
    }
}
#endif /* INSTALL_AV_SECURITY_CONTROL */

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvDelayReportCmdHandler
 *
 *  DESCRIPTION
 *      Handle cmd from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvDelayReportCmdHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    if( prim->length < 3 )
    {
        /* invalid message length, return reject */
        csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_LENGTH, 0);
    }
    else
    {
        CsrUint8 s = findStreamFromLocalSeid(instData, (CsrUint8) (prim->payload[2]>>2), conId);

        if( s < CSR_BT_AV_MAX_NUM_STREAMS)
        {
            CsrBtAvDelayReportInd *avPrim;
            
            avPrim = (CsrBtAvDelayReportInd *) CsrPmemAlloc(sizeof(CsrBtAvDelayReportInd));
            avPrim->type         = CSR_BT_AV_DELAY_REPORT_IND;
            avPrim->shandle      = s;
            avPrim->tLabel       = (CsrUint8)(prim->payload[0]>>4);
            avPrim->delay        = (CsrUint16) (((CsrUint16)(prim->payload[3] << 8)) | (prim->payload[4]));
            
            PUT_PRIM_TO_APP(avPrim);
        }
        else
        {
            csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_ACP_SEID, 0);
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvGetAllCapabilitiesCmdHandler
 *
 *  DESCRIPTION
 *      Handle cmd from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvGetAllCapabilitiesCmdHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    if( prim->length != 3 )
    {
        /* invalid message length, return reject */
        csrBtAvCommonSignalReject(instData, conId, CSR_BT_RESULT_CODE_A2DP_BAD_LENGTH, 0);
    }
    else
    {
        CsrBtAvGetAllCapabilitiesInd *avPrim;

        avPrim = (CsrBtAvGetAllCapabilitiesInd *) CsrPmemAlloc(sizeof(CsrBtAvGetAllCapabilitiesInd));
        avPrim->type         = CSR_BT_AV_GET_ALL_CAPABILITIES_IND;
        avPrim->connectionId = conId;
        avPrim->tLabel       = prim->payload[0]>>4;
        avPrim->acpSeid      = prim->payload[2]>>2;
        PUT_PRIM_TO_APP(avPrim);

        /* if the peer is able to send this cmd, it must be supporting AVDTP v1.3+ */
        if( instData->con[conId].remoteAVDTPVersion == DEFAULT_AVDTP_VERSION )
        {
            instData->con[conId].remoteAVDTPVersion = AVDTP_1_3_VERSION;
        }
    }
}


/* handle ACP messages from peer */
/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvDiscoverAcpHandler
 *
 *  DESCRIPTION
 *      Handle accept response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvDiscoverAcpHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 list_length, i, *data;
    CsrBtCmL2caDataInd *prim;

    CsrBtAvDiscoverCfm *avPrim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;
    if( (instData->con[conId].pendingSigProc == AVDTP_DISCOVER_SID) && (instData->con[conId].timerId != 0x00) )
    {
        CsrSchedTimerCancel(instData->con[conId].timerId, NULL, NULL);
        instData->con[conId].timerId = 0;
        instData->con[conId].pendingSigProc = 0;

        list_length = (CsrUint8)((prim->length - 2) / 2);
        data = &prim->payload[2]; /* point to the first seid info */

        avPrim = (CsrBtAvDiscoverCfm *) CsrPmemAlloc(sizeof (CsrBtAvDiscoverCfm));
        avPrim->type                = CSR_BT_AV_DISCOVER_CFM;
        avPrim->connectionId        = conId;
        avPrim->avResultCode        = CSR_BT_RESULT_CODE_AV_SUCCESS;
        avPrim->avResultSupplier    = CSR_BT_SUPPLIER_AV;
        avPrim->tLabel              = (CsrUint8)(*prim->payload >> 4);
        avPrim->seidInfoCount       = list_length;
        avPrim->seidInfo            = (CsrBtAvSeidInfo*)CsrPmemAlloc(sizeof(CsrBtAvSeidInfo)*list_length);

        i = 0;
        while(list_length)
        {
            avPrim->seidInfo[i].acpSeid   = (CsrUint8)(*data >> 2);
            avPrim->seidInfo[i].inUse     = (CsrBool)((*data >> 1) & 0x01);
            data++;
            avPrim->seidInfo[i].mediaType = (CsrUint8)(*data >> 4);
            avPrim->seidInfo[i].sepType   = (CsrUint8)((*data >> 3) & 0x01);
            data++;
            i++;
            list_length--;
        }

        PUT_PRIM_TO_APP(avPrim);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvGetCapabilitiesAcpHandler
 *
 *  DESCRIPTION
 *      Handle accept response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvGetCapabilitiesAcpHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrBtCmL2caDataInd *prim;

    CsrBtAvGetCapabilitiesCfm *avPrim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;
    
    if( ((instData->con[conId].pendingSigProc == AVDTP_GET_CAPABILITIES_SID)
            || (instData->con[conId].pendingSigProc == AVDTP_GET_ALL_CAPABILITIES_SID))
        && (instData->con[conId].timerId != 0x00) )
    {
        CsrSchedTimerCancel(instData->con[conId].timerId, NULL, NULL);
        instData->con[conId].timerId = 0;
        instData->con[conId].pendingSigProc = 0;
        
        avPrim = (CsrBtAvGetCapabilitiesCfm*) CsrPmemAlloc(sizeof (CsrBtAvGetCapabilitiesCfm));

        avPrim->type            = CSR_BT_AV_GET_CAPABILITIES_CFM;
        avPrim->connectionId    = conId;
        avPrim->avResultCode      = CSR_BT_RESULT_CODE_AV_SUCCESS;
        avPrim->avResultSupplier  = CSR_BT_SUPPLIER_AV;
        avPrim->tLabel          = (CsrUint8)(*prim->payload >> 4);

        avPrim->servCapData     = CsrPmemAlloc(prim->length - 2);
        avPrim->servCapLen      = (CsrUint16)(prim->length - 2);
        SynMemCpyS(avPrim->servCapData, avPrim->servCapLen, &(prim->payload[2]), avPrim->servCapLen);

        PUT_PRIM_TO_APP(avPrim);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvSetConfigurationAcpHandler
 *
 *  DESCRIPTION
 *      Handle accept response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvSetConfigurationAcpHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 s;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if( CsrBtAvStopStreamTimer(instData, s) )
        {
            CsrBtAvSetConfigurationCfmSend(instData, conId, (CsrUint8)(prim->payload[0]>>4), s, 0,
                CSR_BT_RESULT_CODE_AV_SUCCESS,
                CSR_BT_SUPPLIER_AV);

            instData->stream[s].streamState = CONFIGURED_S;
            instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */
        }
    }
}

#ifdef INSTALL_AV_GET_CONFIGURATION
/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvGetConfigurationAcpHandler
 *
 *  DESCRIPTION
 *      Handle accept response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvGetConfigurationAcpHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 s;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        CsrUint8 *servCapData;
        if( CsrBtAvStopStreamTimer(instData, s) )
        {
            servCapData = CsrPmemAlloc(prim->length - 2);
            SynMemCpyS(servCapData, prim->length - 2, &(prim->payload[2]), prim->length - 2);
            instData->stream[s].bitRate = csrBtAvCalculateStreamBitRate(instData, servCapData,(CsrUint16)(prim->length - 2),s);;
            CsrBtAvGetConfigurationCfmSend(instData, (CsrUint8)(prim->payload[0]>>4), s, (CsrUint16)(prim->length - 2), servCapData,
                CSR_BT_RESULT_CODE_AV_SUCCESS,
                CSR_BT_SUPPLIER_AV);
            instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */
        }
    }
}
#endif /* INSTALL_AV_GET_CONFIGURATION */

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvReconfigureAcpHandler
 *
 *  DESCRIPTION
 *      Handle accept response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvReconfigureAcpHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 s;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if( CsrBtAvStopStreamTimer(instData, s) )
        {
            CsrBtAvReconfigureCfmSend(instData, (CsrUint8)(prim->payload[0]>>4), s, 0,
                CSR_BT_RESULT_CODE_AV_SUCCESS,
                CSR_BT_SUPPLIER_AV);
            instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvOpenAcpHandler
 *
 *  DESCRIPTION
 *      Handle accept response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvOpenAcpHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 s;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if( CsrBtAvStopStreamTimer(instData, s) )
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

            CsrBtCml2caConnectHighDataPriorityReqSend(CSR_BT_AV_IFACEQUEUE,
                                                      instData->con[conId].remoteDevAddr,
                                                      CSR_BT_AVDTP_PSM,
                                                      CSR_BT_AVDTP_PSM,
                                                      CsrBtAvGetMtuSize(conId),
                                                      secOutgoing,
                                                      CSR_BT_CM_PRIORITY_HIGH,
                                                      CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                                             CSR_BT_AV_DEFAULT_ENC_KEY_SIZE_VAL));

            instData->stream[s].streamState = OPENING_S;
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvStartAcpHandler
 *
 *  DESCRIPTION
 *      Handle accept response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvStartAcpHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUintFast8 s;
    CsrUint8 index, tLabel;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;
    tLabel = (CsrUint8)(*prim->payload >> 4);

    index = findStreamFromTLabel(instData, conId, tLabel);
    if (index < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        for (s=index;s<CSR_BT_AV_MAX_NUM_STREAMS;s++)
        {
            if( (instData->stream[s].conId == conId)
                && (instData->stream[s].streamState == STARTING_S)
                && (instData->stream[s].tLabel == tLabel) )
            {
                CsrBtAvStopStreamTimer(instData, (CsrUint8) s);
                
                CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_STREAM_CHANNEL,instData->con[conId].remoteDevAddr,instData->stream[s].mediaCid);
                instData->stream[s].streamState = STREAMING_S;
                instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
                CsrBtAvStreamStartStopIndSend(instData, s, TRUE);
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

                CsrBtCmA2dpBitRateReqSend(instData->con[conId].remoteDevAddr,(CsrUint8)s, instData->stream[s].bitRate);
            }
        }
        
        CsrBtAvStartCfmSend(instData, index, tLabel, CSR_BT_RESULT_CODE_AV_SUCCESS, CSR_BT_SUPPLIER_AV);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvCloseAcpHandler
 *
 *  DESCRIPTION
 *      Handle accept response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvCloseAcpHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 s;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if( CsrBtAvStopStreamTimer(instData, s) )
        {
            CsrBtCml2caDisconnectReqSend(instData->stream[s].mediaCid);
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
            /* av stop indication */
            CsrBtAvStreamStartStopIndSend(instData, s, FALSE);
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */
            instData->stream[s].streamState = CLOSING_S;
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvSuspendAcpHandler
 *
 *  DESCRIPTION
 *      Handle accept response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvSuspendAcpHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUintFast8 s;
    CsrUint8 index, tLabel;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;
    tLabel = (CsrUint8)(*prim->payload >> 4);

    index = findStreamFromTLabel(instData, conId, tLabel);
    if (index < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        for (s=index;s<CSR_BT_AV_MAX_NUM_STREAMS;s++)
        {
            if( (instData->stream[s].conId == conId)
                && (instData->stream[s].streamState == STREAMING_S)
                && (instData->stream[s].tLabel == tLabel) )
            {
                CsrBtAvStopStreamTimer(instData, (CsrUint8) s);
                
                instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */
                instData->stream[s].streamState = OPENED_S;
#ifndef CSR_STREAMS_ENABLE
                CsrBtAvClearStreamQ(&instData->stream[s]);
#endif
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
                CsrBtAvStreamStartStopIndSend(instData, s, FALSE);
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

                CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL,
                    instData->con[instData->stream[s].conId].remoteDevAddr,instData->stream[s].mediaCid);
                CsrBtCmA2dpBitRateReqSend(instData->con[instData->stream[s].conId].remoteDevAddr,
                    (CsrUint8)s,CSR_BT_A2DP_BIT_RATE_STREAM_SUSPENDED);
            }
        }
        
        CsrBtAvSuspendCfmSend(instData, index, tLabel,
            CSR_BT_RESULT_CODE_AV_SUCCESS,
            CSR_BT_SUPPLIER_AV);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvAbortAcpHandler
 *
 *  DESCRIPTION
 *      Handle accept response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvAbortAcpHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 s;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if( CsrBtAvStopStreamTimer(instData, s) )
        {
            if( instData->stream[s].mediaCid != 0)
            {
                CsrBtCml2caDisconnectReqSend(instData->stream[s].mediaCid);
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
                /* av stop indication */
                CsrBtAvStreamStartStopIndSend(instData, s, FALSE);
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */
                instData->stream[s].streamState = ABORTING_S;
            }
            else
            {
                CsrBtAvAbortCfmSend(instData, s, instData->stream[s].tLabel);
                if (instData->stream[s].streamState == ABORT_REQUESTED_S)
                {/* ABORT response is received after media channel is disconnected by remote */
                    CsrBtAvClearStream(instData,(CsrUint8) s);
                }                
            }
        }
    }
}

#ifdef INSTALL_AV_SECURITY_CONTROL
/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvSecurityControlAcpHandler
 *
 *  DESCRIPTION
 *      Handle accept response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvSecurityControlAcpHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 s;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        CsrUint16 data_length;
        CsrUint8 *contProtMethodData = NULL;

        if( CsrBtAvStopStreamTimer(instData, s) )
        {
            data_length = prim->length - 2;
            if( data_length > 0)
            {
                contProtMethodData = (CsrUint8 *) CsrPmemAlloc(data_length);
                SynMemCpyS(contProtMethodData, data_length, &prim->payload[2], data_length);
            }
            CsrBtAvSecurityControlCfmSend(instData, s, (CsrUint8)(prim->payload[0]>>4), data_length, contProtMethodData,
                CSR_BT_RESULT_CODE_AV_SUCCESS,
                CSR_BT_SUPPLIER_AV);
            instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */
        }
    }
}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvDelayReportAcpHandler
 *
 *  DESCRIPTION
 *      Handle accept response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvDelayReportAcpHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 s;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        CsrBtAvStopStreamTimer(instData, s);
            
        CsrBtAvDelayReportCfmSend(instData, s, (CsrUint8)(prim->payload[0]>>4),
                                    CSR_BT_RESULT_CODE_AV_SUCCESS,
                                    CSR_BT_SUPPLIER_AV);
        instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */
    }
}


/* handle REJ messages from peer */
/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvDiscoverRejHandler
 *
 *  DESCRIPTION
 *      Handle reject response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvDiscoverRejHandler(av_instData_t * instData, CsrUint8 conId)
{
    if( (instData->con[conId].pendingSigProc == AVDTP_DISCOVER_SID)
        && (instData->con[conId].timerId != 0x00) )
    {
        CsrBtCmL2caDataInd *prim;

        prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;
    
        CsrSchedTimerCancel(instData->con[conId].timerId, NULL, NULL);
        instData->con[conId].timerId = 0;
        instData->con[conId].pendingSigProc = 0;

        CsrBtAvDiscoverCfmSendReject(instData, conId, (CsrUint8)(prim->payload[0]>>4), prim->payload[2],
            CSR_BT_SUPPLIER_A2DP);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvGetCapabilitiesRejHandler
 *
 *  DESCRIPTION
 *      Handle reject response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvGetCapabilitiesRejHandler(av_instData_t * instData, CsrUint8 conId)
{
    if( ((instData->con[conId].pendingSigProc == AVDTP_GET_CAPABILITIES_SID)
            || (instData->con[conId].pendingSigProc == AVDTP_GET_ALL_CAPABILITIES_SID))
        && (instData->con[conId].timerId != 0x00) )
    {
        CsrBtCmL2caDataInd *prim;

        prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

        CsrSchedTimerCancel(instData->con[conId].timerId, NULL, NULL);
        instData->con[conId].timerId = 0;
        instData->con[conId].pendingSigProc = 0;

        CsrBtAvGetCapabilitiesCfmSendReject(instData, conId, (CsrUint8)(prim->payload[0]>>4),
            prim->payload[2], CSR_BT_SUPPLIER_A2DP);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvSetConfigurationRejHandler
 *
 *  DESCRIPTION
 *      Handle reject response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvSetConfigurationRejHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 s;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if( CsrBtAvStopStreamTimer(instData, s) )
        {
            CsrBtAvSetConfigurationCfmSend(instData, conId, (CsrUint8)(prim->payload[0]>>4), s, prim->payload[2],
                prim->payload[3], CSR_BT_SUPPLIER_A2DP);

            CsrBtAvClearStream(instData, s);
        }
    }
}

#ifdef INSTALL_AV_GET_CONFIGURATION
/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvGetConfigurationRejHandler
 *
 *  DESCRIPTION
 *      Handle reject response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvGetConfigurationRejHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 s;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if( CsrBtAvStopStreamTimer(instData, s) )
        {
            CsrBtAvGetConfigurationCfmSend(instData, (CsrUint8)(prim->payload[0]>>4), s, 0, NULL,
                prim->payload[2],
                CSR_BT_SUPPLIER_A2DP);
            instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */
        }
    }
}
#endif /* INSTALL_AV_GET_CONFIGURATION */

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvReconfigureRejHandler
 *
 *  DESCRIPTION
 *      Handle reject response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvReconfigureRejHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 s;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if( CsrBtAvStopStreamTimer(instData, s) )
        {
            CsrBtAvReconfigureCfmSend(instData, (CsrUint8)(prim->payload[0]>>4), s, prim->payload[2],
                prim->payload[3],
                CSR_BT_SUPPLIER_A2DP);
            instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvOpenRejHandler
 *
 *  DESCRIPTION
 *      Handle reject response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvOpenRejHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 s;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if( CsrBtAvStopStreamTimer(instData, s) )
        {
            CsrBtAvOpenCfmSend(instData, s, (CsrUint8)(prim->payload[0]>>4),
                prim->payload[2],
                CSR_BT_SUPPLIER_A2DP);
            instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvStartRejHandler
 *
 *  DESCRIPTION
 *      Handle reject response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvStartRejHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUintFast8 s, tmp, tLabel;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;
    
    tLabel = (CsrUint8)(prim->payload[0]>>4);
    tmp = CSR_BT_AV_MAX_NUM_STREAMS;

    for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
    {
        if((conId == instData->stream[s].conId)
        && (tLabel == instData->stream[s].tLabel) 
        && (instData->stream[s].streamState == STARTING_S))
        {
            break;
        }
    }
    
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if(instData->stream[s].timerId != 0x00)
        {
            for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
            {
                tmp = (CsrUint8)(prim->payload[2]>>2);
                if( (tmp == instData->stream[s].remoteSeid)
                    && (conId == instData->stream[s].conId)
                    && (tLabel == instData->stream[s].tLabel) )
                {
                    CsrBtAvStartCfmSend(instData,(CsrUint8) s, (CsrUint8)(prim->payload[0]>>4),
                        prim->payload[3],
                        CSR_BT_SUPPLIER_A2DP);
                    tmp = s;
                    break;
                }
            }

            for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
            {
                if ( (conId == instData->stream[s].conId)
                     && (tLabel == instData->stream[s].tLabel)
                     && (instData->stream[s].streamState == STARTING_S) )
                {
                    if( s<tmp )
                    {
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
                        CsrBtAvStreamStartStopIndSend(instData, s, TRUE);
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */
                        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_STREAM_CHANNEL, instData->con[conId].remoteDevAddr, instData->stream[s].mediaCid);
                        instData->stream[s].streamState = STREAMING_S;
                    }
                    else
                    {
                        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL,instData->con[conId].remoteDevAddr,instData->stream[s].mediaCid);
                        instData->stream[s].streamState = OPENED_S;
                    }
                    instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */

                    CsrBtAvStopStreamTimer(instData, (CsrUint8) s);
                }
            }
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvCloseRejHandler
 *
 *  DESCRIPTION
 *      Handle reject response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvCloseRejHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUintFast8 s;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if( CsrBtAvStopStreamTimer(instData, (CsrUint8) s) )
        {
            CsrBtAvCloseCfmSend(instData,(CsrUint8) s, (CsrUint8)(prim->payload[0]>>4),
                prim->payload[2],
                CSR_BT_SUPPLIER_A2DP);
            instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvSuspendRejHandler
 *
 *  DESCRIPTION
 *      Handle reject response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvSuspendRejHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUintFast8 s, tmp, tLabel;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    tLabel = (CsrUint8)(prim->payload[0]>>4);

    tmp = CSR_BT_AV_MAX_NUM_STREAMS;
    for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
    {
        if((conId == instData->stream[s].conId)
        && (tLabel == instData->stream[s].tLabel) 
        && (instData->stream[s].streamState == STREAMING_S))
        {
            break;
        }
    }

    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if(instData->stream[s].timerId != 0x00)
        {
            for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
            {
                tmp = (CsrUint8)(prim->payload[2]>>2);
                if( (tmp == instData->stream[s].remoteSeid)
                    && (conId == instData->stream[s].conId)
                    && (tLabel == instData->stream[s].tLabel) )
                {
                    CsrBtAvSuspendCfmSend(instData,(CsrUint8) s, (CsrUint8)(prim->payload[0]>>4),
                        prim->payload[3],
                        CSR_BT_SUPPLIER_A2DP);
                    tmp = s;
                    break;
                }
            }

            for(s=0; s<CSR_BT_AV_MAX_NUM_STREAMS; s++)
            {
                if ( (conId == instData->stream[s].conId)
                     && (tLabel == instData->stream[s].tLabel)
                     && (instData->stream[s].streamState == STREAMING_S) )
                {
                    if( s<tmp )
                    {
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
                        /* av stop indication */
                        CsrBtAvStreamStartStopIndSend(instData, s, FALSE);
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */
                        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL,instData->con[conId].remoteDevAddr,instData->stream[s].mediaCid);
                        instData->stream[s].streamState = OPENED_S;
                    }

                    instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */

                    CsrBtAvStopStreamTimer(instData, (CsrUint8) s);
                }
            }
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvAbortRejHandler
 *
 *  DESCRIPTION
 *      Handle reject response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvAbortRejHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 s;
    CsrBtCmL2caDataInd *prim;

     prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));

    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        CsrBtAvStopStreamTimer(instData, s);

        if( instData->stream[s].mediaCid != 0)
        {
            CsrBtCml2caDisconnectReqSend(instData->stream[s].mediaCid);
            instData->stream[s].streamState = ABORTING_S;
        }
        else
        {
            CsrBtAvAbortCfmSend(instData, s, instData->stream[s].tLabel);
            if (instData->stream[s].streamState == ABORT_REQUESTED_S)
            {/* ABORT response is received after media channel is disconnected by remote */
                CsrBtAvClearStream(instData,(CsrUint8) s);
            }                
        }

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
        /* av stop indication */
        CsrBtAvStreamStartStopIndSend(instData, s, FALSE);
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */
    }
}

#ifdef INSTALL_AV_SECURITY_CONTROL
/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvSecurityControlRejHandler
 *
 *  DESCRIPTION
 *      Handle reject response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvSecurityControlRejHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 s;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if ( CsrBtAvStopStreamTimer(instData, s) )
        {
            CsrBtAvSecurityControlCfmSend(instData, s, (CsrUint8)(prim->payload[0]>>4), 0, NULL,
                prim->payload[2],
                CSR_BT_SUPPLIER_A2DP);
            instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */
        }
    }
}
#endif /* INSTALL_AV_SECURITY_CONTROL */

/*----------------------------------------------------------------------------*
 *  NAME
 *      csrBtAvDelayReportRejHandler
 *
 *  DESCRIPTION
 *      Handle reject response from peer device
 *
 *  RETURNS
 *      void
 *
 *---------------------------------------------------------------------------*/
static void csrBtAvDelayReportRejHandler(av_instData_t * instData, CsrUint8 conId)
{
    CsrUint8 s;
    CsrBtCmL2caDataInd *prim;

    prim = (CsrBtCmL2caDataInd *) instData->recvMsgP;

    s = findStreamFromTLabel(instData, conId, (CsrUint8)((*prim->payload) >> 4));
    if (s < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        if ( CsrBtAvStopStreamTimer(instData, s) )
        {
            CsrBtAvDelayReportCfmSend(instData, s, (CsrUint8)(prim->payload[0]>>4),
                prim->payload[2],
                CSR_BT_SUPPLIER_A2DP);
            instData->stream[s].tLabel = 0xFF; /* invalidate tLabel */
        }
    }
}

