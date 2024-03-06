/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Charger Case Protocol / Case Comms Protocol
*/

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>
#include "main.h"
#include "wire.h"
#include "bitmap.h"
#include "ccp.h"
#include "timer.h"
#include "case_channel.h"

#ifdef USB_QCOM_DEBUG_DEVICE
#include "qcom_debug_channel.h"
#endif

/*-----------------------------------------------------------------------------
------------------ PREPROCESSOR DEFINITIONS -----------------------------------
-----------------------------------------------------------------------------*/

/*
 * CCP_POLL_US: The default period between polls in microseconds.
 * This is used to set CCP_TRANSACTION.poll_period_time
 */
#ifdef SCHEME_A
#define CCP_POLL_US (250 * 1000)
#else
#define CCP_POLL_US (1220)
#endif

/*
* CCP_MAX_POLLS: Maximum number of polls before giving up.
*/
#define CCP_MAX_POLLS 6

#define CCP_MAX_MSG_SIZE CCP_MAX_PAYLOAD_SIZE

/*
* Case Comms Header field definitions.
*/
#define CCP_HDR_MASK_CHAN_ID 0xF0
#define CCP_HDR_BIT_CHAN_ID  4
#define CCP_HDR_MASK_MSG_ID  0x0F
#define CCP_HDR_BIT_MSG_ID   0
#define CCP_HDR_SIZE         1
#define CCP_HDR_OFFSET       0

/*
 * Number of retries
 */
#define CCP_RETRY_COUNT 1
#define CCP_ALLOWED_FAILED_RETIES 30

/* Case Comms Headers info */
#define CASE_COMMS_HEADER_BYTE 1


/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/

typedef struct
{
    bool busy;
    uint64_t poll_timeout;
    uint64_t poll_period_time;
    uint8_t poll_count;

    uint8_t retries;
    uint8_t failed_retries;
    uint8_t tx_buf[CCP_MAX_MSG_SIZE];
    uint8_t tx_len;
    CCP_CHANNEL channel;
}
CCP_TRANSACTION;

/*-----------------------------------------------------------------------------
------------------ PROTOTYPES -------------------------------------------------
-----------------------------------------------------------------------------*/

static void ccp_rx(uint8_t earbud, uint8_t *data, uint16_t len, bool final_piece);
static void ccp_ack(uint8_t earbud);
static void ccp_nack(uint8_t earbud);
static void ccp_give_up(uint8_t earbud, bool retries_fail);
static void ccp_no_response(uint8_t earbud);
static void ccp_abort(uint8_t earbud);
static void ccp_broadcast_finished(void);

/*-----------------------------------------------------------------------------
------------------ VARIABLES --------------------------------------------------
-----------------------------------------------------------------------------*/

static const CCP_USER_CB *ccp_user[NUMBER_OF_CCP_CHANNELS];

static const WIRE_USER_CB ccp_wire_cb =
{
    ccp_rx,
    ccp_ack,
    ccp_nack,
    ccp_give_up,
    ccp_no_response,
    ccp_abort,
    ccp_broadcast_finished
};

static CCP_TRANSACTION ccp_transaction[NO_OF_EARBUDS] = {0};

/*
 * The following functions are dummy CCP user functions which we initialise
 * ccp_user with.
 */

static void dummy_rx(uint8_t earbud, uint8_t msg_id, uint8_t *data, uint16_t len)
{
    (void)earbud;
    (void)msg_id;
    (void)data;
    (void)len;
}

static void dummy_give_up(uint8_t earbud, bool retries_fail)
{
    (void)earbud;
    (void)retries_fail;
}

static void dummy_ack(uint8_t earbud)
{
    (void)earbud;
}

#define dummy_nack dummy_ack
#define dummy_no_response dummy_ack
#define dummy_abort dummy_ack
static void dummy_broadcast_finished(void) {}

static const CCP_USER_CB dummy_ccp_cb =
{
    dummy_rx,
    dummy_ack,
    dummy_nack,
    dummy_give_up,
    dummy_no_response,
    dummy_abort,
    dummy_broadcast_finished
};

/*-----------------------------------------------------------------------------
------------------ FUNCTIONS --------------------------------------------------
-----------------------------------------------------------------------------*/

/*
* Send a message to an earbud.
*/
bool ccp_tx(
    uint8_t msg_id,
    CCP_CHANNEL chan,
    WIRE_DESTINATION dest,
    uint8_t *data,
    uint16_t len,
    bool need_answer)
{
    bool ret = false;

    if ((len + 2) <= CCP_MAX_MSG_SIZE)
    {
        uint8_t tx_buf[CCP_MAX_MSG_SIZE];
        uint8_t *buf = tx_buf;

        if (dest != WIRE_DEST_BROADCAST)
        {
            CCP_TRANSACTION *ct =
                &ccp_transaction[(dest==WIRE_DEST_LEFT) ? EARBUD_LEFT:EARBUD_RIGHT];
            buf = ct->tx_buf;
            ct->tx_len = len+1;
        }

        /*
        * Case Comms Header.
        */
        buf[0] = BITMAP_SET(CCP_HDR, CHAN_ID, chan) |
                 BITMAP_SET(CCP_HDR, MSG_ID, msg_id);

        if (data && len)
        {
            memcpy(&buf[1], data, len);
        }

        if (dest==WIRE_DEST_BROADCAST)
        {
            CCP_TRANSACTION left = ccp_transaction[EARBUD_LEFT];
            CCP_TRANSACTION right = ccp_transaction[EARBUD_RIGHT];

#ifdef SCHEME_A
            if (left.busy)
            {
                ccp_abort(EARBUD_LEFT);
            }
            if (right.busy)
            {
                ccp_abort(EARBUD_RIGHT);
            }
            /* Store the channel info in either of the earbud's ccp_transaction
             * variable as Broadcast messages are not ACK'd or NAK'd for any earbud.
             * Currently, the channel id is stored for left earbud, and 
             * accordingly the fetching of channel id for broadcast purpose
             * is done from left earbud ccp_transaction variable.
             */
            ccp_transaction[EARBUD_LEFT].channel= chan;

            ret = wire_tx(dest, buf, len+1);
#else
            /* Only send a broadcast if we not in an active transfer with
             * either of the earbuds. */
            if (!left.busy && !right.busy)
            {
                /* Store the channel info in either of the earbud's ccp_transaction
                 * variable as Broadcast messages are not ACK'd or NAK'd for any earbud.
                 * Currently, the channel id is stored for left earbud, and 
                 * accordingly the fetching of channel id for broadcast purpose
                 * is done from left earbud ccp_transaction variable.
                 */
                ccp_transaction[EARBUD_LEFT].channel= chan;

                ret = wire_tx(dest, buf, len+1);
            }
#endif

        }
        else
        {
            CCP_TRANSACTION *ct =
                &ccp_transaction[(dest==WIRE_DEST_LEFT) ? EARBUD_LEFT:EARBUD_RIGHT];
            CCP_TRANSACTION other_ct =
                ccp_transaction[(dest==WIRE_DEST_LEFT) ? EARBUD_RIGHT:EARBUD_LEFT];

            if (!ct->busy && !other_ct.busy && wire_tx(dest, buf, len+1))
            {
                if (need_answer)
                {
                    ct->busy = true;
                    ct->retries = 0;
                    ct->failed_retries = 0;
                    ct->poll_count = CCP_MAX_POLLS;
                    ct->poll_period_time = CCP_POLL_US;
                    ct->poll_timeout = global_time_us + CCP_POLL_US;
                }
                /* Store the channel information if wire_tx is successful */
                ct->channel = chan;
                ret = true;
            }
        }
    }

    return ret;
}

/**
 * \brief Retransmit the previous message to a particular earbud.
 * \param earbud The earbud ID to retransmit to (EARBUD_LEFT or EARBUD_RIGHT)
 * \return True if the retransmit was made, False otherwise.
 */
static bool ccp_retransmit(uint8_t earbud)
{
    bool ret = false;
    CCP_TRANSACTION *ct = &ccp_transaction[earbud];
    CCP_TRANSACTION other_ct = ccp_transaction[(earbud==EARBUD_LEFT) ? EARBUD_RIGHT:EARBUD_LEFT];

    if (!other_ct.busy && wire_tx(earbud == EARBUD_LEFT ? WIRE_DEST_LEFT : WIRE_DEST_RIGHT, ct->tx_buf, ct->tx_len))
    {
        ct->busy = true;
        ct->poll_count = CCP_MAX_POLLS;
        ct->poll_period_time = CCP_POLL_US;
        ct->poll_timeout = global_time_us + CCP_POLL_US;
        ret = true;
    }

    return ret;
}


/*
* Receive a message from an earbud.
*/
void ccp_rx(
    uint8_t earbud,
    uint8_t *data,
    uint16_t len,
    bool final_piece)
{
    cli_tx_hex(CLI_BROADCAST, "WIRE->CCP", data, len);

    /* Stip the case comms header from the payload */
    if (len < CCP_HDR_SIZE)
    {
        return;
    }
    len = len - CCP_HDR_SIZE;

    if (final_piece)
    {
        uint8_t msg_id = BITMAP_GET(CCP_HDR, MSG_ID, data[CCP_HDR_OFFSET]);
        CCP_CHANNEL received_channel = BITMAP_GET(CCP_HDR, CHAN_ID, data[CCP_HDR_OFFSET]);

        /* Update the busy status when the received channel id matches with
         * the channel id stored in ccp_transaction[earbud].channel
         */
        if(ccp_transaction[earbud].channel == received_channel)
        {
            ccp_transaction[earbud].busy = false;
        }

        switch (received_channel)
        {
            case CCP_CH_CASE_INFO:
                case_channel_rx(earbud, msg_id, &data[CCP_HDR_SIZE], len);
                break;

            case CCP_CH_DFU:
                /* Handle the DFU specific data in dfu_earbud_if module */
                ccp_user[CCP_CH_DFU]->rx(earbud,
                                         msg_id,
                                         &data[CASE_COMMS_HEADER_BYTE],
                                         len);
                break;

#ifdef USB_QCOM_DEBUG_DEVICE
            case CCP_CH_QCOM_DEBUG_CHANNEL:
                qcom_debug_channel_rx(earbud, msg_id, &data[CCP_HDR_SIZE], len);
                break;
#endif
            default:
                break;
        }

    }
}

/*
* Receive an acknowledgement from an earbud.
*/
static void ccp_ack(uint8_t earbud)
{
    if (ccp_transaction[earbud].busy)
    {
        /*
        * We want an actual response, not just an ack. Set the timeout for the
        * next poll.
        */
        ccp_transaction[earbud].poll_timeout = global_time_us + ccp_transaction[earbud].poll_period_time;
    }
    else
    {
        ccp_user[ccp_transaction[earbud].channel]->ack(earbud);
    }
}

/*
* Receive a NACK from an earbud.
*/
static void ccp_nack(uint8_t earbud)
{
    ccp_user[ccp_transaction[earbud].channel]->nack(earbud);
}

/*
* Notification that a message could not be sent.
*/
static void ccp_give_up(uint8_t earbud, bool retries_fail)
{
    ccp_user[ccp_transaction[earbud].channel]->give_up(earbud, retries_fail);
    ccp_transaction[earbud].busy = false;
}

/*
* Notification that there was no response.
*/
static void ccp_no_response(uint8_t earbud)
{
    ccp_user[ccp_transaction[earbud].channel]->no_response(earbud);
    ccp_transaction[earbud].busy = false;
}

/*
* Notification that a message to be sent was aborted.
*/
static void ccp_abort(uint8_t earbud)
{
    ccp_user[ccp_transaction[earbud].channel]->abort(earbud);
    ccp_transaction[earbud].busy = false;
}

/*
* Notification that broadcasting has finished.
*/
static void ccp_broadcast_finished(void)
{
    ccp_user[ccp_transaction[EARBUD_LEFT].channel]->broadcast_finished();
}

/*
* Send an AT command to an earbud. Note that the 'source' argument refers to
* the CLI.
*/
bool ccp_at_command(
    uint8_t cmd_source __attribute__((unused)),
    WIRE_DESTINATION dest __attribute__((unused)),
    char *at_cmd __attribute__((unused)))
{
    return false;
}

/*
* Charger Case Protocol initialisation for case info channel.
*/
void ccp_init(const CCP_USER_CB *user_cb)
{
    /* Initialise all channels with dummy callbacks. If a channel requires
     * additional behaviour they can register new callbacks using
     * \a ccp_register_channel
     */
    size_t i;
    for (i = 0; i < NUMBER_OF_CCP_CHANNELS; i++)
    {
        ccp_register_channel(&dummy_ccp_cb, (CCP_CHANNEL)i);
    }

    ccp_user[CCP_CH_CASE_INFO] = user_cb;
    wire_init(&ccp_wire_cb);
}

/*
* Case Comms Channel Registration for callback
*/
void ccp_register_channel(const CCP_USER_CB *cb, CCP_CHANNEL channel)
{
    ccp_user[channel] = cb;
}

bool ccp_poll(uint8_t earbud, uint16_t count, uint64_t period_us)
{
    CCP_TRANSACTION *ct = &ccp_transaction[earbud];

    if (ct->busy)
    {
        return false;
    }

    ct->busy = true;
    ct->retries = CCP_RETRY_COUNT;
    ct->poll_timeout = global_time_us;
    ct->poll_count = count;
    ct->poll_period_time = period_us;
    return true;
}

static void ccp_manage_transaction(uint8_t earbud)
{
    CCP_TRANSACTION *ct = &ccp_transaction[earbud];

    if ((ct->busy))
    {
        /*
        * Previously we received an empty ack, but we are waiting for an
        * actual response.
        */
        if (global_time_us >= ct->poll_timeout)
        {
            if (ct->poll_count != 0)
            {
                /*
                * Send a poll.
                */
                if (wire_tx(wire_dest[earbud], NULL, 0))
                {
                    ct->poll_timeout = global_time_us + ct->poll_period_time;
                    ct->poll_count--;
                }
                else
                {
                    /*
                    * Poll failed, this would be because wire or
                    * charger_comms are (temporarily) busy. Increment the
                    * timeout counter so that we will try again next time
                    * round. Don't count this as one of our retries.
                    */
                    ct->poll_timeout = global_time_us + ct->poll_period_time;
                }
            }
            else
            {
                if (ct->retries < CCP_RETRY_COUNT)
                {
                    if (ccp_retransmit(earbud))
                    {
                        ct->retries++;
                    }
                    else
                    {
                        ct->failed_retries++;
                        /* If we are blocked from retransmitting, eventually timeout. */
                        if (ct->failed_retries > CCP_ALLOWED_FAILED_RETIES)
                        {
                            ct->busy = false;
                            ccp_give_up(earbud, true);
                        }
                    }

                }
                else
                {
                    ct->busy = false;
                    ccp_give_up(earbud, true);
                }
            }
        }
    }
}

/*
* Charger Case Protocol periodic function.
*/
void ccp_periodic(void)
{
    ccp_manage_transaction(EARBUD_LEFT);
    ccp_manage_transaction(EARBUD_RIGHT);
}


