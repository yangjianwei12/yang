/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

/**
 * Header file for "ml_normalization" library containing the normalization libraries
 * Comments show the syntax to call the routine
 */
#if !defined(EAI_NORMALIZATION_H)
#define EAI_NORMALIZATION_H

#include <stdfix.h>
#include "preproc_common.h"

#define INITIAL_BUFFER_SIZE (2)

/* PUBLIC TYPES DEFINITIONS *************************************************/

/*! \enum normalization type
    \brief NONE -> No normalization desired.
           FIXED -> Use a fixed number of samples. Can be greater than the incoming buffer size, zero padded.
           ROLLING -> First order IIR
 */
typedef enum {
    NONE = 0,
    FIXED = 1,  // fixed number of samples
    ROLLING = 2  // first order IIR, currently not supported
} raw_audio_norm_type;

/*typedef struct
{
    raw_audio_norm_type norm_type;
    int buffer_size;  // data across which normalization is required
    int packet_size;  // incoming packet size per invocation
    sat fract alpha;
} tEAINorm;
*/
typedef struct
{
    raw_audio_norm_type norm_type;
    bool is_process_initial;
    sat fract initial_buffer[INITIAL_BUFFER_SIZE << 1];
    int fixed_size;
    int packet_size;
    int packet_count;
    sat fract mean_std[2];
    sat fract * fixed_x_hat;
    sat fract * fixed_x2_hat;
    sat fract alpha;
} tEAINorm;




/*** PUBLIC FUNCTION PROTOTYPES ***********************************************/

void ml_norm_create(tEAINorm ** norm_data_ptr, int buffer_size, int packet_size, sat fract alpha, raw_audio_norm_type norm_type);
void ml_norm_update(tEAINorm * norm_data, signed *algo_input, ALGO_OUTPUT_INFO *algo_output, int input_size);
void ml_norm_destroy(tEAINorm * norm_data);
#endif
