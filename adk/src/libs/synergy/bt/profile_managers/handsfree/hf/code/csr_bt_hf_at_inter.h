#ifndef CSR_BT_HF_AT_INTER_H__
#define CSR_BT_HF_AT_INTER_H__
/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_hf_main.h"

#ifdef __cplusplus
extern "C" {
#endif

void CsrBtHfHandleMultipleAtCommand(HfMainInstanceData_t *instData, CsrUint32 theConnectionId);
CsrBool HfProcessSavedAtMessages(HfMainInstanceData_t *instData, HfHsData_t *data, CsrBool *startAtResponseTimer);
#ifdef __cplusplus
}
#endif

#endif
