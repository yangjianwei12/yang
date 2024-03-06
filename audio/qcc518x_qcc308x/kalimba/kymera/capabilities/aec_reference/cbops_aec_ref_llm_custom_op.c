/****************************************************************************
 * Copyright (c) 2022 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  cbops_aec_ref_llm_custom_op.c
 * \ingroup cbops
 *
 * This file contains functions for the custom cbops operator used in LLMode
 */

/****************************************************************************
 Include Files
*/

#include "aec_reference_cap_c.h"
#include "cbops_aec_ref_llm_custom_op.h"
#include <stdfix.h>

/****************************************************************************
Public Function Definitions
*/

#ifdef AEC_REFERENCE_ENABLE_LOW_LATENCY_PATH

/**
 * create_aec_ref_llm_custom_op
 * \brief   Creates the custom cbop operator used in low latency mode.
 *
 * \param num_inputs Number of input channels (either 1 or 2).
 * \param input_idx  cbop buffer indices for input.
 * \param output_idx cbop buffer indices for output.
 *
 * \return The custom cbop operator created.
 *
 */
cbops_op* create_aec_ref_llm_custom_op(unsigned num_inputs,
                                       unsigned *input_idx,
                                       unsigned *output_idx)
{
    cbops_op *op = NULL;
    /* LLM supports up to a maximum of 2 input mics */
    if(num_inputs > 2)
    {
        return op;
    }
    /* LLM supports only one output */
    unsigned num_outputs = 1;


    // cbops param struct size (header plus cbop-specific parameters)
    op = (cbops_op*)xzpmalloc(sizeof_cbops_op(cbops_aec_ref_llm_custom_op,
                                              num_inputs,
                                              num_outputs));
    if(op)
    {
        cbops_aec_ref_llm_custom_op *params;

        /* Setup the operator function table */
        op->function_vector = cbops_aec_ref_llm_custom_op_table;

        /* Setup cbop param struct header info */
        params = (cbops_aec_ref_llm_custom_op*)cbops_populate_param_hdr(op,
                                                                        num_inputs,
                                                                        num_outputs,
                                                                        input_idx,
                                                                        output_idx);

        /* Setup cbop specific parameter */
        params->num_inputs = num_inputs;
    }
    return op;
}

void configure_aec_ref_llm_custom_op(cbops_op *op, cbops_aec_ref_llm_custom_op_ui *ui_params)
{
    cbops_aec_ref_llm_custom_op *params = CBOPS_PARAM_PTR(op, cbops_aec_ref_llm_custom_op);

    /* Map user params to actual parameters here
     * As an example, the first parameter is mapped to the gain
     * value here. By default - this value is 1.
     */
    params->gain = ui_params->rsv_param_0;
}

/**
 * aec_ref_llm_custom_op_main_processing
 * \brief   The main processing function for the custom cbop operator in
 *          low latency mode.
 *
 * \param input_buff1 Input buffer pointer for the first channel.
 * \param input_buff2 Input buffer pointer for the second channel.
 * \param output_buff Output buffer pointer.
 * \param params      cbop operator parameters.
 *
 * \return The amount of data that the operator has written into
 *         the output buffer.
 *
 * This is the main processing function of the cbop operator where the
 * underlying operator runs. When this function is called, the cbop
 * framework provides the pointers to the input buffer and the output
 * buffer. Note that the input and output buffers are linear buffers.
 * The 'amt_data' field is also updated by the framework. Please
 * note that the cbop operator at this stage is obliged to read exactly
 * that amount of data from the input buffers.
 */
unsigned aec_ref_llm_custom_op_main_processing(int *input_buff1,
                                               int *input_buff2,
                                               int *output_buff,
                                               cbops_aec_ref_llm_custom_op *params)
{
    /* The amount of data(in words) that the main processing function is
     * obliged to process at this point.
     */
    unsigned amt_data_in = params->amt_data;
    /* The example here shows a 1:1 operator, hence the amount of
     * data produced is same as the amount of data consumed
     */
    unsigned amt_data_out = amt_data_in;
    accum temp = 0k;

    /* For mono - just copy the input to output with the gain */
    if(params->num_inputs == 1)
    {
        #pragma loop minitercount(1)
        for(unsigned i=0; i < amt_data_in; i++)
        {
            temp   = __builtin_reinterpret_int_to_fract(params->gain) *
                     __builtin_reinterpret_int_to_fract(*input_buff1++);
            *(sat fract *)output_buff++ = (sat fract)temp;
        }
    }
    else
    {
        /* For stereo  - MIXER example */
        #pragma loop minitercount(1)
        for(unsigned i=0; i < amt_data_in; i++)
        {
            /* (gain * input_buff1) + ((1-gain) * input_buff2) */
            temp =  __builtin_reinterpret_int_to_fract(params->gain) *
                    __builtin_reinterpret_int_to_fract(*input_buff1++);
            temp += __builtin_reinterpret_int_to_fract(INT_MAX - params->gain) *
                    __builtin_reinterpret_int_to_fract(*input_buff2++);
            *(sat fract *)output_buff++ = (sat fract)temp;
        }
    }
    return amt_data_out;
}

/**
 * aec_ref_llm_custom_op_amount_to_process
 * \brief   Amount to process function for the custom cbop operator
 *          in low latency mode.
 *
 * \param max_amt_data  Maximum amount of data the operator can process
 *
 * \return              The amount of data that the operator will process
 *                      from its input buffer.
 *
 * This function is invoked by the cbop graph before running the main
 * processing function of the cbop operator. In this function, the
 * operator needs to inform the graph about the amount of data it
 * can consume from its input. The function will provide as an argument
 * the maximum amount of data that the operator can consume from its
 * input assuming it to be a 1:1 operator. This means that the operator
 * also is limited by the same amount of space in its output buffer.
 * This function is responsible to adjust this amount so that the
 * operator does not produce more data then the output buffers can accept.
 *
 * If the operator is a 1:1 operator (amount produced is same as the
 * amount consumed), then the returned amount can be either equal to
 * or less than the maximum amount.
 * If the operator is a 1:N operator (amount produced is more than the
 * amount consumed), then the returned amount should strictly be less
 * than the maximum amount.
 *
 * Once this amount is decided, the operator is obliged to process
 * this amount of data from its input buffers in the main processing
 * function.
 */
unsigned aec_ref_llm_custom_op_amount_to_process(unsigned max_amt_data)
{
    /* As an example, we assume a 1:1 operator and process the
     * maximum amount of data that we can process */
    return max_amt_data;
}

#endif /* AEC_REFERENCE_ENABLE_LOW_LATENCY_PATH */