/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */
/****************************************************************************
FILE
    charger_comms_if.h

CONTAINS
    Definitions for the charger communication subsystem.

DESCRIPTION
    This file is seen by the stack, and VM applications, and
    contains things that are common between them.
*/

#ifndef __APP_CHARGER_COMMS_IF_H__
#define __APP_CHARGER_COMMS_IF_H__

#include "app/uart/uart_if.h"

/*! @brief Status of the previous charger comms message sent by the ChargerCommsTransmit() trap.*/
typedef enum
{
    /*! The message was sent and acknowledged by the recipient successfully.*/
    CHARGER_COMMS_MSG_SUCCESS,

    /*! Charger was removed during message transmission. */
    CHARGER_COMMS_MSG_INTERRUPTED,

    /*! The message was not acknowledged by the recipient. */
    CHARGER_COMMS_MSG_FAILED,

    /*! The message request was rejected as the transmit queue was full. */
    CHARGER_COMMS_MSG_QUEUE_FULL,

    /*! An unexpected error occurred. */
    CHARGER_COMMS_MSG_UNKNOWN_ERROR
} charger_comms_msg_status;


/*! The ChargerComms message includes ChargerComms header, 16-bit CRC and optional
 * ChargerComms payload. ChargerComms header is derived from message header
 * written into ChargerCommsUart stream. The maximum message payload length that can
 * be written into ChargerCommsUart is 378 octets. ChargerComms header and 16-bit CRC
 * are appended to the message payload before transmission to destination address.
 */
#define CHARGER_COMMS_UART_MSG_MAX_LENGTH_OCTETS (383)
/*! ChargerComms header length in octet */
#define CHARGER_COMMS_UART_MSG_CHARGER_COMMS_HEADER_LENGTH_OCTETS (2)
/*! CaseComms header length in octet */
#define CHARGER_COMMS_UART_MSG_CASE_COMMS_HEADER_LENGTH_OCTETS (1)
/*! CRC in octet */
#define CHARGER_COMMS_UART_MSG_CRC_LENGTH_OCTETS (2)
/*! The minimum length of ChargerComms message in octet */
#define CHARGER_COMMS_UART_MSG_MIN_LENGTH_OCTETS \
    (CHARGER_COMMS_UART_MSG_CHARGER_COMMS_HEADER_LENGTH_OCTETS + \
     CHARGER_COMMS_UART_MSG_CRC_LENGTH_OCTETS)
/* The Maximum length in octet of the CaseComms payload
 * excluding case comms header. */
 /*! The maximum length of CaseComms payload in octet */
#define CHARGER_COMMS_UART_MSG_CASE_COMMS_PAYLOAD_MAX_LENGTH_OCTETS  \
    (CHARGER_COMMS_UART_MSG_MAX_LENGTH_OCTETS - \
     (CHARGER_COMMS_UART_MSG_CHARGER_COMMS_HEADER_LENGTH_OCTETS + \
      CHARGER_COMMS_UART_MSG_CASE_COMMS_HEADER_LENGTH_OCTETS + \
      CHARGER_COMMS_UART_MSG_CRC_LENGTH_OCTETS))

/*! @brief Device id for ChargerCommsUart used by the ChargerCommsUartConfigure() trap.
 *
 * The supported device ID includes case, earbud right and earbud left.
 */
typedef enum
{
    /*! case */
    CHARGER_COMMS_UART_DEVICE_ID_CASE,
     /*! earbud right */
    CHARGER_COMMS_UART_DEVICE_ID_EB_R,
     /*! earbud left */
    CHARGER_COMMS_UART_DEVICE_ID_EB_L,
     /*! Number of the supported device ID */
    CHARGER_COMMS_UART_DEVICE_ID_MAX
}charger_comms_uart_device_id_t;

/*! @brief Destination address for ChargerCommsUart stream header. */
typedef enum
{
    /*! Case */
    CHARGER_COMMS_UART_ADDRESS_CASE      = 0,
    /*! Right Earbud */
    CHARGER_COMMS_UART_ADDRESS_EB_R      = 1,
    /*! Left Earbud */
    CHARGER_COMMS_UART_ADDRESS_EB_L      = 2,
    /*! Broadcast (right and left earbuds) */
    CHARGER_COMMS_UART_ADDRESS_BROADCAST = 3,
    /*! Invalid address */
    CHARGER_COMMS_UART_ADDRESS_NONE
} charger_comms_uart_address;

/*! @brief Configuration key used by the ChargerCommsUartConfigure() trap. */
typedef enum
{
    /*! This key is used to enable/disable UART Rx.
     * 0: Disable, 1: Enable. */
    CHARGER_COMMS_UART_CFG_KEY_RX_ENABLE,
    /*! This key is used to set the device id.
     * The valid configuration value refers to charger_comms_uart_device_id_t. */
    CHARGER_COMMS_UART_CFG_KEY_DEVICE_ID,
    /*! This key is used to set baud rate of UART.
     * The configuration value refers to vm_uart_rate. */
    CHARGER_COMMS_UART_CFG_KEY_BAUD_RATE,
    /*! This key is used to set the parity bit of UART.
     * The configuration value refers to vm_uart_parity. */
    CHARGER_COMMS_UART_CFG_KEY_PARITY,
    /*! This key is used to set the stop bits of UART.
     * The configuration value refers to vm_uart_stop. */
    CHARGER_COMMS_UART_CFG_KEY_STOP_BITS,
    /*! This key is used to set the tx time out of UART. */
    CHARGER_COMMS_UART_CFG_KEY_TIME_OUT,
    /*! This key is used to set rx idle time out in us of UART. */
    CHARGER_COMMS_UART_CFG_KEY_RX_IDLE_TIME_TOUT,
    /*! Sets whether charger detection messages should be suppressed. */
    CHARGER_COMMS_UART_CFG_KEY_SUPPRESS_CHARGER_DETECT,
    /*! Time in microseconds to delay between replying to a case message. */
    CHARGER_COMMS_UART_CFG_KEY_REPLY_DELAY,
    /*! Enable or disable debug over charger comms feature */
    CHARGER_COMMS_UART_CFG_KEY_DEBUGGER_ENABLE,
    /*! Configure special compatibility mode */
    CHARGER_COMMS_UART_CFG_KEY_COMPAT_MODE
}charger_comms_uart_config_key_t;

/*! @brief Status of the transmission of ChargerComms message via ChargerCommsUart stream.*/
typedef enum
{
    /*! The message was sent and acknowledged by the recipient successfully.*/
    CHARGER_COMMS_UART_TX_SUCCESS,

    /*! The message was not acknowledged by the recipient. */
    CHARGER_COMMS_UART_TX_FAILED,

    /*! The message request was rejected as the request is invalid
     * (i.e invalid packet type, or invalid destination ID etc)
     */
    CHARGER_COMMS_UART_TX_INVALID_REQ,

    /*! The message request was rejected as the UART HW tx buffer did not have
     * enough space or no free MSG for buffer
     */
    CHARGER_COMMS_UART_TX_HW_BUFFER_FULL,

    /*! The message failed to acknowledged by the recipient within the transmit time out.
     */
    CHARGER_COMMS_UART_TX_TIMEOUT
} charger_comms_uart_tx_status;

/*! @brief ChargerComms message type to be transmitted.
 *  Only CHARGER_COMMS_UART_MSG_TYPE_COMMS_DATA allowed in earbud configuration.
 */
typedef enum
{
    /*!ChargerComms message with CaseComms Header plus optional payload */
    CHARGER_COMMS_UART_MSG_TYPE_COMMS_DATA,

    /*!ChargerComms message without CaseComms Header and payload */
    CHARGER_COMMS_UART_MSG_TYPE_POLL
} charger_comms_uart_msg_type;

/*! @brief Definition of Message header used for ChargerCommsUart Sink stream..
 *
 * List of supported ChargerComms message types:
 *
 * 1. Poll (sent by Case device) without CaseComms header and payload.
 *        dest_address : CHARGER_COMMS_UART_ADDRESS_EB_R/
 *                       CHARGER_COMMS_UART_ADDRESS_EB_L
 *        type         : CHARGER_COMMS_UART_MSG_TYPE_POLL
 *
 * 2. Broadcast without CaseComms header and payload.
 *        dest_address : CHARGER_COMMS_UART_ADDRESS_BROADCAST
 *        type         : CHARGER_COMMS_UART_MSG_TYPE_POLL
 *
 * 3. Broadcast with ChargerComms header plus optional payload.
 *        dest_address : CHARGER_COMMS_UART_ADDRESS_BROADCAST
 *        type         : CHARGER_COMMS_UART_MSG_TYPE_COMMS_DATA
 *
 * 4. Normal Data with CaseComms header plus optional payload.
 *        dest_address : CHARGER_COMMS_UART_ADDRESS_EB_R /
 *                       CHARGER_COMMS_UART_ADDRESS_EB_R /
 *                       CHARGER_COMMS_UART_ADDRESS_CASE
 *        type         : CHARGER_COMMS_UART_MSG_TYPE_COMMS_DATA
*
 */
typedef struct charger_comms_uart_tx_msg_header_t
{
    /*! ChargerComm message type */
    charger_comms_uart_msg_type type;
    /*! destination address which the message should be sent to */
    charger_comms_uart_address dest_address;
    /*! CaseComms Header if the message includes the CaseComms header
     *  Set header to be 0x00 if the message does not contain CaseComms header */
    uint8 header;
} charger_comms_uart_tx_msg_header_t;

/*! @brief Message header definition for receiving ChargerCommsUart message. */
typedef struct charger_comms_uart_rx_msg_header_t
{
    /*! source id which the received message is from */
    charger_comms_uart_address src_address;
} charger_comms_uart_rx_msg_header_t;


#define CHARGER_COMMS_DIAGNOSTICS_HISTORY_SIZE (8)

typedef enum
{
    /** Received packets */

    /*! A placeholder that represents an invalid entry */
    CHARGER_COMMS_MSG_STATUS_NONE,
    /*! The raw length of this packet or the length field is invalid. */
    CHARGER_COMMS_MSG_STATUS_INVALID_LENGTH,
    /*! An unexpected source has sent a message to this device.*/
    CHARGER_COMMS_MSG_STATUS_INVALID_SOURCE,
    /*! A packet that is not destined for this device */
    CHARGER_COMMS_MSG_STATUS_INVALID_DESTINATION,
    /*! The CRC validation failed on this packet */
    CHARGER_COMMS_MSG_STATUS_CRC_FAILED,
    /*! The incoming packet cannot be processed due to the RX buffer running
     * out of space */
    CHARGER_COMMS_MSG_STATUS_NO_SPACE_TO_RX,
    /* A valid packet */
    CHARGER_COMMS_MSG_STATUS_VALID_RX,


    /** Transmitted packets */

    /*! Reply with a positive acknowledgement that the last packet received
     * was valid. */
    CHARGER_COMMS_MSG_STATUS_VALID_REPLY_WITH_ACK,
    /*! Negative acknowledgement to the last packet received */
    CHARGER_COMMS_MSG_STATUS_VALID_REPLY_WITH_NAK,
    /*! Retransmit of the previous packet */
    CHARGER_COMMS_MSG_STATUS_VALID_REPLY_WITH_RETRANSMIT,
    /*! A transmitted priority packet */
    CHARGER_COMMS_MSG_STATUS_VALID_REPLY_PRIORITY

} charger_comms_msg_diag_status;

typedef enum
{
    /*! A placeholder that represents an invalid entry */
    CHARGER_COMMS_ATTACHED_STATUS_PLACEHOLDER,
    /*! A charger has been attached. */
    CHARGER_COMMS_ATTACHED_STATUS_ATTACHED,
    /*! A charger change is in progress.*/
    CHARGER_COMMS_ATTACHED_STATUS_PENDING,
    /*! A charger has been detached. */
    CHARGER_COMMS_ATTACHED_STATUS_DETACHED
} charger_comms_attached_status;

typedef struct charger_comms_msg
{
    /*! Time that this message was received or sent */
    uint32 time;

    /*! The status of this packet. */
    charger_comms_msg_diag_status status;
    
    /*! The charger comm header of this packet */
    uint16 charger_comms_header;

    /*! The case comms header of this packet */
    uint8 case_comms_header;
    
} charger_comms_msg;

typedef struct charger_comms_attach_report
{
    /*! Time that this attach change occurred. */
    uint32 time;

    /*! The charger attach status at this time */
    charger_comms_attached_status status;

} charger_comms_attach_report;

typedef struct
{
    /*! Information about the last valid case comms message that has been received */
    charger_comms_msg last_case_comm_msg;

    /*! History of last charger comms message received and transmitted */
    charger_comms_msg history[CHARGER_COMMS_DIAGNOSTICS_HISTORY_SIZE];

    /*! The next index to be written into the history buffer. */
    size_t history_next;

    /*! History of the last Charger detection attach/detaches */
    charger_comms_attach_report attached[CHARGER_COMMS_DIAGNOSTICS_HISTORY_SIZE];

    /*! The next index to be written into the attach history buffer */
    size_t attached_next;
} charger_comms_diagnostics;

#endif /* __APP_CHARGER_COMMS_IF_H__  */

