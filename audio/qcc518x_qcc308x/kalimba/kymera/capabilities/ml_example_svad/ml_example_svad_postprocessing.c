/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  ml_example_svad_postprocessing.c
 * \ingroup  capabilities
 *
 *
 *
 */

#include "ml_example_svad_cap.h"
#include "capabilities.h"
#include "svad_post_processing.h"


/**
 * \brief Function to perform svad post-processing
 * \param pointer to the svad operator data
 * \return none
 */
static void svad_post_processing(ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data)
{
    t_POSTPROC_CONTAINER  *post_processing_container = ml_example_svad_data->post_processing_container;
    ML_ENGINE_OP_DATA *mle_data = ml_example_svad_data->ml_engine_container;
    USECASE_INFO *usecase_info = (USECASE_INFO *)ml_engine_list_find(mle_data->use_cases, (uint16)mle_data->uc_id);

    tCbuffer *cbuff = post_processing_container->vad_op_buffer.cdata;
    signed *buff = post_processing_container->vad_op_buffer.data;

    /* Fetch output tensors from model runner linked list and send it to the post processing */
    for(int i =0; i<usecase_info->output_tensor.num_tensors; i++)
    {
        MODEL_TENSOR_INFO tensor = usecase_info->output_tensor.tensors[i];
        if(tensor.data != NULL)
        {
            int32 *result = (int32 *)(tensor.data);
            cbuffer_write(cbuff, result, tensor.data_length);
            if(!(cbuffer_calc_amount_space_in_words(cbuff)))
            {
                cbuffer_read(cbuff, (int *)buff, VAD_RESULT_COUNT);
                cbuffer_move_read_to_write_point(cbuff, VAD_RESULT_COUNT - 1);
                /* Save current state and invoke post-processing library */
                post_processing_container->vad_status.state_change_det = FALSE;
                int curr_state  = post_processing_container->vad_status.current_state;
                int last_state  = curr_state;
                svad_detect_state_change(&curr_state, post_processing_container->vad_op_buffer.data);
                /* Check if a state change is detected */
                if(last_state != curr_state)
                {
                    /* VAD status change detected */
                    post_processing_container->vad_status.state_change_det = TRUE;
                    post_processing_container->vad_status.current_state = curr_state;
                }
            }
            else
            {
                return;
            }
        }
    }
}

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief ml_example_svad Postprocessing  data process function
 * \param pointer to the post_processing data container
 * \return none
 */
void ml_example_svad_post_process(ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data)
{
    svad_post_processing(ml_example_svad_data);
    return;
}

/**
 * \brief  ml_example_svad initialise post-processing
 *
 * \param  op_ext_data          Pointer to extra operator data
 * \return none
 *
 */
bool ml_example_svad_init_post_process(ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data)
{
    t_POSTPROC_CONTAINER  *post_processing_container = ml_example_svad_data->post_processing_container;
    /* Allocate buffer for post-processing result */
    tCbuffer *cbuff = cbuffer_create_with_malloc_fast(VAD_RESULT_COUNT + 1, BUF_DESC_SW_BUFFER);
    signed *buff = xzpnewn(VAD_RESULT_COUNT, int32);
    if((NULL == cbuff) || (NULL == buff))
    {
        L2_DBG_MSG("ML_EXAMPLE_SVAD:Postprocessing:VAD Output buffer allocation failed!");
        return FALSE;
    }
    post_processing_container->vad_op_buffer.cdata = cbuff;
    post_processing_container->vad_op_buffer.data = buff;
    /* Initialise state change detected to FALSE */
    post_processing_container->vad_status.state_change_det = FALSE;
    /* Intialise current state to -1 */
    post_processing_container->vad_status.current_state = -1;
    return TRUE;
}

/**
 * \brief  ml_example_svad deinit post-processing
 *
 * \param  op_ext_data          Pointer to extra operator data
 * \return none
 *
 */
void ml_example_svad_deinit_post_process(ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data)
{
    t_POSTPROC_CONTAINER  *post_processing_container = ml_example_svad_data->post_processing_container;
    /* Destroy the VAD buffer */
    cbuffer_destroy(post_processing_container->vad_op_buffer.cdata);
    pfree(post_processing_container->vad_op_buffer.data);
    post_processing_container->vad_op_buffer.cdata = NULL;
    post_processing_container->vad_op_buffer.data = NULL;
}
