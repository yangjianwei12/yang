/*!
    \copyright  Copyright (c) 2008 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file
    \defgroup   pairing Pairing
    @{
        \ingroup    bt_domain
        \brief      Header file for the Pairing module.

                    The module provides services to allow other devices to discover this
                    device and permanently pair.

                    APIs are provided to allow pairing to any device (Pairing_Pair()), or to
                    a specific device (Pairing_PairAddress()).

                    If needed, the operation of the pairing module can be monitored by 
                    registering for activity events. Register using Pairing_ActivityClientRegister()
                    following which PAIRING_ACTIVITY messages will be sent.

                    Events suitable for use by the user interface are also generated. These
                    are accesible via the MessageBroker, as the group ID #PAIRING_MESSAGE_GROUP.

        \startuml

            [*] -down-> NULL
            NULL -down-> INITIALISING : Pairing_Init()
            INITIALISING : Registering EIR data
            INITIALISING -down-> IDLE : EIR registration complete
            IDLE : Page and Inquiry scan disabled
            
            state DevicePairing {
                DevicePairing : Page scan enabled
                IDLE -down-> DISCOVERABLE : INTERNAL_PAIR_REQ
                IDLE -down-> PENDING_AUTHENTICATION : INTERNAL_PAIR_REQ(known addr)
                DISCOVERABLE : Inquiry scan enabled
                DISCOVERABLE : Awaiting device connection
                DISCOVERABLE -up-> IDLE : PAIR_CFM(timeout/cancelled)
                DISCOVERABLE -down-> PENDING_AUTHENTICATION : Start authentication
                PENDING_AUTHENTICATION : Pairing in progress
                PENDING_AUTHENTICATION --> IDLE : PAIR_CFM(success/failed)
                LI_PENDING_AUTHENTICATION : Pairing in progress (locally initiated)
                LI_PENDING_AUTHENTICATION --> IDLE : PAIR_CFM(success/failed)
            }

            footer Note that PAIRING_STATE_ prefix dropped from states and PAIRING_ prefix dropped from messages.

        \enduml
*/

#ifndef PAIRING_H_
#define PAIRING_H_

#include <connection.h>

#include "domain_message.h"
#include <task_list.h>
#include <marshal.h>
#include <bt_device.h>


/*! Defines the pairing client task list initial capacity */
#define PAIRING_CLIENT_TASK_LIST_INIT_CAPACITY 1
/*! Defines the pairing activity task list initial capacity */
#define PAIRING_ACTIVITY_LIST_INIT_CAPACITY 3


/*! \brief Pairing module state machine states */
typedef enum pairing_states
{
    PAIRING_STATE_NULL,                       /*!< Startup state */
    PAIRING_STATE_INITIALISING,               /*!< Initialising state */
    PAIRING_STATE_IDLE,                       /*!< No pairing happening */
    PAIRING_STATE_DISCOVERABLE,               /*!< Discoverable to the device */
    PAIRING_STATE_PENDING_AUTHENTICATION,     /*!< Waiting to authenticate with device */
    PAIRING_STATE_LI_PENDING_AUTHENTICATION,  /*!< Waiting to authenticate with device
                                                   for Locally Initiated(LI) pairing. */
 } pairingState;

/*! \brief Pairing module UI Provider contexts */
typedef enum
{
    context_handset_pairing_idle,
    context_handset_pairing_active,

} pairing_provider_context_t;

/*! \brief Internal message IDs */
enum pairing_internal_message_ids
{
                                                /*!  Pair with handset/phone/AV source */
    PAIRING_INTERNAL_PAIR_REQ = INTERNAL_MESSAGE_BASE,
    PAIRING_INTERNAL_LE_PEER_PAIR_REQ,          /*!< Pair with le peer */
    PAIRING_INTERNAL_PAIR_LE_REQ,               /*!< Pair with le handset */
    PAIRING_INTERNAL_TIMEOUT_IND,               /*!< Pairing has timed out */
    PAIRING_INTERNAL_DISCONNECT_IND,            /*!< Link disconnected */
    PAIRING_INTERNAL_PAIR_STOP_REQ,             /*!< Stop in progress pairing */
    PAIRING_INTERNAL_LE_PAIR_TIMEOUT,           /*!< Disable LE pairing */
    PAIRING_INTERNAL_CTKD_TIMEOUT,              /*!< Wait for CTKD over BR/EDR-SM has timed out */

    /*! This must be the final message */
    PAIRING_INTERNAL_MESSAGE_END
};

typedef struct
{
    /*! If set to a non-zero address pairing will be initiated with this address. */
    bdaddr addr;
    /*! If the request was user initiated (opposed to automatically initiated by software */
    bool is_user_initiated;
    /*! Whether or not to enable page scan as well as inquiry scan */
    bool control_page_scan;
} pairing_params_t;

/*! \brief Definition of the #PAIRING_INTERNAL_PAIR_REQ message content */
typedef struct
{
    /*! The requester's task */
    Task client_task;
    /*! Additional parameters */
    pairing_params_t params;
} PAIR_REQ_T;

/*! \brief Definition of the #PAIRING_INTERNAL_LE_PEER_PAIR_REQ message content */
typedef struct
{
    /*! The requester's task */
    Task client_task;
    /*! Address to pair */
    typed_bdaddr typed_addr;
    /*! le peer pairing works differently if it's a client or server */
    bool le_peer_server;
} PAIR_LE_PEER_REQ_T;

/*! \brief Definition of the #PAIRING_INTERNAL_PAIR_STOP_REQ_T message content */
typedef struct
{
    /*! The requester's task */
    Task client_task;
}PAIRING_INTERNAL_PAIR_STOP_REQ_T;

/*! Handling of pairing requests for a BLE device */
typedef enum
{
        /* only BLE connections to devices support secure connections
           will be permitted. These devices provide BLE pairing
           automatically when the handset pairs over BREDR. */
    pairingBleDisallowed,
        /* Pairing requests for a BLE link will be processed BUT when
           the simple pairing completes, if the public address does
           not match a paired device - the link will be disconnected
           and pairing forgotten */
    pairingBleOnlyPairedDevices,
        /* Pairing requests for a BLE link will be processed if a
           random address is used.
           When pairing completes, the device will be saved, unless it
           turns out that the address was not resolvable. In which case
           the link is disconnected and pairing forgotten */
    pairingBleAllowOnlyResolvable,
        /* All pairing requests for a BLE link will be processed.
           When pairing completes, the device will be saved, using
           the resolvable (public) address if available. */
    pairingBleAllowAll,
} pairingBlePermission;


/*! Pairing task structure */
typedef struct
{
    /*! The pairing module task */
    TaskData task;
    /*! The pairing module client's task */
    Task     client_task;
    /*! The pairing stop request task */
    Task     stop_task;
    /*! Client task for concurrent BLE handset pairing */
    Task     pair_le_task;
    /*! Client task for handling key indication requests */
    Task     key_indication_handler;
    /*! client list that the pairing module shall send indication messages to */
    TASK_LIST_WITH_INITIAL_CAPACITY(PAIRING_CLIENT_TASK_LIST_INIT_CAPACITY) client_list;
    /*! The current pairing state */
    pairingState state;
    /*! Set if the link is Secure and CTKD over BR/EDR-SM is expected to begin soon */
    unsigned     smp_ctkd_expected:1;
    /*! Set if CTKD has started */
    unsigned     smp_ctkd_ongoing:1;
    /*! Ensure only 1 pairing operation can be running. */
    uint16   pairing_lock;
    /*! Number of unacknowledged peer signalling msgs */
    uint16   outstanding_peer_sig_req;
    /*! How to handle BLE pairing */
    pairingBlePermission    ble_permission;
    /*! The current BLE link pending pairing. This will be random address if used. */
    typed_bdaddr            pending_ble_address;
    /*! The type of device being paired */
    deviceType device_type;
    /*! BT address of device IO caps received, 0 otherwise */
    bdaddr   device_io_caps_rcvd_bdaddr;
    /*! Pairing parameters */
    pairing_params_t params;

    TASK_LIST_WITH_INITIAL_CAPACITY(PAIRING_ACTIVITY_LIST_INIT_CAPACITY) pairing_activity;

    /*! List of client tasks registered for security related events */
    task_list_t *security_client_tasks;
} pairingTaskData;

/*! \brief Pairing status codes */
typedef enum pairing_status
{
    pairingSuccess,
    pairingNotReady,
    pairingAuthenticationFailed,
    pairingtNoLinkKey,
    pairingTimeout,
    pairingUnknown,
    pairingStopped,
    pairingFailed,

    pairingInProgress,      /*!< Error code for #PAIRING_STOP_CFM, also an Activity status */
} pairingStatus;

/*! \brief Status codes used in #PAIRING_ACTIVITY messages 

    \note Some values are defined in terms of pairingStatus for compatibility.
          Previously values came from the #pairingStatus enum. Customer code
          could still be using these.
 */
typedef enum pairing_activity_status
{
    pairingActivitySuccess = pairingSuccess,
    pairingActivityInProgress = pairingInProgress,

    pairingActivityNotInProgress,
    pairingActivityCompleteVersionChanged,
    pairingActivityLinkKeyReceived,
} pairingActivityStatus;

/*! \brief Message IDs from Pairing task to main application task */
enum pairing_messages
{
    /*! Message confirming pairing module initialisation is complete. */
    PAIRING_INIT_CFM = PAIRING_MESSAGE_BASE,
    /*! Message confirming pairing is complete. */
    PAIRING_PAIR_CFM,
    /*! Message in response to a Pairing_PairStop(). 
        If no message content then pairing has stopped, otherwise the status
        may indicate an error stopping. */
    PAIRING_STOP_CFM,
    PAIRING_ACTIVITY,
    PAIRING_ACTIVE,
    PAIRING_INACTIVE,
    PAIRING_COMPLETE,
    PAIRING_FAILED,
    /*! Message in response to a security confirmation event 

        Note: As of now certain security events/errors such as pin/key
        missing(reconnection) is consumed by the pairing module and not propagated
        to the clients. Clients that acts in the master role may need to
        detect these conditions and take corrective action. */
    PAIRING_SECURITY_CFM,

    /*! This must be the final message */
    PAIRING_MESSAGE_END
};

/*! \brief Definition of #PAIRING_PAIR_CFM message. */
typedef struct
{
    /*! The status result of the pairing */
    pairingStatus status;
    /*! The address of the paired device */
    bdaddr device_bd_addr;
} PAIRING_PAIR_CFM_T;

/*! \brief Definition of #PAIRING_STOP_CFM message. */
typedef struct
{
    /*! The status result of the pairing stop */
    pairingStatus status;
} PAIRING_STOP_CFM_T;

/*! \brief Message indicating pairing activity.
    For example pairing is in progress.
*/
typedef struct
{
    pairingActivityStatus status;   /*!< The pairing activity being reported */
    bool user_initiated;            /*!< Whether pairing is currently user_initiated */
    bool permanent;                 /*!< Used in the case of pairingActivitySuccess
                                         to indicate if device_addr is a permanent address
                                         or a random one */
    bdaddr device_addr;             /*!< The address affected. For most messages this
                                         will be empty. 
                                         In the case of pairingSuccess this is the
                                         address of the device now paired */
} PAIRING_ACTIVITY_T;

/*! \brief Definition of #PAIRING_SECURITY_CFM message. */
typedef struct
{
    /*! The status result of the security cfm */
    uint8 hci_status;
    /*! The context used for the security operation */
    uint16 context;
    /*! remote device address for which this security event occured */
    typed_bdaddr typed_addr;
} PAIRING_SECURITY_CFM_T;

#ifdef ENABLE_LE_DEBUG_SECONDARY
/*! \brief Callback to override pairing module CM Prim handler */
typedef bool (*pairing_override_cm_handler_callback_t)(Message msg);
#endif /* ENABLE_LE_DEBUG_SECONDARY */

/*! Marshalling type definition for PAIRING_ACTIVITY_T */
extern const marshal_type_descriptor_t marshal_type_descriptor_PAIRING_ACTIVITY_T;

/*!< App pairing task */
extern pairingTaskData pairing_task_data;

/*! Get pointer to Pairing data structure */
#define PairingGetTaskData()             (&pairing_task_data)

/*! Get pointer to Pairing client list */
#define PairingGetClientList()             (task_list_flexible_t *)(&pairing_task_data.client_list)

/*! Get pointer to Pairing's pairing activity */
#define PairingGetPairingActivity()             (task_list_flexible_t *)(&pairing_task_data.pairing_activity)

/*! \brief Initialise the pairing application module.
 */
bool Pairing_Init(Task init_task);

/*! \brief Pair with a device, where inquiry scanning is required.

    \param[in] client_task      Task to send #PAIRING_PAIR_CFM response message to.
    \param     params->addr     If set to a non-zero value pairing will be initiated with this address. 
                                If set to a zero address, inquiry (and optionally page) scan will be enabled
                                to allow incoming pairing requests
    \param     params->is_user_initiated    Indicates whether pairing was user initiated or automatic
    \param     params->control_page_scan    Set to TRUE to enable page scan as part of this request. Set to 
                                            FALSE if page scan is controlled by another entity.
 */
void Pairing_PairWithParams(Task client_task, pairing_params_t* params);

/*! \brief Pair with a device, where inquiry scanning is required.

    \param[in] client_task       Task to send #PAIRING_PAIR_CFM response message to.
    \param     is_user_initiated TRUE if this is a user initiated request.
 */
void Pairing_Pair(Task client_task, bool is_user_initiated);

/*! \brief Pair with a device where the address is already known.

    Used to pair with a device where the BT address is already known and inquiry 
    scanning is not required. Typically in response to receiving the address from 
    peer earbud via peer signalling channel.

    \param[in] client_task  Task to send #PAIRING_PAIR_CFM response message to.
    \param[in] handset_addr Pointer to BT address of handset.
 */
void Pairing_PairAddress(Task client_task, bdaddr* device_addr);


/*! \brief Stop a pairing.

    If successfully stopped the client_task passed when initiating pairing
    will receive a PAIRING_PAIR_CFM message with a status code of pairingStopped.

    The client_task passed to this function will receive the message 
    PAIRING_STOPPED_CFM once pairing has stopped. Note that if it is too
    late to stop the pairing then PAIRING_STOPPED_CFM will be sent when the pairing
    is complete. In such circumstances the client passed to the pairing function will receive
    a PAIRING_PAIR_CFM message with a status code indicating the result of the
    pairing operation.

    Calling this function a second time, before receiving PAIRING_STOP_CFM will
    result in a panic.

    \param[in] client_task  Task to send #PAIRING_STOP_CFM response message to.
 */
void Pairing_PairStop(Task client_task);

#ifndef USE_SYNERGY
/*! Handler for all connection library messages not sent directly

    This function is called to handle any connection library messages sent to
    the application that the pairing module is interested in. If a message
    is processed then the function returns TRUE.

    \note Some connection library messages can be sent directly as the
        request is able to specify a destination for the response.

    \param  id              Identifier of the connection library message
    \param  message         The message content (if any)
    \param  already_handled Indication whether this message has been processed by
                            another module. The handler may choose to ignore certain
                            messages if they have already been handled.

    \returns TRUE if the message has been processed, otherwise returns the
        value in already_handled
 */
bool Pairing_HandleConnectionLibraryMessages(MessageId id,Message message, bool already_handled);
#endif /* !USE_SYNERGY */

/*! \brief Register to receive PAIRING_ACTIVITY messages.

    If required the registration can be removed using Pairing_ActivityClientUnregister().

    \param task Task to send the messages to.
*/
void Pairing_ActivityClientRegister(Task task);

/*! \brief Remove registration for PAIRING_ACTIVITY messages.

    Remove a task registration added by call to Pairing_ActivityClientRegister().

    \param task Task to send the messages to.
*/
void Pairing_ActivityClientUnregister(Task task);

/*! Add a device to the paired device list.

    This is used to add a device to the PDL

    \param  address         The address of the device
    \param  key_length      Length of link_key
    \param  link_key        Pointer to the link key
 */
void Pairing_AddAuthDevice(const bdaddr* address, const uint16 key_length, const uint16* link_key);

/*! \brief Pair with le peer device. This is a temporary function until
           discovery is removed from the pairing module.
    \note The peer pairing shall set the device flags once the full peer pairing procedure is
          complete.
 */
void Pairing_PairLePeer(Task client_task, typed_bdaddr* device_addr, bool server);

/*! \brief Pair with a BLE handset device.

    \param[in] client_task  Task to send #PAIRING_PAIR_CFM response message to.
    \param[in] handset_addr Pointer to BT address of handset.
*/
void Pairing_PairLeAddress(Task client_task, const typed_bdaddr* device_addr);

/*! \brief Initiate master directed pairing with a BLE device.

    \param[in] client_task  Task to send #PAIRING_PAIR_CFM response message to.
    \param[in] device_addr Pointer to BT address of remote device

    \note If pairing is re-initiated to a device which is already paired and if pairing info is not lost
          on the remote side, then also #PAIRING_PAIR_CFM will be sent with status as success.
*/
void Pairing_PairLeAddressAsMaster(Task client_task, const typed_bdaddr* device_addr);

/*! \brief Registers a task to handle key request indications */
void Pairing_RegisterKeyIndicationHandler(Task task);

/*! \brief Registers a task to receive notifications about pairing state */
void Pairing_RegisterNotificationClient(Task task);

/*! \brief TEST FUNCTION to force link key TX to peer on reboot. */
void Pairing_SetLinkTxReqd(void);

/*! \brief TEST FUNCTION */
void Pairing_ClearLinkTxReqd(void);

/*! \brief Register a client task to receive security related notifications */
void Pairing_RegisterForSecurityEvents(Task task);

/*! \brief Unregister the client task from receiving security related notifications */
void Pairing_UnregisterForSecurityEvents(Task task);

/* Useful Pairing State Macros */

/*! \brief Pairing Is Idle */
#define PairingIsIdle() \
    (PairingGetTaskData()->state == PAIRING_STATE_IDLE)

/*! \brief Pairing Is Discoverable */
#define PairingIsDiscoverable() \
    (PairingGetTaskData()->state == PAIRING_STATE_DISCOVERABLE)

/*! \brief Pairing Is Busy : Authenticating */
#define PairingIsBusy() \
    (PairingGetTaskData()->state >= PAIRING_STATE_PENDING_AUTHENTICATION)

/*! \brief Callback for Remote IO capability */
typedef void (*pairing_remote_io_capability_callback_t)(const CL_SM_REMOTE_IO_CAPABILITY_IND_T* ind);

#ifdef USE_SYNERGY
typedef struct
{
    bdaddr address;
    uint8 status;
} bond_indication_ind_t;
#endif /* USE_SYNERGY */

typedef struct
{
    cl_sm_io_capability io_capability;
    mitm_setting        mitm;
    bool                bonding;
    uint16              key_distribution;
    uint16              oob_data;
    uint8*              oob_hash_c;
    uint8*              oob_rand_r;
} pairing_io_capability_rsp_t;

/*! \brief Callback for IO capability request */
typedef pairing_io_capability_rsp_t (*pairing_io_capability_request_callback_t)(const CL_SM_IO_CAPABILITY_REQ_IND_T* ind);

typedef enum
{
    pairing_user_confirmation_reject,
    pairing_user_confirmation_accept,
    pairing_user_confirmation_wait,
} pairing_user_confirmation_rsp_t;

#ifdef USE_SYNERGY
/*! \brief Callback for bond indication */
typedef void (*bond_indication_callback_t)(const bond_indication_ind_t* ind);
#endif

/*! \brief Callback for user confirmation */
typedef pairing_user_confirmation_rsp_t (*pairing_user_confirmation_callback_t)(const CL_SM_USER_CONFIRMATION_REQ_IND_T* ind);

/*! \brief Pairing plugin */
typedef struct
{
    pairing_remote_io_capability_callback_t     handle_remote_io_capability;
    pairing_io_capability_request_callback_t    handle_io_capability_req;
    pairing_user_confirmation_callback_t        handle_user_confirmation_req;
#ifdef USE_SYNERGY
    bond_indication_callback_t                  handle_bond_indication;
#endif /* USE_SYNERGY */
} pairing_plugin_t;

/*! 
    \brief Register pairing plugin callback

    \param plugin The plugin to register. This function will panic if
           called multiple times without calling Pairing_PluginUnregister
           inbetween
 */
void Pairing_PluginRegister(pairing_plugin_t plugin);

/*!
    \brief Retry user confirmation where pairing_user_confirmation_callback_t
           previously returned pairing_user_confirmation_wait

    \returns TRUE if user confirmation can be retried, FALSE if there is no
             pending user confirmation to retry. This may happen due to
             incorrect call sequence, pairing failure/timeout or shortage of
             memory to store the confirmation request.
 */
bool Pairing_PluginRetryUserConfirmation(void);

/*!
    \brief Unregister pairing plugin callback

    \param plugin The plugin to unregister. This function will panic if
           this does not match the last plugin passed to Pairing_PluginRegister
 */
void Pairing_PluginUnregister(pairing_plugin_t plugin);

#ifdef ENABLE_LE_DEBUG_SECONDARY
/*!
    \brief Register callback function to handle CM security prims

    CM security prims are routed to registered callback, On successful handling
    these prims are not handled by Pairing module's CM Prim handler.

    \param  callback function to handle CM Security Prims , NULL to clear the filter (unregister callbacks) enabling
            pairing module to handle all CM Prims.
 */
void Pairing_OverrideCmPrimHandler(const pairing_override_cm_handler_callback_t cm_prim_handler_callback);
#endif /* ENABLE_LE_DEBUG_SECONDARY */

#endif /* PAIRING_H_ */

/*! @} */

