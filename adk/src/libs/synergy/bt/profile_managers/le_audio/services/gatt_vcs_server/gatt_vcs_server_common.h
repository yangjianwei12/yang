/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_VCS_SERVER_COMMON_H
#define GATT_VCS_SERVER_COMMON_H

#include "gatt_vcs_server_private.h"
#include "gatt_vcs_server_debug.h"
#include "csr_bt_gatt_lib.h"
#include "csr_pmem.h"

#define GATT_VCS_SERVER_CCC_NOTIFY                   (0x01)
#define GATT_VCS_SERVER_CCC_INDICATE                 (0x02)

/* Required octets for the Client Configuration Charactetistic */
#define GATT_VCS_SERVER_CCC_VALUE_SIZE               (2)

/* Size of the volume state characteristic (number of octets)*/
#define GATT_VCS_SERVER_VOLUME_STATE_SIZE            (3)

/* Size of the volume Flag characteristic (number of octets)*/
#define GATT_VCS_SERVER_VOLUME_FLAG_SIZE             (1)

/* Application Error code in case of opcode not supported
   in the Volume Control Point Characteristic */
#define GATT_VCS_SERVER_ERR_OPCODE_NOT_SUPPORTED      (0x81)
#define GATT_VCS_SERVER_ERR_INVALID_CHANGE_COUNTER    (0x80)

/* Maximum value of change counter */
#define GATT_VCS_SERVER_CHANGE_COUNTER_VALUE_MAX      (255)

/* VCS Server invalid cid index value */
#define GATT_VCS_SERVER_INVALID_CID_INDEX  (0xFF)

/**************************************************************************
NAME
    vcsServerSendAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
void vcsServerSendAccessRsp(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 result,
        uint16 size_value,
        uint8 *const value
        );

/**************************************************************************
NAME
    vcsServerSendAccessErrorRsp

DESCRIPTION
    Send an access response to the GATT Manager library with an error status.
*/

#define vcsServerSendAccessErrorRsp(task, cid, handle, error) \
    vcsServerSendAccessRsp( \
            task, \
            cid, \
            handle, \
            error, \
            0, \
            NULL \
            )

/**************************************************************************
NAME
    vcsServerWriteGenericResponse

DESCRIPTION
    Send a generic response to the clien after a writing.
*/
void vcsServerWriteGenericResponse(
        CsrBtGattId task,
        connection_id_t cid,
        uint16      result,
        uint16      handle
        );

/***************************************************************************
NAME
    vcsServerSendCharacteristicChangedNotification

DESCRIPTION
    Send a notification to the client to notify that the value of
    a characteristic is changed.
*/
void vcsServerSendCharacteristicChangedNotification(
        CsrBtGattId  task,
        connection_id_t cid,
        uint16 handle,
        uint16 size_value,
        uint8 *const value
        );

/***************************************************************************
NAME
    vcsServerHandleReadClientConfigAccess

DESCRIPTION
    Deals with access of the HANDLE_VOLUME_SETTING_CLIENT_CONFIG,
    and HANDLE_VOLUME_FLAG_CLIENT_CONFIG handles in case of reading.
*/
void vcsServerHandleReadClientConfigAccess(
        CsrBtGattId  task,
        connection_id_t cid,
        uint16 handle,
        const uint16 client_config
        );

/***************************************************************************
NAME
    vcsServerHandleWriteClientConfigAccess

DESCRIPTION
    Deals with the access of a client config handle to be written and indicated
    to the application.
*/

void vcsServerHandleWriteClientConfigAccess(
        GVCS *volume_control_server,
        GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind);

/***************************************************************************
NAME
    vcsServerComposeVolumeStateValue

DESCRIPTION
    Compose the Volume State characteritstic value.
*/

void vcsServerComposeVolumeStateValue(
        uint8 *value,
        GVCS *const volume_control_server
        );

/***************************************************************************
NAME
    vcsServerHandleChangeCounter

DESCRIPTION
    Handle an increment of the change counter of the
    Volume State characteristic.
*/
void vcsServerHandleChangeCounter(GVCS *volume_control_server);

/***************************************************************************
NAME
    vcsServerGetCidIndex

DESCRIPTION
    Search for a connected client by its cid and return its index in the array.
    If the client is not found, GATT_VCS_SERVER_INVALID_CID_INDEX is returned.
*/
uint8 vcsServerGetCidIndex(GVCS *volume_control_server, connection_id_t cid);

#endif /* GATT_VCS_SERVER_COMMON_H */
