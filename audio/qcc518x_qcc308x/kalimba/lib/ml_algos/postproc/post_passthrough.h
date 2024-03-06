/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/
#if !defined (POST_PASSTHROUGH_H)
#define POST_PASSTHROUGH_H

#include <stdfix.h>
#include "postproc_common.h"

typedef struct t_PostPassthrough_Struct
{

   int frame_size;

} t_PostPassthrough_Struct;

/**
 * \brief Function for passthrough creation
 * \param Pointer to algorithms config data required to create its context
 * \return Pointer to private structure of this instance
 */
void post_passthrough_create(t_PostPassthrough_Struct ** passthrough_data_ptr, int frame_size);

/**
 * \brief Function for passthrough destroy and release memory.
 * \param Pointer to algorithms config data required to delete its context
 * \return void
 */
void post_passthrough_destroy(t_PostPassthrough_Struct * passthrough_data);

/**
 * \brief Function for applying the passthrough to the input.
 * \param Pointer to algorithms config data required to execute its context
 * \param Pointer to input data
 * \param input_size
 * \param output
 * \return void
 */
void post_passthrough_update(t_PostPassthrough_Struct * passthrough_data, signed *audio_output, ALGO_INPUT_INFO * ml_output);
#endif
