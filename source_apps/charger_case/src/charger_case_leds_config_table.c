/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       charger_case_leds_config_table.c
\brief      ChargerCase led configuration table module
*/
#include "charger_case_leds_config_table.h"
#include "charger_case_led.h"
#include "charger_case_sm.h"

#include <audio_sources.h>
#include <domain_message.h>
#include <ui_indicator_leds.h>
#include <pairing.h>
#include <charger_monitor.h>
#include <power_manager.h>

#include "charger_case_buttons.h"

const ui_provider_context_consumer_indicator_table_t charger_case_ui_leds_context_indications_table[] =
{
    {.provider=ui_provider_app_sm,
     .context=context_app_sm_pairing,                   { .led.action = LED_START_PATTERN,
                                                          .led.data.pattern = app_led_pattern_pairing,
                                                          .led.priority = LED_PRI_LOW,
                                                          .led.local_only = TRUE }},
    {.provider=ui_provider_app_sm,
     .context=context_app_sm_streaming,                 { .led.action = LED_START_PATTERN,
                                                          .led.data.pattern = app_led_pattern_streaming,
                                                          .led.priority = LED_PRI_LOW,
                                                          .led.local_only = TRUE }},
    {.provider=ui_provider_app_sm,
     .context=context_app_sm_idle,                      { .led.action = LED_STOP_PATTERN,
                                                          .led.data.pattern = NULL,
                                                          .led.priority = LED_PRI_LOW,
                                                          .led.local_only = TRUE }},
    {.provider=ui_provider_app_sm,
     .context=context_app_sm_connecting,                { .led.action = LED_START_PATTERN,
                                                          .led.data.pattern = app_led_pattern_connecting,
                                                          .led.priority = LED_PRI_LOW,
                                                          .led.local_only = TRUE }},
    {.provider=ui_provider_app_sm,
     .context=context_app_sm_connected,                 { .led.action = LED_START_PATTERN,
                                                          .led.data.pattern = app_led_pattern_idle_connected,
                                                          .led.priority = LED_PRI_LOW,
                                                          .led.local_only = TRUE }},
};

const ui_event_indicator_table_t charger_case_ui_leds_table[] =
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

    {.sys_event=CHARGER_MESSAGE_DETACHED,               { .led.action = LED_CANCEL_FILTER,
                                                          .led.data.filter = NULL,
                                                          .led.priority = LED_PRI_MEDIUM,
                                                          .led.local_only = TRUE}},

    {.sys_event=CHARGER_MESSAGE_DISABLED,               { .led.action = LED_CANCEL_FILTER,
                                                          .led.data.filter = NULL,
                                                          .led.priority = LED_PRI_MEDIUM,
                                                          .led.local_only = TRUE}},
#if defined(HAVE_TRICOLOUR_LED)
    {.sys_event=CHARGER_MESSAGE_COMPLETED,              { .led.action = LED_SET_FILTER,
                                                          .led.data.filter = app_led_filter_charging_complete,
                                                          .led.priority = LED_PRI_MEDIUM,
                                                          .led.local_only = TRUE}},
#else
    {.sys_event=CHARGER_MESSAGE_COMPLETED,              { .led.action = LED_CANCEL_FILTER,
                                                          .led.data.filter = NULL,
                                                          .led.priority = LED_PRI_MEDIUM,
                                                          .led.local_only = TRUE}},
#endif /* HAVE_TRICOLOUR_LED */
    {.sys_event=CHARGER_MESSAGE_CHARGING_OK,            { .led.action = LED_SET_FILTER,
                                                          .led.data.filter = app_led_filter_charging_ok,
                                                          .led.priority = LED_PRI_MEDIUM,
                                                          .led.local_only = TRUE}},

    {.sys_event=CHARGER_MESSAGE_CHARGING_LOW,           { .led.action = LED_SET_FILTER,
                                                          .led.data.filter = app_led_filter_charging_low,
                                                          .led.priority = LED_PRI_MEDIUM,
                                                          .led.local_only = TRUE}},

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

uint8 ChargerCaseLedsConfigTable_GetSize(void)
{
    return ARRAY_DIM(charger_case_ui_leds_table);
}

uint8 ChargerCaseLedsConfigTable_ContextsTableGetSize(void)
{

    return ARRAY_DIM(charger_case_ui_leds_context_indications_table);
}
