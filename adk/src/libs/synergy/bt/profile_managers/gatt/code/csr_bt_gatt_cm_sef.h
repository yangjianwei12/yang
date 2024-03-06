#ifndef CSR_BT_GATT_CM_SEF_H__
#define CSR_BT_GATT_CM_SEF_H__
/******************************************************************************
 Copyright (c) 2010-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_gatt_main.h"

#ifdef __cplusplus
extern "C" {
#endif

void CsrBtGattDispatchCm(GattMainInst *inst);

typedef struct
{
    CsrBtTypedAddr  address;
    CsrBool         encrypted;
} CsrBtGattEncryptIds;


#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_GATT_CM_SEF_H__ */

