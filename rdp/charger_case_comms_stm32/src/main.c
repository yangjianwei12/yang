/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Charger case application
*/

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include "main.h"
#include "cli.h"
#include "adc.h"
#include "gpio.h"
#include "interrupt.h"
#include "uart.h"
#include "timer.h"
#include "led.h"
#include "pfn.h"
#include "wdog.h"
#include "power.h"
#include "case.h"
#include "memory.h"
#include "flash.h"
#include "usb.h"
#include "dfu.h"
#include "clock.h"
#include "rtc.h"
#include "charger.h"
#include "timestamp.h"
#include "config.h"
#include "battery.h"
#ifdef DFU_EARBUD_IF
#include "dfu_earbud_if.h"
#endif

/*-----------------------------------------------------------------------------
------------------ VARIABLES --------------------------------------------------
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
------------------ FUNCTIONS --------------------------------------------------
-----------------------------------------------------------------------------*/

int main(void)
{
    mem_init();
    interrupt_init();
    rtc_init();

    /*
     * Enter STANDBY if configured to do so.
     */
    if (mem_cfg_standby())
    {
        gpio_init();
        power_enter_standby();
    }

    /* Wait in STOP mode if configured to do so. */
    if (mem_cfg_stop())
    {
        gpio_init();
        power_enter_stop_after_reset();
    }

    /*
    * Initialisation.
    */
    flash_init();
    config_init();
    wdog_init();
    clock_init();
    gpio_init();
    uart_init();
    cli_init();
    timer_init();
    led_init();
    adc_init();
    case_init();
    dfu_init();
#ifdef DFU_EARBUD_IF
    dfu_earbud_if_init();
#endif
    charger_init();
    battery_init();

    rtc_set_alarm_every_second();

    PRINT_BU("\x1B[2J\x1B[H");
    PRINT_B("-------------------------------------------------------------------------------");
    PRINT_B("QUALCOMM " VARIANT_NAME);
    PRINTF_B("Build time %s %s", DATE_STAMP, TIME_STAMP);
    PRINT_B("-------------------------------------------------------------------------------");

    /*
    * Main loop.
    */
    while (1)
    {
        /*
        * Call all the periodic functions.
        */
        pfn_periodic();
    }
}
