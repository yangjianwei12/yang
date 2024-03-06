/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/
#if !defined (PRE_PASSTHROUGH_H)
#define PRE_PASSTHROUGH_H

#include <stdfix.h>
#include "preproc_common.h"

typedef struct t_PrePassthrough_Struct
{

   int frame_size;

} t_PrePassthrough_Struct;

/**
 * \brief Function for passthrough creation
 * \param Pointer to algorithms config data required to create its context
 * \return Pointer to private structure of this instance
 */
void pre_passthrough_create(t_PrePassthrough_Struct ** passthrough_data_ptr,int frame_size);

/**
 * \brief Function for passthrough destroy and release memory.
 * \param Pointer to algorithms config data required to delete its context
 * \return void
 */
void pre_passthrough_destroy(t_PrePassthrough_Struct * passthrough_data);

/**
 * \brief Function for applying the passthrough to the input.
 * \param Pointer to algorithms config data required to execute its context
 * \param Pointer to input data
 * \param input_size
 * \param output
 * \return void
 */
void pre_passthrough_update(t_PrePassthrough_Struct * passthrough_data, signed *algo_input, ALGO_OUTPUT_INFO * output);

#endif
