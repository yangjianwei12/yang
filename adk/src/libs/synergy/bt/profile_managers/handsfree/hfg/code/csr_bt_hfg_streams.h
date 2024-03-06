#ifndef CSR_BT_HFG_STREAMS_H__
#define CSR_BT_HFG_STREAMS_H__
/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CSR_STREAMS_ENABLE

#include "csr_bt_hfg_main.h"
#include "csr_streams.h"

void CsrBtHfgMessageMoreSpaceHandler(HfgMainInstance_t *instData);
void CsrBtHfgMessageMoreDataHandler(HfgMainInstance_t *instData);
void CsrBtHfgStreamsRegister(HfgMainInstance_t *instData, CsrBtConnId btConnId);

#endif /* CSR_STREAMS_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* !CSR_BT_HFG_STREAMS_H__ */
