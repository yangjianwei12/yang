/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for LE audio client broadcast router
*/

#ifndef LE_AUDIO_CLIENT_BROADCAST_ROUTER_H
#define LE_AUDIO_CLIENT_BROADCAST_ROUTER_H

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

#include "tmap_client_source_broadcast.h"
#include "pbp_client_source.h"

/*! \brief Maximum number of BIS supported by LE audio client broadcast router */
#define LEA_CLIENT_BCAST_ROUTER_MAX_BIS    TMAP_BROADCAST_MAX_SUPPORTED_BIS

/*! Number of subgroups for broadcast */
#define LE_AUDIO_CLIENT_BROADCAST_NUM_SUB_GROUPS    1

/*! \brief Invalid broadcast source handle */
#define LEA_CLIENT_INVALID_BCAST_SRC_HANDLE    0xFFFF

/*! \brief Macro for creating LE audio client broadcast router messages */
#define MAKE_LEA_CLIENT_INTERNAL_MESSAGE(TYPE) \
    TYPE##_T *message = PanicUnlessNew(TYPE##_T);

/*! \brief List of profiles that LE audio client supports for broadcast operations */
typedef enum
{
    LEA_CLIENT_BCAST_ROUTER_MODE_TMAP,
    LEA_CLIENT_BCAST_ROUTER_MODE_PBP,
}lea_client_bcast_router_mode_t;

/*! \brief LE audio client router status codes */
typedef enum
{
    /*! The requested operation completed successfully */
    LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS,

    /*! The requested operation failed to complete */
    LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_FAILED,
} lea_client_bc_router_msg_status_t;

/*! \brief Broadcast events sent by TMAP profile to other modules. */
#define LE_AUDIO_CLIENT_INTERNAL_BCAST_ROUTER_MESSAGES \
           /*! Event to inform broadcast source have been initialised */ \
           LEA_CLIENT_INTERNAL_BCAST_SRC_INIT_COMPLETE, \
           /*! Event to inform broadcast source have been configured */ \
           LEA_CLIENT_INTERNAL_BCAST_SRC_CONFIG_COMPLETE, \
           /*! Event to inform broadcast source started the streaming */ \
           LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_START_CFM, \
           /*! Event to inform broadcast source stopped the streaming */ \
           LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_STOP_CFM, \
           /*! Event to inform broadcast source have deinitialized */ \
           LEA_CLIENT_INTERNAL_BCAST_SRC_DEINIT_COMPLETE, \
           /*! Event to inform broadcast source stream have updated */ \
           LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_UPDATE, \
           /*! Event to inform broadcast source stream have removed */ \
           LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_REMOVE, \
           /*! Event to inform broadcast assistant has started scanning */ \
           LEA_CLIENT_INTERNAL_BCAST_ASST_START_SCAN_CFM, \
           /*! Event to inform broadcast assistant with source scanning report */ \
           LEA_CLIENT_INTERNAL_BCAST_ASST_SCAN_REPORT, \
           /*! Event to inform broadcast assistant has stopped scanning */ \
           LEA_CLIENT_INTERNAL_BCAST_ASST_STOP_SCAN_CFM, \
           /*! Event to inform broadcast assistant has registered for GATT notifications */ \
           LEA_CLIENT_INTERNAL_BCAST_ASST_REGISTER_NOTIFICATION_CFM, \
           /*! Event to inform broadcast assistant has added the source */ \
           LEA_CLIENT_INTERNAL_BCAST_ASST_ADD_SOURCE_CFM, \
           /*! Event to inform broadcast assistant has modified the source */ \
           LEA_CLIENT_INTERNAL_BCAST_ASST_MODIFY_SOURCE_CFM, \
           /*! Event to inform broadcast assistant has removed the source */ \
           LEA_CLIENT_INTERNAL_BCAST_ASST_REMOVE_SOURCE_CFM, \
           /*! Event to inform BRS indication notifications */ \
           LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_IND, \
           /*! Event to inform BRS indication as part of sink receiver state read */ \
           LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_IND, \
           /*! Event to inform broadcast receiver state read status */ \
           LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_CFM, \
           /*! This must be the last broadcast router related message */ \
           LEA_CLIENT_INTERNAL_BCAST_MAX \

/*! \brief Common data associated with broadcast router operation */
typedef struct
{
    /*! Current LE audio client router mode in which the operation is performed */
    lea_client_bcast_router_mode_t      mode;

    /*! Status which tells if the requested operation is success or not */
    lea_client_bc_router_msg_status_t   status;
} LEA_CLIENT_BCAST_COMMON_CFM_T;

/*! \brief Data associated with TMAP profile in broadcast router */
typedef struct
{
    /*! Profile handle for the broadcast source */
    TmapClientProfileHandle handle;
}
lea_client_bcast_router_tmap_data_t;

/*! \brief Data associated with PBP profile in broadcast router */
typedef struct
{
    /*! Profile handle for the broadcast source */
    PbpProfileHandle handle;
}
lea_client_bcast_router_pbp_data_t;

/*! \brief Data associated with broadcast router */
typedef struct
{
    /*! Currently active/configured LE audio client router mode */
    lea_client_bcast_router_mode_t          mode;

    /*! Data associated with TMAP profile */
    lea_client_bcast_router_tmap_data_t     tmap_data;

    /*! Data associated with PBP profile */
    lea_client_bcast_router_pbp_data_t      pbp_data;
}
lea_client_bcast_router_data_t;

/*! \brief Data associated with broadcast router streaming operation */
typedef struct
{
    /*! Current LE audio client router mode in which the operation is performed */
    lea_client_bcast_router_mode_t    mode;

    /*! Status which tells if the requested operation is success or not */
    lea_client_bc_router_msg_status_t status;

    /*! BIG Identifier */
    uint8                             big_id;

    /*! BIG Sync delay */
    uint32                            big_sync_delay;

    /*! Number of BISes for broadcast stream */
    uint8                             num_bis;

    /*! Connection handle of BISes */
    uint16                            bis_handles[LEA_CLIENT_BCAST_ROUTER_MAX_BIS];

    /*! Sampling frequency for broadcast audio streaming */
    uint16                            sampling_frequency;

    /*! Frame duration for broadcast audio streaming */
    uint8                             frame_duration;

    /*! Octects per frame for broadcast audio streaming */
    uint16                            octets_per_frame;

    /*! BIG transport latency for the given BIG configuration */
    uint32                            transport_latency_big;

    /*! BIG ISO interval configured for the given BIG configuration */
    uint16                            iso_interval;
}LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_START_CFM_T;

/*! \brief Data associated with broadcast router scan report */
typedef struct
{
    /*! Current LE audio client router mode in which the operation is performed */
    lea_client_bcast_router_mode_t  mode;

    /*! Connection id */
    uint32                          cid;

    /*! BT address of the Broadcast Source State */
    TYPED_BD_ADDR_T                 source_addr;

    /*! Advertising SID */
    uint8                           adv_sid;

    /*! Advertising handle for PA */
    uint8                           adv_handle;

    /*! TRUE if the broadcast source is collocated */
    bool                            collocated;

    /*! Broadcast Identifier */
    uint32                          broadcast_id;

    /*! Number of subgroup */
    uint8                           num_subgroup;

    /*! subgroup info such as bis index, no. of bis, etc.*/
    BapBigSubgroup                  *subgroup_info;
}LEA_CLIENT_INTERNAL_BCAST_ASST_SCAN_REPORT_T;

/*! \brief Data associated with broadcast router scan stop operation */
typedef struct
{
    /*! Current LE audio client router mode in which the operation is performed */
    lea_client_bcast_router_mode_t      mode;

    /*! Status which tells if the requested operation is success or not */
    lea_client_bc_router_msg_status_t   status;

    /*! scan handle */
    uint16                              scan_handle;
}LEA_CLIENT_INTERNAL_BCAST_ASST_START_SCAN_CFM_T;

/*! \brief Data associated with broadcast receiver state indication */
typedef struct
{
    /*! Current LE audio client router mode in which the operation is performed */
    lea_client_bcast_router_mode_t      mode;

    /*! Status which tells if the requested operation is success or not */
    lea_client_bc_router_msg_status_t   status;

    /*! Source ID of the Broadcast Receive State characteristic */
    uint8                               source_id;

    /*! Address of the source */
    typed_bdaddr                        source_address;

    /*! Advertising SID */
    uint8                               adv_sid;

    /*! PA sync state */
    uint8                               pa_sync_state;

    /*! BIG encryption */
    uint8                               big_encryption;

    /*! Broadcast ID */
    uint32                              broadcast_id;
}LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_IND_T;

typedef LEA_CLIENT_BCAST_COMMON_CFM_T LEA_CLIENT_INTERNAL_BCAST_SRC_INIT_COMPLETE_T;
typedef LEA_CLIENT_BCAST_COMMON_CFM_T LEA_CLIENT_INTERNAL_BCAST_SRC_CONFIG_COMPLETE_T;
typedef LEA_CLIENT_BCAST_COMMON_CFM_T LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_STOP_CFM_T;
typedef LEA_CLIENT_BCAST_COMMON_CFM_T LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_REMOVE_T;
typedef LEA_CLIENT_BCAST_COMMON_CFM_T LEA_CLIENT_INTERNAL_BCAST_ASST_STOP_SCAN_CFM_T;
typedef LEA_CLIENT_BCAST_COMMON_CFM_T LEA_CLIENT_INTERNAL_BCAST_ASST_ADD_SOURCE_CFM_T;
typedef LEA_CLIENT_BCAST_COMMON_CFM_T LEA_CLIENT_INTERNAL_BCAST_ASST_MODIFY_SOURCE_CFM_T;
typedef LEA_CLIENT_BCAST_COMMON_CFM_T LEA_CLIENT_INTERNAL_BCAST_ASST_REMOVE_SOURCE_CFM_T;
typedef LEA_CLIENT_BCAST_COMMON_CFM_T LEA_CLIENT_INTERNAL_BCAST_ASST_REGISTER_NOTIFICATION_CFM_T;
typedef LEA_CLIENT_BCAST_COMMON_CFM_T LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_CFM_T;
typedef LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_IND_T LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_IND_T;

/*! \brief Initialise broadcast router
           LE audio client should handle LEA_CLIENT_INTERNAL_BCAST_SRC_INIT_COMPLETE
           to check whether the instance has been successfully created or not.
*/
void leAudioClientBroadcastRouter_Init(void);

/*! \brief Set the broadcast router mode

    \param mode  Mode to set. ie, TMAP or PBP
*/
void leAudioClientBroadcastRouter_SetMode(lea_client_bcast_router_mode_t mode);

/*! \brief Get the broadcast router mode

    \return mode
*/
lea_client_bcast_router_mode_t leAudioClientBroadcastRouter_GetMode(void);

/*! \brief Configure broadcast streaming.
           LE audio client should handle LEA_CLIENT_INTERNAL_BCAST_SRC_CONFIG_COMPLETE
           to check whether the configuration has successfully completed or not.

    \param presentation_delay  encryption status for the broadcast streaming.
    \param num_subgroup  Broadcast code that set for the broadcast source role
    \param subgroup_info  contains stream configuration parameters such as number of BIS, metadata, etc.
    \param source_name_len  Length of Broadcast Source name.
    \param source_name  Name of Broadcast Source. Note that if router is in PBP mode, router will
                        append "_PBP" at the end of source_name
    \param broadcast_type  Valid for only PBP mode
            PBP_HQ_BROADCAST : For High quality broadcast
            PBP_STANDARD_BROADCAST : For standard broadcast
    \bcast_config_params  Broadcast configuration parameters
*/
void leAudioClientBroadcastRouter_Configure(uint32 presentation_delay,
                                            uint8 num_subgroup,
                                            const TmapClientBigSubGroup *subgroup_info,
                                            uint8 source_name_len,
                                            const char *source_name,
                                            uint8 broadcast_type,
                                            const CapClientBcastConfigParam *bcast_config_params);

/*! \brief Start broadcast streaming.
           LE audio client should handle LEA_CLIENT_INTERNAL_BCAST_STREAM_START_CFM
           to check whether the streaming has successfully started or not.

    \param handle  Group handle on which the streaming has to be started.
    \param encryption  TRUE if streaming is encrypted
    \param broadcast_code  Broadcast code that set for the broadcast source role
*/
void leAudioClientBroadcastRouter_StartStreaming(bool  encryption,
                                                 const uint8* broadcast_code);

/*! \brief Stop broadcast streaming.
           LE audio client should handle LEA_CLIENT_INTERNAL_BCAST_STREAM_STOP_CFM
           to check whether the streaming has successfully stopped or not.
*/
void leAudioClientBroadcastRouter_StopStreaming(void);

/*! \brief Remove broadcast source.
           LE audio client should LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_REMOVE
           to check whether the source removal has been successfully completed or not.
*/
void leAudioClientBroadcastRouter_RemoveStream(void);

/*! \brief Updates metadata send over the broadcast periodic advertisements.

    \param metadata_len Length of metadta.
    \param metadata_ltv Metadata in LTV format.
*/
void leAudioClientBroadcastRouter_UpdateMetadataInPeriodicTrain(uint8 metadata_len, uint8* metadata);

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
/*! \brief Assistant starts scanning for broadcast sources.
           LE audio client should get collocated source reports as message
           LEA_CLIENT_INTERNAL_BCAST_ASST_SCAN_REPORT.
           LE audio client should also handle LEA_CLIENT_INTERNAL_BCAST_ASST_START_SCAN_CFM
           to check whether the assistant has successfully started scanning or not.

    \param cap_group_id  group id
    \param cid  connection id
    \param broadcast_type  Valid for only PBP mode
            PBP_HQ_BROADCAST : For High quality broadcast
            PBP_STANDARD_BROADCAST : For standard broadcast
    \param audio_context  broadcast audio context
*/
void leAudioClientBroadcastRouter_StartScanningForSource(ServiceHandle cap_group_id,
                                                         uint32 cid,
                                                         uint8 broadcast_type,
                                                         CapClientContext audio_context);

/*! \brief Assistant stops scanning for broadcast sources. LE audio client should handle
           LEA_CLIENT_INTERNAL_BCAST_ASST_STOP_SCAN_CFM to check whether the
           assistant has successfully stopped scanning or not.

    \param cap_group_id  group id
    \param cid  connection id
*/
void leAudioClientBroadcastRouter_StopScanningForSource(ServiceHandle cap_group_id, uint32 cid);

/*! \brief Add specified broadcast source to the assistant.
           LE audio client should handle LEA_CLIENT_INTERNAL_BCAST_ASST_ADD_SOURCE_CFM
           to check whether the source has been successfully added or not.

    \param cap_group_id  group id
    \param cid  connection id
    \param source_taddr  bluetooth address of broadcast source
    \param adv_handle  advertising handle of PA
    \param adv_sid  advertising SID
    \param broadcast_id  Identifier of broadcast source's BIG
    \param bis_index
*/
void leAudioClientBroadcastRouter_AddSource(ServiceHandle cap_group_id,
                                         uint32 cid,
                                         typed_bdaddr source_taddr,
                                         uint8 adv_handle,
                                         uint8 adv_sid,
                                         uint32 broadcast_id,
                                         uint32 bis_index);

/*! \brief Modify specified broadcast source
           LE audio client should handle LEA_CLIENT_INTERNAL_BCAST_ASST_MODIFY_SOURCE_CFM
           to check whether the source has been successfully modified or not.

    \param cap_group_id  group id
    \param cid  connection id
    \param adv_handle  advertising handle of PA
    \param adv_sid  advertising SID
    \param bis_index Bitmasked bit index
    \param source_id Source identifier
    \param pa_sync_enable TRUE to synchronize PA, FALSE to not synchronize
*/
void leAudioClientBroadcastRouter_ModifySource(ServiceHandle cap_group_id,
                                               uint32 cid,
                                               uint8 adv_handle,
                                               uint8 adv_sid,
                                               uint32 bis_index,
                                               uint8 source_id,
                                               bool pa_sync_enable);

/*! \brief Remove specified broadcast source from the assistant.
           LE audio client should handle LEA_CLIENT_INTERNAL_BCAST_ASST_REMOVE_SOURCE_CFM
           to check whether the source has been successfully removed or not.

    \param cap_group_id  group id
    \param cid  connection id
    \param source_id  Source ID to remove
*/
void leAudioClientBroadcastRouter_RemoveSource(ServiceHandle cap_group_id,
                                               uint32 cid,
                                               uint8 source_id);

/*! \brief Assistant registers for GATT notifications from broadcast sources.
           LE audio client should handle LEA_CLIENT_INTERNAL_BCAST_ASST_REGISTER_NOTIFICATION_CFM
           to check whether the assistant has successfully subscribed to notifications or not.

    \param cap_group_id  group id
    \param cid  connection id. If cid is zero notifications will be enabled for all sources.
    \param source_id  Source identifier
*/
void leAudioClientBroadcastRouter_RegisterForGattNotification(ServiceHandle cap_group_id,
                                                              uint32 cid,
                                                              uint8 source_id);

/*! \brief Assistant stops scanning for broadcast sources.
           LE audio client should handle LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_IND(s)
           and also LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_CFM confirmation message to
           check the status of operation.

    \param cap_group_id  group id
    \param cid  connection id
*/
void leAudioClientBroadcastRouter_ReadReceiverSinkState(ServiceHandle cap_group_id, uint32 cid);
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
/*! \brief Update the periodic advertising settings for broadcast.

    \param bcast_adv_settings  Broadcast advertisement settings to use.
*/
void leAudioClientBroadcastRouter_UpdateAdvSettings(const CapClientBcastSrcAdvParams *bcast_adv_settings);

/*! \brief Update the Broadcast ID for broadcast.

    \param bcast_id  Broadcast ID to use.
*/
void leAudioClientBroadcastRouter_SetBroadcastId(uint32 bcast_id);

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
#endif /* LE_AUDIO_CLIENT_BROADCAST_ROUTER_H */
