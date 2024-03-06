// *****************************************************************************
// Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
// *****************************************************************************

#ifndef MBDRC100_LIB_H
#define MBDRC100_LIB_H

#include "types.h"
#include "cvclib_c.h"

// MBDRC100 Kalimba cVc input structure
typedef struct mbdrc100_in_buffer_t
{
    t_filter_bank_channel_object* cvc_freq_buffer;
    unsigned num_bins;
}mbdrc100_in_buffer_t;

// DRC lib master lib structure
typedef struct mbdrc_lib_t
{
    void *lib_mem_ptr; // ptr to the total chunk of lib mem
} mbdrc_lib_t;

// DRC lib mem requirements structure
typedef struct mbdrc_lib_mem_requirements_t
{
    uint32 lib_memory_size; // size of the lib mem pointed by lib_mem_ptr
    uint32 lib_stack_size;  // stack mem size
} mbdrc_lib_mem_requirements_t;

typedef struct mbdrc100_object_t
{
    mbdrc100_in_buffer_t *in_buffer;
    void *in_buffer_complex_2x16b;      /* cint2x16* */
    int32_t *gains;
    unsigned *param_ptr;                /* ptr to user parameters */
    unsigned fftbin_factor;
    mbdrc_lib_t mbdrc_lib;
    mbdrc_lib_mem_requirements_t mbdrc_mem_req;
}mbdrc100_object_t;

#endif /* #ifndef MBDRC100_LIB_H */
