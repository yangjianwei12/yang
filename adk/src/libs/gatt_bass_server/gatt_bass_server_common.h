/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_BASS_SERVER_COMMON_H
#define GATT_BASS_SERVER_COMMON_H

#include <csrtypes.h>
#include <message.h>

#include <gatt_manager.h>

#include "gatt_bass_server.h"
#include "gatt_bass_server_private.h"

#define GATT_BASS_CLIENT_CONFIG_NOTIFY                  (0x01)
#define GATT_BASS_CLIENT_CONFIG_INDICATE                (0x02)

/* Size of a Broadcast Receive State characteristic in the GATT database */
#define GATT_BASS_BROADCAST_RECEIVE_STATE_DB_SIZE    (1)

/* Required octets for values sent to Client Configuration Descriptor */
#define GATT_BASS_CLIENT_CONFIG_VALUE_SIZE           (2)

/* Minimum size of the the vaue of the Broadcast Receive State characteristic */
#define GATT_BASS_BROADCAST_RECEIVE_STATE_SIZE_MIN       (15)

/* Application Error code in case of opcode not supported
   in the Broadcast Audio Scan Control Point Characteristic */
#define GATT_BASS_SERVER_ERR_UNSUPPORTED_OPCODE  (0x80)

/* Application Error code in case of the Source_ID written by a client
 * does not match any Source_ID exposed in a Broadcast Receive State
 * characteristic value by the server. */
#define GATT_BASS_SERVER_ERR_INVALID_SOURCE_ID  (0x81)

/* Minimum fixed size of the Broadcoast Audio Control Point characteristic in case of Add Source Opcode */
#define GATT_BASS_ADD_SOURCE_OPCODE_SIZE       (16)

/* Minimum fixed size of the Broadcoast Audio Control Point characteristic in case of Modify Source Opcode */
#define GATT_BASS_MODIFY_SOURCE_OPCODE_SIZE    (6)

#define GATT_BASS_SERVER_BIS_STATE_NO_PREFERENCE (0xFFFFFFFFu)

/**************************************************************************
NAME
    sendBassServerAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
void sendBassServerAccessRsp(
        Task task,
        connection_id_t cid,
        uint16 handle,
        uint16 result,
        uint16 size_value,
        const uint8 *value
        );

/**************************************************************************
NAME
    sendBassServerAccessErrorRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/

#define sendBassServerAccessErrorRsp(task, cid, handle, error) \
    sendBassServerAccessRsp( \
            task, \
            cid, \
            handle, \
            error, \
            0, \
            NULL \
            )

/**************************************************************************
NAME
    gattBassServerWriteGenericResponse

DESCRIPTION
    Send a generic response to the client after a writing.
*/
void gattBassServerWriteGenericResponse(
        Task        task,
        connection_id_t      cid,
        uint16      result,
        uint16      handle
        );

/***************************************************************************
NAME
    bassServerSendCharacteristicChangedNotification

DESCRIPTION
    Send a notification to the client to notify that the value of
    a characteristic is changed.
*/
void bassServerSendCharacteristicChangedNotification(
        Task  task,
        connection_id_t cid,
        uint16 handle,
        uint16 size_value,
        const uint8 *value
        );

/***************************************************************************
NAME
    bassServerHandleReadClientConfigAccess

DESCRIPTION
    Deals with access of all the HANDLE_BROADCAST_RECEIVE_STATE_CLIENT_CONFIG,
    handles in case of reading.
*/
void bassServerHandleReadClientConfigAccess(
        Task task,
        connection_id_t cid,
        uint16 handle,
        const uint16 client_config
        );

/***************************************************************************
NAME
    bassServerHandleWriteClientConfigAccess

DESCRIPTION
    Deals with access of a client config handle to be written and indicated
    to the application.
*/
void bassServerHandleWriteClientConfigAccess(
        Task task,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
        uint16 *client_config);

/***************************************************************************
NAME
    bassFindBroadcastSource

DESCRIPTION
    Check if there is a specific broadcast source in one of
    the Broadcast Receive State characteristics:
    if true, its index it will be saved in the index paramenter
*/
/***************************************************************************/
bool bassFindBroadcastSource(GBASSSS *bass_server, uint8 source_id, uint8 *index);

/***************************************************************************
NAME
    bassIsBroadcastReceiveStateFree

DESCRIPTION
    Check if a Broadcast Receive State characteristic is free
*/
/***************************************************************************/
bool bassIsBroadcastReceiveStateFree(GBASSSS *bass_server, uint8 index);

/***************************************************************************
NAME
    bassServerCheckBroadcastSourceInfo

DESCRIPTION
    Check if the values of the provided info for a Broadcast Source are valid.
*/
GattBassServerStatus bassServerCheckBroadcastSourceInfo(GattBassServerReceiveState *source_info);

/***************************************************************************
NAME
    bassServerSwapByteTransmissionOrder

DESCRIPTION
    Convert the byte trasmission order.
*/
void bassServerSwapByteTransmissionOrder(uint8 *value_to_swap,
                                         uint8 len,
                                         uint8 *value);

/***************************************************************************
NAME
    bassConvertBroadcastReceiveStateValue

DESCRIPTION
    Convert the value of a Broadcast Receive State characteristic.
*/
void bassConvertBroadcastReceiveStateValue(GBASSSS *bass_server, uint8 *value, uint8 index);

/***************************************************************************
NAME
    bassServerNotifyBroadcastReceiveStateCharacteristic

DESCRIPTION
    Send a notification of the value of a Broadcast Receive State
    characteristic to all the clients have been registered for notifications.
*/
void bassServerNotifyBroadcastReceiveStateCharacteristic(GBASSSS *bass_server, uint8 index);

/***************************************************************************
NAME
    bassServerGetHandleBroadcastReceiveStateCharacteristic

DESCRIPTION
    Get the handle of a broadacast receive state characteristic
    in the ATT database.
*/
uint16 bassServerGetHandleBroadcastReceiveStateCharacteristic(uint8 index);

/***************************************************************************
NAME
    bassServerIsSynchronized

DESCRIPTION
    Check if we are synchronized with the PA and/or to the BIS/BIG of a
    broadcast source.
*/
bool bassServerIsSynchronized(GBASSSS * bass_server, uint8 source_index);

/***************************************************************************
NAME
    bassServerCalculateAddSourceOpLength

DESCRIPTION
    Calculate the size of the Broadcast Audio Scan Control Point
    characteristic for the Add Source opcode
*/
uint16 bassServerCalculateAddSourceOpLength(uint8 *value);

/***************************************************************************
NAME
    bassServerCalculateModifySourceOpLength

DESCRIPTION
    Calculate the size of the Broadcast Audio Scan Control Point
    characteristic for the Modify Source opcode
*/
uint16 bassServerCalculateModifySourceOpLength(uint8 *value);

/***************************************************************************
NAME
    bassServerIsValidBisSync

DESCRIPTION
    Check if BIS sync is a valid value.
*/
bool bassServerIsValidBisSync(uint32 *bisSync, uint8 numSubGroups);

/***************************************************************************
NAME
    bassServerCalculateBroadcastReceiveStateCharacteristicLen

DESCRIPTION
    Calculate the size of the value of a Broadcast Receive State
    characteristic.
*/
uint16 bassServerCalculateBroadcastReceiveStateCharacteristicLen(gatt_bass_broadcast_source_info_t *sourceInfo);

/***************************************************************************
NAME
    bassServerIsValidBadCodeValue

DESCRIPTION
    Check if the Bad_Code value  is valid.
*/
bool bassServerIsValidBadCodeValue(uint8 *badCode, GattBassServerBroadcastBigEncryption bigEncryption);

/***************************************************************************
NAME
    bassServerCalcNumBroadcastReceiveStateCharacteristicsNotEmpty

DESCRIPTION
    Return the number of Broadcast Receive State characteristics not empty.
    It will return zero, if all are empty.
*/
uint8 bassServerCalcNumBroadcastReceiveStateCharacteristicsNotEmpty(GBASSSS * bassServer);

/***************************************************************************
NAME
    bassServerIsAnyBroadcastReceiveStateCharacteristicsEmpty

DESCRIPTION
    If there is a free Broadcast Receive State Characteristic,
    this function will return true and it will put in index the value
    of the index of the first free Broadcast Receive State Characteristic;
    otherwhise it will return FALSE and it will set index to zero.
*/
bool bassServerIsAnyBroadcastReceiveStateCharacteristicsEmpty(GBASSSS * bassServer, uint8 *index);

/***************************************************************************
NAME
    bassServerIsValidSourceId

DESCRIPTION
    Check if the Source_id value is valid.
*/
bool bassServerIsValidSourceId(GBASSSS *bassServer, uint8 sourceId);

#endif /* GATT_BASS_SERVER_COMMON_H */
