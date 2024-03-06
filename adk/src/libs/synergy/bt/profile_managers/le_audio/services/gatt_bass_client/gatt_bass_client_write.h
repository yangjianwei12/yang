/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_BASS_CLIENT_WRITE_H_
#define GATT_BASS_CLIENT_WRITE_H_

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
#define BASS_CLIENT_BROADCAST_ADD_SOURCE_CTRL_POINT_PARAM_SIZE_MIN (16)

/* Minumum size of the parameters of Broadcast Audio Scan Control Point Characteristic in number of octets
   for the opcodes 0x03 (Modify Source operation) */
#define BASS_CLIENT_BROADCAST_MODIFY_SOURCE_CTRL_POINT_PARAM_SIZE_MIN (6)

/* Size of the Broadcast Audio Scan Control Point Characteristic in number of octets
   for the opcodes 0x05 (Remove Source operation) */
#define BASS_CLIENT_BROADCAST_REMOVE_SOURCE_CTRL_POINT_PARAM_SIZE (1)

/***************************************************************************
NAME
    bassClientHandleWriteValueRespCfm

DESCRIPTION
    Handle the CsrBtGattWriteCfm message.
*/
void bassClientHandleWriteValueRespCfm(GBASSC *const bass_client,
                                       const CsrBtGattWriteCfm *const write_cfm);

#endif
