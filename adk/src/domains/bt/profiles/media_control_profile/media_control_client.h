/*!
    \copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   media_control_client Media Control Client
    @{
    \ingroup    profiles
    \brief      Header file for Media control profile client
*/

#ifndef MEDIA_CONTROL_CLIENT_H
#define MEDIA_CONTROL_CLIENT_H

#ifdef USE_SYNERGY

#include "mcp_mcs_common.h"
#include "bt_types.h"
#include "audio_sources.h"

/*! \brief Initialise the Media control client component
 */
void MediaControlClient_Init(void);

/*! \brief Function to send the MCS Opcode to GMCS server.
    \param[in] cid    GATT Connection id to which the play/pause opcode to send.If connection
                      id is invalid, this function will send the play/pause opcode to all 
                      connected media servers.
    \param[in] op   - MCS client control opcode to be sent to remote MCS server.
    \param[in] val  - value to be associated with the opcode(not all opcodes will have value).
    \return
 */
void MediaControlClient_SendMediaControlOpcode(gatt_cid_t cid, GattMcsOpcode op, int32 val);

/*! \brief Funtion which returns the audio provider context based on the current media state.
    \return Audio provider context
*/
audio_source_provider_context_t MediaClientControl_GetAudioSourceContext(gatt_cid_t cid);

/*! \brief Checks the local media state and sends either Play/Pause opcode to remote server.
    \param[in] cid  GATT Connection id on which the play/pause will be applied.If connection
                    id is invalid, this function will send the play/pause opcode to  all 
                    connected media servers.
    \return
 */
void MediaControlClient_TogglePlayPause(gatt_cid_t cid);

/*! \brief Read the media player attribute.
    \param[in] cid    GATT Connection id on which the attribute has to be read.
    \param[in] charac Attribute to read.
    \return
 */
void MediaControlClient_GetMediaPlayerAttribute(gatt_cid_t cid, MediaPlayerAttribute charac);

/*! \brief Set the media player attribute.
    \param[in] cid     GATT Connection id on which the attribute has to be set.
    \param[in] charac  Attribute to write to.
    \param[in] len     Len of the value to be written
    \param[in] val     Value to write.
    \return
 */
void MediaControlClient_SetMediaPlayerAttribute(gatt_cid_t cid,
                                                MediaPlayerAttribute charac,
                                                uint16 len,
                                                uint8 *val);

/*! \brief Register for media notifications
    \param[in] cid         GATT Connection id on which the notification has to be enabled.
    \param[in] characType  Bitmask of MCS characteristics 
    \param[in] notif_value Bitmask to enable/disable respective characteristics CCCD.
    \return
 */
void MediaControlClient_RegisterForNotifications(gatt_cid_t cid,
                                                 MediaPlayerAttributeMask characType,
                                                 uint32 notif_value);

/*! \brief Check if the Media control service is available in remote server and is discovered */
void MediaControlClient_ConnectProfile(gatt_cid_t cid);

/*! \brief Get the content control id for media control client
    \param[in] cid    GATT Connection id to which the content control id is to read.

    \return Content control id of the media control client
 */
uint8 MediaClientControl_GetContentControlId(gatt_cid_t cid);

/*! \brief Register the Media control client component with the Device Database Serialiser */
void MediaControlClient_RegisterAsPersistentDeviceDataUser(void);

/*! \brief Return the device for the audio source */
device_t MediaClientControl_GetDeviceForAudioSource (audio_source_t source);

/*! \brief Callback interface defined for the clients to receive MCP state change indications
    \param[in] media_state  Value to indicate new MCP media state
    \param[in] cid  CID value for the connected device
*/
typedef struct
{
    void (*media_state_change_callback)(uint8 media_state, gatt_cid_t cid);

} media_control_client_callback_if;

/*! \brief Register to receive media state change indications
    \param[in] callback_if  Pointer to the structure of type media_control_client_callback_if
 */
void mediaControlClient_RegisterForMediaStateChangeIndications(media_control_client_callback_if * callback_if);

#else /* USE_SYNERGY */

#define MediaControlClient_Init()
#define MediaControlClient_SendMediaControlOpcode(cid, op, val)
#define MediaControlClient_GetMediaPlayerAttribute(cid, charac)
#define MediaControlClient_SetMediaPlayerAttribute(cid, charac, len, val)
#define MediaControlClient_RegisterForNotifications(cid, characType, notif_value)
#define MediaClientControl_GetAudioSourceContext(cid) (0)
#define MediaControlClient_RegisterAsPersistentDeviceDataUser()

#endif /* USE_SYNERGY */

#endif /* MEDIA_CONTROL_CLIENT_H */

/*! @} */
