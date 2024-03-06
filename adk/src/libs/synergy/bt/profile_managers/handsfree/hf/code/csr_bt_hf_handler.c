/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_hf_handler.h"
#include "csr_bt_hf_connect_sef.h"
#include "csr_bt_hf_call_sef.h"
#include "csr_bt_hfhs_data_sef.h"
#include "csr_bt_cm_prim.h"
#include "csr_log_text_2.h"

/* ---------- HF jump table ---------- */
static const HfStateHandlerType hfStateHandlers[HfNumberOfStates][CSR_BT_HF_PRIM_DOWNSTREAM_COUNT] =
{
    /* Activate_s */
    {
        NULL,                                                    /* CSR_BT_HF_ACTIVATE_REQ */
        NULL,                                                    /* CSR_BT_HF_DEACTIVATE_REQ */
        NULL,                                                    /* CSR_BT_HF_CONFIG_AUDIO_REQ */
        NULL,                                                    /* CSR_BT_HF_SERVICE_CONNECT_REQ */
        NULL,                                                    /* CSR_BT_HF_CANCEL_CONNECT_REQ */
        NULL,                                                    /* CSR_BT_HF_DISCONNECT_REQ */
        NULL,                                                    /* CSR_BT_HF_AUDIO_CONNECT_REQ */
        NULL,                                                    /* CSR_BT_HF_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                    /* CSR_BT_HF_AUDIO_DISCONNECT_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_ALL_STATUS_INDICATORS_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_CURRENT_CALL_LIST_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_ECHO_AND_NOISE_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_VOICE_RECOGNITION_REQ */
        NULL,                                                    /* CSR_BT_HF_BT_INPUT_REQ */
        NULL,                                                    /* CSR_BT_HF_GENERATE_DTMF_REQ */
        NULL,                                                    /* CSR_BT_HF_SPEAKER_GAIN_STATUS_REQ */
        NULL,                                                    /* CSR_BT_HF_MIC_GAIN_STATUS_REQ */
        NULL,                                                    /* CSR_BT_HF_DIAL_REQ */
        NULL,                                                    /* CSR_BT_HF_CALL_ANSWER_REQ */
        NULL,                                                    /* CSR_BT_HF_CALL_END_REQ */
        NULL,                                                    /* CSR_BT_HF_CALL_HANDLING_REQ */
        NULL,                                                    /* CSR_BT_HF_AT_CMD_REQ */
        NULL,                                                    /* CSR_BT_HF_SECURITY_IN_REQ */
        NULL,                                                    /* CSR_BT_HF_SECURITY_OUT_REQ */
        NULL,                                                    /* CSR_BT_HF_DEREGISTER_TIME_REQ */
        NULL,                                                    /* CSR_BT_HF_INDICATOR_ACTIVATION_REQ */
        NULL,                                                    /* CSR_BT_HF_UPDATE_SUPPORTED_CODEC_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_HF_INDICATOR_VALUE_REQ*/
        NULL,                                                    /* CSR_BT_HF_UPDATE_QCE_CODEC_REQ */
        NULL,                                                    /* HF_UPDATE_OPTIONAL_CODEC_REQ*/
    },
    /* Connect_s */
    {
        NULL,                                                    /* CSR_BT_HF_ACTIVATE_REQ */
        NULL,                                                    /* CSR_BT_HF_DEACTIVATE_REQ */
        NULL,                                                    /* CSR_BT_HF_CONFIG_AUDIO_REQ */
        NULL,                                                    /* CSR_BT_HF_SERVICE_CONNECT_REQ */
        CsrBtHfXStateHfCancelReqHandler,                         /* CSR_BT_HF_CANCEL_CONNECT_REQ */
        CsrBtHfXStateHfCancelReqHandler,                         /* CSR_BT_HF_DISCONNECT_REQ */
        NULL,                                                    /* CSR_BT_HF_AUDIO_CONNECT_REQ */
        NULL,                                                    /* CSR_BT_HF_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                    /* CSR_BT_HF_AUDIO_DISCONNECT_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_ALL_STATUS_INDICATORS_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_CURRENT_CALL_LIST_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_ECHO_AND_NOISE_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_VOICE_RECOGNITION_REQ */
        NULL,                                                    /* CSR_BT_HF_BT_INPUT_REQ */
        NULL,                                                    /* CSR_BT_HF_GENERATE_DTMF_REQ */
        NULL,                                                    /* CSR_BT_HF_SPEAKER_GAIN_STATUS_REQ */
        NULL,                                                    /* CSR_BT_HF_MIC_GAIN_STATUS_REQ */
        NULL,                                                    /* CSR_BT_HF_DIAL_REQ */
        NULL,                                                    /* CSR_BT_HF_CALL_ANSWER_REQ */
        NULL,                                                    /* CSR_BT_HF_CALL_END_REQ */
        NULL,                                                    /* CSR_BT_HF_CALL_HANDLING_REQ */
        NULL,                                                    /* CSR_BT_HF_AT_CMD_REQ */
        NULL,                                                    /* CSR_BT_HF_SECURITY_IN_REQ */
        NULL,                                                    /* CSR_BT_HF_SECURITY_OUT_REQ */
        NULL,                                                    /* CSR_BT_HF_DEREGISTER_TIME_REQ */
        NULL,                                                    /* CSR_BT_HF_INDICATOR_ACTIVATION_REQ */
        NULL,                                                    /* CSR_BT_HF_UPDATE_SUPPORTED_CODEC_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_HF_INDICATOR_VALUE_REQ*/
        NULL,                                                    /* CSR_BT_HF_UPDATE_QCE_CODEC_REQ */
        NULL,                                                    /* HF_UPDATE_OPTIONAL_CODEC_REQ*/
    },
    /* Connected_s */
    {
        NULL,                                                    /* CSR_BT_HF_ACTIVATE_REQ */
        NULL,                                                    /* CSR_BT_HF_DEACTIVATE_REQ */
        NULL,                                                    /* CSR_BT_HF_CONFIG_AUDIO_REQ */
        NULL,                                                    /* CSR_BT_HF_SERVICE_CONNECT_REQ */
        NULL,                                                    /* CSR_BT_HF_CANCEL_CONNECT_REQ */
        CsrBtHfXStateHfDisconnectReqHandler,                     /* CSR_BT_HF_DISCONNECT_REQ */
        CsrBtHfConnectedStateHfAudioReqHandler,                  /* CSR_BT_HF_AUDIO_CONNECT_REQ */
        NULL,                                                    /* CSR_BT_HF_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                    /* CSR_BT_HF_AUDIO_DISCONNECT_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_ALL_STATUS_INDICATORS_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_CURRENT_CALL_LIST_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_ECHO_AND_NOISE_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_VOICE_RECOGNITION_REQ */
        NULL,                                                    /* CSR_BT_HF_BT_INPUT_REQ */
        NULL,                                                    /* CSR_BT_HF_GENERATE_DTMF_REQ */
        CsrBtHfConnectedStateHfSpeakerGainStatusReqHandler,      /* CSR_BT_HF_SPEAKER_GAIN_STATUS_REQ */
        CsrBtHfConnectedStateHfMicGainStatusReqHandler,          /* CSR_BT_HF_MIC_GAIN_STATUS_REQ */
        NULL,                                                    /* CSR_BT_HF_DIAL_REQ */
        CsrBtHfConnectedStateHfAnswerReqHandler,                 /* CSR_BT_HF_CALL_ANSWER_REQ */
        CsrBtHfConnectedStateHfCallEndReqHandler,                /* CSR_BT_HF_CALL_END_REQ */
        CsrBtHfConnectedStateHfChldReqHandler,                   /* CSR_BT_HF_CALL_HANDLING_REQ */
        CsrBtHfConnectedStateHfAtCmdReqHandler,                  /* CSR_BT_HF_AT_CMD_REQ */
        NULL,                                                    /* CSR_BT_HF_SECURITY_IN_REQ */
        NULL,                                                    /* CSR_BT_HF_SECURITY_OUT_REQ */
        NULL,                                                    /* CSR_BT_HF_DEREGISTER_TIME_REQ */
        NULL,                                                    /* CSR_BT_HF_INDICATOR_ACTIVATION_REQ */
        NULL,                                                    /* CSR_BT_HF_UPDATE_SUPPORTED_CODEC_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_HF_INDICATOR_VALUE_REQ*/
        NULL,                                                    /* CSR_BT_HF_UPDATE_QCE_CODEC_REQ */
        NULL,                                                    /* HF_UPDATE_OPTIONAL_CODEC_REQ*/
    },
    /* ServiceSearch_s */
    {
        NULL,                                                    /* CSR_BT_HF_ACTIVATE_REQ */
        NULL,                                                    /* CSR_BT_HF_DEACTIVATE_REQ */
        NULL,                                                    /* CSR_BT_HF_CONFIG_AUDIO_REQ */
        NULL,                                                    /* CSR_BT_HF_SERVICE_CONNECT_REQ */
        CsrBtHfXStateHfCancelReqHandler,                         /* CSR_BT_HF_CANCEL_CONNECT_REQ */
        NULL,                                                    /* CSR_BT_HF_DISCONNECT_REQ */
        CsrBtHfSaveMessage,                                      /* CSR_BT_HF_AUDIO_CONNECT_REQ */
        NULL,                                                    /* CSR_BT_HF_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                    /* CSR_BT_HF_AUDIO_DISCONNECT_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_ALL_STATUS_INDICATORS_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_REQ */
        NULL,                                                    /* CSR_BT_HF_GET_CURRENT_CALL_LIST_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_ECHO_AND_NOISE_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_VOICE_RECOGNITION_REQ */
        NULL,                                                    /* CSR_BT_HF_BT_INPUT_REQ */
        NULL,                                                    /* CSR_BT_HF_GENERATE_DTMF_REQ */
        CsrBtHfSaveMessage,                                      /* CSR_BT_HF_SPEAKER_GAIN_STATUS_REQ */
        CsrBtHfSaveMessage,                                      /* CSR_BT_HF_MIC_GAIN_STATUS_REQ */
        NULL,                                                    /* CSR_BT_HF_DIAL_REQ */
        CsrBtHfSaveMessage,                                      /* CSR_BT_HF_CALL_ANSWER_REQ */
        NULL,                                                    /* CSR_BT_HF_CALL_END_REQ */
        NULL,                                                    /* CSR_BT_HF_CALL_HANDLING_REQ */
        CsrBtHfSaveMessage,                                      /* CSR_BT_HF_AT_CMD_REQ */
        NULL,                                                    /* CSR_BT_HF_SECURITY_IN_REQ */
        NULL,                                                    /* CSR_BT_HF_SECURITY_OUT_REQ */
        NULL,                                                    /* CSR_BT_HF_DEREGISTER_TIME_REQ */
        NULL,                                                    /* CSR_BT_HF_INDICATOR_ACTIVATION_REQ */
        NULL,                                                    /* CSR_BT_HF_UPDATE_SUPPORTED_CODEC_REQ */
        NULL,                                                    /* CSR_BT_HF_SET_HF_INDICATOR_VALUE_REQ*/
        NULL,                                                    /* CSR_BT_HF_UPDATE_QCE_CODEC_REQ */
        NULL,                                                    /* HF_UPDATE_OPTIONAL_CODEC_REQ*/
    }
};


static const HfStateHandlerType hfCmStateHandlers[HfNumberOfStates][CSR_BT_CM_RFC_PRIM_UPSTREAM_COUNT] =
{
    /* Activate_s */
    {
        NULL,                                                    /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_CONNECT_CFM */
        CsrBtHfActivateStateCmConnectAcceptCfmHandler,           /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        NULL,                                                    /* CSR_BT_CM_REGISTER_CFM */
        CsrBtHfXStateCmDisconnectIndHandler,                     /* CSR_BT_CM_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_SCO_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        CsrBtHfHsXStateCmDataIndHandler,                         /* CSR_BT_CM_DATA_IND */
        CsrBtHfHsXStateCmDataCfmHandler,                         /* CSR_BT_CM_DATA_CFM */
        NULL,                                                    /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                    /* CSR_BT_CM_MODE_CHANGE_IND */
        NULL,                                                    /* CSR_BT_CM_PORTNEG_IND */
        NULL,                                                    /* CSR_BT_CM_PORTNEG_CFM */
        HfActivateStateCmRfcConnectAcceptIndHandler,             /* CSR_BT_CM_RFC_CONNECT_ACCEPT_IND */
    },
    /* Connect_s */
    {
        NULL,                                                    /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_CONNECT_CFM */
        CsrBtHfXStateCmConnectAcceptCfmHandler,                  /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        NULL,                                                    /* CSR_BT_CM_REGISTER_CFM */
        CsrBtHfXStateCmDisconnectIndHandler,                     /* CSR_BT_CM_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_SCO_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_DATA_IND */
        NULL,                                                    /* CSR_BT_CM_DATA_CFM */
        NULL,                                                    /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                    /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        NULL,                                                    /* CSR_BT_CM_PORTNEG_IND */
        NULL,                                                    /* CSR_BT_CM_PORTNEG_CFM */
        HfActivateStateCmRfcConnectAcceptIndHandler,             /* CSR_BT_CM_RFC_CONNECT_ACCEPT_IND */
    },
    /* Connected_s */
    {
        NULL,                                                    /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_CONNECT_CFM */
        CsrBtHfXStateCmConnectAcceptCfmHandler,                  /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        NULL,                                                    /* CSR_BT_CM_REGISTER_CFM */
        CsrBtHfXStateCmDisconnectIndHandler,                     /* CSR_BT_CM_DISCONNECT_IND */
        CsrBtHfConnectedStateCmScoConnectCfmHandler,             /* CSR_BT_CM_SCO_CONNECT_CFM */
        CsrBtHfConnectedStateCmScoDisconnectIndHandler,          /* CSR_BT_CM_SCO_DISCONNECT_IND */
        CsrBtHfConnectedStateCmScoAcceptConnectCfmHandler,       /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        CsrBtHfHsXStateCmDataIndHandler,                         /* CSR_BT_CM_DATA_IND */
        CsrBtHfHsXStateCmDataCfmHandler,                         /* CSR_BT_CM_DATA_CFM */
        NULL,                                                    /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                    /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        NULL,                                                    /* CSR_BT_CM_PORTNEG_IND */
        NULL,                                                    /* CSR_BT_CM_PORTNEG_CFM */
        HfActivateStateCmRfcConnectAcceptIndHandler,             /* CSR_BT_CM_RFC_CONNECT_ACCEPT_IND */
    },
    /* ServiceSearch_s */
    {
        NULL,                                                    /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_CONNECT_CFM */
        CsrBtHfXStateCmConnectAcceptCfmHandler,                  /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        NULL,                                                    /* CSR_BT_CM_REGISTER_CFM */
        CsrBtHfXStateCmDisconnectIndHandler,                     /* CSR_BT_CM_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_SCO_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        CsrBtHfHsXStateCmDataIndHandler,                         /* CSR_BT_CM_DATA_IND */
        CsrBtHfHsXStateCmDataCfmHandler,                         /* CSR_BT_CM_DATA_CFM */
        NULL,                                                    /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                    /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        NULL,                                                    /* CSR_BT_CM_PORTNEG_IND */
        NULL,                                                    /* CSR_BT_CM_PORTNEG_CFM */
        HfActivateStateCmRfcConnectAcceptIndHandler,             /* CSR_BT_CM_RFC_CONNECT_ACCEPT_IND */
    },
};



void CsrBtHfpHandler(HfMainInstanceData_t * instData)
{
    void * msg;
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    msg = instData->recvMsgP;

    switch(instData->eventClass)
    {
        case CSR_BT_HF_PRIM:
            {
                CsrBtHfPrim         *primType;

                /* find the message type */
                primType = (CsrBtHfPrim *)msg;
                if ((*primType < CSR_BT_HF_PRIM_DOWNSTREAM_COUNT) &&
                    (linkPtr->state < HfNumberOfStates) &&
                    (hfStateHandlers[linkPtr->state][*primType] != NULL))
                {
                    hfStateHandlers[linkPtr->state][*primType](instData);
                }
                else if (*primType != CSR_BT_HF_HOUSE_CLEANING)
                {
                    /* State/Event ERROR! */
                    CsrGeneralException(CsrBtHfLto,
                                        0,
                                        CSR_BT_HF_PRIM,
                                        (CsrUint16)(*primType),
                                        linkPtr->state,
                                        "Unknown HF prim or undefined state IN HF statehandler");
                }
                break;
            }

        case CSR_BT_CM_PRIM:
        {
            CsrPrim         *primType;

            /* find the message type */
            primType = (CsrPrim *)msg;
            if (((CsrUint16)(*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST) < CSR_BT_CM_RFC_PRIM_UPSTREAM_COUNT) &&
                (linkPtr->state < HfNumberOfStates) &&
                (hfCmStateHandlers[linkPtr->state][(CsrUint16)(*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)] != NULL))
            {
                hfCmStateHandlers[linkPtr->state][(CsrUint16)(*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)](instData);
            }
            else if (CSR_BT_CM_MAP_SCO_PCM_IND == *primType)
            {
                CsrBtHfXStateMapScoPcmIndHandler(instData);
            }
            else
            {
                CsrGeneralException(CsrBtHfLto,
                                    0,
                                    CSR_BT_CM_PRIM,
                                    *primType,
                                    linkPtr->state,
                                    "Unknown CM prim or undefined state IN HF statehandler");
                CsrBtCmFreeUpstreamMessageContents(CSR_BT_CM_PRIM, instData->recvMsgP);
            }
            break;
        }
        default:
            {
                /* State/Event ERROR! */
                CsrGeneralException(CsrBtHfLto,
                                    0,
                                    instData->eventClass,
                                    0xFF,
                                    linkPtr->state,
                                    "Unknown primitive type received IN HF statehandler");
            }
    }
}





