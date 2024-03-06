/* Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_BASS_CLIENT_COMMON_H_
#define GATT_BASS_CLIENT_COMMON_H_

#include "gatt_csis_client.h"
#include "gatt_bass_client_private.h"

typedef struct
{
    connection_id_t cid;
    uint16 start_handle;
    uint16 end_handle;
} gatt_client_registration_params_t;

#define BASS_ADD_SERVICE_HANDLE(_List) \
    (ServiceHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ServiceHandleListElm_t))

#define BASS_REMOVE_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        bassInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

ServiceHandle getBassServiceHandle(GBASSC **gatt_bass_client, CsrCmnList_t *list);

CsrBool bassInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

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
                                                  const status_t status,
                                                  uint8 source_id);

/***************************************************************************
NAME
    bassClientSendBroadcastAudioScanControlOpCfm

DESCRIPTION
    Send a write confirmation message for a specific control point operation.
*/
void bassClientSendBroadcastAudioScanControlOpCfm(GBASSC *const bass_client,
                                                  const status_t status,
                                                  GattBassClientMessageId id);

/***************************************************************************
NAME
    bassClientConvertGattStatus

DESCRIPTION
    Convert a status_t in the appropriate gatt_bass_server_status
*/
GattBassClientStatus bassClientConvertGattStatus(status_t status);

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
                                                   status_t status,
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

bool GattBassRegisterClient(gatt_bass_client_registration_params_t *reg_param,
                            GBASSC *gatt_bass_client);

/***************************************************************************
NAME
    bassClientSwapByteTrasmissionOrder

DESCRIPTION
    Convert the byte trasmission order for the metadata value.
*/
void bassClientSwapByteTrasmissionOrder(uint8 *valueToSwap,
                                        uint8 len,
                                        uint8 *swapedValue);

/***************************************************************************
NAME
    bassClientExtractBroadcastReceiveStateCharacteristicValue

DESCRIPTION
    Extract the Broadcast Receive State characteristic value.
*/
void bassClientExtractBroadcastReceiveStateCharacteristicValue(uint8 *value,
                                                               GattBassClientBroadcastReceiveState *brscValue);

/***************************************************************************
NAME
    bassClientSetSourceId

DESCRIPTION
    Set the Source Id in the right Broadcast Receive State characteristic
    using its handle.
*/
void bassClientSetSourceId(GBASSC *bass_client,
                           uint16 handle,
                           uint8 sourceId);

#endif
