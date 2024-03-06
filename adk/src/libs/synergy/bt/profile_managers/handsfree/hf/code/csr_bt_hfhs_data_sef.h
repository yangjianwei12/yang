#ifndef CSR_BT_HFHS_DATA_SEF_H__
#define CSR_BT_HFHS_DATA_SEF_H__
/******************************************************************************
 Copyright (c) 2009-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"
#include "csr_bt_result.h"

void CsrBtHfHsXStateCmDataIndHandler(HfMainInstanceData_t *instData);
void CsrBtHfHsXStateCmDataCfmHandler(HfMainInstanceData_t *instData);
void CsrBtHfHsSendCmDataReq(HfMainInstanceData_t *instData,
                       CsrUint16 payloadLen,
                       CsrUint8 *payload);

#ifdef INSTALL_HF_CUSTOM_SECURITY_SETTINGS
void CsrBtHfSecurityInCfmSend(CsrSchedQid appHandle, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);
void CsrBtHfSecurityOutCfmSend(CsrSchedQid appHandle, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);
#endif 

HfLastAtCmdSent_t FindCurrentCmdFromPayload(CsrUint8 *string);

#ifdef __cplusplus
}
#endif

#endif
