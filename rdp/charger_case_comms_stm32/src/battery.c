/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Battery
*/

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include <stdio.h>
#include "main.h"
#include "adc.h"
#include "gpio.h"
#include "led.h"
#include "timer.h"
#include "case_charger.h"
#include "config.h"
#include "power.h"
#include "current_senses.h"
#include "battery.h"
#include "battery_data.h"
#include "vreg.h"
#include "cmsis.h"

/*-----------------------------------------------------------------------------
------------------ PREPROCESSOR DEFINITIONS -----------------------------------
-----------------------------------------------------------------------------*/

/**
 * Time in microseconds that we must wait for the VBAT monitor to settle before taking
 * a measurement from it.
 */
#define BATTERY_READ_DELAY_TIME 20000

/**
 * Time in microseconds wait for the ADC reading to take place.
 */
#define BATTERY_ADC_DELAY_TIME 1000

/*
* BATTERY_NO_OF_CUTOFF_READS: Number of reads we will make if the voltage is
* below the cutoff voltage.
*/
#define BATTERY_NO_OF_CUTOFF_READS 3

/**
 * Maximum current that can be drawn in total from the VBUS pogo pins.
 * If this value is exceeded we must switch off VBUS to protect the battery.
 */
#define BATTERY_MAX_LOAD_MA 330

/**
 * Maximum current that can be drawn from a single earbud.
 * If this value is exceeded we must switch off VBUS to protect the battery.
 */
#define BATTERY_MAX_LOAD_PER_EARBUD_MA 200

/**
 * Time in microseconds that the VBUS output will be disabled if we detect it is overloaded
 * with current.
 */
#define BATTERY_OVERLOAD_US (3 * 1000 * 1000)

/**
 * The battery voltage at which this battery cannot support running the
 * voltage regualtor.
 * If too much current is drawn from the regualtor, it risks the battery
 * voltage to drop too far and cause a brownout.
 */
#define BATTERY_CUT_VREG_MV (3500)

/**
 * The battery voltage at which the voltage regulator should be cut off if there
 * is no charger attached
 */
#define BATTERY_CUT_VREG_NO_CHARGER_MV  (3700)

/**
 * The battery voltage at which it is deemed safe to re-enable the voltage
 * regulator if it had been disabled before.
 * It is recommended this value is greater than BATTERY_CUT_VREG_MV to provide
 * hysterisis for if the regulator can be enabled or not.
 */
#define BATTERY_REENABLE_VREG_MV (3900)

/**
 * The minimum voltage in millivolts that is considered a battery being present.
 *
 * This isn't 0 as there may some stray capacitance on the line which the ADC 
 * may read.
 */
#define BATTERY_PRESENT_MV 100

/**
 * The number of battery readings to keep track of which are used to calculate
 * a rolling average.
 */
#define BATTERY_HISTORY_COUNT 5

/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/

typedef enum
{
    BATTERY_IDLE,
    BATTERY_START_READING,
    BATTERY_READING,
    BATTERY_STOP_READING,
    BATTERY_DONE
}
BATTERY_STATE;

typedef enum
{
    BATTERY_LOAD_READING,
    BATTERY_LOAD_OVERLOADED
}
BATTERY_LOAD_STATE;

typedef struct
{
    bool first_reading_complete;
    size_t index;
    uint8_t percentages[BATTERY_HISTORY_COUNT];
    uint8_t last_percentage;
    uint16_t voltages[BATTERY_HISTORY_COUNT];
} BATTERY_LEVEL_HISTORY;

typedef struct
{
    BATTERY_STATE state;
    uint8_t current_battery_percent;
    uint16_t current_battery_mv;
    BATTERY_LEVEL_HISTORY history;
    uint64_t delay_time;
    bool led;
    uint8_t cmd_source;
    uint32_t cutoff_mv;
    uint8_t read_ctr;
    bool is_charging;
}
BATTERY_STATUS;

static BATTERY_STATUS battery_status =
{
    .state = BATTERY_IDLE,
    .cmd_source = CLI_SOURCE_NONE
};


static BATTERY_LOAD_STATE battery_load_state = BATTERY_LOAD_READING;
static uint64_t battery_load_timer = 0;
static bool overcurrent_event = false;

static uint16_t battery_monitor_reason = 0;

/*-----------------------------------------------------------------------------
------------------ FUNCTIONS --------------------------------------------------
-----------------------------------------------------------------------------*/

void battery_init(void)
{
    size_t i = 0;

    battery_status.history.first_reading_complete = false;
    battery_status.history.index = 0;
    battery_status.history.last_percentage = BATTERY_READING_INVALID_PERCENT;

    /* Populate the battery reading history with invalid readings */
    for (i = 0; i < BATTERY_HISTORY_COUNT; i++)
    {
        battery_status.history.voltages[i] = BATTERY_READING_INVALID_MV;
        battery_status.history.percentages[i] = BATTERY_READING_INVALID_PERCENT;
    }

    /* Disallow the charger until we've read the battery voltage */
    charger_set_reason(CHARGER_OFF_NO_BATTERY);
}

static void battery_monitor_enable_evaluate(void)
{
    if (battery_monitor_reason) 
    {
        gpio_enable(GPIO_VBAT_MONITOR_ON_OFF);
    }
    else
    {
        gpio_disable(GPIO_VBAT_MONITOR_ON_OFF);
    }
}

void battery_monitor_set_reason(BATTERY_MONITOR_REASON reason)
{
    battery_monitor_reason |= (1 << reason);
    battery_monitor_enable_evaluate();
}

void battery_monitor_clear_reason(BATTERY_MONITOR_REASON reason)
{
    battery_monitor_reason &= ~(1 << reason);
    battery_monitor_enable_evaluate();
}

/**
 * \brief Helper function to check if a delay has finished
 * \return True if the delay has elapsed, false if it is still ongoing
 */
static bool battery_delay_finished(void)
{
    return global_time_us >= battery_status.delay_time;
}

/**
 * \brief Enter the next state in battery_status
 * \param state The state to advance to.
 * \param us_to_wait Number of microseconds to wait till next state.
 * advancing to the next state.
 */
static void battery_next_state(BATTERY_STATE state,
                               uint64_t us_to_wait)
{
    battery_status.state = state;
    battery_status.delay_time = global_time_us + us_to_wait; 
}

/**
 * \brief Helper function to check if a delay has finished
 * \return True if the delay has elapsed, false if it is still ongoing
 */
static bool battery_load_delay_finished(void)
{
    return global_time_us >= battery_load_timer;
}

/**
 * \brief Enter the next battery load state
 * \param state The state to advance to.
 * \param us_to_wait Number of microseconds to wait till next state.
 * advancing to the next state.
 */
static void battery_load_next_state(BATTERY_LOAD_STATE state,
                               uint64_t us_to_wait)
{
    battery_load_state = state;
    battery_load_timer = global_time_us + us_to_wait; 
}

/**
 * \brief Add a battery reading to the system.
 * \param raw_mv The battery voltage to add in millivolts.
 * \param percentage The battery state of charge percentage to add
 */
static void battery_add_reading(uint16_t raw_mv, uint8_t percentage)
{
    size_t index = battery_status.history.index;

    /* Mark that we've added at least one valid reading. */
    battery_status.history.first_reading_complete = true;

    /* Record the voltage. */
    battery_status.current_battery_mv = raw_mv;
    battery_status.history.voltages[index] = raw_mv;

    /* Record the percentage */
    battery_status.current_battery_percent = percentage;
    battery_status.history.percentages[index] = percentage;

    /* If this is the first reading, we must initialise the previous reading
     * with a valid value */
    if (index == 0 &&
        battery_status.history.last_percentage == BATTERY_READING_INVALID_PERCENT)
    {
        battery_status.history.last_percentage = percentage;
    }

    /* Update the index */
    battery_status.history.index = ++index % BATTERY_HISTORY_COUNT;
}

static uint8_t battery_percentage(uint16_t mv)
{
    size_t i;
    uint8_t ret = 0;

    /* Any voltage less than the minimum level is considered 0% */
    if (mv > battery_levels[0].voltage_mv)
    {
        /* Any voltage larger than the maximum is considered 100% */
        ret = 100;

        /* Find the battery levels which the measured voltage sits between and
         * linearly interpolate the resulting percentage */
        for (i = 1; i < battery_levels_num; i++)
        {
            if (mv < battery_levels[i].voltage_mv)
            {
                /* Linearly interpolate a percentage based on this level and the previous */
                uint16_t range_mv = battery_levels[i].voltage_mv - battery_levels[i - 1].voltage_mv;
                uint8_t  range_pc = battery_levels[i].percentage - battery_levels[i - 1].percentage;
                uint8_t  d_mv = mv - battery_levels[i - 1].voltage_mv;

                ret = battery_levels[i - 1].percentage +
                    ((10 * range_pc * d_mv) / range_mv + 5) / 10;
                break;
            }
        }
    }

    return ret;
}

/**
 * Return the battery state of charge percentage based on a rolling average
 * from previous battery readings.
 */
static inline uint8_t battery_percentage_rolling_average(void)
{
    uint32_t accum = 0;
    size_t valid_readings = 0;
    size_t i;

#ifdef HAVE_CURRENT_SENSE
    /* Accumulate all valid stored percentages to use for a mean. */
    for (i = 0; i < BATTERY_HISTORY_COUNT; i++)
    {
        uint16_t percentage = battery_status.history.percentages[i];
        if (percentage != BATTERY_READING_INVALID_PERCENT)
        {
            accum += percentage;
            valid_readings++;
        }
    }

    /* Calculate percentage based on a rolling mean of stored percentages. */
    return accum/valid_readings;

#else
    /* Accumulate all valid voltage readings to use for a mean. */
    for (i = 0; i < BATTERY_HISTORY_COUNT; i++)
    {
        uint16_t voltage = battery_status.history.voltages[i];
        if (voltage != BATTERY_READING_INVALID_MV)
        {
            accum += voltage;
            valid_readings++;
        }
    }

    /* Calculate percentage based on a rolling mean of battery readings. */
    return battery_percentage(accum/valid_readings);
#endif
}

uint8_t battery_percentage_current(void)
{
    uint8_t result;
    uint8_t previous = battery_status.history.last_percentage;

    bool is_charging = battery_status.is_charging;
    bool charger_attached = charger_connected();

    /* If we've not taken a reading we should just return an invalid 
     * value now. */
    if (!battery_status.history.first_reading_complete)
    {
        return BATTERY_READING_INVALID_PERCENT;
    }

    result = battery_percentage_rolling_average();

    /* If a charger is attached, don't report a lower battery percentage.
     * If no charger is attached, don't report a higher battery percentage. */
    if ((result >= previous && !charger_attached) ||
         (result < previous && charger_attached))
    {
        result = previous;
    }

    if (charger_attached)
    {
        /* 100% state of charge should only be reported after the charger has terminated. */
        if (previous != 100 && result == 100 && is_charging)
        {
            result = 99;
        }

        /* Return 100% if charging has terminated and battery level is still above 95% */
        if (!is_charging && result >= 95)
        {
            result = 100;
        }
    }


    battery_status.history.last_percentage = result;
    return result;
}

#ifdef HAVE_CURRENT_SENSE
static uint32_t battery_calculate_drop(uint16_t total_ma, BATTERY_DROP *drops, size_t drops_num)
{
    size_t i;
    uint32_t drop_mv = 0;

    /* Only compensate if there is a load. */
    if (total_ma > drops[0].load_mA)
    {
        /* Any load larger than the maximum is considered to impose a 200mV drop. */
        drop_mv = 200;

        /* Find the battery load which the measured load sits between and
         * linearly interpolate the resulting battery drop*/
        for (i = 1; i < drops_num; i++)
        {
            if (total_ma < drops[i].load_mA)
            {
                /* Linearly interpolate a battery_drop_mv based on this level and the previous */
                uint16_t range_ma = drops[i].load_mA - drops[i - 1].load_mA;
                uint8_t  range_mv = drops[i].battery_drop_mv - drops[i - 1].battery_drop_mv;
                uint8_t  d_ma = total_ma - drops[i - 1].load_mA;

                drop_mv = drops[i - 1].battery_drop_mv +
                    ((10 * range_mv * d_ma) / range_ma + 5) / 10;
                break;
            }
        }
    }

    return drop_mv; 
}

static uint32_t battery_compensated_voltage(uint16_t raw_mv, uint32_t total_ma)
{
    size_t i;
    uint32_t drop_mv = 0;

    /* Any load larger than the maximum is considered to impose a 200mV drop. */
    drop_mv = 200;

    /* Find the battery load which the measured load sits between and
     * linearly interpolate the resulting battery drop*/
    for (i = 1; i < battery_drops_num; i++)
    {
        if (raw_mv < battery_drops[i].battery_voltage_mv)
        {
            /* Linearly interpolate a battery_drop_mv based on this level and the previous */
            uint16_t range_ma = battery_drops[i].battery_voltage_mv - battery_drops[i - 1].battery_voltage_mv;
            uint32_t drop_a = battery_calculate_drop(total_ma, battery_drops[i - 1].drops, battery_drops[i - 1].num_drops);
            uint32_t drop_b = battery_calculate_drop(total_ma, battery_drops[i].drops, battery_drops[i].num_drops);
            uint8_t  range_mv = drop_a > drop_b ? drop_a - drop_b : drop_b - drop_a;
            uint8_t  d_ma = raw_mv - battery_drops[i - 1].battery_voltage_mv;

            drop_mv = (drop_a > drop_b ? drop_a : drop_b) -
                ((10 * range_mv * d_ma) / range_ma + 5) / 10;
            break;
        }
    }

    return raw_mv + drop_mv; 
}
#endif

static uint16_t battery_mv(void)
{
    return adc_read_mv(ADC_VBAT, 6600);
}

void battery_overcurrent(void)
{
    overcurrent_event = true;
}

static void battery_overcurrent_event(void)
{
#ifdef SCHEME_A
    charger_comms_vreg_reset();
#endif
    vreg_off_set_reason(VREG_REASON_OFF_OVERCURRENT);
    battery_load_next_state(BATTERY_LOAD_OVERLOADED, BATTERY_OVERLOAD_US);
    power_set_run_reason(POWER_RUN_CURRENT_MON);
}

#ifdef HAVE_CURRENT_SENSE
/**
 * Monitor the approximate load on the battery by measuring the load
 * on VBUS.
 * If the total load exceeds the maximum then temporarily disable VBUS.
 * Once re-enabled continue to monitor and disable VBUS again if necessary.
 */
static void battery_current_monitoring(void)
{
    switch(battery_load_state)
    {
        case BATTERY_LOAD_READING:
        {
            uint32_t left_ma;
            uint32_t right_ma;
            battery_fetch_load_ma(&left_ma, &right_ma);
            uint32_t total_load_ma = left_ma + right_ma;

            /* Too much current from one earbud or the total load. */
            if (!charger_comms_is_active() &&
                (left_ma > BATTERY_MAX_LOAD_PER_EARBUD_MA ||
                 right_ma > BATTERY_MAX_LOAD_PER_EARBUD_MA ||
                 total_load_ma > BATTERY_MAX_LOAD_MA))
            {
                PRINTF_B("VBUS load l=%umA r=%umA exceeds max, switch off VBUS", left_ma, right_ma);
                battery_load_next_state(BATTERY_LOAD_OVERLOADED, BATTERY_OVERLOAD_US);
                charger_comms_vreg_reset();
                vreg_off_set_reason(VREG_REASON_OFF_OVERCURRENT);
            }
            else
            {
                power_clear_run_reason(POWER_RUN_CURRENT_MON);
            }

            /* TODO: This is necessary to keep the battery voltage monitoring happy.
             * This could be improved. */
            if (battery_status.state != BATTERY_READING)
            {
                adc_start_measuring();
            }
            break;
        }
        case BATTERY_LOAD_OVERLOADED:
            if (battery_load_delay_finished())
            {
                /* Once the timer expires, we re-enable VBUS and start
                 * monitoring the current again. If it still exceeds
                 * the permitted maximum, we will switch VBUS off again.
                 */
                charger_comms_vreg_high();
                vreg_off_clear_reason(VREG_REASON_OFF_OVERCURRENT);
                battery_load_next_state(BATTERY_LOAD_READING, 0);
            }
            break;

        default:
            break;
    }
}
#endif


#ifdef HAVE_LOAD_SWITCH
static void battery_load_switch_monitor(void)
{
    switch(battery_load_state)
    {
    case BATTERY_LOAD_OVERLOADED:
        if (battery_load_delay_finished())
        {
            /* Once the timer expires, we re-enable VBUS and start
             * monitoring the current again. If it still exceeds
             * the permitted maximum, we will switch VBUS off again.
             */
            vreg_off_clear_reason(VREG_REASON_OFF_OVERCURRENT);
            battery_load_next_state(BATTERY_LOAD_READING, 0);
            power_clear_run_reason(POWER_RUN_CURRENT_MON);
        }
        break;

    default:
        break;
    }
}
#endif

/**
 * \brief Handle events based on battery voltage
 * \param battery_mv The most recent battery voltage in millivolts.
 */
static void battery_handle_voltage_events(uint16_t battery_mv)
{
    if (case_charger_is_resolved() && charger_connected() && charger_is_charging() && battery_mv < BATTERY_CUT_VREG_MV)
    {
        vreg_off_set_reason(VREG_REASON_OFF_LOW_BATTERY);
    }
    else 
    {
        if ((!charger_connected() && battery_mv > BATTERY_CUT_VREG_NO_CHARGER_MV) || battery_mv >= BATTERY_REENABLE_VREG_MV)
        {
            vreg_off_clear_reason(VREG_REASON_OFF_LOW_BATTERY);
        }
    }

    /* Force the charger off if there is no battery present.
     * Leaving it on will lead to inaccurate battery measurements.
     */
    if (charger_connected())
    {
        if (battery_mv < BATTERY_PRESENT_MV)
        {
            charger_set_reason(CHARGER_OFF_NO_BATTERY);
        }
        else
        {
            if (case_charger_can_trust())
            {
                charger_clear_reason(CHARGER_OFF_NO_BATTERY);
            }
        }
    }
}

/**
 * \brief Perform monitoring of the battery voltage with the most recent battery reading.
 */
static void battery_voltage_monitoring(void)
{
    battery_handle_voltage_events(charger_connected() ? battery_mv() : battery_status.current_battery_mv);
}

/**
 * \brief Perform monitoring of the battery voltage with the most recent battery reading.
 */
static void battery_temperature_monitoring(void)
{
    uint16_t ntc = battery_read_ntc();

    if (ntc > BATTERY_THERMISTOR_MIN_TEMP ||
           (ntc < BATTERY_THERMISTOR_MAX_TEMP &&
            ntc >= BATTERY_THERMISTOR_MIN_VALUE))
    {
        vreg_off_set_reason(VREG_REASON_OFF_TEMPERATURE);

        /* Enter STOP mode immediately without considering any wake
         * reasons. Ensure that the VBUS regulator is disabled. */
        power_reset_to_stop(VBUS_AFTER_STOP_DISABLED);
    }
    else
    {
        vreg_off_clear_reason(VREG_REASON_OFF_TEMPERATURE);
    }
}

void battery_periodic(void)
{
    if (overcurrent_event)
    {
        overcurrent_event = false;

        if (battery_load_state != BATTERY_LOAD_OVERLOADED)
        {
            if (vreg_is_switched_on())
            {
                PRINTF_B("VSYS overloaded");
                battery_overcurrent_event();
                vreg_off_set_reason(VREG_REASON_OFF_LOW_BATTERY);
            }
        }
    }

    battery_voltage_monitoring();

    battery_temperature_monitoring();

#ifdef HAVE_CURRENT_SENSE
    battery_current_monitoring();
#endif

#ifdef HAVE_LOAD_SWITCH
    battery_load_switch_monitor();
#endif


    /* Ensure that we read the case battery before initiating the status
     * sequence with the earbuds. */
    switch (battery_status.state)
    {
        case BATTERY_START_READING:
            /* We must enable the VBAT monitor circuit and then wait for it
             * to settle before taking a measurement. */
            battery_monitor_set_reason(BATTERY_MONITOR_REASON_READING);
            vreg_off_set_reason(VREG_REASON_OFF_BATTERY_READING);

            uint16_t pre_battery_mv = battery_mv();

            /* Remember the current charging status before we temporarily stop it */
            battery_status.is_charging = charger_is_charging();

            /* We only need to disable the charger if a battery is present. */
            if (pre_battery_mv >= BATTERY_PRESENT_MV)
            {
                charger_set_reason(CHARGER_OFF_BATTERY_READ);
            }

#ifdef HAVE_CURRENT_SENSE
            current_senses_set_sense_amp(CURRENT_SENSE_AMP_BATTERY);
#endif
            battery_next_state(BATTERY_READING, BATTERY_READ_DELAY_TIME);
            battery_status.read_ctr = 0;
            battery_status.cutoff_mv = config_get_battery_cutoff_mv();
            power_set_run_reason(POWER_RUN_BATTERY_READ);
            break;

        case BATTERY_READING:
            /* Wait and then instruct the ADC to take the measurement. */
            if (battery_delay_finished())
            {
                if (adc_start_measuring())
                {
                    battery_next_state(BATTERY_STOP_READING, BATTERY_ADC_DELAY_TIME);
                }
            }
            break;

        case BATTERY_STOP_READING:
            /* Wait for the ADC to take the measurement. */
            if (battery_delay_finished())
            {
                uint16_t raw_mv = battery_mv();
                uint8_t percentage;

#ifdef HAVE_CURRENT_SENSE
                {
                    uint32_t total_load_ma = battery_fetch_total_load_ma();
                    percentage = battery_percentage(
                                    battery_compensated_voltage(raw_mv, total_load_ma));
                }
#else
                percentage = battery_percentage(raw_mv);
#endif
                battery_add_reading(raw_mv, percentage);

                if (raw_mv < battery_status.cutoff_mv)
                {
                    /*
                     * Reading was below the configured cutoff threshold.
                     */
                    if (++battery_status.read_ctr < BATTERY_NO_OF_CUTOFF_READS)
                    {
                        battery_next_state(BATTERY_READING, 0);
                        break;
                    }
                    else
                    {
                        /*
                         * Readings persistently low, so go to standby if no
                         * charger is present.
                         */
                        if (!charger_connected())
                        {
                            power_set_standby_reason(POWER_STANDBY_LOW_BATTERY);
                        }
                    }
                }
                else
                {
                    power_clear_standby_reason(POWER_STANDBY_LOW_BATTERY);
                }

                /* We no longer need the VBAT monitor HW to be powered */
                battery_monitor_clear_reason(BATTERY_MONITOR_REASON_READING);
#ifdef HAVE_CURRENT_SENSE
                current_senses_clear_sense_amp(CURRENT_SENSE_AMP_BATTERY);
#endif
                charger_clear_reason(CHARGER_OFF_BATTERY_READ);
                vreg_off_clear_reason(VREG_REASON_OFF_BATTERY_READING);
                power_clear_run_reason(POWER_RUN_BATTERY_READ);
                battery_next_state(BATTERY_DONE, BATTERY_READ_DELAY_TIME);

                if (battery_status.led)
                {
                    led_indicate_battery(battery_percentage_current());
                }

                /*
                * If a command to read the battery is in progress, display
                * the result.
                */
                if (battery_status.cmd_source != CLI_SOURCE_NONE)
                {
                    uint8_t cmd_source = battery_status.cmd_source;

                    /* Report the instantaneous battery voltage and a 
                     * rolling average battery percentage */
                    PRINTF("%u,%u",
                        battery_status.current_battery_mv,
                        battery_percentage_current());
                    PRINTF("OK");
                    battery_status.cmd_source = CLI_SOURCE_NONE;
                }
            }
            break;

        case BATTERY_IDLE:
        case BATTERY_DONE:
        default:
            break;
    }
}

void battery_read_request(bool led)
{
    if ((battery_status.state==BATTERY_IDLE) ||
        (battery_status.state==BATTERY_DONE))
    {
        /*
        * No battery read in progress, so start one immediately 
        */
        battery_next_state(BATTERY_START_READING, 0);
        battery_status.led = led;
    }
    else if (led)
    {
        /*
        * Set the led flag, so that the battery read already in progress
        * will report to the LED module at the end.
        */
        battery_status.led = true;
    }
}

bool battery_read_done(void)
{
    return (battery_status.state == BATTERY_DONE) ? true:false;
}

uint16_t battery_read_ntc(void)
{
    uint16_t ntc_mv;

    gpio_enable(GPIO_NTC_MONITOR_ON_OFF);
    adc_blocking_measure();
    ntc_mv = adc_read_mv(ADC_NTC, 3300);
    gpio_disable(GPIO_NTC_MONITOR_ON_OFF);

    return ntc_mv;
}

CLI_RESULT atq_ntc(uint8_t cmd_source)
{
    PRINTF("%u", battery_read_ntc());
    return CLI_OK;
}

CLI_RESULT atq_battery(uint8_t cmd_source)
{
    CLI_RESULT ret = CLI_ERROR;

    if (battery_status.cmd_source==CLI_SOURCE_NONE)
    {
        battery_read_request(false);
        battery_status.cmd_source = cmd_source;
        ret = CLI_WAIT;
    }

    return ret;
}
