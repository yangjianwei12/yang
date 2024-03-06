/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  @@@cap_name@@@_engine.c
 * \ingroup  capabilities
 *
 *
 *
 */

#include "@@@cap_name@@@_cap.h"
#include "capabilities.h"
#include "ml_struct.h"

/****************************************************************************
Private Constant Declarations
*/

/**
 * \brief @@@cap_name@@@ Engine  data process function - calls model runner libs
 * \param pointer to theml_engine data container
 * \return none
 */
void @@@cap_name@@@_run_model(@@@cap_name^U@@@_OP_DATA *@@@cap_name@@@_data)
{
    ML_ENGINE_OP_DATA *ml_engine_data = @@@cap_name@@@_data->ml_engine_container;
    USECASE_INFO *usecase_info = (USECASE_INFO *)ml_engine_list_find(ml_engine_data->use_cases, (uint16)ml_engine_data->uc_id);
    
    /* Now execute the model */
    ml_execute(usecase_info->ml_model_context);
    return;
}

