/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Charger Case Protocol
*/

#ifndef CASE_CHANNEL_H_
#define CASE_CHANNEL_H_

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

/*-----------------------------------------------------------------------------
------------------ PREPROCESSOR DEFINITIONS -----------------------------------
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/

typedef enum
{
    CCP_MSG_STATUS        = 0,
    CCP_MSG_EARBUD_STATUS = 1,
    CCP_MSG_RESET         = 2,
    CCP_MSG_STATUS_REQ    = 3,
    CCP_MSG_LOOPBACK      = 4,
    CCP_MSG_EARBUD_CMD    = 5,
    CCP_MSG_EARBUD_RSP    = 6,
    CCP_MSG_CASE_RSP      = 7
}
CCP_MESSAGE;

typedef enum
{
    CCP_EC_PEER_PAIRING    = 0,
    CCP_EC_HANDSET_PAIRING = 1,
    CCP_EC_SHIPPING_MODE   = 2,
    CCP_EC_COMMAND_REQUEST = 3
}
CCP_EC;

typedef enum
{
    CCP_CASE_CMD_NONE =         0,
    CCP_CASE_CMD_RX_SERIAL_NO = 1,
} CCP_CASE_CMD;

typedef enum
{
    CCP_IT_BT_ADDRESS = 0
}
CCP_INFO_TYPE;

typedef struct
{
    void (*rx_earbud_status)(
        uint8_t earbud,
        uint8_t pp,
        uint8_t chg_rate,
        uint8_t dfu_available,
        uint8_t cmd_requested,
        uint8_t battery,
        uint8_t charging);

    void (*rx_bt_address)(
        uint8_t earbud, uint16_t nap, uint8_t uap, uint32_t lap);
    void (*loopback)(uint8_t earbud, uint8_t *data, uint16_t len);
    void (*shipping)(uint8_t earbud, uint8_t sm);
    void (*handset_pairing)(uint8_t earbud, uint8_t hp);
    void (*case_cmd)(uint8_t earbud, uint8_t *data, uint16_t len);
}
CASE_CHANNEL_USER_CB;

/*-----------------------------------------------------------------------------
------------------ FUNCTIONS --------------------------------------------------
-----------------------------------------------------------------------------*/

void case_channel_init(const CASE_CHANNEL_USER_CB *user_cb);
void case_channel_rx(uint8_t earbud, uint8_t msg_id, uint8_t *data, uint16_t len);
bool ccp_tx_short_status(bool lid, bool charger, bool charge_rate);
bool ccp_tx_status(
    bool lid,
    bool charger_connected,
    bool charging,
    bool charge_rate,
    uint8_t battery_case,
    uint8_t battery_left,
    uint8_t battery_right,
    uint8_t charging_left,
    uint8_t charging_right);
bool ccp_tx_shipping_mode(uint8_t earbud);
bool ccp_tx_loopback(uint8_t earbud, uint8_t *data, uint16_t len);
bool ccp_tx_status_request(uint8_t earbud);
bool ccp_tx_xstatus_request(uint8_t earbud, uint8_t info_type);
bool ccp_tx_reset(uint8_t earbud, bool factory);
bool ccp_tx_handset_pairing(uint8_t earbud);
bool ccp_tx_case_command_request(uint8_t earbud);
bool ccp_tx_case_command_rx_serial_no(uint8_t earbud, uint64_t serial);

#endif /* CASE_CHANNEL_H_ */
