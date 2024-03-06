/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "gatt_gmas_server_common.h"


void gattGmasServerSendAccessRsp(CsrBtGattId task,
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