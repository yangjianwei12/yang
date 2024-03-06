/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup tmap_profile
    \brief      Private defines for MCS and TBS server
    @{
*/

#ifndef TMAP_PROFILE_MCS_TBS_PRIVATE_H
#define TMAP_PROFILE_MCS_TBS_PRIVATE_H

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
#include <logging.h>
#include <synergy.h>
#include <task_list.h>
#include "bt_types.h"
#include <gatt_handler_db_if.h>
#include "gatt_mcs_server.h"
#include "gatt_telephone_bearer_server.h"

#define TMAP_INVALID_SERVICE_HANDLE ((ServiceHandle)(0x0000))

/*! Default values to use in MCS server */
#define TMAP_PROFILE_MCS_SERVER_MEDIA_PLAYER_NAME         "generic"
#define TMAP_PROFILE_MCS_SERVER_MEDIA_PLAYER_NAME_LEN     sizeof(TMAP_PROFILE_MCS_SERVER_MEDIA_PLAYER_NAME)
#define TMAP_PROFILE_MCS_SERVER_TRACK_TITLE               ""
#define TMAP_PROFILE_MCS_SERVER_TRACK_TITLE_LEN           sizeof(TMAP_PROFILE_MCS_SERVER_TRACK_TITLE)
#define TMAP_PROFILE_MCS_SERVER_SUPPORTED_OPCODES         (MCS_SUPPORTED_OPCODE_PLAY            |  \
                                                           MCS_SUPPORTED_OPCODE_PAUSE           |  \
                                                           MCS_SUPPORTED_OPCODE_NEXT_TRACK      |  \
                                                           MCS_SUPPORTED_OPCODE_PREVIOUS_TRACK)
#define TMAP_PROFILE_MCS_SERVER_PLAYING_ORDER_SUPPORTED   MCS_POS_SINGLE_ONCE
#define TMAP_PROFILE_MCS_SERVER_PLAYBACK_SPEED_X1         0
#define TMAP_PROFILE_MCS_SERVER_PLAYBACK_SPEED            TMAP_PROFILE_MCS_SERVER_PLAYBACK_SPEED_X1
#define TMAP_PROFILE_MCS_SERVER_NO_SEEKING                0
#define TMAP_PROFILE_MCS_SERVER_SEEKING_SPEED             TMAP_PROFILE_MCS_SERVER_NO_SEEKING
#define TMAP_PROFILE_MCS_OPCODE_INVALID                   0

/*! Default values used in TBS server */
#define TMAP_DEFAULT_TBS_PROVIDER_NAME                    "generic"
#define TMAP_DEFAULT_TBS_PROVIDER_NAME_LEN                sizeof(TMAP_DEFAULT_TBS_PROVIDER_NAME)
#define TMAP_DEFAULT_TBS_UCI                              "un000"
#define TMAP_DEFAULT_TBS_UCI_LEN                          sizeof(TMAP_DEFAULT_TBS_UCI)
#define TMAP_DEFAULT_TBS_URI_PREFIX                       ""
#define TMAP_DEFAULT_TBS_URI_PREFIX_LEN                   sizeof(TMAP_DEFAULT_TBS_URI_PREFIX)
/*! Default signal strength: Value from 0 to 100, or 255. Values 101 to 254 are RFU. A value of 0
 *  indicates that there is no service, a value of 100 indicates the maximum signal strength, and
 *  a value of 255 indicates that the signal strength is unavailable or has no meaning for this
 *  particular bearer. The meaning of the values between 1 to 99 is left up to the implementation.*/
#define TMAP_DEFAULT_TBS_SIGNAL_STRENGTH                      255
/*! Default signal strength reporting interval in seconds.
 *  If the reporting interval is set to 0, the signal strength is reported as soon as the signal strength changes. */
#define TMAP_DEFAULT_TBS_SIGNAL_STRENGTH_REPORTING_INTERVAL   0
/*! Default status flags : TBS_STATUS_FLAGS_INBAND_RINGTONE - Inband ringtone enabled
 *  TBS_STATUS_FLAGS_SILENT_MODE - Server is in silent mode. ie, server disables ringtones */
#define TMAP_DEFAULT_TBS_STATUS_FLAGS                         0

/*! Application defined unique content control IDs for MCS and TBS */
#define TMAP_PROFILE_MCS_CCID                             0
#define TMAP_PROFILE_TBS_CCID                             1

#define TMAP_PROFILE_REMOTE_CALL_CONTROLS_LIST_INIT_CAPACITY  1

/*! Values to use in PTS test case mode */
#define TMAP_PROFILE_MCS_SERVER_PTS_TRACK_TITLE           "music"
#define TMAP_PROFILE_MCS_SERVER_PTS_TRACK_TITLE_LEN       sizeof(TMAP_PROFILE_MCS_SERVER_PTS_TRACK_TITLE)
#define TMAP_PROFILE_MCS_SERVER_PTS_TRACK_DURATION        (6000)
#define TMAP_PROFILE_TBS_SERVER_PTS_TARGET_BEARER_URI     "skype:abc"
#define TMAP_PROFILE_TBS_SERVER_PTS_TARGET_BEARER_URI_LEN sizeof(TMAP_PROFILE_TBS_SERVER_PTS_TARGET_BEARER_URI)
#define TMAP_PROFILE_TBS_SERVER_PTS_FRIENDLY_NAME         "unknown"
#define TMAP_PROFILE_TBS_SERVER_PTS_FRIENDLY_NAME_LEN     sizeof(TMAP_PROFILE_TBS_SERVER_PTS_FRIENDLY_NAME)
#define TMAP_PROFILE_TBS_SERVER_PTS_DEF_PROVIDER_NAME     "unknown"
#define TMAP_PROFILE_TBS_SERVER_PTS_DEF_PROVIDER_NAME_LEN sizeof(TMAP_PROFILE_TBS_SERVER_PTS_DEF_PROVIDER_NAME)
#define TMAP_PROFILE_TBS_SERVER_PTS_DEF_URI_PREFIX        ""
#define TMAP_PROFILE_TBS_SERVER_PTS_DEF_URI_PREFIX_LEN    sizeof(TMAP_PROFILE_TBS_SERVER_PTS_DEF_PROVIDER_NAME)

/*! \brief Get status notify list */
#define tmapProfileTbsServer_GetStatusNotifyList() (task_list_flexible_t *)(&(tmapProfile_tbs_data.status_notify_list))

/*! \brief TMAP server common data */
typedef struct
{
    /*! Current PTS mode. TRUE if in PTS mode */
    bool pts_mode;
}tmap_profile_mcs_tbs_server_common_data_t;

/*! \brief MCS server data */
typedef struct
{
    /*! Service handle */
    ServiceHandle service_handle;

    /*! Current media state */
    GattMcsMediaStateType media_state;

    /*! Length of current track in 0.01 resolution */
    int32 track_duration;

    /*! Track position in 0.01 resolution */
    int32 track_position;
} tmap_profile_mcs_server_data_t;

/*! \brief Telephone bearer server data */
typedef struct
{
    /*! Service handle */
    ServiceHandle service_handle;

    /*! Call identifier */
    uint8 call_id;

    /*! List of tasks to receive call controls from remote side */
    TASK_LIST_WITH_INITIAL_CAPACITY(TMAP_PROFILE_REMOTE_CALL_CONTROLS_LIST_INIT_CAPACITY) status_notify_list;

    /*! Is URI is valid or not */
    bool valid_uri_flag;
} tmap_profile_tbs_server_data_t;

/*! \brief Check if call is in active state */
#define tmapProfileTbsServer_IsCallInActiveState() (tmapProfileTbsServer_GetCallState() == TBS_CALL_STATE_ACTIVE)

/*! TMAP server common data */
extern tmap_profile_mcs_tbs_server_common_data_t tmap_server_data;

/*! \brief Check if PTS mode is enabled or not */
#define  tmapServer_IsPtsModeEnabled()   tmap_server_data.pts_mode

/*! \brief Initialise media control service server
    \param app_task Task to receive messages from MCS server
*/
void tmapProfileMcsServer_Init(Task app_task);

/*! \brief Add configurations to the MCS server
    \param cid  GATT Connection identifier on which configuration
                needs to be done.
*/
void tmapProfileMcsServer_AddConfig(gatt_cid_t cid);

/*! \brief Remove configurations of the MCS server
    \param cid  GATT Connection identifier on which configuration
                needs to be removed
*/
void tmapProfileMcsServer_RemoveConfig(gatt_cid_t cid);

/*! \brief Sets the current media state in MCS server
    \param new_state  New media state
    \return TRUE if success, FALSE otherwise
*/
bool tmapProfileMcsServer_SetMediaState(GattMcsMediaStateType new_state);

/*! \brief Get the current media state
    \return Returns the media state.
*/
GattMcsMediaStateType tmapProfileMcsServer_GetMediaState(void);

/*! \brief Process the messages received from MCS server
    \param message  Message to process
*/
void tmapProfileMcsServer_ProcessMcsServerMessage(Message message);

/*! \brief Initialise telephone bearer service server
    \param app_task Task to receive messages from TBS server
*/
void tmapProfileTbsServer_Init(Task app_task);

/*! \brief Register with TBS server to receive call controls from remote device
    \param  task The task being registered to receive notifications.
 */
void tmapProfileTbsServer_RegisterForRemoteCallControls(Task task);

/*! \brief Add configurations to the TBS server
    \param cid  GATT Connection identifier on which configuration
                needs to be done.
*/
void tmapProfileTbsServer_AddConfig(gatt_cid_t cid);

/*! \brief Remove configurations of the TBS server
    \param cid  GATT Connection identifier on which configuration
                needs to be removed
*/
void tmapProfileTbsServer_RemoveConfig(gatt_cid_t cid);

/*! \brief Create a new call
    \param call_state  Initial state of the call
    \param call_flags  Call flags
    \return TRUE if success, FALSE otherwise
*/
GattTbsCcpNotificationResultCodes tmapProfileTbsServer_CreateCall(GattTbsCallStates call_state,
                                                                  GattTbsCallFlags call_flags,
                                                                  uint16 target_uri_size,
                                                                  const char *target_uri);

/*! \brief Sets the call state in TBS server
    \param call_state  New state of the call to set
    \return TRUE if success, FALSE otherwise
*/
bool tmapProfileTbsServer_SetCallState(GattTbsCallStates call_state);

/*! \brief Get the state of current call
    \return Returns the call state. If call is not found, then TBS_CALL_STATE_INVALID
            will be returned.
*/
GattTbsCallStates tmapProfileTbsServer_GetCallState(void);

/*! \brief Terminate the call
    \param reason   Reason for termination
    \return TRUE if success, FALSE otherwise
*/
bool tmapProfileTbsServer_TerminateCall(GattTbsCallTerminationReason reason);

/*! \brief Check if any call is present
    \return TRUE if call is present, FALSE otherwise
*/
bool tmapProfileTbsServer_IsCallPresent(void);

/*! \brief Process the messages received from TBS server
    \param message  Message to process
*/
void tmapProfileTbsServer_ProcessTbsServerMessage(Message message);

/*! Functions added for PTS test cases */

/*! \brief Enable/Disable PTS mode in MCS server
    \param enable  TRUE to enable, FALSE otherwise
*/
void tmapProfileMcsServer_EnablePtsMode(bool enable);

/*! \brief Sets the TBS provider name
    \param name_len  Length of the provider name
    \param provider_name  Pointer to provider name
    \return TRUE if success, FALSE otherwise
*/
bool tmapProfileTbsServer_SetProviderName(uint8 name_len, char *provider_name);

/*! \brief Sets the TBS technology used
    \param technology  Technology to set
    \return TRUE if success, FALSE otherwise
*/
bool tmapProfileTbsServer_SetTechnology(GattTbsTechnology technology);

/*! \brief Sets the URI prefix list
    \param prefix_len  Length of the URI prefix
    \param prefix  Pointer to the prfix
    \return TRUE if able to set the URI prefix, FALSE otherwise
*/
bool tmapProfileTbsServer_SetUriPrefixList(uint8 prefix_len, char *prefix);

/*! \brief Sets the status flags in the TBS server
    \param status_flags  Flag value to set
           Bit 0 : Inband ringtone enable/disable
           Bit 1 : Silent mode enable/disable
           Bit 2-15: RFU
    \return TRUE if success, FALSE otherwise
*/
bool tmapProfileTbsServer_SetStatusFlags(uint16 status_flags);

/*! \brief Set valid URI flag
    \param is_valid  TRUE to set valid URI, FALSE otherwise
*/
void tmapProfileTbsServer_SetValidUriFlag(bool is_valid);

/*! \brief Enable/Disable PTS mode in TBS server
    \param enable  TRUE to enable, FALSE otherwise
*/
void tmapProfileTbsServer_EnablePtsMode(bool enable);

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#endif /* TMAP_PROFILE_MCS_TBS_PRIVATE_H */
/*! @} */