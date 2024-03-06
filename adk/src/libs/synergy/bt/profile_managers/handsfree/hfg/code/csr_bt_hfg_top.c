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
#include "csr_bt_result.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_util.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_SD_MODULE
#include "csr_bt_sd_private_lib.h"
#endif
#include "csr_log_text_2.h"

#include "csr_bt_hfg_prim.h"
#include "csr_bt_hfg_main.h"
#include "csr_bt_hfg_proto.h"

#include "csr_bt_cmn_sdc_rfc_util.h"
#include "csr_bt_cmn_sdr_tagbased_lib.h"
#include "csr_bt_cmn_sdp_lib.h"

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_hfg_streams.h"
#endif /* CSR_STREAMS_ENABLE */

#ifdef CSR_LOG_ENABLE
/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtHfgLto);
#endif

void CsrBtHfgInit(void **gash);
void CsrBtHfgHandler(void **gash);

/* HFG jump table */
static const HfgMainHandler_t hfgMainHandlers[MainNum_s - MainNull_s][CSR_BT_HFG_PRIM_DOWNSTREAM_COUNT] =
{
    /* MainNull_s */
    {
        CsrBtHfgMainNullHfgActivateReqHandler,                        /* CSR_BT_HFG_ACTIVATE_REQ */
        CsrBtHfgMainSaveMessage,                                      /* CSR_BT_HFG_DEACTIVATE_REQ */
        CsrBtHfgMainSaveMessage,                                      /* CSR_BT_HFG_SERVICE_CONNECT_REQ */
        CsrBtHfgMainNullIdleHfgCancelConnectReqHandler,               /* CSR_BT_HFG_CANCEL_CONNECT_REQ */
        NULL,                                                         /* CSR_BT_HFG_DISCONNECT_REQ */
        NULL,                                                         /* CSR_BT_HFG_AUDIO_CONNECT_REQ */
        NULL,                                                         /* CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                         /* CSR_BT_HFG_AUDIO_DISCONNECT_REQ */
        CsrBtHfgMainXSecurityInReqHandler,                            /* CSR_BT_HFG_SECURITY_IN_REQ */
        CsrBtHfgMainXSecurityOutReqHandler,                           /* CSR_BT_HFG_SECURITY_OUT_REQ */
#ifdef CSR_BT_INSTALL_HFG_CONFIG_AUDIO
        CsrBtHfgMainSaveMessage,                                      /* CSR_BT_HFG_CONFIG_AUDIO_REQ */
#else
        NULL,                                                         /* CSR_BT_HFG_CONFIG_AUDIO_REQ */
#endif
        NULL,                                                         /* CSR_BT_HFG_RING_REQ */
        NULL,                                                         /* CSR_BT_HFG_CALL_WAITING_REQ */
        NULL,                                                         /* CSR_BT_HFG_CALL_HANDLING_REQ */
        NULL,                                                         /* CSR_BT_HFG_CALL_HANDLING_RES */
        NULL,                                                         /* CSR_BT_HFG_DIAL_RES */
        NULL,                                                         /* CSR_BT_HFG_SPEAKER_GAIN_REQ */
        NULL,                                                         /* CSR_BT_HFG_MIC_GAIN_REQ */
        NULL,                                                         /* CSR_BT_HFG_AT_CMD_REQ */
        NULL,                                                         /* CSR_BT_HFG_OPERATOR_RES */
        NULL,                                                         /* CSR_BT_HFG_CALL_LIST_RES */
        NULL,                                                         /* CSR_BT_HFG_SUBSCRIBER_NUMBER_RES */
        CsrBtHfgMainIndicatorSetupReqHandler,                         /* CSR_BT_HFG_STATUS_INDICATOR_SET_REQ */
        NULL,                                                         /* CSR_BT_HFG_INBAND_RINGING_REQ */
        NULL,                                                         /* CSR_BT_HFG_BT_INPUT_RES */
        NULL,                                                         /* CSR_BT_HFG_VOICE_RECOG_REQ */
        NULL,                                                         /* CSR_BT_HFG_VOICE_RECOG_RES */
        NULL,                                                         /* CSR_BT_HFG_MANUAL_INDICATOR_RES */
        CsrBtHfgXConfigSingleAtcmd,                                   /* CSR_BT_HFG_CONFIG_SINGLE_ATCMD_REQ */
        CsrBtHfgXConfigAtCmdHandling,                                 /* CSR_BT_HFG_CONFIG_ATCMD_HANDLING_REQ */
        CsrBtHfgMainXSetDeregisterTimeReqHandler,                     /* CSR_BT_HFG_DEREGISTER_TIME_REQ */
        NULL,                                                         /* CSR_BT_HFG_SET_HF_INDICATOR_STATUS_REQ */
#ifdef CSR_BT_HFG_ENABLE_SWB_SUPPORT
        NULL,                                                         /* CSR_BT_HFG_SWB_RSP */
#endif
    },
    /* MainIdle_s */
    {
        NULL,                                                         /* CSR_BT_HFG_ACTIVATE_REQ */
        CsrBtHfgMainIdleDeactivateReqHandler,                         /* CSR_BT_HFG_DEACTIVATE_REQ */
        CsrBtHfgMainSaveMessage,                                      /* CSR_BT_HFG_SERVICE_CONNECT_REQ */
        CsrBtHfgMainNullIdleHfgCancelConnectReqHandler,               /* CSR_BT_HFG_CANCEL_CONNECT_REQ */
        NULL,                                                         /* CSR_BT_HFG_DISCONNECT_REQ */
        NULL,                                                         /* CSR_BT_HFG_AUDIO_CONNECT_REQ */
        NULL,                                                         /* CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                         /* CSR_BT_HFG_AUDIO_DISCONNECT_REQ */
        CsrBtHfgMainXSecurityInReqHandler,                            /* CSR_BT_HFG_SECURITY_IN_REQ */
        CsrBtHfgMainXSecurityOutReqHandler,                           /* CSR_BT_HFG_SECURITY_OUT_REQ */
#ifdef CSR_BT_INSTALL_HFG_CONFIG_AUDIO
        CsrBtHfgMainSaveMessage,                                      /* CSR_BT_HFG_CONFIG_AUDIO_REQ */
#else
        NULL,                                                         /* CSR_BT_HFG_CONFIG_AUDIO_REQ */
#endif
        NULL,                                                         /* CSR_BT_HFG_RING_REQ */
        NULL,                                                         /* CSR_BT_HFG_CALL_WAITING_REQ */
        NULL,                                                         /* CSR_BT_HFG_CALL_HANDLING_REQ */
        NULL,                                                         /* CSR_BT_HFG_CALL_HANDLING_RES */
        NULL,                                                         /* CSR_BT_HFG_DIAL_RES */
        NULL,                                                         /* CSR_BT_HFG_SPEAKER_GAIN_REQ */
        NULL,                                                         /* CSR_BT_HFG_MIC_GAIN_REQ */
        NULL,                                                         /* CSR_BT_HFG_AT_CMD_REQ */
        NULL,                                                         /* CSR_BT_HFG_OPERATOR_RES */
        NULL,                                                         /* CSR_BT_HFG_CALL_LIST_RES */
        NULL,                                                         /* CSR_BT_HFG_SUBSCRIBER_NUMBER_RES */
        CsrBtHfgMainIndicatorSetupReqHandler,                         /* CSR_BT_HFG_STATUS_INDICATOR_SET_REQ */
        NULL,                                                         /* CSR_BT_HFG_INBAND_RINGING_REQ */
        NULL,                                                         /* CSR_BT_HFG_BT_INPUT_RES */
        NULL,                                                         /* CSR_BT_HFG_VOICE_RECOG_REQ */
        NULL,                                                         /* CSR_BT_HFG_VOICE_RECOG_RES */
        NULL,                                                         /* CSR_BT_HFG_MANUAL_INDICATOR_RES */
        CsrBtHfgXConfigSingleAtcmd,                                   /* CSR_BT_HFG_CONFIG_SINGLE_ATCMD_REQ */
        CsrBtHfgXConfigAtCmdHandling,                                 /* CSR_BT_HFG_CONFIG_ATCMD_HANDLING_REQ */
        CsrBtHfgMainXSetDeregisterTimeReqHandler,                     /* CSR_BT_HFG_DEREGISTER_TIME_REQ */
        NULL,                                                         /* CSR_BT_HFG_SET_HF_INDICATOR_STATUS_REQ */
#ifdef CSR_BT_HFG_ENABLE_SWB_SUPPORT
        NULL,                                                         /* CSR_BT_HFG_SWB_RSP */
#endif
    },
    /* MainActive_s (see csr_bt_hfg_con.c for second-level jump-table) */
    {
        NULL,                                                    /* CSR_BT_HFG_ACTIVATE_REQ */
        CsrBtHfgMainActiveHfgDeactivateReqHandler,                    /* CSR_BT_HFG_DEACTIVATE_REQ */
        CsrBtHfgMainActiveHfgServiceConnectReqHandler,                /* CSR_BT_HFG_SERVICE_CONNECT_REQ */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_CANCEL_CONNECT_REQ */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_DISCONNECT_REQ */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_AUDIO_CONNECT_REQ */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_RES */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_AUDIO_DISCONNECT_REQ */
        CsrBtHfgMainXSecurityInReqHandler,                            /* CSR_BT_HFG_SECURITY_IN_REQ */
        CsrBtHfgMainXSecurityOutReqHandler,                           /* CSR_BT_HFG_SECURITY_OUT_REQ */
#ifdef CSR_BT_INSTALL_HFG_CONFIG_AUDIO
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_CONFIG_AUDIO_REQ */
#else
        NULL,                                                         /* CSR_BT_HFG_CONFIG_AUDIO_REQ */
#endif
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_RING_REQ */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_CALL_WAITING_REQ */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_CALL_HANDLING_REQ */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_CALL_HANDLING_RES */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_DIAL_RES */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_SPEAKER_GAIN_REQ */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_MIC_GAIN_REQ */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_AT_CMD_REQ */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_OPERATOR_RES */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_CALL_LIST_RES */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_SUBSCRIBER_NUMBER_RES */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_STATUS_INDICATOR_SET_REQ */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_INBAND_RINGING_REQ */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_BT_INPUT_RES */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_VOICE_RECOG_REQ */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_VOICE_RECOG_RES */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_MANUAL_INDICATOR_RES */
        CsrBtHfgXConfigSingleAtcmd,                                   /* CSR_BT_HFG_CONFIG_SINGLE_ATCMD_REQ */
        CsrBtHfgXConfigAtCmdHandling,                                 /* CSR_BT_HFG_CONFIG_ATCMD_HANDLING_REQ */
        CsrBtHfgMainXSetDeregisterTimeReqHandler,                     /* CSR_BT_HFG_DEREGISTER_TIME_REQ */
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_SET_HF_INDICATOR_STATUS_REQ */
#ifdef CSR_BT_HFG_ENABLE_SWB_SUPPORT
        CsrBtHfgMainXSecondHfg,                                       /* CSR_BT_HFG_SWB_RSP */
#endif

    },
    /* MainDeactivate_s */
    {
        NULL,                                                         /* CSR_BT_HFG_ACTIVATE_REQ */
        NULL,                                                         /* CSR_BT_HFG_DEACTIVATE_REQ */
        NULL,                                                         /* CSR_BT_HFG_SERVICE_CONNECT_REQ */
        NULL,                                                         /* CSR_BT_HFG_CANCEL_CONNECT_REQ */
        NULL,                                                         /* CSR_BT_HFG_DISCONNECT_REQ */
        NULL,                                                         /* CSR_BT_HFG_AUDIO_CONNECT_REQ */
        NULL,                                                         /* CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                         /* CSR_BT_HFG_AUDIO_DISCONNECT_REQ */
        CsrBtHfgMainXSecurityInReqHandler,                            /* CSR_BT_HFG_SECURITY_IN_REQ */
        CsrBtHfgMainXSecurityOutReqHandler,                           /* CSR_BT_HFG_SECURITY_OUT_REQ */
        NULL,                                                         /* CSR_BT_HFG_CONFIG_AUDIO_REQ */
        NULL,                                                         /* CSR_BT_HFG_RING_REQ */
        NULL,                                                         /* CSR_BT_HFG_CALL_WAITING_REQ */
        NULL,                                                         /* CSR_BT_HFG_CALL_HANDLING_REQ */
        NULL,                                                         /* CSR_BT_HFG_CALL_HANDLING_RES */
        NULL,                                                         /* CSR_BT_HFG_DIAL_RES */
        NULL,                                                         /* CSR_BT_HFG_SPEAKER_GAIN_REQ */
        NULL,                                                         /* CSR_BT_HFG_MIC_GAIN_REQ */
        NULL,                                                         /* CSR_BT_HFG_AT_CMD_REQ */
        NULL,                                                         /* CSR_BT_HFG_OPERATOR_RES */
        NULL,                                                         /* CSR_BT_HFG_CALL_LIST_RES */
        NULL,                                                         /* CSR_BT_HFG_SUBSCRIBER_NUMBER_RES */
        NULL,                                                         /* CSR_BT_HFG_STATUS_INDICATOR_SET_REQ */
        NULL,                                                         /* CSR_BT_HFG_INBAND_RINGING_REQ */
        NULL,                                                         /* CSR_BT_HFG_BT_INPUT_RES */
        NULL,                                                         /* CSR_BT_HFG_VOICE_RECOG_REQ */
        NULL,                                                         /* CSR_BT_HFG_VOICE_RECOG_RES */
        NULL,                                                         /* CSR_BT_HFG_MANUAL_INDICATOR_RES */
        CsrBtHfgXConfigSingleAtcmd,                                   /* CSR_BT_HFG_CONFIG_SINGLE_ATCMD_REQ */
        CsrBtHfgXConfigAtCmdHandling,                                 /* CSR_BT_HFG_CONFIG_ATCMD_HANDLING_REQ */
        CsrBtHfgMainXSetDeregisterTimeReqHandler,                     /* CSR_BT_HFG_DEREGISTER_TIME_REQ */
        NULL,                                                         /* CSR_BT_HFG_SET_HF_INDICATOR_STATUS_REQ */
#ifdef CSR_BT_HFG_ENABLE_SWB_SUPPORT
        NULL,                                                         /* CSR_BT_HFG_SWB_RSP */
#endif
    }
};

/* CM jump table for top-most CM primitives */
static const HfgMainHandler_t cmMainHandlers[MainNum_s - MainNull_s][CSR_BT_CM_RFC_PRIM_UPSTREAM_COUNT] =
{
    /* MainNull_s */
    {
        NULL,                                                    /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        CsrBtHfgMainNullCmRegisterCfmHandler,                    /* CSR_BT_CM_REGISTER_CFM */
        NULL,                                                    /* CSR_BT_CM_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_SCO_DISCONNECT_IND */
        NULL,                                                    /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        NULL,                                                    /* CSR_BT_CM_DATA_IND */
        NULL,                                                    /* CSR_BT_CM_DATA_CFM */
        NULL,                                                    /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                    /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        CsrBtHfgMainXCmPortnegIndHandler,                        /* CSR_BT_CM_PORTNEG_IND */
        NULL,                                                    /* CSR_BT_CM_PORTNEG_CFM */
    },
    /* MainIdle_s */
    {
        NULL,                                                           /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                           /* CSR_BT_CM_CONNECT_CFM */
        NULL,                                                           /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        NULL,                                                           /* CSR_BT_CM_REGISTER_CFM */
        NULL,                                                           /* CSR_BT_CM_DISCONNECT_IND */
        NULL,                                                           /* CSR_BT_CM_SCO_CONNECT_CFM */
        NULL,                                                           /* CSR_BT_CM_SCO_DISCONNECT_IND */
        NULL,                                                           /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        NULL,                                                           /* CSR_BT_CM_DATA_IND */
        NULL,                                                           /* CSR_BT_CM_DATA_CFM */
        NULL,                                                           /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                           /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        CsrBtHfgMainXCmPortnegIndHandler,                               /* CSR_BT_CM_PORTNEG_IND */
        NULL,                                                           /* CSR_BT_CM_PORTNEG_CFM */
    },
    /* MainActive_s (see csr_bt_hfg_con.c for second level jumptable) */
    {
        CsrBtHfgMainActiveCmCancelAcceptConnectCfmHandler,            /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        CsrBtHfgMainXSecondCm,                                        /* CSR_BT_CM_CONNECT_CFM */
        CsrBtHfgMainActiveCmConnectAcceptCfmHandler,                  /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        NULL,                                                         /* CSR_BT_CM_REGISTER_CFM */
        CsrBtHfgMainXSecondCm,                                        /* CSR_BT_CM_DISCONNECT_IND */
        CsrBtHfgMainXSecondCm,                                        /* CSR_BT_CM_SCO_CONNECT_CFM */
        CsrBtHfgMainXSecondCm,                                        /* CSR_BT_CM_SCO_DISCONNECT_IND */
        CsrBtHfgMainXSecondCm,                                        /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        CsrBtHfgMainXSecondCm,                                        /* CSR_BT_CM_DATA_IND */
        CsrBtHfgMainXSecondCm,                                        /* CSR_BT_CM_DATA_CFM */
        CsrBtHfgMainXIgnore,                                          /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                         /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        CsrBtHfgMainXCmPortnegIndHandler,                             /* CSR_BT_CM_PORTNEG_IND */
        NULL,                                                         /* CSR_BT_CM_PORTNEG_CFM */
    },
    /* MainDeactivate_s */
    {
        CsrBtHfgMainDeactivateCmCancelAcceptConnectCfmHandler,        /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        CsrBtHfgMainDeactivateCmConnectCfmHandler,                    /* CSR_BT_CM_CONNECT_CFM */
        CsrBtHfgMainDeactivateCmConnectAcceptCfmHandler,              /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        NULL,                                                         /* CSR_BT_CM_REGISTER_CFM */
        CsrBtHfgMainDeactivateCmDisconnectIndHandler,                 /* CSR_BT_CM_DISCONNECT_IND */
        CsrBtHfgMainDeactivateCmScoConnectCfmHandler,                 /* CSR_BT_CM_SCO_CONNECT_CFM */
        CsrBtHfgMainDeactivateCmScoDisconnectIndHandler,              /* CSR_BT_CM_SCO_DISCONNECT_IND */
        CsrBtHfgMainDeactivateCmScoAcceptConnectCfmHandler,           /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        CsrBtHfgMainDeactivateCmDataIndHandler,                       /* CSR_BT_CM_DATA_IND */
        CsrBtHfgMainDeactivateCmDataCfmHandler,                       /* CSR_BT_CM_DATA_CFM */
        CsrBtHfgMainXSecondCm,                                        /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                         /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        CsrBtHfgMainXIgnore,                                          /* CSR_BT_CM_PORTNEG_IND */
        NULL,                                                         /* CSR_BT_CM_PORTNEG_CFM */
    }
};

static void csrHfgSdcRfcResultHandler(CsrSdcOptCallbackType cbType, void *context)
{
    CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "csrHfgSdcRfcResultHandler")); 


    switch(cbType)
    {
        case CSR_SDC_OPT_CB_SEARCH_RESULT:
        {
            CsrSdcResultFuncType *params = (CsrSdcResultFuncType *)context;

            CsrBtHfgSdcResultHandler(params->instData,
                                     params->sdpTagList,
                                     params->deviceAddr,
                                     params->resultCode,
                                     params->resultSupplier);
        }
        break;

        case CSR_SDC_OPT_CB_CON_SELECT_SERVICE_HANDLE:
        {
            CsrRfcConSelectServiceHandleType *params = (CsrRfcConSelectServiceHandleType *)context;

            CsrBtHfgSdcSelectServiceHandler(params->instData,
                                            params->cmSdcRfcInstData,
                                            params->deviceAddr,
                                            params->serverChannel,
                                            params->entriesInSdpTaglist,
                                            params->sdpTagList);
        }
        break;

        case CSR_SDC_OPT_RFC_CON_RESULT:
        {
            CsrRfcConResultType *params = (CsrRfcConResultType *) context;

            CsrBtHfgRfcSdcConResultHandler(params->instData,
                                           params->localServerCh,
                                           params->btConnId,
                                           params->deviceAddr,
                                           params->maxFrameSize,
                                           params->validPortPar,
                                           *(params->portPar),
                                           params->resultCode,
                                           params->resultSupplier,
                                           params->sdpTag);
        }
        break;

        default:
            break;
    }
}


#ifdef CSR_BT_GLOBAL_INSTANCE
HfgMainInstance_t csrBtHfgInstance;
#endif

/* Initialize task */
void CsrBtHfgInit(void **gash)
{
    HfgMainInstance_t *inst;
    CsrUintFast8 i;

    CSR_LOG_TEXT_REGISTER(&CsrBtHfgLto, "BT_HFG", 0, NULL);

#ifdef CSR_BT_GLOBAL_INSTANCE
        *gash = &csrBtHfgInstance;
#else
    /* Allocate and initialise instance data space */
    *gash = (void*)CsrPmemZalloc(sizeof(HfgMainInstance_t));
#endif
    inst = (HfgMainInstance_t*)*gash;

    /* Clear restore flags and timers so we don't try to empty an
     * invalid save-queue or stop stop non-existent timers in the
     * initializer */

    /* Server channels are persistent across activations, so reset
     * that array here */
    for(i=(CSR_BT_HFG_IDX_AG*CSR_BT_HFG_NUM_RECORDS); i<((CSR_BT_HFG_IDX_AG+1)*CSR_BT_HFG_NUM_RECORDS); i++)
    {
        inst->service[i].chan = CSR_BT_NO_SERVER;
        inst->service[i].type = CSR_BT_HFG_CONNECTION_AG;
    }
    for(i=(CSR_BT_HFG_IDX_HFG*CSR_BT_HFG_NUM_RECORDS); i<((CSR_BT_HFG_IDX_HFG+1)*CSR_BT_HFG_NUM_RECORDS); i++)
    {
        inst->service[i].chan = CSR_BT_NO_SERVER;
        inst->service[i].type = CSR_BT_HFG_CONNECTION_HFG;
    }
    /* Ok, reset the instance data */
    CsrBtHfgResetProfile(inst);

    /* Default value defined in user_config.h
       First byte, bit 0 represents the first AT command in the table;
       First byte, bit 1 represents the second AT command in the table;
       ...
       Second byte, bit 0 represents the eigth AT command in the table;
       and so forth.*/
    inst->HfgSendAtToApp[0] = CSR_BT_HFG_AT_CUSTOM_BYTE_0;
    inst->HfgSendAtToApp[1] = CSR_BT_HFG_AT_CUSTOM_BYTE_1;
    inst->HfgSendAtToApp[2] = CSR_BT_HFG_AT_CUSTOM_BYTE_2;
    inst->HfgSendAtToApp[3] = CSR_BT_HFG_AT_CUSTOM_BYTE_3;
    inst->HfgSendAtToApp[4] = CSR_BT_HFG_AT_CUSTOM_BYTE_4;
    inst->HfgSendAtToApp[5] = CSR_BT_HFG_AT_CUSTOM_UNUSED;

#ifdef INSTALL_HFG_CUSTOM_SECURITY_SETTINGS
    CsrBtScSetSecInLevel(&inst->secIncoming, CSR_BT_SEC_DEFAULT,
        CSR_BT_HANDSFREE_GW_MANDATORY_SECURITY_INCOMING,
        CSR_BT_HANDSFREE_GW_DEFAULT_SECURITY_INCOMING,
        CSR_BT_RESULT_CODE_HFG_SUCCESS,
        CSR_BT_RESULT_CODE_HFG_UNACCEPTABLE_PARAMETER);

    CsrBtScSetSecOutLevel(&inst->secOutgoing, CSR_BT_SEC_DEFAULT,
        CSR_BT_HANDSFREE_GW_MANDATORY_SECURITY_OUTGOING,
        CSR_BT_HANDSFREE_GW_DEFAULT_SECURITY_OUTGOING,
        CSR_BT_RESULT_CODE_HFG_SUCCESS,
        CSR_BT_RESULT_CODE_HFG_UNACCEPTABLE_PARAMETER);
#endif /* INSTALL_HFG_CUSTOM_SECURITY_SETTINGS */

    /* Common search lib stuff */
    inst->sdpHfgSearchConData = CsrBtUtilSdpRfcConInit(csrHfgSdcRfcResultHandler,
                                                       CSR_SDC_OPT_CB_SELECT_SVC_HANDLE_MASK | CSR_SDC_OPT_CB_RFC_CON_RESULT_MASK,
                                                       CSR_BT_HFG_IFACEQUEUE);

    inst->sdpHfgSearchData    = CsrBtUtilSdcInit(csrHfgSdcRfcResultHandler,
                                                 CSR_BT_HFG_IFACEQUEUE);
#ifndef EXCLUDE_CSR_BT_SD_MODULE
    /* Tell the SD that it must look for the CSR_BT_HF_PROFILE_UUID service,
       when it perform a SD_READ_AVAILABLE_SERVICE_REQ                  */
    CsrBtSdRegisterAvailableServiceReqSend(CSR_BT_HF_PROFILE_UUID);
#endif

    /* Send a register to CM to allocate the internal server channel
     * number for the client role */
    inst->modemStatus = CsrBtMapSendingControlSignal(CSR_BT_MODEM_CTS_MASK | CSR_BT_MODEM_DSR_MASK, CSR_BT_DCE);
    CsrBtCmRegisterReqSend(CSR_BT_HFG_IFACEQUEUE);
    inst->localHfIndicatorList = NULL;
    inst->indCount = 0;

}

/* Deinitialize task */
#ifdef ENABLE_SHUTDOWN
void CsrBtHfgDeinit(void **gash)
{
    HfgMainInstance_t *inst;
    CsrUint16 msg_type;
    void *msg_data;
    CsrBool found;
    CsrUint8 i;

    inst = (HfgMainInstance_t*)(*gash);

    CsrBtUtilSdcRfcDeinit(&(inst->sdpHfgSearchData));
    CsrBtUtilSdcRfcDeinit(&(inst->sdpHfgSearchConData));

    /* Reset profile to stop timers etc. */
    CsrBtHfgResetProfile(inst);

    /* Continue to poll any message of the input queue */
    found = TRUE;
    while(found)
    {
        /* Pop off global save queues first */
        msg_data = NULL;
        msg_type = 0;
        found = CsrMessageQueuePop(&inst->saveQueue, &msg_type, &msg_data);

        /* Try local savequeues */
        i = 0;
        while(!found && (i<CSR_BT_HFG_NUM_SERVERS))
        {
            found = CsrMessageQueuePop(&(inst->linkData[i].saveQueue),
                               &msg_type,
                               &msg_data);
            i++;
        }

        /* And finally, try scheduler queue */
        if(!found)
        {
            found = CsrSchedMessageGet(&msg_type, &msg_data);
        }

        if(found && (msg_data != NULL))
        {
            CsrBtHfgFreeMessage(msg_type, msg_data);
        }
    }
#ifdef CSR_BT_GLOBAL_INSTANCE
    CsrMemSet(inst, 0, sizeof(HfgMainInstance_t));
#else
    CsrPmemFree(inst);
#endif /* CSR_BT_GLOBAL_INSTANCE */
    *gash = NULL;
}
#endif

/* If Null state and activate request in queue, make sure that activate request is the first message handled */
static CsrBool CsrBtHfgNullStateFindActivateInQueue(HfgMainInstance_t *inst, CsrUint16 *class, void **msg)
{
    CsrBool found = FALSE;

    if (inst->state == MainNull_s)
    {
        CsrMessageQueueType *localSaveQueue = NULL;

        while (CsrMessageQueuePop(&inst->saveQueue, class, msg) && !found)
        {
            CsrBtHfgPrim *primType = (CsrBtHfgPrim*)*msg;

            if ((*primType == CSR_BT_HFG_ACTIVATE_REQ) && (*class == CSR_BT_HFG_PRIM))
            {/* found: restore the global queue, return TRUE and handle the activate request primitive */
                found = TRUE;
                break;
            }
            else
            {/* Not the primitive searched for: insert in the local temp queue */
                CsrMessageQueuePush(&localSaveQueue, *class, *msg);
            }
        }
        
        if (localSaveQueue)
        {/* Move from temporary queue back to global save-queue */
            void *tmpMsg;
            CsrUint16 tmpClass;
            
            while(CsrMessageQueuePop(&localSaveQueue, &tmpClass, &tmpMsg))
            {
                CsrMessageQueuePush(&inst->saveQueue, tmpClass, tmpMsg);
            }
        }
    }
    
    if (!found)
    {
        *msg = NULL;
        *class = 0;
    }

    return found;
}

/* Task handler */
void CsrBtHfgHandler(void **gash)
{
    HfgMainInstance_t *inst;
    CsrUint16 class;
    void *msg;
    CsrUint8 i;

    inst = (HfgMainInstance_t*)(*gash);
    msg = NULL;
    class = 0;

    /* Pop from local save queue */
    for(i=0; i<CSR_BT_HFG_NUM_SERVERS; i++)
    {
        if(inst->linkData[i].restoreFlag)
        {
            if(!CsrMessageQueuePop(&(inst->linkData[i].saveQueue), &class, &msg))
            { /* The local saveQueue is empty                           */
                inst->linkData[i].restoreFlag = FALSE;
                msg = NULL;
            }
            else
            { /* A message has been restored from the local saveQueue.
                 Break the for loop to start handling this message      */
                break;
            }
        }
    }

    /* Pop from global save queue */
    if(msg == NULL)
    {
        if(inst->restoreFlag)
        {
            if (!CsrBtHfgNullStateFindActivateInQueue(inst, &class, &msg))
            {
                if(!CsrMessageQueuePop(&inst->saveQueue, &class, &msg))
                {
                    inst->restoreFlag = FALSE;
                    msg = NULL;
                }
            }
        }
    }

    /* Get from scheduler */
    if(msg == NULL)
    {
        CsrSchedMessageGet(&class , &msg);
    }
    inst->msg = msg;
    inst->msgClass = class;

    CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "CsrBtHfgHandler class = 0x%x", class)); 

    switch(class)
    {
        case CSR_BT_HFG_PRIM:
            {
                CsrBtHfgPrim         *primType;

                /* find the message type */
                primType = (CsrBtHfgPrim *)msg;
                CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "CsrBtHfgHandler CSR_BT_HFG_PRIM prim = 0x%x", *primType)); 

                if((*primType < CSR_BT_HFG_PRIM_DOWNSTREAM_COUNT) &&
                   (hfgMainHandlers[inst->state][*primType] != NULL))
                {
                    hfgMainHandlers[inst->state][*primType](inst);
                }
                else if(*primType != CSR_BT_HFG_HOUSE_CLEANING)
                {
                    /* State/Event ERROR! */
                    CsrGeneralException(CsrBtHfgLto,
                                        0,
                                        CSR_BT_HFG_PRIM,
                                        (CsrUint16)*primType,
                                        (CsrUint16)inst->state,
                                        "Unknown HFG prim or undefined state");
                }
                break;
            }

#ifdef CSR_STREAMS_ENABLE
        case MESSAGE_MORE_SPACE:
        {
            CsrBtHfgMessageMoreSpaceHandler(inst);
            break;
        }
        case MESSAGE_MORE_DATA:
        {
            CsrBtHfgMessageMoreDataHandler(inst);
            break;
        }
#endif /* CSR_STREAMS_ENABLE */

        case CSR_BT_CM_PRIM:
            {
                CsrPrim *primType;

                /* Find the message type */
                primType = (CsrPrim *)msg;
                CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "CsrBtHfgHandler CSR_BT_CM_PRIM prim = 0x%x", *primType)); 

                if(((CsrUint16)(*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST) <  CSR_BT_CM_RFC_PRIM_UPSTREAM_COUNT) &&
                   (cmMainHandlers[inst->state][(CsrUint16)(*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)] != NULL))
                {
                    cmMainHandlers[inst->state][(CsrUint16)(*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)](inst);
                }
                /* To avoid unnecessary large jump tables, catch
                 * a few special primitives here*/
                else if(*primType == CSR_BT_CM_MAP_SCO_PCM_IND)
                {
                    CsrBtHfgMainXSecondCm(inst);
                }
                else if((*primType == CSR_BT_CM_WRITE_LINK_POLICY_ERROR_IND) || (*primType == CSR_BT_CM_SDC_RELEASE_RESOURCES_CFM) ||
                        (*primType == CSR_BT_CM_WRITE_VOICE_SETTINGS_CFM))
                {
                    /* Error writing link policy - ignore */
                    /* Or release_resources_cfm received after service search operation close: ignore */
                    /* Or change voice settings cfm received - just ignore by now....*/
                }
                else if(CSR_BT_CM_SCO_RENEGOTIATE_IND == *primType)
                {
                    /* To avoid unnecessary large jump tables the
                     * profile does support renegotiation, no
                     * instruction is given in spec for handling this
                     * primitive, the primitive is simply ignored */
                }
                else if (CSR_BT_CM_SDS_REGISTER_CFM == *primType)
                {
                    if (MainIdle_s == inst->state)
                    {
                        CsrBtHfgMainIdleCmSdsRegisterCfmHandler(inst);
                    }
                    else if (MainActive_s == inst->state)
                    {
                        CsrBtHfgMainActiveCmSdsRegisterCfmHandler(inst);
                    }
                    else if (MainNull_s == inst->state)
                    {
                        CsrBtHfgMainNullCmSdsRegisterCfmHandler(inst);
                    }
                    else if (MainDeactivate_s == inst->state)
                    {
                        CsrBtHfgMainDeactivateCmSdsRegisterCfmHandler(inst);
                    }
                    else
                    {
                        CsrGeneralException(CsrBtHfgLto,
                                            0,
                                            CSR_BT_CM_PRIM,
                                            (CsrUint16)*primType,
                                            (CsrUint16)inst->state,
                                            "Unknown CM prim");
                    }
                }
                else if (CSR_BT_CM_SDS_UNREGISTER_CFM == *primType)
                {
                    if (MainActive_s == inst->state)
                    {
                        CsrBtHfgMainActiveCmSdsUnregisterCfmHandler(inst);
                    }
                    else if (MainDeactivate_s == inst->state)
                    {
                        CsrBtHfgMainDeactivateCmSdsUnregisterCfmHandler(inst);
                    }
                    else
                    {
                        CsrBtHfgMainXCmSdsUnregisterCfmHandler(inst);
                    }
                }
                else if (CSR_BT_CM_WRITE_AUTH_PAYLOAD_TIMEOUT_CFM == *primType)
                {
                /* Ignore the APT confirmation, will handle it when profile has dependency on the result code
                 * no instruction is given in the profile spec for handling the error code */
                }
                else if ((MainIdle_s < inst->state) && (CsrBtUtilRfcConVerifyCmMsg(inst->msg)))
                {
                    CsrBtHfgMainXSecondCm(inst);
                }
                else
                {
                    /* State/Event ERROR! */
                    CsrGeneralException(CsrBtHfgLto,
                                        0,
                                        CSR_BT_CM_PRIM,
                                        (CsrUint16)*primType,
                                        (CsrUint16)inst->state,
                                        "Unknown CM prim or undefined state");
                }
                break;
            }
        case CSR_SCHED_PRIM:
            /* Environment cleanup */
            {
                switch (inst->state)
                {
                    case MainNull_s:
                        /* Ignore message in null state */
                        break;

                    case MainIdle_s:
                        CsrBtHfgMainIdleActiveEnvCleanupHandler(inst);
                        break;

                    case MainActive_s:
                        CsrBtHfgMainIdleActiveEnvCleanupHandler(inst);
                        break;

                    case MainDeactivate_s:
                        /* Ignore message in deactivate state */
                        break;

                    default:
                        /* State/event error */
                        CsrGeneralException(CsrBtHfgLto,
                                            0,
                                            CSR_SCHED_PRIM,
                                            (CsrUint16)0xFFFF,
                                            (CsrUint16)inst->state,
                                            "Unknown state");
                        break;
                }
                break;
            }


        default:
            /* State/event error */
            CsrGeneralException(CsrBtHfgLto,
                                0,
                                class,
                                (CsrUint16)0xFFFF,
                                (CsrUint16)inst->state,
                                "Unknown primitive type received in main");
            break;
    }

    /* Free the received message and it's members using the generic
     * free function */
    if(inst->msg != NULL)
    {
        CsrBtHfgFreeMessage(inst->msgClass, inst->msg);
    }
    inst->msg = NULL;
    inst->msgClass = 0;
}

#ifdef CSR_STREAMS_ENABLE
void CsrBtHfgCmDataPrimHandler(HfgMainInstance_t *inst)
{
    CsrPrim *primType;
    /* Find the message type */
    primType = (CsrPrim *)inst->msg;

    if(((CsrUint16)(*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST) <  CSR_BT_CM_RFC_PRIM_UPSTREAM_COUNT) &&
       (cmMainHandlers[inst->state][(CsrUint16)(*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)] != NULL))
    {
        cmMainHandlers[inst->state][(CsrUint16)(*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)](inst);
    }
    else
    {
        /* State/Event ERROR! */
        CsrGeneralException(CsrBtHfgLto,
                            0,
                            CSR_BT_CM_PRIM,
                            (CsrUint16)*primType,
                            (CsrUint16)inst->state,
                            "Unknown CM prim or undefined state");
    }
}
#endif /* CSR_STREAMS_ENABLE */

#if defined(CSR_LOG_ENABLE) && defined(CSR_LOG_ENABLE_STATE_TRANSITION)
void HfgChangeState(const CsrCharString *file, CsrUint32 lineno, 
                        CsrUint32 *state, const CsrCharString *stateName, 
                        CsrUint32 newState, const CsrCharString *newStateName)
{
    CsrLogStateTransition(CsrBtHfgLto,
                          0,
                          CSR_LOG_STATE_TRANSITION_MASK_FSM_NAME |
                          CSR_LOG_STATE_TRANSITION_MASK_NEXT_STATE |
                          CSR_LOG_STATE_TRANSITION_MASK_NEXT_STATE_STR |
                          CSR_LOG_STATE_TRANSITION_MASK_PREV_STATE,
                          0,
                          stateName,
                          *state,
                          NULL,
                          0,
                          NULL,
                          newState,
                          newStateName,
                          lineno,
                          file);

    *state = newState;
}
#endif

