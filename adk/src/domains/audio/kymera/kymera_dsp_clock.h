/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera private header related to configuring the DSP clock/power settings
*/

#ifndef KYMERA_DSP_CLOCK_H_
#define KYMERA_DSP_CLOCK_H_

#include <audio_clock.h>

#define DEFAULT_VERY_LOW_POWER_CLK_SPEED_MHZ (16)
#define BOOSTED_VERY_LOW_POWER_CLK_SPEED_MHZ (32)
#define DEFAULT_LOW_POWER_CLK_SPEED_MHZ (32)
#define BOOSTED_LOW_POWER_CLK_SPEED_MHZ (45)

typedef struct
{
    bool(*get_dsp_clock_config)(audio_dsp_clock_configuration * config, audio_power_save_mode * mode);
} kymera_dsp_clock_user_if_t;

/*! \brief Boost the DSP clock to the maximum speed available.

    \note Changing the clock with chains already started may cause audible
    glitches if using I2S output.
*/
void appKymeraBoostDspClockToMax(void);

/*! \brief Configure power mode and clock frequencies of the DSP for the lowest
           power consumption possible based on the current state / codec.

   \note Calling this function with chains already started may cause audible
   glitches if using I2S output.
*/
void appKymeraConfigureDspPowerMode(void);

/*! \brief Register a user of the Kymera DSP clock module.
           A user will be given a say whenever the DSP clock is set.
           The Kymera DSP clock module will ensure the highest DSP clock value is set.
    \param user_if The DSP clock user's registered interface.
*/
void appKymeraRegisterDspClockUser(const kymera_dsp_clock_user_if_t * const user_if);

/*! \brief Resets the current DSP clock configuration.
 */
void appKymeraResetCurrentDspClockConfig(void);


#endif /* KYMERA_DSP_CLOCK_H_ */
