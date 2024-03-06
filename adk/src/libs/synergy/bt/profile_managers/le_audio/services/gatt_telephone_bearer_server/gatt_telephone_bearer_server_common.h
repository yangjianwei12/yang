/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef GATT_TELEPHONE_BEARER_SERVER_COMMON_H
#define GATT_TELEPHONE_BEARER_SERVER_COMMON_H

#include "csr_types.h"
#include "csr_sched.h"
#include "csr_list.h"
#include "gatt_telephone_bearer_server_private.h"

#define CLIENT_CONFIG_NOTIFY                  (0x01)
#define CLIENT_CONFIG_INDICATE                (0x02)

/* Required octets for values sent to Client Configuration Descriptor */
#define CLIENT_CONFIG_VALUE_SIZE           (2)

/**************************************************************************
NAME
    sendTbsServerAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
void sendTbsServerAccessRsp(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 result,
        uint16 sizeValue,
        uint8 const *value
        );

/**************************************************************************
NAME
    sendTbsServerAccessErrorRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/

#define sendTbsServerAccessErrorRsp(task, cid, handle, error) \
    sendTbsServerAccessRsp( \
            task, \
            cid, \
            handle, \
            error, \
            0, \
            NULL \
            )

/**************************************************************************
NAME
    gattTelephoneBearerServerWriteGenericResponse

DESCRIPTION
    Send a generic response to the client after a writing.
*/
void gattTelephoneBearerServerWriteGenericResponse(
        CsrBtGattId       task,
        connection_id_t   cid,
        uint16            result,
        uint16            handle
        );

/***************************************************************************
NAME
    tbsServerSendCharacteristicChangedNotification

DESCRIPTION
    Send a notification to the client to notify that the value of
    a characteristic is changed.
*/
void tbsServerSendCharacteristicChangedNotification(
        CsrBtGattId     task,
        connection_id_t cid,
        uint16          handle,
        uint16          sizeValue,
        uint8 const *value
        );

/***************************************************************************
NAME
    tbsHandleReadClientConfigAccess

DESCRIPTION
    Deals with access of the X_CLIENT_CONFIG handles in case of reading.
*/
void tbsHandleReadClientConfigAccess(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 const clientConfig
        );

/***************************************************************************
NAME
    tbsHandleWriteClientConfigAccess

DESCRIPTION
    Deals with access of a client config handle to be written and indicated
    to the application.
*/

bool tbsHandleWriteClientConfigAccess(
        CsrBtGattId task,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd,
        uint16 *clientConfig
        );
        
void TbsMessageSendLater(AppTask task, void *msg);

void *MemRealloc(void* ptr, uint16 size);

#endif /* GATT_TELEPHONE_BEARER_SERVER_COMMON_H */
