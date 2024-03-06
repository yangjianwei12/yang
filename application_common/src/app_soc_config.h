/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       app_soc_config.h
\brief      Application specific voltage->percentage configuration
*/

#ifndef APP_SOC_CONFIG_H_
#define APP_SOC_CONFIG_H_

#include "state_of_charge.h"

/*! \brief Return the voltage->percentage lookup configuration table for the application.

    The configuration table can be passed directly to the SoC component in
    domains.

    \param table_length - used to return the number of rows in the config table.

    \return Application specific SoC configuration table.
*/
const soc_lookup_t* AppSoC_GetConfigTable(unsigned* table_length);

/* The app wants the "low" battery notifications below 30%, and "full" above 90%
   of the full battery voltage (4080mV for a 4200mV float voltage).
   The low/full battery status controls which LED pattern is displayed.
   The RDP is configured such that 100% is reported at less than the vfloat voltage. This
   is to compensate for the slight change in voltage measured between the charging and
   discharging cycles, to avoid a sudden drop in reported voltage when the user removes 
   the charger.
   Since the percentage is taken from the look-up table in app_soc_config.c rather than
   the battery voltage level, we need to choose the percentage value which corresponds to 
   this voltage rather than the percentage based on the full range.
*/
#define appSocBatteryLow() (0)
#ifdef HAVE_RDP_UI
#define appSocBatteryFull() (99)
#else
#define appSocBatteryFull() (90)
#endif /* HAVE_RDP_UI */

#endif /* APP_SOC_CONFIG_H_ */

