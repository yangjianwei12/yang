/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  download_ml_engine_lib_cap.c
 * \ingroup  capabilities
 *
 * Download ML Engine Lib Capability
 *
 */

/****************************************************************************
Include Files
*/
#include "download_ml_engine_lib_cap.h"

/****************************************************************************
Private Type Definitions
*/
#ifdef CAPABILITY_DOWNLOAD_BUILD
#define DOWNLOAD_ML_ENGINE_LIB_CAP_ID                 CAP_ID_DOWNLOAD_ML_ENGINE
#else
#error "DOWNLOAD_ML_ENGINE_LIB capability is only supported as a downloadable"
#endif

/****************************************************************************
Private Constant/functions Declarations
*/
static void download_ml_engine_lib_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched);
static bool download_ml_engine_lib_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
static bool download_ml_engine_lib_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/** The DOWNLOAD_ML_ENGINE_LIB capability function handler table */
const handler_lookup_struct download_ml_engine_lib_handler_table =
{
    download_ml_engine_lib_create,          /* OPCMD_CREATE  */
    download_ml_engine_lib_destroy,         /* OPCMD_DESTROY */
    base_op_start,                          /* OPCMD_START   */
    base_op_stop,                           /* OPCMD_STOP    */
    base_op_reset                           /* OPCMD_RESET   */
};

/* Null terminated operator message handler table - this is the set of operator
 * messages that the capability understands and will attempt to service. */
const opmsg_handler_lookup_table_entry download_ml_engine_lib_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,             base_op_opmsg_get_capability_version},
    {0, NULL}
};

/* Capability data - This is the definition of the capability that Opmgr uses to
 * create the capability from. */
const CAPABILITY_DATA download_ml_engine_lib_cap_data =
{
    DOWNLOAD_ML_ENGINE_LIB_CAP_ID,                       /* Capability ID */
    DOWNLOAD_ML_ENGINE_LIB_MAOR2_VERSION_MAJOR,          /* Version information - hi and lo parts */
    DOWNLOAD_ML_ENGINE_LIB_MAOR2_VERSION_MINOR,
    DOWNLOAD_ML_ENGINE_LIB_MAX_INPUT_TERMINALS,          /* Max number of sinks/inputs */
    DOWNLOAD_ML_ENGINE_LIB_MAX_OUTPUT_TERMINALS,         /* Max number of sources/outputs */
    &download_ml_engine_lib_handler_table,               /* Pointer to message handler function table */
    download_ml_engine_lib_opmsg_handler_table,          /* Pointer to operator message handler function table */
    download_ml_engine_lib_process_data,                 /* Pointer to data processing function */
    0,
    sizeof(DOWNLOAD_ML_ENGINE_LIB_OP_DATA),              /* Size of capability-specific per-instance data */
};

MAP_INSTANCE_DATA(DOWNLOAD_ML_ENGINE_LIB_CAP_ID, DOWNLOAD_ML_ENGINE_LIB_OP_DATA)

/****************************************************************************
Private Function Definitions
*/

/* Accessing the capability-specific per-instance data function */
static inline DOWNLOAD_ML_ENGINE_LIB_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (DOWNLOAD_ML_ENGINE_LIB_OP_DATA *) base_op_get_instance_data(op_data);
}

/****************************************************************************
Capability Function Handlers
*/

/**
 * \brief Initialises the DOWNLOAD_ML_ENGINE_LIB capability.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the create request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
static bool download_ml_engine_lib_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    /* call base_op create, which also allocates and fills response message */
    if (!base_op_create_lite(op_data, response_data))
    {
        return FALSE;
    }
    /* Get the patch's pointer to the function table */
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    if(NULL == ml_engine_lib_fn_table)
    {
        L2_DBG_MSG("Download MLEngineLib: unable to hook into the patch bundle");
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* Verify that it is not in use (in that case, another Download MLEngineLib is active) */
    if(NULL != *ml_engine_lib_fn_table)
    {
        L2_DBG_MSG("Download MLEngineLib: already installed - only one instance is allowed");
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* Initialise the patch function table */
    DOWNLOAD_ML_ENGINE_LIB_OP_DATA *op_ext_data = get_instance_data(op_data);

    op_ext_data->ml_engine_lib_table.ml_engine_load                   = ml_engine_load;
    op_ext_data->ml_engine_lib_table.ml_engine_activate               = ml_engine_activate;
    op_ext_data->ml_engine_lib_table.ml_engine_reset_buffer           = ml_engine_reset_buffer;
    op_ext_data->ml_engine_lib_table.ml_engine_check_status           = ml_engine_check_status;

    op_ext_data->ml_engine_lib_table.ml_load                          = ml_load;
    op_ext_data->ml_engine_lib_table.ml_unload                        = ml_unload;
    op_ext_data->ml_engine_lib_table.ml_free_context                  = ml_free_context;
    op_ext_data->ml_engine_lib_table.ml_set_config                    = ml_set_config;
    op_ext_data->ml_engine_lib_table.ml_get_config                    = ml_get_config;
    op_ext_data->ml_engine_lib_table.ml_execute                       = ml_execute;

    op_ext_data->ml_engine_lib_table.ml_engine_list_add               = ml_engine_list_add;
    op_ext_data->ml_engine_lib_table.ml_engine_list_find              = ml_engine_list_find;
    op_ext_data->ml_engine_lib_table.ml_engine_delete_node            = ml_engine_delete_node;
    op_ext_data->ml_engine_lib_table.ml_engine_delete_all_node        = ml_engine_delete_all_node;
    op_ext_data->ml_engine_lib_table.ml_engine_free_usecase_node      = ml_engine_free_usecase_node;

    *ml_engine_lib_fn_table = &op_ext_data->ml_engine_lib_table;
    return TRUE;
}

/**
 * \brief De-allocates the DOWNLOAD_ML_ENGINE_LIB specific capability memory.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the create request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
static bool download_ml_engine_lib_destroy (OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    /* call base_op_destroy_lite which also allocates and fills response data */
    if(!base_op_destroy_lite(op_data, response_data))
    {
        return FALSE;
    }

    /* Get the patch's pointer to the function table */
    DOWNLOAD_ML_ENGINE_LIB_TABLE **ml_engine_lib_fn_table = dsl_ml_engine_lib_get_fn_table();
    if(NULL == ml_engine_lib_fn_table)
    {
        L2_DBG_MSG("Download MLEngineLib: unable to hook into the patch bundle");
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* Verify that it is in use (in that case, Download MLEngineLib is not active) */
    if(NULL == *ml_engine_lib_fn_table)
    {
        L2_DBG_MSG("Download MLEngineLib: no instance present ");
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    *ml_engine_lib_fn_table = NULL;
    return TRUE;
}

/****************************************************************************
Capability Data Processing function
*/

static void download_ml_engine_lib_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    /* dummy function - does nothing */
    return;
}
