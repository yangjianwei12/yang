/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       usb_dongle_leds_config_table.c
\brief      UsbDongle led configuration table module
*/
#include "usb_dongle_leds_config_table.h"
#include "usb_dongle_led.h"
#include "usb_dongle_sm.h"

#include <domain_message.h>
#include <ui_indicator_leds.h>
#include <pairing.h>
#include <power_manager.h>
#include <sink_service.h>

#include "usb_dongle_buttons.h"

/* The ordering of entries in this table is important. LED manager will stop at
   the first matching provider/context pair it finds when iterating through. */
const ui_provider_context_consumer_indicator_table_t usb_dongle_ui_leds_context_indications_table[] =
{
    {.provider=ui_provider_sink_service,
     .context=context_sink_pairing,                     { .led.action = LED_START_PATTERN,
                                                          .led.data.pattern = app_led_pattern_pairing,
                                                          .led.priority = LED_PRI_LOW,
                                                          .led.local_only = TRUE }},
    {.provider=ui_provider_app_sm,
     .context=context_app_sm_streaming,                 { .led.action = LED_START_PATTERN,
                                                          .led.data.pattern = app_led_pattern_streaming_gaming_mode,
                                                          .led.priority = LED_PRI_LOW,
                                                          .led.local_only = TRUE }},
    {.provider=ui_provider_sink_service,
     .context=context_sink_connecting,                  { .led.action = LED_START_PATTERN,
                                                          .led.data.pattern = app_led_pattern_connecting,
                                                          .led.priority = LED_PRI_LOW,
                                                          .led.local_only = TRUE }},
    {.provider=ui_provider_sink_service,
     .context=context_sink_connected,                   { .led.action = LED_START_PATTERN,
                                                          .led.data.pattern = app_led_pattern_idle_connected,
                                                          .led.priority = LED_PRI_LOW,
                                                          .led.local_only = TRUE }},
    {.provider=ui_provider_app_sm,
     .context=context_app_sm_idle,                      { .led.action = LED_STOP_PATTERN,
                                                          .led.data.pattern = NULL,
                                                          .led.priority = LED_PRI_LOW,
                                                          .led.local_only = TRUE }},
};

const ui_event_indicator_table_t usb_dongle_ui_leds_table[] =
{
    {.sys_event=POWER_ON,                               { .led.action = LED_START_PATTERN,
                                                          .led.data.pattern = app_led_pattern_power_on,
                                                          .led.priority = LED_PRI_EVENT,
                                                          .led.local_only = TRUE },
                                                          .await_indication_completion = TRUE },

    {.sys_event=POWER_OFF,                              { .led.action = LED_START_PATTERN,
                                                          .led.data.pattern = app_led_pattern_power_off,
                                                          .led.priority = LED_PRI_EVENT,
                                                          .led.local_only = TRUE },
                                                          .await_indication_completion = TRUE },

    {.sys_event=LI_MFB_BUTTON_SINGLE_PRESS,             { .led.action = LED_START_PATTERN,
                                                          .led.data.pattern = app_led_pattern_button_press,
                                                          .led.priority = LED_PRI_HIGH,
                                                          .led.local_only = TRUE}},

    {.sys_event=LI_MFB_BUTTON_HELD_1SEC,                { .led.action = LED_START_PATTERN,
                                                          .led.data.pattern = app_led_pattern_button_press,
                                                          .led.priority = LED_PRI_HIGH,
                                                          .led.local_only = TRUE}},

    {.sys_event=LI_MFB_BUTTON_HELD_3SEC,                { .led.action = LED_START_PATTERN,
                                                          .led.data.pattern = app_led_pattern_button_held_3sec,
                                                          .led.priority = LED_PRI_HIGH,
                                                          .led.local_only = TRUE}},

    {.sys_event=LI_MFB_BUTTON_HELD_6SEC,                { .led.action = LED_START_PATTERN,
                                                          .led.data.pattern = app_led_pattern_button_held_6sec,
                                                          .led.priority = LED_PRI_HIGH,
                                                          .led.local_only = TRUE}},

    {.sys_event=LI_MFB_BUTTON_HELD_8SEC,                { .led.action = LED_START_PATTERN,
                                                          .led.data.pattern = app_led_pattern_button_held_8sec,
                                                          .led.priority = LED_PRI_HIGH,
                                                          .led.local_only = TRUE}},
};

uint8 UsbDongleLedsConfigTable_GetSize(void)
{
    return ARRAY_DIM(usb_dongle_ui_leds_table);
}

uint8 UsbDongleLedsConfigTable_ContextsTableGetSize(void)
{

    return ARRAY_DIM(usb_dongle_ui_leds_context_indications_table);
}
