/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_sched.h"
#include "csr_pmem.h"
#include "csr_env_prim.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_util.h"
#include "csr_bt_hfg_prim.h"
#include "csr_bt_hfg_main.h"
#include "csr_bt_hfg_proto.h"
#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_hfg_streams.h"
#endif /* CSR_STREAMS_ENABLE */
#include "csr_log_text_2.h"

/* We use MAIN_NULL to denote that the top jump-table handler takes
 * care of a particular state-event-function. This is done solely
 * to ease readability */
#define MAIN_NULL (NULL)

static const HfgConnHandler_t hfgConnHandlers[ConnectionNum_s - Activate_s][CSR_BT_HFG_PRIM_DOWNSTREAM_COUNT] =
{
    /* Activate_s */
    {
        MAIN_NULL,                                               /* CSR_BT_HFG_ACTIVATE_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_DEACTIVATE_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_SERVICE_CONNECT_REQ*/
        CsrBtHfgXIgnore,                                         /* CSR_BT_HFG_CANCEL_CONNECT_REQ */
        CsrBtHfgXHfgDisconnectReqHandler,                        /* CSR_BT_HFG_DISCONNECT_REQ */
        NULL,                                                    /* CSR_BT_HFG_AUDIO_CONNECT_REQ */
        NULL,                                                    /* CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                    /* CSR_BT_HFG_AUDIO_DISCONNECT_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_SECURITY_IN_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_SECURITY_OUT_REQ */
        CsrBtHfgXHfgConfigAudioReqHandler,                       /* CSR_BT_HFG_CONFIG_AUDIO_REQ */
        NULL,                                                    /* CSR_BT_HFG_RING_REQ */
        NULL,                                                    /* CSR_BT_HFG_CALL_WAITING_REQ */
        NULL,                                                    /* CSR_BT_HFG_CALL_HANDLING_REQ */
        NULL,                                                    /* CSR_BT_HFG_CALL_HANDLING_RES */
        NULL,                                                    /* CSR_BT_HFG_DIAL_RES */
        NULL,                                                    /* CSR_BT_HFG_SPEAKER_GAIN_REQ */
        NULL,                                                    /* CSR_BT_HFG_MIC_GAIN_REQ */
        NULL,                                                    /* CSR_BT_HFG_AT_CMD_REQ */
        NULL,                                                    /* CSR_BT_HFG_OPERATOR_RES */
        NULL,                                                    /* CSR_BT_HFG_CALL_LIST_RES */
        NULL,                                                    /* CSR_BT_HFG_SUBSCRIBER_NUMBER_RES */
        CsrBtHfgXHfgStatusIndicatorSetReqHandler,                     /* CSR_BT_HFG_STATUS_INDICATOR_SET_REQ */
        NULL,                                                    /* CSR_BT_HFG_INBAND_RINGING_REQ */
        NULL,                                                    /* CSR_BT_HFG_BT_INPUT_RES */
        NULL,                                                    /* CSR_BT_HFG_VOICE_RECOG_REQ */
        NULL,                                                    /* CSR_BT_HFG_VOICE_RECOG_RES */
        NULL,                                                    /* CSR_BT_HFG_MANUAL_INDICATOR_RES */
        MAIN_NULL,                                               /* CSR_BT_HFG_CONFIG_SINGLE_ATCMD_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_CONFIG_ATCMD_HANDLING_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_DEREGISTER_TIME_REQ */
        NULL,                                                    /* CSR_BT_HFG_SET_HF_INDICATOR_STATUS_REQ */
#ifdef CSR_BT_HFG_ENABLE_SWB_SUPPORT
        NULL,                                                    /* CSR_BT_HFG_SWB_RSP */
#endif
    },
    /* Connect_s */
    {
        MAIN_NULL,                                               /* CSR_BT_HFG_ACTIVATE_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_DEACTIVATE_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_SERVICE_CONNECT_REQ*/
        CsrBtHfgConnectHfgCancelConnectReqHandler,               /* CSR_BT_HFG_CANCEL_CONNECT_REQ */
        CsrBtHfgXHfgDisconnectReqHandler,                        /* CSR_BT_HFG_DISCONNECT_REQ */
        NULL,                                                    /* CSR_BT_HFG_AUDIO_CONNECT_REQ */
        NULL,                                                    /* CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                    /* CSR_BT_HFG_AUDIO_DISCONNECT_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_SECURITY_IN_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_SECURITY_OUT_REQ */
        CsrBtHfgXHfgConfigAudioReqHandler,                       /* CSR_BT_HFG_CONFIG_AUDIO_REQ */
        NULL,                                                    /* CSR_BT_HFG_RING_REQ */
        NULL,                                                    /* CSR_BT_HFG_CALL_WAITING_REQ */
        NULL,                                                    /* CSR_BT_HFG_CALL_HANDLING_REQ */
        NULL,                                                    /* CSR_BT_HFG_CALL_HANDLING_RES */
        NULL,                                                    /* CSR_BT_HFG_DIAL_RES */
        NULL,                                                    /* CSR_BT_HFG_SPEAKER_GAIN_REQ */
        NULL,                                                    /* CSR_BT_HFG_MIC_GAIN_REQ */
        NULL,                                                    /* CSR_BT_HFG_AT_CMD_REQ */
        NULL,                                                    /* CSR_BT_HFG_OPERATOR_RES */
        NULL,                                                    /* CSR_BT_HFG_CALL_LIST_RES */
        NULL,                                                    /* CSR_BT_HFG_SUBSCRIBER_NUMBER_RES */
        CsrBtHfgXHfgStatusIndicatorSetReqHandler,                     /* CSR_BT_HFG_STATUS_INDICATOR_SET_REQ */
        NULL,                                                    /* CSR_BT_HFG_INBAND_RINGING_REQ */
        NULL,                                                    /* CSR_BT_HFG_BT_INPUT_RES */
        NULL,                                                    /* CSR_BT_HFG_VOICE_RECOG_REQ */
        NULL,                                                    /* CSR_BT_HFG_VOICE_RECOG_RES */
        NULL,                                                    /* CSR_BT_HFG_MANUAL_INDICATOR_RES */
        MAIN_NULL,                                               /* CSR_BT_HFG_CONFIG_SINGLE_ATCMD_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_CONFIG_ATCMD_HANDLING_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_DEREGISTER_TIME_REQ */
        NULL,                                                    /* CSR_BT_HFG_SET_HF_INDICATOR_STATUS_REQ */
#ifdef CSR_BT_HFG_ENABLE_SWB_SUPPORT
        NULL,                                                    /* CSR_BT_HFG_SWB_RSP */
#endif
    },
    /* ServiceSearch_s */
    {
        MAIN_NULL,                                               /* CSR_BT_HFG_ACTIVATE_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_DEACTIVATE_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_SERVICE_CONNECT_REQ */
        CsrBtHfgServiceSearchHfgCancelConnectReqHandler,         /* CSR_BT_HFG_CANCEL_CONNECT_REQ */
        CsrBtHfgServiceSearchHfgDisconnectReqHandler,            /* CSR_BT_HFG_DISCONNECT_REQ */
        CsrBtHfgServiceSearchHfgXReqHandler,                     /* CSR_BT_HFG_AUDIO_CONNECT_REQ */
        NULL,                                                    /* CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_RES */
        CsrBtHfgServiceSearchHfgXReqHandler,                     /* CSR_BT_HFG_AUDIO_DISCONNECT_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_SECURITY_IN_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_SECURITY_OUT_REQ */
        CsrBtHfgXHfgConfigAudioReqHandler,                       /* CSR_BT_HFG_CONFIG_AUDIO_REQ */
        CsrBtHfgServiceSearchHfgXReqHandler,                     /* CSR_BT_HFG_RING_REQ */
        CsrBtHfgServiceSearchHfgXReqHandler,                     /* CSR_BT_HFG_CALL_WAITING_REQ */
        NULL,                                                    /* CSR_BT_HFG_CALL_HANDLING_REQ */
        NULL,                                                    /* CSR_BT_HFG_CALL_HANDLING_RES */
        NULL,                                                    /* CSR_BT_HFG_DIAL_RES */
        CsrBtHfgServiceSearchHfgXReqHandler,                     /* CSR_BT_HFG_SPEAKER_GAIN_REQ */
        CsrBtHfgServiceSearchHfgXReqHandler,                     /* CSR_BT_HFG_MIC_GAIN_REQ */
        CsrBtHfgServiceSearchHfgXReqHandler,                     /* CSR_BT_HFG_AT_CMD_REQ */
        NULL,                                                    /* CSR_BT_HFG_OPERATOR_RES */
        NULL,                                                    /* CSR_BT_HFG_CALL_LIST_RES */
        NULL,                                                    /* CSR_BT_HFG_SUBSCRIBER_NUMBER_RES */
        CsrBtHfgXHfgStatusIndicatorSetReqHandler,                /* CSR_BT_HFG_STATUS_INDICATOR_SET_REQ */
        CsrBtHfgServiceSearchHfgXReqHandler,                     /* CSR_BT_HFG_INBAND_RINGING_REQ */
        NULL,                                                    /* CSR_BT_HFG_BT_INPUT_RES */
        CsrBtHfgServiceSearchHfgXReqHandler,                     /* CSR_BT_HFG_VOICE_RECOG_REQ */
        NULL,                                                    /* CSR_BT_HFG_VOICE_RECOG_RES */
        CsrBtHfgServiceSearchHfgManualIndicatorResHandler,            /* CSR_BT_HFG_MANUAL_INDICATOR_RES */
        MAIN_NULL,                                               /* CSR_BT_HFG_CONFIG_SINGLE_ATCMD_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_CONFIG_ATCMD_HANDLING_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_DEREGISTER_TIME_REQ */
        CsrBtHfgServiceSearchHfgXReqHandler,                     /* CSR_BT_HFG_SET_HF_INDICATOR_STATUS_REQ */
#ifdef CSR_BT_HFG_ENABLE_SWB_SUPPORT
        CsrBtHfgSwbRspHandler,                                    /* CSR_BT_HFG_SWB_RSP */
#endif
    },
    /* Connected_s */
    {
        MAIN_NULL,                                               /* CSR_BT_HFG_ACTIVATE_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_DEACTIVATE_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_SERVICE_CONNECT_REQ*/
        CsrBtHfgConnectedHfgCancelConnectReqHandler,             /* CSR_BT_HFG_CANCEL_CONNECT_REQ */
        CsrBtHfgConnectedHfgDisconnectReqHandler,                /* CSR_BT_HFG_DISCONNECT_REQ */
        CsrBtHfgConnectedHfgAudioReqHandler,                     /* CSR_BT_HFG_AUDIO_CONNECT_REQ */
        CsrBtHfgConnectedHfgMapScoPcmResHandler,                 /* CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_RES */
        CsrBtHfgConnectedHfgAudioDisconnectReqHandler,           /* CSR_BT_HFG_AUDIO_DISCONNECT_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_SECURITY_IN_REQ*/
        MAIN_NULL,                                               /* CSR_BT_HFG_SECURITY_OUT_REQ */
        CsrBtHfgXHfgConfigAudioReqHandler,                       /* CSR_BT_HFG_CONFIG_AUDIO_REQ */
        CsrBtHfgConnectedHfgRingReqHandler,                      /* CSR_BT_HFG_RING_REQ */
        CsrBtHfgConnectedHfgCallWaitingReqHandler,               /* CSR_BT_HFG_CALL_WAITING_REQ */
        CsrBtHfgConnectedHfgCallHandlingReqHandler,              /* CSR_BT_HFG_CALL_HANDLING_REQ */
        CsrBtHfgConnectedHfgCallHandlingResHandler,              /* CSR_BT_HFG_CALL_HANDLING_RES */
        CsrBtHfgConnectedHfgDialResHandler,                      /* CSR_BT_HFG_DIAL_RES */
        CsrBtHfgConnectedHfgSpeakerGainReqHandler,               /* CSR_BT_HFG_SPEAKER_GAIN_REQ */
        CsrBtHfgConnectedHfgMicGainReqHandler,                   /* CSR_BT_HFG_MIC_GAIN_REQ */
        CsrBtHfgConnectedHfgAtCmdReqHandler,                     /* CSR_BT_HFG_AT_CMD_REQ */
        CsrBtHfgConnectedHfgOperatorResHandler,                  /* CSR_BT_HFG_OPERATOR_RES */
        CsrBtHfgConnectedHfgCallListResHandler,                  /* CSR_BT_HFG_CALL_LIST_RES */
        CsrBtHfgConnectedHfgSubscriberNumberResHandler,          /* CSR_BT_HFG_SUBSCRIBER_NUMBER_RES */
        CsrBtHfgXHfgStatusIndicatorSetReqHandler,                /* CSR_BT_HFG_STATUS_INDICATOR_SET_REQ */
        CsrBtHfgConnectedHfgInbandRingingReqHandler,             /* CSR_BT_HFG_INBAND_RINGING_REQ */
        CsrBtHfgConnectedHfgBtInputResHandler,                   /* CSR_BT_HFG_BT_INPUT_RES */
        CsrBtHfgConnectedHfgVoiceRecogReqHandler,                /* CSR_BT_HFG_VOICE_RECOG_REQ */
        CsrBtHfgConnectedHfgVoiceRecogResHandler,                /* CSR_BT_HFG_VOICE_RECOG_RES */
        NULL,                                                    /* CSR_BT_HFG_MANUAL_INDICATOR_RES */
        MAIN_NULL,                                               /* CSR_BT_HFG_CONFIG_SINGLE_ATCMD_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_CONFIG_ATCMD_HANDLING_REQ */
        MAIN_NULL,                                               /* CSR_BT_HFG_DEREGISTER_TIME_REQ */
        CsrBtHfgSetHfIndicatorStatusReqHandler,                  /* CSR_BT_HFG_SET_HF_INDICATOR_STATUS_REQ */
#ifdef CSR_BT_HFG_ENABLE_SWB_SUPPORT
        CsrBtHfgSwbRspHandler,                                    /* CSR_BT_HFG_SWB_RSP */
#endif
    }
};

/* CM jump table for local connection */
static const HfgConnHandler_t cmConnHandlers[ConnectionNum_s - Activate_s][CSR_BT_CM_RFC_PRIM_UPSTREAM_COUNT] =
{
    /* Activate_s */
    {
        NULL,                                                    /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_CONNECT_CFM */
        MAIN_NULL,                                               /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        MAIN_NULL,                                               /* CSR_BT_CM_REGISTER_CFM */
        NULL,                                                    /* CSR_BT_CM_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_SCO_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_DATA_IND */
        NULL,                                                    /* CSR_BT_CM_DATA_CFM */
        MAIN_NULL,                                               /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                    /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        MAIN_NULL,                                               /* CSR_BT_CM_PORTNEG_IND */
        MAIN_NULL,                                               /* CSR_BT_CM_PORTNEG_CFM */
    },
    /* Connect_s */
    {
        CsrBtHfgConnectCmCancelAcceptConnectCfmHandler,          /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_CONNECT_CFM */
        MAIN_NULL,                                               /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        MAIN_NULL,                                               /* CSR_BT_CM_REGISTER_CFM */
        NULL,                                                    /* CSR_BT_CM_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_SCO_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_DATA_IND */
        NULL,                                                    /* CSR_BT_CM_DATA_CFM */
        MAIN_NULL,                                               /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                    /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        MAIN_NULL,                                               /* CSR_BT_CM_PORTNEG_IND */
        MAIN_NULL,                                               /* CSR_BT_CM_PORTNEG_CFM */
    },
    /* ServiceSearch_s */
    {
        CsrBtHfgXIgnore,                                         /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_CONNECT_CFM */
        MAIN_NULL,                                               /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        MAIN_NULL,                                               /* CSR_BT_CM_REGISTER_CFM */
        CsrBtHfgServiceSearchCmDisconnectIndHandler,             /* CSR_BT_CM_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_SCO_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        CsrBtHfgXCmDataIndHandler,                               /* CSR_BT_CM_DATA_IND */
        CsrBtHfgXCmDataCfmHandler,                               /* CSR_BT_CM_DATA_CFM */
        MAIN_NULL,                                               /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                    /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        MAIN_NULL,                                               /* CSR_BT_CM_PORTNEG_IND */
        MAIN_NULL,                                               /* CSR_BT_CM_PORTNEG_CFM */
    },
    /* Connected_s */
    {
        NULL,                                                    /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_CONNECT_CFM */
        MAIN_NULL,                                               /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        MAIN_NULL,                                               /* CSR_BT_CM_REGISTER_CFM */
        CsrBtHfgXCmDisconnectIndHandler,                         /* CSR_BT_CM_DISCONNECT_IND */
        CsrBtHfgConnectedCmScoConnectCfmHandler,                 /* CSR_BT_CM_SCO_CONNECT_CFM */
        CsrBtHfgConnectedCmScoDisconnectIndHandler,              /* CSR_BT_CM_SCO_DISCONNECT_IND */
        CsrBtHfgConnectedCmScoAcceptConnectCfmHandler,           /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        CsrBtHfgXCmDataIndHandler,                               /* CSR_BT_CM_DATA_IND */
        CsrBtHfgXCmDataCfmHandler,                               /* CSR_BT_CM_DATA_CFM */
        MAIN_NULL,                                               /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                    /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        MAIN_NULL,                                               /* CSR_BT_CM_PORTNEG_IND */
        MAIN_NULL,                                               /* CSR_BT_CM_PORTNEG_CFM */
    }
};

/* Cancel pending connection from save-queue */
void CsrBtHfgMainCancelPendingConnectHandler(HfgMainInstance_t *inst)
{
    CsrBtHfgCancelConnectReq *prim;
    CsrMessageQueueType *localSaveQueue;
    void *msg;
    CsrUint16 event;

    CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "CsrBtHfgMainCancelPendingConnectHandler")); 

    localSaveQueue = NULL;
    prim = (CsrBtHfgCancelConnectReq *)inst->msg;

    /* Check SaveMessages queue to see if there is any
     * CSR_BT_HFG_SERVICE_CONNECT_REQs and move them from global to temporary
     * savequeue, but take out the connection requests */
    while(CsrMessageQueuePop(&inst->saveQueue, &event, &msg))
    {
        CsrBool found;
        CsrBtHfgPrim *primType;
        primType = (CsrBtHfgPrim*)msg;

        found = FALSE;
        if(*primType == CSR_BT_HFG_SERVICE_CONNECT_REQ)
        {
            CsrBtHfgServiceConnectReq *connectPrim;
            connectPrim = (CsrBtHfgServiceConnectReq*)msg;

            if(CsrBtBdAddrEq(&prim->deviceAddr, &connectPrim->deviceAddr) &&
               (inst->appHandle != CSR_SCHED_QID_INVALID))
            {
                /* Found the connect request to cancel. Send
                 * confirmation to app. We have to construct the
                 * message ourselves here as no linkData currently
                 * exists for it */
                CsrBtHfgServiceConnectInd *newPrim;
                newPrim = (CsrBtHfgServiceConnectInd*)CsrPmemZalloc(sizeof(CsrBtHfgServiceConnectInd));

                newPrim->type = CSR_BT_HFG_SERVICE_CONNECT_IND;
                newPrim->connectionId = CSR_BT_HFG_CONID_NONE;
                newPrim->connectionType = CSR_BT_HFG_CONNECTION_UNKNOWN;
                newPrim->serviceName = NULL;
                newPrim->deviceAddr = prim->deviceAddr;
                newPrim->supportedFeatures = 0;
                newPrim->remoteVersion = 0;
                newPrim->resultCode = CSR_BT_RESULT_CODE_HFG_CANCELLED_CONNECT_ATTEMPT;
                newPrim->resultSupplier = CSR_BT_SUPPLIER_HFG;
                newPrim->btConnId = CSR_BT_CONN_ID_INVALID;
                newPrim->hfSupportedHfIndicators = NULL;
                newPrim->hfSupportedHfIndicatorsCount = 0;

                found = TRUE;
                CsrBtHfgMessagePut(inst->appHandle, newPrim);
                CsrPmemFree(msg);
            }
        }
        if(!found)
        {
            /* Suppressing Lint warning 449:  Pointer variable 'msg' previously deallocated.
             * As 'msg' will be pushed in local queue only when 'found' is not TRUE implying 'msg' is not freed. */
             
            /*lint -e(449) */
            CsrMessageQueuePush(&localSaveQueue, event, msg);
        }
    }

    /* Move from temporary queue back to global save-queue */
    while(CsrMessageQueuePop(&localSaveQueue, &event, &msg))
    {
        CsrMessageQueuePush(&inst->saveQueue, event, msg);
    }
}

/* Second level HFG message handler. Here we call the second level
 * jump-tables for HFG primitives*/
void CsrBtHfgMainXSecondHfg(HfgMainInstance_t *inst)
{
    CsrBtHfgPrim *type;
    HfgInstance_t *link;
    CsrBool loop;
    CsrUintFast8 i;

    CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "CsrBtHfgMainXSecondHfg")); 

    type = (CsrBtHfgPrim*)inst->msg;
    loop = FALSE;
    i = CsrBtHfgFindIndexWithHfgPrim(inst, *type);

    /* Should we loop over connections? */
    if(i == CSR_BT_HFG_NO_CONID)
    {
        if (*type == CSR_BT_HFG_CANCEL_CONNECT_REQ)
        {   /* Remember to remove from mailqueue if it is there...*/
            CsrBtHfgMainCancelPendingConnectHandler(inst);
            return;
        }
        else
        {
            i = 0;
            loop = TRUE;
        }
    }

    /* Loop over all connections if required */
    do
    {
        /* Prepare connection instance */
        link = &(inst->linkData[i]);
        link->msg = inst->msg;
        link->msgClass = inst->msgClass;

        /* Invoke */
        if((*type < CSR_BT_HFG_PRIM_DOWNSTREAM_COUNT) &&
           (link->state < ConnectionNum_s) &&
           (hfgConnHandlers[link->state][*type] != NULL))
        {
            hfgConnHandlers[link->state][*type](link);
        }
        /* Override state for indicator settings */
        else if(*type == CSR_BT_HFG_STATUS_INDICATOR_SET_REQ)
        {
            CsrBtHfgXHfgStatusIndicatorSetReqHandler(link);
        }
        else if (*type == CSR_BT_CM_WRITE_VOICE_SETTINGS_CFM)
        {/* Just ignore so far.... */
        }
        else if(!loop)
        {
            /* State/event error */
            CsrGeneralException(CsrBtHfgLto,
                                0,
                                CSR_BT_HFG_PRIM,
                                (CsrUint16)*type,
                                (CsrUint16)link->state,
                                "Unknown HFG prim or undefined state");
        }

        /* Always increment index. If we shouldn't loop we'll stop due
         * to the loop variable being false */
        i++;
    }
    while ((i<CSR_BT_HFG_NUM_SERVERS) && loop);
}

/* Second level CM message handler. Here we call the second level
 * jump-table for CM primitives */
void CsrBtHfgMainXSecondCm(HfgMainInstance_t *inst)
{
    CsrPrim *type;
    HfgInstance_t *linkData;

    type = (CsrPrim *)inst->msg;
    linkData = CsrBtHfgFindLinkWithCmPrim(inst, *type);

    CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "CsrBtHfgMainXSecondCm type = 0x%x", *type)); 

    if(linkData == NULL)
    {
        if ((*type != CSR_BT_CM_SDC_CLOSE_IND) && (*type != CSR_BT_CM_DATA_CFM) 
            && (*type != CSR_BT_CM_MAP_SCO_PCM_IND) && (*type != CM_SDC_SERVICE_SEARCH_ATTR_CFM))
        {
            /* State/event error. The signals above may be received in some cross situations */
            CsrGeneralException(CsrBtHfgLto,
                                0,
                                CSR_BT_CM_PRIM,
                                *type,
                                0,
                                "Invalid (non-connection) CM primitive");
        }
        return;
    }

    linkData->msg = inst->msg;
    linkData->msgClass = inst->msgClass;

    if(((*type - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST) < CSR_BT_CM_RFC_PRIM_UPSTREAM_COUNT) &&
       (cmConnHandlers[linkData->state][(CsrUint16)(*type - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)] != NULL))
    {
        cmConnHandlers[linkData->state][(CsrUint16)(*type - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)](linkData);
    }       
    /* Handle a few special primitives here to avoid the jumptables
     * getting too large */
    else if(*type == CSR_BT_CM_MAP_SCO_PCM_IND)
    {
        CsrBtHfgXCmMapScoPcmIndHandler(linkData);
    }
    else if ((CsrBtUtilRfcConVerifyCmMsg(linkData->msg)) && (linkData->searchAndCon))
    {
        CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "CsrBtUtilRfcConCmMsgHandler link type = %d", linkData->linkType)); 

        CsrBtUtilRfcConCmMsgHandler(linkData, inst->sdpHfgSearchConData, linkData->msg);
        /* To avoid trying to free them when already freed...*/
        linkData->msg = NULL;
    }
    else if ((CsrBtUtilSdcVerifyCmMsg(linkData->msg)) && (linkData->pendingSearch))
    {
        CsrBtUtilSdcCmMsgHandler(linkData, inst->sdpHfgSearchData, linkData->msg);
        /* To avoid trying to free them when already freed...*/
        linkData->msg = NULL;
    }
    else
    {
        /* State/event error */
        CsrGeneralException(CsrBtHfgLto,
                            0,
                            CSR_BT_CM_PRIM,
                            (CsrUint16)*type,
                            (CsrUint16)linkData->state,
                            "Unknown CM prim or undefined state");
    }
}

/* Ignore top-level message */
void CsrBtHfgMainXIgnore(HfgMainInstance_t *inst)
{
    (void)inst;
}

/* Ignore connection-level message */
void CsrBtHfgXIgnore(HfgInstance_t *inst)
{
    (void)inst;
}

