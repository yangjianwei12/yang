/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  @@@cap_name@@@_preprocessing.c
 * \ingroup  capabilities
 *
 *
 *
 */

#include "@@@cap_name@@@_cap.h"
#include "capabilities.h"
#include "pre_passthrough.h"
/****************************************************************************
Private Constant Declarations
*/


/**
 * \brief @@@cap_name@@@ Preprocessing data process function
 * \param pointer to the @@@cap_name@@@ op data 
 * \return none
 */
void @@@cap_name@@@_pre_process(@@@cap_name^U@@@_OP_DATA *@@@cap_name@@@_data)
{
    /* The data for running the preprocessing algorithms is copied from the capability's
     * input buffer. Once the input buffer of the capability has enough data:
     * Step 1: Copy one frame worth of data into the linear buffer.
     * Step 2: Run the preprocessing algorithm. This preprocessing algorithm reads from the
     *         linear input buffer, processes it and writes into the buffer associated with
     *         the algo output - which is in turn is the input tensor buffer.
     */
    int copied;
    copied = cbuffer_read(@@@cap_name@@@_data->ip_buffer,
                              @@@cap_name@@@_data->pre_processing_container->linear_input_buffer,
                              @@@cap_name@@@_data->pre_processing_container->input_block_size);

    L2_DBG_MSG1("postproc: copied to input: %d",copied);
    /* Run preprocessing algorithm */
    pre_passthrough_update(@@@cap_name@@@_data->pre_processing_container->passthrough_data,
                           @@@cap_name@@@_data->pre_processing_container->linear_input_buffer,
                           @@@cap_name@@@_data->pre_processing_container->algo_output);

    return;
}

/**
 * \brief @@@cap_name@@@ Preprocessing create
 * \param pointer to the @@@cap_name@@@ op data 
 * \return none
 */
void @@@cap_name@@@_pre_processing_create(@@@cap_name^U@@@_OP_DATA *@@@cap_name@@@_data)
{
    /* Initilaise preprocessing:
     * 1. Create a pre_processing conatiner and initialise it.
     * 2. Populate algo_output. This is a static array of type ALGO_OUTPUT_INFO (see lib/ml_algos/preproc_common.h)
     *    The size of this array depends on the number of input tensors(defined in *_defs.h).
     *    For each input tensor, this stores the tensor_id, tensor_size and has a pointer to the
     *    data buffer of the tensor. This is the buffer into which the preprocessing algorithms writes
     *    input tensors to. This buffer pointer needs to be pointed to the input tensor buffer already
     *    allocated by the Kymera Machine learning framework on activating the model. Every time a
     *    model is loaded and activated using specific operator messages, the Kymera Machine learning
     *    framework will allocate buffers for each tensors internally. We iterate over all the tensors
     *    in the model and find the buffer associated with the input tensors and associate it to
     *    pointer in the correponding algo_output entry.
     * 3. Create a linear buffer which stores one frame required by the preprocessing algorithms. Data
     *    is copied here from the input buffer of the capability (circular buffer). Once we accumulate
     *    enough data required by the preprocessing algorithm, we copy data into this linear buffer. The
     *    preprocessing algorithms uses this buffer as input data, processes it and writes into the buffer
     *    associated with the algo output above - which in turn is the input tensor buffer.
     */

    /* Create and initialise preprocessing container */
    t_PREPROC_CONTAINER *pre_proc_container = @@@cap_name@@@_data->pre_processing_container;
    /* Each iteration of the postprocessing algorithms gives us input_block_size worth of data */
    pre_proc_container->input_block_size = (int) (INPUT_BLOCK_PERIOD * INPUT_SAMPLE_RATE);
    pre_proc_container->input_sample_rate = INPUT_SAMPLE_RATE;
    pre_proc_container->num_tensors = NUM_IP_TENSORS;

    /* Populate algo_output: We have one preprocessing algorithm and one input tensor.
     * Hence we have only one ALGO_OUTPUT_INFO type entry in our preprocessing container
     * and the size of algo_output is 1.
     * Change this depending on the number of preprocessing algorithms and number of input
     * tensors.
     */    
    pre_proc_container->algo_output[0].tensor_id = INPUT_TENSOR_ID;
    pre_proc_container->algo_output[0].size = IP_TENSOR_SIZE;   

    /* Create linear input buffer */
    pre_proc_container->linear_input_buffer = xzpmalloc(pre_proc_container->input_block_size * sizeof(signed));

    pre_passthrough_create(&pre_proc_container->passthrough_data,pre_proc_container->input_block_size);

    /* Map the data buffer pointer for each entry in algo_output to the corresponding
     * input tensor buffer as described above.
     */
    ALGO_OUTPUT_INFO * algo_output = NULL;
    unsigned tensor_id;
    /* Get the input tensors to be filled */
    USECASE_INFO *usecase_info = (USECASE_INFO *)ml_engine_list_find(@@@cap_name@@@_data->ml_engine_container->use_cases,(uint16)@@@cap_name@@@_data->ml_engine_container->uc_id);
    for(int i=0; i<usecase_info->input_tensor.num_tensors; i++)
    {
        MODEL_TENSOR_INFO *tensor = &usecase_info->input_tensor.tensors[i];
        tensor_id = tensor->tensor_id;
        for(unsigned j=0; j<usecase_info->input_tensor.num_tensors; j++)
        {
            if(pre_proc_container->algo_output[j].tensor_id == tensor_id)
            {
                /* Found the output buffer details corresponding to this tensor */
                algo_output = &pre_proc_container->algo_output[j];
                /* Map engine tensor data location to algo_output */
                algo_output->output_data = (signed *)tensor->data;
                L2_DBG_MSG1("ML_EXAMPLE_SVAD:Preprocessing:mapping tensor id: %d",tensor_id);
                break;                
            }
        }
        if(!algo_output)
        {
            L2_DBG_MSG1("ML_EXAMPLE_SVAD:Preprocessing: cannot map tensor id: %d",tensor->tensor_id);
        }    
    }
    return;
}
/**
 * \brief @@@cap_name@@@ Preprocessing destroy
 * 
 * \param pointer to the preprocessing container data structure 
 * \return none
 */
void @@@cap_name@@@_pre_processing_destroy(t_PREPROC_CONTAINER *pre_proc_container)
{
    pre_passthrough_destroy(pre_proc_container->passthrough_data);
    pfree(pre_proc_container->linear_input_buffer);
    return;
}
 
