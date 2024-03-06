/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    ml_example
\brief      Setup of the audio hardware
 
*/

#ifndef ML_AUDIO_SETUP_H_
#define ML_AUDIO_SETUP_H_

#include <source.h>

/*@{*/

/*! \brief Non ML audio setup

    It configures a microphone.

    \param mic_gain Gain of the microphone 0 to CODEC_INPUT_GAIN_RANGE

    \return Source from the microphone input

*/
Source Ml_SetupAudio(int mic_gain);

/*@}*/

#endif /* ML_AUDIO_SETUP_H_ */
