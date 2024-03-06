/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_TMAS_SERVER_COMMON_H
#define GATT_TMAS_SERVER_COMMON_H

#include "gatt_tmas_server_private.h"
#include "gatt_tmas_server_debug.h"
#include "csr_bt_gatt_lib.h"
#include "csr_pmem.h"

/* TMAS role characteristic size */
#define GATT_TMAS_SERVER_ROLE_SIZE (2)

/* TMAS Server invalid cid index value */
#define GATT_TMAS_SERVER_INVALID_CID_INDEX  (0xFF)


/**************************************************************************
NAME
    gattTmasServerSendAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
void gattTmasServerSendAccessRsp(CsrBtGattId task,
                                 connection_id_t cid,
                                 uint16 handle,
                                 uint16 result,
                                 uint16 sizeValue,
                                 uint8 *const value);

/**************************************************************************
NAME
    gattTmasServerSendAccessErrorRsp

DESCRIPTION
    Send an access response to the GATT Manager library with an error status.
*/

#define gattTmasServerSendAccessErrorRsp(task, cid, handle, error) \
    gattTmasServerSendAccessRsp( \
            task, \
            cid, \
            handle, \
            error, \
            0, \
            NULL \
            )

#endif /* GATT_TMAS_SERVER_COMMON_H */
