/*!
\copyright  Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module with kymera VA common definitions

*/

#ifdef INCLUDE_VOICE_UI
#include "kymera_va_common.h"

static unsigned va_sample_rate;

void Kymera_SetVaSampleRate(unsigned sample_rate)
{
    va_sample_rate = sample_rate;
}

unsigned Kymera_GetVaSampleRate(void)
{
    return va_sample_rate;
}

#endif /*#ifdef INCLUDE_VOICE_UI */
