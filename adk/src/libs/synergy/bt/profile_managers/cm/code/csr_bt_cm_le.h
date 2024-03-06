#ifndef _CSR_BT_CM_LE_H__
#define _CSR_BT_CM_LE_H__
/******************************************************************************
 Copyright (c) 2010-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_prim.h"
#include "dm_prim.h"
#include "dmlib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_cm_util.h"
#include "csr_bt_cm_callback_q.h"
#include "csr_bt_cm_events_handler.h"

#ifdef CSR_BT_LE_ENABLE
#ifdef __cplusplus
extern "C" {
#endif

void CsrBtCmLeGapRegisterReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeScanReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeAdvertiseReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeWhitelistSetReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeConnparamReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeConnparamUpdateReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeAcceptConnparamUpdateResHandler(cmInstanceData_t *cmData);
void CsrBtCmLeReceiverTestReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeTransmitterTestReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeTestEndReqHandler(cmInstanceData_t *cmData);
void CmLeEnhancedReceiverTestReqHandler(cmInstanceData_t *cmData);
void CmLeEnhancedTransmitterTestReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeReportIndHandler(cmInstanceData_t *cmInst,
                               DM_HCI_ULP_ADVERTISING_REPORT_IND_T *report);
void CsrBtCmLeConnectionUpdateCmpIndHandler(cmInstanceData_t *cmInst,
                                            DM_HCI_ULP_CONNECTION_UPDATE_COMPLETE_IND_T* dmPrim);
void CsrBtCmLeAclOpenedIndHandler(cmInstanceData_t *cmData);
void CsrBtCmLeAcceptConnparamUpdateIndHandler(cmInstanceData_t *cmData);
void CsrBtCmLeAclClosedIndHandler(cmInstanceData_t *cmData);
void CsrBtCmLePhysicalLinkStatusReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeDeinit(cmInstanceData_t *cmData);
void CsrBtCmLeLockSmQueueHandler(cmInstanceData_t *cmData);
void CsrBtCmLeUnLockSmQueueHandler(cmInstanceData_t *cmData);
void CsrBtCmLeGetControllerInfoReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeReadRemoteUsedFeaturesReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeReadLocalSupportedFeaturesReqHandler(cmInstanceData_t *cmData);
#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
void CsrBtCmLeReadResolvingListSizeReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeSetPrivacyModeReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeReadLocalIrkReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeReadLocalIrkCompleteHandler(cmInstanceData_t *cmData);
#else /* !CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */
#define CsrBtCmLeReadResolvingListSizeReqHandler NULL
#define CsrBtCmLeSetPrivacyModeReqHandler        NULL
#define CsrBtCmLeReadLocalIrkReqHandler          NULL
#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */
void CsrBtCmLeSetOwnAddressTypeReqHandler(cmInstanceData_t *cmInst);
void CsrBtCmLeSetPvtAddrTimeoutReqHandler(cmInstanceData_t *cmInst);
void CsrBtCmLeSetStaticAddressReqHandler(cmInstanceData_t *cmInst);
void CsrBtCmLeReadRandomAddressCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmLeReadRandomAddressReqHandler(cmInstanceData_t *cmInst);
void CsrBtCmLeConfigureRandomAddress(cmInstanceData_t *cmData);
void CsrBtCmLeSirkOperationCompleteHandler(cmInstanceData_t *cmData);
#if !defined(EXCLUDE_DM_SM_SIRK_OPERATION_REQ)
void CsrBtCmLeSirkOperationReqHandler(cmInstanceData_t *cmInst);
#else
#define CsrBtCmLeSirkOperationReqHandler                    NULL
#endif
#if defined (DM_ULP_SET_DATA_RELATED_ADDRESS_CHANGES_REQ)
void CsrBtCmLeSetDataRelatedAddressChangesReqHandler(cmInstanceData_t *cmInst);
#else
#define CsrBtCmLeSetDataRelatedAddressChangesReqHandler     NULL
#endif
void CsrBtCmLeSetDataRelatedAddressChangesCompleteHandler(cmInstanceData_t *cmData);

leConnVar *CsrBtCmLeFindConn(cmInstanceData_t *cmData, const CsrBtTypedAddr *addr);
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_SUBRATE_CHANGE
void CsrBtCmLeSetDefaultSubrateReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeSetDefaultSubrateCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmLeSubrateChangeReqHandler(cmInstanceData_t *cmData);
void CsrBtCmLeSubrateChangeCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmLeSubrateChangeIndHandler(cmInstanceData_t *cmData);
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_SUBRATE_CHANGE */

void CmLeAddDeviceToWhiteListReqHandler(cmInstanceData_t *cmData);
void CmLeRemoveDeviceFromWhiteListReqHandler(cmInstanceData_t *cmData);

#ifdef __cplusplus
}
#endif

#else /* !CSR_BT_LE_ENABLE */
#define CsrBtCmLeScanReqHandler                             NULL
#define CsrBtCmLeAdvertiseReqHandler                        NULL
#define CsrBtCmLeWhitelistSetReqHandler                     NULL
#define CsrBtCmLeConnparamReqHandler                        NULL
#define CsrBtCmLeConnparamUpdateReqHandler                  NULL
#define CsrBtCmLeAcceptConnparamUpdateResHandler            NULL
#define CsrBtCmLeReceiverTestReqHandler                     NULL
#define CsrBtCmLeTransmitterTestReqHandler                  NULL
#define CsrBtCmLeTestEndReqHandler                          NULL
#define CsrBtCmLeUnLockSmQueueHandler                       NULL
#define CsrBtCmLeGetControllerInfoReqHandler                NULL
#define CsrBtCmLeReadRemoteUsedFeaturesReqHandler           NULL
#define CsrBtCmLeReadLocalSupportedFeaturesReqHandler       NULL
#define CsrBtCmLeReadResolvingListSizeReqHandler            NULL
#define CsrBtCmLeSetPrivacyModeReqHandler                   NULL
#define CsrBtCmLeSetOwnAddressTypeReqHandler                NULL
#define CsrBtCmLeSetStaticAddressReqHandler                 NULL
#define CsrBtCmLeSetPvtAddrTimeoutReqHandler                NULL
#define CsrBtCmLeReadLocalIrkReqHandler                     NULL
#define CsrBtCmLeLockSmQueueHandler                         NULL
#define CsrBtCmLePhysicalLinkStatusReqHandler               NULL
#define CsrBtCmLeReadRandomAddressReqHandler                NULL
#define CsrBtCmLeSirkOperationReqHandler                    NULL
#define CsrBtCmLeSetDataRelatedAddressChangesReqHandler     NULL
#define CmLeAddDeviceToWhiteListReqHandler                  NULL
#define CmLeRemoveDeviceFromWhiteListReqHandler             NULL
#endif /* CSR_BT_LE_ENABLE */
#endif /* _CSR_BT_CM_LE_H__ */
