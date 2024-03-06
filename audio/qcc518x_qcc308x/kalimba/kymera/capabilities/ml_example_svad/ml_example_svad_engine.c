/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  ml_example_svad_engine.c
 * \ingroup  capabilities
 *
 *
 *
 */

#include "ml_example_svad_cap.h"
#include "capabilities.h"

/****************************************************************************
Private Constant Declarations
*/

/**
 * \brief ml_example_svad Engine  data process function - calls model runner libs
 * \param pointer to theml_engine data container
 * \return none
 */
void ml_example_svad_run_model(ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data)
{
    ML_ENGINE_OP_DATA *ml_engine_data = ml_example_svad_data->ml_engine_container;
    USECASE_INFO *usecase_info = (USECASE_INFO *)ml_engine_list_find(ml_engine_data->use_cases, (uint16)ml_engine_data->uc_id);

    /* copy data from the tensor rolling circular buffers to the tensor linear buffers */
    for(unsigned i=0; i<usecase_info->input_tensor.num_tensors; i++)
    {
        MODEL_TENSOR_INFO tensor = usecase_info->input_tensor.tensors[i];
        cbuffer_read(tensor.cdata, (signed*)tensor.data, tensor.data_length);
        L2_DBG_MSG2("ML_EXAMPLE_SVAD_ENGINE: Copying tensor id %d of length %d from preprocessing to engine",tensor.tensor_id, tensor.data_length);
        /* move the read pointer back to start of buffer to make to trigger windowing logic
         * if configured for a tensor */
        cbuffer_move_read_to_write_point(tensor.cdata, tensor.data_length);        
    }

    /* Execute the Model */
    ml_execute(usecase_info->ml_model_context);

    return;
}

