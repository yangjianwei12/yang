/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       app_sensor_bus.h
\brief      App interface to Sensor Bus
*/

#ifndef APP_SENSOR_BUS_H
#define APP_SENSOR_BUS_H

#include <message.h>
#include <sensor.h>

/*! \brief Initialise the App Sensor Bus
    \param task Task to receive completion message.  Not used.
    \return Always returns TRUE
*/
bool AppSensorBus_Init(Task task);

/*! \brief Get the ID of the App common sensor bus
    \return bus The sensor bus common to App peripherals
*/
Bus AppSensorBus_Bus(void);

#endif /* APP_SENSOR_BUS_H */
