/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       lsm6ds.h
\brief      Header file for lsm6ds(3/tq) Accelerometer/Gyro
*/

#ifndef SENSOR_LSM6DS_H
#define SENSOR_LSM6DS_H

#include <sensor/sensor_if.h>

Sensor SensorLsm6ds(Bus bus_id);

enum
{
    SENSOR_CONFIG_LSM6DS_INTERVAL = SENSOR_CONFIG_DRIVER,
    SENSOR_CONFIG_LSM6DS_GYRO_DPS,
    SENSOR_CONFIG_LSM6DS_ACCEL_G,
};

#define LSM6DS_13HZ      (13)
#define LSM6DS_26HZ      (26)
#define LSM6DS_52HZ      (52)
#define LSM6DS_104HZ     (104)
#define LSM6DS_208HZ     (208)
#define LSM6DS_416HZ     (416)
#define LSM6DS_PD        (0)


#endif
