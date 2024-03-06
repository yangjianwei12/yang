#ifndef CSR_BT_SPP_SEF_H__
#define CSR_BT_SPP_SEF_H__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_SPP_MODULE

#include "csr_types.h"
#include "csr_bt_result.h"
#include "csr_bt_profiles.h"
#include "rfcomm_prim.h"
#include "bluetooth.h"
#include "hci_prim.h"
#include "csr_sched.h"
#include "csr_message_queue.h"
#include "csr_bt_spp_main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define Spp handlers */
void CsrBtSppEnvironmentCleanupHandler(SppInstanceData_t *instData);
void CsrBtSppIdleStateSppActivateReqHandler(SppInstanceData_t *instData);
void CsrBtSppIdleStateSppDeactivateReqHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateSppDisconnectReqHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateSppDeactivateReqHandler(SppInstanceData_t *instData);
void CsrBtSppActivateStateSppDeactivateReqHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateSppPortnegResHandler(SppInstanceData_t *instData);
void CsrBtSppSaveMessage(SppInstanceData_t *instData);
void CsrBtSppConnectedStateSppAudioReqHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateSppAcceptAudioReqHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateSppAudioRenegotiateReqHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateSppCancelAcceptAudioReqHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateSppAudioReleaseReqHandler(SppInstanceData_t *instData);
void CsrBtSppIdleStateSppExtendedConnectReqHandler(SppInstanceData_t *instData);
void CsrBtSppActivateStateSppExtendedConnectReqHandler(SppInstanceData_t *instData);
void CsrBtSppIdleStateSppExtendedUuidConnectReqHandler(SppInstanceData_t *instData);
void CsrBtSppActivateStateSppExtendedUuidConnectReqHandler(SppInstanceData_t *instData);
void CsrBtSppAllStateSppGetInstancesQIDReqHandler(SppInstanceData_t *instData);
void CsrBtSppAllStateSppRegisterQIDReqHandler(SppInstanceData_t *instData);
void CsrBtSppMapScoPcmResHandler(SppInstanceData_t * instData);
void CsrBtSppIdleStateCmCancelAcceptConnectCfmHandler(SppInstanceData_t *instData);
void CsrBtSppSecurityInReqHandler(SppInstanceData_t *instData);
void CsrBtSppSecurityOutReqHandler(SppInstanceData_t *instData);
void CsrBtSppSdpDeinitReq(SppInstanceData_t *instData);

#ifndef CSR_STREAMS_ENABLE
void CsrBtSppNotConnectedSppDataReqHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateSppDataReqHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateCmDataCfmHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateCmDataIndHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateSppDataResHandler(SppInstanceData_t *instData);
void CsrBtSppAnyStateRegisterDataPathHandleReqHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateSppDataPathStatusReqHandler(SppInstanceData_t *instData);
#else /* CSR_STREAMS_ENABLE */
#define CsrBtSppNotConnectedSppDataReqHandler   NULL
#define CsrBtSppConnectedStateSppDataReqHandler NULL
#define CsrBtSppConnectedStateCmDataCfmHandler  NULL
#define CsrBtSppConnectedStateCmDataIndHandler  NULL
#define CsrBtSppAnyStateRegisterDataPathHandleReqHandler  NULL
#define CsrBtSppConnectedStateSppDataPathStatusReqHandler NULL
#define CsrBtSppConnectedStateSppDataResHandler NULL
#endif /* !CSR_STREAMS_ENABLE */

#ifdef INSTALL_SPP_MODEM_STATUS_COMMAND
void CsrBtSppConnectedStateSppControlReqHandler(SppInstanceData_t *instData);
#else /* !INSTALL_SPP_MODEM_STATUS_COMMAND */
#define CsrBtSppConnectedStateSppControlReqHandler NULL
#endif /* INSTALL_SPP_MODEM_STATUS_COMMAND */

#ifdef INSTALL_SPP_OUTGOING_CONNECTION
void CsrBtSppIdleStateSppConnectReqHandler(SppInstanceData_t *instData);
void CsrBtSppActivateStateSppConnectReqHandler(SppInstanceData_t *instData);
void CsrBtSppIdleStateSppServiceNameResHandler(SppInstanceData_t *instData);
void CsrBtSppInitStateSppCancelConnectReqHandler(SppInstanceData_t * instData);
void CsrBtSppXStateSppCancelConnectReqHandler(SppInstanceData_t * instData);
void CsrBtSppConnectedStateSppCancelConnectReqHandler(SppInstanceData_t * instData);
void CsrBtSppDeactivateStateSppCancelConnectReqHandler(SppInstanceData_t * instData);
#else /* !INSTALL_SPP_OUTGOING_CONNECTION */
#define CsrBtSppIdleStateSppConnectReqHandler     NULL
#define CsrBtSppActivateStateSppConnectReqHandler NULL
#define CsrBtSppIdleStateSppServiceNameResHandler NULL
#define CsrBtSppInitStateSppCancelConnectReqHandler NULL
#define CsrBtSppXStateSppCancelConnectReqHandler    NULL
#define CsrBtSppConnectedStateSppCancelConnectReqHandler  NULL
#define CsrBtSppDeactivateStateSppCancelConnectReqHandler NULL
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */

#ifdef INSTALL_SPP_CUSTOM_SECURITY_SETTINGS
void CsrBtSppSecurityInReqHandler(SppInstanceData_t *instData);
void CsrBtSppSecurityOutReqHandler(SppInstanceData_t *instData);
#else
#define CsrBtSppSecurityInReqHandler        NULL
#define CsrBtSppSecurityOutReqHandler       NULL
#endif

#ifdef INSTALL_SPP_REMOTE_PORT_NEGOTIATION
void CsrBtSppXStatePortnegReqHandler(SppInstanceData_t *instData);
void CsrBtCppConnectedStateCmPortnegCfmHandler(SppInstanceData_t * instData);
#else /* !INSTALL_SPP_REMOTE_PORT_NEGOTIATION */
#define CsrBtSppXStatePortnegReqHandler NULL
#define CsrBtCppConnectedStateCmPortnegCfmHandler NULL
#endif /* INSTALL_SPP_REMOTE_PORT_NEGOTIATION */

#ifndef EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ
#define CsrBtSppInitStateSppExtendedActivateReqHandler CsrBtSppSaveMessage
void CsrBtSppIdleStateSppExtendedActivateReqHandler(SppInstanceData_t *instData);
void CsrBtSppDummyStateSppActivateReqHandler(SppInstanceData_t *instData);
#else /* EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */
#define CsrBtSppInitStateSppExtendedActivateReqHandler NULL
#define CsrBtSppIdleStateSppExtendedActivateReqHandler NULL
#define CsrBtSppDummyStateSppActivateReqHandler NULL
#endif /* !EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */

/* Define cm handlers */
void CsrBtSppInitStateCmRegisterCfmHandler(SppInstanceData_t *instData);
/*void SppIdleStateCmConnectCfmHandler(SppInstanceData_t *instData);*/
void CsrBtSppIdleStateCmDisconnectIndHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateCmControlIndHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateCmDisconnectIndHandler(SppInstanceData_t *instData);
void CsrBtSppActivateStateCmCancelAcceptConnectCfmHandler(SppInstanceData_t *instData);
void CsrBtSppDeactivateStateCmCancelAcceptConnectCfmHandler(SppInstanceData_t *instData);
void CsrBtSppDeactivateStateCmDisconnectIndHandler(SppInstanceData_t *instData);
void CsrBtSppDeOrActivatedStateCmConnectAcceptCfmHandler(SppInstanceData_t *instData);
void CsrBtSppNotConnectedCmDataIndHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateCmPortnegIndHandler(SppInstanceData_t *instData);
void CsrBtSppIdleOrConnectedStateSdsUnregisterCfmHandler(SppInstanceData_t *instData);

void CsrBtSppConnectedStateCmScoConnectCfmHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateCmScoDisconnectIndHandler(SppInstanceData_t *instData);
void CsrBtSppConnectedStateCmScoAcceptConnectCfmHandler(SppInstanceData_t *instData);
void CsrBtSppDeactivateStateCmScoXHandler(SppInstanceData_t *instData);
void CsrBtSppCmScoRenegotiateCfmHandler(SppInstanceData_t *instData);
void CsrBtSppCmScoRenegotiateIndHandler(SppInstanceData_t *instData);
void CsrBtSppCmMapScoPcmHandler(SppInstanceData_t * instData);

/* Define sdp handlers */
void CsrBtSppInitStateSdsRegisterCfmHandler(SppInstanceData_t *instData);
void CsrBtSppDeactivateStateSdsUnregisterCfmHandler(SppInstanceData_t *instData);
void CsrBtSppActivatedStateSdsUnregisterCfmHandler(SppInstanceData_t *instData);

/* Due to race conditions (during disconnect) just call empty function */
void CsrBtSppIgnoreMessageHandler(SppInstanceData_t *instData);

#ifdef INSTALL_SPP_CUSTOM_SECURITY_SETTINGS
/* Upstream helpers */
void CsrBtSppSecurityInCfmSend(CsrSchedQid appHandle, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);
void CsrBtSppSecurityOutCfmSend(CsrSchedQid appHandle, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);
#endif

/* Prototypes from spp_free_down.c */
void CsrBtSppFreeDownstreamMessageContents(CsrUint16 eventClass, void * message);


#ifdef __cplusplus
}
#endif

#endif
#endif
