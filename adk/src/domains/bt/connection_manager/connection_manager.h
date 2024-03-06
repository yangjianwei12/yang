/*!
\copyright  Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       connection_manager.h
\defgroup   connection_manager_domain Connection Manager
\ingroup    bt_domain
\brief      Header file for Connection Manager
*/

#ifndef CONNECTION_MANAGER_H_
#define CONNECTION_MANAGER_H_

#include "domain_message.h"


#include <message.h>
#include <connection.h>
#include <bdaddr.h>
#include <task_list.h>
#include <bt_device.h>
#include <hci.h>

#include "domain_message.h"
#include "link_policy.h"

#include <rtime.h>
#include <marshal.h>

/*! @{ */

/*! Iterator used with ConManager_IterateFirstActiveConnection() and
    ConManager_IterateNextActiveConnection() */
typedef struct
{
    void* _state;
} cm_connection_iterator_t;


#define TransportToCmTransport(transport) (1 << transport)

#define US_TO_MS(us) ((us) / US_PER_MS)

#define SLOT_DURATION_US 625

/*! Guard period intended to provide headroom for retransmission and resource contention on the ACL when in sniff. */
#define SNIFF_POTENTIAL_RETRANSMISSION_SLOTS 20

#define SNIFF_INTERVAL_US(slots) (((slots)+SNIFF_POTENTIAL_RETRANSMISSION_SLOTS)*SLOT_DURATION_US)

/*! \brief Transports for which connections are managed */
typedef enum
{
    /*! No transport */
    cm_transport_none   = 0,
    /*! BR/EDR transport */
    cm_transport_bredr  = TransportToCmTransport(TRANSPORT_BREDR_ACL),
    /*! BLE transport */
    cm_transport_ble    = TransportToCmTransport(TRANSPORT_BLE_ACL),
    /*! All transports */
    cm_transport_all = cm_transport_bredr | cm_transport_ble
} cm_transport_t;

/*! \brief QoS settings 

    The order of settings in this enum are important. Where two
    conflicting QoS settings are requested, the higher value
    setting will override the lower value setting.
*/
typedef enum
{
    /*! Invalid QoS */
    cm_qos_invalid,
    /*! Optimise for low power */
    cm_qos_low_power,
    /*! Optimise for low latency */
    cm_qos_low_latency,
    /*! LEAudio assistant link. Idle */
    cm_qos_lea_idle,
    /*! Optimise for audio data */
    cm_qos_audio,
    /*! Balance for short data exchange while have other activities */
    cm_qos_short_while_streaming,
    /*! Optimise for short data exchange */
    cm_qos_short_data_exchange,
    /*! Always accept remote device's settings, use 
        cm_qos_low_power for outgoing connections */
    cm_qos_passive,
    /*! Max QoS (always last) */
    cm_qos_max
} cm_qos_t;

/*! \brief Callback to adjust connection parameters. */
typedef struct
{
    /*! Function (callback) that can vary the connection interval to be
        requested.

        The callback is used when selecting a new set of parameters. The
        parameter update may be triggered from the link policy module if 
        all links are checked.

        \param[in] tp_addr Bluetooth address of link being adjusted. NULL if global parameters are requested
        \param[in,out] min_interval the minimum connection interval requested
        \param[in,out] max_interval the maximum connection interval requested
        \returns Return TRUE if parameters changed, FALSE otherwise */
    bool (*LeParams)(const tp_bdaddr *tpaddr, uint16 *min_interval, uint16 *max_interval);
} con_manager_connparams_callback_t;

/*! \brief Message IDs for connection manager messages to other tasks. */
enum    av_headset_conn_manager_messages
{
    /*! Message ID for a \ref CON_MANAGER_CONNECTION_IND_T message sent when
        the state of an ACL changes */
    CON_MANAGER_CONNECTION_IND = CON_MANAGER_MESSAGE_BASE,
    /*! Message ID for a \ref CON_MANAGER_TP_CONNECT_IND_T message sent when
        an ACL is connected */
    CON_MANAGER_TP_CONNECT_IND,
    /*! Message ID for a \ref CON_MANAGER_TP_DISCONNECT_IND_T message sent when
        an ACL is disconnected */
    CON_MANAGER_TP_DISCONNECT_IND,
    /*! Message ID for a \ref CON_MANAGER_TP_DISCONNECT_REQUESTED_IND_T message sent when
        an ACL disconnect has been requested */
    CON_MANAGER_TP_DISCONNECT_REQUESTED_IND,
    /*! Message ID sent when all ACLs have been disconnected. This message is 
        sent even if there were no ACLs to disconnect */
    CON_MANAGER_CLOSE_ALL_CFM,
    /*! Message ID sent when handset connections are allowed */
    CON_MANAGER_HANDSET_CONNECT_ALLOW_IND,
    /*! Message ID sent when handset connections are not allowed */
    CON_MANAGER_HANDSET_CONNECT_DISALLOW_IND,
    /*! Message ID sent when all BLE connections have been disconnected. */
    CON_MANAGER_DISCONNECT_ALL_LE_CONNECTIONS_CFM,
    /*! Message ID sent when BLE connection parameters have been updated. */
    CON_MANAGER_BLE_PARAMS_UPDATE_IND,
    /*! Message ID sent to authorise incoming connections. */
    CON_MANAGER_AUTHORISE_IND,

    /*! This must be the final message */
    CON_MANAGER_MESSAGE_END
};

/*! \brief Connection manager notification group types. */
typedef enum
{
    con_manager_notify_group_type_connect,
    con_manager_notify_group_type_disconnect,
    con_manager_notify_group_type_max
} con_manager_notify_group_type_t;

/*! Definition of message sent to clients to indicate connection status. */
typedef struct
{
    /*! BT address of (dis)connected device. */
    bdaddr bd_addr;
    /*! Connection status of the device, TRUE connected, FALSE disconnected. */
    bool connected;
    /*! Whether the connection is to a BLE device */
    bool ble;
    /*! Reason given for disconnection. For a connection, this will always be hci_success. */
    hci_status reason;
} CON_MANAGER_CONNECTION_IND_T;
extern const marshal_type_descriptor_t marshal_type_descriptor_CON_MANAGER_CONNECTION_IND_T;

/*! Definition of message sent to clients to indicate connection. */
typedef struct
{
    /*! Transport Bluetooth Address of connected device. */
    tp_bdaddr tpaddr;
    /*! Whether this connection is incoming */
    bool      incoming;
} CON_MANAGER_TP_CONNECT_IND_T;

/*! Definition of message sent to clients to indicate disconnection. */
typedef struct
{
    /*! Transport Bluetooth Address of disconnected device. */
    tp_bdaddr tpaddr;
    /*! Reason given for disconnection. */
    hci_status reason;
} CON_MANAGER_TP_DISCONNECT_IND_T;

/*! Definition of message sent to clients to indicate disconnect requested */
typedef struct
{
    /*! Transport Bluetooth Address of disconnect requested device. */
    tp_bdaddr tpaddr;
} CON_MANAGER_TP_DISCONNECT_REQUESTED_IND_T;

/*! Definition of message sent to clients to indicate a BLE connection parameter update. */
typedef struct
{
    /*! Transport Bluetooth Address of device whose connection parameters have been updated. */
    tp_bdaddr tpaddr;
    /*! The updated connection interval */
    uint16 conn_interval;
    /*! The updated slave latency */
    uint16 slave_latency;
} CON_MANAGER_BLE_PARAMS_UPDATE_IND_T;

/*! Definition of message sent to clients to indicate link authorize indication. */
typedef struct
{
    /*! The BT address of the remote device */
    typed_bdaddr bd_addr;
    /*! The protocol (L2CAP, RFCOMM, etc) */
    dm_protocol_id protocol_id;
    /*! The channel number */
    uint32 channel;
    /*! flag to determine the direction of connection */
    bool incoming;
} CON_MANAGER_AUTHORISE_IND_T;

/*! \brief  Callback to filter out connection manager notfications from registered observers.

    Connection manager notifications shall be routed to callback, On successful handling
    these notifications gets filtered out (will not be sent to) all other registered observers

    \param[in] tpaddr typed bluetooth address with transport of the connecting device.
    \param[in] is_connected TRUE for connection notfication, FALSE for disconnection notification.
    \param[in] reason given for disconnection. For a connection, this will always be hci_success.

    \return TRUE if the notifications are handled by the application callback, FALSE to allow
            connection manager to notify other registered observers.
*/
typedef bool (*con_manager_filter_notification_cb_t)(const tp_bdaddr *tpaddr, con_manager_notify_group_type_t notify_group_type, hci_status reason);

/*! \brief Initialise the connection manager module.
 */
bool ConManagerInit(Task init_task);


/*! \brief Request to create ACL to device

    Called to request an ACL to the specified BR/EDR device.  If the ACL already 
    exists then this function does nothing apart from increment usage count on 
    the ACL.

    If the ACL doens't exist then this function will request Bluestack to open
    an ACL.

    \note This function should not be called to create a BLE ACL. To create a BLE
    ACL ConManagerCreateTpAcl should be used.

    \param[in] addr Pointer to a BT address.

    \return uint16 Pointer to lock that will be cleared when ACL is available, or paging failed.
*/
uint16 *ConManagerCreateAcl(const bdaddr *addr);


/*! \brief Release ownership on ACL

    Called to release ownership on an ACL, if that ACL has no other users the
    ACL will be 'closed'.  Bluestack will only actually close the ACL if there
    are no L2CAP connections, so it's safe to call this function once a profile
    connection has been setup, the ACL will be closed automatically once the
    profiles are closed.

    \param[in] addr Pointer to a BT address.
*/
void ConManagerReleaseAcl(const bdaddr *addr);

/*! \brief Disconnect Handset

    This function is called to disconnect all BR/EDR handsets which are
    currently connected except the one whose bluetooth address is being
    passed here in the function. Bluestack will close the ACL and profile
    level connections will be teared down later when ACL close indication
    has been received.

    \param[in] tpaddr Pointer to a BT address.

*/
void ConManagerDisconnectOtherHandset(tp_bdaddr *new_connection);

#ifdef USE_SYNERGY
void ConManagerHandleDmSmAuthoriseInd(const DM_SM_AUTHORISE_IND_T *ind);
#else
/*! Handler for connection library messages not sent directly

    This function is called to handle any connection library messages sent to
    the application that the connection manager is interested in. If a message 
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
bool ConManagerHandleConnectionLibraryMessages(MessageId id,Message message, bool already_handled);
#endif

/*! \brief Handle Authorisation response

    This function is called from services which responds to link authorisation.

    \param[in] authroise    TRUE if accepted else FALSE
    \param[in] bd_addr      Pointer to a BT address.
    \param[in] protocol_id  Protocol for which authorised was requested 
    \param[in] channel      Channel within the protocol which is required to be authorised
    \param[in] incoming     TRUE if incoming connection else FALSE
*/
void ConManagerSendAuthoriseResponse(bool authorise,const typed_bdaddr* bd_addr, dm_protocol_id  protocol_id,uint32 channel,bool incoming);

/*! \brief Register a client task to receive notifications of connections.

    \param[in] client_task Task which will receive CON_MANAGER_CONNECTION_IND message
 */
void ConManagerRegisterConnectionsClient(Task client_task);

/*! \brief Unregister a client task to stop receiving notifications of connections.

    \param[in] client_task Task to unregister.
 */
void ConManagerUnregisterConnectionsClient(Task client_task);

/*! \brief Query if a device is currently connected.

    \param[in] addr Pointer to a BT address.

    \return bool TRUE device is connected, FALSE device is not connected.
 */
bool ConManagerIsConnected(const bdaddr *addr);

/*! \brief Query if a device is currently connecting or connected.

    \param[in] addr Pointer to a BT address.

    \return bool TRUE device is either in connecting or connected, FALSE otherwise.
 */
bool ConManagerIsConnectedOrConnecting(const bdaddr *addr);

/*! \brief Query if a device is currently connected.

    \param[in] tpaddr Pointer to a BT address.

    \return bool TRUE device is connected, FALSE device is not connected.
 */
bool ConManagerIsTpConnected(const tp_bdaddr *tpaddr);

/*! \brief Query if a device is currently connecting or connected.

    \param[in] tpaddr Pointer to a TP BT address.

    \return bool TRUE device is either in connecting or connected, FALSE otherwise.
 */
bool ConManagerIsTpConnectedOrConnecting(const tp_bdaddr *tpaddr);

/*! \brief Query if a ACL to device was locally initiated.

    \param[in] addr Pointer to a BT address.

    \return bool TRUE ACL to device was locally initiated, FALSE is remotely initiated.
 */
bool ConManagerIsAclLocal(const bdaddr *addr);

/*! \brief Query if a ACL to device was locally initiated.

    \param[in] tpaddr Pointer to a BT address.

    \return bool TRUE ACL to device was locally initiated, FALSE is remotely initiated.
 */
bool ConManagerIsTpAclLocal(const tp_bdaddr *tpaddr);

/*! \brief Query if an ACL is presently encrypted.

    \param[in] tpaddr Pointer to a BT address.

    \return bool TRUE ACL is encrypted, FALSE otherwise.
 */
bool ConManagerIsTpAclEncrypted(const tp_bdaddr *tpaddr);

/*! \brief Query if an ACL is presently a Secure Connection.

    \param[in] tpaddr Pointer to a BT address.

    \return bool TRUE ACL is encrypted and Secure, FALSE otherwise.
 */
bool ConManagerIsTpAclSecure(const tp_bdaddr *tpaddr);

/*! \brief Set the link policy per-connection state.

    \param[in] addr     Pointer to a BT address.
    \param[in] lp_state  Address of state to store.
*/
void ConManagerSetLpState(const bdaddr *addr, lpPerConnectionState lp_state);

/*! \brief Set the link policy per-connection state.

    \param[in] addr     Pointer to a #tp_bdaddr address .
    \param[in] lp_state  Address of state to store.
*/
void ConManagerSetLpStateTp(const tp_bdaddr *addr, lpPerConnectionState lp_state);

/*! \brief Get the link policy per-connection state.

    \param[in]  addr    Pointer to a BT address.
    \param[out] lp_state Address of state to update with retrieved state.
*/
bool ConManagerGetLpState(const bdaddr *addr, lpPerConnectionState *lp_state);

/*! \brief Get the link policy per-connection state.

    \param[in]  addr    Pointer to a #tp_bdaddr address 
    \param[out] lp_state Address of state to update with retrieved state.
*/
bool ConManagerGetLpStateTp(const tp_bdaddr *addr, lpPerConnectionState *lp_state);

/*! \brief Get the power mode of the connection

    \param[in]  tpaddr    Pointer to a BT address.
    \param[in]  mode      Pointer variable to receive power mode 
    \return bool returns TRUE if connection is valid, else FALSE
*/
bool ConManagerGetPowerMode(const tp_bdaddr *tpaddr,lp_power_mode* mode);

/*! \brief Get the sniff interval of the connection(in number of slots)

    \param[in]  tpaddr          Pointer to a BT address.
    \param[in]  sniff_interval  Pointer variable to receive sniff interval
    \return bool returns TRUE if connection is valid, else FALSE
*/
bool ConManagerGetSniffInterval(const tp_bdaddr *tpaddr, uint16* sniff_interval);

/*! \brief Get the connection interval (in 1.25ms slots) of the connection.

    \param[in]  tpaddr          Pointer to a BT address.
    \param[in]  conn_interval   Pointer variable to receive connection interval
    \return bool returns TRUE if connection is valid, else FALSE
*/
bool ConManagerGetConnInterval(const tp_bdaddr *tpaddr, uint16* conn_interval);

/*! \brief Get the slave latency (in number of connection events) of the connection.

    \param[in]  tpaddr          Pointer to a BT address.
    \param[in]  slave_latency   Pointer variable to receive slave latency
    \return bool returns TRUE if connection is valid, else FALSE
*/
bool ConManagerGetSlaveLatency(const tp_bdaddr *tpaddr, uint16* slave_latency);

/*! \brief Manually close the ACL to a device.

    \param[in] addr  Pointer to a BT address.
    \param[in] force TRUE aggressively close regardless of L2CAP connectivity.
*/
void ConManagerSendCloseAclRequest(const bdaddr *addr, bool force);


/*! \brief Check if there are any CONNECTED links.

    \return bool TRUE if any link managed by Con Manager is active,
            FALSE if no links

    \note This function ignores links that are connecting or discovering
        services.
 */
bool ConManagerAnyLinkConnected(void);


/*! \brief Control if handset connections are allowed.

    \see ConManagerIsHandsetConnectAllowed to check value

    \param[in] allowed TRUE to allow connections, FALSE to reject.
 */
void ConManagerAllowHandsetConnect(bool allowed);


/*! \brief Find out if handset connections are allowed.

    \see ConManagerAllowHandsetConnect for function to enable/disable

    \return TRUE if connections are allowed, FALSE otherwise.
 */
bool ConManagerIsHandsetConnectAllowed(void);


/*! \brief Control if connections to a transport are allowed.
           BR/EDR enabled by default, BLE disabled by default

    \see ConManagerIsConnectionAllowed for function to check value

    \param[in] transport_mask The transport(s) for which to allow connections
    \param[in] enable TRUE to allow connection on the transports set in
               transport_mask, FALSE to disable connection on the transports 
               set in transport_mask. Transports not set in transport_mask are
               unaffected.
 */
void ConManagerAllowConnection(cm_transport_t transport_mask, bool enable);

/*! \brief Check if connections to a transport are allowed.

    \see ConManagerAllowConnection for function to disable/enable

    \param[in] transport_mask The transport(s) to check
    \return bool TRUE if the all transports set in the transport_mask
            are enabled, otherwise FALSE
 */
bool ConManagerIsConnectionAllowed(cm_transport_t transport_mask);

/*! \brief Control if Handset pairing mode is enabled 

    \see ConManagerIsHandsetPairingMode for function to check value

    \param[in] enable TRUE to allow handset pairing mode, otherwise FALSE
 */

void ConManagerHandsetPairingMode(bool allowed);

/*! \brief Check if device is in handset pairing mode

    \see ConManagerHandsetPairingMode for function to disable/enable

     \return bool TRUE if the handset pairing is allowed, otherwise FALSE
 */
bool ConManagerIsHandsetPairingMode(void);

/*! \brief Create an ACL. As ConManagerCreateAcl but supports
           creation of BLE or BR/EDR ACL. If used to establish
           a BLE ACL this function will pause any le_scan_manager 
           scans in progress until the ACL has either been 
           established or has failed.

    \param[in] tpaddr The transport, type and address of the 
               target device
    \return uint16 Pointer to lock that will be cleared when ACL 
            is available, or paging failed.
 */
uint16 *ConManagerCreateTpAcl(const tp_bdaddr *tpaddr);

/*! \brief Release an ACL along with a reason code.

    \param[in] tpaddr The transport, type and address of the
               target device
    \param[in] hci_reason_code HCI disconnect reason code
 */
void ConManagerReleaseTpAclWithReasonCode(const tp_bdaddr *tpaddr, uint8 hci_reason_code);

/*! \brief Release an ACL. As ConManagerReleaseAcl but supports
           release of BLE or BR/EDR ACL with reason code as HCI_ERROR_OETC_USER

    \param[in] tpaddr The transport, type and address of the 
               target device
 */
void ConManagerReleaseTpAcl(const tp_bdaddr *tpaddr);

/*! \brief Register a client task to receive notifications of 
    connections and disconnections
    
    Clients will receive:
    - CON_MANAGER_TP_CONNECT_IND
    - CON_MANAGER_TP_DISCONNECT_IND
    - CON_MANAGER_TP_DISCONNECT_REQUESTED_IND
    - CON_MANAGER_BLE_PARAMS_UPDATE_IND for BLE connections only

    \param[in] transport_mask The transport(s) to receive notifications for
    \param[in] client_task Task which will receive notifications
 */
void ConManagerRegisterTpConnectionsObserver(cm_transport_t transport_mask, Task client_task);

/*! \brief Unregister a client task to stop receiving notifications of connections and disconnections.
 
    Client will no longer receive messages
    - CON_MANAGER_TP_CONNECT_IND
    - CON_MANAGER_TP_DISCONNECT_IND
    - CON_MANAGER_TP_DISCONNECT_REQUESTED_IND
    - CON_MANAGER_BLE_PARAMS_UPDATE_IND for BLE connections only

    \param[in] transport_mask The transport(s) to unregister from notifications.
    \param[in] client_task Task to unregister from notifications.
*/
void ConManagerUnregisterTpConnectionsObserver(cm_transport_t transport_mask, Task client_task);

/*! \brief Register a client task to receive notifications of 
    connections allowed or dis-allowed
    
    Clients will receive:
    - CON_MANAGER_HANDSET_CONNECT_ALLOW_IND
    - CON_MANAGER_HANDSET_CONNECT_DISALLOW_IND

    \param[in] client_task Task which will receive notifications
 */

void ConManagerRegisterAllowedConnectionsObserver(Task client_task);

/*! \brief Unregister a client task to stop receiving notifications of 
    connections allowed or dis-allowed
    
    Clients will receive:
    - CON_MANAGER_HANDSET_CONNECT_ALLOW_IND
    - CON_MANAGER_HANDSET_CONNECT_DISALLOW_IND

    \param[in] client_task Task to unregister from notifications
 */
void ConManagerUnregisterAllowedConnectionsObserver(Task client_task);


/*! \brief Check if there are any CONNECTED links on a given transport

    \return bool TRUE if any link managed by Con Manager is active
            on the specified transport(s), FALSE if no links

    \note This function ignores links that are connecting or discovering
        services.
 */
bool ConManagerAnyTpLinkConnected(cm_transport_t transport_mask);

/*! \brief Request the QoS to use for new connections
    
           This function may be called multiple times. The QoS setting
           selected will be the highest priority requested. 
           
           The selected QoS setting will be used to select the parameters 
           for locally initiated connections. This QoS setting will be used
           to select parameters to request for remotely initiated 
           connections, although they may not be accepted by a remote central
           device.
    
    \param transport_mask Transport on which to use this QoS setting. 
    
    \param qos The QoS setting
    
    \note Only cm_transport_ble is currently supported
 */
void ConManagerRequestDefaultQos(cm_transport_t transport_mask, cm_qos_t qos);

/*! \brief Request the QoS be updated for a connection

           The current QoS will be re-applied.
           Normally this will have no effect, but if a callback
           is used to adjust parameters - an update may be sent.

           Note that parameters may not be accepted by a remote central
           device.

    \param tpaddr The address of the remote device

 */
void ConManagerUpdateDeviceQos(const tp_bdaddr *tpaddr);

/*! \brief Request to update the QoS to use for a specific device
    
           This function may be called multiple times. The QoS setting
           selected will be the highest priority requested. This will 
           override the default setting regardless of priority.
           
           This QoS setting will be used to select parameters to request, 
           although they may not be accepted by a remote central device.
    
    \param tpaddr The address of the remote device
    
    \param qos The QoS setting
    
    \note Only addresses with TRANSPORT_BLE_ACL are currently supported
 */
void ConManagerRequestDeviceQos(const tp_bdaddr *tpaddr, cm_qos_t qos);

/*! \brief Release the QoS to use for a specific device
    
           This function may be called multiple times. The QoS setting 
           to fall back to will either be the highest priority QoS setting
           requested for this device, or the default if there are no outstanding
           requests
           
           This QoS setting will be used to select parameters to request, 
           although they may not be accepted by a remote central device.
           
           Calling this function for a QoS setting which has not previously 
           been requested will result in a Panic.
    
    \param tpaddr The address of the remote device
    
    \param qos The QoS setting to release
    
    \note Only addresses with TRANSPORT_BLE_ACL are currently supported
 */
void ConManagerReleaseDeviceQos(const tp_bdaddr *tpaddr, cm_qos_t qos);

/*! \brief Set the maximum permitted QoS
    
           Any subsequent requests for a QoS above the maximum will result
           in parameters for the maximum QoS being requested. Any requests
           below or equal to the maximum QoS are unaffected.
           
           When the maximum QoS is changed the connection parameters for any 
           active connections will be updated to use whichever is smaller of 
           the maximum QoS or the requested QoS for the device. If no QoS has 
           been requested for the device then whichever is smaller of the 
           default QoS or the maximum QoS will be used.
    
    \param qos The maximum QoS setting permitted
    
    \note Connection parameter updates for a peripheral device may be rejected
          by the central device.
 */
void ConManagerSetMaxQos(cm_qos_t qos);

/*! \brief Forcibly close any connections with open ACLs

        It should only be called when there has been an attempt to close known 
        connections.

        Sends a CON_MANAGER_CLOSE_ALL_CFM response to all the requester.
        CON_MANAGER_CONNECTION_IND messages may be sent to any registered
        observers

        \note The function does not protect against any new connections
        being created. This should be blocked by calls to 
        ConManagerAllowConnection().

        \param[in] requester task to be sent CON_MANAGER_CLOSE_ALL_CFM 
                message
 */
void ConManagerTerminateAllAcls(Task requester);

/*! \brief Close all BLE links.

    For all open BLE connections:
        * notify any clients that the link will be disconnected, such
          that they can clean up and release any L2CAP channels using
          the BLE ACL.
        * wait for DM_ACL_CLOSED_IND notification from Bluestack

    Once all LE links are disconnected, send CON_MANAGER_DISCONNECT_ALL_LE_CONNECTIONS_CFM
    to the requester task.
        
    \note This API will cleanly disconnect LE links, it *does not* forcibly
    disconnect BLE ACLs.

    \note The function does not protect against any new connections
    being created. This should be blocked by calls to 
    ConManagerAllowConnection().

    \param[in] requester task to be sent CON_MANAGER_DISCONNECT_ALL_LE_CONNECTIONS_CFM 
                message
*/    
void ConManagerDisconnectAllLeConnectionsRequest(Task requester);

/*! \brief Set Qlmp(Qualcomm link manager protocol) connected status for the connection.

    \param[in] addr            Pointer to a BT address.
    \param[in] qlmp_connected  Qlmp connected status to store.
*/  
void ConManagerSetQlmpConnectStatus(const bdaddr *addr, bool qlmp_connected);

/*! \brief Set Qhs(Qualcomm High Speed) supported status for the remote device.

    \param[in] addr            Pointer to a BT address.
    \param[in] qhs_supported   Qhs supported status to store.
*/  
void ConManagerSetQhsSupportStatus(const bdaddr *addr, bool qhs_supported);

/*! \brief Set Qhs(Qualcomm High Speed) connected status for the connection.

    \param[in] addr            Pointer to a BT address.
    \param[in] qhs_connected   Qhs connected status to store.
*/  
void ConManagerSetQhsConnectStatus(const bdaddr *addr, bool qhs_connected);

/*! \brief Get Qhs(Qualcomm High Speed) connected status for the connection.

    \param[in] addr            Pointer to a BT address where qhs connection
                               is to be checked.
*/  
bool ConManagerGetQhsConnectStatus(const bdaddr *addr);

/*! \brief Set Qualcomm fast exit sniff subrate supported status for the remote device.

    \param[in] addr            Pointer to a BT address.
    \param[in] supported       Set if the feature is supported.
*/
void ConManagerSetFastExitSniffSubrateSupportStatus(const bdaddr *addr, bool supported);

/*! \brief Get Qualcomm fast exit sniff subrate supported status for the remote device.
    \param[in] addr            Pointer to a BT address.
    \return TRUE if supported.
*/
bool ConManagerGetFastExitSniffSubrateSupportStatus(const bdaddr *addr);

/*! \brief Register callbacks to allow LE and BREDR parameters to be adjusted.

    \param[in] callback        Function to accept or reject incoming connections
*/  
void ConManager_SetConnParamCallback(con_manager_connparams_callback_t *callback);

/*! \brief Find the first active connection and initialise an iterator

    Finds the first active connection of any type. A #cm_connection_iterator_t is
    initialised and can be used to call ConManager_IterateNextActiveConnection().

    \param[out] iterator    Pointer to an iterator that is populated by this function
    \param[out] addr        Pointer to a #tp_bdaddr address that will be populated if
                            an active connection is found. Pointer can be NULL.

    \return TRUE if there is an active connection, FALSE otherwise
 */
bool ConManager_IterateFirstActiveConnection(cm_connection_iterator_t* iterator, tp_bdaddr *addr);

/*! \brief Given an iterator finds the next active connection, if any

    Finds the next active connection of any type given a #cm_connection_iterator_t 
    initialised by a previous call to ConManager_IterateFirstActiveConnection().

    \param[in,out] iterator Pointer to an initialised iterator
    \param[out] addr        Pointer to a #tp_bdaddr address that will be populated if
                            an active connection is found. Pointer can be NULL.

    \return TRUE if there is an active connection, FALSE otherwise
 */
bool ConManager_IterateNextActiveConnection(cm_connection_iterator_t* iterator, tp_bdaddr *addr);

/*! \brief Resolves the RPA address of LE-ACL link. In case of BREDR-ACL it copies the unaltered tpaddr.

    \param[in] tpaddr Pointer to address of remote device
    \param[out] resolved_tpaddr Pointer to resolved public address of remote device

    \returns FALSE if a LE random address was not resolved successfully. Otherwise TRUE.
        Note that the typed address will be copied from tpaddr to resolved_tpaddr if returns FALSE.
 */
bool ConManagerResolveTpaddr(const tp_bdaddr *tpaddr, tp_bdaddr *resolved_tpaddr);

#ifdef USE_SYNERGY
/*! \brief Update Peer Address.

    \param[in] newPeerAddr     Pointer to a New Peer BT address.
    \param[in] oldPeerAddr     Pointer to a Old Peer BT address.
*/  
void ConManagerUpdatePeerAddress(const bdaddr *newPeerAddr, const bdaddr *oldPeerAddr);

/*! \brief Request to establish LE ACL connection with remote device present in Filter
           Accept List(ie white list).

           Ensure the remote device to connect with is added to Filter Accept List using
           ConManagerSendAddDeviceToLeWhiteListRequest before using this API.
*/
void ConManagerSendAclOpenUseFilterAcceptListRequest(void);

/*! \brief Cancel the ACL open request sent for filter accept list.
           ie, cancel the request sent using ConManagerSendAclOpenUseFilterAcceptListRequest()
*/
void ConManagerCancelAclOpenRequestForFilterAcceptList(void);

/*! \brief Request to clear all the added devices in whitelist
*/
void ConManagerSendClearAllDevicesFromLeWhiteListRequest(void);
#endif

/*! \brief Set a page timeout in connection manager.

    \param[in] page_timeout  Page time used to connect to any AG.

    \note page_timeout is in BT Slots.
*/  
void ConManager_SetPageTimeout(uint16 page_timeout);

/*! \brief Gets connection count for given transport type.

    \param[in] transport  Type of tranport to be searched.

    \return uint8 Number of specified tranport connections.
*/
uint8 ConManagerGetConnectedDeviceCount(TRANSPORT_T transport);

/*! \brief Send add device to LE whitelist request to Bluestack

    \param[in] taddr The address of the remote device
 */
void ConManagerSendAddDeviceToLeWhiteListRequest(typed_bdaddr *taddr);

/*! \brief Send remove device from LE whitelist request to Bluestack

    \param[in] taddr The address of the remote device
 */
void ConManagerSendRemoveDeviceFromLeWhiteListRequest(typed_bdaddr *taddr);

/*! \brief  Enable/Disable connection manager notification filter

    \param[in]  callback function to enable the notification filter ,
                NULL to disable.
*/  
void ConManager_SetNotificationFilter(const con_manager_filter_notification_cb_t callback);

/*! @} */

#endif /* CONNECTION_MANAGER_H_ */
