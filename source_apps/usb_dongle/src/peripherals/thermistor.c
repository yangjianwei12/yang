/*!
\copyright  Copyright (c) 2018-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       thermistor.c
\brief      Support for thermistor temperature sensing, implementing the API defined
            in temperature_sensor.h.
*/

#ifdef INCLUDE_TEMPERATURE
#ifdef HAVE_THERMISTOR
#include <panic.h>
#include <pio.h>
#include <hydra_macros.h>

#include "adk_log.h"
#include "temperature_sensor.h"
#include "thermistor.h"
#include "thermistor_config.h"

#include CSR_EXPAND_AND_STRINGIFY(THERMISTOR_DATA_FILE)

/*! \brief Returns the PIO bank number.
    \param pio The pio. */
#define PIO2BANK(pio) ((uint16)((pio) / 32))
/*! \brief Returns the PIO bit position mask within a bank.
    \param pio The pio. */
#define PIO2MASK(pio) (1UL << ((pio) % 32))

/*! Clamp a temperature within the limits specified in the THERMISTOR_DATA_FILE */
static int8 temperatureClamp(int8 temperature)
{
    temperature = MIN(temperature, THERMISTOR_TEMP_MAX);
    temperature = MAX(temperature, THERMISTOR_TEMP_MIN);
    return temperature;
}

uint16 appThermistorDegreesCelsiusToMillivolts(int8 temperature)
{
    temperature = temperatureClamp(temperature);
    return thermistor_voltage_lookup_table[temperature - THERMISTOR_TEMP_MIN];
}

/*! \brief Enable/disable the thermistor 'on' PIO */
void appThermistorPIOEnable(bool enable)
{
    uint8 on = appConfigThermistor()->on;
    if (on != THERMISTOR_PIO_UNUSED)
    {
        uint16 bank = PIO2BANK(on);
        uint32 mask = PIO2MASK(on);
        PanicNotZero(PioSet32Bank(bank, mask, enable ? mask : 0));
    }
}

void appTemperatureSensorRequestMeasurement(Task task)
{
    appThermistorPIOEnable(TRUE);
    AdcReadRequest(task, adcsel_vref_hq_buff, 0, 0);
    AdcReadRequest(task, appConfigThermistor()->adc, 0, 0);
}

void appTemperatureSensorInit(void)
{
    uint8 on = appConfigThermistor()->on;

    if (on != THERMISTOR_PIO_UNUSED)
    {
        /* Setup 'on' PIO as output (driving low) */
        uint16 bank = PIO2BANK(on);
        uint32 mask = PIO2MASK(on);
        PanicNotZero(PioSetMapPins32Bank(bank, mask, mask));
        PanicNotZero(PioSetDir32Bank(bank, mask, mask));
        PanicNotZero(PioSet32Bank(bank, mask, 0));
    }
}

vm_adc_source_type appTemperatureSensorAdcSource(void)
{
    return appConfigThermistor()->adc;
}

#endif /* HAVE_THERMISTOR */
#endif /* INCLUDE_TEMPERATURE */
