/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/
#if !defined(EAI_MEL_H)
#define EAI_MEL_H

#include <stdfix.h>
#include "preproc_common.h"

typedef struct tEAIMelStuct
{
    int sampling_rate;
    int fft_size;
    int n_spectral_channels;
    int * output_buffer;
} tEAIMelStruct;

/**
 * \brief Function for mel filters initialization.
 * \param Pointer to algorithms config data required to create its context
 * \return Pointer to private structure of this instance
 */
void ml_mel_create(tEAIMelStruct ** mel_data, int sampling_rate, int fft_size, int n_spectral_channels);

/**
 * \brief Function for applying mel filters to the input.
 * \param Pointer to algorithms config data required to execute its context
 * \param Pointer to input data
 * \param input_size
 * \param output
 * \return void
 */
void ml_mel_update(tEAIMelStruct * mel_data, signed *algo_input, unsigned input_size, ALGO_OUTPUT_INFO * output);

/**
 * \brief Function for mel filters shutdown and release memory.
 * \param Pointer to algorithms config data required to delete its context
 * \return void
 */
void ml_mel_destroy(tEAIMelStruct * mel_data);

#endif
