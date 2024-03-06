/*!
\copyright  Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       usb_dongle_led.c
\brief      Source file for the USB dongle application user interface LED indications.
*/
#include "usb_dongle_led.h"
#include "usb_dongle_ui.h"

const led_manager_hw_config_t usb_dongle_led_config =
#if defined(HAVE_1_LED)
{
    .number_of_leds = 1,
    .leds_use_pio = TRUE,
    .led0_pio = CHIP_LED_0_PIO,
    .led1_pio = 0,
    .led2_pio = 0,
};
#elif defined(HAVE_3_LEDS)
{
    .number_of_leds = 3,
    .leds_use_pio = TRUE,
    .led0_pio = CHIP_LED_0_PIO,
    .led1_pio = CHIP_LED_1_PIO,
    .led2_pio = CHIP_LED_2_PIO,
};
#else
#error LED config not correctly defined.
#endif
/*!@{ \name Definition of LEDs, and basic colour combinations

    The basic handling for LEDs is similar, whether there are
    3 separate LEDs, a tri-color LED, or just a single LED.
 */
#if defined(HAVE_3_LEDS)
#define LED_0_STATE  (1 << 0)
#define LED_1_STATE  (1 << 1)
#define LED_2_STATE  (1 << 2)
#else
/* We only have 1 LED so map all control to the same LED */
#define LED_0_STATE  (1 << 0)
#define LED_1_STATE  (1 << 0)
#define LED_2_STATE  (1 << 0)
#endif

#define LED_BLUE    (LED_0_STATE)
#define LED_GREEN   (LED_1_STATE)
#define LED_RED     (LED_2_STATE)
#define LED_WHITE   (LED_0_STATE | LED_1_STATE | LED_2_STATE)
#define LED_YELLOW  (LED_RED | LED_GREEN)
#define LED_CYAN    (LED_BLUE | LED_GREEN)
/*!@} */


#if defined(HAVE_TRICOLOUR_LED)
/*! \brief Swap the 'off' LED state and a specific colour

    This function causes a specific colour to become the default output, and is
    intended for use from a filter function.

    If the LEDs currently requested are a different colour, then the colour is
    not modified. However, if no colour is to be shown (off), then the new
    default colour is applied instead.

    To support flashing, if the LEDs currently requested match the selected
    default colour exactly, then change this to 'off' instead. The flash will be
    "inverted", but this is better than not showing it at all.

    \b Example: If RED is the selected fixed colour, and the LEDs would normally
    be flashing RED, then the LEDs will instead be RED and flash to the off state.

    All other (non-matching) flash colours will appear as normal over the top of
    the selected fixed colour, i.e. switching to the flash colour briefly before
    returning to the fixed colour.

    \param led_state    Requested state of LEDs prior to filtering
    \param colour       New default colour to swap around with the 'off' state

    \returns The new, filtered, state
*/
static uint16 swap_off_state_and_solid_colour(uint16 led_state, uint16 colour)
{
    uint16 new_state = led_state;

    /* Swap 'off' state for solid colour, and vice versa. */
    if ((new_state == colour) || (new_state == 0))
    {
        new_state ^= colour;
    }
    return new_state;
}
#endif /* HAVE_TRICOLOUR_LED */

/*! \brief An LED filter used for low charging level

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_charging_low(uint16 led_state)
{
#if defined(HAVE_TRICOLOUR_LED)
    return swap_off_state_and_solid_colour(led_state, LED_RED);
#else
    /* Turn on dedicated charge LED */
    return led_state ^ LED_RED;
#endif /* HAVE_TRICOLOUR_LED */
}

/*! \brief An LED filter used for charging level OK

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_charging_ok(uint16 led_state)
{
#if defined(HAVE_TRICOLOUR_LED)
    return swap_off_state_and_solid_colour(led_state, LED_YELLOW);
#else
    /* Turn on dedicated charge LED */
    return led_state ^ LED_RED;
#endif /* HAVE_TRICOLOUR_LED */
}

/*! \brief An LED filter used for charging complete

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_charging_complete(uint16 led_state)
{
#if defined(HAVE_TRICOLOUR_LED)
    return swap_off_state_and_solid_colour(led_state, LED_GREEN);
#else
    /* Turn off dedicated charge LED */
    return led_state;
#endif /* HAVE_TRICOLOUR_LED */
}

/*! \brief An LED filter used for gaming (aptX Adaptive LL) mode.

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_gaming_mode(uint16 led_state)
{
#if defined(HAVE_TRICOLOUR_LED)
    return swap_off_state_and_solid_colour(led_state, LED_CYAN);
#else
    /* Turn on dedicated gaming mode LED */
    return led_state ^ LED_RED;
#endif /* HAVE_TRICOLOUR_LED */
}


/*! \cond led_patterns_well_named
    No need to document these. The public interface is
    from public functions such as UsbDongleUi_Init()
 */

const led_pattern_t app_led_pattern_power_on[] =
{
    LED_LOCK,
    LED_ON(LED_RED),    LED_WAIT(100),
    LED_ON(LED_GREEN),  LED_WAIT(100),
    LED_ON(LED_BLUE),   LED_WAIT(100),
    LED_OFF(LED_RED),   LED_WAIT(100),
    LED_OFF(LED_GREEN), LED_WAIT(100),
    LED_OFF(LED_BLUE),  LED_WAIT(100),
    LED_UNLOCK,
    LED_END
};

const led_pattern_t app_led_pattern_power_off[] =
{
    LED_LOCK,
    LED_ON(LED_WHITE), LED_WAIT(100), LED_OFF(LED_WHITE), LED_WAIT(100),
    LED_REPEAT(1, 2),
    LED_UNLOCK,
    LED_END
};

const led_pattern_t app_led_pattern_error[] =
{
    LED_LOCK,
    LED_ON(LED_RED), LED_WAIT(100), LED_OFF(LED_RED), LED_WAIT(100),
    LED_REPEAT(1, 2),
    LED_UNLOCK,
    LED_END
};

const led_pattern_t app_led_pattern_button_press[] =
{
    LED_LOCK,
    LED_ON(LED_GREEN), LED_WAIT(100), LED_OFF(LED_GREEN), LED_WAIT(150),
    LED_UNLOCK,
    LED_END
};

const led_pattern_t app_led_pattern_button_held_3sec[] =
{
    LED_LOCK,
    LED_ON(LED_GREEN), LED_WAIT(100), LED_OFF(LED_GREEN), LED_WAIT(150),
    LED_REPEAT(1, 1),
    LED_UNLOCK,
    LED_END
};

const led_pattern_t app_led_pattern_button_held_6sec[] =
{
    LED_LOCK,
    LED_ON(LED_GREEN), LED_WAIT(100), LED_OFF(LED_GREEN), LED_WAIT(150),
    LED_REPEAT(1, 2),
    LED_UNLOCK,
    LED_END
};

const led_pattern_t app_led_pattern_button_held_8sec[] =
{
    LED_LOCK,
    LED_ON(LED_GREEN), LED_WAIT(100), LED_OFF(LED_GREEN), LED_WAIT(150),
    LED_REPEAT(1, 3),
    LED_UNLOCK,
    LED_END
};

const led_pattern_t app_led_pattern_idle_connectable[] =
{
    LED_SYNC(2000),
    LED_LOCK,
    LED_ON(LED_BLUE), LED_WAIT(100), LED_OFF(LED_BLUE),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const led_pattern_t app_led_pattern_connecting[] =
{
    LED_LOCK,
    LED_ON(LED_BLUE), LED_WAIT(50), LED_OFF(LED_BLUE), LED_WAIT(50),
    LED_UNLOCK,
    LED_REPEAT(0, 0)
};

const led_pattern_t app_led_pattern_idle_connected[] =
{
    LED_SYNC(2000),
    LED_LOCK,
    LED_ON(LED_BLUE), LED_WAIT(100), LED_OFF(LED_BLUE), LED_WAIT(100),
    LED_REPEAT(2, 1),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const led_pattern_t app_led_pattern_pairing[] =
{
    LED_LOCK,
    LED_ON(LED_BLUE), LED_WAIT(100), LED_OFF(LED_BLUE), LED_WAIT(100),
    LED_UNLOCK,
    LED_REPEAT(0, 0)
};

const led_pattern_t app_led_pattern_pairing_deleted[] =
{
    LED_LOCK,
    LED_ON(LED_BLUE), LED_WAIT(100), LED_OFF(LED_BLUE), LED_WAIT(100),
    LED_REPEAT(1, 2),
    LED_UNLOCK,
    LED_END
};

const led_pattern_t app_led_pattern_streaming_gaming_mode[] =
{
    LED_SYNC(2000),
    LED_LOCK,
    LED_ON(LED_BLUE), LED_WAIT(100), LED_OFF(LED_BLUE), LED_WAIT(100),
    LED_WAIT(500),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const led_pattern_t app_led_pattern_streaming_hq[] =
{
    LED_SYNC(2000),
    LED_LOCK,
    LED_ON(LED_BLUE), LED_WAIT(100), LED_OFF(LED_BLUE), LED_WAIT(100),
    LED_REPEAT(2, 1),
    LED_WAIT(500),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const led_pattern_t app_led_pattern_streaming_broadcast[] =
{
    LED_SYNC(2000),
    LED_LOCK,
    LED_ON(LED_BLUE), LED_WAIT(100), LED_OFF(LED_BLUE), LED_WAIT(100),
    LED_REPEAT(2, 2),
    LED_WAIT(500),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const led_pattern_t app_led_pattern_streaming_aptx[] =
{
    LED_SYNC(2000),
    LED_LOCK,
    LED_ON(LED_BLUE), LED_WAIT(50), LED_OFF(LED_BLUE), LED_WAIT(50),
    LED_REPEAT(2, 3),
    LED_WAIT(500),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const led_pattern_t app_led_pattern_streaming_aptx_adaptive[] =
{
    LED_SYNC(2000),
    LED_LOCK,
    LED_ON(LED_BLUE), LED_WAIT(50), LED_OFF(LED_BLUE), LED_WAIT(50),
    LED_REPEAT(2, 3),
    LED_WAIT(400),
    LED_ON(LED_BLUE), LED_WAIT(50), LED_OFF(LED_BLUE), LED_WAIT(50),
    LED_WAIT(500),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const led_pattern_t app_led_pattern_sco[] =
{
    LED_SYNC(1000),
    LED_LOCK,
    LED_ON(LED_BLUE), LED_WAIT(50), LED_OFF(LED_BLUE), LED_WAIT(50),
    LED_REPEAT(2, 1),
    LED_WAIT(500),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const led_pattern_t app_led_pattern_call_incoming[] =
{
    LED_LOCK,
    LED_SYNC(1000),
    LED_ON(LED_WHITE), LED_WAIT(50), LED_OFF(LED_WHITE), LED_WAIT(50),
    LED_REPEAT(2, 1),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};
