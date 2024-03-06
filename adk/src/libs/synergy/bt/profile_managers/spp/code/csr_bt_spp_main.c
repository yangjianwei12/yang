/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_SPP_MODULE
#include "csr_panic.h"
#include "csr_sched.h"
#include "csr_pmem.h"
#include "csr_env_prim.h"
#include "bluetooth.h"
#include "hci_prim.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_util.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_SD_MODULE
#include "csr_bt_sd_private_lib.h"
#endif
#include "csr_log_text_2.h"
#include "csr_bt_spp_main.h"
#include "csr_bt_spp_prim.h"
#include "csr_bt_spp_sef.h"
#include "sds_prim.h"

#include "csr_bt_cmn_sdc_rfc_util.h"
#include "csr_bt_cmn_sdr_tagbased_lib.h"
#include "csr_bt_cmn_sdp_lib.h"

#ifdef CSR_LOG_ENABLE
/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtSppLto);
#ifdef CSR_TARGET_PRODUCT_VM
#include "csr_bt_spp_prim_enum_dbg.h"
CSR_PRESERVE_GENERATED_ENUM(CsrBtSppPrim)
#endif /*CSR_TARGET_PRODUCT_VM*/
#endif

#ifdef CSR_BT_GLOBAL_INSTANCE
SppInstanceData_t sppInstanceData;
#endif

void CsrBtSppSaveMessage(SppInstanceData_t *instData)
{
    CsrMessageQueuePush(&instData->saveQueue, CSR_BT_SPP_PRIM, instData->recvMsgP);
    instData->recvMsgP = NULL;
}

/* ---------- SPP jump table ----------*/
static const SppStateHandlerType sppStateHandlers[SppNumberOfStates][CSR_BT_SPP_PRIM_DOWNSTREAM_COUNT] =
{
    /* init_s */
    {
        CsrBtSppSaveMessage,                                             /* CSR_BT_SPP_CONNECT_REQ */
        CsrBtSppSaveMessage,                                             /* CSR_BT_SPP_ACTIVATE_REQ */
        CsrBtSppSaveMessage,                                             /* CSR_BT_SPP_DEACTIVATE_REQ */
        CsrBtSppSaveMessage,                                             /* CSR_BT_SPP_DISCONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_DATA_REQ */
        NULL,                                                            /* CSR_BT_SPP_DATA_RES */
        NULL,                                                            /* CSR_BT_SPP_CONTROL_REQ */
        NULL,                                                            /* CSR_BT_SPP_PORTNEG_RES */
        CsrBtSppXStatePortnegReqHandler,                                 /* CSR_BT_SPP_PORTNEG_REQ */
        NULL,                                                            /* CSR_BT_SPP_SERVICE_NAME_RES */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_CONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_ACCEPT_AUDIO_REQ */
        NULL,                                                            /* CSR_BT_SPP_CANCEL_ACCEPT_AUDIO_REQ */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_DISCONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_EXTENDED_CONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_EXTENDED_UUID_CONNECT_REQ */
        CsrBtSppInitStateSppExtendedActivateReqHandler,                  /* CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */
        CsrBtSppAnyStateRegisterDataPathHandleReqHandler,                /* CSR_BT_SPP_REGISTER_DATA_PATH_HANDLE_REQ */
        NULL,                                                            /* CSR_BT_SPP_DATA_PATH_STATUS_REQ */
        CsrBtSppSaveMessage,                                             /* SPP_GET_INSTANCE_QID_REQ */
        CsrBtSppAllStateSppRegisterQIDReqHandler,                        /* CSR_BT_SPP_REGISTER_QID_REQ */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_RENEGOTIATE_REQ */
        CsrBtSppInitStateSppCancelConnectReqHandler,                     /* CSR_BT_SPP_CANCEL_CONNECT_REQ */
        CsrBtSppSecurityInReqHandler,                                    /* CSR_BT_SPP_SECURITY_IN_REQ */
        CsrBtSppSecurityOutReqHandler,                                   /* CSR_BT_SPP_SECURITY_OUT_REQ */
    },
    /* idle_s */
    {
        CsrBtSppIdleStateSppConnectReqHandler,                           /* CSR_BT_SPP_CONNECT_REQ */
        CsrBtSppIdleStateSppActivateReqHandler,                          /* CSR_BT_SPP_ACTIVATE_REQ */
        CsrBtSppIdleStateSppDeactivateReqHandler,                        /* CSR_BT_SPP_DEACTIVATE_REQ */
        CsrBtSppIgnoreMessageHandler,                                    /* CSR_BT_SPP_DISCONNECT_REQ */
        CsrBtSppNotConnectedSppDataReqHandler,                           /* CSR_BT_SPP_DATA_REQ */
        CsrBtSppIgnoreMessageHandler,                                    /* CSR_BT_SPP_DATA_RES */
        NULL,                                                            /* CSR_BT_SPP_CONTROL_REQ */
        CsrBtSppConnectedStateSppPortnegResHandler,                      /* CSR_BT_SPP_PORTNEG_RES */
        CsrBtSppXStatePortnegReqHandler,                                 /* CSR_BT_SPP_PORTNEG_REQ */
        CsrBtSppIdleStateSppServiceNameResHandler,                       /* CSR_BT_SPP_SERVICE_NAME_RES */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_CONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_ACCEPT_AUDIO_REQ */
        NULL,                                                            /* CSR_BT_SPP_CANCEL_ACCEPT_AUDIO_REQ */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_DISCONNECT_REQ */
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
        CsrBtSppIdleStateSppExtendedConnectReqHandler,                   /* CSR_BT_SPP_EXTENDED_CONNECT_REQ */
        CsrBtSppIdleStateSppExtendedUuidConnectReqHandler,               /* CSR_BT_SPP_EXTENDED_UUID_CONNECT_REQ */
#else
        NULL,                                                            /* CSR_BT_SPP_EXTENDED_CONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_EXTENDED_UUID_CONNECT_REQ */
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
        CsrBtSppIdleStateSppExtendedActivateReqHandler,                  /* CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */
        CsrBtSppAnyStateRegisterDataPathHandleReqHandler,                /* CSR_BT_SPP_REGISTER_DATA_PATH_HANDLE_REQ */
        NULL,                                                            /* CSR_BT_SPP_DATA_PATH_STATUS_REQ */
        CsrBtSppAllStateSppGetInstancesQIDReqHandler,                    /* CSR_BT_SPP_GET_INSTANCES_QID_REQ */
        CsrBtSppAllStateSppRegisterQIDReqHandler,                        /* CSR_BT_SPP_REGISTER_QID_REQ */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_RENEGOTIATE_REQ */
        CsrBtSppXStateSppCancelConnectReqHandler,                        /* CSR_BT_SPP_CANCEL_CONNECT_REQ */
        CsrBtSppSecurityInReqHandler,                                    /* CSR_BT_SPP_SECURITY_IN_REQ */
        CsrBtSppSecurityOutReqHandler,                                   /* CSR_BT_SPP_SECURITY_OUT_REQ */
    },
    /* activated_s */
    {
        CsrBtSppActivateStateSppConnectReqHandler,                       /* CSR_BT_SPP_CONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_ACTIVATE_REQ */
        CsrBtSppActivateStateSppDeactivateReqHandler,                    /* CSR_BT_SPP_DEACTIVATE_REQ */
        NULL,                                                            /* CSR_BT_SPP_DISCONNECT_REQ */
        CsrBtSppNotConnectedSppDataReqHandler,                           /* CSR_BT_SPP_DATA_REQ */
        NULL,                                                            /* CSR_BT_SPP_DATA_RES */
        NULL,                                                            /* CSR_BT_SPP_CONTROL_REQ */
        CsrBtSppConnectedStateSppPortnegResHandler,                      /* CSR_BT_SPP_PORTNEG_RES */
        CsrBtSppXStatePortnegReqHandler,                                 /* CSR_BT_SPP_PORTNEG_REQ */
        NULL,                                                            /* CSR_BT_SPP_SERVICE_NAME_RES */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_CONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_ACCEPT_AUDIO_REQ */
        NULL,                                                            /* CSR_BT_SPP_CANCEL_ACCEPT_AUDIO_REQ */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_DISCONNECT_REQ */
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
        CsrBtSppActivateStateSppExtendedConnectReqHandler,               /* CSR_BT_SPP_EXTENDED_CONNECT_REQ */
        CsrBtSppActivateStateSppExtendedUuidConnectReqHandler,           /* CSR_BT_SPP_EXTENDED_UUID_CONNECT_REQ */
#else
        NULL,                                                            /* CSR_BT_SPP_EXTENDED_CONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_EXTENDED_UUID_CONNECT_REQ */
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
        CsrBtSppDummyStateSppActivateReqHandler,                         /* CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */
        CsrBtSppAnyStateRegisterDataPathHandleReqHandler,                /* CSR_BT_SPP_REGISTER_DATA_PATH_HANDLE_REQ */
        NULL,                                                            /* CSR_BT_SPP_DATA_PATH_STATUS_REQ */
        CsrBtSppAllStateSppGetInstancesQIDReqHandler,                    /* CSR_BT_SPP_GET_INSTANCES_QID_REQ */
        CsrBtSppAllStateSppRegisterQIDReqHandler,                        /* CSR_BT_SPP_REGISTER_QID_REQ */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_RENEGOTIATE_REQ */
        CsrBtSppXStateSppCancelConnectReqHandler,                        /* CSR_BT_SPP_CANCEL_CONNECT_REQ */
        CsrBtSppSecurityInReqHandler,                                    /* CSR_BT_SPP_SECURITY_IN_REQ */
        CsrBtSppSecurityOutReqHandler,                                   /* CSR_BT_SPP_SECURITY_OUT_REQ */
    },
    /* connected_s */
    {
        NULL,                                                            /* CSR_BT_SPP_CONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_ACTIVATE_REQ */
        CsrBtSppConnectedStateSppDeactivateReqHandler,                   /* CSR_BT_SPP_DEACTIVATE_REQ */
        CsrBtSppConnectedStateSppDisconnectReqHandler,                   /* CSR_BT_SPP_DISCONNECT_REQ */
        CsrBtSppConnectedStateSppDataReqHandler,                         /* CSR_BT_SPP_DATA_REQ */
        CsrBtSppConnectedStateSppDataResHandler,                         /* CSR_BT_SPP_DATA_RES */
        CsrBtSppConnectedStateSppControlReqHandler,                      /* CSR_BT_SPP_CONTROL_REQ */
        CsrBtSppConnectedStateSppPortnegResHandler,                      /* CSR_BT_SPP_PORTNEG_RES */
        CsrBtSppXStatePortnegReqHandler,                                 /* CSR_BT_SPP_PORTNEG_REQ */
        NULL,                                                            /* CSR_BT_SPP_SERVICE_NAME_RES */
#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
        CsrBtSppConnectedStateSppAudioReqHandler,                        /* CSR_BT_SPP_AUDIO_CONNECT_REQ */
        CsrBtSppConnectedStateSppAcceptAudioReqHandler,                  /* CSR_BT_SPP_ACCEPT_AUDIO_REQ */
        CsrBtSppConnectedStateSppCancelAcceptAudioReqHandler,            /* CSR_BT_SPP_CANCEL_ACCEPT_AUDIO_REQ */
        CsrBtSppConnectedStateSppAudioReleaseReqHandler,                 /* CSR_BT_SPP_AUDIO_DISCONNECT_REQ */
#else /* CSR_BT_INSTALL_SPP_EXTENDED */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_CONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_ACCEPT_AUDIO_REQ */
        NULL,                                                            /* CSR_BT_SPP_CANCEL_ACCEPT_AUDIO_REQ */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_DISCONNECT_REQ */
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#else /* !EXCLUDE_CSR_AM_MODULE */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_CONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_ACCEPT_AUDIO_REQ */
        NULL,                                                            /* CSR_BT_SPP_CANCEL_ACCEPT_AUDIO_REQ */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_DISCONNECT_REQ */
#endif /* EXCLUDE_CSR_AM_MODULE */
        NULL,                                                            /* CSR_BT_SPP_EXTENDED_CONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_EXTENDED_UUID_CONNECT_REQ */
        CsrBtSppDummyStateSppActivateReqHandler,                         /* CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */
        CsrBtSppAnyStateRegisterDataPathHandleReqHandler,                /* CSR_BT_SPP_REGISTER_DATA_PATH_HANDLE_REQ */
        CsrBtSppConnectedStateSppDataPathStatusReqHandler,               /* CSR_BT_SPP_DATA_PATH_STATUS_REQ */
        CsrBtSppAllStateSppGetInstancesQIDReqHandler,                    /* CSR_BT_SPP_GET_INSTANCES_QID_REQ */
        CsrBtSppAllStateSppRegisterQIDReqHandler,                        /* CSR_BT_SPP_REGISTER_QID_REQ */
#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
        CsrBtSppMapScoPcmResHandler,                                     /* CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_RES */
        CsrBtSppConnectedStateSppAudioRenegotiateReqHandler,             /* CSR_BT_SPP_AUDIO_RENEGOTIATE_REQ */
#else /* CSR_BT_INSTALL_SPP_EXTENDED */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_RENEGOTIATE_REQ */
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#else /* !EXCLUDE_CSR_AM_MODULE */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_RES */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_RENEGOTIATE_REQ */
#endif /* EXCLUDE_CSR_AM_MODULE */
        CsrBtSppConnectedStateSppCancelConnectReqHandler,                /* CSR_BT_SPP_CANCEL_CONNECT_REQ */
        CsrBtSppSecurityInReqHandler,                                    /* CSR_BT_SPP_SECURITY_IN_REQ */
        CsrBtSppSecurityOutReqHandler,                                   /* CSR_BT_SPP_SECURITY_OUT_REQ */
    },
    /* deactivate_s */
    {
        NULL,                                                            /* CSR_BT_SPP_CONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_ACTIVATE_REQ */
        NULL,                                                            /* CSR_BT_SPP_DEACTIVATE_REQ */
        NULL,                                                            /* CSR_BT_SPP_DISCONNECT_REQ */
        CsrBtSppNotConnectedSppDataReqHandler,                           /* CSR_BT_SPP_DATA_REQ */
        NULL,                                                            /* CSR_BT_SPP_DATA_RES */
        NULL,                                                            /* CSR_BT_SPP_CONTROL_REQ */
        NULL,                                                            /* CSR_BT_SPP_PORTNEG_RES */
        CsrBtSppXStatePortnegReqHandler,                                 /* CSR_BT_SPP_PORTNEG_REQ */
        NULL,                                                            /* CSR_BT_SPP_SERVICE_NAME_RES */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_CONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_ACCEPT_AUDIO_REQ */
        NULL,                                                            /* CSR_BT_SPP_CANCEL_ACCEPT_AUDIO_REQ */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_DISCONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_EXTENDED_CONNECT_REQ */
        NULL,                                                            /* CSR_BT_SPP_EXTENDED_UUID_CONNECT_REQ */
        CsrBtSppDummyStateSppActivateReqHandler,                         /* CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */
        CsrBtSppAnyStateRegisterDataPathHandleReqHandler,                /* CSR_BT_SPP_REGISTER_DATA_PATH_HANDLE_REQ */
        NULL,                                                            /* CSR_BT_SPP_DATA_PATH_STATUS_REQ */
        CsrBtSppAllStateSppGetInstancesQIDReqHandler,                    /* CSR_BT_SPP_GET_INSTANCES_QID_REQ */
        CsrBtSppAllStateSppRegisterQIDReqHandler,                        /* CSR_BT_SPP_REGISTER_QID_REQ */
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
        CsrBtSppMapScoPcmResHandler,                                     /* CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_RES */
#else
        NULL,                                                            /* CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_RES */
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
        NULL,                                                            /* CSR_BT_SPP_AUDIO_RENEGOTIATE_REQ */
        CsrBtSppDeactivateStateSppCancelConnectReqHandler,               /* CSR_BT_SPP_CANCEL_CONNECT_REQ */
        CsrBtSppSecurityInReqHandler,                                    /* CSR_BT_SPP_SECURITY_IN_REQ */
        CsrBtSppSecurityOutReqHandler,                                   /* CSR_BT_SPP_SECURITY_OUT_REQ */
    }
};

/* ---------- CM jump table ---------- */
static const SppStateHandlerType cmStateHandlers[SppNumberOfStates][CSR_BT_CM_RFC_PRIM_UPSTREAM_COUNT] =
{
    /* init_s */
    {
        NULL,                                                            /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                            /* CSR_BT_CM_CONNECT_CFM */
        NULL,                                                            /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        CsrBtSppInitStateCmRegisterCfmHandler,                           /* CSR_BT_CM_REGISTER_CFM */
        NULL,                                                            /* CSR_BT_CM_DISCONNECT_IND */
        NULL,                                                            /* CSR_BT_CM_SCO_CONNECT_CFM */
        NULL,                                                            /* CSR_BT_CM_SCO_DISCONNECT_IND */
        NULL,                                                            /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        CsrBtSppNotConnectedCmDataIndHandler,                            /* CSR_BT_CM_DATA_IND */
        NULL,                                                            /* CSR_BT_CM_DATA_CFM */
        NULL,                                                            /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                            /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        NULL,                                                            /* CSR_BT_CM_PORTNEG_IND */
        NULL,                                                            /* CSR_BT_CM_PORTNEG_CFM */
    },
    /* idle_s */
    {
        CsrBtSppIdleStateCmCancelAcceptConnectCfmHandler,                /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                            /* CSR_BT_CM_CONNECT_CFM */
        NULL,                                                            /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        CsrBtSppInitStateCmRegisterCfmHandler,                           /* CSR_BT_CM_REGISTER_CFM */
        CsrBtSppIdleStateCmDisconnectIndHandler,                         /* CSR_BT_CM_DISCONNECT_IND */
        NULL,                                                            /* CSR_BT_CM_SCO_CONNECT_CFM */
#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
        CsrBtSppIgnoreMessageHandler,                                    /* CSR_BT_CM_SCO_DISCONNECT_IND */
#else
        NULL,                                                            /* CSR_BT_CM_SCO_DISCONNECT_IND */
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#else /* !EXCLUDE_CSR_AM_MODULE */
        NULL,                                                            /* CSR_BT_CM_SCO_DISCONNECT_IND */
#endif /* EXCLUDE_CSR_AM_MODULE */
        NULL,                                                            /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        CsrBtSppNotConnectedCmDataIndHandler,                            /* CSR_BT_CM_DATA_IND */
        NULL,                                                            /* CSR_BT_CM_DATA_CFM */
        NULL,                                                            /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                            /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        NULL,                                                            /* CSR_BT_CM_PORTNEG_IND */
        NULL,                                                            /* CSR_BT_CM_PORTNEG_CFM */
    },
    /* activate_s */
    {
        CsrBtSppActivateStateCmCancelAcceptConnectCfmHandler,            /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                            /* CSR_BT_CM_CONNECT_CFM */
        CsrBtSppDeOrActivatedStateCmConnectAcceptCfmHandler,             /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        NULL,                                                            /* CSR_BT_CM_REGISTER_CFM */
        NULL,                                                            /* CSR_BT_CM_DISCONNECT_IND */
        NULL,                                                            /* CSR_BT_CM_SCO_CONNECT_CFM */
        NULL,                                                            /* CSR_BT_CM_SCO_DISCONNECT_IND */
        NULL,                                                            /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
        CsrBtSppNotConnectedCmDataIndHandler,                            /* CSR_BT_CM_DATA_IND */
        NULL,                                                            /* CSR_BT_CM_DATA_CFM */
        NULL,                                                            /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                            /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        NULL,                                                            /* CSR_BT_CM_PORTNEG_IND */
        NULL,                                                            /* CSR_BT_CM_PORTNEG_CFM */
    },
    /* connected_s */
    {
        NULL,                                                            /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                            /* CSR_BT_CM_CONNECT_CFM */
        NULL,                                                            /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        NULL,                                                            /* CSR_BT_CM_REGISTER_CFM */
        CsrBtSppConnectedStateCmDisconnectIndHandler,                    /* CSR_BT_CM_DISCONNECT_IND */
#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
        CsrBtSppConnectedStateCmScoConnectCfmHandler,                    /* CSR_BT_CM_SCO_CONNECT_CFM */
        CsrBtSppConnectedStateCmScoDisconnectIndHandler,                 /* CSR_BT_CM_SCO_DISCONNECT_IND */
        CsrBtSppConnectedStateCmScoAcceptConnectCfmHandler,              /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
#else
        NULL,                                                            /* CSR_BT_CM_SCO_CONNECT_CFM */
        NULL,                                                            /* CSR_BT_CM_SCO_DISCONNECT_IND */
        NULL,                                                            /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#else /* !EXCLUDE_CSR_AM_MODULE */
        NULL,                                                            /* CSR_BT_CM_SCO_CONNECT_CFM */
        NULL,                                                            /* CSR_BT_CM_SCO_DISCONNECT_IND */
        NULL,                                                            /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
#endif /* EXCLUDE_CSR_AM_MODULE */
        CsrBtSppConnectedStateCmDataIndHandler,                          /* CSR_BT_CM_DATA_IND */
        CsrBtSppConnectedStateCmDataCfmHandler,                          /* CSR_BT_CM_DATA_CFM */
        CsrBtSppConnectedStateCmControlIndHandler,                       /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                            /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        NULL,                                                            /* CSR_BT_CM_PORTNEG_IND */
        CsrBtCppConnectedStateCmPortnegCfmHandler,                       /* CSR_BT_CM_PORTNEG_CFM */
    },
    /* deactivate_s */
    {
        CsrBtSppDeactivateStateCmCancelAcceptConnectCfmHandler,          /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
        NULL,                                                            /* CSR_BT_CM_CONNECT_CFM */
        CsrBtSppDeOrActivatedStateCmConnectAcceptCfmHandler,             /* CSR_BT_CM_CONNECT_ACCEPT_CFM */
        NULL,                                                            /* CSR_BT_CM_REGISTER_CFM */
        CsrBtSppDeactivateStateCmDisconnectIndHandler,                   /* CSR_BT_CM_DISCONNECT_IND */
#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
        CsrBtSppDeactivateStateCmScoXHandler,                            /* CSR_BT_CM_SCO_CONNECT_CFM */
        CsrBtSppDeactivateStateCmScoXHandler,                            /* CSR_BT_CM_SCO_DISCONNECT_IND */
        CsrBtSppDeactivateStateCmScoXHandler,                            /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
#else
        NULL,                                                            /* CSR_BT_CM_SCO_CONNECT_CFM */
        NULL,                                                            /* CSR_BT_CM_SCO_DISCONNECT_IND */
        NULL,                                                            /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#else /* !EXCLUDE_CSR_AM_MODULE */
        NULL,                                                            /* CSR_BT_CM_SCO_CONNECT_CFM */
        NULL,                                                            /* CSR_BT_CM_SCO_DISCONNECT_IND */
        NULL,                                                            /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM */
#endif /* EXCLUDE_CSR_AM_MODULE */
        CsrBtSppNotConnectedCmDataIndHandler,                            /* CSR_BT_CM_DATA_IND */
        NULL,                                                            /* CSR_BT_CM_DATA_CFM */
        NULL,                                                            /* CSR_BT_CM_CONTROL_IND */
        NULL,                                                            /* CSR_BT_CM_RFC_MODE_CHANGE_IND */
        NULL,                                                            /* CSR_BT_CM_PORTNEG_IND */
        NULL,                                                            /* CSR_BT_CM_PORTNEG_CFM */
    }
};
/* ---------- End of jump tables ---------- */
static void csrBtSppInitCommon(SppInstanceData_t  *sppData)
{
    /* clear own instance data for first time use */
    sppData->state = Init_s;
    sppData->sppConnId = SPP_NO_CONNID;

#ifdef INSTALL_SPP_CUSTOM_SECURITY_SETTINGS
    CsrBtScSetSecInLevel(&sppData->secIncoming, CSR_BT_SEC_DEFAULT,
        CSR_BT_SERIAL_PORT_MANDATORY_SECURITY_INCOMING,
        CSR_BT_SERIAL_PORT_DEFAULT_SECURITY_INCOMING,
        CSR_BT_RESULT_CODE_SPP_SUCCESS,
        CSR_BT_RESULT_CODE_SPP_UNACCEPTABLE_PARAMETER);

    CsrBtScSetSecOutLevel(&sppData->secOutgoing, CSR_BT_SEC_DEFAULT,
        CSR_BT_SERIAL_PORT_MANDATORY_SECURITY_OUTGOING,
        CSR_BT_SERIAL_PORT_DEFAULT_SECURITY_OUTGOING,
        CSR_BT_RESULT_CODE_SPP_SUCCESS,
        CSR_BT_RESULT_CODE_SPP_UNACCEPTABLE_PARAMETER);
#endif /* INSTALL_SPP_CUSTOM_SECURITY_SETTINGS */

    /* Both control and data app handles must be invalid */
    sppData->ctrlHandle             = CSR_SCHED_QID_INVALID;

#ifndef CSR_STREAMS_ENABLE
    sppData->dataHandle             = CSR_SCHED_QID_INVALID;
#endif


    sppData->modemStatus            = CSR_BT_MODEM_SEND_CTRL_DTE_DEFAULT;
    sppData->breakSignal            = CSR_BT_DEFAULT_BREAK_SIGNAL;
#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
    sppData->amConnId               = CSR_AM_NO_CONN_ID;
    CsrAmInitReqSend(sppData->myAppHandle);
    sppData->amSppCallBack          = CsrSppAmInitCfm;
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#endif /* !EXCLUDE_CSR_AM_MODULE */

    if (sppData->myAppHandle == CSR_BT_SPP_IFACEQUEUE)
    {
        /* This is the SPP manager */
        sppData->sppInstances = CsrPmemZalloc(sizeof(SppInstancesPool_t));
    }
    else
    {
        /* This is one of the extra instances */
        sppData->sppInstances = NULL;
    }

#ifdef INSTALL_SPP_OUTGOING_CONNECTION
    /* search data will be allocated at the time of starting the search. */
    sppData->sdpSppSearchConData    = NULL;
    sppData->portPar = CsrPmemZalloc(sizeof(*sppData->portPar));
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */

#ifndef EXCLUDE_CSR_BT_SD_MODULE
     /* Tell the SD that it must look for the CSR_BT_SPP_PROFILE_UUID
        service, when it perform a SD_READ_AVAILABLE_SERVICE_REQ    */
    CsrBtSdRegisterAvailableServiceReqSend(CSR_BT_SPP_PROFILE_UUID);
#endif

    /* send a register to CM to allocate the internal server channel number for the client role. */
    CsrBtCmRegisterReqSend(sppData->myAppHandle);
}

void CsrBtSppInit(void **gash);
void CsrBtSppHandler(void **gash);

/* Both the SPP init and handler functions can be reused when instantiating multiple instances of the SPP
   profile. However, new instances will need a new task ID added to synergy service SYNERGY_TASK_ID_T enum,
   which will be used to exclusively identify each SPP intance of the profile. The SynergyTaskInit[] and
   SynergyTaskHandler[] tables will need to be updated with entries for their corresponding init and handle
   functions respectively. */

void CsrBtSppInit(void **gash)
{
    SppInstanceData_t  *sppData;
    CsrBtSppRegisterQidReq *sppPrim;

    CSR_LOG_TEXT_REGISTER(&CsrBtSppLto, "BT_SPP", 0, NULL);

#ifdef CSR_BT_GLOBAL_INSTANCE
    if (CsrSchedTaskQueueGet() == CSR_BT_SPP_IFACEQUEUE)
    {
        *gash = &sppInstanceData;
    }
    else
#endif
    {
        /* allocate and initialise instance data space */
        *gash = (void *) CsrPmemZalloc(sizeof(SppInstanceData_t));
    }
    sppData = (SppInstanceData_t *) *gash;
    sppData->myAppHandle = CsrSchedTaskQueueGet();

    /* init the instance data */
    csrBtSppInitCommon(sppData);

    sppPrim          = (CsrBtSppRegisterQidReq *)CsrPmemAlloc(sizeof(CsrBtSppRegisterQidReq));
    sppPrim->type    = CSR_BT_SPP_REGISTER_QID_REQ;
    sppPrim->phandle = sppData->myAppHandle;
    CsrBtSppMessagePut(CSR_BT_SPP_IFACEQUEUE, sppPrim);
}


#ifdef ENABLE_SHUTDOWN
void releaseMessage(CsrUint16 msg_type, void *msg_data)
{
    switch (msg_type)
    {
        case CSR_BT_CM_PRIM:
        {
            CsrBtCmFreeUpstreamMessageContents(msg_type, msg_data);
            break;
        }
        case CSR_BT_SPP_PRIM:
        {
            CsrBtSppFreeDownstreamMessageContents(msg_type, msg_data);
            break;
        }

        default:
            {
                break;
            }
    }
}

/****************************************************************************
    This function is called by the scheduler to perform a graceful shutdown
    of a scheduler task.
    This function must:
    1)    empty the input message queue and free any allocated memory in the
        messages.
    2)    free any instance data that may be allocated.
****************************************************************************/
void CsrBtSppDeinit(void **gash)
{
    CsrUint16              msg_type=0;
    void                *msg_data=NULL;
    CsrBool              lastMsg;
    SppInstanceData_t   *sppData;
    SppInstancesPool_t  *ptr;
    SppInstancesPool_t  *next;

    sppData = (SppInstanceData_t *) *gash;

    /* continue to poll any message of the input queue */
    lastMsg = FALSE;

    while (!lastMsg)
    {
        if (!CsrMessageQueuePop(&sppData->saveQueue, &msg_type, &msg_data))
        {
            lastMsg = !CsrSchedMessageGet(&msg_type, &msg_data);
        }

        if (!lastMsg)
        {
            releaseMessage(msg_type, msg_data);
            CsrPmemFree (msg_data);
        }
    }

#ifndef EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ
    if(sppData->extendedActivationData)
    {
        CsrPmemFree(sppData->extendedActivationData->serviceRecord);
        CsrPmemFree(sppData->extendedActivationData);
        sppData->extendedActivationData = NULL;
    }
    else
#endif /* !EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */
    {
        CsrPmemFree(sppData->serviceName);
    }

#ifdef INSTALL_SPP_OUTGOING_CONNECTION
    CsrBtUtilSdcRfcDeinit(&(sppData->sdpSppSearchConData));
    CsrPmemFree(sppData->portPar);
    CsrPmemFree(sppData->sdpServiceNameList);
    CsrPmemFree(sppData->serviceHandleList);
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */

    if (sppData->sppInstances)
    {
        ptr = sppData->sppInstances;
        next = NULL;

        while(ptr)
        {
            next = ptr->next;
            CsrPmemFree(ptr);
            ptr = next;
        }
    }
    else
    {
        /* Do nothing */
    }
#ifdef CSR_BT_GLOBAL_INSTANCE
    CsrMemSet(sppData, 0, sizeof(SppInstanceData_t));
#else
    CsrPmemFree(sppData);
#endif
    *gash = NULL;
}
#endif


static void CsrBtSppRaiseException(SppInstanceData_t *sppData, CsrUint16 primType, CsrUint16 eventClass)
{
    CSR_UNUSED(primType);
    switch(eventClass)
    {
        case CSR_BT_SPP_PRIM:
        {
                CsrGeneralException(CsrBtSppLto,
                                    0,
                                    CSR_BT_SPP_PRIM,
                                    primType,
                                    sppData->state,
                                    "Unknown SPP prim or undefined state");
            CsrBtSppFreeDownstreamMessageContents(eventClass, sppData->recvMsgP);
            break;
        }
        case CSR_BT_CM_PRIM:
        {
                CsrGeneralException(CsrBtSppLto,
                                    0,
                                    CSR_BT_CM_PRIM,
                                    primType,
                                    sppData->state,
                                    "Unknown CM prim or undefined state");
            CsrBtCmFreeUpstreamMessageContents(eventClass, sppData->recvMsgP);
            break;
        }
        case CSR_AM_PRIM:
        {
                CsrGeneralException(CsrBtSppLto,
                                    0,
                                    CSR_AM_PRIM,
                                    primType,
                                    sppData->state,
                                    "Unknown AM prim or no callback active");
            break;
        }
        default:
        {
                CsrGeneralException(CsrBtSppLto,
                                    0,
                                    eventClass,
                                    0xFF,
                                    sppData->state,
                                    "Unknown primitive type received in main");
            break;
        }
    }
}


void CsrBtSppHandler(void **gash)
{
    SppInstanceData_t    *sppData;
    CsrUint16            eventClass=0;
    void                *msg=NULL;

    sppData = (SppInstanceData_t *) (*gash);

    if(!sppData->restoreSppFlag)
    {
        CsrSchedMessageGet(&eventClass , &msg);
    }
    else
    {
        if(!CsrMessageQueuePop(&sppData->saveQueue, &eventClass, &msg))
        {
            sppData->restoreSppFlag = FALSE;
            CsrSchedMessageGet(&eventClass , &msg);
        }
    }
    sppData->recvMsgP = msg;

    switch(eventClass)
    {
        case CSR_BT_SPP_PRIM:
            {
                CsrBtSppPrim         *primType;
                /* find the message type */
                primType = (CsrBtSppPrim *)msg;
#ifdef CSR_TARGET_PRODUCT_VM
                CSR_LOG_TEXT_INFO((CsrBtSppLto, 0, "CsrBtSppHandler MESSAGE:CsrBtSppPrim:0x%x", *primType));
#endif
                if ((*primType < CSR_BT_SPP_PRIM_DOWNSTREAM_COUNT) && sppStateHandlers[sppData->state][*primType] != NULL)
                {
                    sppStateHandlers[sppData->state][*primType](sppData);
                }
                else if (*primType != CSR_BT_SPP_HOUSE_CLEANING)
                {  /* State/Event ERROR! */
                    CsrBtSppRaiseException(sppData,*primType,eventClass);
                }
                break;
            }

        case CSR_BT_CM_PRIM:
            {
                CsrPrim *primType;
                /* find the message type */
                primType = (CsrPrim *)msg;
#ifdef CSR_TARGET_PRODUCT_VM
                CSR_LOG_TEXT_INFO((CsrBtSppLto, 0, "CsrBtSppHandler MESSAGE:CsrBtCmPrim:%d", *primType));
#endif

                if(((CsrUint16)(*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST) < CSR_BT_CM_RFC_PRIM_UPSTREAM_COUNT) &&
                   (cmStateHandlers[sppData->state][(CsrUint16) (*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)] != NULL))
                {
                    cmStateHandlers[sppData->state][(CsrUint16) (*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)](sppData);
                }
                else
                {
                    switch(*primType)
                    {
#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
                        case CSR_BT_CM_SCO_RENEGOTIATE_CFM:
                        {
                            CsrBtSppCmScoRenegotiateCfmHandler(sppData);
                            break;
                        }
                        case CSR_BT_CM_SCO_RENEGOTIATE_IND:
                        {
                            if (sppData->state == Connected_s)
                            {
                                CsrBtSppCmScoRenegotiateIndHandler(sppData);
                            }
                            else
                            {
                                ;
                            }
                            break;
                        }
                        case CSR_BT_CM_MAP_SCO_PCM_IND:
                        {
                            CsrBtSppCmMapScoPcmHandler(sppData);
                            break;
                        }
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#endif /* !EXCLUDE_CSR_AM_MODULE */
                        case CSR_BT_CM_SDS_UNREGISTER_CFM:
                        {
                            switch(sppData->state)
                            {
                                case Idle_s:
                                case Connected_s:
                                {
                                    CsrBtSppIdleOrConnectedStateSdsUnregisterCfmHandler(sppData);
                                    break;
                                }
                                case Activated_s:
                                {
                                    CsrBtSppActivatedStateSdsUnregisterCfmHandler(sppData);
                                    break;
                                }
                                case Deactivate_s:
                                {
                                    CsrBtSppDeactivateStateSdsUnregisterCfmHandler(sppData);
                                    break;
                                }
                                default:
                                {
                                    CsrBtSppRaiseException(sppData,*primType,eventClass);
                                    break;
                                }

                            }
                            break;
                        }
                        case CSR_BT_CM_SDS_REGISTER_CFM:
                        {
                            if ((sppData->state == Init_s) || (sppData->state == Idle_s))
                            {
                                CsrBtSppInitStateSdsRegisterCfmHandler(sppData);
                            }
                            else
                            {
                                CsrBtSppRaiseException(sppData,*primType,eventClass);
                            }
                            break;
                        }
                        case CSR_BT_CM_PORTNEG_IND:
                        {
                            if ((sppData->state == Connected_s) || (sppData->state == Activated_s))
                            {
                                CsrBtSppConnectedStateCmPortnegIndHandler(sppData);
                                break;
                            }
                            /*else no break intended!*/
                        }
                        /*Fall-through*/
                        case CSR_BT_CM_SDC_UUID128_SEARCH_IND:
                        case CSR_BT_CM_SDC_ATTRIBUTE_CFM:
                        case CSR_BT_CM_SDC_CLOSE_IND:
                        case CSR_BT_CM_SDC_SEARCH_CFM:
                        case CSR_BT_CM_SDC_SEARCH_IND:
                        case CM_SDC_SERVICE_SEARCH_ATTR_CFM:
                        case CM_SDC_SERVICE_SEARCH_ATTR_IND:
                        {
                            #ifdef INSTALL_SPP_OUTGOING_CONNECTION
                            if ((sppData->state == Idle_s) && (CsrBtUtilRfcConVerifyCmMsg(sppData->recvMsgP)))
                            {
                                CsrBtUtilRfcConCmMsgHandler(sppData, sppData->sdpSppSearchConData, sppData->recvMsgP);
                            }
                            else
                            {
                                CsrBtSppRaiseException(sppData,*primType,eventClass);
                            }
                            #endif /* INSTALL_SPP_OUTGOING_CONNECTION */
                            break;
                        }
                        case CSR_BT_CM_CONNECT_CFM:
                        case CSR_BT_CM_SDC_RELEASE_RESOURCES_CFM:
                        {
                            #ifdef INSTALL_SPP_OUTGOING_CONNECTION
                            if (CsrBtUtilRfcConVerifyCmMsg(sppData->recvMsgP))
                            {
                                CsrBtUtilRfcConCmMsgHandler(sppData, sppData->sdpSppSearchConData, sppData->recvMsgP);
                            }
                            #endif /* INSTALL_SPP_OUTGOING_CONNECTION */
                            break;
                        }
                        default:
                        {
                            CsrBtSppRaiseException(sppData,*primType,eventClass);
                            break;
                        }
                    }
                }
                break;
            }

        case CSR_SCHED_PRIM:
            {
                /* Environment cleanup */
                CsrEnvPrim *primType;
                primType = (CsrEnvPrim*)msg;
                if(*primType == CSR_CLEANUP_IND)
                {
                    CsrBtSppEnvironmentCleanupHandler(sppData);
                }
                break;
            }
#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
        case CSR_AM_PRIM:
            {
                if (sppData->amSppCallBack != NULL)
                {
                    sppData->amSppCallBack(sppData);
                }
                else
                {
                    CsrAmPrim *primType = (CsrAmPrim *)msg;
                    CsrBtSppRaiseException(sppData,*primType,eventClass);
                }
                break;
            }
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#endif /* !EXCLUDE_CSR_AM_MODULE */
        default:
            {  /* State/Event ERROR! */
                CsrBtSppRaiseException(sppData,0xFFFF,eventClass);
                break;
            }
    }

    /* free the received message. if the ptr is NULL then CsrSched just ignores */
    SynergyMessageFree(eventClass, sppData->recvMsgP);
    sppData->recvMsgP = NULL;
}
#endif /* !EXCLUDE_CSR_BT_SPP_MODULE */
