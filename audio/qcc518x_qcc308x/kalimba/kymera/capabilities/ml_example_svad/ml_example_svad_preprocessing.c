/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  ml_example_svad_preprocessing.c
 * \ingroup  capabilities
 *
 *
 *
 */

#include "ml_example_svad_cap.h"
#include "capabilities.h"
#include "preproc_common.h"

/****************************************************************************
Private Constant Declarations
*/

/**
 * \brief Helper function to update if all the input tensors are completly filled
 *
 * \param pre_processing_container pre processing data container
 * \return void
 */
static void update_tensor_formation_status(ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data)
{
    /* Get the input tensors to be filled */
    USECASE_INFO *usecase_info = (USECASE_INFO *)ml_engine_list_find(ml_example_svad_data->ml_engine_container->use_cases,(uint16)ml_example_svad_data->ml_engine_container->uc_id);
    for(int i=0; i<usecase_info->input_tensor.num_tensors; i++)
    {
        MODEL_TENSOR_INFO t = usecase_info->input_tensor.tensors[i];
        if(!t.is_filled)
        {
            /* one of the tensor is not filled */
            return;
        }
        
    }
    /* all required tensors are filled, we can proceed to run the engine */
    ml_example_svad_data->pre_processing_container->tensor_formation_complete = TRUE;
    return;
}

/**
 * \brief Helper function to fill a tensor tagged to an algorithm with
 *        algorithms output.
 *
 * \param ml_example_svad operator data structure
 *
 * \return void
 */
static void fill_tensor(ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data)
{
    /* If the tensor is completely filled apply rolling window assuming all the data
     * from current tensor has been moved to its linear buffer in previous iteration */
    ALGO_OUTPUT_INFO * algo_output = NULL;
    // ALGO_RECORD *algo_rec = ml_example_svad_data->pre_processing_container->algo_config;
    t_PREPROC_CONTAINER *pre_processing_container = ml_example_svad_data->pre_processing_container;
    unsigned num_tensors = pre_processing_container->num_tensors;
    unsigned tensor_id;
    /* Get the input tensors to be filled */
    USECASE_INFO *usecase_info = (USECASE_INFO *)ml_engine_list_find(ml_example_svad_data->ml_engine_container->use_cases,(uint16)ml_example_svad_data->ml_engine_container->uc_id);
    for(int i=0; i<num_tensors; i++)
    {
        MODEL_TENSOR_INFO *tensor = &usecase_info->input_tensor.tensors[i];
        tensor_id = tensor->tensor_id;
        for(unsigned j=0; j<num_tensors; j++)
        {
            if(pre_processing_container->algo_output[j].tensor_id == tensor_id)
            {
                algo_output = &pre_processing_container->algo_output[j];
                /* Found the output buffer details corresponding to this tensor */
                L2_DBG_MSG1("ML_EXAMPLE_SVAD:Preprocessing:Filling tensor id: %d",tensor_id);
                break;                
            }
        }
        if(!algo_output)
        {
            L2_DBG_MSG1("ML_EXAMPLE_SVAD:Preprocessing:Trying to filling invalid tensor id: %d",tensor->tensor_id);
        }
        if(!(cbuffer_calc_amount_space_in_words(tensor->cdata)))
        {
            /* Circular buffer associated with the tensor is filled, apply rolling window */
            L2_DBG_MSG1("ML_EXAMPLE_SVAD:Preprocessing:Apply rolling window since tensor is full %d", cbuffer_calc_amount_space_in_words(tensor->cdata));
            if(tensor->is_rolling)
            {
                /* move read pointer to algo_output->size before the write pointer buffer to achieve a right shift needed for rolling window */
                cbuffer_move_read_to_write_point(tensor->cdata, ROLLING_READ_PTR_OFFSET(tensor->data_length, algo_output->size));
                L2_DBG_MSG1("ML_EXAMPLE_SVAD:Preprocessing:before roll tensor space %d", cbuffer_calc_amount_space_in_words(tensor->cdata));
                /* move data from algo output to tensor cyclic buffer */
                cbuffer_write(tensor->cdata, algo_output->output_data, algo_output->size);

                L2_DBG_MSG1("ML_EXAMPLE_SVAD:Preprocessing:after roll tensor space %d", cbuffer_calc_amount_space_in_words(tensor->cdata));
                /* Mark the tensor as filled */
                tensor->is_filled = TRUE;
                L2_DBG_MSG3("ML_EXAMPLE_SVAD:Preprocessing:roll over Tensor filled completely - tensor_id: %d, is_filled:%d, tensor_size:%d", tensor->tensor_id, tensor->is_filled, tensor->data_length);
            }
        }
        else
        {
            /* The tensor circular buffer has space in it.Move data from algo output*/
            cbuffer_write(tensor->cdata, algo_output->output_data, algo_output->size);
            if((cbuffer_calc_amount_space_in_words(tensor->cdata)))
            {
                /* Tensor is not filled completly */
                tensor->is_filled = FALSE;
                L2_DBG_MSG3("ML_EXAMPLE_SVAD: Preprocessing: Tensor NOT filled completely - tensor_id: %d, is_filled:%d, tensor_space_left:%d", tensor->tensor_id, tensor->is_filled, cbuffer_calc_amount_space_in_words(tensor->cdata));
                /* we return since this tensor is NOT READY and so is the associated use case */
            }
            else
            {
                /* mark the tensor as filled and ready to be processed by the engine*/
                tensor->is_filled = TRUE;
                L2_DBG_MSG3("ML_EXAMPLE_SVAD: Preprocessing: Tensor filled completely - tensor_id: %d, is_filled:%d, tensor_size:%d", tensor->tensor_id, tensor->is_filled, tensor->data_length);
            }
        }
        algo_output = NULL;
    }
}

/**
 * \brief ml_example_svad Preprocessing data process function
 * \param pointer to the ml_example_svad op data 
 * \return none
 */
void ml_example_svad_pre_process(ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data)
{
    t_PREPROC_CONTAINER *pre_processing_container = ml_example_svad_data->pre_processing_container;
    unsigned amt_to_copy, copied;
    unsigned frame_count = ml_example_svad_data->ml_engine_container->frames_processed;

    /*********************************************************************************
     *                                                     
     *    ALGO IP           +----------------------+      TENSOR          
     *     BUFFER           |                      |      BUFFER          +----------+
     * +--------------+     |     PRE      --------|   +--------------+   |          |
     * | cBuff | lin  | --> |  PROCESSING |ALGO OP |-->| cBuff | lin  |-> |  ENGINE  |
     * |       | Buff |     |             |  BUFF  |   |       | Buff |   |          |
     * +--------------+     |              --------|   +--------------+   +----------+
     *                      +----------------------+                    
     *********************************************************************************
     */

    /*************************************************************************
     *                           FRAMING                                     
     * ***********************************************************************
     * Each complete frame for the input example SVAD is of size 40ms which has 
     * an overlap of 20ms with the previous frame.
     */

    amt_to_copy = pre_processing_container->input_roll_size;
    copied = cbuffer_copy(pre_processing_container->vad_ip_buffer.cdata, ml_example_svad_data->ip_buffer, amt_to_copy);

    /* since pre-processing and post-processing are not mapped 1:1, we delete all metadata */
    /* tags associated with the input data and create new tags at the output if required */
    if(buff_has_metadata(ml_example_svad_data->ip_buffer))
    {
        unsigned b4idx = 0, afteridx = 0;
        metadata_tag *mtag = buff_metadata_remove(ml_example_svad_data->ip_buffer, amt_to_copy * OCTETS_PER_SAMPLE, &b4idx, &afteridx);
        /* Free all the incoming tags */
        buff_metadata_tag_list_delete(mtag);
    }

    if(frame_count != 0)
    {
        /* Each time we are copying data worth 20ms. The Frame size for the SVAD preprocessing is 40ms with an */
        /* overlap of 20ms. Hence for the first frame, we will not have enough data to proceed. */

        /* Sanity check: check that the amount of data in the buffer is frame size */
        unsigned input_size = cbuffer_calc_amount_data_in_words(pre_processing_container->vad_ip_buffer.cdata);
        if (input_size != pre_processing_container->input_block_size)
        {
            L2_DBG_MSG1("ML_EXAMPLE_SVAD:Preprocessing:Trying to run preprocessing algorithm with an incorrect frame size of %d words",input_size);
            return;
        }
        
        /* Starting from the second frame onwards, read a complete frame worth of data into the algorithms input buffer */
        copied = cbuffer_read(pre_processing_container->vad_ip_buffer.cdata,
                              pre_processing_container->vad_ip_buffer.data,
                              pre_processing_container->input_block_size);

        /* Sanity check: If the intended amount of data was copied */
        if (copied != pre_processing_container->input_block_size)
        {
            L2_DBG_MSG1("ML_EXAMPLE_SVAD:Preprocessing:Could not copy complete frame to input buffer. Copied only %d words",copied);
            return;
        }        

        /* Move the read pointer of the circular buffer back for the next iteration */
        cbuffer_move_read_to_write_point(pre_processing_container->vad_ip_buffer.cdata, pre_processing_container->input_roll_size); 

        /* *************************************************************************************
         *                               TENSOR FORMATION
         * *************************************************************************************
         * The machine learning model for the example SVAD use-case expects 3 tensors in a batch
         * 1. Tensor ID 4 of size 80 words
         * 2. Tensor ID 0 of size 8 words    
         * 3. Tensor ID 2 of size 2 words     
         * Each execute of the Example SVAD preprocessing algorithm
         * which uses 20ms of new input data @ 16KHz will generate 3 tensors:
         * 1. Tensor ID 4 of size 16 words
         * 2. Tensor ID 0 of size 1 word
         * 3. Tensor ID 2 of size 2 words
         * This is done in the function vad_spectrum_update() which runs for every new
         * 20ms of data and produces output in the algorithm's output buffer.
         *
         * Inorder to form the first batch for the machine learning model to process
         * we need to wait till each of the tensors are filled. Tensor ID 4 will take 5
         * frames to fill(5*16=80), Tensor ID 0 will take 8 frames to fill(8*1=8) and
         * Tensor ID 2 will be completly filled in 1 frame(2*1=2). Hence for the machine
         * learning engine to start processing the first batch, we will wait for 8 frames.
         * For subsequent batches:
         * Tensor ID 4 will have 16 words from the new frame and (80-16=64 words) from the previous batch.
         * Tensor ID 0 will have 1 word from the new frame and (8-1=7 words) from the previous batch.
         * Tensor ID 2 will have 2 words from the new frame and nothing from the previous batch.
         *
         * This rolling tensor functionality is provided by using a buffer structure in the tensor 
         * which has a combination of both circular buffer and linear buffer.The function fill_tensor()
         * pulls out data from the algorithm's output buffer and puts it into the circular buffer of 
         * the tensor data structure and provides the rolling tensor functionality.The linear buffer 
         * is then filled with the tensors required by the machine learning engine for the current batch.
         * ***********************************************************************************************  
         */
        
        ml_spectrum_update(pre_processing_container->spectrum_data, pre_processing_container->vad_ip_buffer.data, &pre_processing_container->algo_output[0], pre_processing_container->input_block_size);
        /* Fill tensors using the algorithm's output */
        fill_tensor(ml_example_svad_data);

        /* Check if all tensors associated with the model are completly filled
         * If this is true, then the machine learning engine will copy data for
         * the current batch from the tensors's circular buffer to its linear buffer
         * and proceed to run the model
         */
        update_tensor_formation_status(ml_example_svad_data);
    
    }
    L2_DBG_MSG1("ML_EXAMPLE_SVAD:Preprocessing:Processed frame number %d",frame_count);
    return;
}

/**
 * \brief  ml_example_svad initialise pre-processing
 *
 * \param  ml_example_svad_data          Pointer to extra operator data
 * \param  usecase_id            Usecase identifier
 *
 */
bool ml_example_svad_init_pre_process(ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data, unsigned usecase_id)
{
    /* Get the usecase corresponding to the ID */
    USECASE_INFO *usecase_info = (USECASE_INFO *)ml_engine_list_find(ml_example_svad_data->ml_engine_container->use_cases,(uint16)usecase_id);

    /* get the preprocesing container data-structure */
    t_PREPROC_CONTAINER *pre_processing_container = ml_example_svad_data->pre_processing_container;

    pre_processing_container->num_tensors = usecase_info->input_tensor.num_tensors;
    ml_spectrum_create(&pre_processing_container->spectrum_data, pre_processing_container->input_sample_rate, FFT_INPUT_SIZE,
                                        FFT_SIZE, SPECTRAL_CHANNELS, SPEC_OUT_IDX, STAT_OUT_IDX, NORM_OUT_IDX, NORM_REQ, NORM_ALPHA);

    if (pre_processing_container->spectrum_data == NULL)
    {
        L2_DBG_MSG("ML_EXAMPLE_SVAD_init:Preprocessing: cannot create spectrum algo");
        return FALSE;
    }

    for(int i=0; i<usecase_info->input_tensor.num_tensors; i++)
    {
        MODEL_TENSOR_INFO *tensor = &usecase_info->input_tensor.tensors[i];
        /* create circular buffer for rolling tensor support */
        tCbuffer *cbuff = cbuffer_create_with_malloc_fast((tensor->data_length+1), BUF_DESC_SW_BUFFER);
        if(NULL == cbuff)
        {
            L2_DBG_MSG("ML_EXAMPLE_SVAD:Preprocessing:No memory left for adding preprocessing tensor data");
            return FALSE;
        }
        tensor->cdata = cbuff;
        tensor->is_rolling = TRUE;
        tensor->is_filled = FALSE;
    }

    memset(pre_processing_container->algo_output, 0, sizeof(ALGO_OUTPUT_INFO) * NUM_TENSORS);

    /* Allocate memory for the buffers used by the preprocessing algorithms */
    /* size of both the buffers will be same the input_block_size */
    unsigned size = pre_processing_container->input_block_size;
    tCbuffer *cbuff = cbuffer_create_with_malloc_fast(size+1, BUF_DESC_SW_BUFFER);
    signed *buff = xzpnewn(size, int32);
    pre_processing_container->vad_ip_buffer.cdata = cbuff;
    pre_processing_container->vad_ip_buffer.data = buff;
    return TRUE;
}

/**
 * \brief  ml_example_svad deinit pre-processing
 *
 * \param  ml_example_svad_data        Pointer to extra operator data
 * \param  usecase_id          Usecase identifier
 *
 */
bool ml_example_svad_deinit_pre_process(ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data, unsigned usecase_id)
{
    /* get the preprocesing container data-structure */
    t_PREPROC_CONTAINER *pre_processing_container = ml_example_svad_data->pre_processing_container;


    /* remove the cbuffer used for supporting rolling tensors */
    USECASE_INFO *usecase_info = (USECASE_INFO *)ml_engine_list_find(ml_example_svad_data->ml_engine_container->use_cases,(uint16)usecase_id);
    for(int i=0; i<usecase_info->input_tensor.num_tensors; i++)
    {
        MODEL_TENSOR_INFO tensor = usecase_info->input_tensor.tensors[i];
        if(NULL != tensor.cdata)
        {
            cbuffer_destroy(tensor.cdata);
            tensor.cdata = NULL;
        }
    }    
    /* Delete the algorithm's private data */
    
    ml_spectrum_destroy(pre_processing_container->spectrum_data);
 

    /* Free the allocated memory for the VAD input buffers */
    cbuffer_destroy(pre_processing_container->vad_ip_buffer.cdata);
    pfree(pre_processing_container->vad_ip_buffer.data);
    pre_processing_container->vad_ip_buffer.cdata = NULL;
    pre_processing_container->vad_ip_buffer.data = NULL;
    return TRUE;
}
