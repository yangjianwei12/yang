#ifndef MBDRC_H
#define MBDRC_H
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

//#include "cvclib_api.h"
#include "cvclib_c.h"
#include "drc_api.h"
#include "mbdrc_calibration_api.h"
#include "mbdrc100_library.h"
#include "mbdrc_api_public.h"

/*
mbdrc_get_mem_req
- Provides memory requirements of the library based on the configuration set
- This function is expected to be called before mbdrc_init_memory is invoked.

Return:
result - MBDRC_RESULT
   */



/*
mbdrc_set_param
- Sets parameter defined by the param ids
- Supports Param IDs:
  - PARAM_ID_MBDRC_PARAM: TBD

Return:
result - MBDRC_RESULT
*/
MBDRC_RESULT mbdrc_set_param(
    /* mbdrc_lib_ptr: [IN/OUT] :
    pointer to the library structure  */
    mbdrc_lib_t *mbdrc_lib_ptr,
    /* param_buffer_ptr: [IN] :
    pointer to parameter buffer */
    int8_t *params_buffer_ptr,
    /* param_id: [IN] :
    Unique parameter ID*/
    uint32 param_id,
    /* param_size: [IN] :
    size of the parameter data*/
    uint32 param_size);

/*
mbdrc_set_param
- Sets parameter defined by the param ids
- Supports Param IDs:
  - PARAM_ID_MBDRC_PARAM: TBD

Return:
result - MBDRC_RESULT
   */
MBDRC_RESULT mbdrc_set_public_param(
	/* mbdrc_lib_ptr: [IN/OUT] : pointer to the library structure  */
	mbdrc_lib_t *mbdrc_lib_ptr,
	/* param_buffer_ptr: [IN] : pointer to parameter buffer */
	int8_t *mbdrc_public_params
	);
/*
mbdrc_get_param
- Returns parameters defined by the param ids in the buffer pointer
- Supports Param IDs:
 - PARAM_ID_MBDRC_PARAM: TBD
Return:
result - MBDRC_RESULT
   */
MBDRC_RESULT mbdrc_get_param(
    /* mbdrc_lib_ptr: [IN/OUT] :
    pointer to the library structure  */
    mbdrc_lib_t *mbdrc_lib_ptr,
    /* param_buffer_ptr: [OUT] :
    pointer to parameter buffer for writing param data by library */
    int8_t *params_buffer_ptr,
    /* param_id: [IN] :
    Unique parameter ID*/
    uint32 param_id,
    /* buffer_size: [IN] :
    Size of the input buffer available for writing */
    int32_t buffer_size,
    /* param_size_ptr: [OUT] :
    Actual size of the parameter data written by the library */
    uint32 *param_size_ptr);

/*
mbdrc_process
- Returns a pointer to linear MB-DRC gains in Q8.23 (out_gain_ptr)
Return:
result - MBDRC_RESULT
*/
MBDRC_RESULT mbdrc_process(
    /* pData [IN]: pointer to MBDRC data struct */
    mbdrc_lib_t *mbdrc_lib_ptr,
    /* in_ptr [IN]:
    pointer to freq-domain input audio channel */
    freqbuffer_16_t *in_ptr,
    /* out_gain_ptr [OUT]:
    pointer to output gain */
    int32_t *out_gain_ptr);

MBDRC_RESULT mbdrc_public_param_dynamic(
	/* mbdrc_lib_ptr: [IN/OUT] : pointer to the library structure  */
	mbdrc_lib_t *mbdrc_lib_ptr,
	/* param_buffer_ptr: [IN] : pointer to parameter buffer */
	int8_t *mbdrc_public_params
	);
#endif /* #ifndef MBDRC_H */
