/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/
#include "mel.h"
#include "math_lib.h"
#include <string.h>
#include <audio_log/audio_log.h>


/**
 * \brief Function for mel filters initialization.
 * \param Pointer to algorithms config data required to create its context
 * \return Pointer to private structure of this instance
 */
void ml_mel_create(tEAIMelStruct ** mel_data_ptr, int sampling_rate, int fft_size, int n_spectral_channels)
{  
    tEAIMelStruct * mel_data;
    // allocate memory for storing configuration data
    *mel_data_ptr = xzpmalloc(sizeof(tEAIMelStruct));
    mel_data = *mel_data_ptr;
    if (mel_data) {
        // allocate memory for output buffer
        int * output_buffer = xzppmalloc(sizeof(sat fract) * n_spectral_channels, MALLOC_PREFERENCE_DM1);
        if (output_buffer) {
            // only if output buffer allocation is successful memcpy is useful
            // TODO: use memwcpy
            // memcpy(config, config_src, sizeof(tEAIMelStruct) + additional_memory_required);
            mel_data->sampling_rate = sampling_rate;
            mel_data->fft_size = fft_size;
            mel_data->n_spectral_channels = n_spectral_channels;
            mel_data->output_buffer = output_buffer;
        }
        else {
            // error, we have failed to allocate memory for output buffer. release private memory and return NULL
             L2_DBG_MSG("ML_MEL: cannot create ouput buffer - mel fails"); 
            pfree(mel_data);
            mel_data = NULL;
        }
    }
}

/**
 * \brief Function for mel filters shutdown and release memory.
 * \param Pointer to algorithms config data required to delete its context
 * \return void
 */
void ml_mel_destroy(tEAIMelStruct * mel_data)
{
    if (mel_data) {
        // if we ever allocated any output buffer, release
        if (mel_data->output_buffer) {
            pfree(mel_data->output_buffer);
        }
        pfree(mel_data);
    }
}

// optimised, actual values to be taken from the app
// The following table represents 1/ ( delta * 2 ** 10)
#pragma datasection const
accum mel_const_values[17] = {0.42393624881414604k, 0.4043279880050309k, 0.3856266413811983k, 0.36779028648665607k,
    0.3507789149355481k, 0.3345543688463478k, 0.3190802552506425k, 0.3043218644607654k,
    0.2902460922131593k, 0.2768213654128387k, 0.2640175713123824k, 0.2518059899666115k,
    0.2401592298114281k, 0.22905116622231933k, 0.2184568829147213k, 0.20835261605476993k, 0.19871570095510574k};

static void apply_mel(tEAIMelStruct * mel_data, signed *algo_input) {
    // calculate and store in mel private data
    sat fract mel_const1 = 0.9537470885552466;
    sat fract mel_const2 = 0.043750000186264515;
    sat fract mel_const3 = 0.003750000149011612;

    sat fract left = mel_const3;
    sat fract middle = (mel_const2 + left) / mel_const1 - mel_const2;
    memset(mel_data->output_buffer, 0, mel_data->n_spectral_channels << 2);

    int signdet = __builtin_signdet(mel_data->fft_size) + 1;
    unsigned mask = (1 << signdet) -1;
    sat fract * out_buffer_ptr = (sat fract *)mel_data->output_buffer;
    sat fract * algo_input_sf = (sat fract *)algo_input;

    for (int i=0; i < mel_data->n_spectral_channels; i++) {
//      right: Q131Scalar = (self.mel_const2 + middle) / self.mel_const1 - self.mel_const2
        sat fract right = (mel_const2 + middle) / mel_const1 - mel_const2;
        sat fract trial;
//            idx_left: int = left.ceil(self.fft_size)
        trial = left >> signdet;
        int idx_left =  *(int *)&trial;
        if (__builtin_and_fract(left, mask) > 0r) {
            idx_left++;
        }
//            idx_middle: int = middle.ceil(self.fft_size)
        trial = middle >> signdet;
        int idx_middle =  *(int *)&trial;
        if (__builtin_and_fract(middle, mask) > 0r) {
            idx_middle++;
        }
//            idx_right: int = right.ceil(self.fft_size)
        trial = right >> signdet;
        int idx_right =  *(int *)&trial;
        if (__builtin_and_fract(right, mask) > 0r) {
            idx_right++;
        }

//        delta_left = middle - left
//        delta_right = right - middle

        int j;
        accum out_buffer_ptr_val = 0;
//            for m in range(idx_left, idx_middle):
        for (j=idx_left; j < idx_middle; j++ ) {
//                mid = Q131Scalar.from_int32_ratio(m, self.fft_size)
            int j_mid = j << signdet;
            sat fract mid = *(sat fract *)&j_mid;
//                self.working_buffer[n] += self.working_buffer[m] * (mid - left) / delta_left
            out_buffer_ptr_val += algo_input_sf[j] * ((mid - left) * mel_const_values[i]);
        }
//            for m in range(idx_middle, idx_right):
        for (j=idx_middle; j < idx_right; j++) {
//                mid = Q131Scalar.from_int32_ratio(m, self.fft_size)
            int j_mid = j << signdet;
            sat fract mid = *(sat fract *)&j_mid;
//                self.working_buffer[n] += self.working_buffer[m] * (right - mid) / delta_right
            out_buffer_ptr_val += algo_input_sf[j] * ((right - mid) * mel_const_values[i+1]);
        }
        out_buffer_ptr[i] = (sat fract)(out_buffer_ptr_val << 10);
        left = middle;
        middle = right;
    }
}

/**
 * \brief Function for applying mel filters to te input.
 * \param Pointer to algorithms config data required to execute its context
 * \param Pointer to input data
 * \param input_size
 * \param output
 * \return void
 */
void ml_mel_update(tEAIMelStruct * mel_data, signed *algo_input, unsigned input_size, ALGO_OUTPUT_INFO * output)
{
    if (input_size == mel_data->fft_size)
    {  
        apply_mel(mel_data, algo_input);
        output->size = mel_data->n_spectral_channels;
        output->output_data = mel_data->output_buffer;
    }
}
