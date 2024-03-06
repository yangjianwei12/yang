/*!
\copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      DFU_EARBUD_IF
*/

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f0xx_crc.h"
#include "stm32f0xx_rcc.h"
#include "main.h"
#include "ccp.h"
#include "wire.h"
#include "dfu_earbud_if.h"
#include "timer.h"
#include "flash.h"
#include "power.h"
#include "stm32f0xx_hal_def.h"
#include "cli.h"
#include "case.h"
#include "config.h"


/*-----------------------------------------------------------------------------
------------------ PREPROCESSOR DEFINITIONS -----------------------------------
-----------------------------------------------------------------------------*/

#define DFU_EARBUD_IF_WAIT_TIMEOUT_SECONDS 20
#define VARIANT_SIZE 4

#define MSG_QUEUE_SIZE 3
#define MAX_DFU_DATA_SIZE 256
#define DFU_DATA_HEADER 3

#define DFU_EARBUD_IF_ACTIVITY_TIMEOUT_SECONDS 120
#define DFU_EARBUD_IF_ABORT_SCHEDULED_TIMEOUT 1

/* Msg lengths */
#define DFU_EARBUD_IF_CHECK_MSG_LEN 7
#define DFU_EARBUD_IF_BUSY_MSG_LEN 3
#define DFU_EARBUD_IF_READY_MSG_LEN 4
#define DFU_EARBUD_IF_START_MSG_LEN 7
#define DFU_EARBUD_IF_CHECKSUM_MSG_LEN 3
#define DFU_EARBUD_IF_VERIFY_MSG_LEN 3
#define DFU_EARBUD_IF_COMPLETE_MSG_LEN 3
#define DFU_EARBUD_IF_ACK_MSG_LEN 4
#define DFU_EARBUD_IF_NACK_MSG_LEN 3
#define DFU_EARBUD_IF_SYNC_MSG_LEN 3
#define DFU_EARBUD_IF_ERROR_MSG_LEN 4

/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/

typedef struct
{
    uint8_t primary_earbud;
    DFU_EARBUD_IF_MSG_ID msg_id;
    DFU_EARBUD_IF_ACTIVITY activity;
    DFU_EARBUD_IF_ACTIVITY prev_activity;
    DFU_EB_IF_EB_STATUS left_eb_status;
    DFU_EB_IF_EB_STATUS right_eb_status;
    uint8_t dfu_bit_set;
    bool commit_phase_of_dfu;
    bool abort_scheduled;

    /* Boolean Sequence number used by ACK messages. */
    uint8_t sn;
}
DFU_EARBUD_IF_STATUS;

typedef struct
{
    DFU_EARBUD_IF_MSG_ID msg_id;
    DFU_EARBUD_IF_ACTIVITY activity;
}
DFU_EB_IF_MSG_LIST;


/*-----------------------------------------------------------------------------
------------------ PROTOTYPES -------------------------------------------------
-----------------------------------------------------------------------------*/

static void Set1Byte(uint8_t *dst, uint16_t byteIndex, uint8_t val);
static void Set2Bytes(uint8_t *dst, uint16_t byteIndex, uint16_t val);

static void dfu_earbud_if_rx(uint8_t earbud, uint8_t msg_id, uint8_t *data, uint16_t len);
static void dfu_earbud_if_ack(uint8_t earbud);
static void dfu_earbud_if_nack(uint8_t earbud);
static void dfu_earbud_if_give_up(uint8_t earbud, bool retries_fail);
static void dfu_earbud_if_no_response(uint8_t earbud);
static void dfu_earbud_if_abort(uint8_t earbud);
static void dfu_earbud_if_broadcast_finished(void);
static void dfu_earbud_if_set_activity(DFU_EARBUD_IF_ACTIVITY new_activity);
static void dfu_earbud_if_send_dfu_ready_msg(char running_image);
static void dfu_earbud_if_send_ack(void);
static void dfu_earbud_if_send_nack(void);
static void dfu_earbud_if_send_dfu_start_msg(char *variant);
static void dfu_earbud_if_send_abort_request(void);
static void dfu_earbud_if_cleanup(void);
static void dfu_earbud_if_unified_cleanup(void);
static void dfu_earbud_if_set_prev_activity(DFU_EARBUD_IF_ACTIVITY prev_activity);
static void dfu_earbud_if_set_activity_with_msg(DFU_EARBUD_IF_MSG_ID msg_id,
                                         DFU_EARBUD_IF_ACTIVITY activity);
static void dfu_earbud_if_queue_event(DFU_EARBUD_IF_MSG_ID msg_id, DFU_EARBUD_IF_ACTIVITY activity);
static void dfu_earbud_if_handle_queue_event(void);
static void dfu_earbud_if_send_ack_with_request(void);
static void dfu_earbud_if_send_dfu_checksum_msg(void);
static void dfu_earbud_if_store_file_error_msg(DFU_FILE_ERROR_MSG err);
static void dfu_earbud_if_clean_up_on_scheduled_abort(void);


/*-----------------------------------------------------------------------------
------------------ VARIABLES --------------------------------------------------
-----------------------------------------------------------------------------*/

static DFU_EARBUD_IF_STATUS dfu_earbud_if_status = {0};
static uint32_t dfu_earbud_if_start_time;
static char dfu_earbud_if_running_image;
static char variant_name[VARIANT_SIZE];
static DFU_EARBUF_IF_ERROR_MSG error_msg;

static DFU_EB_IF_MSG_LIST *msg_queue[MSG_QUEUE_SIZE];
static uint8_t msg_queue_head = 0;
static uint8_t msg_queue_tail = 0;
static bool block_other_queue_event = false;
static bool ack_received_for_data_ack = false;
static uint8_t *dfu_data;
static uint32_t dfu_earbud_if_reset_time;
static uint32_t dfu_earbud_if_abort_scheduled_time;
static bool lid_open_event = false;

static const CCP_USER_CB dfu_earbud_if_ccp_cb =
{
    dfu_earbud_if_rx,
    dfu_earbud_if_ack,
    dfu_earbud_if_nack,
    dfu_earbud_if_give_up,
    dfu_earbud_if_no_response,
    dfu_earbud_if_abort,
    dfu_earbud_if_broadcast_finished
};

static const DFU_USER_CB dfu_earbud_if_dfu_cb =
{
    dfu_earbud_if_send_dfu_ready_msg,
    dfu_earbud_if_send_ack,
    dfu_earbud_if_send_nack,
    dfu_earbud_if_send_dfu_start_msg,
    dfu_earbud_if_send_abort_request,
    dfu_earbud_if_send_ack_with_request,
    dfu_earbud_if_send_dfu_checksum_msg,
    dfu_earbud_if_store_file_error_msg
};

/*-----------------------------------------------------------------------------
------------------ FUNCTIONS --------------------------------------------------
-----------------------------------------------------------------------------*/

static void Set1Byte(uint8_t *dst, uint16_t byteIndex, uint8_t val)
{
    dst[byteIndex] = val;
}

static void Set2Bytes(uint8_t *dst, uint16_t byteIndex, uint16_t val)
{
    dst[byteIndex] = val >> 8;
    dst[byteIndex+1] = val;
}

static void dfu_earbud_if_store_file_error_msg(DFU_FILE_ERROR_MSG err)
{
    error_msg = err;
}

void dfu_earbud_if_init(void)
{
    uint8_t dfu_reboot = config_get_dfu_reboot();

    ccp_register_channel(&dfu_earbud_if_ccp_cb, CCP_CH_DFU);

    PRINTF_B("current img count %lu", dfu_get_image_count());

    /* Handle the dfu reboot scenario */
    /* Reset the DFU Reboot bit */
    if(dfu_reboot == DFU_EB_IF_LEFT_EB_DFU ||
       dfu_reboot == DFU_EB_IF_RIGHT_EB_DFU)
    {
        PRINTF_B("reset dfu bit");
        /* Get the earbud information which was mediating the DFU process */
        dfu_earbud_if_status.primary_earbud = (dfu_reboot == DFU_EB_IF_LEFT_EB_DFU) ?
                                                             EARBUD_LEFT: EARBUD_RIGHT;
        config_set_dfu_reboot(DFU_EB_IF_NO_DFU);
    }

    /* If the DFU image count is not set, then its a DFU reboot. Start the commit
     * process.
     */
    if(dfu_get_image_count() == 0xFFFFFFFF)
    {
        /* DFU in progress, so don't allow chargercase to enter sleep mode */
        power_set_run_reason(POWER_RUN_DFU);

        /* Stop any case status specific operation until the dfu is completed */
        case_allow_dfu();

        dfu_set_state_to_busy();

        /* Set this variable, as this variable get used during lid open event.
         * So if the dfu is aborted during commit phase, the lid open event
         * should clear states of dfu variables.
         */
        dfu_set_dfu_through_eb(true);
        /* Variable to indicate we are in commit phase now */
        dfu_earbud_if_status.commit_phase_of_dfu = true;

        PRINTF_B("Queue dfu verify event");
       /* Start the DFU Verify activity */
        dfu_earbud_if_queue_event(DFU_EARBUD_IF_VERIFY, DFU_EB_IF_SEND_MESSAGE);
       /* Ticks setup to reset the case if commit is not completed within
        * stipulated time
        */
       dfu_earbud_if_reset_time = ticks;
    }
}

static void dfu_earbud_if_rx(uint8_t earbud, uint8_t msg_id, uint8_t *data, uint16_t len)
{
    /* Currently, msg_id is unused */
    UNUSED(msg_id);

    if(earbud == dfu_earbud_if_status.primary_earbud && len != 0 
       && dfu_is_dfu_through_eb() && !dfu_earbud_if_status.abort_scheduled)
    {
        switch(data[0])
        {
            case DFU_EARBUD_IF_INITIATE:
            {
                if(dfu_earbud_if_status.activity == DFU_EB_IF_WAIT_FOR_DS ||
                   dfu_earbud_if_status.prev_activity == DFU_EB_IF_WAIT_FOR_DS)
                {
                    PRINTF_B("Received DS Msg");
                    /* If dfu is not ongoing, then initiate flash erash request and start the
                     * dfu engine
                     */
                    if(dfu_is_state_idle())
                    {
                        dfu_earbud_if_queue_event(DFU_EARBUD_IF_INTERNAL, DFU_EB_IF_REQUEST_ERASE);
                    }
                    /* dfu is already in progress. Send "DFU BUSY" msg to earbud */
                    else
                    {
                        PRINTF_B("Busy");
                        dfu_earbud_if_queue_event(DFU_EARBUD_IF_BUSY, DFU_EB_IF_SEND_MESSAGE);
                    }
                    return;
                }
            }
            break;

            case DFU_EARBUD_IF_REBOOT:
            {
                if(dfu_earbud_if_status.activity == DFU_EB_IF_WAIT_FOR_DR ||
                   dfu_earbud_if_status.prev_activity == DFU_EB_IF_WAIT_FOR_DR)
                {
                    dfu_earbud_if_queue_event(DFU_EARBUD_IF_INTERNAL, DFU_EB_IF_RESET);
                    return;
                }
            }
            break;

            case DFU_EARBUD_IF_COMMIT:
            {
                if(dfu_earbud_if_status.activity == DFU_EB_IF_WAIT_FOR_DC ||
                   dfu_earbud_if_status.prev_activity == DFU_EB_IF_WAIT_FOR_DC)
                {
                    dfu_earbud_if_queue_event(DFU_EARBUD_IF_INTERNAL, DFU_EB_IF_HANDLE_COMMIT);
                    return;
                }
            }
            break;

            case DFU_EARBUD_IF_ABORT:
            {
                PRINTF_B("Received DA");
                /* If we are in commit state, then reset the device */
                if(dfu_earbud_if_status.activity == DFU_EB_IF_WAIT_FOR_DC)
                {
                    dfu_thru_eb_reset();
                }
                else
                {
                    /* Clean up dfu_earbud_if and dfu components */
                    dfu_earbud_if_unified_cleanup();
                }
                return;
            }

            case DFU_EARBUD_IF_DATA:
            {
                uint8_t *payload = &data[3];
                if(payload[1] == '0')
                {
                    PRINTF_B("S0 Record");
                }
                else if(payload[1] == '3')
                {
                    PRINTF_B("S3 Record");
                }
                else if(payload[1] == '7')
                {
                    PRINTF_B("S7 Record");
                }
                /* Handle the "S0,S3 and S7" msg */
                /* As the srec data comes from earbuds, there can be a bit delay
                 * in receiving them post sending ACK for previous data.
                 * Sometime, can lead to sync request being sent to earbuds, and
                 * before the sync is sent, the dfu_earbud_if_status.activity is
                 * set as SYNC first. After setting the activity, if in between
                 * the dfu data is received, then, this can caused the data to
                 * unhandle, as the sync request is yet to be send, post which
                 * the activity is set to WAIT_FOR_SREC. So, check for prev
                 * activity in this case.
                 */
                if(dfu_earbud_if_status.activity == DFU_EB_IF_WAIT_FOR_SREC ||
                   dfu_earbud_if_status.prev_activity == DFU_EB_IF_WAIT_FOR_SREC)
                {
                    memcpy(dfu_data, payload, len - DFU_DATA_HEADER);
                    dfu_earbud_if_queue_event(DFU_EARBUD_IF_INTERNAL, DFU_EB_IF_HANDLE_DFU_DATA);
                    return;
                }
            }
            break;
        }

        /* Print the msg id and activity details for out of order failure */
        PRINTF_B("dfu_error msg_id %d", dfu_earbud_if_status.msg_id);

        PRINTF_B("dfu_activity %d prev_activity %d", dfu_earbud_if_status.activity,
                                                    dfu_earbud_if_status.prev_activity);

        /* If the message doesn't fit any criteria in above switch then its out of order. */
        error_msg = dfu_error_out_of_order;
        dfu_earbud_if_queue_event(DFU_EARBUD_IF_ERROR, DFU_EB_IF_SEND_MESSAGE);
    }
}

static void dfu_earbud_if_ack(uint8_t earbud)
{
    PRINTF_B("dfu_earbud_if_ack");
    UNUSED(earbud);

    if(ack_received_for_data_ack)
    {
        /* Ack received already, do nothing */
        PRINTF_B("ack already received");
    }
    else
    {
        block_other_queue_event = false;
        /* If ack is received during an abort scheduled, that indicates, its 
         * time to cleanup everything as we don't expect any more dfu activities
         */
        if(dfu_earbud_if_status.abort_scheduled)
        {
            PRINTF_B("abort scheduled cleanup");
            dfu_earbud_if_clean_up_on_scheduled_abort();
        }
    }
}

static void dfu_earbud_if_nack(uint8_t earbud)
{
    UNUSED(earbud);
    PRINTF_B("dfu_earbud_if_nack");
}

static void dfu_earbud_if_give_up(uint8_t earbud, bool retries_fail)
{
    PRINTF_B("DFU EB Give up (%c)", earbud_letter[earbud]);
    if(retries_fail)
    {
        if(dfu_is_dfu_through_eb())
        {
            PRINTF_B("Msg Retries Fail");
            dfu_earbud_if_set_activity_with_msg(DFU_EARBUD_IF_SYNC, DFU_EB_IF_SEND_MESSAGE);
        }
        else
        {
            /* This scenario can arise when the lid open event took place during
             * dfu and dfu states are cleared, where as the last msg of give up
             * is received in dfu_earbud_if interface through ccp level. In that
             * case, its better to not honor the msg.
             */
        }
    }
    else
    {
        if(dfu_earbud_if_status.commit_phase_of_dfu)
        {
            /* Call dfu.c to reset the device now */
            dfu_thru_eb_reset();
        }
        else
        {
            PRINTF_B("Abort the DFU - Spurious Nack");
            /* Clean up dfu_earbud_if and dfu components */
            dfu_earbud_if_unified_cleanup();
        }
    }
}

static void dfu_earbud_if_no_response(uint8_t earbud)
{
    PRINTF_B("DFU EB No response (%c)", earbud_letter[earbud]);

    if(dfu_earbud_if_status.commit_phase_of_dfu)
    {
        /* Call dfu.c to reset the device now */
        dfu_thru_eb_reset();
    }
    else
    {
        /* Clean up dfu_earbud_if and dfu components */
        dfu_earbud_if_unified_cleanup();
    }
}

static void dfu_earbud_if_abort(uint8_t earbud)
{
    PRINTF_B("DFU EB Abort (%c)", earbud_letter[earbud]);
    /* Currently, abort is called only for Broacast msg handling. Since, during
     * DFU, no case status specific events, including Broadcasting, will occur,
     * it's safe to not do anything here now.
     */
}

static void dfu_earbud_if_broadcast_finished(void)
{

}

static void dfu_earbud_if_queue_event(DFU_EARBUD_IF_MSG_ID msg_id, DFU_EARBUD_IF_ACTIVITY activity)
{
    DFU_EB_IF_MSG_LIST *queue_msg = (DFU_EB_IF_MSG_LIST *)malloc(sizeof(DFU_EB_IF_MSG_LIST));

    queue_msg->msg_id = msg_id;
    queue_msg->activity = activity;

    msg_queue[msg_queue_tail] = queue_msg;
    msg_queue_tail++;
}

static void dfu_earbud_if_handle_queue_event(void)
{
    PRINTF_B("Handle queued event");
    dfu_earbud_if_set_activity_with_msg(msg_queue[msg_queue_head]->msg_id,
                                        msg_queue[msg_queue_head]->activity);

    /* These activities/events are internal and singleton basis, so no blocking
     * is required
     */
    if(!(msg_queue[msg_queue_head]->activity == DFU_EB_IF_REQUEST_ERASE || 
         msg_queue[msg_queue_head]->activity == DFU_EB_IF_HANDLE_DFU_DATA ||
         msg_queue[msg_queue_head]->activity == DFU_EB_IF_RESET ||
         msg_queue[msg_queue_head]->activity == DFU_EB_IF_HANDLE_COMMIT))
    {
        block_other_queue_event = true;
    }

    /* Free the queue data after it is utilized */
    free(msg_queue[msg_queue_head]);
    msg_queue_head ++;

    if(msg_queue_head == msg_queue_tail)
    {
        msg_queue_head = 0;
        msg_queue_tail = 0;
    }

}

static void dfu_earbud_if_send_dfu_ready_msg(char running_image)
{
    dfu_earbud_if_running_image = running_image;
    dfu_earbud_if_queue_event(DFU_EARBUD_IF_READY, DFU_EB_IF_SEND_MESSAGE);
}

static void dfu_earbud_if_send_ack(void)
{
    dfu_earbud_if_queue_event(DFU_EARBUD_IF_ACK, DFU_EB_IF_SEND_MESSAGE);
}

static void dfu_earbud_if_send_ack_with_request(void)
{
    dfu_earbud_if_queue_event(DFU_EARBUD_IF_ACK_WITH_REQUEST, DFU_EB_IF_SEND_MESSAGE);
}

static void dfu_earbud_if_send_nack(void)
{
    dfu_earbud_if_set_activity_with_msg(DFU_EARBUD_IF_NACK, DFU_EB_IF_SEND_MESSAGE);
}

static void dfu_earbud_if_send_dfu_start_msg(char *variant)
{
    /* Store the variant to be used while sending the DFU Start msg */
    memcpy(variant_name, variant, VARIANT_SIZE - 1);

    /* Store the null character at the end */
    variant_name[3] = '\0';

    dfu_earbud_if_queue_event(DFU_EARBUD_IF_START, DFU_EB_IF_SEND_MESSAGE);
}

static void dfu_earbud_if_send_dfu_checksum_msg(void)
{
    dfu_earbud_if_queue_event(DFU_EARBUD_IF_CHECKSUM, DFU_EB_IF_SEND_MESSAGE);
}

static void dfu_earbud_if_send_abort_request(void)
{
    dfu_earbud_if_queue_event(DFU_EARBUD_IF_ERROR, DFU_EB_IF_SEND_MESSAGE);
}

static void dfu_earbud_if_unified_cleanup(void)
{
    /* Clean up dfu_earbud_if specific component */
    dfu_earbud_if_cleanup();
    /* Clean up dfu specific component */
    dfu_cleanup();
}

static void dfu_earbud_if_cleanup(void)
{
    PRINTF_B("dfu_earbud_if_cleanup");
    /* Set dfu_earbud_if variables to its default values */
    dfu_earbud_if_status.activity = DFU_EB_IF_IDLE;
    dfu_earbud_if_status.msg_id = DFU_EARBUD_IF_CHECK;
    dfu_earbud_if_status.primary_earbud = false;
    dfu_set_dfu_through_eb(false);
    block_other_queue_event = false;
    dfu_earbud_if_status.dfu_bit_set = 0;
    dfu_earbud_if_status.commit_phase_of_dfu = false;
    dfu_earbud_if_status.abort_scheduled = false;
    dfu_earbud_if_status.sn = 0;
    /* Clear the message queues */
    msg_queue_head = 0;
    msg_queue_tail = 0;

    if(dfu_data)
    {
        free(dfu_data);
        dfu_data = NULL;
    }
}

void dfu_earbud_if_clear_eb_status(void)
{
    dfu_earbud_if_status.left_eb_status = DFU_EB_IF_NO_EB_STATUS;
    dfu_earbud_if_status.right_eb_status = DFU_EB_IF_NO_EB_STATUS;
}

bool dfu_earbud_if_tx(uint8_t earbud, DFU_EARBUD_IF_MSG_TYPE type, uint8_t *data, uint16_t len)
{
    bool output = false;

    if((type == DFU_EARBUD_IF_REQUEST) || (type == DFU_EARBUD_IF_RESPONSE_WITH_REQUEST))
    {
        return ccp_tx((uint8_t)type, CCP_CH_DFU, wire_dest[earbud], data, len, true);
    }
    else if(type == DFU_EARBUD_IF_RESPONSE)
    {
        return ccp_tx((uint8_t)type, CCP_CH_DFU, wire_dest[earbud], data, len, false);
    }
    else
    {
        return output;
    }
}

static void dfu_earbud_if_set_activity(DFU_EARBUD_IF_ACTIVITY new_activity)
{
    dfu_earbud_if_status.activity = new_activity;
}

static void dfu_earbud_if_set_prev_activity(DFU_EARBUD_IF_ACTIVITY prev_activity)
{
    dfu_earbud_if_status.prev_activity = prev_activity;
}

static void dfu_earbud_if_set_activity_with_msg(DFU_EARBUD_IF_MSG_ID msg_id,
                                         DFU_EARBUD_IF_ACTIVITY activity)
{
    dfu_earbud_if_status.msg_id = msg_id;
    dfu_earbud_if_status.activity = activity;
}

static void dfu_earbud_if_clean_up_on_scheduled_abort(void)
{
    /* Clean Up DFU process on lid open event */
    dfu_earbud_if_unified_cleanup();
    /* Case lid open status exchange event was on hold until the abort process
     * is completed. So, execute the case lid open status exchange event.
     */
    if(lid_open_event)
    {
        lid_open_event = false;
#ifdef HAVE_EARBUD_SWITCHES
        case_lid_intr_h();
#else
        case_event_occurred();
#endif
    }
}

void dfu_earbud_if_check_for_dfu_event(uint8_t earbud, uint8_t dfu_available)
{
    if(dfu_earbud_if_status.activity != DFU_EB_IF_IDLE)
    {
        PRINTF_B("Already DFU in progress, don't allow other activity");
        return;
    }

    if(dfu_available)
    {
        dfu_earbud_if_status.dfu_bit_set = dfu_available;
        /* Store the primary earbud information */
        dfu_earbud_if_status.primary_earbud = earbud;
    }

    if(earbud == EARBUD_LEFT)
    {
        PRINTF_B("Left EB Status");
        dfu_earbud_if_status.left_eb_status = DFU_EB_IF_RECEIVED_EB_STATUS;
    }
    else if(earbud == EARBUD_RIGHT)
    {
        PRINTF_B("Right EB Status");
        dfu_earbud_if_status.right_eb_status = DFU_EB_IF_RECEIVED_EB_STATUS;
    }

    /* Allow charger case DFU to start only if both the EB's status are received
     * and the DA bit is set
     */
    if(dfu_earbud_if_status.right_eb_status == DFU_EB_IF_RECEIVED_EB_STATUS &&
       dfu_earbud_if_status.left_eb_status == DFU_EB_IF_RECEIVED_EB_STATUS)
    {
        /* clear the eb status variables */
        dfu_earbud_if_clear_eb_status();

        if(dfu_earbud_if_status.dfu_bit_set)
        {
            /* DFU in progress, so don't allow chargercase to enter sleep mode */
            power_set_run_reason(POWER_RUN_DFU);
            /* Store the start time */
            dfu_earbud_if_start_time = ticks;
            /* Start the DFU check activity */
            dfu_earbud_if_queue_event(DFU_EARBUD_IF_CHECK, DFU_EB_IF_SEND_MESSAGE);

            dfu_set_dfu_through_eb(true);
        }
    }
}

void dfu_earbud_if_lid_open_event(void)
{
    /* If during commit phase, lid is open, then reboot */
    if(dfu_earbud_if_status.commit_phase_of_dfu)
    {
        dfu_thru_eb_reset();
    }
    else
    {
        /* During data transfer phase, schedule the abort of charger case on
         * cleanup, so as to provide time for flushing out any pending data
         * in comms level.
         */
        lid_open_event = true;
        dfu_earbud_if_status.abort_scheduled = true;
        dfu_earbud_if_abort_scheduled_time = ticks;
    }

}

void dfu_earbud_if_periodic(void)
{
    /* Start a timer tick of 120sec during DFU commit phase, so that if the
     * commit is not completed, then DFU will reboot
     */
    if(dfu_earbud_if_status.commit_phase_of_dfu)
    {
        if((ticks - dfu_earbud_if_reset_time) >
                  (DFU_EARBUD_IF_ACTIVITY_TIMEOUT_SECONDS * TIMER_FREQUENCY_HZ))
        {
            dfu_thru_eb_reset();
        }
    }

    /* If an abort is scheduled to take place after sometime, keep checking 
     * on the timer ticks, and it timeout occurs, abort the DFU
     */
    if(dfu_earbud_if_status.abort_scheduled)
    {
        if((ticks - dfu_earbud_if_abort_scheduled_time) >
                   (DFU_EARBUD_IF_ABORT_SCHEDULED_TIMEOUT * TIMER_FREQUENCY_HZ))
        {
            PRINTF_B("timeout cleanup");
            dfu_earbud_if_clean_up_on_scheduled_abort();
        }
    }

    /* Handle any queued event */
    if(msg_queue_head != msg_queue_tail && !block_other_queue_event)
    {
        dfu_earbud_if_handle_queue_event();
        return;
    }

    switch (dfu_earbud_if_status.activity)
    {
        case DFU_EB_IF_IDLE:
            break;

        case DFU_EB_IF_SEND_MESSAGE:
        {
            if(dfu_earbud_if_status.msg_id == DFU_EARBUD_IF_CHECK)
            {
                /* Check if any case operation is in progress or not before starting DFU*/
                if(case_allow_dfu())
                {
                    uint8_t buf[DFU_EARBUD_IF_CHECK_MSG_LEN];

                    /* Prepare the DFU CHECK msg and transmit it to primary earbud */
                    Set1Byte(buf, 0, DFU_EARBUD_IF_CHECK);
                    Set2Bytes(buf, 1, 4);
                    Set2Bytes(buf, 3, DFU_MAJOR_VERSION);
                    Set2Bytes(buf, 5, DFU_MINOR_VERSION);

                    PRINTF_B("Send DFU Check");
                    if(dfu_earbud_if_tx(dfu_earbud_if_status.primary_earbud,
                                        DFU_EARBUD_IF_REQUEST, buf,
                                        DFU_EARBUD_IF_CHECK_MSG_LEN))
                    {
                        PRINTF_B("Wait for DS");
                        dfu_earbud_if_set_activity(DFU_EB_IF_WAIT_FOR_DS);
                        /* Store the current activity information in prev_activity
                         * variable so that if in case, the earbud response
                         * to the current activity is delayed, and DFU sync
                         * activity kicks in to retries to connect with the
                         * earbuds, then the chargercase should be able to
                         * figure out for which previous activity DFU sync
                         * kicked in
                         */
                        dfu_earbud_if_set_prev_activity(DFU_EB_IF_WAIT_FOR_DS);
                    }

                 }
                 /* If the wait time is more than 200ms, abort the DFU process */
                 else if(((ticks - dfu_earbud_if_start_time) >
                (DFU_EARBUD_IF_WAIT_TIMEOUT_SECONDS * TIMER_FREQUENCY_HZ)))
                 {
                     error_msg = dfu_error_activity_timeout;
                     dfu_earbud_if_queue_event(DFU_EARBUD_IF_ERROR, DFU_EB_IF_SEND_MESSAGE);
                 }
                 else
                 {
                    /* Wait for the chargercase current work to get over*/
                 }
            }
            /* Sed DFU Busy message */
            if(dfu_earbud_if_status.msg_id == DFU_EARBUD_IF_BUSY)
            {
                uint8_t buf[DFU_EARBUD_IF_BUSY_MSG_LEN];

                /* Prepare the DFU BUSY msg and transmit it to primary earbud */
                Set1Byte(buf, 0, DFU_EARBUD_IF_BUSY);
                Set2Bytes(buf, 1, 0);

                if(dfu_earbud_if_tx(dfu_earbud_if_status.primary_earbud,
                                    DFU_EARBUD_IF_RESPONSE, buf,
                                    DFU_EARBUD_IF_BUSY_MSG_LEN))
                {
                    dfu_earbud_if_cleanup();
                }
            }
            /* Send DFU Ready message along with the running image information */
            if(dfu_earbud_if_status.msg_id == DFU_EARBUD_IF_READY)
            {
                PRINTF_B("Send DFU Ready");
                uint8_t buf[DFU_EARBUD_IF_READY_MSG_LEN];

                /* Prepare the DFU Ready msg and transmit it to primary earbud */
                Set1Byte(buf, 0, DFU_EARBUD_IF_READY);
                Set2Bytes(buf, 1, 1);
                Set1Byte(buf, 3, (uint8_t)dfu_earbud_if_running_image);

                if(dfu_earbud_if_tx(dfu_earbud_if_status.primary_earbud,
                                    DFU_EARBUD_IF_RESPONSE_WITH_REQUEST, buf,
                                    DFU_EARBUD_IF_READY_MSG_LEN))
                {
                    dfu_earbud_if_set_activity(DFU_EB_IF_WAIT_FOR_SREC);
                    /* Store the current activity information in prev_activity
                     * variable so that if in case, the earbud response
                     * to the current activity is delayed, and DFU sync
                     * activity kicks in to retries to connect with the
                     * earbuds, then the chargercase should be able to
                     * figure out for which previous activity DFU sync
                     * kicked in
                     */
                    dfu_earbud_if_set_prev_activity(DFU_EB_IF_WAIT_FOR_SREC);
                }
            }
            /* Send DFU Start message along with the variant information */
            if(dfu_earbud_if_status.msg_id == DFU_EARBUD_IF_START)
            {
                uint8_t buf[DFU_EARBUD_IF_START_MSG_LEN];
                PRINTF_B("Send DFU Start");

                /* Prepare the DFU Start msg and transmit it to primary earbud */
                Set1Byte(buf, 0, DFU_EARBUD_IF_START);
                Set2Bytes(buf, 1, 4);
                memcpy(&buf[3], variant_name, VARIANT_SIZE);

                if(dfu_earbud_if_tx(dfu_earbud_if_status.primary_earbud,
                                    DFU_EARBUD_IF_REQUEST, buf,
                                    DFU_EARBUD_IF_START_MSG_LEN))
                {
                    dfu_earbud_if_set_activity(DFU_EB_IF_WAIT_FOR_SREC);
                    /* Store the current activity information in prev_activity
                     * variable so that if in case, the earbud response
                     * to the current activity is delayed, and DFU sync
                     * activity kicks in to retries to connect with the
                     * earbuds, then the chargercase should be able to
                     * figure out for which previous activity DFU sync
                     * kicked in
                     */
                    dfu_earbud_if_set_prev_activity(DFU_EB_IF_WAIT_FOR_SREC);
                }
            }
            /* Send DFU Checksum message */
            if(dfu_earbud_if_status.msg_id == DFU_EARBUD_IF_CHECKSUM)
            {
                uint8_t buf[DFU_EARBUD_IF_CHECKSUM_MSG_LEN];
                PRINTF_B("Send DFU Checksum");

                /* Prepare the DFU Checksum msg and transmit it to primary earbud */
                Set1Byte(buf, 0, DFU_EARBUD_IF_CHECKSUM);
                Set2Bytes(buf, 1, 0);

                if(dfu_earbud_if_tx(dfu_earbud_if_status.primary_earbud,
                                    DFU_EARBUD_IF_REQUEST, buf,
                                    DFU_EARBUD_IF_CHECKSUM_MSG_LEN))
                {
                    dfu_earbud_if_set_activity(DFU_EB_IF_WAIT_FOR_DR);
                    /* Store the current activity information in prev_activity
                     * variable so that if in case, the earbud response
                     * to the current activity is delayed, and DFU sync
                     * activity kicks in to retries to connect with the
                     * earbuds, then the chargercase should be able to
                     * figure out for which previous activity DFU sync
                     * kicked in
                     */
                    dfu_earbud_if_set_prev_activity(DFU_EB_IF_WAIT_FOR_DR);
                }
            }

            /* Send DFU Verify message */
            if(dfu_earbud_if_status.msg_id == DFU_EARBUD_IF_VERIFY)
            {
                uint8_t buf[DFU_EARBUD_IF_VERIFY_MSG_LEN];

                /* Prepare the DFU Verify msg and transmit it to primary earbud */
                Set1Byte(buf, 0, DFU_EARBUD_IF_VERIFY);
                Set2Bytes(buf, 1, 0);

                if(dfu_earbud_if_tx(dfu_earbud_if_status.primary_earbud,
                                    DFU_EARBUD_IF_REQUEST, buf,
                                    DFU_EARBUD_IF_VERIFY_MSG_LEN))
                {
                    PRINTF_B("Send DFU Verify");
                    dfu_earbud_if_set_activity(DFU_EB_IF_WAIT_FOR_DC);
                    /* Store the current activity information in prev_activity
                     * variable so that if in case, the earbud response
                     * to the current activity is delayed, and DFU sync
                     * activity kicks in to retries to connect with the
                     * earbuds, then the chargercase should be able to
                     * figure out for which previous activity DFU sync
                     * kicked in
                     */
                    dfu_earbud_if_set_prev_activity(DFU_EB_IF_WAIT_FOR_DC);
                }
            }

            /* Send DFU Complete message */
            if(dfu_earbud_if_status.msg_id == DFU_EARBUD_IF_COMPLETE)
            {
                uint8_t buf[DFU_EARBUD_IF_COMPLETE_MSG_LEN];
                PRINTF_B("Send DFU Complete");

                /* Prepare the DFU Checksum msg and transmit it to primary earbud */
                Set1Byte(buf, 0, DFU_EARBUD_IF_COMPLETE);
                Set2Bytes(buf, 1, 0);

                if(dfu_earbud_if_tx(dfu_earbud_if_status.primary_earbud,
                                    DFU_EARBUD_IF_RESPONSE, buf,
                                    DFU_EARBUD_IF_COMPLETE_MSG_LEN))
                {
                    /* Clean up dfu_earbud_if and dfu components */
                    dfu_earbud_if_unified_cleanup();
                }
            }

            /* Send DFU ACK message */
            if(dfu_earbud_if_status.msg_id == DFU_EARBUD_IF_ACK ||
               dfu_earbud_if_status.msg_id == DFU_EARBUD_IF_ACK_WITH_REQUEST)
            {
                uint8_t buf[DFU_EARBUD_IF_ACK_MSG_LEN];
                uint8_t msg_type;
                PRINTF_B("Send DFU Ack");
                /* Prepare the DFU ACK msg and transmit it to primary earbud */
                Set1Byte(buf, 0, DFU_EARBUD_IF_ACK);
                Set2Bytes(buf, 1, 1);
                Set1Byte(buf, 3, dfu_earbud_if_status.sn);

                if(dfu_earbud_if_status.msg_id == DFU_EARBUD_IF_ACK)
                {
                    msg_type = DFU_EARBUD_IF_RESPONSE;
                }
                else
                {
                    msg_type = DFU_EARBUD_IF_RESPONSE_WITH_REQUEST;
                }

                if(dfu_earbud_if_tx(dfu_earbud_if_status.primary_earbud,
                                    msg_type, buf, DFU_EARBUD_IF_ACK_MSG_LEN))
                {
                    dfu_earbud_if_set_activity(DFU_EB_IF_WAIT_FOR_SREC);
                    /* new DFU ACK message, so clear the variable */
                    ack_received_for_data_ack = false;
                }
            }
            /* Send DFU NACK message */
            if(dfu_earbud_if_status.msg_id == DFU_EARBUD_IF_NACK)
            {
                uint8_t buf[DFU_EARBUD_IF_NACK_MSG_LEN];

                /* Prepare the DFU Nack msg and transmit it to primary earbud */
                Set1Byte(buf, 0, DFU_EARBUD_IF_NACK);
                Set2Bytes(buf, 1, 0);

                if(dfu_earbud_if_tx(dfu_earbud_if_status.primary_earbud,
                                    DFU_EARBUD_IF_RESPONSE, buf,
                                    DFU_EARBUD_IF_NACK_MSG_LEN))
                {
                    dfu_earbud_if_set_activity(DFU_EB_IF_WAIT_FOR_SREC);
                }
            }
            /* Send DFU SYNC message */
            if(dfu_earbud_if_status.msg_id == DFU_EARBUD_IF_SYNC)
            {
                uint8_t buf[DFU_EARBUD_IF_SYNC_MSG_LEN];

                /* Prepare the DFU Sync msg and transmit it to primary earbud */
                Set1Byte(buf, 0, DFU_EARBUD_IF_SYNC);
                Set2Bytes(buf, 1, 0);

                if(dfu_earbud_if_tx(dfu_earbud_if_status.primary_earbud,
                                    DFU_EARBUD_IF_REQUEST, buf,
                                    DFU_EARBUD_IF_SYNC_MSG_LEN))
                {
                    /* Store the previous activity information */
                    dfu_earbud_if_set_activity(dfu_earbud_if_status.prev_activity);
                }
            }

            /* Send DFU Error message */
            if(dfu_earbud_if_status.msg_id == DFU_EARBUD_IF_ERROR)
            {
                uint8_t buf[DFU_EARBUD_IF_ERROR_MSG_LEN];

                /* Prepare the DFU Error msg and transmit it to primary earbud */
                Set1Byte(buf, 0, DFU_EARBUD_IF_ERROR);
                Set2Bytes(buf, 1, 1);
                Set1Byte(buf, 3, (uint8_t)error_msg);

                if(dfu_earbud_if_tx(dfu_earbud_if_status.primary_earbud,
                                    DFU_EARBUD_IF_RESPONSE, buf,
                                    DFU_EARBUD_IF_ERROR_MSG_LEN))
                {
                    if(dfu_earbud_if_status.commit_phase_of_dfu)
                    {
                        /* Call dfu.c to reset the device now */
                        dfu_thru_eb_reset();
                    }
                    else
                    {
                        /* Clean up dfu_earbud_if and dfu components */
                        dfu_earbud_if_unified_cleanup();
                    }
                }
            }

        }
            break;

        case DFU_EB_IF_WAIT_FOR_DS:
            break;

        case DFU_EB_IF_REQUEST_ERASE:
            {
                /* Call dfu module to erase the flash and do some initial configurations */
                dfu_thru_eb_initial_config();
                /* Register for callback fn. */
                dfu_register_cb(&dfu_earbud_if_dfu_cb);
                /* Malloc a block of memory for storing dfu data and memset to zero.
                 */
                dfu_data = (uint8_t *)malloc((MAX_DFU_DATA_SIZE)*sizeof(uint8_t));
                memset(dfu_data, 0, (MAX_DFU_DATA_SIZE)*sizeof(uint8_t));
            }
            break;

        case DFU_EB_IF_DFU_WAITING:
            break;

        case DFU_EB_IF_WAIT_FOR_SREC:
            break;

        case DFU_EB_IF_HANDLE_DFU_DATA:
            {
                /* We have received a new S-record so, flip the sn for next ACK message. */
                dfu_earbud_if_status.sn = !dfu_earbud_if_status.sn;
                /* Already an ack is received, don't entertain another one */
                ack_received_for_data_ack = true;
                /* Send the dfu data to dfu engine */
                dfu_rx(CLI_SOURCE_NONE, (char *)dfu_data);
                dfu_earbud_if_set_activity(DFU_EB_IF_WAIT_FOR_SREC);
            }
            break;

        case DFU_EB_IF_WAIT_FOR_DR:
            break;

        case DFU_EB_IF_RESET:
            {
                /* Free the malloc memory */
                free(dfu_data);
                dfu_data = NULL;

                /* Set the DFU reboot bit */
                (dfu_earbud_if_status.primary_earbud == EARBUD_LEFT) ?
                                     config_set_dfu_reboot(DFU_EB_IF_LEFT_EB_DFU):
                                     config_set_dfu_reboot(DFU_EB_IF_RIGHT_EB_DFU);
                /* Call dfu.c to reset the device now */
                dfu_thru_eb_reset();
                dfu_earbud_if_set_activity(DFU_EB_IF_WAIT_FOR_RESET);
            }
            break;

        case DFU_EB_IF_WAIT_FOR_RESET:
        case DFU_EB_IF_WAIT_FOR_DC:
            break;

        case DFU_EB_IF_HANDLE_COMMIT:
            {
                if(dfu_update_image_count())
                {
                    PRINTF_B("DFU Image count updated");
                    dfu_earbud_if_queue_event(DFU_EARBUD_IF_COMPLETE, DFU_EB_IF_SEND_MESSAGE);
                }
                else
                {
                    PRINTF_B("write count failed");
                    error_msg = dfu_error_write_count_failed;
                    dfu_earbud_if_queue_event(DFU_EARBUD_IF_ERROR, DFU_EB_IF_SEND_MESSAGE);
                }
            }
            break;

        default:
            break;
    }

}

