/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   bap_profile_client BAP Profile Client
    @{
    \ingroup    le_bap
    \brief      Header file for BAP Profile client
*/

#ifndef BAP_PROFILE_CLIENT_H
#define BAP_PROFILE_CLIENT_H

#include "bt_types.h"

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

/*! \brief BAP Client status codes. */
typedef enum
{
    /*! The requested operation completed successfully */
    BAP_PROFILE_CLIENT_STATUS_SUCCESS,

    /*! The requested operation failed to complete */
    BAP_PROFILE_CLIENT_STATUS_FAILED,
} bap_profile_client_status_t;

/*! \brief Events sent by BAP profile to other modules. */
typedef enum
{
    /*! Discovered ASCS, PACS & VCS Service and connected */
    BAP_PROFILE_CLIENT_MSG_ID_INIT_COMPLETE,

    /*! Event to inform BAP Profile has been disconnected */
    BAP_PROFILE_CLIENT_MSG_ID_PROFILE_DISCONNECT,

    /*! This must be the final message */
    BAP_PROFILE_CLIENT_MSG_ID_MESSAGE_END
} bap_profile_client_msg_id_t;

/*! \brief Data associated with BAP Profile intialization*/
typedef struct
{
    /*! status which tells if BAP Profile instance is created or not */
    bap_profile_client_status_t    status;

    /*! status which tells if more devices needs to be discovered */
    bool more_devices_needed;
} BAP_PROFILE_CLIENT_INIT_COMPLETE_T;

/*! \brief Data associated with BAP Profile disconnection */
typedef struct
{
    /*! status which tells if BAP Profile instance is destroyed or not */
    bap_profile_client_status_t            status;

    /*! Current number of connected BAP Servers */
    uint8 connected_server_cnt;
} BAP_PROFILE_CLIENT_DISCONNECT_T;

typedef union
{
    BAP_PROFILE_CLIENT_INIT_COMPLETE_T             init_complete;
    BAP_PROFILE_CLIENT_DISCONNECT_T                disconnected;
} bap_profile_client_message_body_t;

typedef struct
{
    bap_profile_client_msg_id_t         id;
    bap_profile_client_message_body_t   body;
} bap_profile_client_msg_t;

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
} bap_media_config_t;

typedef struct
{
    uint16 source_iso_handle;
    uint16 source_iso_handle_right;
    uint16 sample_rate;
    uint16 frame_length;
    uint16 frame_duration;
    uint32 presentation_delay;
    uint8 codec_type;
    uint8 codec_version;
    uint8 codec_frame_blocks_per_sdu;
    uint8 mic_mute_state;
} bap_microphone_config_t;

/*! \brief Callback structure used when an observer registers with the BAP Client module. */
typedef void (*bap_profile_client_callback_handler_t)(const bap_profile_client_msg_t *message);


/*! \brief Creates an instance for the BAP Profile.

    \param cid GATT Connection identifier for which to create BAP Profile connection

    \note A success return value indicates that a create instance request has been placed
          successfully.Client should receive BAP_CLIENT_MSG_ID_INIT_COMPLETE and check if
          instance got successfully created/failed.
*/
bool BapProfileClient_CreateInstance(gatt_cid_t cid, bap_profile_client_callback_handler_t handler);

/*! \brief Destroy the BAP profile instance for the given GATT connection identifier

    \param cid GATT Connection identifier for which to close BAP Profile connection

    \note A success return value indicates that a destroy instance request has been placed
          successfully.
          Clients should receive BAP_CLIENT_MSG_ID_PROFILE_DISCONNECT which informs whether
          the profile got successfully disconnected or Failed.
*/
bool BapProfileClient_DestroyInstance(gatt_cid_t cid);

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
/* Add needed PTS test interfaces below */

/*! \brief Read Audio location characteristics for Sink or Source 

    \param location_type  Sink or Source for which the supported location have to be read.

    \note Send 1 to read Sink Audio locations
          Send 2 to read Source Audio locations
*/
void BapProfileClient_ReadAudioLocation(gatt_cid_t cid, uint8 location_type);

/*! \brief Read Audio Context(Available/Supported) characteristics 

    \param audio_context_type  Available/Supported Audio context to be read.

    \note Send 1 to read Available Audio Context
          Send 2 to read Supported Audio Context
*/
void BapProfileClient_ReadAudioContext(gatt_cid_t cid, uint8 audio_context_type);

/*! \brief Read ASE Information 

    \param ase_id   ASE Id to be read.
    \param ase_type Sink/Source ASE type to be read

    \note   Use 0xFF to read info about all Sink/Source ASE's, else pass the relevant Ase Id
            Send 1 for ase_type to read Sink Ase.
            Send 2 for ase_type to read Source Ase.
*/
void BapProfileClient_ReadAseInfo(gatt_cid_t cid, uint8 ase_id, uint8 ase_type);

/*! \brief Discover Sink/Source capabilties 

    \param record_type  Sink/Source PAC record type

    \note Send 1 to read Sink Record
          Send 2 to read Source Record
*/
void BapProfileClient_DiscoverCapabilityRequest(gatt_cid_t cid, uint8 pac_record_type);

/*! \brief Configure Codec for a Sink ASE

    \param sink_ase_id     Ase ID to do a codec config.
    \param channel_alloc   Channel allocation count.
    \param frame_duration  Frame duration 7.5ms/10ms.
    \param samp_freq       Sampling frequency.
    \param octets_per_frame Octets per codec frame.
    \param target_latency   target_latency
    \param target_phy       phy medium types
*/
void BapProfileClient_ConfigureCodecForSinkAse(gatt_cid_t cid,
                                               uint8 sink_ase_id,
                                               uint8 channel_alloc,
                                               uint8 frame_duration,
                                               uint16 samp_freq,
                                               uint16 octets_per_frame,
                                               uint8 target_latency,
                                               uint8 target_phy);

/*! \brief Configure Codec for a Source ASE

    \param source_ase_id    Ase ID to do a codec config.
    \param channel_alloc    Channel allocation count.
    \param frame_duration   Frame duration 7.5ms/10ms.
    \param samp_freq        Sampling frequency.
    \param octets_per_frame Octets per codec frame.
    \param target_latency   target_latency
    \param target_phy       phy medium type
*/
void BapProfileClient_ConfigureCodecForSourceAse(gatt_cid_t cid,
                                                 uint8 source_ase_id,
                                                 uint8 channel_alloc,
                                                 uint8 frame_duration,
                                                 uint16 samp_freq,
                                                 uint16 octets_per_frame,
                                                 uint8 target_latency,
                                                 uint8 target_phy);

/*! \brief Enable ASE

    \param ase_id            Ase ID to enable
    \param streaming_context context for which the streaming is enabled for.
*/
void BapProfileClient_AseEnableRequest(gatt_cid_t cid, uint8 ase_id, uint32 streaming_context);

/*! \brief Create and Configure CIG.

    \param cis_count          Number of CIS to create in the CIG group
    \param trans_lat_m_to_s   Transport latency from master to slave
    \param trans_lat_s_to_m   Transport latency from slave to master
    \param sdu_int_m_to_s     SDU interval from master to slave
    \param sdu_int_s_to_m     SDU interval from slave to master
    \param maxSduMtoS         Max SDU size from master to slave
    \param maxSduStoM         Max SDU size from slave to master
    \param rtn                Retransmission number.
*/
void BapProfileClient_ConfigureCig(uint8 cis_count,
                                   uint16 trans_lat_m_to_s,
                                   uint16 trans_lat_s_to_m,
                                   uint32 sdu_int_m_to_s,
                                   uint32 sdu_int_s_to_m,
                                   uint16 maxSduMtoS,
                                   uint16 maxSduStoM,
                                   uint8 rtn);

/*! \brief Initiate a QoS config for all the previosuly codec configured ASE's

    \param ase_id             ASE to do QoS configuration
    \param cis_id             Cis Id to be associated with the ASE.
    \param sdu_interval       SDU interval
    \param sdu_size           SDU size
    \param rtn                Retransmission number.
    \param trans_lat          Transport Latency
    \param phy                PHY medium
*/
void BapProfileClient_ConfigureQoSForAse(gatt_cid_t cid,
                                         uint8 ase_id,
                                         uint8 cis_id,
                                         uint32 sdu_interval,
                                         uint16 sdu_size,
                                         uint8 rtn,
                                         uint16 trans_lat,
                                         uint8 phy);

/*! \brief Create a CIS

    \param cis_id             Cis Id for which the CIS to be created.
*/
void BapProfileClient_CreateCis(gatt_cid_t cid, uint8 cis_id);

/*! \brief Create the data path for the CIS

    \param ase_id             Source ASE Id for which to execute Receiver Start/Stop Ready
    \param ready_req          Receiver stop ready/Receiver Stop ready
*/
void BapProfileClient_ExecuteAseReceiverready(gatt_cid_t cid, uint8 ase_id, uint8 ready_req);

/*! \brief Places an ASE disable request for the supplied ASE Id

    \param ase_id           ASE Id for which the disable has to be placed.
*/
void BapProfileClient_DisableAse(gatt_cid_t cid, uint8 ase_id);

/*! \brief Places an ASE release request for the supplied ASE Id

    \param ase_id           ASE Id for which the release has to be placed.
*/
void BapProfileClient_ReleaseAse(gatt_cid_t cid, uint8 ase_id);

/*! \brief Places an update metadata request.

    \param ase_id              ASE for which the metadata has to be updated.
    \param streaming_contexts  Context for the the metadata update request is placed.
*/
void BapProfileClient_UpdateMetadataRequest(gatt_cid_t cid, uint8 ase_id, uint32 streaming_contexts);

/*! \brief Starts scanning for broadcast sources.

    \param cid                 connection_id.
    \param is_collocated       TRUE if scanning for collocated source else FALSE
    \param bd_addr             BD address of the non-collocated source that needs to be added.
*/
bool BapProfileClient_StartScanningForSource(gatt_cid_t cid, bool is_collocated);

/*! \brief Starts syncing to the specified broadcast source.

    \param cid                 connection_id.
*/
bool BapProfileClient_SyncToSource(gatt_cid_t cid);

/*! \brief Stops scanning for broadcast sources.

    \param cid                 connection_id.
*/
bool BapProfileClient_StopScanningForSource(gatt_cid_t cid);

/*! \brief Reads the Broadcast Receive state of broadcast source.

    \param cid                 connection_id.
*/
bool BapProfileClient_ReadBrs(gatt_cid_t cid);

/*! \brief Register for gatt notifications from broadcast assistant.

    \param cid                 connection_id.
*/
bool BapProfileClient_RegisterForGattNotification(gatt_cid_t cid);

/*! \brief Adds broadcast source to the assistnat.

    \param cid                 connection_id.
    \param is_collocated       TRUE if adding a collocated source else FALSE
    \param bis_index           bis_index
    \param pa_sync_state       PA sync state
*/
bool BapProfileClient_AddSource(gatt_cid_t cid, bool is_collocated, uint32 bis_index, uint8 pa_sync_state);

/*! \brief Modifies the existing broadcast source.

    \param cid                 connection_id.
    \param is_collocated       TRUE if modifying a collocated source else FALSE
    \param bis_index           bis_index
    \param pa_sync_state       PA sync state
*/
bool BapProfileClient_ModifySource(gatt_cid_t cid, bool is_collocated, uint32 bis_index, uint8 pa_sync_state);

/*! \brief Removess broadcast source from the assistnat.

    \param cid                 connection_id.
*/
bool BapProfileClient_RemoveSource(gatt_cid_t cid);

/*! \brief Sets the Broadcast code

    \param cid                 connection_id.
    \param broadcast_code      pointer to broadcast code
*/
bool BapProfileClient_AssistantBcastSetCode(gatt_cid_t cid, uint8 *broadcast_code);
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

/*! \brief Create the data path for the CIS

    \param ase_id             ASE Id mapping to this CIS.
    \param cis_id             Cis Id for which the data path to be created
    \param direction          Host to controller OR Controller to host.
*/
void BapProfileClient_CreateDataPathForCis(gatt_cid_t cid, uint8 ase_id, uint8 cis_id, uint8 direction);

/*! \brief Prepares the configuration needed for the remote speaker path(To Air data).

    \param ase_id              Sink Ase from which the configuration has to be extracted for the remote Speaker Path.
    \param channel             Audio channel
    \param frame_duration      configured frame duration.
    \param sample_rate         Sampling rate for which the ASE is configured for.
*/
void BapProfileClient_PopulateSpeakerPathConfig(gatt_cid_t cid, uint8 ase_id, uint32 channel, uint16 frame_duration, uint32 sample_rate);

/*! \brief Prepares the configuration needed for the local speaker path(From Air data).

    \param ase_id              Source Ase from which the configuration has to be extracted for the local Speaker Path.
    \param channel             Audio channel
    \param frame_duration      configured frame duration.
    \param sample_rate         Sampling rate for which the ASE is configured for.
*/
void BapProfileClient_PopulateMicPathConfig(gatt_cid_t cid, uint8 ase_id, uint32 channel, uint16 frame_duration, uint32 sample_rate);

/*! \brief Prepares the configuration needed for the local speaker path(From Air data).

    \param connect_all         Set it to TRUE to connect to maximum supported BAP Servers.
*/
void BapProfileClient_ConnectToAllSupportedBapServers(bool connect_all);

/*! \brief Retrieves the media config for the provided GATT Connection.

    \param cid                  CID on which the media configuration has been done.
    \param media_config         media configuration information.
*/
bool BapProfileClient_GetSpeakerPathConfig(gatt_cid_t cid, bap_media_config_t *media_config);

/*! \brief Retrieves the mic config for the provided GATT Connection.

    \param cid                  CID on which the mic configuration has been done.
    \param mic_config           Mic configuration information.
*/
bool BapProfileClient_GetMicPathConfig(gatt_cid_t cid, bap_microphone_config_t *mic_config);

/*! \brief Sets the bd address of non-collocated source that needs to be added.

    \param cid                 connection_id.
    \param bd_addr             BD address of the non-collocated source that needs to be added.
*/
bool BapProfileClient_SetDeviceBdAddressToAdd(gatt_cid_t cid, BD_ADDR_T *bd_addr);

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */

#endif /*BAP_PROFILE_CLIENT_H */

/*! @} */