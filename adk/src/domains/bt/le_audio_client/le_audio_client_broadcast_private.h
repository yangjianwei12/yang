/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Private types and functions for LE Audio Client broadcast
*/

#ifndef LE_AUDIO_CLIENT_BROADCAST_PRIVATE_H_
#define LE_AUDIO_CLIENT_BROADCAST_PRIVATE_H_

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

#include "le_audio_client_broadcast_router.h"
#include "le_audio_client_audio_config.h"

typedef struct
{
    /*! use case for the active session */
    uint16                                          audio_context;

    /*! Audio configuration using for broadcast */
    const le_audio_client_audio_broadcast_config_t *audio_config;

    /*! If TRUE, release all the underlying configuration */
    bool                                            release_config;

    /*! Sampling frequency for broadcast audio streaming */
    uint16                                          sampling_frequency;

    /*! Frame duration for broadcast audio streaming */
    uint8                                           frame_duration;

    /*! Octects per frame for broadcast audio streaming */
    uint16                                          octets_per_frame;

    /*! BIS handles for broadcast audio streaming */
    uint16                                          bis_handles[TMAP_BROADCAST_MAX_SUPPORTED_BIS];

    /* BIG transport latency reported for the given broadcast session */
    uint32                                          transport_latency_big;

    /* ISO interval configured for the given broadcast session */
    uint16                                          iso_interval;
} le_audio_broadcast_session_data_t;

/*! \brief Information related to the source adding to the assistant */
typedef struct
{
    /*! Flag to denote whether current source is pa synced */
    bool             is_current_source_pa_synced;

    /*! Flag to denote whether we need to remove/modify the source before adding new one */
    bool             is_current_source_to_be_removed;

    /*! Flag to denote source have been added to assistant or not */
    bool             is_source_added_to_assistant;

    /*! Advertising SID */
    uint8            adv_sid;

    /*! Advertising handle for PA */
    uint8            adv_handle;

    /*! Currently active source ID */
    uint8            source_id;

    /*! Broadcast Identifier */
    uint32           broadcast_id;

    /*! Bitmasked bis_index */
    uint32           bis_index;
} le_audio_broadcast_asst_src_param_t;

/*! \brief Initialise LE audio client broadcast */
void leAudioClientBroadcast_Init(void);

/*! \brief Start broadcast streaming */
bool leAudioClientBroadcast_StartStreaming(uint16 audio_context);

/*! \brief Stop broadcast streaming */
bool leAudioClientBroadcast_StopStreaming(bool remove_config);

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

/*! \brief Add the source to assistant if not added already */
void leAudioClientBroadcast_AddSourceToAssistant(void);

#else /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#define leAudioClientBroadcast_AddSourceToAssistant()

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

/*! \brief Store broadcast streaming Parameters*/
void leAudioClientBroadcast_StoreAudioParams(const LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_START_CFM_T *stream_params);

/*! Get the Sample rate from BAP sampling frequency */
uint16 leAudioClientBroadcast_GetSampleRate(uint16 bap_sampling_freq);

/*! \brief Resets all information related to broadcast source */
void leAudioClientBroadcast_ResetSourceContext(void);

/*! \brief Process the message if message is from broadcast router. Return TRUE if message
           is processed. FALSE otherwise
 */
bool leAudioClientBroadcast_ProcessMsgIfFromBcastRouter(MessageId id, Message message);

#else /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#define leAudioClientBroadcast_Init()
#define leAudioClientBroadcast_ResetSourceContext()
#define leAudioClientBroadcast_AddSourceToAssistant()
#define leAudioClientBroadcast_ProcessMsgIfFromBcastRouter(id, message) (FALSE)

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#endif /* LE_AUDIO_CLIENT_BROADCAST_PRIVATE_H_ */

