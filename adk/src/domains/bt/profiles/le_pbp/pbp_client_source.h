/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   pbp_client_source LE PBP
    @{
    \ingroup    profiles
    \brief      Header file for PBP Client source
*/

#ifndef PBP_CLIENT_SOURCE_H_
#define PBP_CLIENT_SOURCE_H_

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
#include "pbp.h"

#define PBP_MAX_SUPPORTED_BIS    0x2
#define PBP_SCAN_PARAM_DEFAULT            0x0
#define PBP_NUMBER_OF_SINK_INFO_SUPPORTED 0x1
#define PBP_NUMBER_OF_SUBGROUP_SUPPORTED  0x1

/*! \brief PBP Profile Client status codes for Broadcast. */
typedef enum
{
    /*! The requested operation completed successfully */
    PBP_CLIENT_MSG_STATUS_SUCCESS,

    /*! The requested operation failed to complete */
    PBP_CLIENT_MSG_STATUS_FAILED,
} pbp_client_msg_status_t;

/*! \brief Broadcast events sent by PBP profile to other modules. */
typedef enum
{
    /*! Event to inform PBP profile have been initialised */
    PBP_CLIENT_MSG_ID_INIT_COMPLETE,

    /*! Event to inform status of registration with CAP */
    PBP_CLIENT_MSG_ID_REGISTER_CAP_CFM,

    /*! Event to inform PBP broadcast have been configured */
    PBP_CLIENT_MSG_ID_SRC_CONFIG_COMPLETE,

    /*! Event to inform PBP broadcast started the streaming */
    PBP_CLIENT_MSG_ID_SRC_STREAM_START_CFM,

    /*! Event to inform PBP broadcast stopped the streaming */
    PBP_CLIENT_MSG_ID_SRC_STREAM_STOP_CFM,

    /*! Event to inform PBP broadcast have deinitialized */
    PBP_CLIENT_MSG_ID_SRC_DEINIT_COMPLETE,

    /*! Event to inform PBP broadcast stream have updated */
    PBP_CLIENT_MSG_ID_SRC_STREAM_UPDATE,

    /*! Event to inform PBP broadcast stream have removed */
    PBP_CLIENT_MSG_ID_SRC_STREAM_REMOVE,

    /*! Event to inform PBP broadcast parameters have been set */
    PBP_CLIENT_MSG_ID_SRC_BCAST_PARAM_SET,

    /*! Event to inform PBP broadcast assistant has started scanning */
    PBP_CLIENT_MSG_ID_ASST_START_SCAN_CFM,

    /*! Event to inform PBP broadcast assistant with source scanning report */
    PBP_CLIENT_MSG_ID_ASST_SCAN_REPORT,

    /*! Event to inform PBP broadcast assistant has stopped scanning */
    PBP_CLIENT_MSG_ID_ASST_STOP_SCAN_CFM,

    /*! Event to inform PBP broadcast assistant has registered for GATT notifications */
    PBP_CLIENT_MSG_ID_ASST_REGISTER_NOTIFICATION_CFM,

    /*! Event to inform PBP broadcast assistant has added the source */
    PBP_CLIENT_MSG_ID_ASST_ADD_SOURCE_CFM,

    /*! Event to inform PBP broadcast assistant has modified the source */
    PBP_CLIENT_MSG_ID_ASST_MODIFY_SOURCE_CFM,

    /*! Event to inform PBP broadcast assistant has removed the source */
    PBP_CLIENT_MSG_ID_ASST_REMOVE_SOURCE_CFM,

    /*! Event to inform BRS indication notifications */
    PBP_CLIENT_MSG_ID_ASST_BRS_IND,

    /*! Event to inform BRS indication as part of sink receiver state read */
    PBP_CLIENT_MSG_ID_ASST_BRS_READ_IND,

    /*! Event to inform PBP broadcast receiver state read status */
    PBP_CLIENT_MSG_ID_ASST_BRS_READ_CFM
} pbp_client_msg_id_t;

/*! \brief Common data associated with PBP broadcast profile operation */
typedef struct
{
    /*! Profile handle for broadcast source */
    PbpProfileHandle             handle;

    /*! Status which tells if the requested operation is success or not */
    pbp_client_msg_status_t  status;
} PBP_CLIENT_MSG_ID_COMMON_CFM_T;

/*! \brief Data associated with PBP Profile broadcast streaming */
typedef struct
{
    /*! Profile handle for broadcast source */
    PbpProfileHandle             handle;

    /*! Status which tells if the requested operation is success or not */
    pbp_client_msg_status_t  status;

    /*! BIG Identifier */
    uint8                               big_id;

    /*! BIG Sync delay */
    uint32                              big_sync_delay;

    /*! Number of BISes for broadcast stream */
    uint8                               num_bis;

    /* Connection handle of BISes */
    uint16                              bis_handles[PBP_MAX_SUPPORTED_BIS];

    /*! Sampling frequency for broadcast audio streaming */
    uint16                              sampling_frequency;

    /*! Frame duration for broadcast audio streaming */
    uint8                               frame_duration;

    /*! Octects per frame for broadcast audio streaming */
    uint16                              octets_per_frame;
    
    /*! BIG transport latency for the given BIG configuration */
    uint32                            transport_latency_big;

    /*! BIG ISO interval configured for the given BIG configuration */
    uint16                            iso_interval;
} PBP_CLIENT_MSG_ID_SRC_STREAM_START_CFM_T;

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
} PBP_CLIENT_MSG_ID_ASST_SRC_REPORT_IND_T;

typedef struct
{
    /*! Status which tells if the requested operation is success or not */
    pbp_client_msg_status_t  status;

    /*! scan handle */
    uint16                   scan_handle;
} PBP_CLIENT_MSG_ID_ASST_START_SCAN_CFM_T;

typedef struct
{
    /*! Status which tells if the requested operation is success or not */
    pbp_client_msg_status_t  status;

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
} PBP_CLIENT_MSG_ID_ASST_BRS_READ_IND_T;

typedef PBP_CLIENT_MSG_ID_COMMON_CFM_T PBP_CLIENT_MSG_ID_INIT_COMPLETE_T;
typedef PBP_CLIENT_MSG_ID_COMMON_CFM_T PBP_CLIENT_MSG_ID_CAP_REGISTER_CFM_T;
typedef PBP_CLIENT_MSG_ID_COMMON_CFM_T PBP_CLIENT_MSG_ID_SRC_INIT_COMPLETE_T;
typedef PBP_CLIENT_MSG_ID_COMMON_CFM_T PBP_CLIENT_MSG_ID_SRC_CONFIG_COMPLETE_T;
typedef PBP_CLIENT_MSG_ID_COMMON_CFM_T PBP_CLIENT_MSG_ID_SRC_DEINIT_COMPLETE_T;
typedef PBP_CLIENT_MSG_ID_COMMON_CFM_T PBP_CLIENT_MSG_ID_SRC_STREAM_STOP_CFM_T;
typedef PBP_CLIENT_MSG_ID_COMMON_CFM_T PBP_CLIENT_MSG_ID_SRC_UPDATE_STREAM_CFM_T;
typedef PBP_CLIENT_MSG_ID_COMMON_CFM_T PBP_CLIENT_MSG_ID_SRC_REMOVE_STREAM_CFM_T;
typedef PBP_CLIENT_MSG_ID_COMMON_CFM_T PBP_CLIENT_MSG_ID_ASST_STOP_SCAN_CFM_T;
typedef PBP_CLIENT_MSG_ID_COMMON_CFM_T PBP_CLIENT_MSG_ID_ASST_ADD_SRC_CFM_T;
typedef PBP_CLIENT_MSG_ID_COMMON_CFM_T PBP_CLIENT_MSG_ID_ASST_MODIFY_SRC_CFM_T;
typedef PBP_CLIENT_MSG_ID_COMMON_CFM_T PBP_CLIENT_MSG_ID_ASST_REMOVE_SRC_CFM_T;
typedef PBP_CLIENT_MSG_ID_COMMON_CFM_T PBP_CLIENT_MSG_ID_ASST_REGISTER_GATT_NOTFN_CFM_T;
typedef PBP_CLIENT_MSG_ID_COMMON_CFM_T PBP_CLIENT_MSG_ID_ASST_BRS_READ_CFM_T;
typedef PBP_CLIENT_MSG_ID_ASST_BRS_READ_IND_T PBP_CLIENT_MSG_ID_ASST_BRS_IND_T;

/*! \brief PBP message body structure for broadcast*/
typedef union
{
    PBP_CLIENT_MSG_ID_COMMON_CFM_T                    common_cfm;
    PBP_CLIENT_MSG_ID_INIT_COMPLETE_T                 init_complete;
    PBP_CLIENT_MSG_ID_CAP_REGISTER_CFM_T              cap_register_cfm;
    PBP_CLIENT_MSG_ID_SRC_INIT_COMPLETE_T             src_init_complete;
    PBP_CLIENT_MSG_ID_SRC_CONFIG_COMPLETE_T           cfg_complete;
    PBP_CLIENT_MSG_ID_SRC_STREAM_START_CFM_T          stream_start;
    PBP_CLIENT_MSG_ID_SRC_STREAM_STOP_CFM_T           stream_stop;
    PBP_CLIENT_MSG_ID_SRC_DEINIT_COMPLETE_T           deinit_cfm;
    PBP_CLIENT_MSG_ID_SRC_UPDATE_STREAM_CFM_T         src_stream_update;
    PBP_CLIENT_MSG_ID_SRC_REMOVE_STREAM_CFM_T         src_stream_remove;
    PBP_CLIENT_MSG_ID_ASST_START_SCAN_CFM_T           scan_start;
    PBP_CLIENT_MSG_ID_ASST_SRC_REPORT_IND_T           src_scan_report;
    PBP_CLIENT_MSG_ID_ASST_STOP_SCAN_CFM_T            scan_stop;
    PBP_CLIENT_MSG_ID_ASST_ADD_SRC_CFM_T              add_src;
    PBP_CLIENT_MSG_ID_ASST_MODIFY_SRC_CFM_T           modify_src;
    PBP_CLIENT_MSG_ID_ASST_REMOVE_SRC_CFM_T           remove_src;
    PBP_CLIENT_MSG_ID_ASST_REGISTER_GATT_NOTFN_CFM_T  register_gatt_notfn;
    PBP_CLIENT_MSG_ID_ASST_BRS_READ_IND_T             brs_read_ind;
    PBP_CLIENT_MSG_ID_ASST_BRS_READ_CFM_T             brs_read_cfm;
    PBP_CLIENT_MSG_ID_ASST_BRS_IND_T                  brs_ind;
} pbp_client_message_body_t;

/*! \brief PBP message structure for broadcast*/
typedef struct
{
    pbp_client_msg_id_t          id;
    pbp_client_message_body_t    body;
} pbp_client_msg_t;

typedef void (*pbp_client_source_callback_handler_t)(const pbp_client_msg_t *message);

/*! \brief Register a callback function to get messages from PBP

    \param handler  Callback handler

    Note: If any pointer is received as part of the message, the callback handler
          should free the memory after processing it.
*/
void PbpClientSource_RegisterCallback(pbp_client_source_callback_handler_t handler);

/*! \brief Initialise broadcast source using PBP. Clients should handle PBP_CLIENT_MSG_ID_INIT_COMPLETE
           to check whether the instance has been successfully created or not.
*/
void PbpClientSource_Init(void);


/*! \brief Update the  broadcast advertisement settings to use for PBP broadcast.

    \param pbp_client_adv_param  Broadcast advertisement settings to use.
*/
void PbpClientSource_UpdateAdvSetting(const CapClientBcastSrcAdvParams *pbp_client_adv_param);

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
/*! \brief Register profile with CAP.Clients should handle PBP_CLIENT_MSG_ID_REGISTER_CAP_CFM
           to check whether the instance has been successfully created or not

    \param group_handle  cap group handle
*/
void PbpClientSource_RegisterTaskWithCap(ServiceHandle group_handle);
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

/*! \brief Configure broadcast streaming using PBP. Clients should handle
           PBP_CLIENT_MSG_ID_SRC_CONFIG_COMPLETE to check whether the
           configuration has successfully completed or not.

    \param handle  Profile handle on which the streaming has to be configured.
    \param presentation_delay  encryption status for the broadcast streaming.
    \param num_subgroup  Broadcast code that set for the broadcast source role
    \param subgroup_info  contains stream configuration parameters such as number of BIS, metadata, etc.
    \param source_name_len  Length of Broadcast Source name.
    \param source_name  Name of Broadcast.Source.
    \param bcast_type Public broadcast type. Either SQ or HQ Public Broadcast.
    \param encryption TRUE if broadcast source is encrypted
*/
void PbpClientSource_Configure(PbpProfileHandle handle,
                               uint32 presentation_delay,
                               uint8 num_subgroup,
                               const PbpBigSubGroups *subgroup_info,
                               const uint8 source_name_len,
                               const char *source_name,
                               const BroadcastType bcast_type,
                               bool encryption);

/*! \brief Start broadcast streaming using PBP. Clients should handle
           PBP_CLIENT_MSG_ID_SRC_STREAM_START_CFM to check whether the
           streaming has successfully started or not.

    \param handle  Group handle on which the streaming has to be started.
    \param encryption  TRUE if streaming is encrypted
    \param broadcast_code  Broadcast code that set for the broadcast source role
*/
#define PbpClientSource_StartStreaming(handle, encryption, broadcast_code) \
    PbpBroadcastSrcStartStreamReq(handle, encryption, broadcast_code)

/*! \brief Update parameters of ongoing broadcast streaming using PBP. Clients should handle
           PBP_CLIENT_MSG_ID_SRC_STREAM_UPDATE to check whether the stream update
           has successfully completed or not.

    \param handle  Profile handle on which the streaming parameters has to be updated.
    \param use_case  Will be PBP_CONTEXT_TYPE_MEDIA for broadcast streaming
    \param num_subgroup  Num subgroup
    \param metadata_len  Meta data length
    \param metadata  Meta data
*/
#define PbpClientSource_UpdateStreamForSource(handle, use_case, num_subgroup, metadata_len, metadata) \
    PbpBroadcastSrcUpdateAudioReq(handle, use_case, num_subgroup, metadata_len, metadata)

/*! \brief Stop broadcast streaming using PBP. Clients should handle PBP_CLIENT_MSG_ID_SRC_STREAM_STOP_CFM
 *         to check whether the streaming has successfully stopped or not.

    \param handle  Profile handle on which the streaming has to be stopped.
*/
#define PbpClientSource_StopStreaming(handle) PbpBroadcastSrcStopStreamReq(handle)

/*! \brief Remove broadcast source using PBP. Clients should PBP_CLIENT_MSG_ID_SRC_STREAM_REMOVE
           to check whether the source removal has been successfully completed or not.

    \param handle  Profile handle of source which has to be removed.
*/
#define PbpClientSource_RemoveStream(handle) PbpBroadcastSrcRemoveStreamReq(handle)

/*! \brief Deinitialise broadcast source using PBP. Clients should handle PBP_CLIENT_MSG_ID_SRC_DEINIT_COMPLETE
           to check whether the deinitialization has successfully started or not

    \param handle  Profile handle on which the streaming has to be deinitialized.
*/
#define PbpClientSource_Deinit(handle) PbpBroadcastSrcDeinitReq(handle)

/*! \brief Sets the broadcast configuration parameters. Clients should handle PBP_CLIENT_MSG_ID_SRC_BCAST_PARAM_SET
           to print the status of configuration setting operation.

    \param handle  Profile handle.
    \param bcast_config  Broadcast config parameters
*/
#define PbpClientSource_SetBcastConfigParams(handle, bcast_config) \
    PbpBroadcastSrcSetParamReq(handle, PBP_NUMBER_OF_SUBGROUP_SUPPORTED, bcast_config)

/*! \brief Sets the broadcast ID.

    \param handle        Profile handle.
    \param bcast_id      Broadcast ID
*/
#define PbpClientSourceBroadcast_SetBcastId(handle, bcast_id)\
    PbpBroadcastSrcSetBroadcastId(handle, bcast_id)

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
/*! \brief Add specified broadcast source to the assistant.Clients should handle PBP_CLIENT_MSG_ID_ASST_ADD_SOURCE_CFM
           to check whether the source has been successfully added or not.

    \param cap_group_id  group id
    \param cid  connection id
    \param source_taddr  bluetooth address of broadcast source
    \param adv_handle  advertising handle of PA
    \param adv_sid  advertising SID
    \param broadcast_id  Identifier of broadcast source's BIG
    \param bis_index
*/
void PbpClientSource_AddSource(ServiceHandle cap_group_id,
                               uint32 cid,
                               typed_bdaddr source_taddr,
                               uint8 adv_handle,
                               uint8 adv_sid,
                               uint32 broadcast_id,
                               uint32 bis_index);

/*! \brief Modify specified broadcast source thats already on assistant.Clients should handle
           PBP_CLIENT_MSG_ID_ASST_MODIFY_SOURCE_CFM to check whether the source has
           been successfully modified or not.

    \param cap_group_id  group id
    \param cid  connection id
    \param adv_handle  advertising handle of PA
    \param adv_sid  advertising SID
    \param bis_index Bit masked bis index
    \param source_id Source Identifier
    \param pa_sync_enable TRUE to synchronize PA, FALSE to not synchronize
*/
void PbpClientSource_ModifySource(ServiceHandle cap_group_id,
                                  uint32 cid,
                                  uint8 adv_handle,
                                  uint8 adv_sid,
                                  uint32 bis_index,
                                  uint8 source_id,
                                  bool pa_sync_enable);

/*! \brief Remove specified broadcast source from the assistant.Clients should handle
           PBP_CLIENT_MSG_ID_ASST_REMOVE_SOURCE_CFM to check whether the
           source has been successfully removed or not.

    \param cap_group_id  group id
    \param cid  connection id
    \param source_id  Source ID to remove
*/
void PbpClientSource_RemoveSource(ServiceHandle cap_group_id, uint32 cid, uint8 source_id);

/*! \brief Assistant starts scanning for broadcast sources. Clients should get collocated source reports as message
           PBP_CLIENT_MSG_ID_ASST_BRS_IND. Clients should also handle PBP_CLIENT_MSG_ID_ASST_START_SCAN_CFM
           to check whether the assistant has successfully started scanning or not.

    \param cap_group_id  group id
    \param cid  connection id
    \param bcast_type  Public broadcast type. Either SQ or HQ Public Broadcast.
    \param audio_context  broadcast audio context.
*/
void PbpClientSource_StartScanningForSource(ServiceHandle cap_group_id, uint32 cid,
                                            PbpBcastType bcast_type, CapClientContext audio_context);

/*! \brief Assistant stops scanning for broadcast sources. Clients should handle
           PBP_CLIENT_MSG_ID_ASST_STOP_SCAN_CFM to check whether the
           assistant has successfully stopped scanning or not.

    \param cap_group_id  group id
    \param cid  connection id
*/
void PbpClientSource_StopScanningForSource(ServiceHandle cap_group_id, uint32 cid);

/*! \brief Assistant registers for GATT notifications from broadcast sources. Clients should handle
           PBP_CLIENT_MSG_ID_ASST_REGISTER_NOTIFICATION_CFM to check whether the assistant
           has successfully subscribed to notifications or not.

    \param cap_group_id  group id
    \param cid  connection id. If cid is zero notifications will be enabled for all sources.
    \param source_id  Source identifier
*/
void PbpClientSource_RegisterForGattNotification(ServiceHandle cap_group_id, uint32 cid, uint8 source_id);

/*! \brief Assistant stops scanning for broadcast sources. Clients should handle
           PBP_CLIENT_MSG_ID_ASST_BRS_READ_IND(s) and also
           PBP_CLIENT_MSG_ID_ASST_BRS_READ_CFM confirmation message to
           check the status of operation.

    \param cap_group_id  group id
    \param cid  connection id
*/
void PbpClientSource_ReadReceiverSinkState(ServiceHandle cap_group_id, uint32 cid);
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#endif /* PBP_CLIENT_SOURCE_H_ */
/*! @} */