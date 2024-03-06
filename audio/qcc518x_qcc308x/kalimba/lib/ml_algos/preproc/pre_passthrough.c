/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

#include "math_lib.h"
#include <string.h>
#include <audio_log/audio_log.h>
#include "pre_passthrough.h"


/**
 * \brief Function for passthrough creation.
 * \param Pointer to algorithms config data required to create its context
 * \return Pointer to private structure of this instance
 */
void pre_passthrough_create(t_PrePassthrough_Struct ** passthrough_data_ptr, int frame_size)
{

    t_PrePassthrough_Struct * passthrough_data;
    // allocate memory for storing configuration data
    *passthrough_data_ptr = xzpmalloc(sizeof(t_PrePassthrough_Struct));

    passthrough_data = *passthrough_data_ptr;
    passthrough_data->frame_size = frame_size;
}

/**
 * \brief Function for passthrough destroy and release memory.
 * \param Pointer to algorithms config data required to delete its context
 * \return void
 */
void pre_passthrough_destroy(t_PrePassthrough_Struct * passthrough_data)
{
    
    if (passthrough_data) {
        pfree(passthrough_data);
    }
}

/**
 * \brief Function for applying the passthrough to the input.
 * \param Pointer to algorithms config data required to execute its context
 * \param Pointer to input data
 * \param input_size
 * \param output
 * \return void
 */
void pre_passthrough_update(t_PrePassthrough_Struct * passthrough_data, signed *algo_input, ALGO_OUTPUT_INFO * algo_output)
{
    // Copy data from in to out...
    signed * input_data_ptr = algo_input;
    signed * output_data_ptr = algo_output->output_data;
    
    for(int i=0; i < passthrough_data->frame_size; ++i) {
        *output_data_ptr++ = *input_data_ptr++;
    }
}
