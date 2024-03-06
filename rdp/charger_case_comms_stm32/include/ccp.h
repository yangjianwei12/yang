/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Charger Case Protocol
*/

#ifndef CCP_H_
#define CCP_H_

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include "wire.h"
#include "charger_comms.h"

/*-----------------------------------------------------------------------------
------------------ PREPROCESSOR DEFINITIONS -----------------------------------
-----------------------------------------------------------------------------*/

#define CCP_MAX_PAYLOAD_SIZE (CHARGER_COMMS_MAX_MSG_LEN - 3)

/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/

typedef enum
{
    CCP_CH_CASE_INFO                = 0,
    CCP_CH_DTS                      = 1,
    /* Channel Reserved for Earbud DFU purpose */
    CCP_CH_DFU        = 4,
    CCP_CH_QCOM_DEBUG_CHANNEL       = 8,
    NUMBER_OF_CCP_CHANNELS = 9
}
CCP_CHANNEL;

typedef struct
{
    void (*rx)(uint8_t earbud, uint8_t msg_id, uint8_t *data, uint16_t len);
    void (*ack)(uint8_t earbud);
    void (*nack)(uint8_t earbud);
    void (*give_up)(uint8_t earbud, bool retries_fail);
    void (*no_response)(uint8_t earbud);
    void (*abort)(uint8_t earbud);
    void (*broadcast_finished)(void);
}
CCP_USER_CB;

/*-----------------------------------------------------------------------------
------------------ FUNCTIONS --------------------------------------------------
-----------------------------------------------------------------------------*/

void ccp_init(const CCP_USER_CB *user_cb);
void ccp_register_channel(const CCP_USER_CB *cb, CCP_CHANNEL channel);
void ccp_periodic(void);
bool ccp_tx(
    uint8_t msg_id,
    CCP_CHANNEL chan,
    WIRE_DESTINATION dest,
    uint8_t *data,
    uint16_t len,
    bool need_answer);

bool ccp_at_command(uint8_t cli_source, WIRE_DESTINATION dest, char *at_cmd);

/**
 * \brief Request to send \a count number of charger comms level polls to an earbud.
 *
 * \param earbud The \a WIRE_DESTINATION of the earbud to poll
 * \param count Number of polls to sends
 * \param period_us The period in microseconds between each poll.
 *
 * \return True if poll request was accepted, false otherwise.
 */
bool ccp_poll(uint8_t earbud, uint16_t count, uint64_t period_us);

#endif /* CCP_H_ */
