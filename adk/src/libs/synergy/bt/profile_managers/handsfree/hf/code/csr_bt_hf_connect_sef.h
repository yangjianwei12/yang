#ifndef CSR_BT_HF_CONNECT_SEF_H__
#define CSR_BT_HF_CONNECT_SEF_H__
/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_hf_main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define Hf handlers */
void CsrBtHfXStateHfCancelReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateHfDisconnectReqHandler(HfMainInstanceData_t *instData);

/* Define CM handlers */
void HfActivateStateCmRfcConnectAcceptIndHandler(HfMainInstanceData_t *instData);
void CsrBtHfActivateStateCmConnectAcceptCfmHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateCmConnectAcceptCfmHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateCmDisconnectIndHandler(HfMainInstanceData_t *instData);
#ifdef __cplusplus
}
#endif

#endif
