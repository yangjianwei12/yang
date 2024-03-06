/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      DFU_EARBUD_IF
*/

#ifndef DFU_EARBUD_IF_H_
#define DFU_EARBUD_IF_H_

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include "dfu.h"

/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/

#define DFU_MAJOR_VERSION 0x1

#define DFU_MINOR_VERSION 0x1

typedef enum
{
    DFU_EB_IF_IDLE,
    DFU_EB_IF_WAIT_FOR_DS,
    DFU_EB_IF_SEND_MESSAGE,
    DFU_EB_IF_REQUEST_ERASE,
    DFU_EB_IF_DFU_WAITING,
    DFU_EB_IF_WAIT_FOR_SREC,
    DFU_EB_IF_HANDLE_DFU_DATA,
    DFU_EB_IF_WAIT_FOR_DR,
    DFU_EB_IF_RESET,
    DFU_EB_IF_WAIT_FOR_RESET,
    DFU_EB_IF_WAIT_FOR_DC,
    DFU_EB_IF_HANDLE_COMMIT
}
DFU_EARBUD_IF_ACTIVITY;

typedef enum
{
    DFU_EARBUD_IF_CHECK,
    DFU_EARBUD_IF_BUSY,
    DFU_EARBUD_IF_READY,
    DFU_EARBUD_IF_START,
    DFU_EARBUD_IF_ACK,
    DFU_EARBUD_IF_NACK,
    DFU_EARBUD_IF_CHECKSUM,
    DFU_EARBUD_IF_VERIFY,
    DFU_EARBUD_IF_SYNC,
    DFU_EARBUD_IF_COMPLETE,
    DFU_EARBUD_IF_ERROR,
    DFU_EARBUD_IF_INITIATE,
    DFU_EARBUD_IF_REBOOT,
    DFU_EARBUD_IF_COMMIT,
    DFU_EARBUD_IF_ABORT,
    DFU_EARBUD_IF_DATA,

    DFU_EARBUD_IF_ACK_WITH_REQUEST,
    DFU_EARBUD_IF_INTERNAL
}
DFU_EARBUD_IF_MSG_ID;

typedef enum
{
    DFU_EARBUD_IF_REQUEST,
    DFU_EARBUD_IF_RESPONSE,
    DFU_EARBUD_IF_RESPONSE_WITH_REQUEST
}
DFU_EARBUD_IF_MSG_TYPE;

typedef enum
{
    /* 0-8 error numbers are define as part of DFU_FILE_ERROR_MSG */
    dfu_error_write_count_failed = 9,
    dfu_error_activity_timeout,
    dfu_error_out_of_order,
    dfu_error_lid_open_event
}
DFU_EARBUF_IF_ERROR_MSG;

typedef enum
{
    DFU_EB_IF_NO_DFU,
    DFU_EB_IF_LEFT_EB_DFU,
    DFU_EB_IF_RIGHT_EB_DFU
}
DFU_EB_IF_DFU_STATUS;

typedef enum
{
    DFU_EB_IF_NO_EB_STATUS,
    DFU_EB_IF_RECEIVED_EB_STATUS,
}
DFU_EB_IF_EB_STATUS;

/*-----------------------------------------------------------------------------
------------------ VARIABLES --------------------------------------------------
-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
------------------ PROTOTYPES -------------------------------------------------
-----------------------------------------------------------------------------*/

void dfu_earbud_if_init(void);
void dfu_earbud_if_periodic(void);
bool dfu_earbud_if_tx(uint8_t earbud, DFU_EARBUD_IF_MSG_TYPE type, uint8_t *data, uint16_t len);
void dfu_earbud_if_check_for_dfu_event(uint8_t earbud, uint8_t dfu_available);
void dfu_earbud_if_lid_open_event(void);
void dfu_earbud_if_clear_eb_status(void);

#endif /* DFU_EARBUD_IF_H_ */
