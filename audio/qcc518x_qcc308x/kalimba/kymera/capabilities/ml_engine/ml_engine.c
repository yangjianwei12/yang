/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  ml_engine.c
 * \ingroup  capabilities
 *
 * ML Engine Capability
 *
 */

/****************************************************************************
Include Files
*/
#include <string.h>
#include "ml_engine.h"
#include "ml_engine_usecase_manager.h"

/****************************************************************************
Private Function Definitions
*/

/****************************************************************************
Public Helper functions
*/

/**
 * \brief Helper function to reset the persistent tensors
 *
 * \param opx_data ML Engine extra operator data
 * \param usecase_id usecase identifier of the model
 */
void ml_engine_reset_buffer(ML_ENGINE_OP_DATA* opx_data, unsigned usecase_id)
{
    USECASE_INFO *usecase_info = (USECASE_INFO *)ml_engine_list_find(opx_data->use_cases, (uint16)usecase_id);

    ML_PROPERTY *property = (ML_PROPERTY *)&usecase_id;

    /* Reset usecase */
    ml_set_config(usecase_info->ml_model_context, RESET_USECASE, property);


    /* Reset the current count after resetting the buffer */
    usecase_info->current_batch_count = 0;
}

/**
 * \brief Helper function to load the model file.
 *
 * \param opx_data ML Engine extra operator data.
 * \param usecase_id Usecase identifier for which to load model.
 * \param batch_reset_count batch execution count after which persistent tensor to be reset
 * \param file_handle Kymera file handle to the downloaded model file.
 * \param access_method Access method for the model.
 * \return TRUE if model load is successful else FALSE.
 */
bool ml_engine_load(ML_ENGINE_OP_DATA* opx_data, unsigned usecase_id, unsigned batch_reset_count, unsigned file_handle, unsigned access_method)
{
    bool status = FALSE;
    ML_CONTEXT *ml_model_context = NULL;

    /* load and parse the model file */
    status = ml_load(usecase_id, file_handle, (unsigned) access_method, &ml_model_context);
    /* Throw error and exit if model load fails */
    if(status == FALSE)
    {
        L2_DBG_MSG("MLE: Model Load Failed!");
        return status;
    }

    /* save context info in operator data */
    USECASE_INFO *usecase_info = (USECASE_INFO *)xzppmalloc(sizeof(USECASE_INFO), DM2);
    if (usecase_info == NULL)
    {
        L2_DBG_MSG("Failed to allocate usecase ");
        return FALSE;
    }
    opx_data->uc_id = usecase_id;
    usecase_info->usecase_id = usecase_id;
    usecase_info->batch_reset_count = batch_reset_count;
    usecase_info->ml_model_context = ml_model_context;

    /* add to the usecase list */
    if(!(ml_engine_list_add(&opx_data->use_cases, (void *)usecase_info, (uint16)usecase_info->usecase_id)))
    {
        return FALSE;
    }

    return status;
}

/**
 * \brief Helper function to activate the model file.
 *
 * \param opx_data ML Engine extra operator data.
 * \param usecase_id Usecase ID for which to activate the model.
 * \return TRUE if model activate is successful else FALSE.
 */
bool ml_engine_activate(ML_ENGINE_OP_DATA* opx_data, unsigned usecase_id)
{
    bool status = FALSE;
    USECASE_INFO *usecase_info = (USECASE_INFO *)ml_engine_list_find(opx_data->use_cases, (uint16)usecase_id);
    usecase_info->current_batch_count = 0;

    ML_PROPERTY *property = (ML_PROPERTY *)&usecase_id;

    /* Activate the model */
    status = ml_set_config(usecase_info->ml_model_context, ACTIVATE_USECASE, property);
    /* Throw error and exit if activating the model fails */
    if(status == FALSE)
    {
        L2_DBG_MSG("MLE: Model Activate Failed!");
        return status;
    }

    /* Set the model load status after model load and activate */
    usecase_info->model_load_status = 1;

    /* Get input tensor count */
    usecase_info->input_tensor.num_tensors = 0;
    property = (ML_PROPERTY *)&(usecase_info->input_tensor.num_tensors);
    status = ml_get_config(usecase_info->ml_model_context, INPUT_TENSOR_COUNT, property);

    /* Throw error and exit if getting the input tensor count fails */
    if(status == FALSE)
    {
        L2_DBG_MSG("MLE: Unable to fetch input tensor count!");
        return status;
    }

    /* Get input tensor info */
    if(usecase_info->input_tensor.num_tensors)
    {
        usecase_info->input_tensor.tensors = (MODEL_TENSOR_INFO *)xppmalloc(usecase_info->input_tensor.num_tensors * sizeof(MODEL_TENSOR_INFO), MALLOC_PREFERENCE_DM1);
        property = (ML_PROPERTY *)&(usecase_info->input_tensor);
        status = ml_get_config(usecase_info->ml_model_context, INPUT_TENSOR_INFO, property);
        /* Throw error and exit if getting the input tensor details fails */
        if(status == FALSE)
        {
            L2_DBG_MSG("MLE: Unable to fetch input tensor info!");
            return status;
        }
    }

    for(int i=0; i<usecase_info->input_tensor.num_tensors; i++)
    {
        MODEL_TENSOR_INFO t = usecase_info->input_tensor.tensors[i];
        L2_DBG_MSG2("MLE:tensor id %d, length %d", t.tensor_id, t.data_length);
    }

    /* Do not proceed if the number of input tensors are more than MAX_TENSORS */
    if (usecase_info->input_tensor.num_tensors > MAX_TENSORS)
    {
        status = FALSE;
        L0_DBG_MSG1("MLE: Number of input tensors more than %d are not supported",MAX_TENSORS);
        return status;
    }

    /* Get output tensor count */
    usecase_info->output_tensor.num_tensors = 0;
    property = (ML_PROPERTY *)&(usecase_info->output_tensor.num_tensors);
    status = ml_get_config(usecase_info->ml_model_context, OUTPUT_TENSOR_COUNT, property);
    /* Throw error and exit if getting the output tensor count fails */
    if(status == FALSE)
    {
        L2_DBG_MSG("MLE: Unable to fetch output tensor count!");
        return status;
    }

    /* Get output tensor info */
    if(usecase_info->output_tensor.num_tensors)
    {
        usecase_info->output_tensor.tensors = (MODEL_TENSOR_INFO *)xppmalloc(usecase_info->output_tensor.num_tensors * sizeof(MODEL_TENSOR_INFO), MALLOC_PREFERENCE_DM1);
        property = (ML_PROPERTY *)&(usecase_info->output_tensor);
        status = ml_get_config(usecase_info->ml_model_context, OUTPUT_TENSOR_INFO, property);
        /* Throw error and exit if getting the output tensor details fails */
        if(status == FALSE)
        {
            L2_DBG_MSG("MLE: Unable to fetch output tensor info!");
            return status;
        }
    }

    for(int i=0; i<usecase_info->output_tensor.num_tensors; i++)
    {
        MODEL_TENSOR_INFO t = usecase_info->output_tensor.tensors[i];
        L2_DBG_MSG2("MLE:tensor id %d, length %d", t.tensor_id, t.data_length);
    }

    return status;
}

/**
 * \brief Helper function to check the status of ML_ENGINE.
 *        This is used by other ML capabilities to check if
 *        ML_Engine is correctly instantiated.
 *
 * \return TRUE if ML_ENGINE is instantiated properly else FALSE
 */
bool ml_engine_check_status(void)
{
    return TRUE;
}
