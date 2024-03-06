/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Charger case application for 20-1114
*/

#ifndef MAIN_H_
#define MAIN_H_

/*-----------------------------------------------------------------------------
------------------ PREPROCESSOR DEFINITIONS -----------------------------------
-----------------------------------------------------------------------------*/

#ifdef VARIANT_CB
#define VARIANT_NAME "CB"
#define SCHEME_A
#define HAVE_CURRENT_SENSE
#define HAVE_BATTERY_VDL602045_545MA
#endif

#ifdef VARIANT_ST2
#define VARIANT_NAME "ST2"
#define FAST_TIMER_INTERRUPT
#define SCHEME_B
#define UART_CC_BIT_RATE 1500000
#define FORCE_48MHZ_CLOCK
#define HAVE_LOAD_SWITCH
#define HAVE_BATTERY_BPI_PL652043_590MA
#define DFU_EARBUD_IF
#endif

#ifdef VARIANT_MON
#define VARIANT_NAME "MON"
#define FAST_TIMER_INTERRUPT
#define SCHEME_B
#define UART_CC_BIT_RATE 1200000
#define FORCE_48MHZ_CLOCK
#define HAVE_LOAD_SWITCH
#define HAVE_BATTERY_BPI_PL652043_590MA
#define DFU_EARBUD_IF
#define HAVE_EARBUD_SWITCHES
#endif

#define CHARGER_BQ24230
#define USB_ENABLED

/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
------------------ PROTOTYPES -------------------------------------------------
-----------------------------------------------------------------------------*/

#endif /* MAIN_H_ */
