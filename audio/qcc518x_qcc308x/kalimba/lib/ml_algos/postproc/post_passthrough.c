/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

#include "math_lib.h"
#include <string.h>
#include <audio_log/audio_log.h>
#include "post_passthrough.h"


/**
 * \brief Function for passthrough creation.
 * \param Pointer to algorithms config data required to create its context
 * \return Pointer to private structure of this instance
 */
void post_passthrough_create(t_PostPassthrough_Struct ** passthrough_data_ptr, int frame_size)
{

    t_PostPassthrough_Struct * passthrough_data;
    // allocate memory for storing configuration data
    *passthrough_data_ptr = xzpmalloc(sizeof(t_PostPassthrough_Struct));

    passthrough_data = *passthrough_data_ptr;
    passthrough_data->frame_size = frame_size;

}

/**
 * \brief Function for passthrough destroy and release memory.
 * \param Pointer to algorithms config data required to delete its context
 * \return void
 */
void post_passthrough_destroy(t_PostPassthrough_Struct * passthrough_data)
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
void post_passthrough_update(t_PostPassthrough_Struct * passthrough_data, signed *audio_output, ALGO_INPUT_INFO * ml_output)
{
    // Copy data from in to out...
    signed * input_data_ptr = ml_output->input_data;
    signed * output_data_ptr = audio_output;
    
    for(int i=0; i < passthrough_data->frame_size; ++i) {
        *output_data_ptr++ = *input_data_ptr++;
    }
}
