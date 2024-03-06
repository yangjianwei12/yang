/****************************************************************************
 * Copyright (c) 2014 - 2018 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  ml_example_svad.c
 * \ingroup  capabilities
 *
 *  ML SVAD Capability
 *
 */


#include "ml_example_svad_cap.h"
#include "capabilities.h"

/****************************************************************************
Private Function Definitions
*/
static void ml_example_svad_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched);
static bool ml_example_svad_struct_create(ML_EXAMPLE_SVAD_OP_DATA *op_ext_data);
static bool ml_example_svad_struct_destroy(ML_EXAMPLE_SVAD_OP_DATA *op_ext_data);


/****************************************************************************
Private Constant Declarations
*/
#ifdef CAPABILITY_DOWNLOAD_BUILD
#define ML_EXAMPLE_SVAD_CAP_ID                 CAP_ID_DOWNLOAD_ML_EXAMPLE_SVAD
#else /* CAPABILITY_DOWNLOAD_BUILD */
#define ML_EXAMPLE_SVAD_CAP_ID                 CAP_ID_ML_EXAMPLE_SVAD
#endif  /* CAPABILITY_DOWNLOAD_BUILD */

/** The stub capability function handler table */
const handler_lookup_struct ml_example_svad_handler_table =
{
    ml_example_svad_create,           /* OPCMD_CREATE */
    ml_example_svad_destroy,          /* OPCMD_DESTROY */
    ml_example_svad_start,            /* OPCMD_START */
    base_op_stop,                     /* OPCMD_STOP */
    base_op_reset,                    /* OPCMD_RESET */
    ml_example_svad_connect,          /* OPCMD_CONNECT */
    ml_example_svad_disconnect,       /* OPCMD_DISCONNECT */
    ml_example_svad_buffer_details,   /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,          /* OPCMD_DATA_FORMAT */
    base_op_get_sched_info            /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table - this is the set of operator
 * messages that the capability understands and will attempt to service. */
const opmsg_handler_lookup_table_entry ml_example_svad_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_ML_ENGINE_ID_LOAD_MODEL, ml_example_svad_opmsg_load_model},
    {OPMSG_ML_ENGINE_ID_UNLOAD_MODEL, ml_example_svad_opmsg_unload_model},
    {OPMSG_ML_ENGINE_ID_ACTIVATE_MODEL, ml_example_svad_opmsg_activate_model},
    {0, NULL}
};


/* Capability data - This is the definition of the capability that Opmgr uses to
 * create the capability from. */
const CAPABILITY_DATA ml_example_svad_cap_data =
{
    ML_EXAMPLE_SVAD_CAP_ID,                                              /* Capability ID */
    ML_EXAMPLE_SVAD_CAP_VERSION_MAJOR,ML_EXAMPLE_SVAD_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
    NUM_INPUTS, NUM_OUTPUTS,                                             /* Max number of sinks/inputs and sources/outputs */
    &ml_example_svad_handler_table,                                      /* Pointer to message handler function table */
    ml_example_svad_opmsg_handler_table,                                 /* Pointer to operator message handler function table */
    ml_example_svad_process_data,                                        /* Pointer to data processing function */
    0,                                                                   /* TODO: this would hold processing time information */
    sizeof(ML_EXAMPLE_SVAD_OP_DATA)                                      /* Size of capability-specific per-instance data */
};

MAP_INSTANCE_DATA(ML_EXAMPLE_SVAD_CAP_ID, ML_EXAMPLE_SVAD_OP_DATA)

/****************************************************************************
Private Function Definitions
*/

/* Accessing the capability-specific per-instance data function */
static inline ML_EXAMPLE_SVAD_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (ML_EXAMPLE_SVAD_OP_DATA *) base_op_get_instance_data(op_data);
}

static bool ml_example_svad_struct_create(ML_EXAMPLE_SVAD_OP_DATA* ml_example_svad_data)
{
    /* returns true if succesful, false if failed */

    /* allocate memory */

    /* allocate the Preprocess data object  */
    ml_example_svad_data->pre_processing_container = (t_PREPROC_CONTAINER*)xzpmalloc(sizeof(t_PREPROC_CONTAINER));
    if (ml_example_svad_data->pre_processing_container == NULL)
    {
        return FALSE;
    }

    /* allocate the ML Engine data object  */
    ml_example_svad_data->ml_engine_container = (ML_ENGINE_OP_DATA*)xzpmalloc(sizeof(ML_ENGINE_OP_DATA));
    if (ml_example_svad_data->ml_engine_container == NULL)
    {
        return FALSE;
    }

    /* allocate the Postprocess data object  */
    ml_example_svad_data->post_processing_container = (t_POSTPROC_CONTAINER*)xzpmalloc(sizeof(t_POSTPROC_CONTAINER));
    if (ml_example_svad_data->post_processing_container == NULL)
    {
        return FALSE;
    }

    /* Initialize extended data for operator. */
    /* Initialise pre-preprocessing container. Some fields are also initialised in ml_example_svad_init_pre_process() */
    ml_example_svad_data->pre_processing_container->input_sample_rate = ML_EXAMPLE_SVAD_INPUT_SAMPLE_RATE;
    ml_example_svad_data->pre_processing_container->input_block_size = (unsigned)(ML_EXAMPLE_SVAD_BLOCK_SIZE_SEC * ML_EXAMPLE_SVAD_INPUT_SAMPLE_RATE);
    ml_example_svad_data->pre_processing_container->input_roll_size = (unsigned)(ML_EXAMPLE_SVAD_ROLL_OVER_SIZE_SEC * ML_EXAMPLE_SVAD_INPUT_SAMPLE_RATE);
    ml_example_svad_data->pre_processing_container->tensor_formation_complete = FALSE;

    /* Initialise post-preprocessing container. Some fields are also initialised in ml_example_svad_init_post_process() */
    ml_example_svad_data->post_processing_container->output_sample_rate = ML_EXAMPLE_SVAD_OUTPUT_SAMPLE_RATE;
    ml_example_svad_data->post_processing_container->output_block_size = ML_EXAMPLE_SVAD_OUTPUT_TENSOR_SIZE;

   /* ml_example_svad_data->ml_engine_container->model_loaded = TRUE; */

    /* Initialise the frames_processed */
    ml_example_svad_data->ml_engine_container->frames_processed = 0;
    return TRUE;
}

static bool ml_example_svad_struct_destroy(ML_EXAMPLE_SVAD_OP_DATA* ml_example_svad_data)
{

    /* free the preprocessing data object */
    pfree(ml_example_svad_data->pre_processing_container );
    ml_example_svad_data->pre_processing_container  = NULL;

    /* free the ml engine data object */
    pfree(ml_example_svad_data->ml_engine_container );
    ml_example_svad_data->ml_engine_container  = NULL;

     /* free the postprocessing data object */
    pfree(ml_example_svad_data->post_processing_container );
    ml_example_svad_data->post_processing_container  = NULL;

    return TRUE;

}

/****************************************************************************
Capability Function Handlers
*/

/**
 * \brief Initialises the ML_EXAMPLE_SVAD capability.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the create request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool ml_example_svad_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data = get_instance_data(op_data);

    /* call base_op create, which also allocates and fills response message */
    if (!base_op_create_lite(op_data, response_data))
    {
        return FALSE;
    }

    /* allocate memory */
    if (ml_example_svad_struct_create(ml_example_svad_data) == FALSE )
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        ml_example_svad_struct_destroy(ml_example_svad_data);
        return TRUE;
    }

    /* check if ML_ENGINE is properly instantiated */
    if(!ml_engine_check_status())
    {
        L2_DBG_MSG("ML_EXAMPLE_SVAD: ML_Engine is not correctly instantiated!");
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        ml_example_svad_struct_destroy(ml_example_svad_data);
        return TRUE;
    }

    return TRUE;
}

/**
 * \brief Starts the ML_EXAMPLE_SVAD operator
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the data format request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool ml_example_svad_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    ML_EXAMPLE_SVAD_OP_DATA* ml_example_svad_data = get_instance_data(op_data);
    ML_ENGINE_OP_DATA *ml_engine_data = ml_example_svad_data->ml_engine_container;
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
    if (ml_example_svad_data->ip_buffer == NULL)
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* Do not start the operator if no model has loaded and activated */
    /* get the status corresponding to the model */
    usecase_info = (USECASE_INFO*)ml_engine_list_find(ml_engine_data->use_cases,
                                                     (uint16)ml_engine_data->uc_id);
    if (!usecase_info->model_load_status)
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        L2_DBG_MSG("ML_EXAMPLE_SVAD: Model has not loaded and activated");
        return TRUE;
    }

    return TRUE;
}

/**
 * \brief Connects an ML_EXAMPLE_SVAD instance with endpoints
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the create request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool ml_example_svad_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data = get_instance_data(op_data);
    unsigned terminal_num, terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    /* Only an input(SINK) terminal connection is necessary for ML_EXAMPLE_SVAD */
    if (!(terminal_id & TERMINAL_SINK_MASK))
    {
        L2_DBG_MSG("ML_EXAMPLE_SVAD: Only input terminal is supported");
        return FALSE;
    }
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
    ml_example_svad_data->ip_buffer = pterminal_buf;
    return TRUE;
}

/**
 * \brief Disconnects an ML_EXAMPLE_SVAD instance from endpoints
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the create request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool ml_example_svad_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data = get_instance_data(op_data);
    unsigned terminal_num, terminal_id = OPMGR_GET_OP_DISCONNECT_TERMINAL_ID(message_data);

    /* Only an input(SINK) terminal connection is necessary for ML_EXAMPLE_SVAD */
    if (!(terminal_id & TERMINAL_SINK_MASK))
    {
        L2_DBG_MSG("ML_EXAMPLE_SVAD: Trying to disconnect an invalid terminal");
        return FALSE;
    }
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
    ml_example_svad_data->ip_buffer = NULL;
    return TRUE;
}

/**
 * \brief De-allocates the ML_EXAMPLE_SVAD specific capability memory.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the create request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool ml_example_svad_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data = get_instance_data(op_data);

    /* check that we are not trying to destroy a running operator */
    if (opmgr_op_is_running(op_data))
    {
        /* We can't destroy a running operator. */
        return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
    }
    else
    {
        /* Free the preprocessing data structures and memory */
        unsigned usecase_id = ml_example_svad_data->ml_engine_container->uc_id;
        ml_example_svad_deinit_pre_process(ml_example_svad_data, usecase_id);

        /* Free the post-processing data structures and memory */
        ml_example_svad_deinit_post_process(ml_example_svad_data);

        /* set internal capability state variable to "not_created" */
        ml_engine_delete_all_node(&ml_example_svad_data->ml_engine_container->use_cases, ml_engine_free_usecase_node);
        ml_example_svad_struct_destroy(ml_example_svad_data);

        /* call base_op destroy that creates and fills response message, too */
        return base_op_destroy_lite(op_data, response_data);
    }
}

/**
 * \brief Reports the buffer requirements of the ML_EXAMPLE_SVAD capability.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the create request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool ml_example_svad_buffer_details(OPERATOR_DATA *op_data, void *message_data,
                              unsigned *response_id, void **resp_data)
{
    ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data = get_instance_data(op_data);
    unsigned terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    unsigned terminal_num = terminal_id & TERMINAL_NUM_MASK;

    /* Response pointer */
    OP_BUF_DETAILS_RSP *p_resp;

    int buffer_size;

    if (!base_op_buffer_details_lite(op_data, resp_data))
    {
        return FALSE;
    }

    p_resp = (OP_BUF_DETAILS_RSP*) *resp_data;

    buffer_size = (int)(ml_example_svad_data->pre_processing_container->input_block_size * 2);

    /* Only sink terminal can be connected - If we try to connect a source terminal */
    /* it will fail in connect - this serves just as a sanity check */
    if (terminal_id & TERMINAL_SINK_MASK)
    {
        L4_DBG_MSG1("ml_example_svad buffer details: sink buffer %d", terminal_num);
    }

    /* Can't use invalid ID */
    if (terminal_num >= NUM_INPUTS)
    {
        /* invalid terminal id */
        L4_DBG_MSG1("ml_example_svad buffer details failed: invalid terminal %d",
                    terminal_num);
        base_op_change_response_status(resp_data, STATUS_INVALID_CMD_PARAMS);
        return TRUE;
    }

    p_resp->runs_in_place = FALSE;
    p_resp->b.buffer_size = buffer_size;
    p_resp->supports_metadata = TRUE;

    return TRUE;
}


/****************************************************************************
Capability Data Processing function
*/

/**
 * \brief process function for ML_EXAMPLE_SVAD
 *
 * \param op_data Pointer to the operator instance data.
 * \param touched Structure to return the terminals which this operator wants kicked
 */
static void ml_example_svad_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data = get_instance_data(op_data);
    unsigned available_data;

    /* Input buffer must be connected */
    if(ml_example_svad_data->ip_buffer == NULL)
    {
       return;
    }

    /* number of samples to process at input buffer    */
    available_data = cbuffer_calc_amount_data_in_words(ml_example_svad_data->ip_buffer);
    /* Check for sufficient data at the input buffer*/
    if(available_data < ml_example_svad_data->pre_processing_container->input_roll_size)
    {
        return;
    }

    // Perform Pre-procesing
    ml_example_svad_pre_process(ml_example_svad_data);

    // Run ML model
    if(ml_example_svad_data->pre_processing_container->tensor_formation_complete == TRUE)
    {
        ml_example_svad_run_model(ml_example_svad_data);
        ml_example_svad_data->pre_processing_container->tensor_formation_complete = FALSE;

        // Perform Post-processing
        ml_example_svad_post_process(ml_example_svad_data);

        /* If a status change has been detected in VAD, send an unsolicited opmsg */
        if(ml_example_svad_data->post_processing_container->vad_status.state_change_det)
        {
            unsigned frames_processed = ml_example_svad_data->ml_engine_container->frames_processed;
            unsigned state = ml_example_svad_data->post_processing_container->vad_status.current_state;
            unsigned payload[ML_EXAMPLE_SVAD_UNSOLICITED_MESSAGE_LEN] = {frames_processed, state};
            common_send_unsolicited_message(op_data, ML_EXAMPLE_SVAD_UNSOLICITED_MESSAGE_ID, ML_EXAMPLE_SVAD_UNSOLICITED_MESSAGE_LEN, payload);
        }
    }
    else
    {
        /* tensor formation is not complete, ask for more data */
        touched->sinks = TOUCHED_SINK_0;
    }

    /* Increment the frame count */
    ml_example_svad_data->ml_engine_container->frames_processed++;
}
/****************************************************************************
OPMSG Handlers
*/

/**
 * \brief Loads the ML_EXAMPLE_SVAD model file
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the destroy request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool ml_example_svad_opmsg_load_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
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

    ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data = get_instance_data(op_data);
    ML_ENGINE_OP_DATA *ml_engine_data = ml_example_svad_data->ml_engine_container;

    /* Call helper function to load the model */
    return ml_engine_load(ml_engine_data, usecase_id, batch_reset_count, file_handle, access_method);
}

/**
 * \brief Unloads the ML_EXAMPLE_SVAD model file and releases associated memory
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the destroy request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool ml_example_svad_opmsg_unload_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
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
 * \brief Activates the ML_EXAMPLE_SVAD model file associated with the usecase_id
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the destroy request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool ml_example_svad_opmsg_activate_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* currently not supported while operator is running */
    if (opmgr_op_is_running(op_data))
    {
        L0_DBG_MSG("MLE: Cannot activate model while operator is running  \n");
        return FALSE;
    }

    ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data = get_instance_data(op_data);
    ML_ENGINE_OP_DATA *ml_engine_data = ml_example_svad_data->ml_engine_container;
    /* get the usecase_id from the operator message */
    unsigned usecase_id = OPMSG_FIELD_GET(message_data, OPMSG_ML_ENGINE_ACTIVATE_MODEL, USECASE_ID);

    /* Call helper function to activate the model */
    bool status =  ml_engine_activate(ml_engine_data, usecase_id);
    if (status == FALSE)
    {
        return status;
    }
    /* Initialise post-processing */
    if(!ml_example_svad_init_post_process(ml_example_svad_data))
    {
        return FALSE;
    }
    /* Initialise pre-processing */
    if(!ml_example_svad_init_pre_process(ml_example_svad_data, usecase_id))
    {
        return FALSE;
    }
    return TRUE;
}

