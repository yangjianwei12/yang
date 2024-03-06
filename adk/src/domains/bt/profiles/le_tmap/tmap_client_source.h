/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup tmap_profile
    \brief      Header file for TMAP Client source
    @{
*/

#ifndef TMAP_CLIENT_SOURCE_H
#define TMAP_CLIENT_SOURCE_H

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

#ifdef USE_SYNERGY
#include "bt_types.h"
#include "tmap_client_lib.h"
#include "tmap_client_source_unicast.h"

/*! \brief TMAP Profile Client status codes. */
typedef enum
{
    /*! The requested operation completed successfully */
    TMAP_CLIENT_MSG_STATUS_SUCCESS,

    /*! The requested operation failed to complete */
    TMAP_CLIENT_MSG_STATUS_FAILED,

    /*! The requested operation completed successfully but TMAS server is not found in remote */
    TMAP_CLIENT_MSG_STATUS_SUCCESS_TMAS_SRVC_NOT_FOUND
} tmap_client_msg_status_t;

/*! \brief Events sent by TMAP profile to other modules. */
typedef enum
{
    /*! TMAP profile instance has been created */
    TMAP_CLIENT_MSG_ID_INIT_COMPLETE,

    /*! Event to inform status of registration with CAP */
    TMAP_CLIENT_MSG_ID_REGISTER_CAP_CFM,

    /*! Event to inform TMAP Profile has removed single device */
    TMAP_CLIENT_MSG_ID_PROFILE_DEVICE_REMOVED,

    /*! Event to inform TMAP Profile has been disconnected (ie, all devices are removed) */
    TMAP_CLIENT_MSG_ID_PROFILE_DISCONNECT,

    /*!  Event to inform volume state change from remote */
    TMAP_CLIENT_MSG_ID_VOLUME_STATE_IND,

    /*! This must be the final message */
    TMAP_CLIENT_MSG_ID_MESSAGE_END
} tmap_client_msg_id_t;

/*! \brief Data associated with TMAP client initialisation */
typedef struct
{
    /*! Group handle for which the TMAP profile instance was created. */
    ServiceHandle                   group_handle;

    /*! Status which tells if TMAP Profile instance is created or not */
    tmap_client_msg_status_t        status;

    /*! TMAP role */
    uint16                          role;
} TMAP_CLIENT_MSG_ID_INIT_COMPLETE_T;

/*! \brief Data associated with CAP registration */
typedef struct
{
    /*! CAP group ID */
    ServiceHandle                   group_id;

    /*! Status which tells if CAP register was success or not */
    tmap_client_msg_status_t        status;
} TMAP_CLIENT_MSG_ID_REGISTER_CAP_CFM_T;

/*! \brief Data associated with TMAP Profile device remove */
typedef struct
{
    /*! GATT Connection identifier on which the device got removed. */
    gatt_cid_t                  cid;

    /*! Status which tells if TMAP Profile instance is removed or not */
    tmap_client_msg_status_t    status;

    /*! Flag to indicate if there are still devices yet to be removed */
    bool more_devices_present;
} TMAP_CLIENT_MSG_ID_PROFILE_DEVICE_REMOVED_T;

/*! \brief Data associated with TMAP Profile instance destruction */
typedef struct
{
    /*! Group handle on which the disconnected device belongs */
    ServiceHandle               group_handle;

    /*! Status which tells if TMAP Profile instance is destroyed or not */
    tmap_client_msg_status_t    status;
} TMAP_CLIENT_MSG_ID_PROFILE_DISCONNECT_T;

/*! \brief Data associated with TMAP Profile volume state change */
typedef struct
{
    /*! Group handle for which volume state change is received */
    ServiceHandle     group_handle;

     /*! volume state. */
     uint8            volumeState;

     /*! mute state */
     uint8            mute;

     /*! change counter */
     uint8            changeCounter;
} TMAP_CLIENT_MSG_ID_VOLUME_STATE_IND_T;

/*! \brief TMAP message body structure */
typedef union
{
    TMAP_CLIENT_MSG_ID_INIT_COMPLETE_T               init_complete;
    TMAP_CLIENT_MSG_ID_REGISTER_CAP_CFM_T            cap_register_cfm;
    TMAP_CLIENT_MSG_ID_PROFILE_DEVICE_REMOVED_T      device_removed;
    TMAP_CLIENT_MSG_ID_PROFILE_DISCONNECT_T          disconnect_complete;
    TMAP_CLIENT_MSG_ID_VOLUME_STATE_IND_T            volume_state_ind;
} tmap_client_message_body_t;

/*! \brief TMAP message structure */
typedef struct
{
    tmap_client_msg_id_t         id;
    tmap_client_message_body_t   body;
} tmap_client_msg_t;

/*! Callback function used when an observer registers with the TMAP Client Profile module. */
typedef void (*tmap_client_source_callback_handler_t)(const tmap_client_msg_t *message);

/*! \brief Check if the given TMAP role supports unicast media sender role */
#define TmapClientSource_IsRoleSupportsUnicastMediaReceiver(role) (role & TMAP_ROLE_UNICAST_MEDIA_RECEIVER)
/*! \brief Check if the given TMAP role supports unicast call terminal role */
#define TmapClientSource_IsRoleSupportsCallTerminal(role)         (role & TMAP_ROLE_CALL_TERMINAL)

/*! \brief Initialise the TMAP client component
 */
bool TmapClientSource_Init(Task init_task);

/*! \brief Register as Persistant Device user */
void TmapClientSource_RegisterAsPersistentDeviceDataUser(void);

/*! \brief Read the Tmap role characteristics
    \param[in] cid    GATT Connection id to which the Tmap role is to be read
*/
void TmapClientSource_ReadTmapRole(gatt_cid_t cid);

/*! \brief Register a callback function to get messages from TMAP
    \param handler  Callback handler
*/
void TmapClientSource_RegisterCallback(tmap_client_source_callback_handler_t handler);

/*! \brief Creates an instance for the TMAP Profile.

    \param cid GATT Connection identifier for which to create TMAP Profile connection

    \note A success return value indicates that a create instance request has been placed
          successfully. Clients should handle TMAP_CLIENT_MSG_ID_INIT_COMPLETE to check
          if the instance has been successfully created or not.
*/
bool TmapClientSource_CreateInstance(gatt_cid_t cid);

/*! \brief Used to check if TMAP is connected or not

    \return TRUE if TMAP is connected, FALSE otherwise
*/
bool TmapClientSource_IsTmapConnected(void);

/*! \brief Used to check if TMAP is connected or not for the given cid
           If given cid is zero, it will check if any TMAP instance is connected .

    \return TRUE if TMAP is connected, FALSE otherwise
*/
bool TmapClientSource_IsTmapConnectedForCid(gatt_cid_t cid);

/*! \brief Destroy the TMAP profile instance for the GATT connection identifier.

    \param group_handle Group handle on which the connection to destroy belongs.
    \param cid GATT Connection identifier for which to close TMAP Profile connection.
               If cid given is zero, then all instances in the group will be destroyed.
               (ie, group will be destroyed)

    NOTE: A success return value indicates that a destroy instance request has been placed
          successfully. Clients should handle TMAP_CLIENT_MSG_ID_PROFILE_DEVICE_REMOVED to check
          if the instance has been successfully destroyed or not. The cid parameter in
          message TMAP_CLIENT_MSG_ID_PROFILE_DEVICE_REMOVED will be zero for group destroy.
          In addition to this, client will get message TMAP_CLIENT_MSG_ID_PROFILE_DISCONNECT
          if all instances in the group are removed.
*/
bool TmapClientSource_DestroyInstance(ServiceHandle group_handle, gatt_cid_t cid);

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
/*! \brief Register with CAP

    \param group_handle  group handle on which CAP register needs to be done

    NOTE: A success return value indicates that a register request has been placed
          successfully. Clients should handle TMAP_CLIENT_MSG_ID_REGISTER_CAP_CFM to check
          whether registering successful or not.
          CAP group ID needs to be set using TmapClientSource_SetGroupId() before calling this.
*/
bool TmapClientSource_RegisterTaskWithCap(ServiceHandle group_handle);
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

/*! \brief Set the CAP group ID

    \param cid  GATT Connection on which CAP group ID needs to be set
    \param group_handle  Group handle to set

    \return TRUE if able to update group ID
*/
bool TmapClientSource_SetGroupId(gatt_cid_t cid, ServiceHandle group_handle);

/*! \brief Set the TMAP Client profile into PTS mode.

    \param pts_mode set to TRUE to enable PTS mode.

    NOTE: This function should be used in PTS mode only.
*/
void TmapClientSource_SetPtsMode(bool pts_mode);

/*! \brief Add the device into the the group

    \param cid_array Array containing CID of connected members
    \param count  Number of CID in the array

    \return TRUE if able to add the device into the group. FALSE otherwise
*/
bool TmapClientSource_AddDeviceToGroup(gatt_cid_t *cid_array, uint8 count);

#else /* USE_SYNERGY */
#define TmapClientSource_Init()
#endif /* USE_SYNERGY */

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */

#endif /* TMAP_CLIENT_SOURCE_H */

/*! @} */