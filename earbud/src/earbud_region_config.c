/*!
\copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       earbud_region_config.c
\brief     operating region configuration tables and state handlers

    This file contains battery operating configuration tables
*/

#include "earbud_region_config.h"
#include "battery_region.h"

/*! \brief charge mode config table*/
const charge_region_t app_charge_mode_config_table[] =
{
#ifndef BATTERY_ZJ1454
    {0,       Vfloat_adjusted,   5000, 100, -40, 0,  1, NORMAL_REGION, 0},
    {0,       Vfloat_adjusted,   5000, 100,  0,  45, 1, NORMAL_REGION, 0},
    {0,       Vfloat_adjusted,   5000, 100,  45, 85, 1, NORMAL_REGION, 0},
    {0,         3600, Vfloat_adjusted, 50,  -40, 0,  1, NORMAL_REGION, 0},
    {FAST,      3600, Vfloat_adjusted, 50,  minFastRegionTemp, maxFastRegionTemp,
                                            1, NORMAL_REGION, CHARGING_TIMER_TIMEOUT},
    {0,         3600, Vfloat_adjusted, 50,   45, 85, 1, NORMAL_REGION, 0},
    {0,        Vcrit,   3600, 50,  -40, 0,  1, NORMAL_REGION, 0},
    {FAST,     Vcrit,   3600, 50,  minFastRegionTemp, maxFastRegionTemp,
                                            1, NORMAL_REGION, CHARGING_TIMER_TIMEOUT},
    {0,        Vcrit,   3600, 50,   45, 85, 1, NORMAL_REGION, 0},
    {0,        Vfast,  Vcrit, 50,  -40, 0,  1, CRITICAL_REGION, 0},
    {FAST,     Vfast,  Vcrit, 50,  minFastRegionTemp, maxFastRegionTemp,
                                            1, CRITICAL_REGION, 0},
    {0,        Vfast,  Vcrit, 50,   45, 85, 1, CRITICAL_REGION, 0},    
    {0,         Vpre,  Vfast, 50,  -40, 0,  1, CRITICAL_REGION, 0},
    {PRE,       Vpre,  Vfast, 50,    0, 45, 1, CRITICAL_REGION, 0},
    {0,         Vpre,  Vfast, 50,   45, 85, 1, CRITICAL_REGION, 0},
    {0,            0,   Vpre, 50,  -40, 0,  1, CRITICAL_REGION, 0},
    {TRICKLE,      0,   Vpre, 50,    0, 45, 1, CRITICAL_REGION, 0},
    {0,            0,   Vpre, 50,   45, 85, 1, CRITICAL_REGION, 0},
#else
    /* Treat < 3.0V as "critical" region; Normal region is > 3.0 V */

    /* Do not enable the charger if VBAT > VFloat for all temperatures */
    {CHG_OFF,    Vfloat_adjusted,   5000, 100, minTemp, maxTemp, 1, NORMAL_REGION, 0},

    /* Do not enable the charger if 0 < Temp or Temp > 60 in the "normal" voltage region */
    {CHG_OFF,    Vfast, 5000, 50,  minTemp, minFastRegionTemp,  1, NORMAL_REGION, 0},
    {CHG_OFF,    Vfast, 5000, 50,  maxFastRegionTemp, maxTemp,  1, NORMAL_REGION, 0},

    /* Reduced charge profile between 0 and 15 degrees when VBAT is between 3.0 and Vfloat */
    {REDUCED_FAST,       Vfast, Vfloat_adjusted, 50, minFastRegionTemp,      minExtraFastRegionTemp, 1, NORMAL_REGION, CHARGING_TIMER_TIMEOUT},
    /* Fast charge, 1.6C, between 15 and 45 degrees. We limit to 1.6C otherwise the device risks heating up towards 45 degrees. */
    {FAST, Vfast, Vfloat_adjusted, 50, minExtraFastRegionTemp, maxExtraFastRegionTemp, 1, NORMAL_REGION, CHARGING_TIMER_TIMEOUT},
    /* Do not enable the charger if 0 < Temp or Temp > 60 in the "critical" voltage region */
    {CHG_OFF,       0,  Vfast, 50,  minTemp,           minFastRegionTemp, 1, CRITICAL_REGION, 0},
    {CHG_OFF,       0,  Vfast, 50,  maxFastRegionTemp, maxTemp,           1, CRITICAL_REGION, 0},
    
    /* 2.1V to 3.0V region */
    {FAST,       Vpre,  Vfast, 50,  minFastRegionTemp, maxFastRegionTemp, 1, CRITICAL_REGION, 0},
    /* Dead to 2.1V */
    {FAST,         0,   Vpre,  50,  minFastRegionTemp, maxFastRegionTemp, 1, CRITICAL_REGION, 0},
#endif /* BATTERY_ZJ1454 */
};

/*! \brief discharge mode config table*/
const charge_region_t app_discharge_mode_config_table[] =
{
    {0, 4200, 4350, 100, -40, -20, 1, NORMAL_REGION, 0},
    {0, 4200, 4350, 100, -20,  60, 1, NORMAL_REGION, 0},
    {0, 4200, 4350, 100,  60,  85, 1, NORMAL_REGION, 0},      
    {0, 3300, 4200, 50,  -40, -20, 1, NORMAL_REGION, 0},
    {0, 3300, 4200, 50,  -20,  60, 1, NORMAL_REGION, 0},
    {0, 3300, 4200, 50,   60,  85, 1, NORMAL_REGION, 0},
    {0, 3000, 3300, 50,  -40, -20, 1, CRITICAL_REGION, 0},
    {0, 3000, 3300, 50,  -20,  60, 1, CRITICAL_REGION, 0},
    {0, 3000, 3300, 50,   60,  85, 1, CRITICAL_REGION, 0},
    {0,    0, 3000, 50,  -40, -20, 1, SAFETY_REGION, 0},
    {0,    0, 3000, 50,  -20,  60, 1, SAFETY_REGION, 0},
    {0,    0, 3000, 50,   60,  85, 1, SAFETY_REGION, 0},
};

/*! \brief battery region component various state handlers*/
const battery_region_handlers_t app_region_handlers =
{
    NULL,
    NULL,
    NULL
};

const charge_region_t* AppRegion_GetChargeModeConfigTable(unsigned* table_length)
{
    *table_length = ARRAY_DIM(app_charge_mode_config_table);
    return app_charge_mode_config_table;
}

const charge_region_t* AppRegion_GetDischargeModeConfigTable(unsigned* table_length)
{
    *table_length = ARRAY_DIM(app_discharge_mode_config_table);
    return app_discharge_mode_config_table;
}

const battery_region_handlers_t* AppRegion_GetRegionHandlers(void)
{
    return &app_region_handlers;
}

