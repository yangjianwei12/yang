/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       charger_case_region_config.h
\brief     Application specific battery operating region configuration
*/

#ifndef CHARGER_CASE_REGION_CONFIG_H_
#define CHARGER_CASE_REGION_CONFIG_H_

#include "battery_region.h"
#include "charger_monitor_config.h"

#define TRICKLE appConfigChargerTrickleCurrent()
#define PRE     appConfigChargerPreCurrent()
#define FAST    appConfigChargerFastCurrent()

#define Vpre   2100
#define Vcrit  appConfigChargerCriticalThresholdVoltage()
#define Vfast  appConfigChargerPreFastThresholdVoltage()
#define Vfloat appConfigChargerTerminationVoltage()

/* Enable short timeouts for charger/battery platform testing */
#ifdef CF133_BATT
#define CHARGING_TIMER_TIMEOUT 15
#else
#define CHARGING_TIMER_TIMEOUT 509
#endif

/*! \brief Return the charge mode region configuration table for the charger_case application.

    The configuration table can be passed directly to the battery region component in
    domains.

    \param table_length - used to return the number of rows in the config table.

    \return Application specific battery operating region configuration
*/
const charge_region_t* ChargerCaseRegion_GetChargeModeConfigTable(unsigned* table_length);

/*! \brief Return the discharge mode region configuration table for the charger_case application.

    The configuration table can be passed directly to the battery region component in
    domains.

    \param table_length - used to return the number of rows in the config table.

    \return Application specific battery operating region configuration
*/
const charge_region_t* ChargerCaseRegion_GetDischargeModeConfigTable(unsigned* table_length);

/*! \brief Return battery operating region state handler structure for the charger_case application.

    The configuration table can be passed directly to the battery region component in
    domains.

    \return Application specific battery operating region handler functions
*/
const battery_region_handlers_t* ChargerCaseRegion_GetRegionHandlers(void);

#endif /* CHARGER_CASE_REGION_CONFIG_H_ */

