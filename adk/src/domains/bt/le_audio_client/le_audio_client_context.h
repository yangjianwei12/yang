/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Private types and functions for LE Audio Client.
*/

#ifndef LE_AUDIO_CLIENT_CONTEXT_H_
#define LE_AUDIO_CLIENT_CONTEXT_H_

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

#include "le_audio_client.h"
#include "le_audio_client_messages.h"
#include "le_audio_client_broadcast_private.h"
#include "le_audio_client_volume.h"

#include <device.h>
#include <task_list.h>
#include <logging.h>
#include <panic.h>

#include "system_state.h"
#include "audio_sources.h"
#include "voice_sources.h"
#include "gatt.h"
#include "bap_profile_client.h"
#include "vcp_profile_client.h"
#include "cap_profile_client.h"
#include "tmap_profile.h"

#define LE_AUDIO_CLIENT_MAX_DEVICES_SUPPORTED           (2u)
#define LE_AUDIO_CLIENT_MAX_CIS_PER_DEVICE              (2u)

/*! \brief Invalid CIS handle value. */
#define INVALID_CIS_HANDLE                              0xFFFFu

/*! \brief Invalid group handle value. */
#define INVALID_GROUP_HANDLE                            0u

#define LE_CIS_DIRECTION_DL                             (uint8) (1u << 0u)      /* Dongle to remote */
#define LE_CIS_DIRECTION_UL                             (uint8) (1u << 1u)      /* remote to Dongle */
#define LE_CIS_DIRECTION_BOTH                           (uint8) (LE_CIS_DIRECTION_DL | LE_CIS_DIRECTION_UL)

/*! The maximum time in millisecond to wait for QLL link to be connected between
 *   Source and remote device.
 */
#define leAudioClient_QllConnectTimeout()               (500u)

#define LE_AUDIO_FLUSH_TIMEOUT_MIN_C_TO_P_DEFAULT       0x01u
#define LE_AUDIO_FLUSH_TIMEOUT_MAX_C_TO_P_DEFAULT       0x05u
#define LE_AUDIO_FLUSH_TIMEOUT_MIN_P_TO_C_DEFAULT       0x00u
#define LE_AUDIO_FLUSH_TIMEOUT_MAX_P_TO_C_DEFAULT       0x00u
#define LE_AUDIO_FLUSH_TIMEOUT_TTP_ADJUST_RATE_DEFAULT  0x0au

/*! \brief LE Audio Client state machine states */
typedef enum
{
    /*! Initializing is in progress and not connected to any device */
    LE_AUDIO_CLIENT_STATE_INITIALIZING = 0,

    /*! Initialized and not connected to any device */
    LE_AUDIO_CLIENT_STATE_INITIALIZED,

    /*! Connecting to a CAP GATT Server capable device */
    LE_AUDIO_CLIENT_STATE_CONNECTING,

    /*! Waiting for QLL to connect */
    LE_AUDIO_CLIENT_STATE_WAITING_FOR_QLL_TO_CONNECT,

    /*! Connected to a CAP GATT Server */
    LE_AUDIO_CLIENT_STATE_CONNECTED,

    /*! Disconnecting from a CAP GATT Server */
    LE_AUDIO_CLIENT_STATE_DISCONNECTING,
} le_audio_client_state_t;

/*! \brief Profiles which are supported */
typedef enum
{
    /*! No profile */
    LE_AUDIO_CLIENT_PROFILE_NONE = 0,

    /*! CAP profile */
    LE_AUDIO_CLIENT_PROFILE_CAP,

    /*! TMAP profile */
    LE_AUDIO_CLIENT_PROFILE_TMAP,
} le_audio_client_profiles_t;

/*! Messages used internally only in LE Audio Client.*/
typedef enum
{
    /*! Message indicating QLL did not got connected within specified time */
    LE_AUDIO_CLIENT_INTERNAL_QLL_CONNECT_TIMEOUT,

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
    /*! Messages from from LE audio client broadcast router */
    LE_AUDIO_CLIENT_INTERNAL_BCAST_ROUTER_MESSAGES,
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

    /*! This must be the final message */
    LE_AUDIO_CLIENT_INTERNAL_MAX
} le_audio_client_internal_msg_t;

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

/*! CIS info */
typedef struct
{
    /*! isochronous handle direction */
    uint8                                   direction;

    /*! CIS handle */
    uint16                                  cis_handle;

    /*! configured audio location  */
    uint16                                  audio_location;
} le_audio_client_cis_info_t;

/*! CIS info associated with each connected devices */
typedef struct
{
    /*! GATT connection identifier associated with the cis info */
    gatt_cid_t                              cid;

    /*! total number of CIS that has been established for this session */
    uint8                                   cis_count;

    /*! isochronous handle information for the given cid */
    le_audio_client_cis_info_t              cis_info[LE_AUDIO_CLIENT_MAX_CIS_PER_DEVICE];
} le_audio_client_cis_devices_info_t;

/*! Unicast session data */
typedef struct
{
    /*! Is Voice back channel enabled? */
    bool                                    enable_vbc;

    /*! If TRUE, release all the underlying configuration */
    bool release_config;

    /*! use case for the active session */
    uint16                                  audio_context;

    CapClientAudioConfig                    codec_qos_config;

    const le_audio_client_audio_config_t    *audio_config;

    /*! Devices isochronous handle information */
    le_audio_client_cis_devices_info_t      devices_cis_info[LE_AUDIO_CLIENT_MAX_DEVICES_SUPPORTED];
} le_audio_unicast_session_data_t;

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

/*! \brief LE Audio Client context structure. */
typedef struct
{
    /*! LE Audio client task */
    TaskData                                task_data;

    /*! List of client tasks registered for notifications */
    task_list_t                            *client_tasks;

    /*! LE Audio client present connection state */
    le_audio_client_state_t                 state;

    /*! LE Audio client is streaming or not (either in broadcast or unicast) */
    bool                                    is_streaming;

    /*! Is QHS mode supported for isochronous channels on this GATT link? */
    bool                                    iso_qhs_supported;

    /*! Current mode for LE audio client */
    le_audio_client_mode_t                  mode;

    /*! Current PTS mode */
    uint8                                   pts_mode;

    /*! QHS Level */
    uint8                                   qhs_level;

    /*! Group handle for active operation */
    ServiceHandle                           group_handle;

    /*! Audio contexts requested to configure */
    CapClientContext                        requested_audio_contexts;

    /*! Configured audio contexts */
    CapClientContext                        configured_audio_contexts;

    /*! Callback function to get source data used for PTS mode */
    LeAudioClient_UsbSourceParamCallback    usb_src_param_cb;

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

    /*! GATT Connection Identifier for a active session */
    gatt_cid_t                              gatt_cid;

    /*! Total devices in the group */
    uint8                                   total_devices;

    /*! Number of connected devices in the group */
    uint8                                   connected_devices;

    /*! Data for a active unicast streaming session */
    le_audio_unicast_session_data_t         session_data;

    /*! Array containing GATT cid of all connected members */
    gatt_cid_t                              gatt_cid_list[LE_AUDIO_CLIENT_MAX_DEVICES_SUPPORTED];

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

    /*! Data for a active broadcast streaming session */
    le_audio_broadcast_session_data_t       broadcast_session_data;

    /*! Currently configured audio configuration type for broadcast */
    le_audio_client_broadcast_config_type_t configured_broadcast_config_type;

    /*! TRUE if there is a broadcast name/encryption status change request pending */
    bool                                    is_bcast_config_changed;

    /*! Parameters related to broadcast source */
    le_audio_broadcast_asst_src_param_t     broadcast_asst_src_param;

    /*! Requested router mode */
    lea_client_bcast_router_mode_t          requested_router_mode;

    /*! Data associated with broadcast router */
    lea_client_bcast_router_data_t          router_data;

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

} le_audio_client_context_t;

/*! LE Audio Client Task Data */
extern le_audio_client_context_t le_audio_client_context;

/*! Returns the LE Audio Client context */
#define leAudioClient_GetContext()         (&le_audio_client_context)

#define leAudioClient_IsInPtsMode()        (leAudioClient_GetContext()->pts_mode != LE_AUDIO_CLIENT_PTS_MODE_OFF)

/*! Returns the LE Audio Client task */
#define leAudioClient_GetTask()            (&le_audio_client_context.task_data)

/*! Returns TRUE if the audio context requires microphone */
#define leAudioClient_IsMicrophoneNeededForContext(audio_context) (audio_context == TMAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL || \
                                                                   audio_context == CAP_CLIENT_CONTEXT_TYPE_GAME_WITH_VBC)

/*! Get the LE Audio Client state */
#define LeAudioClient_GetState()   \
    leAudioClient_GetContext()->state

/*! Check if LE audio client is in connected state or not */
#define leAudioClient_IsInConnectedState()   \
    (LeAudioClient_GetState() == LE_AUDIO_CLIENT_STATE_CONNECTED)

/*! Check if LE audio client is not connected */
#define leAudioClient_StateIsNotConnected()   \
    (LeAudioClient_GetState() < LE_AUDIO_CLIENT_STATE_CONNECTED)

/*! Sets the LE Audio Client streaming state */
#define leAudioClient_SetStreamingState()   \
    leAudioClient_GetContext()->is_streaming = TRUE

/*! Clear the LE Audio Client streaming state */
#define leAudioClient_ClearStreamingState()   \
    leAudioClient_GetContext()->is_streaming = FALSE

/*! Check if LE Audio Client is in streaming or not */
#define leAudioClient_IsStreamingEnabled()   \
    leAudioClient_GetContext()->is_streaming

#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
#define leAudioClient_isVSAptxLite(codec_info) ((codec_info.codecId == BAP_CODEC_ID_VENDOR_DEFINED) && \
                                                (codec_info.companyId == BAP_COMPANY_ID_QUALCOMM) && \
                                                (codec_info.vendorCodecId == CAP_CLIENT_VS_CODEC_ID_APTX_LITE))
#else
#define leAudioClient_isVSAptxLite(codec_info) (FALSE)
#endif

#ifdef INCLUDE_LE_APTX_ADAPTIVE
#define leAudioClient_isVSAptxAdaptive(codec_info) ((codec_info.codecId == BAP_CODEC_ID_VENDOR_DEFINED) && \
                                                    (codec_info.companyId == BAP_COMPANY_ID_QUALCOMM) && \
                                                    (codec_info.vendorCodecId == CAP_CLIENT_VS_CODEC_ID_APTX_ADAPTIVE))
#else
#define leAudioClient_isVSAptxAdaptive(codec_info) (FALSE)
#endif

/*! Resets the LE Audio Client context */
void leAudioClient_ResetContext(void);

/*! Set LE audio client state */
void LeAudioClient_SetState(le_audio_client_state_t state);

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

#define leAudioClient_GetFirstConnectedGattCid(client_ctxt)     ((client_ctxt)->gatt_cid)
#define leAudioClient_GetTotalDeviceCount(client_ctxt)          ((client_ctxt)->total_devices)
#define leAudioClient_GetConnectedDeviceCount(client_ctxt)      ((client_ctxt)->connected_devices)

/*! Reset unicast context */
void leAudioClient_ResetUnicastContext(void);

/*! Init unicast context */
void leAudioClient_InitUnicast(void);

/*! Handles unicast related messages */
void leAudioClient_HandleUnicastMessage(MessageId id, Message message);

/*! Check if LE audio client is in unicast streaming or not */
bool leAudioClient_IsInUnicastStreaming(void);

/*! Start unicast streaming */
bool leAudioClient_StartUnicastStreaming(ServiceHandle group_handle, uint16 audio_context);

/*! Stop unicast streaming */
bool leAudioClient_StopUnicastStreaming(ServiceHandle group_handle, bool remove_config);

/*! Get the CIS handle associated with Sink audio location */
uint16 leAudioClient_GetSinkCisHandleForAudioLocation(uint16 audio_location);

/*! Get the CIS handle associated with Source audio location */
uint16 leAudioClient_GetSrcCisHandleForAudioLocation(uint16 audio_location);

/*! Get the unicast iso handles in the given direction (upling/downlink). Also, return the kymera stream type to use
    for streaming. */
appKymeraLeStreamType leAudioClient_GetUnicastIsoHandles(uint8 dir, uint16 *iso_handle, uint16 *iso_handle_right);

/*! Get the current device in use for given source */
device_t leAudioClient_GetDeviceForSource(generic_source_t source);

#else /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#define leAudioClient_GetFirstConnectedGattCid(client_ctxt)     (INVALID_CID)
#define leAudioClient_GetTotalDeviceCount(client_ctxt)          (0)
#define leAudioClient_GetConnectedDeviceCount(client_ctxt)      (0)
#define leAudioClient_ResetUnicastContext()
#define leAudioClient_InitUnicast()
#define leAudioClient_HandleUnicastMessage(id, message)
#define leAudioClient_IsInUnicastStreaming()                    (FALSE)

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

/*! Get the Frame duration in ms */
uint16 leAudioClient_GetFrameDuration(bool is_source, le_audio_client_mode_t  mode, uint8 stream_type);

/*! Get the Sample rate from CAP stream capability */
uint16 leAudioClient_GetSampleRate(uint16 cap_stream_capability);

/*! Derive the codec tobe used for decoding/encoding based on negotiated codec ID */
uint8 leAudioClient_GetCodecType(CapClientAudioConfig cap_client_audio_config);

/*! Calculate the Frame Length based on stream type and codec type */
uint16 leAudioClient_GetFrameLength(uint16 max_sdu_size, uint16 stream_type, uint16 codec_type);

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */

#endif /* LE_AUDIO_CLIENT_CONTEXT_H_ */

