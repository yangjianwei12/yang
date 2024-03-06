/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

/**
 * Header file for "ml_spectral" library containing the spectrum libraries
 * Comments show the syntax to call the routine
 */
#if !defined(EAI_TAPER_H)
#define EAI_TAPER_H

#include <stdfix.h>
#include "preproc_common.h"

/* PUBLIC TYPES DEFINITIONS *************************************************/



typedef struct
{
    int chunk_size;
    int taper_strength_pc;
    sat fract *taper_left;
    int taper_right_offset;
} tEAITaper;
/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

void ml_taper_create(tEAITaper **taper_data, int taper_strength_pc, int fft_chunk_size);
void ml_taper_update(tEAITaper *taper_data, signed *algo_input, signed *algo_output);
void ml_taper_destroy(tEAITaper *taper_data);

#endif
