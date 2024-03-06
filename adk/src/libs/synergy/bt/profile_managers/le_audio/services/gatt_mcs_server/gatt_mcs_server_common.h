/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_MCS_SERVER_COMMON_H
#define GATT_MCS_SERVER_COMMON_H

#include "csr_types.h"
#include "csr_sched.h"
#include "csr_list.h"
#include "gatt_mcs_server_private.h"

#define CLIENT_CONFIG_INDICATE                (0x02)

/* Required octets for values sent to Client Configuration Descriptor */
#define MCS_SERVER_CLIENT_CONFIG_VALUE_SIZE           (2)

/**************************************************************************
NAME
    sendMcsServerAccessRsp

DESCRIPTION
    Send an access response to the GATT  library.
*/
void sendMcsServerAccessRsp(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 result,
        uint16 sizeValue,
        uint8 const *value
        );

/**************************************************************************
NAME
    sendMcsServerAccessErrorRsp

DESCRIPTION
    Send an access response to the GATT  library.
*/

#define sendMcsServerAccessErrorRsp(task, cid, handle, error) \
    sendMcsServerAccessRsp( \
            task, \
            cid, \
            handle, \
            error, \
            0, \
            NULL \
            )

/**************************************************************************
NAME
    gattMcsServerWriteGenericResponse

DESCRIPTION
    Send a generic response to the client after a writing.
*/
void gattMcsServerWriteGenericResponse(
        CsrBtGattId       task,
        connection_id_t   cid,
        uint16            result,
        uint16            handle
        );

/***************************************************************************
NAME
    mcsServerSendCharacteristicChangedNotification

DESCRIPTION
    Send a notification to the client to notify that the value of
    a characteristic is changed.
*/
void mcsServerSendCharacteristicChangedNotification(
        CsrBtGattId     task,
        connection_id_t cid,
        uint16          handle,
        uint16          sizeValue,
        uint8 const *value
        );

/***************************************************************************
NAME
    mcsHandleReadClientConfigAccess

DESCRIPTION
    Deals with access of the X_CLIENT_CONFIG handles in case of reading.
*/
void mcsHandleReadClientConfigAccess(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 const clientConfig
        );

/***************************************************************************
NAME
    mcsHandleWriteClientConfigAccess

DESCRIPTION
    Deals with access of a client config handle to be written and indicated
    to the application.
*/

bool mcsHandleWriteClientConfigAccess(
        CsrBtGattId task,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd,
        uint16 *clientConfig);
        

void* McsMemRealloc(void* ptr, uint16* len, uint16 newLen, uint16 maxLen);

#endif /* GATT_TELEPHONE_BEARER_SERVER_COMMON_H */
