/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Qualcomm USB Debug over Charger Comms channel
*/

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include "qcom_debug_channel.h"
#include "qcom_debug_channel_messages.h"
#include "bitmap.h"
#include "usb.h"
#include "usbd_qcom_usb_debug_if.h"
#include "usbd_core.h"
#include "wire.h"
#include "timer.h"

/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/

/** Messages sent over the Qualcomm debug case comms channel */
typedef enum
{
    /** A debug data message */
    QCOM_DEBUG_CHANNEL_MSG_DATA = 0,
    /** An unlock message */
    QCOM_DEBUG_CHANNEL_MSG_UNLOCK = 1,
} QCOM_DEBUG_CHANNEL_MSG;

/** The state of the debug channel unlock */
typedef enum
{
    /** No unlocking actions pending */
    QCOM_DEBUG_CHANNEL_STATE_IDLE,
    /** A random token from an earbud has been requested by the USB host */
    QCOM_DEBUG_CHANNEL_STATE_REQUESTED_RANDOM,
    /** The unlock key must be sent to the USB host. */
    QCOM_DEBUG_CHANNEL_STATE_SENDING_UNLOCK_KEY,
    /** An unlock status request must be sent to an earbud. */
    QCOM_DEBUG_CHANNEL_STATE_SENDING_STATUS_REQUEST,
    /** Waiting for a unlock status response from an earbud.*/
    QCOM_DEBUG_CHANNEL_STATE_WAITING_STATUS_RESPONSE,
    /** An lock request must be sent to an earbud. */
    QCOM_DEBUG_CHANNEL_STATE_SENDING_LOCK_REQUEST
} QCOM_DEBUG_CHANNEL_STATE;

/*-----------------------------------------------------------------------------
------------------ VARIABLES --------------------------------------------------
-----------------------------------------------------------------------------*/

static QCOM_DEBUG_CHANNEL_TRANSACTION cc_to_usb_transaction[NO_OF_EARBUDS] = 
{
    {{0},{0}}
};

static QCOM_DEBUG_CHANNEL_STATE debug_channel_state;
static uint8_t debug_channel_current_earbud;
static uint8_t debug_channel_key[USB_WLENGTH_UNLOCK_DEBUGGER];
static tx_vendor_rsp_fn debug_channel_vendor_rsp_callback;
static uint64_t debug_channel_timeout;
static uint8_t right_debug_channel_data_to_forward[CCP_MAX_PAYLOAD_SIZE];
static uint8_t left_debug_channel_data_to_forward[CCP_MAX_PAYLOAD_SIZE];
static bool qcom_debug_forward_left_first = true;

/*-----------------------------------------------------------------------------
------------------ FUNCTIONS --------------------------------------------------
-----------------------------------------------------------------------------*/


/**
 * \brief Attempt to forward any stored debug data to the appropriate USB debug device.
 *
 * \return True if there was data to forward and were successful, false otherwise.
 */
static bool qcom_debug_channel_forward_to_usb(void)
{
    uint8_t i;

    for (i = 0; i < NO_OF_EARBUDS; i++)
    {
        uint8_t earbud = qcom_debug_forward_left_first ? i : NO_OF_EARBUDS - 1 - i;

        if (cc_to_usb_transaction[earbud].length)
        {
            if (usb_debug_tx(cc_to_usb_transaction[earbud].earbud,
                             cc_to_usb_transaction[earbud].tx_data,
                             cc_to_usb_transaction[earbud].length) == USBD_OK)
            {
                PRINTF_B("usb tx (%c): %u", earbud == EARBUD_LEFT ? 'L':'R', cc_to_usb_transaction[earbud].length);
                cc_to_usb_transaction[earbud].length = 0;
                qcom_debug_forward_left_first = !qcom_debug_forward_left_first;
                return true;
            }
        }
    }
    qcom_debug_forward_left_first = !qcom_debug_forward_left_first;

    return false;
}

/**
 * \brief Handle incoming debug RX data from the Qualcomm USB debug case comms channel.
 * 
 * \param earbud The earbud this debug data has come from
 * \param data A pointer to the debug data
 * \param len The number of octets of data.
 */
static void qcom_debug_channel_handle_rx_data(uint8_t earbud, uint8_t *data, uint16_t len)
{
    uint8_t *buf = (earbud == EARBUD_LEFT ? left_debug_channel_data_to_forward : right_debug_channel_data_to_forward);

    /* Copy and store information about the data to forward */
    memcpy(buf, data, len * sizeof(*data));
    cc_to_usb_transaction[earbud].tx_data = buf;
    cc_to_usb_transaction[earbud].earbud = earbud;
    cc_to_usb_transaction[earbud].length = len;

    /* Attempt to forward data to USB, if we fail we will try again in the
     * periodic function */
    if (!qcom_debug_channel_forward_to_usb())
    {
        PRINTF_B("usb busy for earbud %u", earbud);
    }
}

/**
 * \brief Handle a incoming debug unlock message from the Qualcomm USB debug
 *        case comms channel.
 * 
 * \param earbud The earbud this debug unlock message has come from
 * \param data A pointer to the debug data
 * \param len The number of octets of data.
 */
static void qcom_debug_channel_handle_rx_unlock(uint8_t earbud, uint8_t *data, uint16_t len)
{
    uint16_t header = (uint16_t)data[CHARGER_COMMS_DEBUG_UNLOCK_HEADER_OFFSET];

    /* Check that this request is valid */
    if (debug_channel_current_earbud != earbud)
    {
        return;
    }

    switch (CHARGER_COMMS_DEBUG_UNLOCK_ACTION_GET(header))
    {
    case CHARGER_COMMS_DEBUG_UNLOCK_ACTION_READ_RANDOM_TOKEN:

        if (len != CHARGER_COMMS_DEBUG_UNLOCK_HEADER_SIZE + USB_WLENGTH_UNLOCK_DEBUGGER)
        {
            return;
        }

        /* Call callback to transmit the random unlock token over USB*/
        if (debug_channel_vendor_rsp_callback)
        {
            debug_channel_vendor_rsp_callback(earbud,
                                          &data[CHARGER_COMMS_DEBUG_UNLOCK_HEADER_SIZE],
                                          len - CHARGER_COMMS_DEBUG_UNLOCK_HEADER_SIZE);
        }

        break;

    case CHARGER_COMMS_DEBUG_UNLOCK_ACTION_READ_UNLOCK_STATUS:
        if (len != CHARGER_COMMS_DEBUG_UNLOCK_HEADER_SIZE)
        {
            return;
        }

        bool is_unlocked = CHARGER_COMMS_DEBUG_UNLOCK_STATUS_GET(header);
        data[0] = is_unlocked ? USB_DEBUG_UNLOCK_STATUS_UNLOCKED :
                                USB_DEBUG_UNLOCK_STATUS_LOCKED;

        /* Call callback to transmit the unlock status over USB*/
        if (debug_channel_vendor_rsp_callback)
        {
            debug_channel_vendor_rsp_callback(earbud,
                                              data,
                                              USB_WLENGTH_UNLOCK_STATUS);
        }

        debug_channel_state = QCOM_DEBUG_CHANNEL_STATE_IDLE;
        break;

    default:
        return;
    }
}

void qcom_debug_channel_rx(uint8_t earbud, uint8_t msg_id, uint8_t *data, uint16_t len)
{
    QCOM_DEBUG_CHANNEL_MSG msg = (QCOM_DEBUG_CHANNEL_MSG)msg_id;

    switch(msg)
    {
        case QCOM_DEBUG_CHANNEL_MSG_DATA:
            qcom_debug_channel_handle_rx_data(earbud, data, len);
            break;

        case QCOM_DEBUG_CHANNEL_MSG_UNLOCK:
            qcom_debug_channel_handle_rx_unlock(earbud, data, len);
            break;

        default:
            break;
    }
}

bool qcom_debug_channel_data_tx(uint8_t earbud, uint8_t *data, uint16_t len)
{
    return ccp_tx((uint8_t)QCOM_DEBUG_CHANNEL_MSG_DATA, CCP_CH_QCOM_DEBUG_CHANNEL,
                  wire_dest[earbud], data, len, true);
}

/**
 * \brief Transmit a random token request to an earbud over the Qualcomm Debug
 *        channel. The earbud is selected by a call to
 *        \a qcom_debug_channel_request_random_token
 */
static bool qcom_debug_channel_tx_random_token_req(void)
{
    uint16_t header;
    uint8_t data[CHARGER_COMMS_DEBUG_UNLOCK_HEADER_SIZE];

    CHARGER_COMMS_DEBUG_UNLOCK_ACTION_SET(header,
        CHARGER_COMMS_DEBUG_UNLOCK_ACTION_READ_RANDOM_TOKEN);
    data[CHARGER_COMMS_DEBUG_UNLOCK_HEADER_OFFSET] = header & 0xFF;
    data[CHARGER_COMMS_DEBUG_UNLOCK_HEADER_OFFSET+1] = (header >> 8) & 0xFF;

    return ccp_tx((uint8_t)QCOM_DEBUG_CHANNEL_MSG_UNLOCK,
                           CCP_CH_QCOM_DEBUG_CHANNEL,
                           wire_dest[debug_channel_current_earbud],
                           data,
                           CHARGER_COMMS_DEBUG_UNLOCK_HEADER_SIZE,
                           true);
}

/**
 * \brief Transmit the stored unlock key to an earbud over the Qualcomm Debug
 *        channel. The earbud is selected by a call to
 *        \a qcom_debug_channel_unlock_key_tx
 */
static bool qcom_debug_channel_tx_unlock_key(void)
{
    uint16_t header;
    uint8_t data[CHARGER_COMMS_DEBUG_UNLOCK_HEADER_SIZE + USB_WLENGTH_UNLOCK_DEBUGGER];

    CHARGER_COMMS_DEBUG_UNLOCK_ACTION_SET(header,
        CHARGER_COMMS_DEBUG_UNLOCK_ACTION_WRITE_UNLOCK_TOKEN);
    data[CHARGER_COMMS_DEBUG_UNLOCK_HEADER_OFFSET] = header & 0xFF;
    data[CHARGER_COMMS_DEBUG_UNLOCK_HEADER_OFFSET+1] = (header >> 8) & 0xFF;

    memcpy(&data[CHARGER_COMMS_DEBUG_UNLOCK_HEADER_SIZE],
           debug_channel_key,
           USB_WLENGTH_UNLOCK_DEBUGGER);

    return ccp_tx((uint8_t)QCOM_DEBUG_CHANNEL_MSG_UNLOCK,
                           CCP_CH_QCOM_DEBUG_CHANNEL,
                           wire_dest[debug_channel_current_earbud],
                           data,
                           sizeof(data),
                           true);
}

/**
 * \brief Transmit a unlock status request over the Qualcomm Debug channel
 * The earbud is selected by a call to \a qcom_debug_channel_unlock_status_req.
 */
static bool qcom_debug_channel_tx_unlock_status_req(void)
{
    uint16_t header;
    uint8_t data[CHARGER_COMMS_DEBUG_UNLOCK_HEADER_SIZE];

    CHARGER_COMMS_DEBUG_UNLOCK_ACTION_SET(header,
        CHARGER_COMMS_DEBUG_UNLOCK_ACTION_READ_UNLOCK_STATUS);
    data[CHARGER_COMMS_DEBUG_UNLOCK_HEADER_OFFSET] = header & 0xFF;
    data[CHARGER_COMMS_DEBUG_UNLOCK_HEADER_OFFSET+1] = (header << 8) & 0xFF;

    return ccp_tx((uint8_t)QCOM_DEBUG_CHANNEL_MSG_UNLOCK,
                           CCP_CH_QCOM_DEBUG_CHANNEL,
                           wire_dest[debug_channel_current_earbud],
                           data,
                           sizeof(data),
                           true);
}

/**
 * \brief Transmit a lock request over the Qualcomm Debug channel
 * The earbud is selected by a call to \a qcom_debug_channel_lock_req
 */
static bool qcom_debug_channel_tx_lock_req(void)
{
    uint16_t header;
    uint8_t data[CHARGER_COMMS_DEBUG_UNLOCK_HEADER_SIZE];

    CHARGER_COMMS_DEBUG_UNLOCK_ACTION_SET(header,
        CHARGER_COMMS_DEBUG_UNLOCK_ACTION_LOCK_DEBUG);
    data[CHARGER_COMMS_DEBUG_UNLOCK_HEADER_OFFSET] = header & 0xFF;
    data[CHARGER_COMMS_DEBUG_UNLOCK_HEADER_OFFSET+1] = (header << 8) & 0xFF;

    return ccp_tx((uint8_t)QCOM_DEBUG_CHANNEL_MSG_UNLOCK,
                           CCP_CH_QCOM_DEBUG_CHANNEL,
                           wire_dest[debug_channel_current_earbud],
                           data,
                           sizeof(data),
                           true);
}

/**
 * \brief Perform any pending action regarding Qualcomm debug channel unlocking
 */
static void qcom_debug_channel_handle_unlock(void)
{
    switch(debug_channel_state)
    {
        case QCOM_DEBUG_CHANNEL_STATE_REQUESTED_RANDOM:
            if (qcom_debug_channel_tx_random_token_req())
            {
                debug_channel_state = QCOM_DEBUG_CHANNEL_STATE_IDLE;
            }
            break;

        case QCOM_DEBUG_CHANNEL_STATE_SENDING_UNLOCK_KEY:
            if (qcom_debug_channel_tx_unlock_key())
            {
                debug_channel_state = QCOM_DEBUG_CHANNEL_STATE_IDLE;
            }
            break;

        case QCOM_DEBUG_CHANNEL_STATE_SENDING_STATUS_REQUEST:
            if (qcom_debug_channel_tx_unlock_status_req() ||
                global_time_us > debug_channel_timeout)
            {
                debug_channel_state = QCOM_DEBUG_CHANNEL_STATE_WAITING_STATUS_RESPONSE;
            }

            break;

        case QCOM_DEBUG_CHANNEL_STATE_WAITING_STATUS_RESPONSE:
            /* If we time-out retreiving a response from the earbud, send UNKNOWN */
            if (global_time_us > debug_channel_timeout)
            {
                uint8_t data[USB_WLENGTH_UNLOCK_STATUS];
                data[0] = USB_DEBUG_UNLOCK_STATUS_UNKNOWN;

                /* Call callback to transmit the unlock status over USB*/
                if (debug_channel_vendor_rsp_callback)
                {
                    debug_channel_vendor_rsp_callback(debug_channel_current_earbud,
                                                      data,
                                                      USB_WLENGTH_UNLOCK_STATUS);
                }
                debug_channel_state = QCOM_DEBUG_CHANNEL_STATE_IDLE;
            }
            break;

        case QCOM_DEBUG_CHANNEL_STATE_SENDING_LOCK_REQUEST:
            if (qcom_debug_channel_tx_lock_req())
            {
                debug_channel_state = QCOM_DEBUG_CHANNEL_STATE_IDLE;
            }
            break;

        case QCOM_DEBUG_CHANNEL_STATE_IDLE:
        default:
            break;
    }
}

void qcom_debug_channel_request_random_token(uint8_t earbud, tx_vendor_rsp_fn fn)
{
    debug_channel_state = QCOM_DEBUG_CHANNEL_STATE_REQUESTED_RANDOM;
    debug_channel_current_earbud = earbud;
    debug_channel_vendor_rsp_callback = fn;
}

void qcom_debug_channel_unlock_key_tx(uint8_t earbud, uint8_t *key, uint16_t len)
{
    (void)len;

    debug_channel_state = QCOM_DEBUG_CHANNEL_STATE_SENDING_UNLOCK_KEY;
    debug_channel_current_earbud = earbud;
    memcpy(debug_channel_key,
           key,
           USB_WLENGTH_UNLOCK_DEBUGGER);
}

void qcom_debug_channel_unlock_status_req(uint8_t earbud, tx_vendor_rsp_fn fn)
{
    debug_channel_state = QCOM_DEBUG_CHANNEL_STATE_SENDING_STATUS_REQUEST;
    debug_channel_current_earbud = earbud;
    debug_channel_vendor_rsp_callback = fn;
    debug_channel_timeout = global_time_us + USB_DEBUG_UNLOCK_STATUS_TIMEOUT_US;
}

void qcom_debug_channel_lock_req(uint8_t earbud)
{
    debug_channel_state = QCOM_DEBUG_CHANNEL_STATE_SENDING_LOCK_REQUEST;
    debug_channel_current_earbud = earbud;
}

void qcom_debug_channel_periodic(void)
{
    /* Do any pending debugger unlock actions */
    qcom_debug_channel_handle_unlock();

    /* Try to forward any stored debug data to USB */
    (void)qcom_debug_channel_forward_to_usb();
}
