/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

#ifndef GATT_GMAS_SERVER_COMMON_H
#define GATT_GMAS_SERVER_COMMON_H

#include "gatt_gmas_server_private.h"
#include "gatt_gmas_server_debug.h"
#include "csr_bt_gatt_lib.h"
#include "csr_pmem.h"

/* GMAS characteristic size */
#define GATT_GMAS_SERVER_CHAR_SIZE (1)

/* GMAS Server invalid cid index value */
#define GATT_GMAS_SERVER_INVALID_CID_INDEX  (0xFF)


/**************************************************************************
NAME
    gattGmasServerSendAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
void gattGmasServerSendAccessRsp(CsrBtGattId task,
                                 connection_id_t cid,
                                 uint16 handle,
                                 uint16 result,
                                 uint16 sizeValue,
                                 uint8 *const value);

/**************************************************************************
NAME
    gattGmasServerSendAccessErrorRsp

DESCRIPTION
    Send an access response to the GATT Manager library with an error status.
*/

#define gattGmasServerSendAccessErrorRsp(task, cid, handle, error) \
    gattGmasServerSendAccessRsp( \
            task, \
            cid, \
            handle, \
            error, \
            0, \
            NULL \
            )

#endif /* GATT_GMAS_SERVER_COMMON_H */
