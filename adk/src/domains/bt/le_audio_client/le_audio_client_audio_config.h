/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief   Header file for the LE Audio Client Audio Configurations.
*/

#ifndef LE_AUDIO_CLIENT_AUDIO_CONFIG_H_
#define LE_AUDIO_CLIENT_AUDIO_CONFIG_H_

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

#include "cap_profile_client.h"
#include "tmap_client_source.h"
#include "qualcomm_connection_manager.h"

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

/*! Maximum audio configurations supported at the same time. Since profiles only supports two
 *  configuration partially, keeping the maximum audio configuration as one for now. */
#define LE_AUDIO_CLIENT_MAX_AUDIO_CONFIGS_SUPPORTED    1

#ifdef INCLUDE_LE_APTX_ADAPTIVE
extern const QCOM_CON_MANAGER_CIS_QHS_PARAMS_T aptx_adaptive_cis_qhs_map[];

#define leAudioClient_GetAptXAdaptiveCisQhsMapConfig()  aptx_adaptive_cis_qhs_map

#endif /* INCLUDE_LE_APTX_ADAPTIVE */

/*! \brief Audio Configuration Information */
typedef struct
{
    /*! Target latency */
    uint8           target_latency;

    /*! Stream capability for sink */
    uint32          sink_stream_capability;

    /*! Stream capability for source */
    uint32          source_stream_capability;

    /*! Preferred ISOAL framing */
    uint8           framing;

    /*! PHY from central */
    uint8           phy_ctop;

    /*! PHY from peripheral */
    uint8           phy_ptoc;

    /*! RTN from central to peripheral */
    uint8           rtn_ctop;

    /*! RTN from peripheral to central */
    uint8           rtn_ptoc;
} le_audio_client_audio_config_t;

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

/*! \brief Data associated with LE Audio Client Broadcast config */
typedef struct
{
    /*! QHS required */
    bool   qhs_required;

    /*! \ref CapClientTargetLatency */
    uint8 target_latency;

    /*! Number of broadcast retransmissions */
    uint8 rtn;

    /*! Maximum codec frames per SDU */
    uint8 max_codec_frames_per_sdu;

    /*! \ref TmapClientContext */
    uint16 audio_context;

    /*! \ref CapClientBcastType */
    uint16 broadcast_type;

    /*! Number of BIS/es */
    uint16 number_of_bis;

    /*! Number of audio channels per BIS */
    uint16 no_of_audio_channels_per_bis;

    /*  Maximum SDU size */
    uint16 sdu_size; 

    /*  Maximum Transport Latency */
    uint16 max_latency;

    /*! PHY channel to be used */
    uint16 phy;

    /*! \ref TmapClientStreamCapability */
    uint32 broadcast_stream_capability;

    /* SDU Interval */
    uint32 sdu_interval;

    /* Presentation Delay */
    uint32 presentation_delay;
} LE_AUDIO_CLIENT_BROADCAST_CONFIG_T;

/*! \brief Structure to hold Broadcast name and encryption code information */
typedef struct
{
    /*! Length of Broadcast source name */
    uint8                                   broadcast_source_name_len;

    /*! Broadcast source name */
    const char                              *broadcast_source_name;

    /*! Broadcast code that has been set for streaming, NULL if unencrypted */
    const uint8                             *broadcast_code;
} LE_AUDIO_CLIENT_BROADCAST_NAME_CODE_T;

/*! \brief Various broadcast audio configuration types */
typedef enum
{
    /*! Broadcast audio configuration type is not known or invalid */
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_INVALID,

    /*! High quality broadcast audio configuration */
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_TYPE_HQ,

    /*! Todo: Add more configurations */
} le_audio_client_broadcast_config_type_t;

/*! \brief Structure to hold audio configuration information for broadcast */
typedef struct
{
    /*! Configuration type to use for broadcast */
    le_audio_client_broadcast_config_type_t config_type;

    /*! Total number of sub groups(non-zero) in the broadcast group */
    uint8 num_sub_group;

    /*! Length of Broadcast source name */
    uint8                                   broadcast_source_name_len;

    /*! Broadcast source name */
    const char                              *broadcast_source_name;

    /*! Broadcast code that has been set for streaming */
    const uint8                             *broadcast_code;

    /*! CapClientBcastType */
    uint16                                  broadcast_type;
    /*! Presentation delay to synchronize the presentation of multiple BISs in a BIG */
    uint32                                  presentation_delay;

    /*! Broadcast ID to use */
    uint32                                  broadcast_id;

    /*! Sub group related information */
    TmapClientBigSubGroup                   *sub_group_info;

    /*! Broadcast config parameters */
    CapClientBcastConfigParam               broadcast_config_params;
} le_audio_client_audio_broadcast_config_t;

/*! \brief Callback interface

    Implemented and registered for LEA configuration updates
*/
typedef struct
{
    /*! \brief Called to retrieve public or private Broadcast audio configuration */
    void (*GetBroadcastAudioConfig)(LE_AUDIO_CLIENT_BROADCAST_CONFIG_T *broadcast_audio_config);

    /*! \brief Called to retrieve broadcast name and encryption code configuration */
    void (*GetBroadcastNameAndEncryptionCode)(LE_AUDIO_CLIENT_BROADCAST_NAME_CODE_T *broadcast_name_code);

    /*! \brief Called to retreive advertising parameters used for broadcast */
    void (*GetBroadcastAdvParams)(CapClientBcastSrcAdvParams **broadcast_adv_params);

    /*! \brief Called to retreive broadcast ID to use for broadcast */
    uint32 (*GetBroadcastId)(void);

} le_audio_client_config_interface_t;

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

/*! \brief Initialise the audio configuration for LE audio client */
void leAudioClient_AudioConfigInit(void);

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

/*! \brief Return audio configuration for the specified audio context and codec type */
const le_audio_client_audio_config_t* leAudioClient_GetAudioConfig(CapClientContext audio_context, uint8 codec);

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

/*! \brief Set the broadcast audio configuration type to use */
void leAudioClient_SetBroadcastAudioConfigType(le_audio_client_broadcast_config_type_t config_type);

/*! \brief Returns the broadcast audio configuration for the curent configuration type  */
const le_audio_client_audio_broadcast_config_t* leAudioClient_GetBroadcastAudioConfig(void);

/*! \brief Returns the broadcast advertisement configuration for the currently set broadcast type  */
void leAudioClient_GetBroadcastAdvConfig(CapClientBcastSrcAdvParams *bcast_adv_settings);

/*! \brief Register callback function to USB dongle app for broadcast audio config from user

    \param broadcast_audio_config_cb  Callback functions.
*/
void LeAudioClient_RegisterConfigInterface(const le_audio_client_config_interface_t * lea_audio_config);

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */

#endif /* LE_AUDIO_CLIENT_AUDIO_CONFIG_H_ */

