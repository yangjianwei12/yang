/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup tmap_profile
    \brief      Header file for TMAP Client source for Unicast
    @{
*/

#ifndef TMAP_CLIENT_SOURCE_UNICAST_H
#define TMAP_CLIENT_SOURCE_UNICAST_H

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

#include "bt_types.h"
#include "tmap_client_lib.h"

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
} tmap_media_config_t;

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
} tmap_microphone_config_t;

/*! \brief TMAP Profile Client status codes. */
typedef enum
{
    /*! The requested operation completed successfully */
    TMAP_CLIENT_UNICAST_MSG_STATUS_SUCCESS,

    /*! The requested operation failed to complete */
    TMAP_CLIENT_UNICAST_MSG_STATUS_FAILED,
} tmap_client_unicast_msg_status_t;

/*! \brief Events sent by TMAP profile to other modules. */
typedef enum
{
    /*! Event to inform Codec & Qos configuration completed */
    TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE,

    /*! Event to inform that CIS got connected */
    TMAP_CLIENT_MSG_ID_UNICAST_CIS_CONNECT,

    /*! Event to inform unicast streaming has started */
    TMAP_CLIENT_MSG_ID_UNICAST_STREAM_START,

    /*! Event to inform Unicast streaming has stopped */
    TMAP_CLIENT_MSG_ID_UNICAST_STREAM_STOP,

    /*!  Event to inform configuration is removed */
    TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_REMOVED,
} tmap_client_unicast_msg_id_t;

/*! \brief Data associated with TMAP Profile Codec & Qos configuration */
typedef struct
{
    /*! Group handle for which the Codec & QoS is done */
    ServiceHandle             group_handle;

    /*! Audio context for which the configuration was done */
    uint16                    audio_context;

    /*! Status which tells configuration is done successfully or not */
    tmap_client_unicast_msg_status_t  status;
} TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE_T;

/*! \brief Data associated with TMAP Profile CIS connections */
typedef struct
{
    /*! GATT Connection identifier for which the CIS got connected. */
    gatt_cid_t                      cid;

    /*! Group handle  */
    ServiceHandle                   group_handle;

    /*! CIS Count */
    uint8                           cis_count;

    /*! Isochronous handles information */
    void                            *cis_handles;

    /*! Negotiated codec and QOS configuration */
    void                            *codec_qos_config;

    /*! Status which tells CIS is connected successfully or not */
    tmap_client_unicast_msg_status_t     status;
} TMAP_CLIENT_MSG_ID_UNICAST_CIS_CONNECT_T;

/*! \brief Data associated with TMAP Profile unicast streaming */
typedef struct
{
    /*! Group handle for which the streaming was started. */
    ServiceHandle group_handle;

    /*! Status which tells unicast streaming started or not*/
    tmap_client_unicast_msg_status_t status;
} TMAP_CLIENT_MSG_ID_UNICAST_STREAM_START_T;

/*! \brief Data associated with TMAP Profile Stream stop indication */
typedef TMAP_CLIENT_MSG_ID_UNICAST_STREAM_START_T TMAP_CLIENT_MSG_ID_UNICAST_STREAM_STOP_T;

/*! \brief Data associated with TMAP Profile instance destruction */
typedef struct
{
    /*! Group handle on which the disconnected device belongs */
    ServiceHandle               group_handle;

    /*! Status which tells if TMAP Profile instance is destroyed or not */
    tmap_client_unicast_msg_status_t    status;
} TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_REMOVED_T;

/*! \brief TMAP message body structure */
typedef union
{
    TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE_T     unicast_config_complete;
    TMAP_CLIENT_MSG_ID_UNICAST_CIS_CONNECT_T         unicast_cis_connect;
    TMAP_CLIENT_MSG_ID_UNICAST_STREAM_START_T        unicast_stream_start;
    TMAP_CLIENT_MSG_ID_UNICAST_STREAM_STOP_T         unicast_stream_stop;
    TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_REMOVED_T      unicast_config_removed;
} tmap_client_unicast_message_body_t;

/*! \brief TMAP message structure */
typedef struct
{
    tmap_client_unicast_msg_id_t         id;
    tmap_client_unicast_message_body_t   body;
} tmap_client_unicast_msg_t;

/*! Callback function used when an observer registers with the TMAP Client Profile module. */
typedef void (*tmap_client_source_unicast_callback_handler_t)(const tmap_client_unicast_msg_t *message);

/*! \brief Register a callback function to get messages from TMAP for unicast

    \param handler  Callback handler
*/
void TmapClientSourceUnicast_RegisterCallback(tmap_client_source_unicast_callback_handler_t handler);

/*! \brief Initiates TMAP configuration

    \param group_handle  Group handle on which the configuration to be done.
    \param sink_capability Sink capability to be used for Media/voice Sink (bit mask from CapClientSreamCapability)
    \param source_capability Source capability to be used for Media/voice Source (bit mask from CapClientSreamCapability)
    \param mic_count Number of microphones (0 - No mic, 1 - Single MIC, 2 - Dual MIC).
    \param tmap_cig_config_mode Config mode to be used (default OR Q2Q).
    \param cig_qhs_config CIG related parameters to be used(only for Q2Q).

    \return TRUE if able to place a request for the specified configuration

    \note A success return value indicates that a configuration request has been placed
          successfully. Clients should handle TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_COMPLETE
          to check whether the configuration was successfully done or not.
          If QHS is requested in tmap_cig_config_mode then cigConfig must be populated,
          else it can be NULL.
*/
bool TmapClientSource_Configure(ServiceHandle group_handle,
                                uint32 sink_capability,
                                uint32 source_capability,
                                uint8 mic_count,
                                TmapClientCigConfigMode tmap_cig_config_mode,
                                TmapClientQhsConfig *cig_qhs_config);

/*! \brief Start unicast streaming using TMAP

    \param group_handle  Group handle on which the streaming has to be started.
    \param audio_context  audio context for which the streaming has to be started.

    \return TRUE if able to place a start request for streaming for the specified context.

    NOTE: A success return value indicates that a streaming request has been placed
          successfully. Clients should handle TMAP_CLIENT_MSG_ID_UNICAST_STREAM_START to check
          whether the streaming has successfully started or not.
*/
bool TmapClientSource_StartUnicastStreaming(ServiceHandle group_handle, TmapClientContext audio_context);

/*! \brief Stop unicast streaming using TMAP

    \param group_handle  Group handle on which the streaming has to be stopped.
    \param remove_config TMAP will erase the configurations done earlier for this audio context.

    \return TRUE if able to place a stop request for streaming

    NOTE: A success return value indicates that a stop streaming request has been placed
          successfully. Clients should handle TMAP_CLIENT_MSG_ID_UNICAST_STREAM_STOP to check
          whether the streaming has successfully stopped or not.
*/
bool TmapClientSource_StopUnicastStreaming(ServiceHandle group_handle, bool remove_configured_context);

/*! \brief Remove configuration from TMAP

    \param group_handle Group handle on which configuration to be removed.
    \param use_case  Usecase configuration that needs to be removed.

    \return TRUE if able to place a remove configuration request.

    NOTE: A success return value indicates that a remove configuration request has been placed
          successfully. Clients should handle TMAP_CLIENT_MSG_ID_UNICAST_CONFIG_REMOVED to check
          whether the configuration has successfully removed or not.
*/
bool TmapClientSource_RemoveConfiguration(ServiceHandle group_handle, TmapClientContext use_case);

/*! \brief Gets the speaker configuration

    \param cid          connection on which the speaker configuration is available.
    \param media_config Speaker configuration information.

    \return TRUE If configuration is present, else FALSE

    NOTE: This function should be used in PTS mode only.
*/
bool TmapClientSource_PtsGetSpeakerPathConfig(gatt_cid_t cid, tmap_media_config_t *media_config);

/*! \brief Gets the microphone configuration

    \param cid          connection on which the microphone configuration is available.
    \param mic_config   microphone configuration information.

    \return TRUE If configuration is present, else FALSE

    NOTE: This function should be used in PTS mode only.
*/
bool TmapClientSource_PtsGetMicPathConfig(gatt_cid_t cid, tmap_microphone_config_t *mic_config);

/*! \brief Returns the first connected GATT device from a group.

    \param group_handle  TMAP group handle

    \return GATT connection identifier

    NOTE: This function should be used in PTS mode only.
*/
gatt_cid_t TmapClientSource_PtsGetFirstGattDeviceCid(void);

/*! \brief Returns the Second connected GATT device from a group. if any.

    \param group_handle  TMAP group handle

    \return GATT connection identifier

    NOTE: This function should be used in PTS mode only.
*/
gatt_cid_t TmapClientSource_PtsGetSecondGattDeviceCid(void);

/*! \brief Returns TRUE if speaker is configured for the session

    \param group_handle  TMAP group handle

    \return TRUE/FALSE depending on whether speaker is configued or not.

    NOTE: This function should be used in PTS mode only.
*/
bool TmapClientSource_PtsIsSpkrPresent(void);

/*! \brief Returns TRUE if micrpphone is configured for the session

    \param group_handle  TMAP group handle

    \return TRUE/FALSE depending on whether microphone is configued or not.

    NOTE: This function should be used in PTS mode only.
*/
bool TmapClientSource_PtsIsMicPresent(void);

/*! \brief Initiates Codec & QoS configuration

    \param sink_capability Sink capability to be used for Gaming Sink (bit mask from CapClientSreamCapability)
    \param source_capability Source capability to be used for Gaming Source (bit mask from CapClientSreamCapability)
    \param latency Target latency to be used.
    \param mic_count Number of microphones (0 - No VBC, 1 - Single MIC, 2 - Dual MIC).
    \param cap_config_mode Config mode to be used (default only).

    \return TRUE if able to place a request for configuration.

    NOTE: This function should be used in PTS mode only
*/
void TmapClientSource_PtsConfigureForStreaming(uint32 sink_capability,
                                                uint32 source_capability,
                                                uint16 use_case,
                                                uint8 mic_count,
                                                TmapClientCigConfigMode tmap_config_mode,
                                                uint32 sink_audio_location,
                                                uint32 src_audio_location);

/*! \brief Start the TMAP Streaming in PTS mode.

    NOTE: This function should be used in PTS mode only.
*/
void TmapClientSource_PtsStartUnicastStreaming(TmapClientContext audio_context, int ccid_count, int ccid_type);

/*! \brief Stop the TMAP Streaming in PTS mode.

    NOTE: This function should be used in PTS mode only.
*/
void TmapClientSource_PtsStopUnicastStreaming(bool remove_configured_context);

/*! \brief Get the CIG ID from group handle

    \param group_handle  Group handle

    \return Valid CIG ID if found, 0 otherwise
*/
uint8 TmapProfileClient_GetCigId(ServiceHandle group_handle);

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#endif /* TMAP_CLIENT_SOURCE_UNICAST_H */
/*! @} */