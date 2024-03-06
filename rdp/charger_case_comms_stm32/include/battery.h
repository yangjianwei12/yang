/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Battery
*/

#ifndef BATTERY_H_
#define BATTERY_H_

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include "cli_parse.h"

/*-----------------------------------------------------------------------------
------------------ DEFINES ----------------------------------------------------
-----------------------------------------------------------------------------*/

/**
 * A magic value to indicate that this battery voltage reading is invalid.
 * This is usually because it has not been taken
 */
#define BATTERY_READING_INVALID_MV 0xFFFF

/**
 * A magic value to indicate that this battery percentage is invalid.
 * This is usually because it has not been taken
 */
#define BATTERY_READING_INVALID_PERCENT 0xFF

/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/

typedef enum
{
    BATTERY_MONITOR_REASON_CHARGER_CONN,
    BATTERY_MONITOR_REASON_READING,
    BATTERY_MONITOR_REASON_COUNT
} BATTERY_MONITOR_REASON;

/*-----------------------------------------------------------------------------
------------------ PROTOTYPES -------------------------------------------------
-----------------------------------------------------------------------------*/

void battery_init(void);
void battery_periodic(void);
uint8_t battery_percentage_current(void);
void battery_read_request(bool led);
bool battery_read_done(void);
uint16_t battery_read_ntc(void);
CLI_RESULT atq_ntc(uint8_t cmd_source);
CLI_RESULT atq_battery(uint8_t cmd_source);
void battery_monitor_set_reason(BATTERY_MONITOR_REASON reason);
void battery_monitor_clear_reason(BATTERY_MONITOR_REASON reason);
void battery_overcurrent(void);

#endif /* BATTERY_H_ */
