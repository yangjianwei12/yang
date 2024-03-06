/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup  csip
    \brief       Header file for private defines and functions for CSIP client
    @{
*/

#ifndef CSIP_CLIENT_PRIVATE_H
#define CSIP_CLIENT_PRIVATE_H

#include <logging.h>
#include <task_list.h>
#include "bt_types.h"
#include "csip.h"
#include "gatt.h"
#include "csip_client.h"

/*! Number of CSIP devices supported */
#define MAX_CSIP_DEVICES_SUPPORTED      (3)

/*! Defines for type of SIRK */
#define CSIP_CLIENT_PLAIN_SIRK_TYPE     (1)
#define CSIP_CLIENT_ENCRYPTED_SIRK_TYPE (0)

/*! CSIP client instance state */
typedef enum
{
    /*! CSIP client instance in idle/free state */
    csip_client_state_idle,

    /*! Discovery in progress state */
    csip_client_state_discovery,

    /*! CSIP client in connected state */
    csip_client_state_connected,

    /*! CSIP client in connected state */
    csip_client_state_disconnecting,
} csip_client_state_t;

/*! CSIP client lock request type */
typedef enum
{
    /*! No lock requests in progress */
    csip_client_no_request_in_progress,

    /*! Request for setting lock in all members of the coordinated set is in progress */
    csip_client_set_lock_request,

    /*! Request for releasing the lock in all members of the coordinated set is in progress */
    csip_client_release_lock_request,
} csip_client_request_type_t;

/*! Types for getting the csip instance by comparison */
typedef enum
{
    /*! Get CSIP Client instance based on connection identifier */
    csip_client_compare_by_cid,

    /*! Get CSIP Client instance by state */
    csip_client_compare_by_state,

    /*! Get CSIP Client instance by profile handle */
    csip_client_compare_by_profile_handle,

    /*! Get CSIP Client instance by valid/invalid cid */
    csip_client_compare_by_valid_invalid_cid
} csip_client_instance_compare_by_type_t;

/*! CSIP device instance */
typedef struct
{
    /*! Connection Identifier for this CSIP client instance */
    gatt_cid_t cid;

    /*! Sirk */
    uint8 sirk[CSIP_SIRK_SIZE];

    /*! CSIP profile Handle */
    CsipProfileHandle csip_profile_handle;

    /*! Instance present state */
    csip_client_state_t state;

    /*! Flag to denote the sirk obtained is encrypted or not */
    bool sirk_encrypted;

    /*! Rank of the device */
    uint8 rank;

    /*! Current status of lock */
    uint8 lock_status;

    /*! Size of the coordinated set */
    uint8 set_size;
} csip_client_device_instance_t;

/*! \brief CSIP profile task data */
typedef struct
{
    /*! CSIP Client profile task */
    TaskData   task_data;

    /*! CSIP individual device instances */
    csip_client_device_instance_t device_instance[MAX_CSIP_DEVICES_SUPPORTED];

    /*! CSIP Client callback handler */
    csip_client_callback_handler_t       callback_handler;

    /*! Request for a lock set in progress */
    csip_client_request_type_t pending_request;
} csip_client_task_data_t;

/*! \brief CSIP client task Data */
extern csip_client_task_data_t csip_client_taskdata;

/*! \brief Returns the CSIP client context */
#define CsipClient_GetContext()           (&csip_client_taskdata)

#endif /* CSIP_CLIENT_PRIVATE_H */
/*! @} */