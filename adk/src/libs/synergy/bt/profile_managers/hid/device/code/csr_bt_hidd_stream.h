#ifndef CSR_BT_HIDD_STREAMS_H__
#define CSR_BT_HIDD_STREAMS_H__
/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
******************************************************************************/

#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CSR_STREAMS_ENABLE

#include "csr_streams.h"
#include "csr_bt_hidd_main.h"

void CsrBtHiddMessageMoreSpaceHandler(HiddInstanceDataType *instData);
void CsrBtHiddMessageMoreDataHandler(HiddInstanceDataType *instData);
void CsrBtHiddStreamsRegister(HiddInstanceDataType *instData, CsrBtConnId btConnId);

#endif /* CSR_STREAMS_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* !CSR_BT_HIDD_STREAMS_H__ */
