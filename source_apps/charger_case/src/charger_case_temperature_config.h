/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       charger_case_temperature_config.h
\brief      Application specific voltage->temperature configuration
*/

#ifndef CHARGER_CASE_TEMPERATURE_CONFIG_H_
#define CHARGER_CASE_TEMPERATURE_CONFIG_H_

#include "temperature.h"

/*! \brief Return the voltage->temperature lookup configuration table for the charger_case application.

    The configuration table can be passed directly to the temperature component in
    domains.

    \param table_length - used to return the number of rows in the config table.

    \return Application specific temperature configuration table.
*/
const temperature_lookup_t* ChargerCaseTemperature_GetConfigTable(unsigned* table_length);

#endif /* CHARGER_CASE_TEMPERATURE_CONFIG_H_ */

