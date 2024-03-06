/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Battery
*/

#ifndef BATTERY_DATA_H_
#define BATTERY_DATA_H_

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/

typedef struct
{
    /* The voltage of the battery at this level in millivolts. */
    uint16_t voltage_mv;

    /* The percentage of charge this level represents from 0% to 100%. */
    uint8_t percentage;
}
BATTERY_LEVEL;

typedef struct
{
    /* The load on VBUS supplying the earbuds. This is assumed to be total load
     * on the case battery so regulator efficiency and other peripheral power
     * consumption is negligible. */
    uint16_t load_mA;

    /* The expected drop in battery voltage under the load. Measured in millivolts. */
    uint16_t battery_drop_mv;
}
BATTERY_DROP;

typedef struct
{
    /* The voltage these drops apply to */
    uint16_t battery_voltage_mv;

    /* A series of voltage drops we expect at different loads.*/
    const BATTERY_DROP *drops;

    /* The number of drops */
    size_t num_drops;
}
BATTERY_DROPS;

/*-----------------------------------------------------------------------------
------------------ VARIABLES --------------------------------------------------
-----------------------------------------------------------------------------*/

#if defined(HAVE_BATTERY_VDL602045_545MA)
/*
 * Measured battery capacity levels of the VDL 602045 545mA
 * 3.7V lithium battery.
 * These were determined using the B2281S-20-6 Battery Simulator in mode 9.
 */
static const BATTERY_LEVEL battery_levels[] =
{   
    {3500, 0},
    {3627, 1},
    {3784, 5},
    {3803, 10},
    {3834, 15},
    {3860, 20},
    {3884, 25},
    {3897, 30},
    {3920, 40},
    {3949, 50},
    {3981, 60},
    {4018, 70},
    {4063, 80},
    {4115, 90},
    {4139, 95},
    {4160, 100},
};
static const size_t battery_levels_num = sizeof(battery_levels) / sizeof(BATTERY_LEVEL);

#ifdef HAVE_CURRENT_SENSE
/*
 * Measured battery voltage drops of the VDL 602045 545mA
 * 3.7V lithium battery.
 * These were determined using the B2281S-20-6 Battery Simulator in mode 9.
 */
static const BATTERY_DROP DROPS_3V5[] =  {{0,0}, {50,38}, {100,73}, {200,161}, {280,215}};
static const BATTERY_DROP DROPS_3V8[] =  {{0,0}, {50,31}, {100,65}, {200,134}, {280,187}};
static const BATTERY_DROP DROPS_3V86[] = {{0,0}, {50,29}, {100,64}, {200,126}, {280,179}};
static const BATTERY_DROP DROPS_4V[] =   {{0,0}, {50,31}, {100,58}, {200,114}, {280,161}};
static const BATTERY_DROP DROPS_4V1[] =  {{0,0}, {50,34}, {100,63}, {200,123}, {280,172}};
static const BATTERY_DROP DROPS_4V16[] = {{0,0}, {50,33}, {100,64}, {200,125}, {280,172}};

static const BATTERY_DROPS battery_drops[] = {
    {3500, DROPS_3V5,  sizeof(DROPS_3V5)/sizeof(BATTERY_DROP)},
    {3803, DROPS_3V8,  sizeof(DROPS_3V8)/sizeof(BATTERY_DROP)},
    {3860, DROPS_3V86, sizeof(DROPS_3V86)/sizeof(BATTERY_DROP)},
    {4063, DROPS_4V,   sizeof(DROPS_4V)/sizeof(BATTERY_DROP)},
    {4115, DROPS_4V1,  sizeof(DROPS_4V1)/sizeof(BATTERY_DROP)},
    {4160, DROPS_4V16, sizeof(DROPS_4V16)/sizeof(BATTERY_DROP)}
};
static const size_t battery_drops_num = sizeof(battery_drops) / sizeof(BATTERY_DROPS);
#endif

/**
 * NTC Thermistor values for various important temperatures.
 *
 * Measured with a ECTH100505 103F 3435 FST thermistor through 3.3V 10k
 * resistor ladder. Values are defined in millivolts.
 */
#define BATTERY_THERMISTOR_VOLTAGE_MINUS_20C     2867
#define BATTERY_THERMISTOR_VOLTAGE_0_C           2360 
#define BATTERY_THERMISTOR_VOLTAGE_15_C          1900 
#define BATTERY_THERMISTOR_VOLTAGE_45_C          1120
#define BATTERY_THERMISTOR_VOLTAGE_60C           781
#define BATTERY_THERMISTOR_MIN_VALUE             100
#define BATTERY_THERMISTOR_MIN_TEMP BATTERY_THERMISTOR_VOLTAGE_MINUS_20C
#define BATTERY_THERMISTOR_MAX_TEMP BATTERY_THERMISTOR_VOLTAGE_60C

#elif defined(HAVE_BATTERY_BPI_PL652043_590MA)

/*
 * Measured battery capacity levels of the BPI_PL652043_590mA
 * 3.7V lithium battery.
 * These were determined using the B2281S-20-6 Battery Simulator in mode 9.
 *
 * Note: There is a -30mV constant offset to all these values which represent
 * the voltage drop between the battery and what the ADC reads.
 */
static const BATTERY_LEVEL battery_levels[] =
{   
    {2870, 0},
    {3660, 1},
    {3750, 5},
    {3800, 10},
    {3825, 15},
    {3850, 20},
    {3870, 25},
    {3890, 30},
    {3905, 35},
    {3920, 40},
    {3935, 45},
    {3950, 50},
    {3965, 55},
    {3980, 60},
    {4000, 65},
    {4020, 70},
    {4050, 75},
    {4080, 80},
    {4090, 85},
    {4110, 90},
    {4130, 95},
    {4145, 100}
};
static const size_t battery_levels_num = sizeof(battery_levels) / sizeof(BATTERY_LEVEL);

/**
 * NTC Thermistor values for various important temperatures.
 *
 * Measured with a ECTH100505 103F 3435 FST thermistor through 3.3V 10k
 * resistor ladder. Values are defined in millivolts.
 */
#define BATTERY_THERMISTOR_VOLTAGE_MINUS_20C     2867
#define BATTERY_THERMISTOR_VOLTAGE_0_C           2360 
#define BATTERY_THERMISTOR_VOLTAGE_15_C          1900 
#define BATTERY_THERMISTOR_VOLTAGE_45_C          1120
#define BATTERY_THERMISTOR_VOLTAGE_60C           781
#define BATTERY_THERMISTOR_MIN_VALUE             100
#define BATTERY_THERMISTOR_MIN_TEMP BATTERY_THERMISTOR_VOLTAGE_MINUS_20C
#define BATTERY_THERMISTOR_MAX_TEMP BATTERY_THERMISTOR_VOLTAGE_60C

#else
#error "No battery data defined"
#endif

/*-----------------------------------------------------------------------------
------------------ PROTOTYPES -------------------------------------------------
-----------------------------------------------------------------------------*/

#endif /* BATTERY_DATA_H_ */
