/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       connection_manager_notify.h
\brief      Header file for Connection Manager Notify
*/

#ifndef __CON_MANAGER_NOTIFY_H
#define __CON_MANAGER_NOTIFY_H

#include <connection_manager.h>

/*! \brief Transports for which connections are managed */
typedef enum
{
    /*! Notification message for outgoing connection created */
    cm_notify_message_connected_outgoing   = 0,
    /*! Connected notification message */
    cm_notify_message_connected_incoming,
    /*! Disconnected notification message */
    cm_notify_message_disconnected,
    /*! Disconnect requested message */
    cm_notify_message_disconnect_requested
} cm_notify_message_t;

/*! \brief Handset connection allowed status */
typedef enum
{
    cm_handset_allowed,
    cm_handset_disallowed
} con_manager_allowed_notify_t;

/*! \brief Notify observers of connection/disconnection
    \param tpaddr The Transport Bluetooth Address of the connection
    \param connected TRUE if connected, FALSE if disconnected
    \param reason Reason for disconnection if disconnected
*/
void conManagerNotifyObservers(const tp_bdaddr *tpaddr, cm_notify_message_t notify_message, hci_status reason);

/*! \brief Notify observers of connections being allowed or not
    \param notify Connection allowed status
*/
void conManagerNotifyAllowedConnectionsObservers(con_manager_allowed_notify_t notify);

/*! \brief Notify observers of connection parameter changes
    \param tpaddr The Transport Bluetooth Address of the connection
    \param conn_interval The updated connection interval
    \param conn_lantecy The updated slave latency
*/
void conManagerNotifyConnParamsObservers(const tp_bdaddr *tpaddr, uint16 conn_interval, uint16 conn_lantecy);

/*! \brief Notify observers to authorise incoming connections.
     \param bd_addr The BT address of the remote device
     \param protocol_id The protocol (L2CAP, RFCOMM, etc)
     \param channel The channel number
     \param incoming TRUE if connection was initiated by the remote device, otherwise FALSE
*/
void conManagerNotifyAuthReqObservers(const typed_bdaddr* bd_addr, dm_protocol_id protocol_id, uint32 channel, bool incoming);


/*! \brief Initialise notify */
void conManagerNotifyInit(void);

#endif
