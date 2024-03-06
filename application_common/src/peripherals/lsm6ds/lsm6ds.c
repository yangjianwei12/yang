/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       lsm6ds.c
\brief      Driver for LSM6DSM/LS6DSTQ accelerometer and gyro.
*/

#ifdef INCLUDE_SPATIAL_DATA

#include <sensor.h>
#include <sensor/sensor_if.h>
#include <logging.h>
#include <panic.h>
#include <stdint.h>

#include "lsm6ds.h"

#define SENSOR_LSM6DSO /* OK for LSM6DSO or LSM6DSTQ */

/* Register addresses */
#define LSM6DS_WHO_AM_I         (0x0F)
#define LSM6DS_INT1_CTRL        (0x0D)
#define LSM6DS_CTRL1_XL         (0x10)
#define LSM6DS_CTRL2_G          (0x11)
#define LSM6DS_CTRL3_C          (0x12)
#define LSM6DS_CTRL6_C          (0x15)
#define LSM6DS_CTRL7_G          (0x16)
#define LSM6DS_CTRL9_XL         (0x18)
#define LSM6DS_CTRL10_C         (0x19)
#define LSM6DS_OUT_TEMP_L       (0x20)
#define LSM6DS_OUT_TEMP_H       (0x21)
#define LSM6DS_OUTX_L_G         (0x22)
#define LSM6DS_OUTX_H_G         (0x23)
#define LSM6DS_OUTY_L_G         (0x24)
#define LSM6DS_OUTY_H_G         (0x25)
#define LSM6DS_OUTZ_L_G         (0x26)
#define LSM6DS_OUTZ_H_G         (0x27)
#define LSM6DS_OUTX_L_XL        (0x28)
#define LSM6DS_OUTX_H_XL        (0x29)
#define LSM6DS_OUTY_L_XL        (0x2A)
#define LSM6DS_OUTY_H_XL        (0x2B)
#define LSM6DS_OUTZ_L_XL        (0x2C)
#define LSM6DS_OUTZ_H_XL        (0x2D)

/* Accelerometer output data rates */
#define LSM6DS_ODR_XL_13HZ      (1 << 4)
#define LSM6DS_ODR_XL_26HZ      (2 << 4)
#define LSM6DS_ODR_XL_52HZ      (3 << 4)
#define LSM6DS_ODR_XL_104HZ     (4 << 4)
#define LSM6DS_ODR_XL_208HZ     (5 << 4)
#define LSM6DS_ODR_XL_416HZ     (6 << 4)
#define LSM6DS_ODR_XL_PD        (0 << 4)

/* Accelerometer full-scale ranges */
#define LSM6DS_FS_XL_2G         (0 << 2)
#define LSM6DS_FS_XL_16G        (1 << 2)
#define LSM6DS_FS_XL_4G         (2 << 2)
#define LSM6DS_FS_XL_8G         (3 << 2)

/* Accelerometer bandwidth options (LSM6DS3 only) */
#define LSM6DS3_BW_XL_400HZ     (0 << 0)
#define LSM6DS3_BW_XL_200HZ     (1 << 0)
#define LSM6DS3_BW_XL_100HZ     (2 << 0)
#define LSM6DS3_BW_XL_50HZ      (3 << 0)

/* Gyro output data rates */
#define LSM6DS_ODR_G_13HZ       (1 << 4)
#define LSM6DS_ODR_G_26HZ       (2 << 4)
#define LSM6DS_ODR_G_52HZ       (3 << 4)
#define LSM6DS_ODR_G_104HZ      (4 << 4)
#define LSM6DS_ODR_G_208HZ      (5 << 4)
#define LSM6DS_ODR_G_416HZ      (6 << 4)
#define LSM6DS_ODR_G_PD         (0 << 4)

/* Gyro full-scale ranges */
#define LSM6DS_FS_G_250DPS      (0 << 2)
#define LSM6DS_FS_G_500DPS      (1 << 2)
#define LSM6DS_FS_G_1000DPS     (2 << 2)
#define LSM6DS_FS_G_2000DPS     (3 << 2)

#define LSM6DS_GYRO_SCALE_128   (145) /* Gyro values multiplied by 145/128 */

/* Sensor driver data structure */
typedef struct
{
    unsigned interrupt_pio:7;   /* PIO to be used for interrupt */
    unsigned use_interrupt:1;   /* 1 if interrupt mode enabled */
    uint8 id;                   /* ID of sensor, read from WHO_AM_I register */
    uint16 interval;             /* Current output data rate interval */
} sensor_lsm6ds_t;


/* Useful structure for table of register values */
typedef struct
{
    uint8 reg;      /* Register address */
    uint8 value;    /* Register value */
} reg_init_t;


/* Function prototypes */
static bool sensorLsm6dsGetConfig(Sensor sensor_id, sensor_config_key_t key, uint32 *value);
static void sensorLsm6dsIntHandler(uintptr_t context, uint32 mask);


/* Scale sensor value. */
static int16 sensorScaleValue(int16 value, uint8 scale_128)
{
    int32 scaled = (int32)value * scale_128 / 128;
    if (scaled >= 0)
    {
        scaled = MIN(scaled, SHRT_MAX);
    }
    else
    {
        scaled = MAX(scaled, SHRT_MIN);
    }
    return (int16)scaled;
}

/* Utility function to write a table of register values */
static void sensorWriteRegs(Sensor sensor_id, const reg_init_t *table, uint8 num_regs)
{
    for (int reg = 0; reg < num_regs; reg++)
    {
        DEBUG_LOG_INFO("sensorWriteRegs, reg %02x=%02x", table[reg].reg, table[reg].value);
        SensorTransfer(sensor_id, &table[reg], 2, NULL, 0);
    }
}


/* Attempts to set accel/gyro ODR interval, returns TRUE if interval was valid, FALSE if it was not */
static bool sensorSetInterval(Sensor sensor_id, uint16 *interval)
{
    reg_init_t r[2] =
    {
#if defined(SENSOR_LSM6DS3)
        { LSM6DS_CTRL1_XL, LSM6DS3_BW_XL_200HZ | LSM6DS_FS_XL_16G },
        { LSM6DS_CTRL2_G, LSM6DS_FS_G_1000DPS},
#elif defined(SENSOR_LSM6DSO)
        { LSM6DS_CTRL1_XL, LSM6DS_FS_XL_16G },
        { LSM6DS_CTRL2_G, LSM6DS_FS_G_1000DPS},
#endif
    };

    /* Lookup table of supported intervals and their corresponding accel and gyro register values */
    const struct { uint16 interval; uint8 g; uint8 xl; } t[] =
    {
        { LSM6DS_PD,    LSM6DS_ODR_G_PD,    LSM6DS_ODR_XL_PD    },
        { LSM6DS_13HZ,  LSM6DS_ODR_G_13HZ,  LSM6DS_ODR_XL_13HZ  },
        { LSM6DS_26HZ,  LSM6DS_ODR_G_26HZ,  LSM6DS_ODR_XL_26HZ  },
        { LSM6DS_52HZ,  LSM6DS_ODR_G_52HZ,  LSM6DS_ODR_XL_52HZ  },
        { LSM6DS_104HZ, LSM6DS_ODR_G_104HZ, LSM6DS_ODR_XL_104HZ },
        { LSM6DS_208HZ, LSM6DS_ODR_G_208HZ, LSM6DS_ODR_XL_208HZ },
        { LSM6DS_416HZ, LSM6DS_ODR_G_416HZ, LSM6DS_ODR_XL_416HZ },
    };

    /* Search table to find lowest interval not less than required */
    uint16 new_interval = 0;
    bool write_interval = FALSE;
    for (uint8 i = 0; i < ARRAY_DIM(t); i++)
    {
        if (*interval <= t[i].interval)
        {
            r[0].value |= t[i].xl;
            r[1].value |= t[i].g;
            new_interval = t[i].interval;
            write_interval = TRUE;
            break;
        }
    }

    if (write_interval)
    {
        if (SensorIsOn(sensor_id))
        {
            sensorWriteRegs(sensor_id, r, ARRAY_DIM(r));
        }
        *interval = new_interval;
    }

    return write_interval;
}

static void sensorConfigInterrupt(Sensor sensor_id, uint8 int_pio)
{
    if (SensorIsOn(sensor_id))
    {
        /* Interrupt line must be low before enabling interrupt otherwise rising edge will never happen */
        PanicFalse(!SensorGetInterruptLevel(int_pio));

        SensorRegisterInterrupt(sensor_id, int_pio, SENSOR_INTERRUPT_RISING, sensorLsm6dsIntHandler);
        PanicFalse(SensorEnablePioTimestamp(int_pio, SENSOR_INTERRUPT_RISING));

        /* Enable data ready interrupt on INT1 */
        const reg_init_t reg_table[] =
        {
            { LSM6DS_INT1_CTRL, 0x01 }     // Enable INT1
        };
        sensorWriteRegs(sensor_id, reg_table, ARRAY_DIM(reg_table));
    }
}

static void sensorDeconfigInterrupt(Sensor sensor_id, uint8 int_pio)
{
    if (SensorIsOn(sensor_id))
    {
        PanicFalse(SensorDisablePioTimestamp(int_pio));
        SensorUnregisterInterrupt(sensor_id, int_pio, sensorLsm6dsIntHandler);

        /* Disable data ready interrupt on INT1 */
        const reg_init_t reg_table[] =
        {
            { LSM6DS_INT1_CTRL, 0x00 }
        };
        sensorWriteRegs(sensor_id, reg_table, ARRAY_DIM(reg_table));

        /* Interrupt line must be low after disabling interrupt  */
        PanicFalse(!SensorGetInterruptLevel(int_pio));
    }
}

/* Called by framework when bus is powered on.  Initialises sensor to required ODR interval, optionally enable interrupt */
static void sensorLsm6dsOn(Bus bus_id, Sensor sensor_id)
{
    UNUSED(bus_id);

    sensor_lsm6ds_t *s = (sensor_lsm6ds_t *)SensorSubClassData(sensor_id);

    /* Display value of WHO_AM_I register for diagnostics */
    uint8 id = 0;
    const uint8 who_am_i = LSM6DS_WHO_AM_I;
    uint32 i2c_address;
    sensorLsm6dsGetConfig(sensor_id, SENSOR_CONFIG_I2C_ADDR, &i2c_address);
    DEBUG_LOG_INFO("SensorLsm6dsOn, reading WHO_AM_I for sensor %d, I2C address %02X", sensor_id, (uint8)i2c_address);
    if (SensorTransfer(sensor_id, &who_am_i, sizeof(who_am_i), &id, sizeof(id)))
    {
        DEBUG_LOG_INFO("SensorLsm6dsOn, Sensor WHO_AM_I %02X.", id);
    }
    else
    {
        DEBUG_LOG_INFO("SensorLsm6dsOn, SensorTransfer failed.");
    }

#if defined(SENSOR_LSM6DS3)
    PanicFalse(id == 0x69);
    const reg_init_t init_table[]=
    {
        { LSM6DS_INT1_CTRL, 0x00 },    // No interrupts by default
        { LSM6DS_CTRL1_XL,  LSM6DS_ODR_XL_PD | LSM6DS3_BW_XL_200HZ | LSM6DS_FS_XL_16G },
        { LSM6DS_CTRL2_G,   LSM6DS_ODR_G_PD | LSM6DS_FS_G_1000DPS },
    };
#elif defined(SENSOR_LSM6DSO)
    PanicFalse((id == 0x6C)||(id == 0x6D)); /* OK for LSM6DSO or LSM6DSTQ */
    const reg_init_t init_table[]=
    {
        { LSM6DS_CTRL3_C,   0x05 },    // SW Reset, auto inc address
        { LSM6DS_INT1_CTRL, 0x00 },    // No interrupts by default
        { LSM6DS_CTRL1_XL,  LSM6DS_ODR_XL_PD | LSM6DS_FS_XL_16G },
        { LSM6DS_CTRL2_G,   LSM6DS_ODR_G_PD | LSM6DS_FS_G_1000DPS },
    };
#endif
    sensorWriteRegs(sensor_id, init_table, ARRAY_DIM(init_table));

    /* Set accel & gyro output data rate interval */
    sensorSetInterval(sensor_id, &s->interval);

    if (s->use_interrupt)
    {
        sensorConfigInterrupt(sensor_id, s->interrupt_pio);
    }

#if defined(SENSOR_LSM6DS3)
    /* Enable accel, gyro */
    const reg_init_t reg_table[] =
    {
        { LSM6DS_CTRL9_XL,  0x38 },    // Acc X, Y, Z axes enabled
        { LSM6DS_CTRL10_C,  0x38 },    // Gyro X, Y, Z axes enabled
    };
    sensorWriteRegs(sensor_id, reg_table, ARRAY_DIM(reg_table));
#endif
}

/* Called by framework when bus is about to be powered off.  Puts sensor into power down mode */
static void sensorLsm6dsOff(Bus bus_id, Sensor sensor_id)
{
    UNUSED(bus_id);
    DEBUG_LOG_INFO("SensorLsm6dsOff, sensor %d", sensor_id);

    const sensor_lsm6ds_t *s = (sensor_lsm6ds_t *)SensorSubClassData(sensor_id);

    /* Deconfigure PIO for interrupts on edges */
    if (s->use_interrupt)
    {
        sensorDeconfigInterrupt(sensor_id, s->interrupt_pio);
    }

#if defined(SENSOR_LSM6DS3)
    const reg_init_t reg_table[] =
    {
        { LSM6DS_CTRL9_XL,  0x00 },    // Acc X, Y, Z axes disabled
        { LSM6DS_CTRL10_C,  0x00 },    // Gyro X, Y, Z axes disabled
        { LSM6DS_CTRL1_XL,  LSM6DS_ODR_XL_PD },
        { LSM6DS_CTRL2_G,   LSM6DS_ODR_G_PD },
        { LSM6DS_INT1_CTRL, 0x00 },    // Disable interrupts
    };
#elif defined(SENSOR_LSM6DSO)
    const reg_init_t reg_table[] =
    {
        { LSM6DS_CTRL1_XL,  LSM6DS_ODR_XL_PD },
        { LSM6DS_CTRL2_G,   LSM6DS_ODR_G_PD },
        { LSM6DS_INT1_CTRL, 0x00 },    // Disable interrupts
    };
#endif
    sensorWriteRegs(sensor_id, reg_table, ARRAY_DIM(reg_table));

    /* Inform framework that sensor is now off */
    SensorOffIndication(sensor_id);
}

/* Called by framework to read a configuration key */
static bool sensorLsm6dsGetConfig(Sensor sensor_id, sensor_config_key_t key, uint32 *value)
{
    UNUSED(sensor_id);
    sensor_lsm6ds_t *s = (sensor_lsm6ds_t *)SensorSubClassData(sensor_id);
    PanicNull(s);

    switch ((int)key)
    {
        /* Provide I2C address of sensor */
        case SENSOR_CONFIG_I2C_ADDR:
            *value = 0x6A;
            return TRUE;

        case SENSOR_CONFIG_I2C_RATE_KHZ:
            *value = 400;
            return TRUE;

        /* Provide FIFO element type this driver will generate */
        case SENSOR_CONFIG_FIFO_ELEMENT_TYPE:
            *value = SENSOR_FIFO_TYPE_TS_XYZx2_INT16_TEMP;
            return TRUE;

        /* Number of entries to store in a buffer. */
        case SENSOR_CONFIG_FIFO_MAX_ELEMENTS:
            *value = 2;
            return TRUE;

        /* Size of buffer required for data reads */
        case SENSOR_CONFIG_RAW_BUFFER_SIZE:
            *value = 14;
            return TRUE;

        /* Current ODR interval */
        case SENSOR_CONFIG_LSM6DS_INTERVAL:
        case SENSOR_CONFIG_DATA_RATE:
            *value = s->interval;
            return TRUE;

        /* Degrees per second configured. */
        case SENSOR_CONFIG_LSM6DS_GYRO_DPS:
            *value = 1000;
            return TRUE;

        /* Acceleration full scale value configured. */
        case SENSOR_CONFIG_LSM6DS_ACCEL_G:
            *value = 16;
            return TRUE;

        /* Key not known */
        default:
            return FALSE;
    }
}

/* Called by framework to write a configuration key */
static bool sensorLsm6dsSetConfig(Sensor sensor_id, sensor_config_key_t key, uint32 value)
{
    DEBUG_LOG_INFO("SensorLsm6dsSetConfig, key %u, value %u", key, value);
    sensor_lsm6ds_t *s = (sensor_lsm6ds_t *)SensorSubClassData(sensor_id);
    switch ((int)key)
    {
        case SENSOR_CONFIG_PERIOD_US:
        {
            if (s->use_interrupt)
            {
                s->use_interrupt = FALSE;
                sensorDeconfigInterrupt(sensor_id, s->interrupt_pio);
            }
        }
        return FALSE;  /* Continue with common processing */

        /* Enable interrupt mode on specified PIO */
        case SENSOR_CONFIG_INTERRUPT_PIO:
        {
            /* Turn off periodic reading */
            SensorConfigure(sensor_id, SENSOR_CONFIG_PERIOD_US, 0);

            /* Enable interrupt immediately */
            s->interrupt_pio = value;
            s->use_interrupt = TRUE;
            sensorConfigInterrupt(sensor_id, value);
        }
        return TRUE;

        /* Set output data rate interval */
        case SENSOR_CONFIG_LSM6DS_INTERVAL:
        case SENSOR_CONFIG_DATA_RATE:
        {
            if (value <= UINT16_MAX)
            {
                uint16 interval = (uint16)value;
                if (sensorSetInterval(sensor_id, &interval))
                {
                    s->interval = value;
                    return TRUE;
                }
            }
        }
        return FALSE;

        /* Key not known */
        default:
            return FALSE;
    }
}

/* Called by framework to queue up I2C/SPI transfer */
static BusTransferHandle sensorLsm6dsReadSetup(Sensor sensor_id, uint8 *raw_data_ptr, uint16 variant)
{
    UNUSED(variant); /* Only support one variant supported, so ignore this parameter */

    /* Read starts at OUT_TEMP_L register */
    static const uint8 reg = LSM6DS_OUT_TEMP_L;

    /* Get transfer handle */
    BusTransferHandle h = SensorTransferBegin(sensor_id);

    /* Write register address and then read 14 bytes */
    SensorAddWrite(sensor_id, h, &reg, sizeof(reg));
    SensorAddRead(sensor_id, h, raw_data_ptr, 14);

    /* Nothing more */
    SensorTransferEnd(sensor_id, h);
    return h;
}


/* Called by framework to process raw data */
static bool sensorLsm6dsReadComplete(Sensor sensor_id, const uint8 *raw_data_ptr, fifo_item_t *fi_ptr)
{
    /* Read raw temperature */
    int16 temp = raw_data_ptr[0]  | (raw_data_ptr[1]  << 8);

#if defined(SENSOR_LSM6DS3)
    /* Temperature is in 12.4 format, shift decimal point left by 4 then add 25.0 */
    fi_ptr->xyz2_int16_temp.temp = (temp << 4) + (25 << 8);
#elif defined(SENSOR_LSM6DSO)
    /* Temperature is already in 8.8 format, add 25.0 */
    fi_ptr->ts_xyz2_int16_temp.temp = temp + (25 << 8);
#endif

    /* Extract 16 bits of X,Y & Z for accelerometer and gyro */
    fi_ptr->ts_xyz2_int16_temp.x[0] = raw_data_ptr[2]  | (raw_data_ptr[3]  << 8);
    fi_ptr->ts_xyz2_int16_temp.y[0] = raw_data_ptr[4]  | (raw_data_ptr[5]  << 8);
    fi_ptr->ts_xyz2_int16_temp.z[0] = raw_data_ptr[6]  | (raw_data_ptr[7]  << 8);
    fi_ptr->ts_xyz2_int16_temp.x[1] = raw_data_ptr[8]  | (raw_data_ptr[9]  << 8);
    fi_ptr->ts_xyz2_int16_temp.y[1] = raw_data_ptr[10] | (raw_data_ptr[11] << 8);
    fi_ptr->ts_xyz2_int16_temp.z[1] = raw_data_ptr[12] | (raw_data_ptr[13] << 8);

    /* Scale the gyro values. */
    fi_ptr->ts_xyz2_int16_temp.x[0] = sensorScaleValue(fi_ptr->ts_xyz2_int16_temp.x[0], LSM6DS_GYRO_SCALE_128);
    fi_ptr->ts_xyz2_int16_temp.y[0] = sensorScaleValue(fi_ptr->ts_xyz2_int16_temp.y[0], LSM6DS_GYRO_SCALE_128);
    fi_ptr->ts_xyz2_int16_temp.z[0] = sensorScaleValue(fi_ptr->ts_xyz2_int16_temp.z[0], LSM6DS_GYRO_SCALE_128);

    /* Set timestamp of when interrupt PIO went (high or when read was schedule for SW driven timing) */
    fi_ptr->ts_xyz2_int16_temp.ts = SensorGetTimestamp(sensor_id);

    /* Return indicating value added to FIFO */
    return TRUE;
}


/* Called when interrupt PIO goes high, check PIO is still high, if so then request sensor read */
static void sensorLsm6dsIntHandler(uintptr_t context, uint32 mask)
{
    UNUSED(mask);

    Sensor sensor_id = context;
    sensor_lsm6ds_t *s = (sensor_lsm6ds_t *)SensorSubClassData(sensor_id);
    if (s)
    {
        uint32 ts = SensorGetPioTimestamp(s->interrupt_pio);
        SensorSetTimestamp(sensor_id, ts);
        DEBUG_LOG_V_VERBOSE("sensorLsm6dsIntHandler, PIO %u, level %u, ts %u", s->interrupt_pio, SensorGetInterruptLevel(s->interrupt_pio), ts);

        /* If interrupt PIO is high trigger immediate read of sensor */
        if (SensorGetInterruptLevel(s->interrupt_pio))
        {
            SensorRead(sensor_id, 0);
        }
    }
}


/* Table of callbacks */
static const sensor_functions_t sensorLsm6dsFunctions =
{
    .pre_power_on = NULL,
    .post_power_on = sensorLsm6dsOn,
    .pre_power_off = sensorLsm6dsOff,
    .post_power_off = NULL,
    .get_config = sensorLsm6dsGetConfig,
    .set_config = sensorLsm6dsSetConfig,
    .get_read_time = NULL,
    .read_setup = sensorLsm6dsReadSetup,
    .read_complete = sensorLsm6dsReadComplete,
};


/* Create instance of sensor driver */
Sensor SensorLsm6ds(Bus bus_id)
{
    /* Create sensor */
    Sensor sensor_id = SensorCreate(&sensorLsm6dsFunctions, sizeof(sensor_lsm6ds_t));
    sensor_lsm6ds_t *s = (sensor_lsm6ds_t *)SensorSubClassData(sensor_id);
    PanicNull(s);

    /* Set default interval.  Everything else is automatically 0 initialised */
    s->interval = 104;

    /* Attach sensor instance to specified bus and return sensor ID */
    BusAttachSensor(bus_id, sensor_id);
    return sensor_id;

}

#endif /* INCLUDE_SPATIAL_DATA */
