/*!
    \copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup tmap_profile
    \brief
    @{
*/

#ifndef TMAP_SERVER_ROLE_H_
#define TMAP_SERVER_ROLE_H_

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
#include "gatt_telephone_bearer_server.h"
#include "gatt_mcs_server.h"
#include "domain_message.h"

/*! \brief Control comamands received from remote side. These will be sending to clients registered
     using LeTmapServer_RegisterForRemoteCallControls() */
typedef enum
{
    /*! Indicates call accept received from remote device */
    TMAP_SERVER_REMOTE_CALL_CONTROL_CALL_ACCEPT = TMAP_SERVER_MESSAGE_BASE,

    /*! Indicates call terminate received from remote device */
    TMAP_SERVER_REMOTE_CALL_CONTROL_CALL_TERMINATE,

    /*! This must be the final message */
    TMAP_SERVER_MESSAGE_END,
} tmap_profile_remote_call_control_msg_t;

#endif

/*! \brief Initialize TMAP server
 */
void LeTmapServer_Init(void);

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

/*! \brief Register with TMAP to receive call controls received from remote device

    \param  task  The task being registered to receive call controls.
 */
void LeTmapServer_RegisterForRemoteCallControls(Task task);

/*! \brief Create a new call
    \param call_state  Initial state of the call
    \param call_flags  Call flags
    \return TRUE if success, FALSE otherwise
*/
bool LeTmapServer_CreateCall(uint8 call_state, uint8 call_flags);

/*! \brief Terminate the call
    \param reason  Reason for termination
    \return TRUE if success, FALSE otherwise
*/
bool LeTmapServer_TerminateCall(uint8 reason);

/*! \brief Check if any call is present
    \return TRUE if call is present, FALSE otherwise
*/
bool LeTmapServer_IsCallPresent(void);

/*! \brief Check if call is in active state or not
    \return TRUE if call is active, FALSE otherwise
*/
bool LeTmapServer_IsCallInActiveState(void);

/*! \brief  Get the call state of current call
    \return Returns the call state. If call is not found, then TBS_CALL_STATE_INVALID
            will be returned.
*/
GattTbsCallStates LeTmapServer_GetCallState(void);

/*! \brief Sets the current call state in TBS server
    \param call_state  New state of the call to set
    \return TRUE if success, FALSE otherwise
*/
bool LeTmapServer_SetCallState(GattTbsCallStates state);

/*! \brief  Get the current media state
    \return Returns the media state.
*/
GattMcsMediaStateType LeTmapServer_GetCurrentMediaState(void);

/*! \brief  Set the current media state
    \param  media_state  New media state to set
    \return TRUE if success, FALSE otherwise
*/
bool LeTmapServer_SetMediaState(GattMcsMediaStateType media_state);

/*! Function added for PTS testing */

/*! \brief Sets the default TBS provider name
    \return TRUE if success, FALSE otherwise
*/
bool LeTmapServer_SetDefaultProviderName(void);

/*! \brief Sets the TBS technology used
    \param technology  Technology to set
    \return TRUE if success, FALSE otherwise
*/
bool LeTmapServer_SetTechnology(uint8 technology);

/*! \brief Sets the default URI prefix list
    \return TRUE if able to set the URI prefix, FALSE otherwise
*/
bool LeTmapServer_SetDefaultUriPrefixList(void);

/*! \brief Sets the status flags in the TBS server
    \param status_flags  Flag value to set
           Bit 0 : Inband ringtone enable/disable
           Bit 1 : Silent mode enable/disable
           Bit 2-15: RFU
    \return TRUE if success, FALSE otherwise
*/
bool LeTmapServer_SetStatusFlags(uint16 flags);

/*! \brief Set valid URI flag
    \param is_valid  TRUE to set valid URI, FALSE otherwise
*/
void LeTmapServer_SetValidUriFlag(bool is_valid);

/*! \brief Enable/Disable PTS mode
    \param enable  TRUE to enable, FALSE otherwise
*/
void LeTmapServer_EnablePtsMode(bool enable);

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
#endif /* TMAP_SERVER_ROLE_H_ */
/*! @} */