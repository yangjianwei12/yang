/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

/**
 * Header file for "ml_spectral" library containing the spectrum libraries
 * Comments show the syntax to call the routine
 */
#if !defined(EAI_SPECTRAL_H)
#define EAI_SPECTRAL_H

#include <stdfix.h>
#include "preproc_common.h"
#include "mel.h"
#include "taper.h"

/* PUBLIC TYPES DEFINITIONS *************************************************/



typedef struct
{
    int sampling_rate;
    int fft_input_size;
    int fft_size;
    int n_spectral_channels;
    int spec_output;
    int stat_output;  // where we require to generate the stat output, -1 for not required
    int norm_output;  // where we require to generate the norm output, -1 for not required
    sat fract norm_alpha;
    unsigned normalization_required:1;
    tEAIMelStruct * mel_data;
    tEAITaper * taper_data;
    accum * normalization_buffer_mean;
    accum * normalization_buffer_sd;
    sat fract * internal_buffer_r;
    sat fract * internal_buffer_i;
    sat fract * internal_buffer_scratch;
    sat fract sample_mean;
    unsigned start_spec_idx;
    unsigned end_spec_idx;
    unsigned is_next_run;
    sat fract prev_mean;
    accum prev_var;
    sat fract stat_out_data;
    sat fract norm_out_data[2];
    /* spec output space is provided by mel */
     sat fract scale_factor;
} tEAISpectrum;
/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

void ml_spectrum_create(tEAISpectrum ** spectrum_data, int sampling_rate, int fft_input_size,
                                        int fft_size, int n_spectral_channels, int spec_output, int stat_output, int norm_output,unsigned normalization_required, sat fract norm_alpha);
void ml_spectrum_update(tEAISpectrum * spectrum_data, signed *algo_input, ALGO_OUTPUT_INFO *algo_output, unsigned input_size);
void ml_spectrum_destroy(tEAISpectrum * spectrum_data);

#ifdef NO_ADK
#define STATIC
/* Utility functions */
void spectral_do_fft(tEAISpectrum * eaiSpec, bool is_ortho);
void apply_scale(tEAISpectrum * spectrum_data, ALGO_OUTPUT_INFO *algo_output);
void stats_and_norm(tEAISpectrum * eaiSpec, sat fract * algo_input, ALGO_OUTPUT_INFO *algo_output);
void compute_spectrum(tEAISpectrum * eaiSpec, sat fract * restrict real, sat fract * restrict imag, sat fract * scratch, ALGO_OUTPUT_INFO *algo_output);
void normalise_spectral_features(tEAISpectrum * eaiSpec, ALGO_OUTPUT_INFO *algo_output);
#else
#define STATIC static
#endif

#endif
