/*!
\copyright  Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle Voice Assistant related internal APIs

*/

#ifndef KYMERA_VA_H_
#define KYMERA_VA_H_

#include "kymera.h"
#include "va_audio_types.h"
#include <source.h>

/*! \brief To be called during components initialisation.
*/
void Kymera_VaInit(void);

/*! \brief Prepare capturing/encoding live mic data.
    \param params Parameters based on which the voice capture will be configured.
    \return True on success.
*/
bool Kymera_PrepareVaLiveCapture(const va_audio_voice_capture_params_t *params);

/*! \brief Start capturing/encoding live mic data (must prepare first).
    \param params Parameters based on which the voice capture will be configured.
    \return The output of the capture chain as a mapped Source or NULL if it fails.
*/
Source Kymera_StartVaLiveCapture(const va_audio_voice_capture_params_t *params);

/*! \brief Stop capturing/encoding mic data.
    \return True on success.
*/
bool Kymera_StopVaCapture(void);

/*! \brief Prepare Wake-Up-Word detection.
    \param wuw_detection_handler Task to receive the Wake-Up-Word detection message from audio.
    \param params Parameters based on which the Wake-Up-Word detection will be configured.
    \return True on success.
*/
bool Kymera_PrepareVaWuwDetection(Task wuw_detection_handler, const va_audio_wuw_detection_params_t *params);

/*! \brief Start Wake-Up-Word detection (must prepare first).
    \param wuw_detection_handler Task to receive the Wake-Up-Word detection message from audio.
    \param params Parameters based on which the Wake-Up-Word detection will be configured.
    \return True on success.
*/
bool Kymera_StartVaWuwDetection(Task wuw_detection_handler, const va_audio_wuw_detection_params_t *params);

/*! \brief Stop Wake-Up-Word detection.
    \return True on success.
*/
bool Kymera_StopVaWuwDetection(void);

/*! \brief Must immediately be called after a Wake-Up-Word detection message from audio.
    \return True on success.
 */
bool Kymera_VaWuwDetected(void);

/*! \brief Start capturing/encoding wuw and live mic data.
           Must be called as a result of a Wake-Up-Word detection message from audio.
    \param params Parameters based on which the voice capture will be configured.
    \return The output of the capture chain as a mapped Source.
*/
Source Kymera_StartVaWuwCapture(const va_audio_wuw_capture_params_t *params);

/*! \brief Must be called when WuW is detected but ignored (capture is not started).
    \return True on success.
 */
bool Kymera_IgnoreDetectedVaWuw(void);

/*! \brief Check if capture is active.
    \return True on success.
*/
bool Kymera_IsVaCaptureActive(void);

/*! \brief Check if Wake-Up-Word detection is active.
*/
bool Kymera_IsVaWuwDetectionActive(void);

/*! \brief Check if Voice Assistant is active.
*/
#ifdef INCLUDE_VOICE_UI
bool Kymera_IsVaActive(void);
#else
#define Kymera_IsVaActive() FALSE
#endif

/*! \brief Get the minimum clock required for the DSP to run the currently active VA chain
 *  \return The minimum clock setting for the DSP
*/
#ifdef INCLUDE_VOICE_UI
audio_dsp_clock_type Kymera_VaGetMinDspClock(void);
#else
#define Kymera_VaGetMinDspClock() (Panic(), (audio_dsp_clock_type) 0)
#endif

/*! \brief Get the minimum low power clock speed in MHz and the minimum very low power clock speed in MHz
 *         required to run the currently active VA chain
 *  \param min_lp_clk_speed_mhz The minimum low power mode clock speed for VA in MHz
 *  \param min_very_lp_clk_speed_mhz The minimum very low power mode clock speed for VA in MHz
*/
void Kymera_VaGetMinLpClockSpeedMhz(uint8 *min_lp_clk_speed_mhz, uint8 *min_very_lp_clk_speed_mhz);

/*! \brief Should always be called when updating the DSP clock to allow VA to update it instead
    \param active_mode The clock to use when WuW engine is not in full processing mode (or when WuW detection is not active)
    \param trigger_mode The clock to use when WuW engine is in full processing mode
    \return TRUE when VA set the clock, FALSE otherwise (in which case the clock should be updated via the framework as normal)
*/
#ifdef INCLUDE_VOICE_UI
bool Kymera_VaSetDspClock(audio_dsp_clock_type active_mode, audio_dsp_clock_type trigger_mode);
#else
#define Kymera_VaSetDspClock(x,y)   FALSE
#endif

#endif /* KYMERA_VA_H_ */
