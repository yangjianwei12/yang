/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "gatt_tmas_server_common.h"
#include "gatt_tmas_server_debug.h"


void gattTmasServerSendAccessRsp(CsrBtGattId task,
                                 connection_id_t cid,
                                 uint16 handle,
                                 uint16 result,
                                 uint16 sizeValue,
                                 uint8 *const value)
{
    CsrBtGattDbReadAccessResSend(task,
                                 cid,
                                 handle,
                                 result,
                                 sizeValue,
                                 value);
}