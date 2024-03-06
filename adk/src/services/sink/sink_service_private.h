/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup sink_service
    \brief      Sink service types and data to be used within sink_service only
    @{
*/

#ifndef SINK_SERVICE_PRIVATE_H
#define SINK_SERVICE_PRIVATE_H

#include "sink_service.h"
#include <domain_message.h>
#include <task_list.h>
#include <bdaddr.h>
#include <bt_device.h>
#include "bt_types.h"

#define SINK_SERVICE_CLIENT_TASKS_LIST_INIT_CAPACITY 1
#define SINK_SERVICE_MAX_SM      4

#ifdef ENABLE_LE_SINK_SERVICE

/*! Maximum supported devices within a coordinated set */
#define SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET    3

/*! This is the T-CSIP discovery timer. ie, the time for which the LEA device discovery (set member discovery)
    is allowed. Post this timeout, the LEA device search will be stopped.
*/
#define sinkServiceLe_LeaDiscoveryTimeout()    D_SEC(30)
#endif /* ENABLE_LE_SINK_SERVICE */

/*! \brief Internal messages for the sink_service */
typedef enum
{
    /*! Request to connect to a handset */
    SINK_SERVICE_INTERNAL_CONNECT_REQ = INTERNAL_MESSAGE_BASE,

    /*! Request to disconnect a handset */
    SINK_SERVICE_INTERNAL_DISCONNECT_REQ,

    /*! Delivered when an ACL connect request has completed. */
    SINK_SERVICE_INTERNAL_CONNECT_ACL_COMPLETE,

    /*! Request to cancel any in-progress connect to SINK. */
    SINK_SERVICE_INTERNAL_CONNECT_STOP_REQ,

    /*! Request to re-try the ACL connection after a failure. */
    SINK_SERVICE_INTERNAL_CONNECT_ACL_RETRY_REQ,

    /*! Timeout message to clear the possible pairing flag for an SM */
    SINK_SERVICE_INTERNAL_POSSIBLE_PAIRING_TIMEOUT,

    /*! Request to connect profiles */
    SINK_SERVICE_INTERNAL_CONNECT_PROFILES_REQ,

#ifdef ENABLE_LE_SINK_SERVICE
    /*! Timeout message to stop searching for LEA Devices (set members) */
    SINK_SERVICE_INTERNAL_LEA_DISCOVERY_TIMEOUT,
#endif

    /*! This must be the final message */
    SINK_SERVICE_INTERNAL_MESSAGE_END
} sink_service_internal_msg_t;

ASSERT_INTERNAL_MESSAGES_NOT_OVERFLOWED(SINK_SERVICE_INTERNAL_MESSAGE_END)

/*! \brief Sink Service state machine states */
typedef enum
{
    SINK_SERVICE_STATE_DISABLED     = 0,        /*!< disabled, no connections, or pairing allowed */
    SINK_SERVICE_STATE_DISCONNECTED = 1,        /*!< enabled, but not connected to a device */
    SINK_SERVICE_STATE_PAIRING,                 /*!< pairing to a device */
    SINK_SERVICE_STATE_CONNECTING_BREDR_ACL,    /*!< connecting the ACL */
    SINK_SERVICE_STATE_CONNECTING_PROFILES,     /*!< waiting for profiles to connect */
#ifdef ENABLE_LE_SINK_SERVICE
    SINK_SERVICE_STATE_CONNECTING_LE_ACL,       /*!< connecting the LE-ACL */
    SINK_SERVICE_STATE_CONNECTING_LE_WHITELIST, /*!< connecting using whitelist */
    SINK_SERVICE_STATE_CONNECTED_AND_SCANNING,  /*!< connected and also scanning for paired devices */
#endif
    SINK_SERVICE_STATE_CONNECTED,               /*!< one or more profiles are connected */
} sink_service_state_t;

/*! \brief Definition of message sent to clients to indicate connection. */
typedef struct
{
    /*! Transport Bluetooth Address of ACL connection attempted device. */
    tp_bdaddr tpaddr;
} SINK_SERVICE_INTERNAL_CONNECT_ACL_COMPLETE_T;

/*! \brief Different device types supported by the sink service */
typedef enum
{
    /*! Device type is not known */
    SINK_SERVICE_DEVICE_UNKNOWN,

    /*! Device supports BREDR only */
    SINK_SERVICE_DEVICE_BREDR,

#ifdef ENABLE_LE_SINK_SERVICE
    /*! Device supports LE only */
    SINK_SERVICE_DEVICE_LE,

    /*! Device supports dual mode (ie both LE and BREDR) */
    SINK_SERVICE_DEVICE_DUAL
#endif /* ENABLE_LE_SINK_SERVICE */

} sink_service_device_type_t;

#ifdef ENABLE_LE_SINK_SERVICE

typedef struct
{
    /*! BLE device address for the device that the instance is holding an ACL */
    tp_bdaddr tp_acl_hold_addr;

    /*! Is GATT service discovery completed for this device? */
    bool gatt_discovery_completed;

    /*! Device instance for this connection */
    device_t sink_device;

    /*! GATT Profile Connection Identifier */
    gatt_cid_t gatt_cid;

    /*! Is the BLE link encrypted? */
    bool link_encrypted;
} lea_device_info_t;

/*! \brief UUID filter to use for RSSI pairing */
typedef struct
{
    /*! Number of UUID filter to use */
    uint8 num_of_uuids;

    /*! List of UUID filters */
    uint16 uuid_list[SINK_SERVICE_MAX_LEA_UUID_FILTER];
} sink_service_lea_uuid_filter_t;
#endif

/*! \brief Context for an instance of a sink service state machine */
typedef struct
{
    /*! Task for this instance. */
    TaskData task_data;

    /*! Current state */
    sink_service_state_t state;

    /*! Device instance (BREDR) this state machine represents */
    device_t sink_device;

    /*! device address for the device that the instance is holding an ACL for */
    bdaddr acl_hold_addr;

    /*! Bitmask of profiles that have been requested 
        \note This is needed to record if profiles connect and disconnect
              during the profile connection timeout */
    uint32 profiles_requested;

#ifdef ENABLE_LE_SINK_SERVICE
    /*! Is Connection(s) locally terminated by source app? */
    bool local_initiated_disconnect;

    /*! LEA devices information for this state */
    lea_device_info_t  lea_device[SINK_SERVICE_MAX_SUPPORTED_LEA_DEVICES_IN_SET];
#endif
} sink_service_state_machine_t;

/*! \brief sink service data */
typedef struct
{
    /*! Init's local task */
    TaskData task;

    /*! List of clients */
    TASK_LIST_WITH_INITIAL_CAPACITY(SINK_SERVICE_CLIENT_TASKS_LIST_INIT_CAPACITY) clients;

    /*! Flag to set if the sink service should pair when there is no device in device list */
    bool pairing_enabled;

    /*! Flag to set if there is a request to Disable the Sink Service. */
    bool disable_request_pending;

    /*! Flag to set if there is a pending pairing request. */
    bool pairing_request_pending;

    /*! Current sink service mode. */
    sink_service_mode_t mode;

#ifdef ENABLE_LE_SINK_SERVICE
    /*! UUID adv filters to use during RSSI pairing */
    sink_service_lea_uuid_filter_t filter_data;
#endif

    /*! Sink Service state machine */
    sink_service_state_machine_t state_machines[SINK_SERVICE_MAX_SM];

} sink_service_data_t;

extern sink_service_data_t sink_service_data;

/*! \brief Get pointer to Sink Service task*/
static inline Task SinkService_GetTask(void)
{
    return &sink_service_data.task;
}

/*! \brief Get pointer to Sink Service data structure */
static inline sink_service_data_t * SinkService_GetTaskData(void)
{
    return &sink_service_data;
}

/*! \brief Get pointer to Sink Service client list */
static inline task_list_flexible_t * SinkService_GetClientList(void)
{
    return (task_list_flexible_t *)&sink_service_data.clients;
}

/*! Get the current sink service mode */
#define sinkService_GetMode()                           (SinkService_GetTaskData()->mode)

/*! Check if current mode of operation is BREDR or not */
#define sinkService_IsInBredrMode()                     (sinkService_GetMode() == SINK_SERVICE_MODE_BREDR)

#ifdef ENABLE_LE_SINK_SERVICE
/*! Check if current mode of operation is LE or not */
#define sinkService_IsInLeMode()                        (sinkService_GetMode() == SINK_SERVICE_MODE_LE)

/*! Check if current mode of operation is dual mode (with LE preference) or not */
#define sinkService_IsInDualModePreferredLe()           (sinkService_GetMode() == SINK_SERVICE_MODE_DUAL_PREF_LE)

/*! Check if current mode of operation is dual mode (with BREDR preference) or not */
#define sinkService_IsInDualModePreferredBredr()        (sinkService_GetMode() == SINK_SERVICE_MODE_DUAL_PREF_BREDR)

/*! Check if current mode of operation is dual mode or not */
#define sinkService_IsInDualMode()                      (sinkService_IsInDualModePreferredLe() || \
                                                         sinkService_IsInDualModePreferredBredr())


#else

/*! Check if current mode of operation is LE or not */
#define sinkService_IsInLeMode()                        (FALSE)

/*! Check if current mode of operation is dual mode (with LE preference) or not */
#define sinkService_IsInDualModePreferredLe()           (FALSE)

/*! Check if current mode of operation is dual mode (with BREDR preference) or not */
#define sinkService_IsInDualModePreferredBredr()        (FALSE)

/*! Check if current mode of operation is dual mode or not */
#define sinkService_IsInDualMode()                      (FALSE)

#endif /* ENABLE_LE_SINK_SERVICE */

/*! \brief Cast a Task to a sink_service_state_machine_t.
    This depends on task_data being the first member of sink_service_state_machine_t. */
#define sinkService_GetSmFromTask(task)                 ((sink_service_state_machine_t *)(task))
#define sinkService_GetTaskForSm(_sm)                   ((&(_sm)->task_data))
#define sinkService_GetStateForSm(_sm)                  ((_sm->state))

/*! \brief Main sink service message handler

    \param[in] task Task data
    \param[in] id Received message id
    \param[in] message Received message data
*/
void sinkService_MainMessageHandler(Task task, MessageId id, Message message);

/* Macro for iterating through all SM instances in the sink service */
#define FOR_EACH_SINK_SM(_sm) for(sink_service_state_machine_t *_sm = sink_service_data.state_machines;\
                                     _sm < &sink_service_data.state_machines[SINK_SERVICE_MAX_SM];\
                                     _sm++)

#endif /* SINK_SERVICE_PRIVATE_H */

/*! @} */