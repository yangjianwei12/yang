/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface for sending messages from LE audio client to registered modules
*/

#ifndef LE_AUDIO_CLIENT_MESSAGES_H_
#define LE_AUDIO_CLIENT_MESSAGES_H_

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

#include "le_audio_client_context.h"

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
/*! Sends LE_AUDIO_CLIENT_STREAM_START_IND to registered clients with unicast as source*/
#define leAudioClientMessages_SendUnicastStreamStartInd(group_handle, stream_started, audio_context) \
            leAudioClientMessages_SendStreamStartInd(group_handle, stream_started, audio_context, FALSE)

/*! Sends LE_AUDIO_CLIENT_STREAM_STOP_IND to registered clients with unicast as source*/
#define leAudioClientMessages_SendUnicastStreamStopInd(group_handle, stream_stopped, audio_context) \
            leAudioClientMessages_SendStreamStopInd(group_handle, stream_stopped, audio_context, FALSE)
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
/*! Sends LE_AUDIO_CLIENT_STREAM_START_IND to registered clients with broadcast as source*/
#define leAudioClientMessages_SendBroadcastStreamStartInd(group_handle, stream_started, audio_context) \
            leAudioClientMessages_SendStreamStartInd(group_handle, stream_started, audio_context, TRUE)

/*! Sends LE_AUDIO_CLIENT_STREAM_STOP_IND to registered clients with broadcast as source*/
#define leAudioClientMessages_SendBroadcastStreamStopInd(group_handle, stream_stopped, audio_context) \
            leAudioClientMessages_SendStreamStopInd(group_handle, stream_stopped, audio_context, TRUE)
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */


/*! Sends LE_AUDIO_CLIENT_CONNECT_IND to registered clients */
void leAudioClientMessages_SendConnectInd(ServiceHandle group_handle, le_audio_client_status_t status,
                                          uint8 total_devices, uint8 connected_devices);

/*! Sends LE_AUDIO_CLIENT_DEVICE_ADDED_IND to registered clients */
void leAudioClientMessages_SendDeviceAddedInd(gatt_cid_t cid, bool device_added, bool more_devices_needed);

/*! Sends LE_AUDIO_CLIENT_DEVICE_REMOVED_IND to registered clients */
void leAudioClientMessages_SendDeviceRemovedInd(gatt_cid_t cid, bool device_removed, bool more_devices_present);

/*! Sends LE_AUDIO_CLIENT_DISCONNECT_IND to registered clients */
void leAudioClientMessages_SendDisconnectInd(ServiceHandle group_handle, bool disconnected);

/*! Sends LE_AUDIO_CLIENT_STREAM_START_IND to registered clients */
void leAudioClientMessages_SendStreamStartInd(ServiceHandle group_handle,
                                              bool stream_started,
                                              uint16 audio_context,
                                              bool is_source_broadcast);

/*! Sends LE_AUDIO_CLIENT_STREAM_START_CANCEL_COMPLETE_IND to registered clients */
void leAudioClientMessages_SendStreamStartCancelCompleteInd(ServiceHandle group_handle, bool cancelled);

/*! Sends LE_AUDIO_CLIENT_STREAM_STOP_IND to registered clients */
void leAudioClientMessages_SendStreamStopInd(ServiceHandle group_handle,
                                             bool stream_stopped,
                                             uint16 audio_context,
                                             bool is_source_broadcast);

#ifdef ENABLE_ACK_FOR_PA_TRANSMITTED
/*! Utility function to send the LE_AUDIO_CLIENT_PA_TRANSMITTED_IND to registered clients */
void leAudioClientMessages_SendPATransmittedInd(void);
#endif /* ENABLE_ACK_FOR_PA_TRANSMITTED */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
/*! Sends LE_AUDIO_CLIENT_REMOTE_CALL_CONTROL_CALL_ACCEPT to registered clients */
void leAudioClientMessages_HandleRemoteCallControlAccept(void);

/*! Sends LE_AUDIO_CLIENT_REMOTE_CALL_CONTROL_CALL_TERMINATE to registered clients */
void leAudioClientMessages_HandleRemoteCallControlTerminate(void);
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */

#endif /* LE_AUDIO_CLIENT_MESSAGES_H_ */

