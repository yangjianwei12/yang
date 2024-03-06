#ifndef CSR_BT_CM_DM_SC_SSP_HANDLER_H__
#define CSR_BT_CM_DM_SC_SSP_HANDLER_H__
/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_rfc.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_prim.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void CsrBtCmSmBondingReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmSmBondingScStateHandler(cmInstanceData_t * cmData);
void CsrBtCmDmBondingCfm(cmInstanceData_t *cmData, 
                         BD_ADDR_T *p_bd_addr, CsrUint8 status);
void CsrBtCmSmBondingCancelReqHandler(cmInstanceData_t *cmData);
#ifdef CSR_BT_INSTALL_CM_SC_MODE_CONFIG
void CsrBtCmSmSecModeConfigReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSmSecModeConfigReqHandler NULL
#endif
void cmSmReadDeviceReqHandler(cmInstanceData_t *cmData);

#ifdef EXCLUDE_CSR_BT_SC_MODULE
CsrBool CmSmIsBondEnabled(cmInstanceData_t *cmData,
                          CsrBtDeviceAddr *addr,
                          CsrUint8 addrType,
                          CsrUint8 transportType);
#endif

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1
void CsrBtCmSmIoCapabilityRequestResHandler(cmInstanceData_t *cmData);
void CsrBtCmSmIoCapabilityRequestNegResHandler(cmInstanceData_t *cmData);
#ifdef CSR_BT_INSTALL_CM_OOB
void CsrBtCmSmReadLocalOobDataReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSmReadLocalOobDataReqHandler NULL
#endif
void CsrBtCmSmSendKeypressNotificationReqHandler(cmInstanceData_t *cmData);
#ifdef INSTALL_CM_SM_REPAIR
void CsrBtCmSmRepairResHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSmRepairResHandler NULL
#endif
void CsrBtCmSmUserConfirmationRequestNegResHandler(cmInstanceData_t *cmData);
void CsrBtCmSmUserConfirmationRequestResHandler(cmInstanceData_t *cmData);
void CsrBtCmSmUserPasskeyRequestNegResHandler(cmInstanceData_t *cmData);
void CsrBtCmSmUserPasskeyRequestResHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSmIoCapabilityRequestResHandler NULL
#define CsrBtCmSmIoCapabilityRequestNegResHandler NULL
#define CsrBtCmSmReadLocalOobDataReqHandler NULL
#define CsrBtCmSmSendKeypressNotificationReqHandler NULL
#define CsrBtCmSmRepairResHandler NULL
#define CsrBtCmSmUserConfirmationRequestNegResHandler NULL
#define CsrBtCmSmUserConfirmationRequestResHandler NULL
#define CsrBtCmSmUserPasskeyRequestNegResHandler NULL
#define CsrBtCmSmUserPasskeyRequestResHandler NULL
#endif

#ifdef CSR_BT_LE_ENABLE
void CsrBtCmSmLeSecurityReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSmLeSecurityReqHandler NULL
#endif

#ifdef INSTALL_CM_KEY_REQUEST_INDICATION
void CmSmKeyRequestRspHandler(cmInstanceData_t *cmData);
#else
#define CmSmKeyRequestRspHandler NULL
#endif /* INSTALL_CM_KEY_REQUEST_INDICATION */

#ifdef __cplusplus
}
#endif

#endif
