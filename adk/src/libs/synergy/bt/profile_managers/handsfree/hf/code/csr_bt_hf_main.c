/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_sched.h"
#include "csr_bt_result.h"
#include "csr_env_prim.h"
#include "csr_bt_util.h"
#include "csr_bt_hf_main.h"
#include "csr_bt_hf_main_sef.h"
#include "csr_bt_hf_lib.h"
#include "csr_bt_hf_connect_sef.h"

#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif

#include "csr_log_text_2.h"

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_hf_streams.h"
#endif

#ifdef CSR_LOG_ENABLE
/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtHfLto);
#ifdef CSR_TARGET_PRODUCT_VM
#include "csr_bt_hf_prim_enum_dbg.h"
CSR_PRESERVE_GENERATED_ENUM(HfLastAtCmdSent)
CSR_PRESERVE_GENERATED_ENUM(CsrBtHfPrim)
#endif /*CSR_TARGET_PRODUCT_VM*/
#endif

static const HfStateHandlerType hfMainStateHandlers[HfMainNumberOfStates][CSR_BT_HF_PRIM_DOWNSTREAM_COUNT] =
{
    /* Null_s */
    {
        CsrBtHfXStateActivateReqHandler,                              /* CSR_BT_HF_ACTIVATE_REQ */
        CsrBtHfNullStateDeactivateReqHandler,                         /* CSR_BT_HF_DEACTIVATE_REQ */
#ifdef CSR_BT_INSTALL_HF_CONFIG_AUDIO
        CsrBtHfXStateConfigAudioReqHandler,                           /* CSR_BT_HF_CONFIG_AUDIO_REQ */
#else
        NULL,                                                         /* CSR_BT_HF_CONFIG_AUDIO_REQ */
#endif
        CsrBtHfSaveMessage,                                           /* CSR_BT_HF_SERVICE_CONNECT_REQ */
        NULL,                                                         /* CSR_BT_HF_CANCEL_CONNECT_REQ */
        NULL,                                                         /* CSR_BT_HF_DISCONNECT_REQ */
        NULL,                                                         /* CSR_BT_HF_AUDIO_CONNECT_REQ */
        NULL,                                                         /* CSR_BT_HF_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                         /* CSR_BT_HF_AUDIO_DISCONNECT_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_GET_ALL_STATUS_INDICATORS_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_GET_CURRENT_CALL_LIST_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_ECHO_AND_NOISE_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_VOICE_RECOGNITION_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_BT_INPUT_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_GENERATE_DTMF_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SPEAKER_GAIN_STATUS_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_MIC_GAIN_STATUS_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_DIAL_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_CALL_ANSWER_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_CALL_END_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_CALL_HANDLING_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_AT_CMD_REQ */
        HsHfSecurityInReqHandler,                                     /* CSR_BT_HF_SECURITY_IN_REQ */
        HsHfSecurityOutReqHandler,                                    /* CSR_BT_HF_SECURITY_OUT_REQ */
        CsrBtHfXStateSetDeregisterTimeReqHandler,                     /* CSR_BT_HF_DEREGISTER_TIME_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_INDICATOR_ACTIVATION_REQ */
        CsrBtHfXStateHfUpdateCodecSupportReqHandler,                  /* CSR_BT_HF_UPDATE_SUPPORTED_CODEC_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_HF_INDICATOR_VALUE_REQ */
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
        CsrBtHfXStateHfUpdateQceSupportReqHandler,                    /* CSR_BT_HF_UPDATE_QCE_CODEC_REQ */
#else
        NULL,                                                         /* CSR_BT_HF_UPDATE_QCE_CODEC_REQ */
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
        HfXStateHfUpdateOptionalCodecReqHandler                        /* HF_UPDATE_OPTIONAL_CODEC_REQ */
    },
    /* Activated_s */
    {
        CsrBtHfXStateActivateReqHandler,                              /* CSR_BT_HF_ACTIVATE_REQ */
        CsrBtHfActivatedStateHfDeactivateReqHandler,                  /* CSR_BT_HF_DEACTIVATE_REQ */
#ifdef CSR_BT_INSTALL_HF_CONFIG_AUDIO
        CsrBtHfXStateConfigAudioReqHandler,                           /* CSR_BT_HF_CONFIG_AUDIO_REQ */
#else
        NULL,                                                         /* CSR_BT_HF_CONFIG_AUDIO_REQ */
#endif
        CsrBtHfActivatedStateHfServiceConnectReq,                     /* CSR_BT_HF_SERVICE_CONNECT_REQ */
        CsrBtHfActivatedStateHfCancelReqHandler,                      /* CSR_BT_HF_CANCEL_CONNECT_REQ */
        CsrBtHfActivatedStateHfDisconnectReqHandler,                  /* CSR_BT_HF_DISCONNECT_REQ */
        CsrBtHfActivatedStateAudioReqHandler,                         /* CSR_BT_HF_AUDIO_CONNECT_REQ */
        CsrBtHfActivatedStateMapScoPcmResHandler,                     /* CSR_BT_HF_AUDIO_ACCEPT_CONNECT_RES */
        CsrBtHfActivatedStateAudioDisconnectReqHandler,               /* CSR_BT_HF_AUDIO_DISCONNECT_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_GET_ALL_STATUS_INDICATORS_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_GET_CURRENT_CALL_LIST_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_ECHO_AND_NOISE_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_VOICE_RECOGNITION_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_BT_INPUT_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_GENERATE_DTMF_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SPEAKER_GAIN_STATUS_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_MIC_GAIN_STATUS_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_DIAL_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_CALL_ANSWER_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_CALL_END_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_CALL_HANDLING_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_AT_CMD_REQ */
        HsHfSecurityInReqHandler,                                     /* CSR_BT_HF_SECURITY_IN_REQ */
        HsHfSecurityOutReqHandler,                                    /* CSR_BT_HF_SECURITY_OUT_REQ */
        CsrBtHfXStateSetDeregisterTimeReqHandler,                     /* CSR_BT_HF_DEREGISTER_TIME_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_INDICATOR_ACTIVATION_REQ */
        CsrBtHfXStateHfUpdateCodecSupportReqHandler,                  /* CSR_BT_HF_UPDATE_SUPPORTED_CODEC_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_HF_INDICATOR_VALUE_REQ */
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
        CsrBtHfXStateHfUpdateQceSupportReqHandler,                    /* CSR_BT_HF_UPDATE_QCE_CODEC_REQ */
#else
        NULL,                                                         /* CSR_BT_HF_UPDATE_QCE_CODEC_REQ */
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
        HfXStateHfUpdateOptionalCodecReqHandler                        /* HF_UPDATE_OPTIONAL_CODEC_REQ */
    },
    /* Deactivate_s */
    {
        NULL,                                                         /* CSR_BT_HF_ACTIVATE_REQ */
        CsrBtHfMainIgnoreMessage,                                     /* CSR_BT_HF_DEACTIVATE_REQ */
#ifdef CSR_BT_INSTALL_HF_CONFIG_AUDIO
        CsrBtHfXStateConfigAudioReqHandler,                           /* CSR_BT_HF_CONFIG_AUDIO_REQ */
#else
        NULL,                                                         /* CSR_BT_HF_CONFIG_AUDIO_REQ */
#endif
        NULL,                                                         /* CSR_BT_HF_SERVICE_CONNECT_REQ */
        NULL,                                                         /* CSR_BT_HF_CANCEL_CONNECT_REQ */
        NULL,                                                         /* CSR_BT_HF_DISCONNECT_REQ */
        NULL,                                                         /* CSR_BT_HF_AUDIO_CONNECT_REQ */
        NULL,                                                         /* CSR_BT_HF_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                         /* CSR_BT_HF_AUDIO_DISCONNECT_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_GET_ALL_STATUS_INDICATORS_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_GET_CURRENT_CALL_LIST_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_ECHO_AND_NOISE_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_VOICE_RECOGNITION_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_BT_INPUT_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_GENERATE_DTMF_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SPEAKER_GAIN_STATUS_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_MIC_GAIN_STATUS_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_DIAL_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_CALL_ANSWER_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_CALL_END_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_CALL_HANDLING_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_AT_CMD_REQ */
        HsHfSecurityInReqHandler,                                     /* CSR_BT_HF_SECURITY_IN_REQ */
        HsHfSecurityOutReqHandler,                                    /* CSR_BT_HF_SECURITY_OUT_REQ */
        CsrBtHfXStateSetDeregisterTimeReqHandler,                     /* CSR_BT_HF_DEREGISTER_TIME_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_INDICATOR_ACTIVATION_REQ */
        CsrBtHfXStateHfUpdateCodecSupportReqHandler,                  /* CSR_BT_HF_UPDATE_SUPPORTED_CODEC_REQ */
        CsrBtHfXStateHfCommonAtCmdReqHandler,                         /* CSR_BT_HF_SET_HF_INDICATOR_VALUE_REQ*/
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
        CsrBtHfXStateHfUpdateQceSupportReqHandler,                    /* CSR_BT_HF_UPDATE_QCE_CODEC_REQ */
#else
        NULL,                                                         /* CSR_BT_HF_UPDATE_QCE_CODEC_REQ */
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
        HfXStateHfUpdateOptionalCodecReqHandler                        /* HF_UPDATE_OPTIONAL_CODEC_REQ */
    }
};

/* ---------- CM jump table ---------- */
/* PLEASE NOTE: only the top most part of the CM primitives are included */
static const HfStateHandlerType mainCmStateHandlers[HfMainNumberOfUpstreamStates][CSR_BT_CM_RFC_PRIM_UPSTREAM_COUNT] =
{

    /* Null_s */
    {
        NULL,                                                    /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        CsrBtHfNullStateCmRegisterCfmHandler,                    /* CSR_BT_CM_REGISTER_CFM */
        NULL,                                                    /* CSR_BT_CM_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_SCO_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_DATA_IND */
        NULL,                                                    /* CSR_BT_CM_DATA_CFM */
        NULL,                                                    /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                    /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        NULL,                                                    /* CSR_BT_CM_PORTNEG_IND */
        NULL,                                                    /* CSR_BT_CM_PORTNEG_CFM */
        NULL,                                                    /* CSR_BT_CM_RFC_CONNECT_ACCEPT_IND */
    },
    /* Activated_s */
    {
        CsrBtHfActivatedStateCmCancelAcceptConnectCfmHandler,    /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_CONNECT_CFM */
        CsrBtHfActivatedStateCmConnectAcceptCfmHandler,          /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        NULL,                                                    /* CSR_BT_CM_REGISTER_CFM */
        CsrBtHfActivatedStateCmDisconnectIndHandler,             /* CSR_BT_CM_DISCONNECT_IND */
        CsrBtHfActivatedStateCmScoConnectCfmHandler,             /* CSR_BT_CM_SCO_CONNECT_CFM */
        CsrBtHfActivatedStateCmScoDisconnectIndHandler,          /* CSR_BT_CM_SCO_DISCONNECT_IND */
        CsrBtHfActivatedStateCmScoAcceptConnectCfmHandler,       /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        CsrBtHfActivatedStateCmDataIndHandler,                   /* CSR_BT_CM_DATA_IND */
        CsrBtHfActivatedStateCmDataCfmHandler,                   /* CSR_BT_CM_DATA_CFM */
        CsrBtHfXStateIgnoreCmControlIndHandler,                  /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                    /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        CsrBtHfActivatedStateCmPortnegIndHandler,                /* CSR_BT_CM_PORTNEG_IND */
        NULL,                                                    /* CSR_BT_CM_PORTNEG_CFM */
        HfActivatedStateCmRfcConnectAcceptIndHandler,            /* CSR_BT_CM_RFC_CONNECT_ACCEPT_IND */
    },
    /* Deactivate_s */
    {
        CsrBtHfDeactivateStateCmCancelAcceptConnectCfmHandler,   /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_CONNECT_CFM */
        CsrBtHfDeactivateStateCmConnectAcceptCfmHandler,         /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        NULL,                                                    /* CSR_BT_CM_REGISTER_CFM */
        CsrBtHfDeactivateStateCmDisconnectIndHandler,            /* CSR_BT_CM_DISCONNECT_IND */
        CsrBtHfDeactivateStateCmScoConnectCfmHandler,            /* CSR_BT_CM_SCO_CONNECT_CFM */
        CsrBtHfDeactivateStateCmScoDisconnectIndHandler,         /* CSR_BT_CM_SCO_DISCONNECT_IND */
        CsrBtHfDeactivateStateCmScoAcceptConnectCfm,             /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        CsrBtHfDeactivateStateCmDataIndHandler,                  /* CSR_BT_CM_DATA_IND */
        CsrBtHfDeactivateStateCmDataCfmHandler,                  /* CSR_BT_CM_DATA_CFM */
        CsrBtHfXStateIgnoreCmControlIndHandler,                  /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                    /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        NULL,                                                    /* CSR_BT_CM_PORTNEG_IND */
        NULL,                                                    /* CSR_BT_CM_PORTNEG_CFM */
        HfDeactivateStateCmRfcConnectAcceptIndHandler,           /* CSR_BT_CM_RFC_CONNECT_ACCEPT_IND */
    }
};

#if defined(CSR_TARGET_PRODUCT_VM) && defined(CSR_LOG_ENABLE)
/* Used for logging connection ID */
static CsrBtHfConnectionId getConIdFromHfPrim(void *msg)
{
    CsrBtHfPrim id = *(CsrBtHfPrim *)msg;
    if (CSR_BT_HF_PRIM_DOWNSTREAM_LOWEST <= id && id <= CSR_BT_HF_PRIM_DOWNSTREAM_HIGHEST)
    {
        switch(id)
        {
            case CSR_BT_HF_ACTIVATE_REQ:
            case CSR_BT_HF_DEACTIVATE_REQ:
            case CSR_BT_HF_SERVICE_CONNECT_REQ :
            case CSR_BT_HF_CANCEL_CONNECT_REQ:
            case CSR_BT_HF_SECURITY_IN_REQ:
            case CSR_BT_HF_SECURITY_OUT_REQ:
            case CSR_BT_HF_DEREGISTER_TIME_REQ:
            case CSR_BT_HF_UPDATE_SUPPORTED_CODEC_REQ:
            case CSR_BT_HF_UPDATE_QCE_CODEC_REQ:
                /*These prims do not have Connection ID*/
                return CSR_BT_CONN_ID_INVALID;
            default:
                return ((CsrBtHfDataPrim*)msg)->connectionId;
        }
    }

    return CSR_BT_CONN_ID_INVALID;
}
#endif /* CSR_TARGET_PRODUCT_VM && CSR_LOG_ENABLE */

void CsrBtHfInit(void **gash);

void CsrBtHfHandler(void **gash);

#ifdef CSR_BT_GLOBAL_INSTANCE
HfMainInstanceData_t csrBtHfInstance;
#endif

void CsrBtHfInit(void **gash)
{
    HfMainInstanceData_t  *instData;

    CSR_LOG_TEXT_REGISTER(&CsrBtHfLto, "BT_HF", 0, NULL);

#ifdef CSR_BT_GLOBAL_INSTANCE
    *gash = &csrBtHfInstance;
#else
    /* allocate and initialise instance data space */
    *gash    = CsrPmemZalloc(sizeof(HfMainInstanceData_t));
#endif
    instData = (HfMainInstanceData_t *) *gash;
    /* Init common instance data */
    instData->appHandle = CSR_SCHED_QID_INVALID;

#ifdef INSTALL_HF_CUSTOM_SECURITY_SETTINGS
    CsrBtScSetSecInLevel(&instData->secIncoming, CSR_BT_SEC_DEFAULT,
        CSR_BT_HANDSFREE_MANDATORY_SECURITY_INCOMING,
        CSR_BT_HANDSFREE_DEFAULT_SECURITY_INCOMING,
        CSR_BT_RESULT_CODE_HF_SUCCESS,
        CSR_BT_RESULT_CODE_HF_UNACCEPTABLE_PARAMETER);

    CsrBtScSetSecOutLevel(&instData->secOutgoing, CSR_BT_SEC_DEFAULT,
        CSR_BT_HANDSFREE_MANDATORY_SECURITY_OUTGOING,
        CSR_BT_HANDSFREE_DEFAULT_SECURITY_OUTGOING,
        CSR_BT_RESULT_CODE_HF_SUCCESS,
        CSR_BT_RESULT_CODE_HF_UNACCEPTABLE_PARAMETER);
#endif /* INSTALL_HF_CUSTOM_SECURITY_SETTINGS */

    instData->generalAudioParams.theAudioQuality  = CSR_BT_ESCO_DEFAULT_CONNECT_AUDIO_QUALITY;
    instData->generalAudioParams.theTxBandwidth   = CSR_BT_ESCO_DEFAULT_CONNECT_TX_BANDWIDTH;
    instData->generalAudioParams.theRxBandwidth   = CSR_BT_ESCO_DEFAULT_CONNECT_RX_BANDWIDTH;
    instData->generalAudioParams.theMaxLatency    = CSR_BT_ESCO_DEFAULT_CONNECT_MAX_LATENCY;
    instData->generalAudioParams.theVoiceSettings = CSR_BT_ESCO_DEFAULT_CONNECT_VOICE_SETTINGS;
    instData->generalAudioParams.theReTxEffort    = CSR_BT_ESCO_DEFAULT_CONNECT_RE_TX_EFFORT;
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
    /* QCE needs to be enabled from the application, at the time of activation, default is disabled */
    instData->hfQceCodecMask           = CSR_BT_HF_QCE_UNSUPPORTED;
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
    instData->atRespWaitTime           = CSR_BT_AT_DEFAULT_RESPONSE_TIME;
    instData->supportedCodecsMask      = CSR_BT_WBS_CVSD_CODEC_MASK | CSR_BT_WBS_MSBC_CODEC_MASK;
    instData->localHfIndicatorList = NULL;
    instData->indCount = 0;
    instData->hfServerChannel = CSR_BT_NO_SERVER;
    instData->hsServerChannel = CSR_BT_NO_SERVER;
#ifdef HF_ENABLE_OPTIONAL_CODEC_SUPPORT
    instData->optionalCodecList = NULL;
    instData->optionalCodecCount = 0;
#endif
}

#ifdef ENABLE_SHUTDOWN
/****************************************************************************
 *  This function is called by the scheduler to perform a graceful shutdown
 *  of a scheduler task.
 *  This function must:
 *  1)  empty the input message queue and free any allocated memory in the
 *      messages.
 *  2)  free any instance data that may be allocated.
 ****************************************************************************/
void CsrBtHfDeinit(void **gash)
{
    CsrUint16 msg_type=0;
    void *msg_data=NULL;
    CsrUintFast8  i;
    CsrUint8 total;
    CsrBool    lastMsg;
    HfMainInstanceData_t  *instData = (HfMainInstanceData_t *) (*gash);
    HfInstanceData_t *linkPtr = instData->linkData;

    /* continue to poll any message of the input queue */
    lastMsg = FALSE;
    total = instData->maxHFConnections + instData->maxHSConnections + instData->allocInactiveHsCons + instData->allocInactiveHfCons;
    while (!lastMsg)
    {
        msg_data = NULL;
        msg_type = 0;
        lastMsg = CsrMessageQueuePop(&instData->saveQueue, &msg_type, &msg_data);


        i = 0;
        while(!lastMsg && (i<total) && linkPtr != NULL)
        {
            linkPtr = (HfInstanceData_t *)&(instData->linkData[i]);
            if (linkPtr != NULL)
            {
                if (linkPtr->data != NULL)
                {
                    lastMsg = CsrMessageQueuePop(&(linkPtr->data->cmDataReqQueue),&msg_type,&msg_data);
                }
                i++;
            }
        }


        if(!lastMsg)
                {
                    lastMsg = !CsrSchedMessageGet(&msg_type, &msg_data);
                }

        if((!lastMsg) && (msg_data != NULL))
        {
            switch (msg_type)
            {
                case CSR_BT_CM_PRIM:
                {
                    CsrBtCmFreeUpstreamMessageContents(msg_type, msg_data);
                    break;
                }

                case CSR_BT_HF_PRIM:
                {
                    CsrBtHfFreeDownstreamMessageContents(msg_type, msg_data);
                    break;
                }
                case CSR_SCHED_PRIM:
                    break;
                default:
                    {
                        CsrGeneralException(CsrBtHfLto,
                                            0,
                                            msg_type,
                                            0,
                                            0,
                                            "Unhandled PRIM in HF Deinit");
                        break;
                    }
            }
            SynergyMessageFree(msg_type, msg_data);
        }
    }

    linkPtr = instData->linkData;

    CsrBtHfFreeInactiveLinkData(instData->linkData, (CsrUint8)(instData->maxHFConnections + instData->maxHSConnections));
    CsrPmemFree(instData->linkData);
    instData->linkData = NULL;

    if(instData->localHfIndicatorList != NULL)
    {
        CsrPmemFree(instData->localHfIndicatorList);
        instData->localHfIndicatorList = NULL;
    }

#ifdef CSR_BT_GLOBAL_INSTANCE
    CsrMemSet(instData, 0, sizeof(HfMainInstanceData_t));
#else
    CsrPmemFree(instData);
#endif
    *gash = NULL;
}
#endif

void CsrBtHfCleanup_queue(HfMainInstanceData_t* instData)
{
    CsrUint16 msg_type;
    void *msg_data;
    CsrBool    lastMsg;
    CsrUint8 i = 0;

    /* continue to poll any message of the input queue */
    lastMsg = FALSE;

    while (!lastMsg)
    {
        msg_data = NULL;
        msg_type = 0;
        lastMsg = CsrMessageQueuePop(&instData->saveQueue, &msg_type, &msg_data);


        i = 0;
        while(!lastMsg && (i < (instData->maxHFConnections + instData->maxHSConnections)))
        {
            lastMsg = CsrMessageQueuePop(&(instData->linkData[i].data->cmDataReqQueue), &msg_type, &msg_data);
            i++;
        }


        if(!lastMsg)
        {
            lastMsg = !CsrSchedMessageGet(&msg_type, &msg_data);
        }

        if((!lastMsg) && (msg_data != NULL))
        {
            switch (msg_type)
            {
                case CSR_BT_CM_PRIM:
                {
                    CsrBtCmFreeUpstreamMessageContents(msg_type, msg_data);
                    break;
                }

                case CSR_BT_HF_PRIM:
                    {
                    CsrBtHfFreeDownstreamMessageContents(msg_type, msg_data);
                    break;
                }

                case CSR_SCHED_PRIM:
                    break;

                default:
                    {
                        CsrGeneralException(CsrBtHfLto,
                                            0,
                                            msg_type,
                                            0,
                                            0,
                                            "Unhandled PRIM in HF Deinit");
                        break;
                    }
            }
            SynergyMessageFree(msg_type, msg_data);
        }
    }
}

void CsrBtHfHandler(void **gash)
{
    HfMainInstanceData_t    *instData = (HfMainInstanceData_t *) (*gash);
    void                *msg=NULL;
    HfInstanceData_t *linkPtr = instData->linkData;


    if(!instData->restoreFlag)
    {
        CsrSchedMessageGet(&instData->eventClass  , &msg);
    }
    else
    {
        if(!CsrMessageQueuePop(&instData->saveQueue, &instData->eventClass , &msg))
        {
            CsrSchedMessageGet(&instData->eventClass  , &msg);
        }
        instData->restoreFlag = FALSE;
    }
    instData->recvMsgP = msg;

    switch(instData->eventClass)
    {
        case CSR_BT_HF_PRIM:
            {
                CsrBtHfPrim         *primType;

                /* find the message type */
                primType = (CsrBtHfPrim *)msg;
                if ((*primType < CSR_BT_HF_PRIM_DOWNSTREAM_COUNT) &&
                    (instData->state < HfMainNumberOfStates) &&
                    (hfMainStateHandlers[instData->state][*primType] != NULL))
                {
#ifdef CSR_TARGET_PRODUCT_VM
                    CSR_LOG_TEXT_INFO((CsrBtHfLto, 0, "CsrBtHfHandler MESSAGE:CsrBtHfPrim:0x%x ConnId:0x%08x", *primType, getConIdFromHfPrim(msg)));
#endif
                    hfMainStateHandlers[instData->state][*primType](instData);
                }
                else if (*primType != CSR_BT_HF_HOUSE_CLEANING)
                {
                    CsrBtHfFreeDownstreamMessageContents(instData->eventClass, msg);
                    /* State/Event ERROR! */
                    CsrGeneralException(CsrBtHfLto,
                                        0,
                                        instData->eventClass,
                                        *primType,
                                        instData->state,
                                        "Unknown HF prim or undefined state");
                }

                break;
            }

#ifdef CSR_STREAMS_ENABLE
        case MESSAGE_MORE_SPACE:
        {
            CsrBtHfMessageMoreSpaceHandler(instData);
            break;
        }
        case MESSAGE_MORE_DATA:
        {
            CsrBtHfMessageMoreDataHandler(instData);
            break;
        }
#endif

        case CSR_BT_CM_PRIM:
        {
            CsrPrim *primType;
            /* find the message type */
            primType = (CsrPrim *)msg;
#ifdef CSR_TARGET_PRODUCT_VM
            CSR_LOG_TEXT_INFO((CsrBtHfLto, 0, "CsrBtHfHandler MESSAGE:CsrBtCmPrim:0x%x ConnId:0x%08x", *primType, getConIdFromHfPrim(msg)));
#endif

            if (((CsrUint16)(*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)< CSR_BT_CM_RFC_PRIM_UPSTREAM_COUNT) &&
                (instData->state < HfMainNumberOfUpstreamStates) &&
                (mainCmStateHandlers[instData->state][(CsrUint16)(*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)] != NULL))
            {
                mainCmStateHandlers[instData->state][(CsrUint16)(*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)](instData);
            }
            else if (CSR_BT_CM_MAP_SCO_PCM_IND == *primType)
            {
                CsrBtHfMainXStateMapScoPcmIndHandler(instData);
            }
            else if ((CSR_BT_CM_REGISTER_CFM == *primType) && (instData->reActivating))
            {
                CsrBtHfNullStateCmRegisterCfmHandler(instData);
            }
            else if (CSR_BT_CM_WRITE_AUTH_PAYLOAD_TIMEOUT_CFM == *primType)
            {
                /* Ignore the APT confirmation, will handle it when profile has dependency on the result code
                 * no instruction is given in the profile spec for handling the error code */
            }
            else if (CsrBtUtilRfcConVerifyCmMsg(instData->recvMsgP))
            {
                CsrUint8 instanceIdx = CSR_BT_NO_SERVER;
                CsrBtDeviceAddr            deviceAddr;
                CsrBtHfSetAddrInvalid(&deviceAddr);

                switch (*primType)
                {
                    case CSR_BT_CM_SDC_SEARCH_IND:
                        {
                            instanceIdx = ((CsrBtCmSdcSearchInd *) primType)->localServerChannel;
                            deviceAddr = ((CsrBtCmSdcSearchInd *) primType)->deviceAddr;
                            break;
                        }
                    case CSR_BT_CM_SDC_SEARCH_CFM:
                        {
                            instanceIdx = ((CsrBtCmSdcSearchCfm *) primType)->localServerChannel;
                            deviceAddr = ((CsrBtCmSdcSearchCfm *) primType)->deviceAddr;
                            break;
                        }
                    case CSR_BT_CM_SDC_CLOSE_IND:
                        {
                            instanceIdx = ((CsrBtCmSdcCloseInd *) primType)->localServerChannel;
                            deviceAddr = ((CsrBtCmSdcCloseInd *) primType)->deviceAddr;
                            break;
                        }
                    case CSR_BT_CM_SDC_RELEASE_RESOURCES_CFM:
                        {
                            instanceIdx = ((CsrBtCmSdcReleaseResourcesCfm *) primType)->localServerChannel;
                            deviceAddr = ((CsrBtCmSdcReleaseResourcesCfm *) primType)->deviceAddr;
                            break;
                        }
                    case CSR_BT_CM_SDC_ATTRIBUTE_CFM:
                        {
                            instanceIdx = ((CsrBtCmSdcAttributeCfm *) primType)->localServerChannel;
                            deviceAddr = ((CsrBtCmSdcAttributeCfm *) primType)->deviceAddr;
                            break;
                        }
                    case CM_SDC_SERVICE_SEARCH_ATTR_IND:
                    case CM_SDC_SERVICE_SEARCH_ATTR_CFM:
                        {
                            instanceIdx = ((CmSdcServiceSearchAttrCfm *) primType)->localServerChannel;
                            deviceAddr = ((CmSdcServiceSearchAttrCfm *) primType)->deviceAddr;
                            break;
                        }
                    case CSR_BT_CM_CONNECT_CFM:
                        {
                            CsrBtCmConnectCfm *prim = (CsrBtCmConnectCfm *)(msg);
                            instanceIdx = (CsrUint8) prim->context;
#ifdef CSR_STREAMS_ENABLE
                            if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                            {
                                CsrBtHfStreamsRegister(instData, prim->btConnId);
                            }
#endif
                            break;
                        }
                    case CSR_BT_CM_REGISTER_CFM:
                        {
                            instanceIdx = (CsrUint8) ((CsrBtCmRegisterCfm *) primType)->context;
                            break;
                        }
                    default:
                        break;
                }

                if (instanceIdx != CSR_BT_NO_SERVER)
                {
                    /* Outgoing connection */
                    if (CsrBtHfSetCurrentConnIndexFromInstId(instData, instanceIdx))
                    {
                        linkPtr = (HfInstanceData_t *) &(instData->linkData[instData->index]);
                        CsrBtUtilRfcConCmMsgHandler(linkPtr, linkPtr->sdpSearchData, instData->recvMsgP);
                    }
                    else
                    {
                        CSR_LOG_TEXT_INFO((CsrBtHfLto,
                                           0,
                                           "HF outgoing connection instance not found"));
                    }
                }
                else
                {
                    /* Incoming connection */
                    if (CsrBtHfSetCurrentConnIndexFromBdAddrSdc(instData, deviceAddr))
                    {
                        linkPtr = (HfInstanceData_t *) &(instData->linkData[instData->index]);
                        CsrBtUtilSdcCmMsgHandler(linkPtr, linkPtr->sdpSearchData, instData->recvMsgP);
                    }
                    else
                    {
                        CSR_LOG_TEXT_INFO((CsrBtHfLto,
                                           0,
                                           "HF incoming connection instance not found"));
                    }
                }
            }
            else if (*primType == CSR_BT_CM_SDS_EXT_REGISTER_CFM)
            {
                if ((instData->state == Null_s) || (instData->state == Activated_s))
                {
                    CsrBtHfXStateCmSdsRegisterCfmHandler(instData);
                }
                else /* Deactivate. Only possible in case of crossing signals. */
                {
                    CsrBtHfDeactivateStateCmSdsRegisterCfmHandler(instData);
                }
            }
            else if ((*primType  == CSR_BT_CM_SDS_UNREGISTER_CFM) &&
                     ( (instData->state == Deactivate_s) || (instData->state == Activated_s) ))
            {
                if (instData->state == Activated_s)
                {
                    CsrBtHfActivatedStateCmSdsUnregisterCfmHandler(instData);
                }
                else if (instData->state == Deactivate_s)
                {
                    CsrBtHfDeactivateStateCmSdsUnregisterCfmHandler(instData);
                }
            }
            else
            {
               if ((*primType  != CSR_BT_CM_SDC_RELEASE_RESOURCES_CFM) && (instData->state != Null_s))
               {
                    CsrGeneralException(CsrBtHfLto,
                                        0,
                                        CSR_BT_CM_PRIM,
                                        (CsrUint16)(*primType - CSR_PRIM_UPSTREAM),
                                        instData->state,
                                        "Unknown CM prim or undefined state");
               }
                CsrBtCmFreeUpstreamMessageContents(CSR_BT_CM_PRIM, instData->recvMsgP);
            }
            break;
        }
        case CSR_SCHED_PRIM:
            {
                CsrEnvPrim *primType;
                primType = (CsrEnvPrim *) msg;
                switch(*primType)
                {
                    case CSR_CLEANUP_IND:
                        {
                            CsrCleanupInd* prim;
                            prim = (CsrCleanupInd*) msg;
                            if(prim->phandle == instData->appHandle)
                            {
                                instData->doingCleanup = TRUE;
                                CsrBtHfHsDeactivateHandler(instData);
                            }
                            break;
                        }
                    default:
                        {
                            /* Other primitives are ignored if exception handler module is excluded! */
                            CsrGeneralException(CsrBtHfLto,
                                                0,
                                                CSR_SCHED_PRIM,
                                                *primType,
                                                instData->state,
                                                "Unknown ENV prim or undefined state");
                        }
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
                                    instData->state,
                                    "Unknown primitive type received in main");
            }
    }
    SynergyMessageFree(instData->eventClass, instData->recvMsgP);    /* free the received message. if the ptr is NULL just CsrSched just ignores */
    instData->recvMsgP = NULL;
}

