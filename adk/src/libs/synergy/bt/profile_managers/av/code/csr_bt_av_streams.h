#ifndef CSR_BT_AV_STREAMS_H__
#define CSR_BT_AV_STREAMS_H__
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

#include "csr_streams.h"
#include "csr_bt_av_main.h"

void CsrBtAvMessageMoreSpaceHandler(av_instData_t *instData);
void CsrBtAvMessageMoreDataHandler(av_instData_t *instData);
void CsrBtAvStreamsRegister(av_instData_t *instData, CsrBtConnId btConnId);

#endif /* CSR_STREAMS_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* !CSR_BT_AV_STREAMS_H__ */
