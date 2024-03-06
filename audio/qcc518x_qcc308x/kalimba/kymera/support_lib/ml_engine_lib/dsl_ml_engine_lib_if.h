/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  dsl_ml_engine_lib_if.h
 * \ingroup support_lib/ml_engine_lib
 *
 * MLEngine support lib interface file. <br>
 *
 */
#ifndef _DSL_ML_ENGINE_LIB_IF_H
#define _DSL_ML_ENGINE_LIB_IF_H

/*****************************************************************************
Include Files
*/
#include "ml_engine.h"
#include "ml_engine_usecase_manager.h"

/*****************************************************************************
Function pointer types of all the public functions exposed by the ML Framework
*/

typedef bool                    (*ml_engine_load_t)(
                                 ML_ENGINE_OP_DATA* ml_engine_data,
                                 unsigned usecase_id,
                                 unsigned batch_reset_count,
                                 unsigned file_handle,
                                 unsigned auto_remove);

typedef bool                    (*ml_engine_activate_t)(
                                 ML_ENGINE_OP_DATA* ml_engine_data,
                                 unsigned usecase_id);
 
typedef void                    (*ml_engine_reset_buffer_t)(
                                 ML_ENGINE_OP_DATA* ml_engine_data,
                                 unsigned usecase_id);

typedef bool                    (*ml_engine_check_status_t)(void);

typedef bool                    (*ml_load_t)(
                                 unsigned usecase_id, unsigned file_handle, unsigned access_method, ML_CONTEXT **ml_context);

typedef bool                    (*ml_unload_t)(
                                 unsigned usecase_id, unsigned file_handle);

typedef bool                    (*ml_free_context_t)(
                                 ML_CONTEXT *ml_context);

typedef bool                    (*ml_set_config_t)(
                                 ML_CONTEXT *ml_context, PropertyID property_id, ML_PROPERTY *property);

typedef bool                    (*ml_get_config_t)(
                                 ML_CONTEXT *ml_context, PropertyID property_id, ML_PROPERTY *property);
 
typedef int                     (*ml_engine_execute_t)(
                                 ML_CONTEXT* context);

typedef bool                    (*ml_engine_list_add_t)(
                                 ML_ENGINE_NODE **head, void *value, uint16 node_id);

typedef void*                   (*ml_engine_list_find_t)(
                                 ML_ENGINE_NODE *p_head, uint16 node_id);

typedef void                    (*ml_engine_delete_node_t)(
                                 ML_ENGINE_NODE **p_head, uint16 node_id, free_ml_engine_node fn_free);

typedef void                    (*ml_engine_delete_all_node_t)(
                                 ML_ENGINE_NODE **p_head, free_ml_engine_node fn_free);

typedef void                    (*ml_engine_free_usecase_node_t)(
                                 void *tensor);

/*****************************************************************************
ML Framework public function pointer table
*/ 
typedef struct DOWNLOAD_ML_ENGINE_LIB_TABLE
{
    ml_engine_load_t                     ml_engine_load;
    ml_engine_activate_t                 ml_engine_activate;
    ml_engine_reset_buffer_t             ml_engine_reset_buffer;
    ml_engine_check_status_t             ml_engine_check_status;
    
    ml_load_t                            ml_load;
    ml_unload_t                          ml_unload;
    ml_free_context_t                    ml_free_context;
    ml_set_config_t                      ml_set_config;
    ml_get_config_t                      ml_get_config;
    ml_engine_execute_t                  ml_execute;

    ml_engine_list_add_t                 ml_engine_list_add;
    ml_engine_list_find_t                ml_engine_list_find;
    ml_engine_delete_node_t              ml_engine_delete_node;
    ml_engine_delete_all_node_t          ml_engine_delete_all_node;
    ml_engine_free_usecase_node_t        ml_engine_free_usecase_node;
} DOWNLOAD_ML_ENGINE_LIB_TABLE;

/*****************************************************************************
Public functions
*/ 
extern DOWNLOAD_ML_ENGINE_LIB_TABLE **dsl_ml_engine_lib_get_fn_table(void);

#endif /* _DSL_ML_ENGINE_LIB_IF_H */
