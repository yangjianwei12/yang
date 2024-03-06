/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

#include "taper.h"
#include "math_lib.h"
#include <audio_log/audio_log.h>

#define TAPER_SIZE_38 (38)

#pragma datasection const
accum taper_init_data[] = {
0.0,	0.001801189,	0.007191809,
0.016133082,	0.028561212,	0.044385262,
0.063492872,	0.085744656,	0.110981873,
0.139021701,	0.169661253,	0.202682689,
0.237845229,	0.274898093,	0.313571631,
0.353588527,	0.394659871,	0.436489,
0.47877875,	0.521218077,	0.563507531,
0.605337232,	0.64641124,	0.686426131,
0.725100221,	0.7621509,	0.797315524,
0.830336796,	0.860977755,	0.88901669,
0.914252107,	0.936504209,	0.955612451,
0.971436488,	0.983864599,	0.992807019,
0.998196462,	0.999996185,
};


void ml_taper_create(tEAITaper ** taper_data_ptr, int taper_strength_pc, int fft_chunk_size)
{
    tEAITaper * taper_data;
    *taper_data_ptr = xzpmalloc(sizeof(tEAITaper));
    taper_data = *taper_data_ptr;
    if (taper_data) {
         taper_data->taper_strength_pc = taper_strength_pc;
         taper_data->chunk_size = (int)(fft_chunk_size * taper_data->taper_strength_pc * 0.005);

        // taper_left = sin(Q131Array(np.arange(self.fft_taper_size) / (2 * (self.fft_taper_size - 1))))
        taper_data->taper_left = xzpmalloc(taper_data->chunk_size * sizeof(sat fract));
        if (taper_data->taper_left) {
            if (taper_data->chunk_size == TAPER_SIZE_38) {
                sat fract * taper_left_ptr = taper_data->taper_left;
                accum * init_data_ptr = taper_init_data;
                for (int i=0; i < TAPER_SIZE_38; i++){
                    *taper_left_ptr++ = (sat fract)*init_data_ptr++;
                }
            }
            else {
                sat fract * taper_left_ptr = taper_data->taper_left;
                int N = taper_data->chunk_size;
                int N_MINUS_1 = (taper_data->chunk_size -1) << 16;
                sat fract N_BY_1 = 0.0000152587890625r / *((sat fract*)&N_MINUS_1); // 0.0000152587890625 = 1 / 2**16
                sat fract val_in_degrees = 0r;
                for (int i=0; i < N; i++){
                    *taper_left_ptr++ = ml_sin(val_in_degrees);
                    val_in_degrees += N_BY_1;
                }
                taper_left_ptr = taper_data->taper_left;
                for (int i=0; i < N; i++){
                    sat fract taper_value = *taper_left_ptr;
                    *taper_left_ptr++ = taper_value * taper_value;
                }
            }
            taper_data->taper_right_offset = fft_chunk_size - taper_data->chunk_size;
            L2_DBG_MSG("ML_TAPER: taper data created"); 
        }
        else {
             L2_DBG_MSG("ML_TAPER: cannot create taper data left - taper fails"); 
            pfree(taper_data);
            taper_data = NULL;
        }
    } 
    else {
        L2_DBG_MSG("ML_TAPER: cannot create taper data - taper fails"); 
    }
}

void ml_taper_destroy(tEAITaper *taper_data)
{
    if (taper_data) {
        if (taper_data->taper_left) {
            pfree(taper_data->taper_left);
        }
        pfree(taper_data);
    }
}

/**
 * @brief ml_taper_update Applies taper to the given input, Writes to output, can be in place 
 * @param taper_data Pointer to taper processing data
 * 
 */
void ml_taper_update(tEAITaper *taper_data, signed *algo_input, signed *algo_output)
{
    
    sat fract * input_data_ptr = (sat fract *)algo_input;
    sat fract * output_data_ptr = (sat fract *)algo_output;
    sat fract * taper_values = taper_data->taper_left;
    for(int i=0; i < taper_data->chunk_size; ++i) {
        *output_data_ptr++ = *taper_values++ * *input_data_ptr++;
    }
    taper_values--;
    output_data_ptr = (sat fract *)(algo_output + taper_data->taper_right_offset);
    input_data_ptr = (sat fract *)(algo_input + taper_data->taper_right_offset);
    for(int i=0; i < taper_data->chunk_size; ++i) {
        *output_data_ptr++ = *taper_values-- * *input_data_ptr++;
    }
    /* copy the rest of the values if not in place */
    if(algo_input != algo_output)
    {
        for(unsigned i=taper_data->chunk_size; i<taper_data->taper_right_offset; i++)
        {
            algo_output[i] = algo_input[i];
        }
    }

}
