#ifndef CSR_BT_CM_STREAMS_HANDLER_H__
#define CSR_BT_CM_STREAMS_HANDLER_H__
/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CSR_STREAMS_ENABLE
#include "csr_streams.h"
#include "csr_bt_cm_main.h"

#ifdef CSR_BT_LE_ENABLE
/*************************************************************************************
  CsrBtCmMessageMoreDataHandler:
  Handles Message More Data events
************************************************************************************/
void CsrBtCmMessageMoreDataHandler(cmInstanceData_t *cmData);
void CsrBtCmStreamProcessData(cmInstanceData_t *cmData, Source source);
void CmStreamFlushSource(Source src);

#endif /* CSR_BT_LE_ENABLE */

#endif /* CSR_STREAMS_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_CM_STREAMS_HANDLER_H__ */
