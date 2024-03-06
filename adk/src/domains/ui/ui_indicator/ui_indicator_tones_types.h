/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup ui_indicator
    \brief      

    @{
*/

#ifndef UI_TONES_TYPES_H
#define UI_TONES_TYPES_H

#include <csrtypes.h>
#include <kymera.h>

/*! \brief Audio tone configuration */
typedef struct
{
    const ringtone_note *tone;
    unsigned interruptible:1;                   /*!< Determines whether the audio tone can be interrupted during playback */
    unsigned queueable:1;                       /*!< Determines whether the audio tone can be queued prior to playback */
    unsigned not_repeatable_within_timeout:1;   /*!< Determines whether the audio tone can be repeated within the minimum timeout period */
    unsigned button_feedback:1;                 /*!< Determines that the tone is for button press feedback to the user */
    unsigned is_loud:1;                         /*!< Determines if the tone is a "loud" tone, for playing audible alerts when not in-ear */
} ui_tone_data_t;

#endif // UI_TONES_TYPES_H

/*! @} */