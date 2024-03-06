/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       app_sensor_bus.h
\brief      App interface to Sensor Bus
*/

#include "app_sensor_bus.h"

#include <logging.h>
#include <sensor.h>

#ifdef INCLUDE_ACCESSORY
#include <accessory.h>
#endif

static Bus sensor_bus = 0;

bool AppSensorBus_Init(Task task)
{
    UNUSED(task);

#if defined RDP_PIO_I2C_SCL_1 && defined RDP_PIO_I2C_SDA_1
    sensor_bus = BusI2c(0, RDP_PIO_I2C_SCL_1, RDP_PIO_I2C_SDA_1);
    DEBUG_LOG_DEBUG("AppSensorBus_Init: bus=%u", sensor_bus);
    PanicFalse(sensor_bus != 0);

#ifdef INCLUDE_ACCESSORY
    Accessory_ConnectSensorBus(sensor_bus);
#endif

#endif /* RDP_PIO_I2C_SCL_1 && RDP_PIO_I2C_SDA_1 */

    return TRUE;
}

Bus AppSensorBus_Bus(void)
{
    return sensor_bus;
}
