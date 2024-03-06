/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       lis25ba.c
\brief      Support for PCM audio input, implementing the 'Device' API defined
            in audio_pcm_common.h, with chip specific I2C commands to configure
            the LIS25BA accelerometer.
*/

#ifdef INCLUDE_LIS25BA_ACCELEROMETER

#include "lis25ba_internal.h"
#include "lis25ba.h"

#include <pio_common.h>
#include <stdlib.h>
#include <panic.h>
#include <string.h>
#include <i2c.h>
#include <pio_common.h>
#include <stdio.h>
#include <logging.h>

/*! Default PCM configuration data for LIS25BA chip
 *  The values are set in function AudioPcmCommonConfigureSource()
 */
const pcm_config_t device_pcm_config =
{
    .sample_rate = 16000,       // lis25ba always runs with 16k. If general mic rate shall be used, set to 0
    .master_mode = 1,           // accelorometer should always be the slave
    .sample_size = 16,
    .slot_count = 3,
    .short_sync_enable = 1,
    .lsb_first_enable = 0,
    .sample_rising_edge_enable = 1,
    .sample_format = 1,         // 16 bits in 16 cycle slot duration
    .master_clock_source = CLK_SOURCE_TYPE_MCLK,
    .master_mclk_mult = 768,
};

/*! Default I2C configuration data for LIS25BA chip */
const lis25ba_accelerometer lis25ba_Config =
{
    .i2c_config =
    {
        .mode = BITSERIAL_MODE_I2C_MASTER,
        .clock_frequency_khz = 400,
        .timeout_ms = 1000,
        .u.i2c_cfg.flags = 0x00,
        .u.i2c_cfg.i2c_address = 0x19,
    },
    .pios =
    {
        /* These PIOs need to be project defines:
         * e.g. RDP_PIO_I2C_SCL_1=34 and RDP_PIO_I2C_SDA_1=37
         * Device is connected to 2nd I2C bus
         */
        .i2c_scl = RDP_PIO_I2C_SCL_1,
        .i2c_sda = RDP_PIO_I2C_SDA_1,
    },
    .block = BITSERIAL_BLOCK_1,
};

/*! \brief Write a single register */
static bitserial_result lis25ba_WriteRegister(unsigned reg_address, unsigned value)
{
    bitserial_handle i2c_handle;
    bitserial_result result;
    uint8 tx_data[] = { (uint8)reg_address, (uint8)value };

    i2c_handle = BitserialOpen(lis25ba_Config.block, &lis25ba_Config.i2c_config);
    PanicFalse(i2c_handle != BITSERIAL_HANDLE_ERROR);

    result = BitserialWrite(i2c_handle, BITSERIAL_NO_MSG, tx_data, sizeof(tx_data), BITSERIAL_FLAG_BLOCK);

    if (result == BITSERIAL_RESULT_SUCCESS)
    {
        DEBUG_LOG_VERBOSE("lis25ba_WriteRegister: adresss 0x%02X has been set to 0x%02X", reg_address, value);
    }
    else
    {
        DEBUG_LOG_WARN("lis25ba_WriteRegister: Failed to write to adresss 0x%02X", reg_address);
    }

    BitserialClose(i2c_handle);
    return result;
}

/*! \brief Read a single register */
static uint8 lis25ba_ReadRegister(unsigned reg_address)
{
    bitserial_handle i2c_handle;
    bitserial_result result;
    const uint8 tx_data[] = { (uint8)reg_address };
    uint8 rx_data[1];

    i2c_handle = BitserialOpen(lis25ba_Config.block, &lis25ba_Config.i2c_config);
    PanicFalse(i2c_handle != BITSERIAL_HANDLE_ERROR);


    /* First write the register address to be read */
    result = BitserialWrite(i2c_handle, BITSERIAL_NO_MSG, tx_data, sizeof(tx_data), BITSERIAL_FLAG_BLOCK);
    if (result == BITSERIAL_RESULT_SUCCESS)
    {
        /* Now read the actual data in the register */
        result = BitserialRead(i2c_handle, BITSERIAL_NO_MSG, rx_data, sizeof(rx_data), BITSERIAL_FLAG_BLOCK);
    }

    if(result == BITSERIAL_RESULT_SUCCESS)
    {
        DEBUG_LOG_VERBOSE("lis25ba_ReadRegister: adresss 0x%02X has value 0x%02X", reg_address, rx_data[0]);
    }
    else
    {
        DEBUG_LOG_WARN("lis25ba_ReadRegister: Failed to read from adresss 0x%02X", reg_address);
    }

    BitserialClose(i2c_handle);
    return rx_data[0];
}

const pcm_config_t *Peripherals_Lis25baGetPcmInterfaceSettings(void)
{
    return &device_pcm_config;
}
void Peripherals_Lis25baInitializeI2cInterface(void)
{
    uint8 rx_data;
    DEBUG_LOG_VERBOSE("Peripherals_Lis25baInitializeI2cInterface: for block %d addr 0x%02X", lis25ba_Config.block, lis25ba_Config.i2c_config.u.i2c_cfg.i2c_address);

    unsigned bank = PioCommonPioBank(lis25ba_Config.pios.i2c_scl);
    unsigned mask = PioCommonPioMask(lis25ba_Config.pios.i2c_scl);
    unsigned mask2 = PioCommonPioMask(lis25ba_Config.pios.i2c_sda);
    mask = mask | mask2;
    PioSetMapPins32Bank(bank, mask, 0);

    switch(lis25ba_Config.block)
    {
        case BITSERIAL_BLOCK_0:
            PioSetFunction(lis25ba_Config.pios.i2c_scl, BITSERIAL_0_CLOCK_OUT);
            PioSetFunction(lis25ba_Config.pios.i2c_scl, BITSERIAL_0_CLOCK_IN);
            PioSetFunction(lis25ba_Config.pios.i2c_sda, BITSERIAL_0_DATA_OUT);
            PioSetFunction(lis25ba_Config.pios.i2c_sda, BITSERIAL_0_DATA_IN);
        break;
        case BITSERIAL_BLOCK_1:
            PioSetFunction(lis25ba_Config.pios.i2c_scl, BITSERIAL_1_CLOCK_OUT);
            PioSetFunction(lis25ba_Config.pios.i2c_scl, BITSERIAL_1_CLOCK_IN);
            PioSetFunction(lis25ba_Config.pios.i2c_sda, BITSERIAL_1_DATA_OUT);
            PioSetFunction(lis25ba_Config.pios.i2c_sda, BITSERIAL_1_DATA_IN);
        break;
        default:
            DEBUG_LOG_ERROR("Peripherals_Lis25baInitializeI2cInterface: Unknown block selected");
            Panic();
        break;
    }

    PanicNotZero(PioSetDir32Bank(bank, mask, 0));
    PanicNotZero(PioSet32Bank(bank, mask, mask));
    PanicNotZero(PioSetStrongBias32Bank(bank, mask, mask));

    rx_data = lis25ba_ReadRegister(lis25ba_addr_who_am_i);
    if(rx_data != LIS25BA_WHO_AM_I_RESPONSE)
    {
        DEBUG_LOG_ERROR("Peripherals_Lis25baInitializeI2cInterface: WHO_AM_I=0x%02X expected=0x%02X", rx_data, LIS25BA_WHO_AM_I_RESPONSE);
        Panic();
    }
}
void Peripherals_Lis25baEnableDevice(void)
{
    // Set TDM_CTR_REG as follows
    // b0000_0010 (Bit mapping as shown below)
    //[ TDM_pd : delayed : data_valid : mapping : 0 : WCLK_fq1 : WCLK_fq0 : 0 ]
    lis25ba_WriteRegister(lis25ba_addr_tdm_ctrl_reg, 0x02);
    // Set CTRL_REG to zero in order to power up the accelorometer in Normal mode
    lis25ba_WriteRegister(lis25ba_addr_ctrl_reg, 0x00);
}
void Peripherals_Lis25baDisableDevice(void)
{
    // Set CTRL_REG to 0x20 in order to power down the accelorometer
    lis25ba_WriteRegister(lis25ba_addr_ctrl_reg, 0x20);
}

#endif /* INCLUDE_LIS25BA_ACCELEROMETER */
