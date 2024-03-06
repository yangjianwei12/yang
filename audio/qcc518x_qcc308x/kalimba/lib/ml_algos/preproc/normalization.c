/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

#include "normalization.h"
#include "math_lib.h"
#include <string.h>
#include <audio_log/audio_log.h>



static sat fract ml_norm_fixed_mean(const sat fract * buffer, int size, sat fract by_size)
{
    accum tmp_mean = 0;
#pragma loop minitercount(1)
    for (int i=0; i < size; i++) {
        tmp_mean += *buffer++;
    }
    tmp_mean = tmp_mean * by_size;
    return (sat fract)tmp_mean;
}

static sat fract ml_norm_fixed_std(sat fract mean, const sat fract * restrict buffer, int size, sat fract by_size)
{
    accum tmp_std = 0;
#pragma loop minitercount(1)
    for (int i=0; i < size; i++) {
        sat fract value = *buffer++ - mean;
        tmp_std += (value * value);
    }
    tmp_std *= by_size;
    int scale_bits_available = __builtin_signdet_accum(tmp_std);
    scale_bits_available &= 0x00FE;
    tmp_std <<= scale_bits_available;
    sat fract sq_temp_std = (sat fract)(tmp_std);
    sq_temp_std = ml_sqrt(sq_temp_std);
    sq_temp_std >>= (scale_bits_available >> 1);
    return sq_temp_std;
}

/**
 * Calculate only for the given buffer
 */
static inline void ml_norm_fixed(tEAINorm * restrict norm_data, const sat fract * restrict buffer, int size, sat fract by_size)
{
    norm_data->mean_std[0] = ml_norm_fixed_mean(buffer, size, by_size);
    norm_data->mean_std[1] = ml_norm_fixed_std(norm_data->mean_std[0], buffer, size, by_size);
}

// TODO: Use circular buffer
static inline void array_roll(sat fract * dest, int count, sat fract value)
{
    memcpy(dest, dest + 1, (count-1)*sizeof(sat fract));
    dest[count-1] = value;
}

/**
 * Calculate long term lean with the given alpha
 */
static inline void direct_update(tEAINorm * norm_data, const sat fract * restrict buffer, int size, sat fract by_size)
{
    sat fract old_std = norm_data->mean_std[1];
    sat fract alpha = norm_data->alpha;
    sat fract one_alpha = SFRACT_MAX - alpha;

    sat fract mean = norm_data->mean_std[0];
    sat fract buffer_mean = ml_norm_fixed_mean(buffer, size, by_size);

    // _x2 = std*std + mean*mean
    sat fract _x2 = old_std* old_std + mean * mean;

    // mean = alpha * buffer.mean() + one_alpha * mean
    mean = alpha * buffer_mean + one_alpha * mean;

    // _x2 = alpha * (buffer ** 2).mean() + one_alpha * _x2
    accum buffer_square_sum = 0;
    for (int i=0; i < size; ++i) {
        sat fract current_buffer_val = *buffer++;
        buffer_square_sum += current_buffer_val * current_buffer_val;
    }
    accum buffer_square_mean = buffer_square_sum * by_size;
    _x2 = (sat fract)(alpha * buffer_square_mean + one_alpha * _x2);

    norm_data->mean_std[0] = mean;

    // std = np.sqrt((_x2 - (mean * mean)))
    sat fract mean_sq_diff = _x2 - mean * mean;
    int var_mean_diff_abs = __builtin_abs(*(int *)&mean_sq_diff);
    norm_data->mean_std[1] = ml_sqrt(*(sat fract*)&var_mean_diff_abs);
}

static void ml_normalization_update(tEAINorm * norm_data, const sat fract * restrict buffer, int size)
{
    // TODO: The by_size value needs to be calculated during init
    int numer1 = 0x1 ;
    sat fract * denom = (sat fract *)&size;
    sat fract * numer = (sat fract *)&numer1;
    sat fract by_size = *numer / *denom;
    if (norm_data->norm_type == FIXED) {
        if (norm_data->fixed_size <= size) {
            ml_norm_fixed(norm_data, buffer, size, by_size);
        }
        else {
            tEAINorm temp_norm_data;
            ml_norm_fixed(&temp_norm_data, buffer, size, by_size);
            // Current `x2_hat`
            sat fract x2_hat = norm_data->mean_std[1] * norm_data->mean_std[1] + norm_data->mean_std[0] * norm_data->mean_std[0];
            // Update the mean
            norm_data->mean_std[0] += (temp_norm_data.mean_std[0] - norm_data->fixed_x_hat[0]) * norm_data->packet_count;
            // Update the std.
            sat fract x2_hat_new = temp_norm_data.mean_std[1] * temp_norm_data.mean_std[1];
            x2_hat += (x2_hat_new - norm_data->fixed_x2_hat[0]) * norm_data->packet_count;
            norm_data->mean_std[1] = ml_sqrt(x2_hat - norm_data->mean_std[0] * norm_data->mean_std[0]);
            // Update the fixed buffers
            array_roll(norm_data->fixed_x_hat, norm_data->packet_count, temp_norm_data.mean_std[0]);
            array_roll(norm_data->fixed_x2_hat, norm_data->packet_count, x2_hat_new);
        }
    }
    else if (norm_data->norm_type == ROLLING) {
        direct_update(norm_data, buffer, size, by_size);
    }
    return;
}

void ml_norm_update(tEAINorm * norm_data, signed *algo_input, ALGO_OUTPUT_INFO *algo_output, int input_size)
{
    if (norm_data->is_process_initial) {
        // if fixed, and fixed size equals packet size, call n times
        //           else call once
        // if rolling, just call n times
        int count_offset = input_size / norm_data->packet_size;
        if (norm_data->norm_type == FIXED) {
            if (norm_data->fixed_size == norm_data->packet_size) {
                if (INITIAL_BUFFER_SIZE >= count_offset) {
                    for (int i=0; i< count_offset; ++i) {
                        ml_normalization_update(norm_data, (sat fract *)(algo_input + i * norm_data->packet_size), norm_data->packet_size);
                        norm_data->initial_buffer[0+i*2] = norm_data->mean_std[0];
                        norm_data->initial_buffer[1+i*2] = norm_data->mean_std[1];
                    }
                    algo_output->size = count_offset;
                    algo_output->output_data = (signed *)norm_data->initial_buffer;
                }
                // else trigger error
            }
            else {
                ml_normalization_update(norm_data, (sat fract *)algo_input, input_size);
                algo_output->size = 2;
                algo_output->output_data = (signed *)norm_data->mean_std;
            }
        }
        else if (norm_data->norm_type == ROLLING) {
            for (int i=0; i< count_offset; ++i) {
                ml_normalization_update(norm_data, (sat fract *)(algo_input + i * norm_data->packet_size), norm_data->packet_size);
            }
            algo_output->size = 2;
            algo_output->output_data = (signed *)norm_data->mean_std;
        }
        norm_data->is_process_initial = 0;
    }
    else {
        ml_normalization_update(norm_data, (sat fract *)(algo_input + input_size - norm_data->packet_size), norm_data->packet_size);
        algo_output->size = 2;
        algo_output->output_data = (signed *)norm_data->mean_std;
    }
}

/**
 * Tries to allocate memory for x_hat and x2_hat, returns true if successful, else false
 */
static bool init_cache_pointers(tEAINorm * norm_data)
{
    if (norm_data->fixed_size % norm_data->packet_size) {
        // error, they should be perfect multiple
        return FALSE;
    }
    norm_data->packet_count = norm_data->fixed_size / norm_data->packet_size;
    norm_data->fixed_x_hat = xzpmalloc(norm_data->packet_count * sizeof(sat fract));
    if (!norm_data->fixed_x_hat) {
        return FALSE;
    }
    norm_data->fixed_x2_hat = xzpmalloc(norm_data->packet_count * sizeof(sat fract));
    if (!norm_data->fixed_x2_hat) {
        pfree(norm_data->fixed_x_hat);
        return FALSE;
    }
    return TRUE;
}

void ml_norm_create(tEAINorm ** norm_data_ptr, int buffer_size, int packet_size, sat fract alpha, raw_audio_norm_type norm_type)
{
    tEAINorm * norm_data;
    *norm_data_ptr = xzpmalloc(sizeof(tEAINorm));
    norm_data = *norm_data_ptr;

    if (norm_data) {
        norm_data->fixed_size = buffer_size;
        norm_data->packet_size = packet_size;
        norm_data->norm_type = norm_type;
        norm_data->mean_std[0] = 0r;
        norm_data->mean_std[1] = 0r;
        // if the incoming data is more than configured length, should we process it
        norm_data->is_process_initial = 1;
        if (norm_data->norm_type == FIXED) {
            // if the buffer for which norm is to be calculated is larger than the given size each call
            if (norm_data->fixed_size > norm_data->packet_size) {
                bool was_allocation_success = init_cache_pointers(norm_data);
                if (!was_allocation_success) {
                    pfree(norm_data);
                    norm_data = NULL;
                }
            }
        }
        else if (norm_data->norm_type == ROLLING) {
            norm_data->alpha = alpha;
        }
    }
    pfree(norm_data);
    norm_data = NULL;
}

void ml_norm_destroy(tEAINorm * norm_data)
{
        if (norm_data) {
            if (norm_data->fixed_x_hat) {
                // free the memory
                pfree(norm_data->fixed_x_hat);
            }
            if (norm_data->fixed_x2_hat) {
                // free the memory
                pfree(norm_data->fixed_x2_hat);
            }
        }
        pfree(norm_data);
        norm_data = NULL;
}
