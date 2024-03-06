#ifndef MBDRC_PUB_H
#define MBDRC_PUB_H
/****************************************************************************
 * (c) 2022 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Confidential and Proprietary
 ****************************************************************************/
/**
 * \file mbdrc_api.h
 * \ingroup mbdrc_lib
 *
 * API header file for the MBDRC library.
 */

/*----------------------------------------------------------------------------
 * Include Files
 * -------------------------------------------------------------------------*/

#include "cvclib_c.h"
#include "mbdrc100_library.h"

/*----------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * -------------------------------------------------------------------------*/
#define MAX_BANDS 14
/*----------------------------------------------------------------------------
 * Type Declarations
 * -------------------------------------------------------------------------*/

typedef enum MBDRC_RESULT
{
    MBDRC_SUCCESS = 0,
    MBDRC_FAILURE,
    MBDRC_MEMERROR
} MBDRC_RESULT;

typedef struct mbdrc_static_struct_t
{
    uint32_t sample_rate; // Hz
    uint32_t framesize;   // Framesize in samples
    uint32_t nfft;
} mbdrc_static_struct_t;

#define PARAM_ID_MBDRC_BAND_EDGES 0x234567L // dummy value

typedef struct mbdrc_complex_buffer
{
   int *real_ptr;
   int *imag_ptr;
   int *exp_ptr;
}mbdrc_complex_buffer;

typedef struct mbdrc_freqbuffer
{
	mbdrc_complex_buffer* complex_buffer;
	int32_t num_bins;
}mbdrc_freqbuffer;

typedef struct mbdrc_band_edges_t
{
    /* Number of bands */
    int32_t n_bands;
    /* Array of Center Frequency */
    int32_t cf[MAX_BANDS];
    /* Low fft bin */
    int32_t low_bin[MAX_BANDS];
    /* High fft bin */
    int32_t high_bin[MAX_BANDS];
} mbdrc_band_edges_t;

MBDRC_RESULT mbdrc_get_mem_req(
    /* mbdrc_lib_mem_requirements_ptr : [OUT] :
    pointer to the structure for memory requirements */
    mbdrc_lib_mem_requirements_t *mbdrc_lib_mem_requirements_ptr,
    /* mbdrc_static_struct_ptr: [IN] :
    pointer to static library configuration */
    mbdrc_static_struct_t *mbdrc_static_struct_ptr);

MBDRC_RESULT mbdrc_init_memory(
    /* mbdrc_lib_ptr: [IN/OUT] :
    pointer to the library structure  */
    mbdrc_lib_t *mbdrc_lib_ptr,
    /* mbdrc_static_struct_ptr: [IN] :
    pointer to static library configuration */
    mbdrc_static_struct_t *mbdrc_static_struct_ptr,
    /* memory_ptr: [IN] :
    pointer to memory */
    int8 *mem_ptr,
    /* memory_size: [IN] :
    size of the memory */
    uint32 mem_size);

#endif // MBDRC_H
