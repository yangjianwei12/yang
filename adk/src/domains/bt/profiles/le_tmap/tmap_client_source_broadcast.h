/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup tmap_profile
    \brief      Header file for TMAP Client source for Broadcast
    @{
*/

#ifndef TMAP_CLIENT_SOURCE_BROADCAST_H
#define TMAP_CLIENT_SOURCE_BROADCAST_H

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

#include "bt_types.h"
#include "tmap_client_lib.h"

#define TMAP_BROADCAST_MAX_SUPPORTED_BIS             0x2
#define TMAP_BROADCAST_SCAN_PARAM_DEFAULT            0x0
#define TMAP_BROADCAST_NUMBER_OF_SINK_INFO_SUPPORTED 0x1
#define TMAP_BROADCAST_NUMBER_OF_SUBGROUP_SUPPORTED  0x1

/*! \brief TMAP Profile Client status codes for Broadcast. */
typedef enum
{
    /*! The requested operation completed successfully */
    TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS,

    /*! The requested operation failed to complete */
    TMAP_CLIENT_BROADCAST_MSG_STATUS_FAILED,
} tmap_client_broadcast_msg_status_t;

/*! \brief Broadcast events sent by TMAP profile to other modules. */
typedef enum
{
    /*! Event to inform TMAP broadcast have been initialised */
    TMAP_CLIENT_MSG_ID_BROADCAST_INIT_COMPLETE,

    /*! Event to inform TMAP broadcast have been configured */
    TMAP_CLIENT_MSG_ID_BROADCAST_CONFIG_COMPLETE,

    /*! Event to inform TMAP broadcast started the streaming */
    TMAP_CLIENT_MSG_ID_BROADCAST_STREAM_START_CFM,

    /*! Event to inform TMAP broadcast stopped the streaming */
    TMAP_CLIENT_MSG_ID_BROADCAST_STREAM_STOP_CFM,

    /*! Event to inform TMAP broadcast have deinitialized */
    TMAP_CLIENT_MSG_ID_BROADCAST_DEINIT_COMPLETE,

    /*! Event to inform TMAP broadcast stream have updated */
    TMAP_CLIENT_MSG_ID_BROADCAST_SRC_STREAM_UPDATE,

    /*! Event to inform TMAP broadcast stream have removed */
    TMAP_CLIENT_MSG_ID_BROADCAST_SRC_STREAM_REMOVE,

    /*! Event to inform TMAP broadcast parameters have been set */
    TMAP_CLIENT_MSG_ID_BROADCAST_SRC_BCAST_PARAM_SET,

    /*! Event to inform TMAP broadcast assistant has started scanning */
    TMAP_CLIENT_MSG_ID_BROADCAST_ASST_START_SCAN_CFM,

    /*! Event to inform TMAP broadcast assistant with source scanning report */
    TMAP_CLIENT_MSG_ID_BROADCAST_ASST_SCAN_REPORT,

    /*! Event to inform TMAP broadcast assistant has stopped scanning */
    TMAP_CLIENT_MSG_ID_BROADCAST_ASST_STOP_SCAN_CFM,

    /*! Event to inform TMAP broadcast assistant has registered for GATT notifications */
    TMAP_CLIENT_MSG_ID_BROADCAST_ASST_REGISTER_NOTIFICATION_CFM,

    /*! Event to inform TMAP broadcast assistant has added the source */
    TMAP_CLIENT_MSG_ID_BROADCAST_ASST_ADD_SOURCE_CFM,

    /*! Event to inform TMAP broadcast assistant has modified the source */
    TMAP_CLIENT_MSG_ID_BROADCAST_ASST_MODIFY_SOURCE_CFM,

    /*! Event to inform TMAP broadcast assistant has removed the source */
    TMAP_CLIENT_MSG_ID_BROADCAST_ASST_REMOVE_SOURCE_CFM,

    /*! Event to inform BRS indication notifications */
    TMAP_CLIENT_MSG_ID_BROADCAST_ASST_BRS_IND,

    /*! Event to inform BRS indication as part of sink receiver state read */
    TMAP_CLIENT_MSG_ID_BROADCAST_ASST_BRS_READ_IND,

    /*! Event to inform TMAP broadcast receiver state read status */
    TMAP_CLIENT_MSG_ID_BROADCAST_ASST_BRS_READ_CFM
} tmap_client_broadcast_msg_id_t;

/*! \brief Common data associated with TMAP broadcast profile operation */
typedef struct
{
    /*! Profile handle for broadcast source */
    TmapClientProfileHandle             handle;

    /*! Status which tells if the requested operation is success or not */
    tmap_client_broadcast_msg_status_t  status;
} TMAP_CLIENT_MSG_ID_BROADCAST_COMMON_CFM_T;

/*! \brief Data associated with TMAP Profile broadcast streaming */
typedef struct
{
    /*! Profile handle for broadcast source */
    TmapClientProfileHandle             handle;

    /*! Status which tells if the requested operation is success or not */
    tmap_client_broadcast_msg_status_t  status;

    /*! BIG Identifier */
    uint8                               big_id;

    /*! BIG Sync delay */
    uint32                              big_sync_delay;

    /*! Number of BISes for broadcast stream */
    uint8                               num_bis;

    /* Connection handle of BISes */
    uint16                              bis_handles[TMAP_BROADCAST_MAX_SUPPORTED_BIS];

    /*! Sampling frequency for broadcast audio streaming */
    uint16                              sampling_frequency;

    /*! Frame duration for broadcast audio streaming */
    uint8                               frame_duration;

    /*! Octects per frame for broadcast audio streaming */
    uint16                              octets_per_frame;

    /*! BIG transport latency for the given BIG configuration */
    uint32                              transport_latency_big;
    
    /*! BIG ISO interval configured for the given BIG configuration */
    uint16                            iso_interval;
} TMAP_CLIENT_MSG_ID_BROADCAST_STREAM_START_CFM_T;

typedef struct
{
    /*! Connection id */
    uint32           cid;

    /*! BT address of the Broadcast Source State */
    TYPED_BD_ADDR_T  source_addr;

    /*! Advertising SID */
    uint8            adv_sid;

    /*! Advertising handle for PA */
    uint8            adv_handle;

    /*! TRUE if the broadcast source is collocated */
    bool             collocated;

    /*! Broadcast Identifier */
    uint32           broadcast_id;

    /*! Number of subgroup */
    uint8            num_subgroup;

    /*! subgroup info such as bis index, no. of bis, etc.*/
    BapBigSubgroup   *subgroup_info;
} TMAP_CLIENT_BROADCAST_ASST_SRC_REPORT_IND_T;

typedef struct
{
    /*! Status which tells if the requested operation is success or not */
    tmap_client_broadcast_msg_status_t  status;

    /*! scan handle */
    uint16                              scan_handle;
} TMAP_CLIENT_BROADCAST_ASST_START_SCAN_CFM_T;

typedef struct
{
    /*! Status which tells if the requested operation is success or not */
    tmap_client_broadcast_msg_status_t  status;

    /*! Source ID of the Broadcast Receive State characteristic */
    uint8                       source_id;

    /*! Address of the source */
    typed_bdaddr                source_address;

    /*! Advertising SID */
    uint8                       adv_sid;

    /*! PA sync state */
    uint8                       pa_sync_state;

    /*! BIG encryption */
    uint8                       big_encryption;

    /*! Broadcast ID */
    uint32                      broadcast_id;
} TMAP_CLIENT_BROADCAST_ASST_BRS_READ_IND_T;

typedef TMAP_CLIENT_MSG_ID_BROADCAST_COMMON_CFM_T TMAP_CLIENT_MSG_ID_BROADCAST_INIT_COMPLETE_T;
typedef TMAP_CLIENT_MSG_ID_BROADCAST_COMMON_CFM_T TMAP_CLIENT_MSG_ID_BROADCAST_CONFIG_COMPLETE_T;
typedef TMAP_CLIENT_MSG_ID_BROADCAST_COMMON_CFM_T TMAP_CLIENT_MSG_ID_BROADCAST_DEINIT_COMPLETE_T;
typedef TMAP_CLIENT_MSG_ID_BROADCAST_COMMON_CFM_T TMAP_CLIENT_MSG_ID_BROADCAST_STREAM_STOP_CFM_T;
typedef TMAP_CLIENT_MSG_ID_BROADCAST_COMMON_CFM_T TMAP_CLIENT_BROADCAST_SRC_UPDATE_STREAM_CFM_T;
typedef TMAP_CLIENT_MSG_ID_BROADCAST_COMMON_CFM_T TMAP_CLIENT_BROADCAST_SRC_REMOVE_STREAM_CFM_T;
typedef TMAP_CLIENT_MSG_ID_BROADCAST_COMMON_CFM_T TMAP_CLIENT_BROADCAST_ASST_STOP_SCAN_CFM_T;
typedef TMAP_CLIENT_MSG_ID_BROADCAST_COMMON_CFM_T TMAP_CLIENT_BROADCAST_ASST_ADD_SRC_CFM_T;
typedef TMAP_CLIENT_MSG_ID_BROADCAST_COMMON_CFM_T TMAP_CLIENT_BROADCAST_ASST_MODIFY_SRC_CFM_T;
typedef TMAP_CLIENT_MSG_ID_BROADCAST_COMMON_CFM_T TMAP_CLIENT_BROADCAST_ASST_REMOVE_SRC_CFM_T;
typedef TMAP_CLIENT_MSG_ID_BROADCAST_COMMON_CFM_T TMAP_CLIENT_BROADCAST_ASST_REGISTER_GATT_NOTFN_CFM_T;
typedef TMAP_CLIENT_MSG_ID_BROADCAST_COMMON_CFM_T TMAP_CLIENT_BROADCAST_ASST_BRS_READ_CFM_T;
typedef TMAP_CLIENT_BROADCAST_ASST_BRS_READ_IND_T TMAP_CLIENT_BROADCAST_ASST_BRS_IND_T;

/*! \brief TMAP message body structure for broadcast*/
typedef union
{
    TMAP_CLIENT_MSG_ID_BROADCAST_COMMON_CFM_T             common_cfm;
    TMAP_CLIENT_MSG_ID_BROADCAST_INIT_COMPLETE_T          init_complete;
    TMAP_CLIENT_MSG_ID_BROADCAST_CONFIG_COMPLETE_T        cfg_complete;
    TMAP_CLIENT_MSG_ID_BROADCAST_STREAM_START_CFM_T       stream_start;
    TMAP_CLIENT_MSG_ID_BROADCAST_STREAM_STOP_CFM_T        stream_stop;
    TMAP_CLIENT_MSG_ID_BROADCAST_DEINIT_COMPLETE_T        deinit_cfm;
    TMAP_CLIENT_BROADCAST_SRC_UPDATE_STREAM_CFM_T         src_stream_update;
    TMAP_CLIENT_BROADCAST_SRC_REMOVE_STREAM_CFM_T         src_stream_remove;
    TMAP_CLIENT_BROADCAST_ASST_START_SCAN_CFM_T           scan_start;
    TMAP_CLIENT_BROADCAST_ASST_SRC_REPORT_IND_T           src_scan_report;
    TMAP_CLIENT_BROADCAST_ASST_STOP_SCAN_CFM_T            scan_stop;
    TMAP_CLIENT_BROADCAST_ASST_ADD_SRC_CFM_T              add_src;
    TMAP_CLIENT_BROADCAST_ASST_MODIFY_SRC_CFM_T           modify_src;
    TMAP_CLIENT_BROADCAST_ASST_REMOVE_SRC_CFM_T           remove_src;
    TMAP_CLIENT_BROADCAST_ASST_REGISTER_GATT_NOTFN_CFM_T  register_gatt_notfn;
    TMAP_CLIENT_BROADCAST_ASST_BRS_READ_IND_T             brs_read_ind;
    TMAP_CLIENT_BROADCAST_ASST_BRS_READ_CFM_T             brs_read_cfm;
    TMAP_CLIENT_BROADCAST_ASST_BRS_IND_T                  brs_ind;
} tmap_client_broadcast_message_body_t;

/*! \brief TMAP message structure for broadcast*/
typedef struct
{
    tmap_client_broadcast_msg_id_t          id;
    tmap_client_broadcast_message_body_t    body;
} tmap_client_broadcast_msg_t;

typedef void (*tmap_client_source_broadcast_callback_handler_t)(const tmap_client_broadcast_msg_t *message);

/*! \brief Register a callback function to get messages from TMAP for broadcast

    \param handler  Callback handler

    Note: If any pointer is received as part of the message, the callback handler
          should free the memory after processing it.
*/
void TmapClientSourceBroadcast_RegisterCallback(tmap_client_source_broadcast_callback_handler_t handler);

/*! \brief Initialise broadcast source using TMAP. Clients should handle TMAP_CLIENT_MSG_ID_BROADCAST_INIT_COMPLETE
           to check whether the instance has been successfully created or not.
*/
void TmapClientSourceBroadcast_Init(void);

/*! \brief Configure broadcast streaming using TMAP. Clients should handle TMAP_CLIENT_MSG_ID_BROADCAST_CONFIG_COMPLETE
           to check whether the configuration has successfully completed or not.

    \param handle  Profile handle on which the streaming has to be configured.
    \param presentation_delay  encryption status for the broadcast streaming.
    \param num_subgroup  Broadcast code that set for the broadcast source role
    \param subgroup_info  contains stream configuration parameters such as number of BIS, metadata, etc.
    \param source_name_len  Length of Broadcast Source name.
    \param source_name  Name of Broadcast.Source.
*/
void TmapClientSourceBroadcast_Configure(TmapClientProfileHandle handle,
                                         uint32 presentation_delay,
                                         uint8 num_subgroup,
                                         const TmapClientBigSubGroup *subgroup_info,
                                         const uint8 source_name_len,
                                         const char *source_name);

/*! \brief Start broadcast streaming using TMAP. Clients should handle TMAP_CLIENT_MSG_ID_BROADCAST_STREAM_START_CFM
           to check whether the streaming has successfully started or not.

    \param handle  Group handle on which the streaming has to be started.
    \param encryption  TRUE if streaming is encrypted
    \param broadcast_code  Broadcast code that set for the broadcast source role
*/
#define TmapClientSourceBroadcast_StartStreaming(handle, encryption, broadcast_code) \
    TmapClientBroadcastSrcStartStreamReq(handle, encryption, broadcast_code)

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
/*! \brief Add specified broadcast source to the assistant.Clients should handle TMAP_CLIENT_MSG_ID_BROADCAST_ASST_ADD_SOURCE_CFM
           to check whether the source has been successfully added or not.

    \param cap_group_id  group id
    \param cid  connection id
    \param source_taddr  bluetooth address of broadcast source
    \param adv_handle  advertising handle of PA
    \param adv_sid  advertising SID
    \param broadcast_id  Identifier of broadcast source's BIG
    \param bis_index
*/
void TmapClientSourceBroadcast_AddSource(ServiceHandle cap_group_id,
                                         uint32 cid,
                                         typed_bdaddr source_taddr,
                                         uint8 adv_handle,
                                         uint8 adv_sid,
                                         uint32 broadcast_id,
                                         uint32 bis_index);

/*! \brief Modify specified broadcast source thats already on assistant.Clients should handle
           TMAP_CLIENT_MSG_ID_BROADCAST_ASST_MODIFY_SOURCE_CFM to check whether the source has
           been successfully modified or not.

    \param cap_group_id  group id
    \param cid  connection id
    \param adv_handle  advertising handle of PA
    \param adv_sid  advertising SID
    \param bis_index Bitmasked bis index
    \param pa_sync_enable TRUE to synchronize PA, FALSE to not synchronize
*/
void TmapClientSourceBroadcast_ModifySource(ServiceHandle cap_group_id,
                                            uint32 cid,
                                            uint8 adv_handle,
                                            uint8 adv_sid,
                                            uint32 bis_index,
                                            uint8 source_id,
                                            bool pa_sync_enable);

/*! \brief Remove specified broadcast source from the assistant.Clients should handle
           TMAP_CLIENT_MSG_ID_BROADCAST_ASST_REMOVE_SOURCE_CFM to check whether the
           source has been successfully removed or not.

    \param cap_group_id  group id
    \param cid  connection id
    \param source_id  Source ID to remove
*/
void TmapClientSourceBroadcast_RemoveSource(ServiceHandle cap_group_id, uint32 cid, uint8 source_id);

/*! \brief Assistant starts scanning for broadcast sources. Clients should get collocated source reports as message
           TMAP_CLIENT_MSG_ID_BROADCAST_ASST_SCAN_REPORT. Clients should also handle TMAP_CLIENT_MSG_ID_BROADCAST_ASST_START_SCAN_CFM
           to check whether the assistant has successfully started scanning or not.

    \param cap_group_id  group id
    \param cid  connection id
    \param audio_context  broadcast audio context
*/
void TmapClientSourceBroadcast_StartScanningForSource(ServiceHandle cap_group_id, uint32 cid,
                                                      CapClientContext audio_context);

/*! \brief Assistant stops scanning for broadcast sources. Clients should handle
           TMAP_CLIENT_MSG_ID_BROADCAST_ASST_STOP_SCAN_CFM to check whether the
           assistant has successfully stopped scanning or not.

    \param cap_group_id  group id
    \param cid  connection id
*/
void TmapClientSourceBroadcast_StopScanningForSource(ServiceHandle cap_group_id, uint32 cid);

/*! \brief Assistant registers for GATT notifications from broadcast sources. Clients should handle
           TMAP_CLIENT_MSG_ID_BROADCAST_ASST_REGISTER_NOTIFICATION_CFM to check whether the assistant
           has successfully subscribed to notifications or not.

    \param cap_group_id  group id
    \param cid  connection id. If cid is zero notifications will be enabled for all sources.
    \param source_id  Source identifier
*/
void TmapClientSourceBroadcast_RegisterForGattNotification(ServiceHandle cap_group_id, uint32 cid, uint8 source_id);

/*! \brief Assistant stops scanning for broadcast sources. Clients should handle
           TMAP_CLIENT_MSG_ID_BROADCAST_ASST_BRS_READ_IND(s) and also
           TMAP_CLIENT_MSG_ID_BROADCAST_ASST_BRS_READ_CFM confirmation message to
           check the status of operation.

    \param cap_group_id  group id
    \param cid  connection id
*/
void TmapClientSourceBroadcast_ReadReceiverSinkState(ServiceHandle cap_group_id, uint32 cid);
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

/*! \brief Update parameters of ongoing broadcast streaming using TMAP. Clients should handle
           TMAP_CLIENT_MSG_ID_BROADCAST_SRC_STREAM_UPDATE to check whether the stream update
           has successfully completed or not.

    \param handle  Profile handle on which the streaming parameters has to be updated.
    \param use_case  Will be TMAP_CLIENT_CONTEXT_TYPE_MEDIA for broadcast streaming
    \param num_subgroup  Num subgroup
    \param metadata_len  Meta data length
    \param metadata  Meta data
*/
#define TmapClientSourceBroadcast_UpdateStreamForSource(handle, use_case, num_subgroup, metadata_len, metadata) \
    TmapClientBroadcastSrcUpdateStreamReq(handle, use_case, num_subgroup, metadata_len, metadata)

/*! \brief Stop broadcast streaming using TMAP. Clients should handle TMAP_CLIENT_MSG_ID_BROADCAST_STREAM_STOP_CFM
 *         to check whether the streaming has successfully stopped or not.

    \param handle  Profile handle on which the streaming has to be stopped.
*/
#define TmapClientSourceBroadcast_StopStreaming(handle) TmapClientBroadcastSrcStopStreamReq(handle)

/*! \brief Remove broadcast source using TMAP. Clients should TMAP_CLIENT_MSG_ID_BROADCAST_SRC_STREAM_REMOVE
           to check whether the source removal has been successfully completed or not.

    \param handle  Profile handle of source which has to be removed.
*/
#define TmapClientSourceBroadcast_RemoveStream(handle) TmapClientBroadcastSrcRemoveStreamReq(handle)

/*! \brief Deinitialise broadcast source using TMAP. Clients should handle TMAP_CLIENT_MSG_ID_BROADCAST_DEINIT_COMPLETE
           to check whether the deinitialization has successfully started or not

    \param handle  Profile handle on which the streaming has to be deinitialized.
*/
#define TmapClientSourceBroadcast_Deinit(handle) TmapClientBroadcastSrcDeinitReq(handle)

/*! \brief Sets the broadcast configuration parameters. Clients should handle TMAP_CLIENT_MSG_ID_BROADCAST_SRC_BCAST_PARAM_SET
           to print the status of configuration setting operation.

    \param handle  Profile handle.
    \bcast_config  Broadcast config parameters
*/
#define TmapClientSourceBroadcast_SetBcastConfigParams(handle, bcast_config)\
    TmapClientSetParamsReq(handle, handle, CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN, \
                           CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN, \
                           TMAP_CLIENT_PARAMS_TYPE_BROADCAST_CONFIG, \
                           TMAP_BROADCAST_NUMBER_OF_SUBGROUP_SUPPORTED, bcast_config)

/*! \brief Sets the broadcast ID.

    \param handle        Profile handle.
    \param bcast_id      Broadcast ID
*/
#define TmapClientSourceBroadcast_SetBcastId(handle, bcast_id)\
    TmapClientBroadcastSrcSetBroadcastId(handle, bcast_id)

/*! \brief Update the  broadcast advertisement settings to use for TMAP broadcast.

    \param tmap_client_adv_param  Broadcast advertisement settings to use.
*/
void TmapClientSource_UpdateAdvSetting(const CapClientBcastSrcAdvParams *tmap_client_adv_param);

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#endif /* TMAP_CLIENT_SOURCE_BROADCAST_H */
/*! @} */