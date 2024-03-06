#ifndef CSR_BT_HF_STREAMS_H__
#define CSR_BT_HF_STREAMS_H__
/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CSR_STREAMS_ENABLE

#include "csr_bt_hf_main.h"
#include "csr_streams.h"

void CsrBtHfMessageMoreSpaceHandler(HfMainInstanceData_t *instData);
void CsrBtHfMessageMoreDataHandler(HfMainInstanceData_t *instData);
void CsrBtHfStreamsRegister(HfMainInstanceData_t *instData, CsrBtConnId btConnId);

#endif /* CSR_STREAMS_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* !CSR_BT_HF_STREAMS_H__ */
