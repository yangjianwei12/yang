/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      DFU
*/

#ifndef DFU_H_
#define DFU_H_

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include "cli_parse.h"

/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/
typedef enum
{
    dfu_error_incompatible,
    dfu_error_spurious_nack,
    dfu_error_record_too_big,
    dfu_error_record_too_small,
    dfu_error_record_checksum,
    dfu_error_length_inconsistent,
    dfu_error_flash_failed,
    dfu_error_checksum,
    dfu_error_timeout
}
DFU_FILE_ERROR_MSG;

typedef struct
{
    void (*send_dfu_ready_msg)(char running_image);
    void (*send_ack)(void);
    void (*send_nack)(void);
    void (*send_dfu_start_msg)(char *variant);
    void (*send_abort_request)(void);
    void (*send_ack_with_request)(void);
    void (*send_dfu_checksum_msg)(void);
    void (*store_file_error_msg)(DFU_FILE_ERROR_MSG err);
}
DFU_USER_CB;

/*-----------------------------------------------------------------------------
------------------ VARIABLES --------------------------------------------------
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
------------------ PROTOTYPES -------------------------------------------------
-----------------------------------------------------------------------------*/

void dfu_init(void);
void dfu_periodic(void);
CLI_RESULT dfu_cmd(uint8_t cmd_source);
bool dfu_is_state_idle(void);
void dfu_rx(uint8_t cmd_source, char *str);
CLI_RESULT atq_running_image(uint8_t cmd_source);

#ifdef DFU_EARBUD_IF
void dfu_thru_eb_initial_config(void);
void dfu_thru_eb_reset(void);
void dfu_register_cb(const DFU_USER_CB *dfu_cb_user);
void dfu_cleanup(void);
void dfu_set_dfu_through_eb(bool is_dfu_through_eb);
bool dfu_update_image_count(void);
uint32_t dfu_get_image_count(void);
void dfu_set_state_to_busy(void);
bool dfu_is_dfu_through_eb(void);

#endif

#endif /* DFU_H_ */
