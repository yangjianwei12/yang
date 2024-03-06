/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_TELEPHONE_BEARER_SERVER_COMMON_H
#define GATT_TELEPHONE_BEARER_SERVER_COMMON_H

#include <csrtypes.h>
#include <message.h>

#include <gatt_manager.h>

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
        Task task,
        uint16 cid,
        uint16 handle,
        uint16 result,
        uint16 size_value,
        const uint8 *value
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
        Task        task,
        uint16      cid,
        uint16      result,
        uint16      handle
        );

/***************************************************************************
NAME
    tbsServerSendCharacteristicChangedNotification

DESCRIPTION
    Send a notification to the client to notify that the value of
    a characteristic is changed.
*/
void tbsServerSendCharacteristicChangedNotification(
        Task  task,
        uint16 cid,
        uint16 handle,
        uint16 size_value,
        const uint8 *value
        );

/***************************************************************************
NAME
    tbsHandleReadClientConfigAccess

DESCRIPTION
    Deals with access of the X_CLIENT_CONFIG handles in case of reading.
*/
void tbsHandleReadClientConfigAccess(
        Task task,
        uint16 cid,
        uint16 handle,
        const uint16 client_config
        );

/***************************************************************************
NAME
    tbsHandleWriteClientConfigAccess

DESCRIPTION
    Deals with access of a client config handle to be written and indicated
    to the application.
*/

bool tbsHandleWriteClientConfigAccess(
        Task task,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
        uint16 *client_config
        );
        
#endif /* GATT_TELEPHONE_BEARER_SERVER_COMMON_H */
