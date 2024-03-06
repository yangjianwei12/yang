#ifndef CBOPS_AECREF_LLM_CUSTOM_OP_H
#define CBOPS_AECREF_LLM_CUSTOM_OP_H

#include "cbuffer_c.h"

typedef struct cbops_aec_ref_llm_custom_op
{
    /* Required entries */

    /* Number of input channels - can be either 1 or 2 */
    unsigned num_inputs;
    /* Amount of data that the custom processing should
     * consume from the input buffer.*/
    unsigned amt_data;

    /* Custom entries */
    /* Default gain parameter as an example */
    unsigned gain;
}cbops_aec_ref_llm_custom_op;

/* User interface parameter structure */
typedef struct cbops_aec_ref_llm_custom_op_ui
{
    int32 rsv_param_0;
    int32 rsv_param_1;
    int32 rsv_param_2;
    int32 rsv_param_3;
    int32 rsv_param_4;
}cbops_aec_ref_llm_custom_op_ui;

/** The address of the function vector table. This is aliased in ASM */
extern unsigned cbops_aec_ref_llm_custom_op_table[];

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
 * The function should also return the amount of data it has written
 * into its output buffers.
 */

unsigned aec_ref_llm_custom_op_main_processing(int *input_buff1,
                                               int *input_buff2,
                                               int *output_buff,
                                               cbops_aec_ref_llm_custom_op *params);

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
 * than the maximum amount. As an example, if the operator produces 2
 * samples for each sample consumed and the max_amt_data is 50 - then
 * the operator can only process 25 samples from its input buffer.
 *
 * Once this amount is decided, the operator is obliged to process
 * this amount of data from its input buffers in the main processing
 * function.
 */

unsigned aec_ref_llm_custom_op_amount_to_process(unsigned max_amt_data);

#endif /* CBOPS_AECREF_LLM_CUSTOM_OP_H */
