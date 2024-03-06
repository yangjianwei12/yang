/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Case status
*/

#ifndef CASE_H_
#define CASE_H_

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include "cli.h"

/*-----------------------------------------------------------------------------
------------------ PREPROCESSOR DEFINITIONS -----------------------------------
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
------------------ VARIABLES --------------------------------------------------
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
------------------ PROTOTYPES -------------------------------------------------
-----------------------------------------------------------------------------*/

void case_init(void);
void case_periodic(void);
void case_tick(void);
void case_enable_debug(void);
void case_disable_debug(void);
void case_start_status_sequence(bool led);
#ifdef HAVE_EARBUD_SWITCHES
void case_switch_intr_h(uint8_t pin);
void case_lid_intr_h(void);
void case_chg_intr_h(void);
#else
void case_event_occurred(void);
#endif
bool case_allow_dfu(void);
void case_dfu_finished(void);
CLI_RESULT case_cmd(uint8_t cmd_source);
CLI_RESULT ats_loopback(uint8_t cmd_source);
CLI_RESULT atq_lid(uint8_t cmd_source);
CLI_RESULT ats_ebstatus(uint8_t cmd_source);
CLI_RESULT ats_ship(uint8_t cmd_source);
CLI_RESULT ats_handset_pair(uint8_t cmd_source);

#endif /* CASE_H_ */



