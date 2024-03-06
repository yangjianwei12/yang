/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_MICS_SERVER_COMMON_H
#define GATT_MICS_SERVER_COMMON_H

#include "csr_types.h"
#include "csr_sched.h"
#include "csr_list.h"
#include "gatt_mics_server_private.h"

#define CLIENT_CONFIG_INDICATE                (0x02)

/* Required octets for values sent to Client Configuration Descriptor */
#define MICS_SERVER_CLIENT_CONFIG_VALUE_SIZE           (2)

/* MICS Server invalid cid index value */
#define GATT_MICS_SERVER_INVALID_CID_INDEX  (0xFF)

/**************************************************************************
NAME
    sendMicsServerAccessRsp

DESCRIPTION
    Send an access response to the GATT  library.
*/
void sendMicsServerAccessRsp(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 result,
        uint16 sizeValue,
        uint8 const *value
        );

/**************************************************************************
NAME
    sendMicsServerAccessErrorRsp

DESCRIPTION
    Send an access response to the GATT  library.
*/

#define sendMicsServerAccessErrorRsp(task, cid, handle, error) \
    sendMicsServerAccessRsp( \
            task, \
            cid, \
            handle, \
            error, \
            0, \
            NULL \
            )

/**************************************************************************
NAME
    gattMicsServerWriteGenericResponse

DESCRIPTION
    Send a generic response to the client after a writing.
*/
void gattMicsServerWriteGenericResponse(
        CsrBtGattId       task,
        connection_id_t   cid,
        uint16            result,
        uint16            handle
        );

/***************************************************************************
NAME
    micsServerSendCharacteristicChangedNotification

DESCRIPTION
    Send a notification to the client to notify that the value of
    a characteristic is changed.
*/
void micsServerSendCharacteristicChangedNotification(
        CsrBtGattId     task,
        connection_id_t cid,
        uint16          handle,
        uint16          sizeValue,
        uint8 const *value
        );

/***************************************************************************
NAME
    micsHandleReadClientConfigAccess

DESCRIPTION
    Deals with access of the X_CLIENT_CONFIG handles in case of reading.
*/
void micsHandleReadClientConfigAccess(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 const clientConfig
        );

/***************************************************************************
NAME
    micsHandleWriteClientConfigAccess

DESCRIPTION
    Deals with access of a client config handle to be written and indicated
    to the application.
*/

bool micsHandleWriteClientConfigAccess(
        GMICS_T *micControlServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *access_ind,
        uint16 *clientConfig);
        
/***************************************************************************
NAME
    micsServerGetCidIndex

DESCRIPTION
    Search for a connected client by its cid and return its index in the array.
    If the client is not found, GATT_MICS_SERVER_INVALID_CID_INDEX is returned.
*/
uint8 micsServerGetCidIndex(GMICS_T *micControlServer, connection_id_t cid);

#endif /* GATT_MICS_SERVER_COMMON_H */
