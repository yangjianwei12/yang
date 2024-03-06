#ifndef CSR_BT_GATT_HANDOVER_H__
#define CSR_BT_GATT_HANDOVER_H__
/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_bt_gatt_prim.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*CsrBtGattCallbackFunctionPointer)(CsrBtTypedAddr *addr, CsrBtConnId btConnId);

void CsrBtGattCallBackRegister(CsrBtGattCallbackFunctionPointer cb);

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_GATT_HANDOVER_H__ */
