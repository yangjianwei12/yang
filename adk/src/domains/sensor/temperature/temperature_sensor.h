/*!
    \copyright  Copyright (c) 2018 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       temperature_sensor.h
    \addtogroup sensor_temperature
    \brief      Header file defining the temperature sensor API. Temperature sensors
                (e.g. thermistor) should provide an implementation for this API.
                The API is called by the temperature module.
    @{
*/

#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include "message.h"
#include "types.h"

#if defined(INCLUDE_TEMPERATURE)

/*! \brief Initialise the sensor for taking measurements */
void appTemperatureSensorInit(void);

/*! \brief Request a new temperature measurement is taken.
    \param task The task to which any measurement related messages should be delivered.
    \note The caller will call #appTemperatureSensorHandleMessage for any messages
    delivered to task. */
void appTemperatureSensorRequestMeasurement(Task task);

/*! \brief Get Thermistor ADC source value.

    \return ADC source */
vm_adc_source_type appTemperatureSensorAdcSource(void);

/*! \brief Enable/disable the thermistor 'on' PIO */
void appThermistorPIOEnable(bool enable);

#endif /* INCLUDE_TEMPERATURE */

#endif /* TEMPERATURE_SENSOR_H */

/*! @} */