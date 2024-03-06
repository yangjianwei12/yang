/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_BASS_CLIENT_WRITE_H_
#define GATT_BASS_CLIENT_WRITE_H_

#include <gatt_manager.h>

#include "gatt_bass_client.h"
#include "gatt_bass_client_private.h"

/* Size of the Broadcast Audio Scan Control Point Characteristic in number of octets
   for the opcodes 0x00 (Remote Scan Stop operation) */
#define BASS_CLIENT_BROADCAST_REMOTE_SCAN_STOP_CTRL_POINT_SIZE (1)

/* Size of the Broadcast Audio Scan Control Point Characteristic in number of octets
   for the opcodes 0x01 (Remote Scan Start operation) */
#define BASS_CLIENT_BROADCAST_REMOTE_SCAN_START_CTRL_POINT_SIZE BASS_CLIENT_BROADCAST_REMOTE_SCAN_STOP_CTRL_POINT_SIZE

/* Minumum size of the parameters of the Broadcast Audio Scan Control Point Characteristic in number of octets
   for the opcodes 0x02 (Add Source operation) */
#define BASS_CLIENT_BROADCAST_ADD_SOURCE_CTRL_POINT_PARAM_SIZE_MIN (14)

/* Minumum size of the parameters of Broadcast Audio Scan Control Point Characteristic in number of octets
   for the opcodes 0x03 (Modify Source operation) */
#define BASS_CLIENT_BROADCAST_MODIFY_SOURCE_CTRL_POINT_PARAM_SIZE_MIN (7)

/* Size of the Broadcast Code*/
#define BASS_CLIENT_BROADCAST_CODE_SIZE (16)

/* Size of the Broadcast Audio Scan Control Point Characteristic in number of octets
   for the opcodes 0x05 (Remove Source operation) */
#define BASS_CLIENT_BROADCAST_REMOVE_SOURCE_CTRL_POINT_PARAM_SIZE (1)

/***************************************************************************
NAME
    bassClientHandleWriteValueRespCfm

DESCRIPTION
    Handle the GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM message.
*/
void bassClientHandleWriteValueRespCfm(GBASSC *const bass_client,
                                       const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *const write_cfm);
/***************************************************************************
NAME
    bassClientHandleWriteWithoutResponseRespCfm

DESCRIPTION
    Handle the GATT_MANAGER_WRITE_WITHOUT_RESPONSE_CFM message.
*/
void bassClientHandleWriteWithoutResponseRespCfm(GBASSC *const bass_client,
                                       const GATT_MANAGER_WRITE_WITHOUT_RESPONSE_CFM_T *const write_cfm);

#endif
