/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       charger_case_region_config.c
\brief     operating region configuration tables and state handlers

    This file contains battery operating configuration tables
*/

#include "charger_case_region_config.h"
#include "battery_region.h"

/*! \brief charge mode config table*/
const charge_region_t charger_case_charge_mode_config_table[] =
{
    {0,       Vfloat,   4350, 100, -40, 0,  1, NORMAL_REGION, 0},
    {0,       Vfloat,   4350, 100,  0,  45, 1, NORMAL_REGION, 0},
    {0,       Vfloat,   4350, 100,  45, 85, 1, NORMAL_REGION, 0},
    {0,         3600, Vfloat, 50,  -40, 0,  1, NORMAL_REGION, 0},
    {FAST,      3600, Vfloat, 50,    0, 45, 1, NORMAL_REGION, CHARGING_TIMER_TIMEOUT},
    {0,         3600, Vfloat, 50,   45, 85, 1, NORMAL_REGION, 0},    
    {0,        Vcrit,   3600, 50,  -40, 0,  1, NORMAL_REGION, 0},
    {FAST,     Vcrit,   3600, 50,    0, 45, 1, NORMAL_REGION, CHARGING_TIMER_TIMEOUT},
    {0,        Vcrit,   3600, 50,   45, 85, 1, NORMAL_REGION, 0},    
    {0,        Vfast,  Vcrit, 50,  -40, 0,  1, CRITICAL_REGION, 0},
    {FAST,     Vfast,  Vcrit, 50,    0, 45, 1, CRITICAL_REGION, 0},
    {0,        Vfast,  Vcrit, 50,   45, 85, 1, CRITICAL_REGION, 0},    
    {0,         Vpre,  Vfast, 50,  -40, 0,  1, CRITICAL_REGION, 0},
    {PRE,       Vpre,  Vfast, 50,    0, 45, 1, CRITICAL_REGION, 0},
    {0,         Vpre,  Vfast, 50,   45, 85, 1, CRITICAL_REGION, 0},
    {0,            0,   Vpre, 50,  -40, 0,  1, CRITICAL_REGION, 0},
    {TRICKLE,      0,   Vpre, 50,    0, 45, 1, CRITICAL_REGION, 0},
    {0,            0,   Vpre, 50,   45, 85, 1, CRITICAL_REGION, 0},
};

/*! \brief discharge mode config table*/
const charge_region_t charger_case_discharge_mode_config_table[] =
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
const battery_region_handlers_t charger_case_region_handlers =
{
    {NULL},
    {NULL},
    {NULL}
};

const charge_region_t* ChargerCaseRegion_GetChargeModeConfigTable(unsigned* table_length)
{
    *table_length = ARRAY_DIM(charger_case_charge_mode_config_table);
    return charger_case_charge_mode_config_table;
}

const charge_region_t* ChargerCaseRegion_GetDischargeModeConfigTable(unsigned* table_length)
{
    *table_length = ARRAY_DIM(charger_case_discharge_mode_config_table);
    return charger_case_discharge_mode_config_table;
}

const battery_region_handlers_t* ChargerCaseRegion_GetRegionHandlers(void)
{
    return &charger_case_region_handlers;
}

