/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_BASS_CLIENT_READ_H_
#define GATT_BASS_CLIENT_READ_H_

#include "gatt_bass_client_private.h"

/***************************************************************************
NAME
    bassClientHandleReadValueResp

DESCRIPTION
    Handle the response to a CsrBtGattReadCfm message.
*/
void bassClientHandleReadValueResp(GBASSC *bass_client,
                                   const CsrBtGattReadCfm *read_cfm);


/***************************************************************************
NAME
    bassClientSendReadBroadcastReceiveStateCfm

DESCRIPTION
    Send GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CFM message
    as a result of a reading of a Broadcast Receive State characteristic
    on the remote device.
*/
void bassClientSendReadBroadcastReceiveStateCfm(GBASSC *bass_client,
                                                status_t status,
                                                uint16 size_value,
                                                uint8 *value);

#endif
