/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       lis25ba_internal.h
\brief      The parameters / enums define the message interface used to
            control the chip-specific I2C interface of the LIS25BA accelerometer.
*/

#ifndef _LIS25BA_INTERNAL_H_
#define _LIS25BA_INTERNAL_H_

#ifdef INCLUDE_LIS25BA_ACCELEROMETER

#include <library.h>
#include <stream.h>
#include <bitserial_api.h>

#define LIS25BA_WHO_AM_I_RESPONSE (0x20)

/*! The high level configuration for the I2C communication with the sensor */
struct __lis25ba_accelerometer
{
    bitserial_config i2c_config;

    /*! The PIOs used to control/communicate with the sensor */
    struct
    {
        uint8 i2c_scl;
        uint8 i2c_sda;
    } pios;

    bitserial_block_index block;
};

typedef struct __lis25ba_accelerometer lis25ba_accelerometer;

/*! Register definitions for the I2C control interface of the LIS25BA device */
typedef enum
{
    lis25ba_addr_test_reg      = (0x0B),
    lis25ba_addr_who_am_i      = (0x0F),
    lis25ba_addr_tdm_cmax_high = (0x24),
    lis25ba_addr_tdm_cmax_low  = (0x25),
    lis25ba_addr_ctrl_reg      = (0x26),
    lis25ba_addr_tdm_ctrl_reg  = (0x2E),
    lis25ba_addr_axes_ctrl_reg = (0x2F),
} lis25ba_addresses;

/*! @{ */
/*! I2C address bytes (also contain the read/write flag as bit 7) */
#define ADDR_WRITE 0x30
#define ADDR_READ  0x31
/*! @} */

/*! @{ */
/*! I2C specific functions in various registers */
#define CTRL_NORM_MODE  (0)
#define CTRL_PD_MODE (1<<5)
#define TDM_PD       (1<<7)
#define TDM_DELAYED  (1<<6)
#define TDM_DVALID   (1<<5)
#define TDM_MAPPING  (1<<4)
#define TDM_WCLK     (0x03<<1)
#define AXESZ_EN     (1<<7)
#define AXESY_EN     (1<<6)
#define AXESX_EN     (1<<5)
#define ODR_AUTO_EN  (1<<0)
/*! @} */

#endif /* INCLUDE_LIS25BA_ACCELEROMETER */

#endif /* _LIS25BA_INTERNAL_H_ */
