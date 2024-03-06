/*!
\copyright  Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   le_audio_messages_domain LE Audio Messages
\ingroup    bt_domain
\brief      Functions for generating LE Audio notification messages.
*/

#ifndef LE_AUDIO_MESSAGES_H_
#define LE_AUDIO_MESSAGES_H_

#include <message.h>

#include "audio_sources.h"
#include "domain_message.h"
#include "multidevice.h"

/*! @{ */

/*! Invalid Isochronous handle value. */
#define LE_AUDIO_INVALID_ISO_HANDLE         ((uint16) 0xFFFF)

/*! CIS direction. */
#define LE_AUDIO_ISO_DIRECTION_DL           ((uint8) (1u << 0u))      /* Phone to Headset */
#define LE_AUDIO_ISO_DIRECTION_UL           ((uint8) (1u << 1u))      /* Headset to phone */
#define LE_AUDIO_ISO_DIRECTION_BOTH         ((uint8) (LE_AUDIO_ISO_DIRECTION_DL | LE_AUDIO_ISO_DIRECTION_UL))

/*! Messages sent by the LE Audio domain to interested clients. */
enum le_audio_domain_messages
{
    /*! message to inform a LE broadcast audio source is connected */
    LE_AUDIO_BROADCAST_CONNECTED = LE_AUDIO_MESSAGE_BASE,

    /*! message to inform a LE broadcast audio source is disconnected */
    LE_AUDIO_BROADCAST_DISCONNECTED,

    /*! message to send broadcast metadata extracted from PA train */
    LE_AUDIO_BROADCAST_METADATA_PAYLOAD,

    /*! LE audio unicast enable recieved from AG. */
    LE_AUDIO_UNICAST_ENABLED,

    /*! message to inform a CIS for LE Unicast audio got connected */
    LE_AUDIO_UNICAST_CIS_CONNECTED,

    /*! message to inform a CIS for LE Unicast audio got disconnected */
    LE_AUDIO_UNICAST_CIS_DISCONNECTED,

    /*! message to inform a LE Unicast voice source is connected */
    LE_AUDIO_UNICAST_VOICE_CONNECTED,

    /*! message to inform a LE Unicast voice source is disconnected */
    LE_AUDIO_UNICAST_VOICE_DISCONNECTED,

    /*! message to inform a LE Unicast audio source has started streaming */
    LE_AUDIO_UNICAST_MEDIA_CONNECTED,

    /*! message to inform a LE Unicast audio source has stopped streaming */
    LE_AUDIO_UNICAST_MEDIA_DISCONNECTED,

    /*! message to inform a LE Unicast audio source has established data path for CISes*/
    LE_AUDIO_UNICAST_MEDIA_DATA_PATH_READY,
};

/*! Message body for \ref LE_AUDIO_UNICAST_CONNECTED. */
typedef struct
{
    /*! Connected audio source. */
    audio_source_t      audio_source;
} LE_AUDIO_BROADCAST_CONNECTED_T;

typedef LE_AUDIO_BROADCAST_CONNECTED_T LE_AUDIO_BROADCAST_DISCONNECTED_T;

/*! Message body for \ref LE_AUDIO_BROADCAST_METADATA_PAYLOAD. */
typedef struct
{
    /*! Length of metadata */
    uint8 metadata_len;
    /*! Metadata Payload */
    uint8 metadata[1];
} LE_AUDIO_BROADCAST_METADATA_PAYLOAD_T;

/*! Message body for \ref LE_AUDIO_UNICAST_ENABLED. */
typedef struct
{
    /*! Indicates enable is for which side */
    multidevice_side_t  side;
} LE_AUDIO_UNICAST_ENABLED_T;

/*! Message body for \ref LE_AUDIO_UNICAST_CIS_CONNECTED. */
typedef struct
{
    /*! Indicates connect is for which side */
    multidevice_side_t  side;
    /*! CIS identifier for the connected isochronous stream */
    uint8               cis_id;
    /*! CIS direction */
    uint8               cis_dir;
    /*! CIS handle for the connected isochronous stream */
    uint16              cis_handle;
} LE_AUDIO_UNICAST_CIS_CONNECTED_T;

/*! Message body for \ref LE_AUDIO_UNICAST_CIS_DISCONNECTED. */
typedef LE_AUDIO_UNICAST_CIS_CONNECTED_T LE_AUDIO_UNICAST_CIS_DISCONNECTED_T;

/*! Message body for \ref LE_AUDIO_UNICAST_MEDIA_CONNECTED. */
typedef struct
{
    /*! Connected audio source. */
    audio_source_t      audio_source;

    /*! Connected audio context. */
    uint16              audio_context;
} LE_AUDIO_UNICAST_MEDIA_CONNECTED_T;

/*! Message body for \ref LE_AUDIO_UNICAST_MEDIA_DISCONNECTED. */
typedef LE_AUDIO_UNICAST_MEDIA_CONNECTED_T LE_AUDIO_UNICAST_MEDIA_DISCONNECTED_T;

/*! Message body for \ref LE_AUDIO_UNICAST_VOICE_CONNECTED. */
typedef struct
{
    /*! Connected audio source. */
    voice_source_t      voice_source;
} LE_AUDIO_UNICAST_VOICE_CONNECTED_T;

typedef LE_AUDIO_UNICAST_VOICE_CONNECTED_T LE_AUDIO_UNICAST_VOICE_DISCONNECTED_T;

/*! \brief Setup the LE Audio message delivery framework.

    \param init_task Not used.

    \return TRUE
 */
#if (defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST))
bool LeAudioMessages_Init(Task init_task);
#else
#define LeAudioMessages_Init(init_task) (FALSE)
#endif

#if (defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST))
/*! \brief Register task to receive LE Audio messages.

    \param task_to_register
 */
void LeAudioMessages_ClientRegister(Task task_to_register);

/*! \brief Deregister task that registered to receive LE Audio messages.

    \param task_to_deregister
 */
void LeAudioMessages_ClientDeregister(Task task_to_deregister);
#else
#define LeAudioMessages_ClientRegister(task_to_register) ((void) (0))
#endif

#ifdef INCLUDE_LE_AUDIO_BROADCAST
/*! \brief Send an LE Audio broadcast connect/disconnect message.

    \param id           The notification message id
    \param audio_source Audio source identifier.
 */
void LeAudioMessages_SendBroadcastConnectStatus(MessageId id, audio_source_t audio_source);

/*! \brief Send broadcast metadata extracted from PA train to registered clients.

    \param metadata_len metadata length
    \param metadata Pointer to broadcast metadata.
 */
void LeAudioMessages_SendBroadcastMetadata(uint8 metadata_len, uint8 *metadata);
#else
#define LeAudioMessages_SendBroadcastConnectStatus(id, audio_source) ((void) (0))
#define LeAudioMessages_SendBroadcastMetadata(metadata_len, metadata) ((void) (0))
#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST

/*! \brief Send an Remote LE Audio unicast enabled notification message.

    \param side    The multidevice side for which the audio was connected
 */
void LeAudioMessages_SendUnicastEnabled(multidevice_side_t side);

/*! \brief Send an LE Audio unicast voice source connect/disconnect message.

    \param id               The notification message id.
    \param voice_source     Voice source identifier.
 */
void LeAudioMessages_SendUnicastVoiceConnectStatus(MessageId id, voice_source_t voice_source);

/*! \brief Send an LE Audio unicast media streaming status message.

    \param id               The notification message id.
    \param audio_source     Audio source identifier.
    \param audio_context    Connected audio context.

 */
void LeAudioMessages_SendUnicastMediaConnectStatus(MessageId id, audio_source_t audio_source, uint16 audio_context);

/*! \brief Send an LE Audio unicast CIS connected/disconnected message.

    \param id          The notification message id.
    \param side        The multidevice side for which the CIS was connected.
    \param cis_id      Identifier for the connected CIS isochronous stream.
    \param cis_handle  Handle for the connected CIS isochronous stream.
    \param cis_dir     CIS direction.
 */
void LeAudioMessages_SendUnicastCisConnectStatus(MessageId id, multidevice_side_t side, uint8 cis_id, uint16 cis_handle, uint8 cis_dir);

#endif /* INCLUDE_LE_AUDIO_UNICAST */

/*! \brief Send LE broadcast audio connected notification. */
#define LeAudioMessages_SendBroadcastAudioConnected(source) \
    LeAudioMessages_SendBroadcastConnectStatus(LE_AUDIO_BROADCAST_CONNECTED, source)

/*! \brief Send LE broadcast audio disconnected notification. */
#define LeAudioMessages_SendBroadcastAudioDisconnected(source) \
    LeAudioMessages_SendBroadcastConnectStatus(LE_AUDIO_BROADCAST_DISCONNECTED, source)

#ifdef INCLUDE_LE_AUDIO_UNICAST
/*! \brief Send LE Unicast Voice connected notification. */
#define LeAudioMessages_SendUnicastVoiceConnected(source) \
    LeAudioMessages_SendUnicastVoiceConnectStatus(LE_AUDIO_UNICAST_VOICE_CONNECTED, source)

/*! \brief Send LE Unicast Voice disconnected notification. */
#define LeAudioMessages_SendUnicastVoiceDisconnected(source) \
    LeAudioMessages_SendUnicastVoiceConnectStatus(LE_AUDIO_UNICAST_VOICE_DISCONNECTED, source)

/*! \brief Send LE Unicast media connected notification. */
#define LeAudioMessages_SendUnicastMediaConnected(source, audio_context) \
    LeAudioMessages_SendUnicastMediaConnectStatus(LE_AUDIO_UNICAST_MEDIA_CONNECTED, source, audio_context)

/*! \brief Send LE Unicast media disconnected notification. */
#define LeAudioMessages_SendUnicastMediaDisconnected(source, audio_context) \
    LeAudioMessages_SendUnicastMediaConnectStatus(LE_AUDIO_UNICAST_MEDIA_DISCONNECTED, source, audio_context)

/*! \brief Send LE unicast audio CIS connected notification. */
#define LeAudioMessages_SendUnicastAudioCisConnected(side, cis_id, cis_handle, cis_dir) \
    LeAudioMessages_SendUnicastCisConnectStatus(LE_AUDIO_UNICAST_CIS_CONNECTED, side, cis_id, cis_handle, cis_dir)

/*! \brief Send LE unicast audio CIS disconnected notification. */
#define LeAudioMessages_SendUnicastAudioCisDisconnected(side, cis_id, cis_handle) \
    LeAudioMessages_SendUnicastCisConnectStatus(LE_AUDIO_UNICAST_CIS_DISCONNECTED, side, cis_id, cis_handle, 0)

/*! \brief Send LE unicast media data path ready notification. */
#define LeAudioMessages_SendUnicastMediaDataPathReady(source, audio_context) \
    LeAudioMessages_SendUnicastMediaConnectStatus(LE_AUDIO_UNICAST_MEDIA_DATA_PATH_READY, source, audio_context)

#endif /* INCLUDE_LE_AUDIO_UNICAST */

/*! @} */

#endif /* LE_AUDIO_MESSAGES_H_ */
