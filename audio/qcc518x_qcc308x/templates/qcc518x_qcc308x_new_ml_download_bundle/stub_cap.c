/****************************************************************************
 * Copyright (c) 2014 - 2018 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  @@@cap_name@@@.c
 * \ingroup  capabilities
 *
 *  A Stub implementation of a Capability that can be built and communicated
 *  with. This is provided to accelerate the development of new capabilities.
 *
 */


#include "@@@cap_name@@@_cap.h"
#include "capabilities.h"

/****************************************************************************
Private Function Definitions
*/
static void @@@cap_name@@@_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched);
static bool @@@cap_name@@@_struct_create(@@@cap_name^U@@@_OP_DATA *op_ext_data);
static bool @@@cap_name@@@_struct_destroy(@@@cap_name^U@@@_OP_DATA *op_ext_data);
static bool @@@cap_name@@@_validate_model(@@@cap_name^U@@@_OP_DATA *@@@cap_name@@@_data);


/****************************************************************************
Private Constant Declarations
*/
#define @@@cap_name^U@@@_CAP_ID                 @@@cap_id@@@

/** The stub capability function handler table */
const handler_lookup_struct @@@cap_name@@@_handler_table =
{
    @@@cap_name@@@_create,           /* OPCMD_CREATE */
    @@@cap_name@@@_destroy,          /* OPCMD_DESTROY */
    @@@cap_name@@@_start,            /* OPCMD_START */
    base_op_stop,             /* OPCMD_STOP */
    base_op_reset,            /* OPCMD_RESET */
    @@@cap_name@@@_connect,          /* OPCMD_CONNECT */
    @@@cap_name@@@_disconnect,       /* OPCMD_DISCONNECT */
    @@@cap_name@@@_buffer_details,   /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,  /* OPCMD_DATA_FORMAT */
    base_op_get_sched_info    /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table - this is the set of operator
 * messages that the capability understands and will attempt to service. */
const opmsg_handler_lookup_table_entry @@@cap_name@@@_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_ML_ENGINE_ID_LOAD_MODEL, @@@cap_name@@@_opmsg_load_model},
    {OPMSG_ML_ENGINE_ID_UNLOAD_MODEL, @@@cap_name@@@_opmsg_unload_model},
    {OPMSG_ML_ENGINE_ID_ACTIVATE_MODEL, @@@cap_name@@@_opmsg_activate_model},
    {0, NULL}
};


/* Capability data - This is the definition of the capability that Opmgr uses to
 * create the capability from. */
const CAPABILITY_DATA @@@cap_name@@@_cap_data =
{
    @@@cap_name^U@@@_CAP_ID,                    /* Capability ID */
    @@@cap_name^U@@@_CAP_VERSION_MAJOR,@@@cap_name^U@@@_CAP_VERSION_MINOR,    /* Version information - hi and lo parts */
    NUM_INPUTS, NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
    &@@@cap_name@@@_handler_table,            /* Pointer to message handler function table */
    @@@cap_name@@@_opmsg_handler_table,       /* Pointer to operator message handler function table */
    @@@cap_name@@@_process_data,              /* Pointer to data processing function */
    0,                                 /* TODO: this would hold processing time information */
    sizeof(@@@cap_name^U@@@_OP_DATA)            /* Size of capability-specific per-instance data */
};

MAP_INSTANCE_DATA(@@@cap_name^U@@@_CAP_ID, @@@cap_name^U@@@_OP_DATA)

/* Accessing the capability-specific per-instance data function */
static inline @@@cap_name^U@@@_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (@@@cap_name^U@@@_OP_DATA *) base_op_get_instance_data(op_data);
}


bool @@@cap_name@@@_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    @@@cap_name^U@@@_OP_DATA *@@@cap_name@@@_data = get_instance_data(op_data);

    /* call base_op create, which also allocates and fills response message */
    if (!base_op_create_lite(op_data, response_data))
    {
        return FALSE;
    }

    /* allocate memory */
    if (@@@cap_name@@@_struct_create(@@@cap_name@@@_data) == FALSE )
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        @@@cap_name@@@_struct_destroy(@@@cap_name@@@_data);
        return TRUE;
    }

    /* check if ML_ENGINE is properly instantiated */
    if(!ml_engine_check_status())
    {
        L2_DBG_MSG("ML_Base: ML_Engine is not correctly instantiated!");
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        @@@cap_name@@@_struct_destroy(@@@cap_name@@@_data);
        return TRUE;
    }

    return TRUE;
}

static bool @@@cap_name@@@_struct_create(@@@cap_name^U@@@_OP_DATA* @@@cap_name@@@_data)
{
    /* returns true if succesful, false if failed */

    /* allocate memory */

    /* allocate the Preprocess data object  */
    @@@cap_name@@@_data->pre_processing_container = (t_PREPROC_CONTAINER*)xzpmalloc(sizeof(t_PREPROC_CONTAINER));
    if (@@@cap_name@@@_data->pre_processing_container == NULL)
    {
        return FALSE;
    }

    /* allocate the ML Engine data object  */
    @@@cap_name@@@_data->ml_engine_container = (ML_ENGINE_OP_DATA*)xzpmalloc(sizeof(ML_ENGINE_OP_DATA));
    if (@@@cap_name@@@_data->ml_engine_container == NULL)
    {
        return FALSE;
    }

    /* allocate the Postprocess data object  */
    @@@cap_name@@@_data->post_processing_container = (t_POSTPROC_CONTAINER*)xzpmalloc(sizeof(t_POSTPROC_CONTAINER));
    if (@@@cap_name@@@_data->post_processing_container == NULL)
    {
        return FALSE;
    }

    /* Initialise the frames_processed */
    @@@cap_name@@@_data->ml_engine_container->frames_processed = 0;

    return TRUE;
}

static bool @@@cap_name@@@_struct_destroy(@@@cap_name^U@@@_OP_DATA* @@@cap_name@@@_data)
{
    @@@cap_name@@@_pre_processing_destroy(@@@cap_name@@@_data->pre_processing_container);
    /* free the preprocessing data object */
    pfree(@@@cap_name@@@_data->pre_processing_container );
    @@@cap_name@@@_data->pre_processing_container  = NULL;

    /* free the ml engine data object */
    pfree(@@@cap_name@@@_data->ml_engine_container );
    @@@cap_name@@@_data->ml_engine_container  = NULL;

    @@@cap_name@@@_post_processing_destroy(@@@cap_name@@@_data->post_processing_container);
     /* free the postprocessing data object */
    pfree(@@@cap_name@@@_data->post_processing_container );
    @@@cap_name@@@_data->post_processing_container  = NULL;

    return TRUE;

}

bool @@@cap_name@@@_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    @@@cap_name^U@@@_OP_DATA* @@@cap_name@@@_data = get_instance_data(op_data);
    ML_ENGINE_OP_DATA *ml_engine_data = @@@cap_name@@@_data->ml_engine_container;
    USECASE_INFO *usecase_info;

    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        return FALSE;
    }

    if (opmgr_op_is_running(op_data))
    {
        /* Operator already started nothing to do. */
        return TRUE;
    }

    /* Check if the input is connected. */
    if (@@@cap_name@@@_data->ip_buffer == NULL)
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* Do not start the operator if no model has loaded and activated */
    /* Get the status corresponding to the model */
    usecase_info = (USECASE_INFO *)ml_engine_list_find(ml_engine_data->use_cases,
                                                      (uint16)ml_engine_data->uc_id);
    if (!usecase_info->model_load_status)
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        L2_DBG_MSG("ML: Model has not loaded and activated");
        return TRUE;
    }
    return TRUE;
}

bool @@@cap_name@@@_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    @@@cap_name^U@@@_OP_DATA *@@@cap_name@@@_data = get_instance_data(op_data);
    unsigned terminal_num, terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    terminal_num = terminal_id & TERMINAL_NUM_MASK;
    tCbuffer* pterminal_buf = OPMGR_GET_OP_CONNECT_BUFFER(message_data);

    if (!base_op_connect(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }
    /* can't connect while running */
    if (opmgr_op_is_running(op_data))
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }
    if ( terminal_num >= NUM_INPUTS )
    {
        /* invalid terminal id */
        base_op_change_response_status(response_data, STATUS_INVALID_CMD_PARAMS);
        return TRUE;
    }

    /* check if the terminal is already connected and if not , connect the terminal */
    if (terminal_id & TERMINAL_SINK_MASK)
    {
        @@@cap_name@@@_data->ip_buffer = pterminal_buf;
    }
    else
    {
        @@@cap_name@@@_data->op_buffer = pterminal_buf;
    }

    return TRUE;
}

bool @@@cap_name@@@_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    @@@cap_name^U@@@_OP_DATA *@@@cap_name@@@_data = get_instance_data(op_data);
    unsigned terminal_num, terminal_id = OPMGR_GET_OP_DISCONNECT_TERMINAL_ID(message_data);
    terminal_num = terminal_id & TERMINAL_NUM_MASK;

    if (!base_op_disconnect(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }
    /* can't disconnect while running */
    if (opmgr_op_is_running(op_data))
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }
    if ( terminal_num >= NUM_INPUTS)
    {
        /* invalid terminal id */
        base_op_change_response_status(response_data, STATUS_INVALID_CMD_PARAMS);
        return TRUE;
    }

    if (terminal_id & TERMINAL_SINK_MASK)
    {
        @@@cap_name@@@_data->ip_buffer = NULL;
    }
    else
    {
        @@@cap_name@@@_data->op_buffer = NULL;
    }


    return TRUE;
}

bool @@@cap_name@@@_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    @@@cap_name^U@@@_OP_DATA *@@@cap_name@@@_data = get_instance_data(op_data);

    /* check that we are not trying to destroy a running operator */
    if (opmgr_op_is_running(op_data))
    {
        /* We can't destroy a running operator. */
        return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
    }
    else
    {
        /* set internal capability state variable to "not_created" */
        ml_engine_delete_all_node(&@@@cap_name@@@_data->ml_engine_container->use_cases, ml_engine_free_usecase_node);
        @@@cap_name@@@_struct_destroy(@@@cap_name@@@_data);
        /* call base_op destroy that creates and fills response message, too */
        return base_op_destroy_lite(op_data, response_data);
    }
}
bool @@@cap_name@@@_buffer_details(OPERATOR_DATA *op_data, void *message_data,
                              unsigned *response_id, void **resp_data)
{
    @@@cap_name^U@@@_OP_DATA *@@@cap_name@@@_data = get_instance_data(op_data);
    unsigned terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    /* Response pointer */
    OP_BUF_DETAILS_RSP *p_resp;

#ifndef DISABLE_IN_PLACE
    unsigned terminal_num = terminal_id & TERMINAL_NUM_MASK;
#endif

    /* Variables used for distinguishing source/sink */
    unsigned max_value;
    int buffer_size;
    tCbuffer* opposite_buffer;

    if (!base_op_buffer_details_lite(op_data, resp_data))
    {
        return FALSE;
    }

    p_resp = (OP_BUF_DETAILS_RSP*) *resp_data;

    buffer_size = (int)(@@@cap_name@@@_data->pre_processing_container->input_block_size * 2);
#ifdef DISABLE_IN_PLACE
    p_resp->runs_in_place = FALSE;
    p_resp->b.buffer_size = buffer_size;
#else

    /* Determine whether sink or source terminal being disconnected */
    if (terminal_id & TERMINAL_SINK_MASK)
    {
        L4_DBG_MSG1("@@@cap_name@@@ buffer details: sink buffer %d", terminal_num);
        max_value = NUM_INPUTS;
        opposite_buffer = @@@cap_name@@@_data->op_buffer;
    }
    else
    {
        L4_DBG_MSG1("@@@cap_name@@@ buffer details: source buffer %d", terminal_num);
        max_value = NUM_OUTPUTS;
        opposite_buffer = @@@cap_name@@@_data->ip_buffer;
        buffer_size = (int)(@@@cap_name@@@_data->post_processing_container->output_block_size * 2);
    }

    /* Can't use invalid ID */
    if (terminal_num >= max_value)
    {
        /* invalid terminal id */
        L4_DBG_MSG1("@@@cap_name@@@ buffer details failed: invalid terminal %d",
                    terminal_num);
        base_op_change_response_status(resp_data, STATUS_INVALID_CMD_PARAMS);
        return TRUE;
    }

    if (opposite_buffer != NULL)
    {
        p_resp->runs_in_place = TRUE;
        p_resp->b.in_place_buff_params.in_place_terminal = \
            terminal_id ^ TERMINAL_SINK_MASK;
        p_resp->b.in_place_buff_params.size = buffer_size;
        p_resp->b.in_place_buff_params.buffer = \
            opposite_buffer;
        L4_DBG_MSG1("@@@cap_name@@@ buffer_details: %d", p_resp->b.buffer_size);
    }
    else
    {
        p_resp->runs_in_place = FALSE;
        p_resp->b.buffer_size = buffer_size;
    }
    p_resp->supports_metadata = TRUE;


#endif /* DISABLE_IN_PLACE */
    return TRUE;
}

static bool @@@cap_name@@@_validate_model(@@@cap_name^U@@@_OP_DATA *@@@cap_name@@@_data)
{
    /* Compare capability tensor definitions to model tensor definitions */
    ML_ENGINE_OP_DATA *ml_engine_data = @@@cap_name@@@_data->ml_engine_container;
    USECASE_INFO *usecase_info = (USECASE_INFO *)ml_engine_list_find(ml_engine_data->use_cases, (uint16)ml_engine_data->uc_id);

    if (usecase_info->input_tensor.num_tensors != @@@cap_name@@@_data->pre_processing_container->num_tensors)
    {
        L2_DBG_MSG2("ML: Num Input Tensor mismatch Model: %d, Preproc: %d - Invalid Model",usecase_info->input_tensor.num_tensors,@@@cap_name@@@_data->pre_processing_container->num_tensors);
        return FALSE;
    }
    if (usecase_info->output_tensor.num_tensors !=  @@@cap_name@@@_data->post_processing_container->num_tensors)
    {
       L2_DBG_MSG2("ML: Num Output Tensor mismatch Model: %d, Postproc: %d - Invalid Model",usecase_info->output_tensor.num_tensors,@@@cap_name@@@_data->post_processing_container->num_tensors);
        return FALSE;
    }
    for (int i = 0; i < @@@cap_name@@@_data->pre_processing_container->num_tensors; i++)
    {
        ALGO_OUTPUT_INFO algo_output =@@@cap_name@@@_data->pre_processing_container->algo_output[i];
        if (usecase_info->input_tensor.tensors[i].data_length != algo_output.size)
        {
            L2_DBG_MSG2("ML:  Size Input Tensor mismatch Model: %d, Preproc: %d- Invalid Model", usecase_info->input_tensor.tensors[i].data_length, algo_output.size);
            return FALSE;
        }

    }
    for ( int i = 0; i <  @@@cap_name@@@_data->post_processing_container->num_tensors; i++)
    {
        ALGO_INPUT_INFO algo_input =@@@cap_name@@@_data->post_processing_container->algo_input[i];
        if (usecase_info->output_tensor.tensors[i].data_length !=algo_input.size)
        {
            L2_DBG_MSG2("ML: Size Output Tensor mismatch Model: %d, Postproc: %d- Invalid Model", usecase_info->output_tensor.tensors[i].data_length, algo_input.size);
            return FALSE;
        }

    }
    L2_DBG_MSG("ML: model validated");
    return TRUE;
}

/* Data processing function */
static void @@@cap_name@@@_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{


    /*
     * TODDO Capability data processing code goes here ...
     */
    @@@cap_name^U@@@_OP_DATA *@@@cap_name@@@_data = get_instance_data(op_data);
    ML_ENGINE_OP_DATA *ml_engine_data = @@@cap_name@@@_data->ml_engine_container;
    USECASE_INFO *usecase_info = (USECASE_INFO *)ml_engine_list_find(ml_engine_data->use_cases, (uint16)ml_engine_data->uc_id);
    unsigned available_space = 0, available_data;

    /* All buffers must be connected*/
    if( (@@@cap_name@@@_data->ip_buffer == NULL) || (@@@cap_name@@@_data->op_buffer == NULL) )
    {
        L2_DBG_MSG("@@@cap_name^U@@@:buffer not connected");
       return;
    }

    /* number of samples to process at input buffer    */
    available_data = cbuffer_calc_amount_data_in_words(@@@cap_name@@@_data->ip_buffer);
    /* Check for sufficient data at the input buffer*/
    if(available_data < @@@cap_name@@@_data->pre_processing_container->input_block_size)
    {
        L2_DBG_MSG("@@@cap_name^U@@@:not enough data");
        return;
    }
    /* Check for sufficient space at the output buffer*/
    available_space = cbuffer_calc_amount_space_in_words(@@@cap_name@@@_data->op_buffer);
    if(available_space < @@@cap_name@@@_data->post_processing_container->output_block_size)
    {
        L2_DBG_MSG("@@@cap_name^U@@@:not enough space");
        return;
    }


    /*************************************************************************
     *                         PREPROCESSING
     *
     * +------------+                      +==========+
     * |            |     +----------+     |          |     +-----------+
     * | capability | --> |  linear  | --> | pre      | --> | ip tensor |
     * | input      |     |  buffer  |     | process  |     | buffer    |
     * | buffer     |     +----------+     |          |     +-----------+
     * +------------+                      +==========+
     *
     *************************************************************************
     */

    /* Perform Pre-procesing */
    @@@cap_name@@@_pre_process(@@@cap_name@@@_data);

    /***********************************************************
     *                         RUN MODEL
     *
     *                        +==========+
     *      +-----------+     |          |     +-----------+
     *      | ip tensor | --> |   run    | --> | op tensor |
     *      | buffer    |     |   model  |     | buffer    |
     *      +-----------+     |          |     +-----------+
     *                        +==========+
     *
     ***********************************************************
     */

    /* Run ML model */
    @@@cap_name@@@_run_model(@@@cap_name@@@_data);

    /* Increment the batch execution count */
    usecase_info->current_batch_count++;

    /************************************************************************
     *                         POSTPROCESSING
     *
     *                   +==========+                       +------------+
     * +-----------+     |          |      +----------+     |            |
     * | op tensor | --> | post     | -->  |  linear  | --> | capability |
     * | buffer    |     | process  |      |  buffer  |     | output     |
     * +-----------+     |          |      +----------+     | buffer     |
     *                   +==========+                       +------------+
     *
     ************************************************************************
     */

    /* Perform Post-processing */
    @@@cap_name@@@_post_process(@@@cap_name@@@_data);



    /* If required reset the persistent tensors */
    if (usecase_info->batch_reset_count && usecase_info->current_batch_count == usecase_info->batch_reset_count)
    {
        ml_engine_reset_buffer(ml_engine_data, usecase_info->usecase_id);
    }

    /*  Copy metadata from input terminal to output terminal */
    /*  metadata_strict_transport can handle an output buffer not being connected
        and it can handle in-place processing */
    metadata_strict_transport(@@@cap_name@@@_data->ip_buffer,
                                    @@@cap_name@@@_data->op_buffer,
                                    @@@cap_name@@@_data->pre_processing_container->input_block_size * OCTETS_PER_SAMPLE);

    /* touched input */
    touched->sinks = TOUCHED_SINK_0 ;

    if( @@@cap_name@@@_data->ip_buffer != NULL  )
    {
        touched->sources = TOUCHED_SOURCE_0;
    }
}
/****************************************************************************
OPMSG Handlers
*/

/**
 * \brief Loads the MLE model file
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the destroy request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool @@@cap_name@@@_opmsg_load_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* Model cannot be loaded if the operator is running */
    if (opmgr_op_is_running(op_data))
    {
        L0_DBG_MSG("MLE: Cannot load model while operator is running");
        return FALSE;
    }
    unsigned usecase_id  = OPMSG_FIELD_GET(message_data, OPMSG_ML_ENGINE_LOAD_MODEL, USECASE_ID);
    unsigned batch_reset_count = OPMSG_FIELD_GET(message_data, OPMSG_ML_ENGINE_LOAD_MODEL, BATCH_RESET_COUNT);
    unsigned file_handle = OPMSG_FIELD_GET(message_data, OPMSG_ML_ENGINE_LOAD_MODEL, FILE_HANDLE);
    unsigned access_method = OPMSG_FIELD_GET(message_data, OPMSG_ML_ENGINE_LOAD_MODEL, ACCESS_METHOD);

    @@@cap_name^U@@@_OP_DATA *@@@cap_name@@@_data = get_instance_data(op_data);
    ML_ENGINE_OP_DATA *ml_engine_data = @@@cap_name@@@_data->ml_engine_container;

    /* Call helper function to load the model */
    return ml_engine_load(ml_engine_data, usecase_id, batch_reset_count, file_handle, access_method);
}

/**
 * \brief Unloads the MLE model file and releases associated memory
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the destroy request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool @@@cap_name@@@_opmsg_unload_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* Model cannot be unloaded if the operator is running */
    if (opmgr_op_is_running(op_data))
    {
        L0_DBG_MSG("MLE: Cannot unload model while operator is running");
        return FALSE;
    }

    /* get the usecase_id of the model to be unloaded from the opmsg */
    unsigned usecase_id = OPMSG_FIELD_GET(message_data, OPMSG_ML_ENGINE_UNLOAD_MODEL, USECASE_ID);
    unsigned file_id    = OPMSG_FIELD_GET(message_data, OPMSG_ML_ENGINE_UNLOAD_MODEL, FILE_ID);
    /* unload the model file - this will direct the file_mgr to release the file handle */
    return ml_unload(usecase_id, file_id);
}

/**
 * \brief Activates the MLE model file associated with the usecase_id
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the destroy request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool @@@cap_name@@@_opmsg_activate_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* currently not supported while operator is running */
    if (opmgr_op_is_running(op_data))
    {
        L0_DBG_MSG("MLE: Cannot activate model while operator is running  \n");
        return FALSE;
    }

    @@@cap_name^U@@@_OP_DATA *@@@cap_name@@@_data = get_instance_data(op_data);
    ML_ENGINE_OP_DATA *ml_engine_data = @@@cap_name@@@_data->ml_engine_container;
    /* get the usecase_id from the operator message */
    unsigned usecase_id = OPMSG_FIELD_GET(message_data, OPMSG_ML_ENGINE_ACTIVATE_MODEL, USECASE_ID);

    /* Call helper function to activate the model */
    if (!ml_engine_activate(ml_engine_data, usecase_id))
    {
        return FALSE;
    }

    USECASE_INFO *usecase_info = (USECASE_INFO *)ml_engine_list_find(@@@cap_name@@@_data->ml_engine_container->use_cases,(uint16)@@@cap_name@@@_data->ml_engine_container->uc_id);
    for(int i=0; i<usecase_info->input_tensor.num_tensors; i++)
    {
        MODEL_TENSOR_INFO *tensor = &usecase_info->input_tensor.tensors[i];
        L2_DBG_MSG2("Input Tensor[%d] Data  pts: %d",i , tensor->data);

    }
    for(int i=0; i<usecase_info->input_tensor.num_tensors; i++)
    {
        MODEL_TENSOR_INFO *tensor = &usecase_info->output_tensor.tensors[i];
        L2_DBG_MSG2("Output Tensor[%d] Data  pts: %d",i , tensor->data);

    }
   /* Connect the pre and post processing directly to the input and output tensors */

    /* Initialise post-processing */
    @@@cap_name@@@_post_processing_create(@@@cap_name@@@_data);
    if (@@@cap_name@@@_data->post_processing_container->passthrough_data == NULL)
    {
        L0_DBG_MSG("POST PROC failed to create \n");
    }


    /* Initialise pre-processing */
    @@@cap_name@@@_pre_processing_create(@@@cap_name@@@_data);
    if (@@@cap_name@@@_data->pre_processing_container->passthrough_data == NULL)
    {
       L0_DBG_MSG("PRE PROC failed to create \n");
    }
    /* Validate ml model */
    /* Compare number and length of input and output tensors to what the capability expects.
        If we are not activating the correct model, fail */
    if (!@@@cap_name@@@_validate_model(@@@cap_name@@@_data))
    {
        L0_DBG_MSG("MLE: Unexpected Model Loaded \n");
        return FALSE;
    }

    return TRUE;
}


