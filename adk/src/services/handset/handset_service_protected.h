/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup    handset_service
    @{
    \brief      Handset service types to be used within handset_service only
*/

#ifndef HANDSET_SERVICE_PROTECTED_H_
#define HANDSET_SERVICE_PROTECTED_H_

#include <bdaddr.h>
#include <logging.h>
#include <panic.h>
#include <task_list.h>
#include <le_advertising_manager.h>
#include <handset_bredr_context.h>

#include "handset_service.h"
#include "handset_service_sm.h"
#include "handset_service_multipoint_sm.h"
#include "handset_service_config.h"

/*! 
    Macros for diagnostic output that can be suppressed.
*/
#define HS_LOG         DEBUG_LOG


/*! Code assertion that can be checked at run time. This will cause a panic. */
#define assert(x) PanicFalse(x)

/*! Client task list initial list */
#define HANDSET_SERVICE_CLIENT_LIST_INIT_CAPACITY 1

#define HANDSET_SERVICE_DISCONNECT_ALL_CLIENT_LIST_INIT_CAPACITY 1

#define HANDSET_SERVICE_MAX_SM      4

/*! \brief Data type to specify the state of LE advertising data set select/release operation */
typedef enum
{
    handset_service_le_adv_data_set_state_not_selected = 0,
    handset_service_le_adv_data_set_state_selected,
    handset_service_le_adv_data_set_state_selecting,
    handset_service_le_adv_data_set_state_releasing,

} handset_service_le_adv_data_set_state_t;

/*! \brief Task data for managing a single LE advertising data set. */
typedef struct
{
    /*! Task for receiving le advertising related messages. */
    TaskData task_data;

    /*! State of advertising set */
    handset_service_le_adv_data_set_state_t state;

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
    le_adv_item_handle handle;
#else
    /*! Handle for the advertising data set */
    le_adv_data_set_handle handle;
    /*! Selected LE advertising data set */
    le_adv_data_set_t set;
#endif
} handset_service_advert_task_data_t;

/*! \brief The global data for the handset_service */
typedef struct
{
    /*! Handset Service task */
    TaskData task_data;

    /*! Handset Service state machine list */
    handset_service_state_machine_t state_machine[HANDSET_SERVICE_MAX_SM];

    /*! Handset Service Multipoint state machine */
    handset_service_multipoint_state_machine_t mp_state_machine;

    /*! Client lists for notifications */
    TASK_LIST_WITH_INITIAL_CAPACITY(HANDSET_SERVICE_CLIENT_LIST_INIT_CAPACITY) client_list;
    TASK_LIST_WITH_INITIAL_CAPACITY(HANDSET_SERVICE_DISCONNECT_ALL_CLIENT_LIST_INIT_CAPACITY) disconnect_all_list;

    /*! Flag to store if handset can be paired */
    bool pairing;
    
    /*! Flag to indicate whether the device is BLE connectable */
    bool ble_connectable;

    /*! Flag to indicate the disconnect all handsets procedure is in progress. */
    bool disconnect_all_in_progress;

    /*! Task data for managing the legacy LE advertising. */
    handset_service_advert_task_data_t legacy_advert;

    /*! Task data for managing the extended LE advertising. */
    handset_service_advert_task_data_t extended_advert;

    Task pairing_task;

    /*! HCI disconnect reason code that needs to be propagated to the remote device. */
    uint8 disconnect_reason_code;

    /*! If pairing_request_pending_lock is TRUE, it indicates that a pairing request initiated by user
     *  is pending due to a locally initiated ACL disconnect in progress. Pairing will resume when the
     *  ACL disconnect gets completed.
     */
    uint16 pairing_request_pending_lock;

#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT
    /*! Application can register its callback to override and provide its desired Handset device and parameters to connect */
    const handset_service_connect_device_callback_t *connect_callback;
#endif
} handset_service_data_t;

/*! \brief Internal messages for the handset_service */
typedef enum
{
    /*! Request to connect to a handset */
    HANDSET_SERVICE_INTERNAL_CONNECT_REQ = INTERNAL_MESSAGE_BASE,

    /*! Request to disconnect a handset */
    HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ,

    /*! Delivered when an ACL connect request has completed. */
    HANDSET_SERVICE_INTERNAL_CONNECT_ACL_COMPLETE,

    /*! Request to cancel any in-progress connect to handset. */
    HANDSET_SERVICE_INTERNAL_CONNECT_STOP_REQ,

    /*! Request to re-try the ACL connection after a failure. */
    HANDSET_SERVICE_INTERNAL_CONNECT_ACL_RETRY_REQ,

    /*! Timeout message to clear the possible pairing flag for an SM */
    HANDSET_SERVICE_INTERNAL_POSSIBLE_PAIRING_TIMEOUT,

    /*! Request to connect profiles */
    HANDSET_SERVICE_INTERNAL_CONNECT_PROFILES_REQ,

    /*! Delivered when the delay period after the maximum number of BREDR ACLs being
        connected has expired, allowing enabling of truncated Page Scan for the
        connection barge-in feature. */
    HANDSET_SERVICE_INTERNAL_TRUNC_PAGE_SCAN_ENABLE,
    
    /*! Delivered after handset services has been page scanning with fast parameters
        with no connection attempts */
    HANDSET_SERVICE_INTERNAL_FAST_PAGE_SCAN_TIMEOUT_IND,

    /*! Request to reconnect to handset(s) */
    HANDSET_SERVICE_INTERNAL_MP_RECONNECT_REQ,
    /*! Delivered after the configured timeout for the directed advertisement */
    HANDSET_SERVICE_INTERNAL_DIRECTED_ADVERT_TIMEOUT,

    /*! Delivered to place a internal pairing request */
    HANDSET_SERVICE_INTERNAL_PAIR_REQ,

    /*! This must be the final message */
    HANDSET_SERVICE_INTERNAL_MESSAGE_END
} handset_service_internal_msg_t;
ASSERT_INTERNAL_MESSAGES_NOT_OVERFLOWED(HANDSET_SERVICE_INTERNAL_MESSAGE_END)

typedef struct
{
    /*! Handset device to connect. */
    device_t device;

    /*! Mask of profile(s) to connect. */
    uint32 profiles;
} HANDSET_SERVICE_INTERNAL_CONNECT_REQ_T;

typedef struct
{
    /*! Address of handset device to disconnect. */
    bdaddr addr;
    /*! Any profiles that are to be excluded from the disconnection. */
    uint32 exclude;
} HANDSET_SERVICE_INTERNAL_DISCONNECT_REQ_T;

typedef struct
{
    /*! Handset device to stop connect for */
    device_t device;
} HANDSET_SERVICE_INTERNAL_CONNECT_STOP_REQ_T;

/* \brief Message structure for HANDSET_SERVICE_INTERNAL_POSSIBLE_PAIRING_TIMEOUT 

    The message contains both the device address and the state machine
    affected. This is because the message cannot be cancelled so
    the state machine could have been cleared and used for another device.
*/
typedef struct
{
    /*! Address of device to clear */
    tp_bdaddr                       address;
    /*! The specific state machine */
    handset_service_state_machine_t *sm;
} HANDSET_SERVICE_INTERNAL_POSSIBLE_PAIRING_TIMEOUT_T;

typedef struct
{
    /*! Task requesting the reconnection. */
    Task task;

    /*! TRUE for link-loss reconnections. */
    bool link_loss;
} HANDSET_SERVICE_INTERNAL_MP_RECONNECT_REQ_T;

/*! \brief Message structure forHANDSET_SERVICE_INTERNAL_DIRECTED_ADVERT_TIMEOUT_T Directed 

    The message contains the device for which the ble context has to be updated to disconnected.
*/
typedef struct
{
    /*! Device to clear ble context */
    device_t device;
} HANDSET_SERVICE_INTERNAL_DIRECTED_ADVERT_TIMEOUT_T;

/*! \brief Send a HANDSET_SERVICE_CONNECTED_IND to registered clients.

    \param device Device that represents the handset
    \param profiles_connected Profiles currently connected to this handset.
*/
void HandsetService_SendConnectedIndNotification(device_t device,
    uint32 profiles_connected);

/*! \brief Send a HANDSET_SERVICE_DISCONNECTED_IND to registered clients.

    \param addr Address of the handset
    \param status Status of the connection.
*/
void HandsetService_SendDisconnectedIndNotification(const bdaddr *addr,
    handset_service_status_t status);

/*! \brief Send a HANDSET_SERVICE_FIRST_PROFILE_CONNECTED_IND to registered clients.

    \param device Device that represents the handset
*/
void HandsetService_SendFirstProfileConnectedIndNotification(device_t device);

/*! \brief Send a HANDSET_SERVICE_FIRST_TRANSPORT_CONNECTED_IND_T to registered clients.

    \param tp_addr Public address of the handset
*/
void HandsetService_SendFirstTransportConnectedIndNotification(tp_bdaddr *tp_addr);

/*! \brief Send a HANDSET_SERVICE_ALL_TRANSPORTS_DISCONNECTED_IND_T to registered clients.

    \param tp_addr Public address of the handset
*/
void HandsetService_SendAllTransportsDisconnectedIndNotification(tp_bdaddr *tp_addr);

/*! Handset Service module data. */
extern handset_service_data_t handset_service;

/*! Get pointer to the Handset Service modules data structure */
#define HandsetService_Get() (&handset_service)

/*! Get the Task for the handset_service */
#define HandsetService_GetTask() (&HandsetService_Get()->task_data)

/*! Get the client list for the handset service. */
#define HandsetService_GetClientList() (task_list_flexible_t *)(&HandsetService_Get()->client_list)

/*! Get the legacy advertising task data */
#define HandsetService_GetLegacyAdvertisingData()   (&HandsetService_Get()->legacy_advert)

/*! Get the Task to use when requesting legacy advertising operations. */
#define HandsetService_GetLegacyAdvertisingTask()   (&HandsetService_GetLegacyAdvertisingData()->task_data)

/*! Get the extended advertising task data */
#define HandsetService_GetExtendedAdvertisingData() (&HandsetService_Get()->extended_advert)

/*! Get the Task to use when requesting extended advertising operations. */
#define HandsetService_GetExtendedAdvertisingTask()   (&HandsetService_GetExtendedAdvertisingData()->task_data)

/*! \brief Get if the handset service has a BLE connection. 

    \return TRUE if there is an active BLE connection. FALSE otherwise
 */
bool HandsetService_IsBleConnected(void);

/*! Get if the handset service is BLE connectable */
#define HandsetService_IsBleConnectable() (HandsetService_Get()->ble_connectable)

#define HandsetService_GetDisconnectAllClientList() (task_list_flexible_t *)(&HandsetService_Get()->disconnect_all_list)

/*! Get the multipoint state machine which connects multiple handsets. */
#define HandsetService_GetMultipointSm() (HandsetService_Get()->mp_state_machine)

/*! Get if the handset service is in pairing mode. */
#define HandsetService_IsPairing() (HandsetService_Get()->pairing)

/*! Check if a new handset connection is allowed */
bool handsetService_CheckHandsetCanConnect(const bdaddr *addr);

/*! Retreive the existing or create new handset statemachine for the requested bluetooth transport address */
handset_service_state_machine_t *handsetService_FindOrCreateSm(const tp_bdaddr *tp_addr);

/*! Resolve *tpaddr if necessary and possible. Populates *resolved_tpaddr with either
    the resolved address or a copy of *tpaddr */
void HandsetService_ResolveTpaddr(const tp_bdaddr *tpaddr, tp_bdaddr *resolved_tpaddr);

/*! \brief Function to get an active handset state machine based on a BR/EDR address.

    \param addr Address to search for. Treated as a BR/EDR address.

    \return Pointer to the matching state machine, or NULL if no match.
*/
handset_service_state_machine_t *HandsetService_GetSmForBdAddr(const bdaddr *addr);

/*! \brief Make message for LE connectable indication and send it to task list */
void HandsetService_SendLeConnectableIndication(bool connectable);

/*! Try to find an active LE handset state machine for a typed address.

    This function will check both the type (PUBLIC or RANDOM) and the bdaddr
    match the LE address for a handset state machine.

    \param taddr Typed bdaddr to search for.
    \return Pointer to the matching state machine, or NULL if no match.
*/
handset_service_state_machine_t *handsetService_GetLeSmForTypedBdAddr(const typed_bdaddr *taddr);

/*! Try to find an active BR/EDR handset state machine for an address.

    \param[in] addr BR/EDR address to search for.
    
    \return Pointer to the matching state machine, or NULL if no match.
*/
handset_service_state_machine_t *handsetService_GetSmForBredrAddr(const bdaddr *addr);

/*! \brief Restart Connection Barge-in enable back-off timer

    \param delay Time delay(in milliseconds) to restart barge-in enable timer.
 */
void HandsetService_RestartBargeInEnableTimer(uint32 delay);

/*! Are all the requested profiles for a handset device currently connected?

    \param[in] sm the state machine for the handset device.

    \return boolean indicating if the ACL and all the requested profiles are connected.
*/
bool handsetServiceSm_AreAllRequestedProfilesConnected(handset_service_state_machine_t *sm);

#endif /* HANDSET_SERVICE_PROTECTED_H_ */
/*! @} */