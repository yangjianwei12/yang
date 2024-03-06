/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_BASS_CLIENT_READ_H_
#define GATT_BASS_CLIENT_READ_H_

#include <gatt_manager.h>

#include "gatt_bass_client_private.h"

/***************************************************************************
NAME
    bassClientHandleReadValueResp

DESCRIPTION
    Handle the response to a GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM message.
*/
void bassClientHandleReadValueResp(GBASSC *bass_client,
                                   const GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM_T *read_cfm);


/***************************************************************************
NAME
    bassClientSendReadBroadcastReceiveStateCfm

DESCRIPTION
    Send GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CFM message
    as a result of a reading of a Broadcast Receive State characteristic
    on the remote device.
*/
void bassClientSendReadBroadcastReceiveStateCfm(GBASSC *bass_client,
                                                gatt_status_t status,
                                                uint16 value_size,
                                                uint8 *value);

#endif
