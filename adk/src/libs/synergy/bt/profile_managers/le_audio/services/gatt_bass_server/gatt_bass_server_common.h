/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#ifndef GATT_BASS_SERVER_COMMON_H
#define GATT_BASS_SERVER_COMMON_H

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

/* BASS Server invalid cid index value */
#define GATT_BASS_SERVER_INVALID_CID_INDEX  (0xFF)

/* Opcode values for the Broadcast Audio Scan Control Point Characteristic */
#define GATT_BASS_REMOTE_SCAN_STOP_OPCODE    (0x00)
#define GATT_BASS_REMOTE_SCAN_START_OPCODE   (0x01)
#define GATT_BASS_ADD_SOURCE_OPCODE          (0x02)
#define GATT_BASS_MODIFY_SOURCE_OPCODE       (0x03)
#define GATT_BASS_SET_BROADCAST_CODE_OPCODE  (0x04)
#define GATT_BASS_REMOVE_SOURCE_OPCODE       (0x05)

/**************************************************************************
NAME
    sendBassServerAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
void sendBassServerAccessRsp(
        CsrBtGattId task,
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
        CsrBtGattId     task,
        connection_id_t cid,
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
        CsrBtGattId  task,
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
        CsrBtGattId task,
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
void bassServerHandleWriteClientConfigAccess(GBASSSS *bass_server,
                                             const CsrBtGattAccessInd *access_ind,
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
void bassServerSwapByteTransmissionOrder(uint8* value_to_swap,
                                         uint8 len,
                                         uint8* value);

/***************************************************************************
NAME
    bassServerSwapByteWriteTransmissionOrder

DESCRIPTION
    Convert the byte trasmission order for a GATT Write payload.
*/
void bassServerSwapByteWriteTransmissionOrder(uint16 numWriteUnits,
                                              CsrBtGattAttrWritePairs *writeUnit,
                                              uint16 startOffset,
                                              uint8 len,
                                              uint8* value);

/***************************************************************************
NAME
    bassConvertBroadcastReceiveStateValue

DESCRIPTION
    Convert the value of a Broadcast Receive State characteristic.
*/
void bassConvertBroadcastReceiveStateValue(GBASSSS *bass_server, uint8 *value, uint8 index);

/***************************************************************************
NAME
    bassServerNotifyBroadcastReceiveStateCharacteristicToSingleClient

DESCRIPTION
    Send a notification of the value of a Broadcast Receive State
    characteristic to a specific client if it has been registered for notifications.
*/
void bassServerNotifyBroadcastReceiveStateCharacteristicToSingleClient(GBASSSS *bass_server,
                                                                       uint8 index,
                                                                       uint8 indexClient);

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
    bassServerGetElementBuffer

DESCRIPTION
    Get an element from the ATT write buffer
*/
uint8 bassServerGetElementBuffer(uint16 numWriteUnits,
                                 CsrBtGattAttrWritePairs *writeUnit,
                                 uint16 offset);

/***************************************************************************
NAME
    bassServerIsControlPointLengthValid

DESCRIPTION
    Check the length of the Broadcast Audio Scan Control Point
    characteristic for the Add and Modify Source opcodes
*/
bool bassServerIsControlPointLengthValid(uint16 length,
                                         uint8 opcode,
                                         uint16 numWriteUnits,
                                         CsrBtGattAttrWritePairs *writeUnit);

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
    Check if the Source_id value  is valid.
*/
bool bassServerIsValidSourceId(GBASSSS *bassServer, uint8 sourceId);

/***************************************************************************
NAME
    bassServerIsBadCodeChanged

DESCRIPTION
    Check if the bad code is changed.
*/
bool bassServerIsBadCodeChanged(GBASSSS *bass_server,
                                uint8 *badCode,
                                uint8 index);

/***************************************************************************
NAME
    bassServerIsBisSyncChanged

DESCRIPTION
    Check if the BIS Sync State is changed.
*/
bool bassServerIsBisSyncChanged(GBASSSS *bass_server,
                                GattBassServerSubGroupsData *subGroupsData,
                                uint8 index);

/***************************************************************************
NAME
    bassServerIsMetadataChanged

DESCRIPTION
    Check if the metadata are changed.
*/
bool bassServerIsMetadataChanged(GBASSSS *bass_server,
                                 GattBassServerSubGroupsData *subGroupsData,
                                 uint8 index);
/***************************************************************************
NAME
    bassServerFindCid

DESCRIPTION
    Search for a connected client by its cid and put its index in the array
    in the index parameter.
    It returns TRUE if the client is found, otherwise FALSE.
*/
bool bassServerFindCid(GBASSSS *bass_server, connection_id_t cid, uint8 *index);

#endif /* GATT_BASS_SERVER_COMMON_H */
