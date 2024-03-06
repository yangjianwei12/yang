/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file   
    \defgroup   csip CSIP
    @{
        \ingroup    profiles
        \brief      Header file for CSIP client
*/

#ifndef CSIP_CLIENT_H
#define CSIP_CLIENT_H

#include "bt_types.h"

/*! \brief CSIP Client status codes. */
typedef enum
{
    /*! The requested operation completed successfully */
    CSIP_CLIENT_STATUS_SUCCESS,

    /*! The requested operation failed to complete */
    CSIP_CLIENT_STATUS_FAILED,
} csip_client_status_t;

/*! \brief Events sent by CSIP profile to other modules. */
typedef enum
{
    /*! Discovered CSIS Service and connected with all the members */
    CSIP_CLIENT_MSG_ID_INIT_COMPLETE,

    /*! Event to inform that device have been added to the set */
    CSIP_CLIENT_MSG_ID_DEVICE_ADDED,

    /*! Event to inform that device have been removed from the set */
    CSIP_CLIENT_MSG_ID_DEVICE_REMOVED,

    /*! Event to inform CSIP Profile has been disconnected (all devices are removed) */
    CSIP_CLIENT_MSG_ID_PROFILE_DISCONNECT,

    /*! This must be the final message */
    CSIP_CLIENT_MSG_ID_MESSAGE_END
} csip_client_msg_id_t;

/*! \brief Data associated with CSIP Profile intialization*/
typedef struct
{
    /*! status which tells if CSIP Profile instance is destroyed or not */
    csip_client_status_t    status;

    /*! Total number of devices in the coordinated set */
    uint8                   total_devices;

    /*! Connected devices in the coordinated set */
    uint8                   connected_devices;
} CSIP_CLIENT_INIT_COMPLETE_T;

/*! \brief Data associated with CSIP device add operation */
typedef struct
{
    /*! GATT connection identifier on which the added device belongs */
    gatt_cid_t              cid;

    /*! status which tells if CSIP device add is success or not */
    csip_client_status_t    status;

    /*! FALSE if all members have been added */
    bool                    more_devices_needed;
} CSIP_CLIENT_DEVICE_ADDED_T;

/*! \brief Data associated with CSIP device remove */
typedef struct
{
    /*! GATT connection identifier of the device which is removed */
    gatt_cid_t              cid;

    /*! status which tells if CSIP device removal is success or not */
    csip_client_status_t    status;

    /*! FALSE if all members have been removed */
    bool                    more_devices_present;
} CSIP_CLIENT_DEVICE_REMOVED_T;

/*! \brief Data associated with CSIP Profile disconnection */
typedef struct
{
    /*! status which tells if CSIP Profile instance is destroyed or not */
    csip_client_status_t            status;
} CSIP_CLIENT_DISCONNECT_T;

typedef union
{
    CSIP_CLIENT_INIT_COMPLETE_T             init_complete;
    CSIP_CLIENT_DEVICE_ADDED_T              device_added;
    CSIP_CLIENT_DEVICE_REMOVED_T            device_removed;
    CSIP_CLIENT_DISCONNECT_T                disconnected;
} csip_client_message_body_t;

typedef struct
{
    csip_client_msg_id_t         id;
    csip_client_message_body_t   body;
} csip_client_msg_t;

/*! Callback structure used when an observer registers with the CSIP Client module. */
typedef void (*csip_client_callback_handler_t)(const csip_client_msg_t *message);

/*! \brief Initialize the CSIP client

    \param handler Callback handler to receive responses.
*/
void CsipClient_Init(csip_client_callback_handler_t handler);

/*! \brief Creates an instance for the CSIP Profile.

    \param cid GATT Connection identifier for which to create CSIP Profile connection

    NOTE: A success return value indicates that a create instance request has been placed
          successfully. Clients should handle CSIP_CLIENT_MSG_ID_DEVICE_ADDED to check
          if the instance has been successfully created or not for the given cid.
          Also it will receive CSIP_CLIENT_MSG_ID_INIT_COMPLETE if all the device
          in the coordinated set got addded.
*/
bool CsipClient_CreateInstance(gatt_cid_t cid);

/*! \brief Destroy the CSIP profile instance for the given GATT connection identifier
           from the group

    \param cid GATT Connection identifier for which to close CSIS Profile connection

    NOTE: A success return value indicates that a destroy instance request has been placed
          successfully.
          Clients will get CSIP_CLIENT_MSG_ID_DEVICE_REMOVED message when the specified device gets
          removed.
          In addition, clients will be receiving CSIP_CLIENT_MSG_ID_PROFILE_DISCONNECT if all
          instances in the coordinated got destroyed.
*/
bool CsipClient_DestroyInstance(gatt_cid_t cid);

/*! \brief Set the lock for the given device

    \param cid GATT Connection identifier on which lock operation needs to be performed.

    \param lock_enabled TRUE for setting lock, FALSE for releasing the lock

    NOTE: A success return value indicates that a lock request has been placed successfully.
*/
bool CsipClient_SetLockForMember(gatt_cid_t cid, bool lock_enabled);

/*! \brief Set the lock for the all the devices in the connected coordinated set

    \param lock_enabled TRUE for setting lock, FALSE for releasing the lock

    NOTE: A success return value indicates that a lock request has been placed successfully.
*/
bool CsipClient_SetLockForAllMembers(bool lock_enabled);

/*! \brief Check if the given advertisement data is from a set member

    \param adv_data Pointer to advertisement data.
    \param adv_data_len Length of the advertisement data

    \return TRUE if the advert is from set member, FALSE otherwise
*/
bool CsipClient_IsAdvertFromSetMember(uint8 *adv_data, uint16 adv_data_len);

/*! \brief Read the set size of the coordinated set

    NOTE: A success return value indicates that a request has been placed successfully.
*/
bool CsipClient_ReadSetSize(void);

#endif /* CSIP_CLIENT_H */
/*! @} */