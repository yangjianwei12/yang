/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief   Header file for the LE Audio Clients.
*/

#ifndef LE_AUDIO_CLIENT_H_
#define LE_AUDIO_CLIENT_H_

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

#include "le_audio_client_audio_config.h"
#include "domain_message.h"
#include "source_param_types.h"
#include "kymera_usb_le_audio.h"


#define LE_LC3_7P5_MS_FRAME_DURATION                    (7500u)
#define LE_LC3_10_MS_FRAME_DURATION                     (10000u)
#define LE_APTX_LITE_DEFAULT_FRAME_DURATION             (5000u)
#define LE_APTX_LITE_FRAME_DURATION_6P25                (6250u)
#define LE_APTX_ADAPTIVE_FRAME_DURATION                 (15000u)

/*! \brief LE Audio Client status codes. */
typedef enum
{
    /*! The requested operation completed successfully */
    LE_AUDIO_CLIENT_STATUS_SUCCESS,

    /*! The requested operation failed to complete */
    LE_AUDIO_CLIENT_STATUS_FAILED
} le_audio_client_status_t;

/*! \brief Events sent by LE Audio Client to other modules. */
typedef enum
{
    /*! Module initialisation is complete */
    LE_AUDIO_CLIENT_INIT_CFM = LE_AUDIO_CLIENT_MESSAGE_BASE,

    /*! Event to indicate LE Audio Client has connected */
    LE_AUDIO_CLIENT_CONNECT_IND,

    /*! Event to indicate LE Audio Client has added one device into the group */
    LE_AUDIO_CLIENT_DEVICE_ADDED_IND,

    /*! Event to indicate LE Audio Client has removed one device from the group */
    LE_AUDIO_CLIENT_DEVICE_REMOVED_IND,

    /*! Event to indicate LE Audio Client has disconnected */
    LE_AUDIO_CLIENT_DISCONNECT_IND,

    /*! Event to indicate LE Audio Client has started streaming */
    LE_AUDIO_CLIENT_STREAM_START_IND,

    /*! Event to indicate LE Audio Client has cancelled start streaming request */
    LE_AUDIO_CLIENT_STREAM_START_CANCEL_COMPLETE_IND,

    /*! Event to indicate LE Audio Client has stopped streaming */
    LE_AUDIO_CLIENT_STREAM_STOP_IND,

    /*! Event to indicate call accept received from remote device */
    LE_AUDIO_CLIENT_REMOTE_CALL_CONTROL_CALL_ACCEPT,

    /*! Event to indicate call terminate received from remote device */
    LE_AUDIO_CLIENT_REMOTE_CALL_CONTROL_CALL_TERMINATE,

#ifdef ENABLE_ACK_FOR_PA_TRANSMITTED
    /*! Event to indicate update metadata PA was transmitted */
    LE_AUDIO_CLIENT_PA_TRANSMITTED_IND,
#endif

    /*! This must be the final message */
    LE_AUDIO_CLIENT_MESSAGE_END
} le_audio_client_msg_t;

/*! \brief Enums which turns on the PTS mode for specific profiles */
typedef enum
{
    /*! Indicates PTS mode is turned off */
    LE_AUDIO_CLIENT_PTS_MODE_OFF,

    /*! Enable CSIP for PTS qualification */
    LE_AUDIO_CLIENT_PTS_MODE_CSIP,

    /*! Enable BAP for PTS qualification */
    LE_AUDIO_CLIENT_PTS_MODE_BAP,

    /*! Enable VCP for PTS qualification */
    LE_AUDIO_CLIENT_PTS_MODE_VCP,

    /*! Enable GMCS for PTS qualification */
    LE_AUDIO_CLIENT_PTS_MODE_GMCS,

    /*! Enable GTBS for PTS qualification */
    LE_AUDIO_CLIENT_PTS_MODE_GTBS,

    /*! Enable CAP for PTS qualification */
    LE_AUDIO_CLIENT_PTS_MODE_CAP,

    /*! Enable TMAP for PTS qualification */
    LE_AUDIO_CLIENT_PTS_MODE_TMAP,
} le_audio_client_pts_mode_t;

/*! \brief Modes supported by LE audio client */
typedef enum
{
    /*! Unicast mode */
    LE_AUDIO_CLIENT_MODE_UNICAST,

    /*! Broadcast mode */
    LE_AUDIO_CLIENT_MODE_BROADCAST,
} le_audio_client_mode_t;

/*! \brief Data associated with LE Audio Client connected message */
typedef struct
{
    /*! Group handle for which LE audio client was connected */
    ServiceHandle group_handle;

    /*! Total devices present in the group */
    uint8 total_devices;

    /*! Number of currently connected devices in the group */
    uint8 connected_devices;

    /*! Is LE Audio Client successfully connected? */
    le_audio_client_status_t status;
} LE_AUDIO_CLIENT_CONNECT_IND_T;

/*! \brief Data associated with LE Audio Client device added message */
typedef struct
{
    /*! GATT identifier for which the LE Audio client was disconnected */
    gatt_cid_t cid;

    /*! Is LE Audio Client successfully connected? */
    le_audio_client_status_t status;

    /*! Bit which indicates that there is more devices to be added into the group
        This will be FALSE if all devices in the group got added. */
    bool more_devices_needed;
} LE_AUDIO_CLIENT_DEVICE_ADDED_IND_T;

/*! \brief Data associated with LE Audio Client device added message */
typedef struct
{
    /*! GATT identifier for which the LE Audio client was removed the device */
    gatt_cid_t cid;

    /*! Is LE Audio Client successfully disconnected the specified device? */
    le_audio_client_status_t status;

    /*! Bit which indicates that there is more devices present in the group
        which yet to be removed. This will be FALSE if all devices in the group
        got removed. */
    bool more_devices_present;
} LE_AUDIO_CLIENT_DEVICE_REMOVED_IND_T;

/*! \brief Data associated with LE Audio Client disconnected message */
typedef struct
{
    /*! Group handle for which the LE Audio client was disconnected. */
    ServiceHandle group_handle;

    /*! Is LE Audio Client successfully disconnected? */
    le_audio_client_status_t status;
} LE_AUDIO_CLIENT_DISCONNECT_IND_T;

/*! \brief Data associated with LE Audio Client streaming message */
typedef struct
{
    /*! Group handle for which the LE audio client started streaming */
    ServiceHandle group_handle;

    /*! Is LE Audio Client successfully started a streaming session? */
    le_audio_client_status_t status;

    /*! Source for which the streaming is started */
    generic_source_t  source;

    /*! Source for which the streaming is started */
    uint16 audio_context;
} LE_AUDIO_CLIENT_STREAM_START_IND_T;

/*! \brief Data associated with LE Audio Client streaming cancelled message */
typedef struct
{
    /*! Group handle for which LE Audio client cancelled streaming request */
    ServiceHandle group_handle;

    /*! Is LE Audio Client successfully cancelled a start streaming request? */
    le_audio_client_status_t status;
} LE_AUDIO_CLIENT_STREAM_START_CANCEL_COMPLETE_IND_T;

/*! \brief Data associated with LE Audio Client streaming message */
typedef struct
{
    /*! Group handle for which the LE Audio streaming was stopped */
    ServiceHandle  group_handle;

    /*! Is LE Audio Client successfully started a streaming session? */
    le_audio_client_status_t status;

    /*! Source for which the streaming is stopped */
    generic_source_t  source;
} LE_AUDIO_CLIENT_STREAM_STOP_IND_T;

/*! \brief Initialises the LE Audio Client Domain module

     \param init_task Task to receive responses.
     \return TRUE if able to initialize, returns FALSE otherwise.
*/
bool LeAudioClient_Init(Task init_task);

/*! \brief Register a Task to receive notifications from LE Audio Module.

    Once registered, #client_task will receive LE Audio Client messages

    \param client_task Task to register to receive LE Audio client messages
*/
void LeAudioClient_ClientRegister(Task client_task);

/*! \brief Un-register a Task that is receiving notifications from LE Audio Client.

    If the task is not currently registered then nothing will be changed.

    \param client_task Task to un-register from LE Audio Client notifications.
*/
void LeAudioClient_ClientUnregister(Task client_task);

/*! \brief Start streaming for the specified stream type

    \param group_handle The group handle on which the unicast streaming has to be started
    \param audio_context Audio context to stream

    NOTE: A success return value indicates that a streaming request has been placed
          successfully. Clients should handle LE_AUDIO_CLIENT_STREAM_START_IND to check
          whether the streaming was successfully started or not.
*/
bool LeAudioClient_StartStreaming(ServiceHandle group_handle, uint16 audio_context);

/*! \brief Cancels the request for start streaming which was sent earlier and pending to complete

    \param group_handle The group handle on which the streaming request has to be cancelled

    \return TRUE if able to initiate the cancel request.
            FALSE will be returned if there is no pending start streaming request or if it is too
            late to cancel the request.

    NOTE: A success return value indicates that a request to cancel the streaming has been placed
          successfully. Clients should handle LE_AUDIO_CLIENT_STREAM_START_CANCEL_COMPLETE_IND to
          check whether the streaming was successfully cancelled or not.
          Clients will also receive LE_AUDIO_CLIENT_STREAM_START_IND with status as failure prior
          to LE_AUDIO_CLIENT_STREAM_START_CANCEL_COMPLETE_IND.
*/
bool LeAudioClient_StartStreamingCancelRequest(ServiceHandle group_handle);

/*! \brief Stops the streaming on the Unicast server

    \param group_handle The group handle on which the unicast streaming has to be stopped.
    \param remove_config. Removes the configuration(codec) on the remote server.

    \return TRUE if able to successfully place a request to stop streaming.

    NOTE: A success return value indicates that a streaming stop request has been placed
          successfully. Clients should handle LE_AUDIO_CLIENT_STREAM_STOP_IND to check
          whether the streaming was successfully stopped or not.
*/
bool LeAudioClient_StopStreaming(ServiceHandle group_handle, bool remove_config);

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

/*! \brief Establishes the profile level connection for the specified GATT Connection.

     \param cid The GATT connection for which the LEA Profiles has to be connected
     \return TRUE if able to initiate the connect request, returns FALSE otherwise.

     NOTE: A success return value indicates that a connect request has been placed
           successfully. Clients should handle LE_AUDIO_CLIENT_DEVICE_ADDED_IND to check
           whether the given device was successfully added to the group or not. Client will
           receive message LE_AUDIO_CLIENT_CONNECT_IND too (after LE_AUDIO_CLIENT_DEVICE_ADDED_IND)
           if all devices in the group added successfully.
*/
bool LeAudioClient_Connect(gatt_cid_t cid);

/*! \brief Indicate that discovery of other group members have failed.
           LE audio client then will try to complete initialisation of existing devices.

     \return TRUE if able to initiate the connect request, returns FALSE otherwise.

     NOTE: A success return value indicates that a initialisation request has been placed
           successfully. Clients should handle LE_AUDIO_CLIENT_CONNECT_IND to check
           whether it is success or not.
*/
bool LeAudioClient_DeviceDiscoveryFailed(void);

/*! \brief Closes profile connections & cleans up the LE Audio context associated with this GATT connection.

     \param cid The GATT connection for which the underlying profile connections has to be closed.
                If cid is given as zero, then the whole group will get disconnected.
     \param disconnect_gracefully Attempt to disconnect the profile connections in a graceful way.

     NOTE: If LE Audio Client is in connected/connecting state, a request to close the underlying profile will be placed.
           If LE Audio Client is in streaming state:

           1. If a graceful disconnect is requested, a request to stop streaming first will be placed followed by a request
              to disconnect the profile.
                  If graceful disconnection is requested when a stop stream request is already in progress, then LE audio
              client will not issue another stream stop request, instead it will wait for the pending one to complete to
              disconnect the profiles and then it will send the disconnect indications.
              ie, LE_AUDIO_CLIENT_DEVICE_REMOVED_IND and LE_AUDIO_CLIENT_DISCONNECT_IND (if all devices removed).
           2. If a graceful disconnect is not requested, a request to directly shutdown the underlying profile will be placed.

           Note: If there is a linkloss OR the LE-ACL link got closed when in streaming state, clients should not try to 
                 request for a graceful disconnect. If still requested, the disconnect behavior might be undefined.

           A success return value indicates that a disconnect request has been initiated. Clients should handle
           LE_AUDIO_CLIENT_DEVICE_REMOVED_IND to check whether the given device was successfully removed from the group or not.
           Client will receive message LE_AUDIO_CLIENT_DISCONNECT_IND too (after LE_AUDIO_CLIENT_DEVICE_REMOVED_IND) if all
           devices in the group got removed successfully.
*/
bool LeAudioClient_Disconnect(gatt_cid_t cid, bool disconnect_gracefully);

/*! \brief Is LE Audio client connected?

    \param group_handle The group handle on which LE Audio Client connection has to be verified.

    \return TRUE if connected, FALSE otherwise.
*/
bool LeAudioClient_IsUnicastConnected(ServiceHandle group_handle);

/*! \brief Is LE Audio client actively streaming on the provided GATT connection?

    \param group_handle The group handle to verify if streaming ongoing
    \return TRUE if streaming, FALSE otherwises.
*/
bool LeAudioClient_IsUnicastStreamingActive(ServiceHandle group_handle);

/*! \brief Sets the absolute volume

    \param group_handle  Group handle on which the volume has to be applied.
    \param volume The absolute volume that has to be applied.

    \return None.

    NOTE: Client has to set the absolute volume in the range 0 - 100.
*/
void LeAudioClient_SetAbsoluteVolume(ServiceHandle group_handle, uint8 volume);

/*! \brief Get the absolute volume

    \param group_handle  Group handle on which get volume needs to be performed.

    \return current volume.
*/
uint8 LeAudioClient_GetAbsoluteVolume(ServiceHandle group_handle);

/*! \brief Place a request to Mute/Unmute for the specified connection

    \param group_handle  Group handle on which the mute setting to be applied.
    \param mute TRUE to mute, FALSE to unmute.

    \return None.
*/
void LeAudioClient_SetMute(ServiceHandle group_handle, bool mute);

/*! \brief Initialize default LE Audio context to connect upon dongle boot

    \param enable Enable gaming mode on boot up, Gaming if TRUE.

    \return None.
*/
void LeAudioClient_SetBootGameModeConfig(bool enable);

/*! \brief Create an incoming call

    \return TRUE if success, FALSE otherwise
*/
bool LeAudioClient_CreateIncomingCall(void);

/*! \brief Create an outgoing call

    \return TRUE if success, FALSE otherwise
*/
bool LeAudioClient_CreateActiveOutgoingCall(void);

/*! \brief Check if any call is active.
    \return TRUE if call is active, FALSE otherwise
*/
bool LeAudioClient_IsCallActive(void);

/*! \brief  Get the current media state

    \return Returns the media state.
*/
uint8 LeAudioClient_GetCurrentMediaState(void);

/*! \brief Get the group handle for the given cid

    \param cid The GATT connection to get the group handle. If 0xFFFF is passed
               as cid, function will return the currently active group handle.

    \return Returns group handle if success, 0 otherwise
*/
ServiceHandle LeAudioClient_GetGroupHandle(gatt_cid_t cid);

/*! \brief Check if LE audio client is in unicast mode or not

    \return TRUE if in unicast mode, FALSE otherwise
*/
bool LeAudioClient_IsInUnicastMode(void);

/*! \brief Returns CAP unicast session audio context

    \return CAP audio context
*/
uint16 LeAudioClient_GetUnicastSessionCapAudioContext(void);

/*! \brief Returns configured codec ID

    \return Codec ID
*/
uint32 LeAudioClient_GetConfiguredCodecId(void);

/*! \brief Check if the given advertisement data is from a set member

    \param adv_data Pointer to advertisement data.
    \param adv_data_len Length of the advertisement data

    \return TRUE if the advert is from set member, FALSE otherwise
*/
bool LeAudioClient_IsAdvertFromSetMember(uint8 *adv_data, uint16 adv_data_len);

#else /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#define LeAudioClient_Connect(cid)                                      (FALSE)
#define LeAudioClient_Disconnect(cid, disconnect_gracefully)            (FALSE)
#define LeAudioClient_IsUnicastConnected(group_handle)                  (FALSE)
#define LeAudioClient_IsUnicastStreamingActive(group_handle)            (FALSE)
#define LeAudioClient_DeviceDiscoveryFailed()                           (FALSE)
#define LeAudioClient_GetGroupHandle(cid)                               (INVALID_GROUP_HANDLE)
#define LeAudioClient_IsInUnicastMode()                                 (FALSE)
#define LeAudioClient_SetBootGameModeConfig(enable)
#define LeAudioClient_IsAdvertFromSetMember(adv_data, adv_data_len)     UNUSED(adv_data), UNUSED(adv_data_len), FALSE

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

/*! \brief Set the default context that needs to be configured when a connection is established.

    \param  default_context Context to configure for
*/
void LeAudioClient_SetDefaultAudioContext(uint16 default_context);

/*! \brief Returns CAP configured audio context

    \return CAP audio context
*/
uint16 LeAudioClient_GetConfiguredCapAudioContext(void);

/*! \brief Get the currently active speaker path sample rate used by the LEA profiles for streaming
           (either unicast/ broadcast)

    \return sample rate in Hz. Returns 0 if no streaming is ongoing
*/
uint32 LeAudioClient_GetActiveSpkrPathSampleRate(void);

/*! \brief Get the currently active mic path sample rate used by the LEA profiles for streaming

    \return sample rate in Hz. Returns 0 if there is no usecase for mic running.
            ie, if no gaming + VBC or conversational streaming
*/
uint32 LeAudioClient_GetActiveMicPathSampleRate(void);

/*! \brief Enable PTS mode for LE Audio Client

    \param mode  The profile for which the PTS qualifcation has to be verified for.

    \return Returns TRUE if able to turn on PTS mode, FALSE otherwise.
*/
bool LeAudioClient_EnablePtsMode(le_audio_client_pts_mode_t mode);

/*! \brief Sets the LE audio client mode

    \param mode  Mode to set
*/
void LeAudioClient_SetMode(le_audio_client_mode_t mode);

/*! Get the QHS level */
uint8 LeAudioClient_GetQhsLevel(void);

/*! \brief Callback function prototype for getting USB audio source connect parameters */
typedef bool (*LeAudioClient_UsbSourceParamCallback)(KYMERA_INTERNAL_USB_LE_AUDIO_START_T *message, bool is_voice, uint16 frame_duration);

/*! \brief Register callback function to USB audio source connect parameter data for PTS mode

    \param usb_audio_src_param_cb  Callback function.
*/
void LeAudioClient_RegisterUsbSourceConnectParamCallback(LeAudioClient_UsbSourceParamCallback usb_src_param_cb);

/*! \brief Start Unicast/Broadcast Streaming in PTS Mode.

    \param  mode          Cid on which the stream to be started
    \param  spkr_path     Enable Speaker Path (Host to Controller) OR (To Air Data)
    \param  mic_path      Enable Mic Path (Controller to Host) OR (From Air Data)

    \return  Return TRUE if able to start streaming in PTS mode. FALSE otherwise.
*/
void LeAudioClient_StartPtsStreamingToDevices(gatt_cid_t cid_1, gatt_cid_t cid_2, bool spkr_path, bool mic_path);

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

/*! \brief Broadcast source name/encryption status changed. Source name/encryption status will be updated locally and be
           effective from next streaming.

    \note If this API is called, it is assumed source name/encryption status has changed.
*/
void LeAudioClientBroadcast_BroadcastConfigChanged(void);

/*! \brief Set the broadcast mode as PBP

    \param pbp_enable TRUE to set broadcast mode as PBP
                      FALSE to set to default TMAP mode
*/
void LeAudioClientBroadcast_SetPbpMode(bool pbp_enable);

/*! \brief Check if LE audio client broadcast is configured in PBP mode or not

    \return TRUE if configured in PBP mode, FALSE otherwise.
*/
bool LeAudioClientBroadcast_IsInPbpMode(void);

/*! \brief Check if LE broadcast source role is available or not

    \return TRUE if available, FALSE otherwise.
*/
bool LeAudioClient_IsBroadcastSourcePresent(void);

/*! \brief Is LE Audio client actively streaming as a broadcast source

    \return TRUE if streaming, FALSE otherwises.
*/
bool LeAudioClient_IsBroadcastSourceStreamingActive(void);

/*! \brief  Updates metadata send over the broadcast periodic advertisements.
            Application can use this function to add metadata onto the PA train in
            LTV format. Refer to BASS Specification V1.0, section 3.2

     NOTE: This will overwrite the existing Metadata in prior PA packet and if the metadata on
           preceding packet was NULL, metadata will be sendout as a fresh data. Also, This
           should be called only while broadcast streaming is ongoing.

    \param length Length of metadta.
    \param metadata Metadata in LTV format. This can contain multiple data units in LTV format.

    \return TRUE if metadata update operation is initiated, else FALSE.
*/
bool LeAudioClientBroadcast_UpdateMetadataInPeriodicTrain(uint8 length, uint8 *metadata);

#else /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#define LeAudioClient_IsBroadcastSourceStreamingActive()                    FALSE
#define LeAudioClientBroadcast_SetPbpMode(mode)                             UNUSED(mode)

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */

#endif /* LE_AUDIO_CLIENT_H_ */

