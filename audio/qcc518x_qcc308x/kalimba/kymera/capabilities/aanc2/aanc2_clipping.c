/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  aanc_clipping.c
 * \ingroup aanc2
 *
 * AANC2 clipping library.
 */

#include "aanc2_clipping.h"

/******************************************************************************
Public Function Definitions
*/

bool aanc2_clipping_initialize(AANC2_CLIP_DETECT *p_clip,
                               tCbuffer *p_data,
                               unsigned threshold,
                               unsigned duration,
                               bool disabled)
{
    unsigned frame_duration;

    p_clip->p_data = p_data;        /* Setup internal data reference */
    p_clip->threshold = threshold;  /* Setup threshold */

    /* Setup frame duration in frames */
    frame_duration = duration * AANC2_FRAME_RATE;
    p_clip->duration = (uint16)(frame_duration >> AANC2_TIMER_PARAM_SHIFT);

    /* Reset counter and detection states */
    p_clip->counter = 0;
    p_clip->peak_value = 0;
    p_clip->frame_detect = FALSE;
    p_clip->disabled = disabled;
    p_clip->detected = FALSE;

    /* Override disabled flag if there's no input buffer */
    if (p_data == NULL)
    {
        p_clip->disabled = TRUE;
    }

    return TRUE;
}

/**
 * \brief  Process a clip detection.
 *
 * \param  p_clip  Pointer to the clip struct.
 *
 * This monitors the frame detection and allows a counter to hold a detection
 * for a given duration (frames).
 */
void aanc2_clipping_process_detection(AANC2_CLIP_DETECT *p_clip)
{
    /* Disabled resets the flag */
    if (p_clip->disabled)
    {
        p_clip->detected = FALSE;
    }
    else
    {
        /* Detection sets the flag and resets the counter */
        if (p_clip->frame_detect)
        {
            p_clip->counter = p_clip->duration;
            p_clip->detected = TRUE;
        }
        else
        {
            /* No detection decrements the counter until 0 */
            if (p_clip->counter > 0)
            {
                p_clip->counter--;
            }
            else
            {
                p_clip->detected = FALSE;
            }
        }
    }
}

