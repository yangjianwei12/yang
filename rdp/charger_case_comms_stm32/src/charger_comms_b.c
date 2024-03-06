/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Charger Comms Scheme B
*/

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include <stdint.h>
#include "main.h"
#include "uart.h"
#include "wire.h"
#include "power.h"
#include "gpio.h"
#include "cli.h"
#include "charger_comms.h"
#include "vreg.h"
#include "timer.h"

#ifdef CHARGER_COMMS_FAKE
#include "fake_earbud.h"
#endif

/*-----------------------------------------------------------------------------
------------------ PREPROCESSOR DEFINITIONS -----------------------------------
-----------------------------------------------------------------------------*/

#define CHARGER_COMMS_WAKE_EARBUD_US            10000
#define CHARGER_COMMS_CASE_START_US             2000
#define CHARGER_COMMS_EARBUD_TURNAROUND_US     (1200 * 1000) 
#define CHARGER_COMMS_CASE_INACTIVE_TIMEOUT_US (500 * 1000)

/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/

typedef enum
{
    CHARGER_COMMS_IDLE,
    CHARGER_COMMS_START,
    CHARGER_COMMS_WAKE_EARBUD,
    CHARGER_COMMS_CASE_START,
    CHARGER_COMMS_COMMS_MODE,
} charger_comms_b_states;

/* Parameters for the AT+TXTEST command. */
typedef enum
{
    DOCK_TX_TEST_MODE_LOW = 0,
    DOCK_TX_TEST_MODE_HIGH = 1
} DOCK_TX_TEST_MODE;

/*-----------------------------------------------------------------------------
------------------ VARIABLES --------------------------------------------------
-----------------------------------------------------------------------------*/

static charger_comms_b_states cc_state = CHARGER_COMMS_IDLE;

static uint8_t cc_rx_buf[CHARGER_COMMS_MAX_MSG_LEN] = {0};
static uint16_t cc_rx_buf_ctr = 0;
static uint8_t *cc_tx_buf = NULL;
static uint16_t cc_tx_len;
static uint8_t cc_dest = WIRE_DEST_INVALID;
static uint32_t configured_timeout;
static uint64_t cc_timeout;
static uint64_t comms_time_us = 0; 

/*-----------------------------------------------------------------------------
------------------ FUNCTIONS --------------------------------------------------
-----------------------------------------------------------------------------*/

void charger_comms_device_init(void)
{
    cc_state = CHARGER_COMMS_IDLE;
    vreg_init();
}

bool charger_comms_is_active(void)
{
    return cc_state != CHARGER_COMMS_IDLE && cc_timeout != 0;
}

static void charger_comms_end(void)
{
    /* On occasion the UART may drop a single byte from a message. We should
     * pass this onto the wire layer so a NAK can be sent back to force the 
     * sender to retry */
    uint16_t current_length = cc_rx_buf_ctr;
    uint16_t expected_length = (wire_get_payload_length(cc_rx_buf) + WIRE_HEADER_BYTES);

    if (current_length != 0 &&
        current_length != expected_length)
    {
        uint8_t earbud = wire_get_packet_src(cc_rx_buf) == 1 ? EARBUD_RIGHT : EARBUD_LEFT;
        wire_rx(earbud, cc_rx_buf, cc_rx_buf_ctr);
    }

    cc_dest = WIRE_DEST_INVALID;
    cc_timeout = 0;
    cc_rx_buf_ctr = 0;
    comms_time_us = global_time_us + CHARGER_COMMS_CASE_INACTIVE_TIMEOUT_US; 
}

static void charger_comms_return_to_vchg(void)
{
    /* 10ms delay before debug over chg comms too long */
    // return;

    /* TODO: We should disable the pull-up and re-enable VBUS when we have
     * finished sending data.
     * Need to be very careful about timing this.*/
    vreg_off_clear_reason(VREG_REASON_OFF_COMMS);

    power_clear_run_reason(POWER_RUN_CHARGER_COMMS);
    cc_state = CHARGER_COMMS_IDLE;
}

static void charger_comms_raw_transmit(void)
{
    uart_tx(UART_DOCK, cc_tx_buf, cc_tx_len);

#ifdef CHARGER_COMMS_FAKE
#ifdef CHARGER_COMMS_FAKE_U
    earbud_rx_ready();
#else
    earbud_rx(cc_tx_buf, cc_tx_len);
#endif
#endif
}

void charger_comms_transmit(
    uint8_t dest,
    uint8_t *buf,
    uint16_t num_tx_octets,
    uint32_t timeout)
{
    cli_tx_hex(CLI_BROADCAST, "WIRE->COMMS", buf, num_tx_octets);

    cc_tx_buf = buf;
    cc_dest = dest;
    cc_tx_len = num_tx_octets;
    cc_timeout = 1;

    configured_timeout = timeout ? timeout : CHARGER_COMMS_EARBUD_TURNAROUND_US; 
    
    power_set_run_reason(POWER_RUN_CHARGER_COMMS);

    if (cc_state == CHARGER_COMMS_IDLE)
    {
        cc_state = CHARGER_COMMS_START;
    }
    else if (cc_state == CHARGER_COMMS_COMMS_MODE)
    {
        charger_comms_raw_transmit();
        cc_timeout = global_time_us + configured_timeout;
        comms_time_us = global_time_us + CHARGER_COMMS_CASE_INACTIVE_TIMEOUT_US; 
    }
}


void charger_comms_transmit_done(void)
{
    if (cc_dest==WIRE_DEST_BROADCAST)
    {
        charger_comms_end();
    }
}

void charger_comms_receive(uint8_t data)
{
    /* Only receive data from earbud while in communicaiton mode and have sent
     * a directed message to a specific earbud. */
    if (charger_comms_is_active() &&
       (cc_dest == WIRE_DEST_LEFT || cc_dest == WIRE_DEST_RIGHT))
    {
        if (cc_rx_buf_ctr < CHARGER_COMMS_MAX_MSG_LEN)
        {
            cc_rx_buf[cc_rx_buf_ctr++] = data;

            if (cc_rx_buf_ctr >= WIRE_NO_OF_BYTES)
            {
                if ((wire_get_payload_length(cc_rx_buf) + WIRE_HEADER_BYTES)==cc_rx_buf_ctr)
                {
                    uint8_t earbud = wire_get_packet_src(cc_rx_buf) == 1 ? EARBUD_RIGHT : EARBUD_LEFT;
                    wire_rx(
                            earbud,
                            cc_rx_buf,
                            cc_rx_buf_ctr);

                    charger_comms_end();
                }
            }
        }
    }
}

void charger_comms_periodic(void)
{
    switch(cc_state)
    {
        case CHARGER_COMMS_IDLE:
            break;

        case CHARGER_COMMS_START:
            vreg_off_set_reason(VREG_REASON_OFF_COMMS);

            /* Drive the UART TX pin high to drive the charger comms wire
             * low */
            gpio_enable(GPIO_DOCK_DATA_TX);
            gpio_output(GPIO_DOCK_DATA_TX);

            comms_time_us = global_time_us + CHARGER_COMMS_WAKE_EARBUD_US;
            cc_state = CHARGER_COMMS_WAKE_EARBUD;
            break;

        case CHARGER_COMMS_WAKE_EARBUD:
            if (global_time_us >= comms_time_us)
            {
                /* Set the TX pin back to UART TX which is required for
                 * CHARGER_COMMS_COMMS_MODE */
                gpio_af(GPIO_DOCK_DATA_TX, GPIO_AF_4);

                /* Now enable the pull-up for Tcase_start*/
                gpio_enable(GPIO_DOCK_PULL_EN);
                cc_state = CHARGER_COMMS_CASE_START;
                comms_time_us = global_time_us + CHARGER_COMMS_CASE_START_US; 
            }
            break;

        case CHARGER_COMMS_CASE_START:
            if (global_time_us >= comms_time_us)
            {
                charger_comms_raw_transmit();

                cc_timeout = global_time_us + configured_timeout;
                cc_state = CHARGER_COMMS_COMMS_MODE;
                comms_time_us = global_time_us + CHARGER_COMMS_CASE_INACTIVE_TIMEOUT_US; 
            }
            break;

        case CHARGER_COMMS_COMMS_MODE:
            if (global_time_us >= cc_timeout && cc_timeout != 0)
            {
                charger_comms_end();
            }
            if (global_time_us >= comms_time_us)
            {
                charger_comms_end();
                charger_comms_return_to_vchg();
            }
            break;

        default:
            break;
    }
}

CLI_RESULT ats_pull(uint8_t cmd_source __attribute__((unused)))
{
    bool ret = CLI_ERROR;
    long int en;

    if (cli_get_next_parameter(&en, 10))
    {
        vreg_off_set_reason(VREG_REASON_OFF_COMMAND);

        if (en)
        {
            gpio_enable(GPIO_DOCK_PULL_EN);
        }
        else
        {
            gpio_disable(GPIO_DOCK_PULL_EN);
        }

        ret = CLI_OK;
    }

    return ret;
}

CLI_RESULT ats_txtest(uint8_t cmd_source __attribute__((unused)))
{
    bool ret = CLI_ERROR;
    bool pass = false;
    long int mode;

    if (cli_get_next_parameter(&mode, 10))
    {
        /* Setup line for comms mode */
        vreg_off_set_reason(VREG_REASON_OFF_COMMAND);
        gpio_enable(GPIO_DOCK_PULL_EN);
        delay_ms(20);

        /* Force the Dock_TX and Dock_RX lines to standard PIO mode so
         * we can manually drive and read them */
        gpio_output(GPIO_DOCK_DATA_TX);
        gpio_input(GPIO_DOCK_DATA_RX);

        switch (mode)
        {
            case DOCK_TX_TEST_MODE_LOW:
                /* Test: Dock TX = 0V. Expect: Line = 1.8V, Dock RX = 0V */
                gpio_disable(GPIO_DOCK_DATA_TX);
                delay_ms(20);
                pass = !gpio_active(GPIO_DOCK_DATA_RX);
                break;
            case DOCK_TX_TEST_MODE_HIGH:
                /* Test: Dock TX = 3.3V. Expect: Line = 0V, Dock RX = 3.3V */
                gpio_enable(GPIO_DOCK_DATA_TX);
                delay_ms(20);
                pass = gpio_active(GPIO_DOCK_DATA_RX);
                break;
            default:
                break;
        }

        /* Return pins back to UART and allow VCHG to be switched back on */
        gpio_af(GPIO_DOCK_DATA_TX, GPIO_AF_4);
        gpio_af(GPIO_DOCK_DATA_RX, GPIO_AF_4);
        vreg_off_clear_reason(VREG_REASON_OFF_COMMAND);

        /* Print result - this will before OK/ERROR from the command */
        if (pass)
        {
            PRINT("PASS");
        }
        else
        {
            PRINT("FAIL");
        }


        ret = CLI_OK;
    }

    return ret;
}

#define MAX_UART_TEST_LEN (128)
static uint8_t uarttestbuf[MAX_UART_TEST_LEN];
CLI_RESULT ats_uarttest(uint8_t cmd_source __attribute__((unused)))
{
    bool ret = CLI_ERROR;

    long char_to_transmit;
    long length;

    if (cli_get_next_parameter(&char_to_transmit, 16) &&
        cli_get_next_parameter(&length, 10))
    {
        if (length <= MAX_UART_TEST_LEN)
        {
            memset(uarttestbuf, char_to_transmit, (length)*sizeof(uint8_t));
            charger_comms_transmit(WIRE_DEST_BROADCAST, uarttestbuf, (uint16_t)length, 0);
            ret = CLI_OK;
        }
    }

    return ret;
}
