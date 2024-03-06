/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   le_cap LE CAP
    @{
    \ingroup    profiles
    \brief      Header file for CAP profile client
   
*/

#ifndef CAP_PROFILE_CLIENT_H
#define CAP_PROFILE_CLIENT_H

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

#include "cap_client_prim.h"
#include "bt_types.h"

#define CAP_PROFILE_CLIENT_MICROPHONE_COUNT_NONE    0 /*! No microphone used */
#define CAP_PROFILE_CLIENT_MICROPHONE_COUNT_ONE     1 /*! One microphone used */
#define CAP_PROFILE_CLIENT_MICROPHONE_COUNT_TWO     2 /*! Two microphone used (left and right) */

/*! \brief CAP Profile Client status codes. */
typedef enum
{
    /*! The requested operation completed successfully */
    CAP_PROFILE_CLIENT_STATUS_SUCCESS,

    /*! The requested operation failed to complete */
    CAP_PROFILE_CLIENT_STATUS_FAILED,

    /*! The requested operation failed to complete because CAS service is not present */
    CAP_PROFILE_CLIENT_STATUS_FAILED_AS_CAS_NOT_PRESENT
} cap_profile_client_status_t;

/*! \brief Events sent by CAP profile to other modules. */
typedef enum
{
    /*! Discovered CAS Service and the profile instance has been created */
    CAP_PROFILE_CLIENT_MSG_ID_INIT_COMPLETE,

    /*! Event to inform Codec & Qos configuration completed */
    CAP_PROFILE_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE,

    /*! Event to inform that CIS got connected */
    CAP_PROFILE_CLIENT_MSG_ID_UNICAST_CIS_CONNECT,

    /*! Event to inform Unicast streaming has started */
    CAP_PROFILE_CLIENT_MSG_ID_UNICAST_STREAM_START,

    /*! Event to inform Unicast streaming has stopped */
    CAP_PROFILE_CLIENT_MSG_ID_UNICAST_STREAM_STOP,

    /*! Event to inform that device have been added to the group */
    CAP_PROFILE_CLIENT_MSG_ID_DEVICE_ADDED,

    /*! Event to inform that device have been removed from the group */
    CAP_PROFILE_CLIENT_MSG_ID_DEVICE_REMOVED,

    /*! Event to inform CAP Profile has been disconnected (all devices are removed) */
    CAP_PROFILE_CLIENT_MSG_ID_PROFILE_DISCONNECT,

    /*! Event to inform CIS Link's has been lost */
    CAP_PROFILE_CLIENT_MSG_ID_CIS_LINK_LOSS,

    /*! Event to inform volume state changes from remote */
    CAP_PROFILE_CLIENT_MSD_ID_VOLUME_STATE,

    /*! Event to inform configuration is removed  */
    CAP_PROFILE_CLIENT_MSD_ID_UNICAST_CONFIG_REMOVED,

    /*! Event to inform Flush timeout information received from remote */
    CAP_PROFILE_CLIENT_MSD_ID_FLUSH_TIMEOUT_INFO,

    /*! This must be the final message */
    CAP_PROFILE_CLIENT_MSG_ID_MESSAGE_END
} cap_profile_client_msg_id_t;

typedef struct
{
    uint8_t min_flush_timeout;      /*<! Minimum flush time out in FT units */
    uint8_t max_flush_timeout;      /*<! Maximum flush time out in FT units */
    uint8_t max_bit_rate;           /*<! Maximum bitrate */
    uint8_t err_resilience;         /*<! Error resilience & downmix */
    uint8_t latency_mode;           /*<! should be 0 (don't change) */
} cap_profile_ft_info_t;

/*! \brief Data associated with CAP Profile instance creation */
typedef struct
{
    /*! Group handle for which the CAP profile instance was created. */
    ServiceHandle                   group_handle;

    /*! Total devices in the coordinated group */
    uint8                           total_devices;

    /*! Connected devices in the coordinated group */
    uint8                           connected_devices;

    /*! status which tells if CAP Profile instance is created or not */
    cap_profile_client_status_t     status;
} CAP_PROFILE_CLIENT_INIT_COMPLETE_T;

/*! \brief Data associated with CAP Profile device add */
typedef struct
{
    /*! GATT Connection identifier for which the CAP profile instance was created. */
    gatt_cid_t                      cid;

    /*! Group handle the device belongs to */
    ServiceHandle                   group_handle;

    /*! status which tells if CAP Profile is added or not */
    cap_profile_client_status_t     status;

    /*! Flag to indicate if there are still devices yet to be added into the group */
    bool more_devices_needed;
} CAP_PROFILE_CLIENT_DEVICE_ADDED_T;

/*! \brief Data associated with CAP Profile device remove */
typedef struct
{
    /*! GATT Connection identifier for which the CAP profile instance was removed.
        For a group destroy operation, this will be zero. */
    gatt_cid_t                      cid;

    /*! status which tells if CAP Profile is removed or not */
    cap_profile_client_status_t     status;

    /*! Flag to indicate if there are still devices yet to be removed */
    bool more_devices_present;
} CAP_PROFILE_CLIENT_DEVICE_REMOVED_T;

/*! \brief Data associated with CAP Profile instance destruction */
typedef struct
{
    /*! Group handle on which the disconnected device belongs */
    ServiceHandle                   group_handle;

    /*! status which tells if CAP Profile instance is destroyed or not */
    cap_profile_client_status_t     status;
} CAP_PROFILE_CLIENT_DISCONNECT_T;

/*! \brief Data associated with CAP Profile Codec & Qos configuration */
typedef struct
{
    /*! Group handle for which the Codec & QoS is done */
    ServiceHandle                   group_handle;

    /*! Audio context for which the configuration was done */
    uint16                          audio_context;

    /*! Status which tells configuration is done successfully or not */
    cap_profile_client_status_t     status;
} CAP_PROFILE_CLIENT_UNICAST_CONFIG_COMPLETE_T;

/*! \brief Data associated with CAP Profile CIS connections */
typedef struct
{
    /*! GATT Connection identifier for which the CIS got connected. */
    gatt_cid_t                      cid;

    /*! CIS Count */
    uint8                           cis_count;

    /*! Isochronous handles information */
    void                            *cis_handles;

    /*! Negotiated codec and QOS configuration */
    void                            *codec_qos_config;

    /*! Status which tells CIS is connected successfully or not */
    cap_profile_client_status_t     status;
} CAP_PROFILE_CLIENT_UNICAST_CIS_CONNECT_T;

/*! \brief Data associated with CAP Profile Unicast streaming */
typedef struct
{
    /*! Group handle for which the streaming was started. */
    ServiceHandle    group_handle;

    /*! Status which tells unicast streaming started or not*/
    cap_profile_client_status_t status;
} CAP_PROFILE_CLIENT_UNICAST_STREAM_START_T;

/*! \brief Data associated with CIS link loss */
typedef struct
{
    /*! GATT Connection identifier for which the CIS link was lost. */
    ServiceHandle    group_handle;
} CAP_PROFILE_CLIENT_CIS_LINK_LOSS_T;

/*! \brief Data associated with volume state change */
typedef struct
{
    /*! Group handle for which the volume has changed. */
    ServiceHandle    group_handle;

    /*! volume state. */
    uint8            volumeState;

    /*! mute state */
    uint8            mute;

    /*! change counter */
    uint8            changeCounter;
} CAP_PROFILE_CLIENT_VOLUME_STATE_T;


typedef struct
{
    /*! Group handle for which the volume has changed. */
    ServiceHandle    group_handle;

    /*! Flush timeout information */
    cap_profile_ft_info_t ft_info;
}CAP_PROFILE_CLIENT_MSD_ID_FLUSH_TIMEOUT_INFO_T;

/*! \brief Data associated with CAP Profile Stream stop indication */
typedef CAP_PROFILE_CLIENT_UNICAST_STREAM_START_T CAP_PROFILE_CLIENT_UNICAST_STREAM_STOP_T;

/*! \brief Data associated with CAP Profile configuration removal */
typedef CAP_PROFILE_CLIENT_DISCONNECT_T CAP_PROFILE_CLIENT_UNICAST_CONFIG_REMOVED_T;

typedef union
{
    CAP_PROFILE_CLIENT_INIT_COMPLETE_T              init_complete;
    CAP_PROFILE_CLIENT_UNICAST_CONFIG_COMPLETE_T    unicast_config_complete;
    CAP_PROFILE_CLIENT_UNICAST_CIS_CONNECT_T        unicast_cis_connect;
    CAP_PROFILE_CLIENT_UNICAST_STREAM_START_T       unicast_stream_start;
    CAP_PROFILE_CLIENT_UNICAST_STREAM_STOP_T        unicast_stream_stop;
    CAP_PROFILE_CLIENT_DISCONNECT_T                 disconnect_complete;
    CAP_PROFILE_CLIENT_DEVICE_ADDED_T               device_added;
    CAP_PROFILE_CLIENT_DEVICE_REMOVED_T             device_removed;
    CAP_PROFILE_CLIENT_CIS_LINK_LOSS_T              cis_link_loss;
    CAP_PROFILE_CLIENT_VOLUME_STATE_T               volume_state;
    CAP_PROFILE_CLIENT_UNICAST_CONFIG_REMOVED_T     unicast_config_removed;
    CAP_PROFILE_CLIENT_MSD_ID_FLUSH_TIMEOUT_INFO_T  flush_timeout_info;
} cap_profile_client_message_body_t;

typedef struct
{
    cap_profile_client_msg_id_t         id;
    cap_profile_client_message_body_t   body;
} cap_profile_client_msg_t;

typedef struct
{
    uint16 source_iso_handle;
    uint16 source_iso_handle_right;
    uint32 sample_rate;
    uint16 frame_length;
    uint16 frame_duration;
    uint16 stream_type;
    uint32 presentation_delay;
    uint8 codec_type;
    uint8 codec_version;
    uint8 codec_frame_blocks_per_sdu;
    bool start_muted;
    bool gaming_mode;
    bool use_cvc;
} cap_profile_client_media_config_t;

/*! Callback structure used when an observer registers with the CAP Client Profile module. */
typedef void (*cap_profile_client_callback_handler_t)(ServiceHandle group_handle, const cap_profile_client_msg_t *message);


/*! \brief Initialize the CAP Profile Domain component.

    \param init_task Task to receive responses.
*/
bool CapProfileClient_Init(cap_profile_client_callback_handler_t handler);

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
/*! \brief Creates an instance for the CAP Profile.

    \param cid GATT Connection identifier for which to create CAP Profile connection

    \note A success return value indicates that a create instance request has been placed
          successfully. Clients should handle CAP_PROFILE_CLIENT_MSG_ID_DEVICE_ADDED to check
          if the instance has been successfully created or not for the given cid.
          Also it will receive CAP_PROFILE_CLIENT_MSG_ID_INIT_COMPLETE if all the device
          in the group got addded. (ie, if only device in the group)
*/
bool CapProfileClient_CreateInstance(gatt_cid_t cid);

/*! \brief Initiates Codec & QoS configuration for Gaming(with VBC)

    \param group_handle  Group handle on which the configuration to be done.
    \param sink_capability Sink capability to be used for Gaming Sink (bit mask from CapClientSreamCapability)
    \param source_capability Source capability to be used for Gaming Source (bit mask from CapClientSreamCapability)
    \param latency Target latency to be used.
    \param mic_count Number of microphones (0 - No VBC, 1 - Single MIC, 2 - Dual MIC).
    \param cap_config_mode Config mode to be used (default OR Q2Q).
    \param cig_qhs_config CIG related parameters to be used(only for Q2Q).

    \return TRUE if able to place a request for configuration for GAMING.

    \note A success return value indicates that a configuration request has been placed
          successfully. Clients should handle CAP_PROFILE_CLIENT_UNICAST_CONFIG_COMPLETE to check
          whether the configuration was successfully done or not.If QHS is requested in cap_config_mode
          then cigConfig must be populated, else it can be NULL.
*/
bool CapProfileClient_ConfigureForGaming(ServiceHandle group_handle,
                                         uint32 sink_capability,
                                         uint32 source_capability,
                                         uint8 latency,
                                         uint8 mic_count,
                                         uint8 cap_config_mode,
                                         const CapClientQhsConfig *cig_qhs_config);

/*! \brief Start Unicast streaming

    \param group_handle  Group handle on which the streaming has to be started.
    \param audio_context  audio context for which the streaming has to be started.

    \return TRUE if able to place a start request for streaming for the specified context.

    \note A success return value indicates that a streaming request has been placed
          successfully. Clients should handle CAP_PROFILE_CLIENT_UNICAST_STREAM_START to check
          whether the streaming has successfully started or not.
*/
bool CapProfileClient_StartUnicastStreaming(ServiceHandle group_handle, uint16 audio_context);

/*! \brief Stop Unicast streaming

    \param group_handle  Group handle on which the streaming has to be stopped.
    \param remove_configured_context CAP will erase the configurations done earlier for this audio context.

    \return TRUE if able to place a stop request for streaming

    \note A success return value indicates that a stop streaming request has been placed
          successfully. Clients should handle CAP_PROFILE_CLIENT_UNICAST_STREAM_STOP to check
          whether the streaming has successfully stopped or not.
*/
bool CapProfileClient_StopUnicastStreaming(ServiceHandle group_handle, bool remove_configured_context);

/*! \brief Sets the absolute volume

    \param group_handle  GATT Connection on which the volume has to be applied.
    \param volume The absolute volume that has to be applied.

    \return None.

    \note Client has to set the absolute volume in the range 0 - 100.
*/
void CapProfileClient_SetAbsoluteVolume(ServiceHandle group_handle, uint8 volume);

/*! \brief Place a request to Mute/Unmute for the specified connection

    \param group_handle  Group handle on which the mute setting to be applied.
    \param mute TRUE to mute, FALSE to unmute.

    \return None.
*/
void CapProfileClient_SetMute(ServiceHandle group_handle, bool mute);

/*! \brief Remove gaming configuration from CAP

    \param group_handle  Group handle on which configuration to be removed.
    \param use_case  Usecase configuration that needs to be removed.

    \return TRUE if able to place a remove configuration request.

    \note A success return value indicates that a remove configuration request has been placed
          successfully. Clients should handle CAP_PROFILE_CLIENT_MSD_ID_UNICAST_CONFIG_REMOVED
          to check whether the configuration has successfully removed or not.
*/
bool CapProfileClient_RemoveGamingConfiguration(ServiceHandle group_handle, uint16 use_case);

/*! \brief Add device to the group

    \param group_handle  Group handle to which the device needs to be added
    \param cid GATT Connection identifier of the device which needs to be added.

    \note A success return value indicates that a add device request has been placed
          successfully. Clients should handle CAP_PROFILE_CLIENT_MSG_ID_DEVICE_ADDED
          to check whether the add operation was successful or not.
          In addition to this, client will receive CAP_PROFILE_CLIENT_MSG_ID_INIT_COMPLETE
          if all devices in the group got added.
*/
bool CapProfileClient_AddDeviceToGroup(ServiceHandle group_handle, gatt_cid_t cid);

/*! \brief Complete CAP instance creation with existing devices in the group.
           This function is used when not all members are added into the group but
           still wanted to operate with the existing members.

    \param group_handle  Group handle to which the device needs to be added
    \param cid GATT Connection identifier of the device which needs to be added.

    \note A success return value indicates that a request has been placed
          successfully. Clients should handle CAP_PROFILE_CLIENT_MSG_ID_INIT_COMPLETE
          to check whether the add operation was successful or not.
*/
bool CapProfileClient_CompleteInitWithExistingDevices(ServiceHandle group_handle);

/*! \brief Initiates Codec & QoS configuration

    \param sink_capability Sink capability to be used (bit mask from CapClientSreamCapability)
    \param source_capability Source capability to be used (bit mask from CapClientSreamCapability)
    \param latency Target latency to be used.
    \param use_case Usecase to configure
    \param sink_audio_location Sink audio location
    \param src_audio_location Source audio location
    \param mic_count Number of microphones
    \param cap_config_mode Config mode to be used (default OR Q2Q).
                           ( 0x00 - default,
                             0x02 - joint stereo )

    \return TRUE if able to place a request for configuration.

    \note A success return value indicates that a configuration request has been placed
          successfully. Clients should handle CAP_PROFILE_CLIENT_UNICAST_CONFIG_COMPLETE to check
          whether the configuration was successfully done or not.
*/
void CapProfileClient_Configure(uint32 sink_capability,
                                uint32 source_capability,
                                uint8 latency,
                                uint16 use_case,
                                uint32 sink_audio_location,
                                uint32 src_audio_location,
                                uint8 mic_count,
                                uint8 cap_config_mode);

/*! \brief Initiates Codec & QoS configuration in PTS mode

    \param preferred_audio_context  Preferred audio context to configure
    \param latency Target latency to be used.
    \param cap_config_mode Config mode to be used
                           ( 0x00 - default,
                             0x02 - joint stereo )

    \return TRUE if able to place a request for configuration.

    \note A success return value indicates that a configuration request has been placed
          successfully. Clients should handle CAP_PROFILE_CLIENT_UNICAST_CONFIG_COMPLETE to check
          whether the configuration was successfully done or not.
          If available audio contexts indicates only one context, that context will be chosen
          to configure. 'preferred_audio_context' is only considered to configure if the available
          audio contexts indicate multiple contexts as available.
*/
bool CapProfileClient_ConfigurePtsForStreaming(uint16 preferred_audio_context, uint8 latency, uint8 cap_config_mode);

/*! \brief Start Unicast streaming

    \param preferred_audio_context Preferred audio context to stream
    \param ccid_count  Number of CCIDs to use

    \return TRUE if able to place a start request for streaming for the specified context.

   \note A success return value indicates that a streaming request has been placed
          successfully. Clients should handle CAP_PROFILE_CLIENT_UNICAST_STREAM_START to check
          whether the streaming has successfully started or not.
          If available audio contexts indicates only one context, that context will be chosen
          to stream. 'preferred_audio_context' is only considered to stream if the available
          audio contexts indicate multiple contexts as available.
*/
bool CapProfileClient_StartPtsUnicastStreaming(uint16 preferred_audio_context, uint8 ccid_count);

/*! \brief Update the audio streaming

    \param use_case Preferred audio context to stream
    \param ccid_count  Number of CCIDs to use

    \return TRUE if able to place a update request for streaming for the specified context.
*/
void CapProfileClient_UpdateAudioStreaming(uint16 use_case, uint8 ccid_count);

/*! \brief Stop Unicast streaming in PTS mode

    \param release If TRUE, CAP will release the configurations done earlier.

    \return TRUE if able to place a stop request for streaming

    \note A success return value indicates that a stop streaming request has been placed
          successfully. Clients should handle CAP_PROFILE_CLIENT_UNICAST_STREAM_STOP to check
          whether the streaming has successfully stopped or not.
*/
bool CapProfileClient_StopPtsUnicastStreaming(bool release);

/*! \brief Read the Volume state characteristics of the given cid
    \param cid GATT Connection identifier to read the volume state
*/
void CapProfileClient_ReadVolumeState(gatt_cid_t cid);

/*! \brief Add specified broadcast source to the assistant.

    \param cid  connection id
    \param pa_sync_state
    \param bis_index
    \param is_collocated

    \return TRUE if able to place a request to add broadcast source
*/
bool CapProfileClient_BroadcastAsstAddSource(uint32 cid,
                                             CapClientPaSyncState pa_sync_state,
                                             uint32 bis_index,
                                             bool is_collocated);

/*! \brief Modify specified existing broadcast source.

    \param cid  connection id
    \param pa_sync_state
    \param bis_index

    \return TRUE if able to place a request to modify broadcast source
*/
bool CapProfileClient_BroadcastAsstModifySource(uint32 cid,
                                                CapClientPaSyncState pa_sync_state,
                                                uint32 bis_index,
                                                bool is_collocated);

/*! \brief Remove specified broadcast source from the assistant.

    \param cid  connection id

    \return TRUE if able to place a request to remove broadcast source
*/
bool CapProfileClient_BroadcastAsstRemoveSource(uint32 cid);

/*! \brief Assistant starts scanning for broadcast sources.

    \param cid  connection id
    \param add_collocated  TRUE if we need to scan for collocated source else FALSE.

    \return TRUE if able to place a request to start scanning for broadcast source
*/
bool CapProfileClient_BroadcastAsstStartScan(uint32 cid, bool add_collocated);

/*! \brief Assistant stops scanning for broadcast sources.

    \param cid  connection id

    \return TRUE if able to place a request to stop scanning for broadcast source
*/
bool CapProfileClient_BroadcastAsstStopScan(uint32 cid);

/*! \brief Assistant registers for GATT notifications from broadcast sources.

    \param cid  connection id

    \return TRUE if able to place a request to register for GATT nofification
*/
bool CapProfileClient_BroadcastAsstRegisterForGattNotfn(uint32 cid);

/*! \brief Assistant stops scanning for broadcast sources.

    \param cid  connection id

    \return TRUE if able to place a request to read Broadcast Receive State
*/
bool CapProfileClient_BroadcastAsstReadBrs(uint32 cid);

/*! \brief Starts syncing to the specified broadcast source.

    \param cid  connection_id.
*/
bool CapProfileClient_BroadcastAsstSyncToSrc(uint32 cid);

/*! \brief Sets the Broadcast code

    \param cid             connection_id.
    \param broadcast_code  pointer to broadcast code
*/
bool CapProfileClient_BroadcastAsstSetCode(gatt_cid_t cid, uint8 *broadcast_code);
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

/*! \brief Destroy the CAP profile instance for the given GATT connection identifier
           from the group

    \param group_handle  Group handle on which the connection to destroy belongs.
    \param cid GATT Connection identifier for which to close CAP Profile connection
               If cid given is zero, then all instances in the entire group will get destroyed.
               (ie, group destroy)

    \note A success return value indicates that a destroy instance request has been placed
          successfully.
          Clients will get CAP_PROFILE_CLIENT_MSG_ID_DEVICE_REMOVED message when the specified device gets
          removed. The parameter cid in the message 'CAP_PROFILE_CLIENT_MSG_ID_DEVICE_REMOVED' will
          be zero for group destroy operation.
          In addition, clients will be receiving CAP_PROFILE_CLIENT_MSG_ID_PROFILE_DISCONNECT if all
          instances in the group got destroyed. This is regardless of its a group destroy operation or not.
*/
bool CapProfileClient_DestroyInstance(ServiceHandle group_handle, gatt_cid_t cid);

/*! \brief Check if the context is in available audio contexts.

    \param group_handle  Group handle on which the availability has to be checked.
    \param audio_context context to be checked.

    \return TRUE if audio context available, FALSE otherwise
*/
bool CapProfileClient_IsAudioContextAvailable(ServiceHandle group_handle, uint16 audio_context);

/*! \brief Used to check if CAP is connected or not for given cid

    \param cid  GATT Connection identifier to check if CAP is connected

    \return TRUE if CAP is connected, FALSE otherwise
*/
bool CapProfileClient_IsCapConnectedForCid(gatt_cid_t cid);

/*! \brief Used to check if CAP is connected or not

    \return TRUE if CAP is connected, FALSE otherwise
*/
bool CapProfileClient_IsCapConnected(void);

/*! \brief Get the source audio locations supported by the given CID

    \param group_handle  Group handle

    \return Source audio locations supported
*/
CapClientAudioLocation CapProfileClient_GetSourceAudioLocation(ServiceHandle group_handle);

/*! \brief Used to check if the remote supports specific stream capability

    \param group_handle  Group handle on which to check VS Aptx Lite support
    \param stream_capability  stream capability to check

    \return TRUE if the CAP instance supports requested stream capability, FALSE otherwise
*/
bool CapProfileClient_IsStreamCapabilitySupported(ServiceHandle group_handle, uint32 stream_capability);

/*! \brief Check if the given advertisement data is from a set member

    \param group_handle  Group handle of the coordinated set to check
    \param adv_data Pointer to advertisement data.
    \param adv_data_len Length of the advertisement data

    \return TRUE if the advert is from set member, FALSE otherwise
*/
bool CapProfileClient_IsAdvertFromSetMember(ServiceHandle group_handle, uint8 *adv_data, uint16 adv_data_len);

/*! \brief Get the CIG ID from group handle

    \param group_handle  Group handle

    \return Valid CIG ID if found, 0 otherwise
*/
uint8 CapProfileClient_GetCigId(ServiceHandle group_handle);

/*! Functions added for PTS */

/*! \brief Get the available sink audio contexts in the remote

    \param group_handle  Group handle

    \return Available sink audio contexts
*/
CapClientContext CapProfileClient_GetAvailableSinkAudioContext(ServiceHandle group_handle);

/*! \brief Get the available source audio contexts in the remote

    \param group_handle  Group handle

    \return Available source audio contexts
*/
CapClientContext CapProfileClient_GetAvailableSourceAudioContext(ServiceHandle group_handle);

/*! \brief Read the Volume state characteristics of the given cid
*/
void CapProfileClient_BroadcastSrcInit(void);

/*! \brief Configure broadcast streaming using CAP.

    \param bcast_type Public broadcast type. Either SQ or HQ Public Broadcast

    \return TRUE if able to place a request for broadcast configure
*/
bool CapProfileClient_BroadcastSrcConfigure(const BroadcastType bcast_type);

/*! \brief Start broadcast streaming using CAP.

    \param encryption  TRUE if streaming is encrypted
    \param broadcast_code  Broadcast code that set for the broadcast source role

    \return TRUE if able to place a request to start broadcast streaming
*/
bool CapProfileClient_BroadcastSrcStartStreaming(bool encryption, uint8* broadcast_code);

/*! \brief Stop broadcast streaming using CAP.

    \return TRUE if able to place a request to stop broadcast streaming
*/
bool CapProfileClient_BroadcastSrcStopStreaming(void);


/*! \brief Remove broadcast source using CAP.

    \param context context to which we need to switch

    \return TRUE if able to place a stream update request
*/
bool CapProfileClient_BroadcastSrcUpdateStream(CapClientContext context);

/*! \brief Remove broadcast source using CAP.

    \return TRUE if able to place a stream remove request
*/
bool CapProfileClient_BroadcastSrcRemoveStream(void);

/*! \brief Deinitialise broadcast source using CAP.

    \return TRUE if able to place a broadcast source deinitialization request
*/
bool CapProfileClient_BroadcastSrcDeinit(void);

/*! \brief Gets the speaker configuration

    \param media_config  Speaker configuration information.

    \return TRUE If configuration is present, else FALSE

    \note This function should be used in PTS mode only.
*/
bool CapProfileClient_PtsGetSpeakerPathConfig(cap_profile_client_media_config_t *media_config);

/*! \brief Sets the bd address of non-collocated source that needs to be added.

    \param cid      connection_id.
    \param bd_addr  BD address of the non-collocated source that needs to be added.

    \return TRUE If BD address was succesfully set
*/
bool CapProfileClient_SetDeviceBdAddressToAdd(bdaddr *bd_addr);

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */

#endif /* CAP_PROFILE_CLIENT_H */

/*! @} */