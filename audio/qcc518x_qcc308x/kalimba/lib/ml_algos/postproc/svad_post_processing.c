/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

#include <stdfix.h>
#include "types.h"
#include "svad_post_processing.h"


/****************************************************************************
Private Constant Declarations
*/

/* refrence thresholds are [5e-4, 1e-3, 2e-4, 5e-5, 1e-5, 3e-6] */
static sat fract const vad_thresholds[VAD_RESULT_COUNT-1] = {0.0005r, 0.001r, 0.0002r, 0.00005r, 0.00001r, 0.000003r};

/****************************************************************************
Private Function Definitions
*/

/**
 * \brief Multiplies last n model outputs and returns the final result.
 * \param pointer to the model output linear buffer
 * \param start index of the linear buffer
 * \param flag indicating if the product needs be subtracted from 1.
 * \return min value
 */
static sat fract mutiply_results(int32 *results, int start, bool substract_from_one)
{
    sat fract *res = (sat fract *)results;
    sat fract prod = res[start];
    if(substract_from_one)
    {
        prod = (1 - (sat fract)res[start]);
    }
    for(int i=start+1; i < VAD_RESULT_COUNT; i++)
    {

        if(substract_from_one)
        {
            prod *= (sat fract)(1-(sat fract)res[i]);
        }
        else
        {
            prod *= (sat fract)res[i];
        }
    }
    return  prod;
}

/**
 * \brief Function to find the minimum out of last n model outputs
 * \param pointer to the model output linear buffer
 * \param start index of the linear buffer
 * \return min value
 */
static sat fract get_min_results(int32 *res, int start)
{
    int min = res[start];
    for(int i=start+1; i< VAD_RESULT_COUNT; i++)
    {
        min = __builtin_min(min, res[i]);
    }
    sat fract *s = (sat fract *)&min;
    return *s;
}

/**
 * \brief Function to process the last n model outputs and interprate if the VAD is detected
 * \param current state
 * \param vad_results
 * \return none
 */
void svad_detect_state_change(int *current_state, int *vad_results)
{
    int curr_state  = *current_state; 

    /* Proceed to compute the VAD decision */
    if(curr_state == 1)
    {
        curr_state = (mutiply_results((int32 *)vad_results, 2, FALSE) >  vad_thresholds[0]);
    }
    else if((mutiply_results((int32 *)vad_results, 2 , TRUE) <  vad_thresholds[3]) && ((get_min_results((int32 *)vad_results, 2) > 0.9)))
    {
        curr_state = 1;
    }
    else if((mutiply_results((int32 *)vad_results, 1 , TRUE) <  vad_thresholds[4]) && ((get_min_results((int32 *)vad_results, 1) > 0.9)))
    {
        curr_state = 1;
    }
    else if((mutiply_results((int32 *)vad_results, 0 , TRUE) <  vad_thresholds[5]) && ((get_min_results((int32 *)vad_results, 0) > 0.9)))
    {
        curr_state = 1;
    }
    else
    {
        curr_state = 0;
    }
    *current_state = curr_state;
    return;
}
