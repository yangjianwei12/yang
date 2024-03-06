/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_BASS_CLIENT_COMMON_H_
#define GATT_BASS_CLIENT_COMMON_H_

#include "gatt_bass_client_private.h"

/***************************************************************************
NAME
    bassClientSendInternalMsgInitRead

DESCRIPTION
    Send a BASS_CLIENT_INTERNAL_MSG_INIT_READ message
*/
void bassClientSendInternalMsgInitRead(GBASSC *gatt_bass_client);

/***************************************************************************
NAME
    bassClientAddHandle

DESCRIPTION
    Add a new item to the list of handles
*/
void bassClientAddHandle(GBASSC *bass_inst,
                         uint8 source_id,
                         uint16 handle,
                         uint16 handle_ccc);

/***************************************************************************
NAME
    bassClientSendBroadcastReceiveStateSetNtfCfm

DESCRIPTION
    Send a GATT_BASS_CLIENT_BROADCAST_RECEIVE_STATE_SET_NTF_CFM message.
*/
void bassClientSendBroadcastReceiveStateSetNtfCfm(GBASSC *const bass_client,
                                                  const gatt_status_t status,
                                                  uint8 source_id);

/***************************************************************************
NAME
    bassClientSendBroadcastAudioScanControlOpCfm

DESCRIPTION
    Send a write confirmation message for a specific control point operation.
*/
void bassClientSendBroadcastAudioScanControlOpCfm(GBASSC *const bass_client,
                                                  const gatt_status_t status,
                                                  GattBassClientMessageId id);

/***************************************************************************
NAME
    bassClientConvertGattStatus

DESCRIPTION
    Convert a gatt_status_t in the appropriate gatt_bass_server_status
*/
GattBassClientStatus bassClientConvertGattStatus(gatt_status_t status);

/***************************************************************************
NAME
    bassClientSourceIdFromCccHandle

DESCRIPTION
    Find the SourceId associated with a Client Characteristic Configuration
    handle.
*/
bool bassClientSourceIdFromCccHandle(GBASSC *bass_client,
                                     uint16 handle_ccc,
                                     uint8 *source_id);

/***************************************************************************
NAME
    bassClientDestroyBroadcastReceiveStateHandlesList

DESCRIPTION
    Destroy the list of handles contained in the client memory instance.
*/
void bassClientDestroyBroadcastReceiveStateHandlesList(GBASSC *bass_inst);

/***************************************************************************
NAME
    bassClientSendReadBroadcastReceiveStateCccCfm

DESCRIPTION
    Send a GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CCC_CFM message.
*/
void bassClientSendReadBroadcastReceiveStateCccCfm(GBASSC *bass_client,
                                                   gatt_status_t status,
                                                   uint16 size_value,
                                                   const uint8 *value,
                                                   uint8 source_id);

/***************************************************************************
NAME
    bassClientHandleFromSourceId

DESCRIPTION
    Find the characteristic handle from the SourceId.
*/
uint16 bassClientHandleFromSourceId(GBASSC *bass_client,
                                    uint8 source_id,
                                    bool isCccHandle);

/***************************************************************************
NAME
    bassClientSwapByteTrasmissionOrder

DESCRIPTION
    Convert the byte trasmission order for the metadata value.
*/
void bassClientSwapByteTrasmissionOrder(uint8 *value_to_swap,
                                        uint8 len,
                                        uint8 *value);
#endif
