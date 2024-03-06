#ifndef CSR_BT_HF_MAIN_SEF_H__
#define CSR_BT_HF_MAIN_SEF_H__
/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/


#include "csr_synergy.h"

#include "csr_sched.h"
#include "csr_pmem.h"
#include "bluetooth.h"
#include "hci_prim.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_util.h"
#include "csr_log_text_2.h"
#include "csr_bt_sdc_support.h"
#include "csr_bt_hf_main.h"
#include "csr_bt_hf_util.h"
#include "csr_bt_hf_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* General functions */
void CsrBtHfCleanup_queue(HfMainInstanceData_t* instData);
void CsrBtHfFreeInactiveLinkData(HfInstanceData_t *linkData, CsrUint8 count);

/* Define CM handlers */
void CsrBtHfNullStateCmRegisterCfmHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateCmSdsRegisterCfmHandler(HfMainInstanceData_t * instData);

/* Activate State, upstream */
void CsrBtHfActivatedStateCmCancelAcceptConnectCfmHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateCmConnectAcceptCfmHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateCmDisconnectIndHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateCmScoConnectCfmHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateCmScoDisconnectIndHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateCmScoAcceptConnectCfmHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateCmDataIndHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateCmDataCfmHandler(HfMainInstanceData_t * instData);
void CsrBtHfXStateIgnoreCmControlIndHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateCmPortnegIndHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateCmSdsUnregisterCfmHandler(HfMainInstanceData_t * instData);
void HfActivatedStateCmRfcConnectAcceptIndHandler(HfMainInstanceData_t *instData);

/* Deactivate State, upstream */
void CsrBtHfDeactivateStateCmCancelAcceptConnectCfmHandler(HfMainInstanceData_t * instData);
void CsrBtHfDeactivateStateCmConnectAcceptCfmHandler(HfMainInstanceData_t * instData);
void CsrBtHfDeactivateStateCmDisconnectIndHandler(HfMainInstanceData_t * instData);
void CsrBtHfDeactivateStateCmScoConnectCfmHandler(HfMainInstanceData_t * instData);
void CsrBtHfDeactivateStateCmScoDisconnectIndHandler(HfMainInstanceData_t * instData);
void CsrBtHfDeactivateStateCmScoAcceptConnectCfm(HfMainInstanceData_t * instData);
void CsrBtHfDeactivateStateCmDataIndHandler(HfMainInstanceData_t * instData);
void CsrBtHfDeactivateStateCmDataCfmHandler(HfMainInstanceData_t * instData);
void CsrBtHfDeactivateStateCmControlIndHandler(HfMainInstanceData_t * instData);
void CsrBtHfDeactivateStateCmSdsUnregisterCfmHandler(HfMainInstanceData_t * instData);
void CsrBtHfDeactivateStateCmSdsRegisterCfmHandler(HfMainInstanceData_t * instData);
void HfDeactivateStateCmRfcConnectAcceptIndHandler(HfMainInstanceData_t *instData);

/* Define handlers for HF prims */
#ifdef CSR_BT_INSTALL_HF_CONFIG_AUDIO
void CsrBtHfXStateConfigAudioReqHandler(HfMainInstanceData_t * instData);
#endif
void CsrBtHfNullStateDeactivateReqHandler(HfMainInstanceData_t * instData);
void CsrBtHfXStateActivateReqHandler(HfMainInstanceData_t * instData);

#ifdef INSTALL_HF_CUSTOM_SECURITY_SETTINGS
void HsHfSecurityInReqHandler(HfMainInstanceData_t * instData);
void HsHfSecurityOutReqHandler(HfMainInstanceData_t * instData);
#else
#define HsHfSecurityInReqHandler            NULL
#define HsHfSecurityOutReqHandler           NULL
#endif /* INSTALL_HF_CUSTOM_SECURITY_SETTINGS */

void CsrBtHfActivatedStateHfDeactivateReqHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateHfCancelReqHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateHfDisconnectReqHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateHfServiceConnectReq(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateAudioDisconnectReqHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateAudioReqHandler(HfMainInstanceData_t * instData);
void CsrBtHfMainXStateMapScoPcmIndHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateMapScoPcmResHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateAnswerReqHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateRejectReqHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateSpeakerGainStatusReqHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateMicGainStatusReqHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateAtCmdReqHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedStateChldReqHandler(HfMainInstanceData_t * instData);
void CsrBtHfXStateHfCopsReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateHfGetAllStatusReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateHfCallListReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateHfSubscriberReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateHfSetExtErrorReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateHfSetClipHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateHfSetCcwaHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateHfSetNrecHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateHfSetBvraHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateDTMFReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfHsDeactivateHandler(HfMainInstanceData_t * instData);
void CsrBtHfActivatedSetStatusIndUpdateReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfActivatedBtInputReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfActivatedDialRequest(HfMainInstanceData_t *instData);
void CsrBtHfMainIgnoreMessage(HfMainInstanceData_t *instData);
void CsrBtHfXStateHfCommonAtCmdReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfCommonAtCmdPrimReqHandler(HfMainInstanceData_t *instData, CsrBtHfPrim *primType);
void CsrBtHfXStateSetDeregisterTimeReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateIndicatorActivationReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateHfUpdateCodecSupportReqHandler(HfMainInstanceData_t *instData);
#ifdef HF_ENABLE_OPTIONAL_CODEC_SUPPORT
void HfXStateHfUpdateOptionalCodecReqHandler(HfMainInstanceData_t *instData);
#else /* !HF_ENABLE_OPTIONAL_CODEC_SUPPORT */
#define HfXStateHfUpdateOptionalCodecReqHandler NULL
#endif

void CsrBtHfXStateHfSetHfIndicatorValueHandler(HfMainInstanceData_t *instData);
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
void CsrBtHfXStateHfUpdateQceSupportReqHandler(HfMainInstanceData_t *instData);
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */

/* Prototypes from hf_free_down.c */
void CsrBtHfFreeDownstreamMessageContents(CsrUint16 eventClass, void * message);

#ifdef __cplusplus
}
#endif

#endif
