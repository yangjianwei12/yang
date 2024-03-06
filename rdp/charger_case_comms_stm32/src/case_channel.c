/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Charger Case Protocol / Case Comms Protocol
*/

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include "case_channel.h"
#include "ccp.h"
#include "bitmap.h"

/*-----------------------------------------------------------------------------
------------------ PREPROCESSOR DEFINITIONS -----------------------------------
-----------------------------------------------------------------------------*/

/*
* First status byte field definitions.
*/
#define CCP_STATUS_1_MASK_CHG_RATE 0x04
#define CCP_STATUS_1_BIT_CHG_RATE  2
#define CCP_STATUS_1_MASK_CC       0x02
#define CCP_STATUS_1_BIT_CC        1
#define CCP_STATUS_1_MASK_L        0x01
#define CCP_STATUS_1_BIT_L         0

/*
* Battery status field definitions.
*/
#define CCP_BATTERY_MASK_C     0x80
#define CCP_BATTERY_BIT_C      7
#define CCP_BATTERY_MASK_LEVEL 0x7F
#define CCP_BATTERY_BIT_LEVEL  0

/*
* Reset message field definitions.
*/
#define CCP_RESET_MASK_R 0x01
#define CCP_RESET_BIT_R  0

/*
* Earbud status field definitions.
*/
#define CCP_EARBUD_STATUS_MASK_INFO     0x80
#define CCP_EARBUD_STATUS_BIT_INFO      1
#define CCP_EARBUD_STATUS_MASK_CHG_RATE 0x02
#define CCP_EARBUD_STATUS_BIT_CHG_RATE  1
#define CCP_EARBUD_STATUS_MASK_PP       0x01
#define CCP_EARBUD_STATUS_BIT_PP        0
#define CCP_EARBUD_STATUS_MASK_DFU_AVAILABLE 0x04
#define CCP_EARBUD_STATUS_BIT_DFU_AVAILABLE 2
#define CCP_EARBUD_STATUS_MASK_CMD_REQUEST 0x08
#define CCP_EARBUD_STATUS_BIT_CMD_REQUEST 3

/*
* Earbud extended status field definitions.
*/
#define CCP_EARBUD_STATUS_MASK_INFO_TYPE 0x7F
#define CCP_EARBUD_STATUS_BIT_INFO_TYPE  0

/*
* Shipping mode response field definitions.
*/
#define CCP_SHIP_RSP_MASK_SM 0x01
#define CCP_SHIP_RSP_BIT_SM  0

/*
* Handset pair response field definitions.
*/
#define CCP_HANDSET_PAIR_RSP_MASK_HP 0x01
#define CCP_HANDSET_PAIR_RSP_BIT_HP 0

/*
 * Common CCP_MSG_CASE_RSP field defintions
 */
#define CCP_CASE_RSP_LEN            1
#define CCP_CASE_RSP_CMD_OFFSET     0
#define CCP_CASE_RSP_PAYLOAD_OFFSET 1

/*
 * Case command: Recieve Serial Number field definitions
 */
#define CCP_CASE_RSP_RX_SERIAL_LEN         8
#define CCP_CASE_RSP_RX_SERIAL_TOTAL_LEN   (CCP_CASE_RSP_LEN + CCP_CASE_RSP_RX_SERIAL_LEN)

/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
------------------ PROTOTYPES -------------------------------------------------
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
------------------ VARIABLES --------------------------------------------------
-----------------------------------------------------------------------------*/

static const CASE_CHANNEL_USER_CB *case_channel_user;

/*-----------------------------------------------------------------------------
------------------ FUNCTIONS --------------------------------------------------
-----------------------------------------------------------------------------*/

void case_channel_init(const CASE_CHANNEL_USER_CB *user_cb)
{
    case_channel_user = user_cb;
}

/*
* Broadcast a short status message.
*/
bool ccp_tx_short_status(bool lid, bool charger, bool charge_rate)
{
    uint8_t buf;

    buf = BITMAP_SET(CCP_STATUS_1, CHG_RATE, charge_rate) |
          BITMAP_SET(CCP_STATUS_1, L, lid) |
          BITMAP_SET(CCP_STATUS_1, CC, charger);

    return ccp_tx(
        (uint8_t)CCP_MSG_STATUS, CCP_CH_CASE_INFO, WIRE_DEST_BROADCAST, &buf, 1, false);
}

/*
* Broadcast a complete status message.
*/
bool ccp_tx_status(
    bool lid,
    bool charger_connected,
    bool charging,
    bool charge_rate,
    uint8_t battery_case,
    uint8_t battery_left,
    uint8_t battery_right,
    uint8_t charging_left,
    uint8_t charging_right)
{
    uint8_t buf[4];

    buf[0] = BITMAP_SET(CCP_STATUS_1, CHG_RATE, charge_rate) |
             BITMAP_SET(CCP_STATUS_1, L, lid) |
             BITMAP_SET(CCP_STATUS_1, CC, charger_connected);

    buf[1] = BITMAP_SET(CCP_BATTERY, LEVEL, battery_case) |
             BITMAP_SET(CCP_BATTERY, C, charging);

    buf[2] = BITMAP_SET(CCP_BATTERY, LEVEL, battery_left) |
             BITMAP_SET(CCP_BATTERY, C, charging_left);
    buf[3] = BITMAP_SET(CCP_BATTERY, LEVEL, battery_right) |
             BITMAP_SET(CCP_BATTERY, C, charging_right);

    return ccp_tx(
        (uint8_t)CCP_MSG_STATUS, CCP_CH_CASE_INFO, WIRE_DEST_BROADCAST, buf, 4, false);
}

/*
* Send a handset pairing request message to the specified earbud.
*/
bool ccp_tx_handset_pairing(uint8_t earbud)
{
    uint8_t buf = CCP_EC_HANDSET_PAIRING;

    return ccp_tx(
        (uint8_t)CCP_MSG_EARBUD_CMD, CCP_CH_CASE_INFO,
        wire_dest[earbud], &buf, 1, true);
}

/*
* Send a shipping mode request message to the specified earbud.
*/
bool ccp_tx_shipping_mode(uint8_t earbud)
{
    uint8_t buf = CCP_EC_SHIPPING_MODE;

    return ccp_tx(
        (uint8_t)CCP_MSG_EARBUD_CMD, CCP_CH_CASE_INFO,
        wire_dest[earbud], &buf, 1, true);
}

/*
* Send a status request message to the specified earbud.
*/
bool ccp_tx_status_request(uint8_t earbud)
{
    return ccp_tx(
        (uint8_t)CCP_MSG_STATUS_REQ, CCP_CH_CASE_INFO,
        wire_dest[earbud], NULL, 0, true);
}

/*
* Send an extended status request message to the specified earbud.
*/
bool ccp_tx_xstatus_request(uint8_t earbud, uint8_t info_type)
{
    return ccp_tx(
        (uint8_t)CCP_MSG_STATUS_REQ, CCP_CH_CASE_INFO,
        wire_dest[earbud], &info_type, 1, true);
}

/*
* Send a loopback message to the specified earbud.
*/
bool ccp_tx_loopback(uint8_t earbud, uint8_t *data, uint16_t len)
{
    return ccp_tx(
        (uint8_t)CCP_MSG_LOOPBACK, CCP_CH_CASE_INFO,
        wire_dest[earbud], data, len, true);
}

/**
 * Send a message to iniate a case command
 */
bool ccp_tx_case_command_request(uint8_t earbud)
{
    uint8_t buf = CCP_EC_COMMAND_REQUEST;
    return ccp_tx(
        (uint8_t)CCP_MSG_EARBUD_CMD, CCP_CH_CASE_INFO,
        wire_dest[earbud], &buf, 1, true);
}

/**
 * Send a message to initiate a case command
 */
static bool ccp_tx_case_command_response(uint8_t earbud, CCP_CASE_CMD cmd, uint8_t *data, uint16_t len)
{
    return ccp_tx(
        (uint8_t)CCP_MSG_CASE_RSP, CCP_CH_CASE_INFO,
        wire_dest[earbud], data, len, false);
}

/**
 * Send a message in response to a "RX serial number" command from an earbud.
 */
bool ccp_tx_case_command_rx_serial_no(uint8_t earbud, uint64_t serial)
{
    uint8_t buf[CCP_CASE_RSP_RX_SERIAL_TOTAL_LEN];
    buf[CCP_CASE_RSP_CMD_OFFSET] = CCP_CASE_CMD_RX_SERIAL_NO;
    memcpy(&(buf[CCP_CASE_RSP_PAYLOAD_OFFSET]), &serial, CCP_CASE_RSP_RX_SERIAL_LEN);

    return ccp_tx_case_command_response(earbud,
                                        CCP_CASE_CMD_RX_SERIAL_NO,
                                        &buf,
                                        CCP_CASE_RSP_RX_SERIAL_TOTAL_LEN);
}

/*
* Send a reset message to the specified earbud.
*/
bool ccp_tx_reset(uint8_t earbud, bool factory)
{
    uint8_t buf;

    buf = BITMAP_SET(CCP_RESET, R, factory);

    return ccp_tx(
        (uint8_t)CCP_MSG_RESET, CCP_CH_CASE_INFO, wire_dest[earbud], &buf, 1, false);
}

void case_channel_rx(uint8_t earbud, uint8_t msg_id, uint8_t *data, uint16_t len)
{
    CCP_MESSAGE msg = (CCP_MESSAGE)msg_id;

    switch(msg)
    {
        case CCP_MSG_EARBUD_STATUS:
            if (BITMAP_GET(CCP_EARBUD_STATUS, INFO, data[0]))
            {
                switch (BITMAP_GET(CCP_EARBUD_STATUS, INFO_TYPE, data[0]))
                {
                    case CCP_IT_BT_ADDRESS:
                        if (len >= 7)
                        {
                            case_channel_user->rx_bt_address(
                                earbud,
                                data[5] + (data[6] << 8),
                                data[4],
                                data[1] + (data[2] << 8) + (data[3] << 16));
                        }
                        break;

                    default:
                        break;
                }
            }
            else
            {
                case_channel_user->rx_earbud_status(
                    earbud,
                    BITMAP_GET(CCP_EARBUD_STATUS, PP, data[0]),
                    BITMAP_GET(CCP_EARBUD_STATUS, CHG_RATE, data[0]),
                    BITMAP_GET(CCP_EARBUD_STATUS, DFU_AVAILABLE, data[0]),
                    BITMAP_GET(CCP_EARBUD_STATUS, CMD_REQUEST, data[0]),
                    BITMAP_GET(CCP_BATTERY, LEVEL, data[1]),
                    BITMAP_GET(CCP_BATTERY, C, data[1]));
            }
            break;

        case CCP_MSG_LOOPBACK:
            case_channel_user->loopback(earbud, &data[0], len);
            break;

        case CCP_MSG_EARBUD_RSP:
            switch (data[0])
            {
                case CCP_EC_SHIPPING_MODE:
                    case_channel_user->shipping(
                        earbud,
                        BITMAP_GET(CCP_SHIP_RSP, SM, data[1]));
                    break;

                case CCP_EC_HANDSET_PAIRING:
                    case_channel_user->handset_pairing(
                        earbud,
                        BITMAP_GET(CCP_HANDSET_PAIR_RSP, HP, data[1]));
                    break;

                case CCP_EC_COMMAND_REQUEST:
                    case_channel_user->case_cmd(earbud, &data[1], len);
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }
}
