/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   handset_service Handset Service
    @{
        \ingroup    services
        \brief      A service that provides procedures for connecting and managing
                    connections to handset(s).

        # Overview

        This service is mainly concerned with managing the connection to a handset.
        That includes both remote (initiated by the handset) and local (initiated by a
        call to one of the connect APIs) connections.

        ## Remote Connections

        Remote connections will be detected by this handset service and notified to
        any clients that have registered for notifications.

        For the handset to initiate a connection the local device must first be
        connectable - there are functions in this service to control whether the local
        device is connectable or not.

        ## Local Connections

        There are functions to request the connection or disconnection of a handset and
        these also include the list of profiles to connect to the handset. This service
        will manage the connect or disconnect procedure until it is completed. When it
        is complete the client Task that made the request will be sent a matching CFM
        message containing the result.

        Both connect and disconnect procedures can be cancelled while they are still in
        progress by making the opposite request. For example, connect will be cancelled
        if a disconnect request is made while it is progress, and vice versa for
        disconnect.

        ## Connection Status

        If the connection status changes for any reason, e.g. link-loss or the handset
        has initiated a disconnect, it will be notified to any clients that have
        registered for notifications.

*/

#ifndef HANDSET_SERVICE_H_
#define HANDSET_SERVICE_H_

#include <message.h>

#include <bdaddr.h>
#include <bt_device.h>
#include <handset_bredr_context.h>
#include "domain_message.h"
#include "lea_advertising_policy.h"

/*! \brief Handset Service UI Provider contexts */
typedef enum
{
    context_handset_connected,
    context_handset_not_connected,

} handset_provider_context_t;

/*! \brief Events sent by handset to other modules. */
typedef enum
{
    /*! Module initialisation complete */
    HANDSET_INIT_CFM = HANDSET_SERVICE_MESSAGE_BASE,

    /*! A handset has initiated a connection. */
    HANDSET_SERVICE_CONNECTED_IND,

    /*! A handset has disconnected. */
    HANDSET_SERVICE_DISCONNECTED_IND,

    /*! Confirmation of completion of a connect request. */
    HANDSET_SERVICE_CONNECT_CFM,

    /*! Confirmation of completion of a disconnect request. */
    HANDSET_SERVICE_DISCONNECT_CFM,

    /*! Confirmation of completion of a cancel connect request. */
    HANDSET_SERVICE_CONNECT_STOP_CFM,

    /*! Confirmation of completion of a ble connectable request. */
    HANDSET_SERVICE_LE_CONNECTABLE_IND,

    /*! Confirmation of completion of reconnect request. */
    HANDSET_SERVICE_MP_CONNECT_CFM,

    /*! Confirmation of completion of cancelling reconnect request. */
    HANDSET_SERVICE_MP_CONNECT_STOP_CFM,

    /*! Confirmation of completion of a disconnect all request. */
    HANDSET_SERVICE_MP_DISCONNECT_ALL_CFM,

    /*! Confirmation that very first profile is connected. */
    HANDSET_SERVICE_FIRST_PROFILE_CONNECTED_IND,

    /*! Confirmation of handset pairing. */
    HANDSET_SERVICE_PAIR_HANDSET_CFM,
    
    /*! Confirmation of cancel handset pairing. */
    HANDSET_SERVICE_CANCEL_PAIR_HANDSET_CFM,

    /*! Confirmation that the first transport has connected.

        Note: This confirmation has been newly added to include 
              BLE connection also to be considered while deciding
              to play connect/disconnect prompts, so the behavior
              may change */
    HANDSET_SERVICE_FIRST_TRANSPORT_CONNECTED_IND,

    /*! Confirmation that all the transports are disconnected. 

        Note: This confirmation has been newly added to include 
              BLE connection also to be considered while deciding
              to play connect/disconnect prompts, so the behavior
              may change */
    HANDSET_SERVICE_ALL_TRANSPORTS_DISCONNECTED_IND,

    /*! This must be the final message */
    HANDSET_SERVICE_MESSAGE_END
} handset_service_msg_t;

/*! \brief Status codes for the handset service. */
typedef enum
{
    handset_service_status_success,
    handset_service_status_failed,
    handset_service_status_cancelled,
    handset_service_status_no_mru,
    handset_service_status_connected,
    handset_service_status_disconnected,
    handset_service_status_link_loss,
} handset_service_status_t;

/*! \brief Handset Service configuration data stored in PDD. */
typedef struct
{
    uint8 max_bredr_connections;
    uint8 acl_connect_attempt_limit;
    /* Number of Bluetooth Low Energy connections allowed.
       Must be greater than 1. If Bluetooth Low Energy is not
       required, changes should be made in Topology or the
       application. */
    uint8 max_le_connections;
    bool enable_connection_barge_in : 1;
    /* A flag indicating whether the acl_connect_attempt_limit should be
       ignored and instead the Handset Service should perform an unlimited
       number of ACL reconnection attempts. */
    bool enable_unlimited_acl_reconnection : 1;
    /* Page interval, used for unlimited link loss reconnection, in ms */
    uint32 unlimited_reconnection_page_interval_ms;
    /* Page timeout, used for initial ACL connection, in Bluetooth slots */
    uint16 initial_page_timeout_slots;
    /* Page interval, used for initial ACL connection, i.e. until acl_connect_attempt_limit is reached */
    uint16 initial_page_interval_ms;
    /* Page timeout, used for unlimited link loss reconnection, in Bluetooth slots */
    uint16 unlimited_reconnection_page_timeout_slots;
} handset_service_config_t;

/*! \brief LEA connect parameters */
typedef struct
{
    lea_adv_policy_announcement_type_t dir_announcement;
    lea_adv_policy_announcement_type_t undir_announcement;
    uint16                             dir_sink_audio_context;
    uint16                             dir_source_audio_context;
    uint16                             undir_sink_audio_context;
    uint16                             undir_source_audio_context;
} handset_service_lea_connect_params_type_t;

/*! \brief Handset service reconnect type */
typedef enum
{
    /* No MRU available, Fresh device*/
    handset_service_reconnect_type_no_mru,
    /* Reconnection scenario after power on, MRU is available */
    handset_service_reconnect_type_mru_available,
    /* Link loss occured and streaming was idle */
    handset_service_reconnect_type_linkloss_no_streaming,
    /* Link loss occured while streaming was active */
    handset_service_reconnect_type_linkloss_streaming
} handset_service_reconnect_type_t;

typedef struct
{
    /*! \brief Callback mechanism which application can use and define to override and provide its desired Handset Device
               to connect BREDR & BLE ACL.
        \param connect_type   Connection scenario which the Handset service is handling (e.g. Power on (No MRU)/Reconnection with MRU/Linkloss).
        \param handset_device Pointer to BT Device which the Handset service is intending to connect. Application can override its desired
                              Handset device to connect. This parameter is Input and Output both.
        \param lea_params     The LE Audio parameters which will be used by default. Application can override with these parameters.
                             The parameters include announcement type and available audio contexts.

        \note 1) If the application has not registered this callback, Handset service will go through its default behavior to handle
                 Connection/Reconnection/Link Loss scenario's.
              2) If the application has registered this callback, Handset service will behave as follows:
                 a) When the device is powered on and MRU devices are available, by default Handset service will pick the Most recently used(MRU)
                    device to connect. It will provide the selected MRU Handset device and the associated LEA parameters information via this callback
                    to get consent from application. If application wants to connect to a different handset device, it can override the handset device
                    and the LEA parameters. If the chosen Handset device support BREDR, Handset service will attempt to make BREDR connection.
                 b) When the LE ACL link is lost, Handset service will attempt to reconnect to the Handset device using Directed Advertisements
                    (with Targeted Announcement). Before starting the directed advertisements, Handset service will provide the Link Lost Handset Device
                    and the LEA parameters information to application via this callback to get consent from application. Application can ONLY override the
                    LEA parameters (announcement type & Audio Context) if it intends to change it.
                c) If application observes that Handset service has shown AUDIO_CONTEXT_UNKNOWN in the LEA parameters, it implies Handset services would
                    like to use the default PACS available audio context for advertising. Application can update the available audio context if it requires
                    specific audio context to be used in the advertising.
     */
    bool (*GetDeviceAndLeaParamsToConnect)(handset_service_reconnect_type_t reconnect_type, tp_bdaddr *device, handset_service_lea_connect_params_type_t *params);
} handset_service_connect_device_callback_t;

/*! Recommended configuration for singlepoint applications */
extern const handset_service_config_t handset_service_singlepoint_config;

/*! Recommended configuration for multipoint applications */
extern const handset_service_config_t handset_service_multipoint_config;

/*! \brief Confirmation for a handset ble connectable request.

    This message will get sent each time for a previously placed
    ble connectable request.
*/
typedef struct
{ 
    /*! Reason for a previously placed ble connectable request */
    handset_service_status_t status;

    /*! Flag to indicate whether the device is LE connectable */
    bool le_connectable;
} HANDSET_SERVICE_LE_CONNECTABLE_IND_T;


/*! \brief Notification that a handset has connected.

    This message will get sent each time one or more profiles connect from
    a handset.

    It is likely the client will receive this message multiple times - up to a
    maximum of one message per profile.
*/
typedef struct
{
    /*! Address of the handset. */
    bdaddr addr;

    /*! Mask of the profile(s) currently connected. */
    uint32 profiles_connected;
} HANDSET_SERVICE_CONNECTED_IND_T;

/*! \brief Notification that first profile with handset is connected.

    It is used to play the "CONNECTED" prompt so don't wait for all the profiles
    to be connected.

    The client will receive this message only when first profile is connected.
*/
typedef struct
{
    /*! Address of the handset. */
    bdaddr addr;
} HANDSET_SERVICE_FIRST_PROFILE_CONNECTED_IND_T;

/*! \brief Notification that the first transport with the handset got connected.

    It is used to play the "CONNECTED" prompt.

    The client will receive this message only when the first transport is connected.
*/
typedef struct
{
    /*! Public address of the handset. */
    tp_bdaddr tp_addr;
} HANDSET_SERVICE_FIRST_TRANSPORT_CONNECTED_IND_T;

/*! \brief Notification that all the transports with the handset got disconnected.

    It is used to play the "DISCONNECTED" prompt.

    The client will receive this message only when all the transports are disconnected.
*/
typedef struct
{
    /*! Public address of the handset. */
    tp_bdaddr tp_addr;
} HANDSET_SERVICE_ALL_TRANSPORTS_DISCONNECTED_IND_T;

/*! \brief Notification that a handset has disconnected.

    This message will get sent once a handset has no profiles connected.
*/
typedef struct
{
    /*! Address of the handset. */
    bdaddr addr;

    /*! Reason for the disconnect */
    handset_service_status_t status;
} HANDSET_SERVICE_DISCONNECTED_IND_T;

/*! \brief Confirmation of completed handset connect request

    This message is sent once handset service has completed the connect,
    or it has been cancelled.

    The request could have completed successfully or it could have failed,
    or have been cancelled by a later disconnect request.
*/
typedef struct
{
    /*! Address of the handset. */
    bdaddr addr;

    /*! Status of the request */
    handset_service_status_t status;
} HANDSET_SERVICE_CONNECT_CFM_T;

/*! \brief Confirmation of completion of reconnect request

    This message is sent once handset service multipoint has completed
    the reconnection.
*/
typedef struct
{
    /*! Status of the request */
    handset_service_status_t status;
} HANDSET_SERVICE_MP_CONNECT_CFM_T;

/*! \brief Confirmation of completion of reconnect request

    This message is sent once handset service multipoint has stopped
    the reconnection.
*/
typedef struct
{
    /*! Status of the request */
    handset_service_status_t status;
} HANDSET_SERVICE_MP_CONNECT_STOP_CFM_T;

/*! \brief Confirmation of completed handset disconnect request

    This message is sent once handset service has completed the
    disconnect, or it has been cancelled.

    The request could have completed successfully or it could have  been
    cancelled by a later connect request.
*/
typedef HANDSET_SERVICE_CONNECT_CFM_T HANDSET_SERVICE_DISCONNECT_CFM_T;

/*! \brief Confirmation of completed handset disconnect all request

    This message is sent once handset service has completed the
    disconnect all request, or it has been cancelled.

    The request could have completed successfully or it could have  been
    cancelled by a later connect request.
*/
typedef struct
{
    /*! Status of the request */
    handset_service_status_t status;
} HANDSET_SERVICE_MP_DISCONNECT_ALL_CFM_T;

/*! \brief Confirmation of completed handset cancel connect request

    This message is sent once handset service has cancelled the ACL
    connection to the handset, or otherwise when the connect has completed
    normally.
*/
typedef HANDSET_SERVICE_CONNECT_CFM_T HANDSET_SERVICE_CONNECT_STOP_CFM_T;

/*! \brief Confirmation of pairing request */
typedef struct
{
    /*! Status of the request */
    handset_service_status_t status;
    /*! Address of the paired device if successful */
    bdaddr bd_addr;
} HANDSET_SERVICE_PAIR_HANDSET_CFM_T;

/*! \brief Confirmation of pairing request cancellation */
typedef struct
{
    /*! Status of the request */
    handset_service_status_t status;
} HANDSET_SERVICE_CANCEL_PAIR_HANDSET_CFM_T;

/*! \brief Initialise the handset_service module.

    \param task The init task to send HANDSET_SERVICE_INIT_CFM to.

    \return TRUE if initialisation is in progress; FALSE if it failed.
*/
bool HandsetService_Init(Task task);

/*! \brief Register a Task to receive notifications from handset_service.

    Once registered, #client_task will receive #handset_msg_t messages.

    \param client_task Task to register to receive handset_service notifications.
*/
void HandsetService_ClientRegister(Task client_task);

/*! \brief Un-register a Task that is receiving notifications from handset_service.

    If the task is not currently registered then nothing will be changed.

    \param client_task Task to un-register from handet_service notifications.
*/
void HandsetService_ClientUnregister(Task client_task);

/*! \brief Connect to a handset specified by a bdaddr.

    Start the connection procedure for a handset with the given address.

    The connection procedure will be cancelled if
    #HandsetService_DisconnectRequest is called while it is still in progress.

    When the request completes, for whatever reason, the result is sent to
    the client #task in a HANDSET_SERVICE_CONNECT_CFM.

    \param task Task the CFM will be sent to when the request is completed.
    \param addr Address of the handset to connect to.
    \param profiles Profiles to connect.
*/
void HandsetService_ConnectAddressRequest(Task task, const bdaddr *addr, uint32 profiles);

#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT

/*! \brief Starts the directed advertisement to the provided handset address.

    \param handset_address   The handset address to which directed advertising should be started
    \param lea_params        The LEA params to be advertised

    \return The API returns FALSE if max BLE connection limit has reached or the provided handset is not LEA capable otherwise returns TRUE.

    \note 1) This API will start directed advertisement to the provided handset address.
          2) If undirected announcement type is also supplied, then undirected advertisement will also start along with directed advertisement.
          3) This API internally starts a timer to stop directed advertisement for configured time. If the targeted device doesn’t connect within
             this time, directed advertisement will be stopped and general advertisement will continue.
          4) This API will enable the connectable advertising, if not enabled before.
*/
bool HandsetService_ConnectLeaDevice(const bdaddr *addr,
                                     handset_service_lea_connect_params_type_t *lea_params);

/*! Application can register its callback to override and provide its desired Handset device and parameters to connect

    \param callback  The callbacks to register.
*/
void HandsetService_RegisterConnectDeviceCallback(const handset_service_connect_device_callback_t *const callback);

#endif
/*! \brief Disconnect a handset with the given bdaddr.

    Start the disconnection procedure for a handset with the given address.

    The disconenction procedure will be cancelled if one of the connect request
    functions are called for the same handset.

    When the request completes, for whatever reason, the result is sent to
    the client #task in a HANDSET_SERVICE_DISCONNECT_CFM.

    \param task Task the CFM will be sent to when the request is completed.
    \param addr Address of the handset to disconnect.
    \param exclude The profiles to be excluded from disconnection.
*/
void HandsetService_DisconnectRequest(Task task, const bdaddr *addr, uint32 exclude);

/*! \brief Disconnect a handset with the given tp_bdaddr, optionally excluding certain profiles.

    Start the disconnection procedure for a handset with the given typed bluetooth address.

    The disconenction procedure will be cancelled if one of the connect request
    functions are called for the same handset.

    When the request completes, for whatever reason, the result is sent to
    the client #task in a HANDSET_SERVICE_DISCONNECT_CFM.

    \param task Task the CFM will be sent to when the request is completed.
    \param tp_addr Typed Address of the handset to disconnect.
    \param exclude The profiles to be excluded from disconnection.
*/
void HandsetService_DisconnectTpAddrRequest(Task task, const tp_bdaddr *tp_addr, uint32 exclude);

/*! \brief Stop the ACL connection to a handset.

    Cancel any in-progress connect to a handset if, and only if, the ACL has
    not been connected yet.

    If the ACL has been connected or the profiles have started to be connected
    then wait until the connection has completed normally.

    When the connection has either been cancelled or has completed, send a 
    #HANDSET_SERVICE_CONNECT_STOP_CFM to the client task.

    If the original connect request completed successfully the status in 
    #HANDSET_SERVICE_CONNECT_STOP_CFM will be handset_service_status_connected;
    otherwise it will be handset_service_status_disconnected.

    If the same client that requested a handset connect calls this function,
    #HANDSET_SERVICE_CONNECT_STOP_CFM will be sent before
    the #HANDSET_SERVICE_CONNECT_CFM for the original connect request.

    Note: Currently only one stop request at a time is supported. If a second
          request is made while one is in progress this function will panic.

    \param task Client task requesting the stop
    \param addr Public address of the handset to stop the connection for.
*/
void HandsetService_StopConnect(Task task, const bdaddr *addr);

/*! \brief Make the local device connectable.

    \param task Task the CFM will be sent to when the request is completed.
*/
void HandsetService_ConnectableRequest(Task task);

/*! \brief Cancel all requests to make the local device connectable.

    \param task Task the CFM will be sent to when the request is completed.
*/
void HandsetService_CancelConnectableRequest(Task task);

/*! \brief Check if the device is connected

    This function is mostly intended for use in tests. Applications should
    be able to request connections or register as clients.

    \param device[in] The device to be checked

    \return TRUE if the device passed is a handset and is completely
        connected. FALSE is returned in all other cased, including connecting
        and disconnecting.
*/
bool HandsetService_Connected(device_t device);

/*! \brief Check if a handset is connected over BR/EDR.

    This function checks if any BR/EDR ACL or profile is connected to the
    given bdaddr.

    \param[in] addr Address of handset to check.

    \return TRUE if BR/EDR ACL or profile(s) are connected, FALSE otherwise.
*/
bool HandsetService_IsBredrConnected(const bdaddr *addr);

/*! \brief Check if handset service has any BR/EDR connections.

    \return TRUE if handset service has any BR/EDR connection, FALSE otherwise.
*/
bool HandsetService_IsAnyBredrConnected(void);

/*! \brief Check if handset service has any fully(ACL + supported profiles) connected BR/EDR
    Handset connections.

    \return TRUE if handset service has any fully connected BR/EDR connection, FALSE otherwise.
*/
bool HandsetService_IsAnyBredrHandsetFullyConnected(void);

/*! \brief Query if handset service is disconnecting handset(s).

    \return TRUE if handset service is disconnecting any handset(s), FALSE otherwise.
*/
bool HandsetService_IsDisconnecting(void);

/*! \brief Query if handset service is connecting handset(s).

    \return TRUE if handset service is connecting any handset(s), FALSE otherwise.
*/
bool HandsetService_IsConnecting(void);

/*! \brief Check if handset service has any LE connections.

    \return TRUE if handset service has any LE connection, FALSE otherwise.
*/
bool HandsetService_IsAnyLeConnected(void);

/*! \brief Check if handset service has any handset connections.

    \return TRUE if handset service has any handset connection, FALSE otherwise.
*/
bool HandsetService_IsAnyDeviceConnected(void);

/*! \brief Get the BT address of a handset with an BREDR connection.

    \note If there are multiple handsets with an BREDR connection, only the first
        will be returned.

    \param tp_addr Set to the typed BT address of a handset with a BREDR connection,
    if the function return value is TRUE.

    \return TRUE if a handset with an BREDR connection is found; FALSE if no handset BREDR connection.
*/
bool HandsetService_GetConnectedBredrHandsetTpAddress(tp_bdaddr * tp_addr);

/*! \brief Get the BT address of a handset with an LE connection.

    \note If there are multiple handsets with an LE connection, only the first
        will be returned.

    \param tp_addr Set to the typed BT address of a handset with an LE connection, 
    if the function return value is TRUE.

    \return TRUE if a handset with an LE connection is found; FALSE if no handset LE connection.
*/
bool HandsetService_GetConnectedLeHandsetTpAddress(tp_bdaddr *tp_addr);

/*! Set if the handset is BLE connectable.

    \param connectable TRUE if the device is BLE connectable.
*/
void HandsetService_SetBleConnectable(bool connectable);

/*! Configure handset service

    \param config Configuration
*/
bool HandsetService_Configure(handset_service_config_t config);

/*! \brief Check how many Bredr handsets are currently connected

    \return Number of connected Bredr handsets.
*/
unsigned HandsetService_GetNumberOfConnectedBredrHandsets(void);

/*! \brief Wrapper function which calls Multipoint API to connect multiple handsets.

    \param task Task the CFM will be sent to when the request is completed.
*/
void HandsetService_ReconnectRequest(Task task);

/*! \brief Wrapper function which calls Multipoint API to reconnect link loss handsets.

    \param task Task the CFM will be sent to when the request is completed.
*/
void HandsetService_ReconnectLinkLossRequest(Task task);

/*! \brief Disconnect the least recently used handset.

    \param task The CFM will be sent to this task when the request is completed.
*/
void HandsetService_DisconnectLruHandsetRequest(Task task);

/*! \brief Wrapper function which calls Multipoint API to stop connect to handset.

    \param task Client task requesting the stop.
*/
void HandsetService_StopReconnect(Task task);

/*! \brief Disconnect all currently connected handsets.

    \param task Client task requesting the stop.
    \param reason_code HCI disconnect reason code that needs to be sent to remote.
*/
void HandsetService_DisconnectAll(Task task, uint8 reason_code);

/*! \brief Set default config after DFU

    It is called to set default handset service configuration,
    after software upgrade from ADK that didn't have handset service configuration.
*/
void HandsetService_SetDefaultConfig(void *value, uint8 size);

/*! \brief Check if BR/EDR multipoint is currently enabled.
 *
    \return TRUE if BR/EDR multipoint is enabled in handset_service, FALSE otherwise.
*/
bool HandsetService_IsBrEdrMultipointEnabled(void);

/*! \brief Check the maximum number of BR/EDR handsets that can be connected at the same time.

    \return Maximum number of handsets that can be connected.
*/
unsigned HandsetService_GetMaxNumberOfConnectedBredrHandsets(void);

/*! \brief Check if connection barge-in is enabled

    \return TRUE if enabled, FALSE if disabled.
*/
bool HandsetService_IsConnectionBargeInEnabled(void);

/*! \brief Configure reconnection parameters to use when there is a link loss

    This API allows the Application to override the default Handset Service link loss
    reconnection settings. The Applcation can mandate that the Handset Service should
    try to reconnect a link loss-ed handset indefinitely, and can also specify the
    paging timeout and interval that should be used.

    \param use_unlimited_reconnection_attempts - TRUE to attempt to reconnect, till the handset
                                                 is disconnected, either by in-case event or
                                                 being connection barged-out.
    \param num_connection_attempts - the number of ACL connection attempts that shall be made,
                                     before starting to use link loss specific interval and timeout.
    \param initial_reconnection_page_interval_ms - the interval at which to page for link loss handsets,
                                                   after the usual connection attempts have expired.
    \param initial_reconnection_page_timeout_ms - the timeout for each individual page attempt for a
                                                  link loss handset, used after the usual connection
                                                  attempts have expired.
    \param unlimited_reconnection_page_interval_ms - the interval at which to page for link loss handsets,
                                                     after the usual connection attempts have expired.
    \param unlimited_reconnection_page_timeout_ms - the timeout for each individual page attempt for a
                                                    link loss handset, used after the usual connection
                                                    attempts have expired.
*/
void HandsetService_ConfigureLinkLossReconnectionParameters(
        bool use_unlimited_reconnection_attempts,
        uint8 num_connection_attempts,
        uint32 initial_reconnection_page_interval_ms,
        uint16 initial_reconnection_page_timeout_ms,
        uint32 unlimited_reconnection_page_interval_ms,
        uint16 unlimited_reconnection_page_timeout_ms);

/*! \brief Find if there is at least one handset in the device database with the matching context

    \param context - The handset context to match

    \return TRUE if a handset that matches the context is found; FALSE otherwise.
*/
bool HandsetService_IsHandsetInBredrContextPresent(handset_bredr_context_t context);

void HandsetService_RegisterPddu(void);

/*! \brief Enable pairing a new handset. This function will disconnect a connected handset
           if required to allow a new handset to connect and pair. If connection barge-in 
           has been enabled then no handset will be disconnected until a new handset 
           attempts to establish an ACL. Note that handset service will not enable 
           incoming connections unless HandsetService_ConnectableRequest has also been called. 

    \param task - The task to send HANDSET_SERVICE_PAIR_HANDSET_CFM to
    \param is_user_initiated - Whether pairing was initiated by the user
*/
void HandsetService_PairHandset(Task task, bool is_user_initiated);

/*! \brief Disable pairing a new handset. This will disable pairing but will not reconnect
           any handsets disconnected due to calling HandsetService_PairHandset. If
           handset service enabled incoming connections as a result of calling 
           HandsetService_EnableHandsetPairingthen they will not be disabled again unless
           HandsetService_CancelConnectableRequest is called.

    \param task - The task to send HANDSET_SERVICE_CANCEL_PAIR_HANDSET_CFM to
*/
void HandsetService_CancelPairHandset(Task task);

#endif /* HANDSET_SERVICE_H_ */
/*! @} */