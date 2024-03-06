/****************************************************************************
 * Copyright (c) 2021 - 2021 Qualcomm Technologies International, Ltd.
 ****************************************************************************/
/**
 * \file  dsl_ml_engine_lib.c
 * \ingroup support_lib/ml_engine_lib
 *
 */

/*****************************************************************************
Include Files
*/
#include "dsl_ml_engine_lib_if.h"

/**
 * \brief Helper function to check the status of ML_ENGINE.
 *        This is used by other ML capabilities to check if
 *        ML_Engine is correctly instantiated.
 *
 * \return TRUE if ML_ENGINE is instantiated properly else FALSE
 */
bool ml_engine_check_status(void)
{
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    if(NULL == ml_engine_lib_fn_table)
    {
        return FALSE;
    }

    if(NULL == *ml_engine_lib_fn_table)
    {
        return FALSE;
    }
    return TRUE;
}

/**
 * \brief Helper function to load the model file.
 *
 * \param opx_data ML Engine extra operator data.
 * \param usecase_id Usecase identifier for which to load model.
 * \param file_handle Kymera file handle to the downloaded model file.
 * \param auto_remove Flag to remove the file manager data.
 * \return TRUE if model load is successful else FALSE.
 */
bool ml_engine_load(ML_ENGINE_OP_DATA* opx_data, unsigned usecase_id, unsigned batch_reset_count, unsigned file_handle, unsigned auto_remove)
{
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    return (*ml_engine_lib_fn_table)->ml_engine_load(opx_data,usecase_id,batch_reset_count,file_handle,auto_remove);

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
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    return (*ml_engine_lib_fn_table)->ml_engine_activate(opx_data,usecase_id);
}

/**
 * \brief Helper function to reset the persistent tensors
 *
 * \param opx_data ML Engine extra operator data
 * \param usecase_id usecase identifier of the model
 */
void ml_engine_reset_buffer(ML_ENGINE_OP_DATA* opx_data, unsigned usecase_id)
{
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    (*ml_engine_lib_fn_table)->ml_engine_reset_buffer(opx_data, usecase_id);
}

/**
 * \brief Helper function to add usecase/tensor to linked list
 *
 * \param head head pointer of linked list
 * \param value pointer of the usecase/tensor structure to be addded
 * \param node_id tensor id/use-case id
 * \return status (TRUE/FALSE)
 */
bool ml_engine_list_add(ML_ENGINE_NODE **head, void *value, uint16 node_id)
{
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    return (*ml_engine_lib_fn_table)->ml_engine_list_add(head,value,node_id);
}

/**
 * \brief Helper function to get the usecase/tensor from linked list for a given tensor-id/usecase-id
 *
 * \param p_head head pointer of linked list
 * \param node_id id of the tensor/use-case
 *
 * \return return the pointer of the use-case/tensor structure
 */
void* ml_engine_list_find(ML_ENGINE_NODE *p_head, uint16 node_id)
{
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    return (*ml_engine_lib_fn_table)->ml_engine_list_find(p_head,node_id);
}

/**
 * \brief Deletes a node from linked-list with given node-id
 *
 * \param p_head head pointer of linked list
 * \param node_id id of the tensor/use-case
 * \param fn_free function pointer to delete the node and associated data
 *
 */
void ml_engine_delete_node(ML_ENGINE_NODE **p_head, uint16 node_id, free_ml_engine_node fn_free)
{
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    (*ml_engine_lib_fn_table)->ml_engine_delete_node(p_head,node_id,fn_free);
}

/**
 * \brief Deletes all nodes of a given linked-list
 *
 * \param p_head head pointer of linked list
 * \param fn_free function pointer to delete the node and associated data
 */
void ml_engine_delete_all_node(ML_ENGINE_NODE **p_head, free_ml_engine_node fn_free)
{
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    (*ml_engine_lib_fn_table)->ml_engine_delete_all_node(p_head,fn_free);
}

/**
 * \brief Deletes all items associated with use-case node.
 *
 * \param usecase pointer
 */
void ml_engine_free_usecase_node(void *usecase)
{
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    (*ml_engine_lib_fn_table)->ml_engine_free_usecase_node(usecase);
}

/**
 * \brief Loads a single model file to RAM.
 *
 * \param usecase_id usecase-id of the model file
 * \param file_handle the file handle returned by OperatorDataLoadEx()
 * \param access_method Access method for the model.
 * \param ml_context the ML_CONTEXT pointer provided by the caller.
 * \return TRUE if success, FALSE otherwise
 */
bool ml_load(unsigned usecase_id, unsigned file_handle, unsigned access_method, ML_CONTEXT **ml_context)
{
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    return (*ml_engine_lib_fn_table)->ml_load(usecase_id,file_handle,access_method,ml_context);
}

/**
 * \brief Frees the file manager data and removes the record for the given usecase-id
 *
 * \param usecase_id usecase-id of the model file
 * \param file_handle the file handle returned by OperatorDataLoadEx()
 * \return TRUE if success, FALSE otherwise
 */
bool ml_unload(unsigned usecase_id, unsigned file_handle)
{
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    return (*ml_engine_lib_fn_table)->ml_unload(usecase_id,file_handle);
}

/**
 * \brief Frees the context returned by ml_load()
 *
 * \param ml_context the context to be freed which was returned by ml_load()
 * \return TRUE if success, FALSE otherwise
 */
bool ml_free_context(ML_CONTEXT *ml_context)
{
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    return (*ml_engine_lib_fn_table)->ml_free_context(ml_context);
}

/**
 * \brief A common interface to set the model property.
 *
 * \param ml_context the context returned by model_loader_load_model()
 * \param property_id an identifier for the property to be set.
 * \param property structure of the property to be set.
 * \return TRUE if success, FALSE otherwise
 */
bool ml_set_config(ML_CONTEXT *ml_context, PropertyID property_id, ML_PROPERTY *property)
{
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    return (*ml_engine_lib_fn_table)->ml_set_config(ml_context,property_id,property);
}

/**
 * \brief A common interface to get the model property.
 *
 * \param ml_context the context returned by model_loader_load_model()
 * \param property_id an identifier for the property to be set.
 * \param property structure of the property to be set.
 * \return TRUE if success, FALSE otherwise
 */
bool ml_get_config(ML_CONTEXT *ml_context, PropertyID property_id, ML_PROPERTY *property)
{
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    return (*ml_engine_lib_fn_table)->ml_get_config(ml_context,property_id,property);
}

/**
 * \brief Executes the neural network model
 * \param mle_context context object containing parameters for this model
 * \return int status of inference
 */
int ml_execute(ML_CONTEXT *ml_context)
{
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    int status = (*ml_engine_lib_fn_table)->ml_execute(ml_context);
    return status;
}
