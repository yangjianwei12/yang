/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  ml_nnops_cap.c
 * \ingroup  capabilities
 *
 * ML_NNOPS Capability
 *
 * This example Kymera capability shows an example of how the use
 * the kalimba machine learning operator library using the Kymera 
 * machine learning operator interface.
 * In order to use the Kymera machine learning operator interface,
 * following changes needs to be done to the capability build
 * config file:
 * 1. Include the library:libeai.a. This is available as a prebuilt
 *    library in kalimba\kymera\lib_release\($CONFIG).
 * 2. Include the library:libml_op_interface.a. This is available as
 *    a prebuilt library in kalimba\kymera\lib_release\($CONFIG).
 * 3. Add the public header paths:
 *    kalimba\lib\ml\common
 *    kalimba\lib\ml\nnops
 *    kalimba\kymera\capabilities\ml_engine
 * For example, please refer to the build config file of this
 * example capability.
 *
 * In this capability, we create a MUL Operator with two input tensors.
 * We specify each tensor with a tensor ID.
 * TENSOR_ID_0: First input tensor to the MUL operator. This is a
 *              1-D vector of size 160.
 * TENSOR_ID_1: Second input tensor to the MUL operator. This is a
 *              scalar value of size 1.
 * TENSOR_ID_2: Output tensor of the MUL operator.This is a 1-D
 *              vector of size 160.
 *
 * For simplicity, the data in TENSOR_ID_0 for this example capability
 * is the input audio data itself.So, the process of generating this
 * input tensor is just copying the mono audio stream from the
 * capability's input terminal buffer to the tensor buffer.
 * The data in TENSOR_ID_1 is a scalar value of 0.5.
 * The MUL operator will perform element wise multiplication of all
 * the elements of TENSOR_ID_0 with the scalar value in TENSOR_ID_1.
 * This means that the output tensor, TENSOR_ID_2 has a scaled version
 * of the data in TENSOR_ID_0.
 * Hence the output data from this capability is the same mono audio
 * stream in the input but scaled by 6dB(since the multiplication
 * factor is 0.5).
 *                                              
 *                  +============+                  
 *    tensor_id_0   |            |   
 *   -------------> |   mul      |   output   
 *       (160)      |   operator | ------------->  
 *                  |            |
 *    tensor_id_1   |            | tensor_id_2 
 *   -------------> |            |   (160)
 *        (1)       +============+             
 *                                                       
 *****************************************************************
 */    

/****************************************************************************
Include Files
*/
#include "ml_nnops_cap.h"

/****************************************************************************
Private Function Definitions
*/

/****************************************************************************
Private Constant Declarations
*/

#ifdef CAPABILITY_DOWNLOAD_BUILD
    #define ML_NNOPS_CAP_ID           CAP_ID_DOWNLOAD_ML_NNOPS
#else
    #define ML_NNOPS_CAP_ID           CAP_ID_ML_NNOPS
#endif /* CAPABILITY_DOWNLOAD_BUILD */

/* Capability function handler table */
const handler_lookup_struct ml_nnops_handler_table =
{
    ml_nnops_op_create,             /* OPCMD_CREATE */
    ml_nnops_op_destroy,            /* OPCMD_DESTROY */
    base_op_start,                  /* OPCMD_START */
    base_op_stop,                   /* OPCMD_STOP */
    base_op_reset,                  /* OPCMD_RESET */
    ml_nnops_op_connect,            /* OPCMD_CONNECT */
    base_op_disconnect,             /* OPCMD_DISCONNECT */
    ml_nnops_op_buffer_details,     /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,        /* OPCMD_DATA_FORMAT */
    base_op_get_sched_info          /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table - this is the set of operator
 * messages that the capability understands and will attempt to service. */
const opmsg_handler_lookup_table_entry ml_nnops_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {0, NULL}
};

/* Capability data - This is the definition of the capability that Opmgr uses to
 * create the capability from. */
const CAPABILITY_DATA ml_nnops_cap_data =
{
    ML_NNOPS_CAP_ID,                    /* Capability ID */
    0, 1,                               /* Version information - hi and lo parts */
    MAX_IP_TERMINALS, MAX_OP_TERMINALS, /* Max number of sinks/inputs and sources/outputs */
    &ml_nnops_handler_table,            /* Pointer to message handler function table */
    ml_nnops_opmsg_handler_table,       /* Pointer to operator message handler function table */
    ml_nnops_process_data,              /* Pointer to data processing function */
    0,                                  /* TODO: this would hold processing time information */
    sizeof(ML_NNOPS_OP_DATA)            /* Size of capability-specific per-instance data */
};

/* Map Instance data for ACAT to show */
MAP_INSTANCE_DATA(ML_NNOPS_CAP_ID, ML_NNOPS_OP_DATA)

/* Accessing the capability-specific per-instance data function */
static inline ML_NNOPS_OP_DATA* get_instance_data(OPERATOR_DATA* op_data)
{
    return (ML_NNOPS_OP_DATA*) base_op_get_instance_data(op_data);
}

/**
 * \brief Generates input tensors from input audio data and fills the
 *        input tensor buffers
 *
 * \param op_data Pointer to the operator instance data.
 */
static unsigned ml_nnops_generate_input_tensors(ML_NNOPS_OP_DATA* opx_data)
{
    unsigned amt = 0;
    if(opx_data->ip_buffer == NULL)
    {
        return amt;
    }
    /* For the example model, the data at the input terminal is
     * the input tensor. We just copy this data from the input
     * terminal cbuffer into the tensor buffer.
     */
    ML_NNOPS_LAYER* head = opx_data->ml_nnops_layer_head;
    layer_lite* layer = (layer_lite*)(head->h_ml_op);
    /* Get the input tensor list */
    tensorlist_lite *input_tensor_list = layer->p_input_tensors;
    /* We have two input tensors for our example layer */
    Tensor *ip_tensor_one = input_tensor_list->tensors[0];
    Tensor *ip_tensor_two = input_tensor_list->tensors[1];

    signed* ip_tensor_one_buffer = (signed*)ip_tensor_one->data;
    signed* ip_tensor_two_buffer = (signed*)ip_tensor_two->data;

    /* Fill the tensor buffer */
    amt = cbuffer_read(opx_data->ip_buffer, ip_tensor_one_buffer, TENSOR_ID_0_NUM_ELEMS);
    L2_DBG_MSG1("ml_nnops_cap.c: copied %d words into input tensor", amt);
    /* The second tensor for our example is a scalar value of 0.5 in Q31*/
    *ip_tensor_two_buffer = ML_NNOPS_POINT_FIVE_IN_Q31;
    
    /* return the amount of data that was copied from the ip buffer */
    return amt;
}

/**
 * \brief Executes all layers
 *
 * \param op_data Pointer to the operator instance data.
 */
static void ml_nnops_execute_layers(ML_NNOPS_OP_DATA* opx_data)
{
    ML_NNOPS_LAYER* layer = opx_data->ml_nnops_layer_head;
    ML_RESULT status;
    unsigned count = 0;
    while(NULL != layer)
    {
        status = ml_op_intf_execute(layer->h_ml_op, layer->op_type);
        L2_DBG_MSG2("ml_nnops_cap.c: status of layer %d is %d", count, status);
        /* Here is where one can copy the output tensors from
         * one layer to the input tensors of the next layer.
         * For our example model, we just have one layer. Hence
         * the input and output layers for us is the same
         */
        layer = layer->next;
        count++;
    }
}

/**
 * \brief Copies output tensors, postprocesses them and copies
 *        into the output terminal buffers.
 *
 * \param op_data Pointer to the operator instance data.
 */
static unsigned ml_nnops_copy_output_data(ML_NNOPS_OP_DATA* opx_data)
{
    unsigned amt = 0;
    if(opx_data->op_buffer == NULL)
    {
        return amt;
    }
    ML_NNOPS_LAYER* layer = opx_data->ml_nnops_layer_head;
    while(NULL != layer->next)
    {
        layer = layer->next;
    }
    /* layer is the last layer in the chain */
    /* Get the output tensor list */
    layer_lite *op_layer = (layer_lite*)(layer->h_ml_op); 
    tensorlist_lite *output_tensor_list = op_layer->p_output_tensors;
    /* We have single output tensors for our example layer */
    Tensor *op_tensor  = output_tensor_list->tensors[0];

    signed* op_tensor_buffer = (signed*)op_tensor->data;

    /* Copy from the tensor buffer */
    amt = cbuffer_write(opx_data->op_buffer, op_tensor_buffer, TENSOR_ID_2_NUM_ELEMS);
    L2_DBG_MSG1("ml_nnops_cap.c: copied %d words from output tensor", amt);
    
    /* return the amount of data that was copied into the output buffer */
    return amt;
    
}

/****************************************************************************
Capability Function Handlers
*/
/**
 * \brief Initialises the ML_NNOPS capability.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the create request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool ml_nnops_op_create(OPERATOR_DATA *op_data, void *message_data,
                        unsigned *response_id, void **response_data)
{
    ML_NNOPS_OP_DATA *opx_data = get_instance_data(op_data);
    /* call base_op_create which also allocates and fills response message */
    if(!base_op_create(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* Create layers  - As an example, here we show how
     * we create a MUL layer
     */
    ML_NNOPS_LAYER *ml_nnops_layer_1 = xzpnew(ML_NNOPS_LAYER);
    if(NULL == ml_nnops_layer_1)
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }
    if(!create_layer(ml_nnops_layer_1))
    {
        /* Creation of layer has failed - change the status message
         * and return */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* We can similarly create another layer and connect it
     * ML_NNOPS_LAYER *ml_nnops_layer_2 = xzpnew(ML_NNOPS_LAYER);
     * create_layer(ml_nnops_layer2);
     * ml_nnops_layer_1->next = ml_nnops_layer2;
     */

    /* The ml_nnops_layer_head in the op extra data points
     * to the first layer in the chain
     */
    opx_data->ml_nnops_layer_head = ml_nnops_layer_1;
    return TRUE;
}

/**
 * \brief Destroy the ML_NNOPS capability.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the create request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool ml_nnops_op_destroy(OPERATOR_DATA *op_data, void *message_data,
                        unsigned *response_id, void **response_data)
{
    ML_NNOPS_OP_DATA *opx_data = get_instance_data(op_data);
    /* call base_op_destroy which also allocates and fills response message */
    if(!base_op_destroy(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* Destroy all the layers */
    ML_NNOPS_LAYER* current_layer = opx_data->ml_nnops_layer_head;
    ML_NNOPS_LAYER* next_layer = NULL;
    while(current_layer != NULL)
    {
        next_layer = current_layer->next;
        if(!destroy_layer(current_layer))
        {
            base_op_change_response_status(response_data, STATUS_CMD_FAILED);
            return TRUE;
        }
        pfree(current_layer);
        current_layer = next_layer;
    }   
    opx_data->ml_nnops_layer_head = NULL;
    return TRUE;
}

/**
 * \brief Connects an ML_NNOPS instance with endpoints
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the create request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool ml_nnops_op_connect(OPERATOR_DATA *op_data, void *message_data,
                         unsigned *response_id, void **response_data)
{
    ML_NNOPS_OP_DATA *opx_data = get_instance_data(op_data);
    unsigned terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    unsigned terminal_num = terminal_id & TERMINAL_NUM_MASK;;

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

    if (terminal_id & TERMINAL_SINK_MASK)
    {
        /* We only support MAX_IP_TERMINALS at the input */
        if (terminal_num >= MAX_IP_TERMINALS)
        {
           base_op_change_response_status(response_data, STATUS_INVALID_CMD_PARAMS);
           return TRUE;    
        }
        opx_data->ip_buffer = OPMGR_GET_OP_CONNECT_BUFFER(message_data);
    }
    else
    {
        /* We only support MAX_OP_TERMINALS at the output */
        if (terminal_num >= MAX_OP_TERMINALS)
        {
            base_op_change_response_status(response_data, STATUS_INVALID_CMD_PARAMS);
            return TRUE;
        }
        opx_data->op_buffer = OPMGR_GET_OP_CONNECT_BUFFER(message_data);
    }

    return TRUE;
}

/**
 * \brief Reports the buffer requirements of the ML_NNOPS capability.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the create request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool ml_nnops_op_buffer_details(OPERATOR_DATA *op_data, void *message_data,
                                unsigned *response_id, void **response_data)
{
    unsigned terminal_id = OPMGR_GET_OP_BUF_DETAILS_TERMINAL_ID(message_data);

    if (!base_op_buffer_details(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    ((OP_BUF_DETAILS_RSP*)*response_data)->supports_metadata = TRUE;
    if (terminal_id & TERMINAL_SINK_MASK)
    {
        /* Input terminal buffer size is twice the ip_frame_size */
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = INPUT_FRAME_SIZE << 1;
    }
    else
    {
        /* Output terminal buffer size is same as the op_frame_size */
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = OUTPUT_FRAME_SIZE;
    }
    return TRUE;
}

/****************************************************************************
Capability Data Processing function
*/
/**
 * \brief process function for ML_NNOPS
 *
 * \param op_data Pointer to the operator instance data.
 * \param touched Structure to return the terminals which this operator wants kicked
 */

void ml_nnops_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    ML_NNOPS_OP_DATA *opx_data = get_instance_data(op_data);

    if(opx_data->ip_buffer != NULL)
    {
        /* calculate the amount of data in the input terminal buffers */
        unsigned amount = cbuffer_calc_amount_data_in_words(opx_data->ip_buffer);
        L2_DBG_MSG1("%d data in input buffer", amount);

        /* process only if we have sufficient data */
        if (amount < INPUT_FRAME_SIZE)
        {
            return;
        }
    }

    if(opx_data->op_buffer != NULL)
    {
        /* check if there is sufficient space in the output buffer */
        unsigned output_space = cbuffer_calc_amount_space_in_words(opx_data->op_buffer);
        L2_DBG_MSG1("%d space in output buffer", output_space);

        /* process only if we have sufficient space */
        if (output_space < OUTPUT_FRAME_SIZE)
        {
            return;
        }
    }

    /* copy data into input tensor */
    unsigned data_processed = ml_nnops_generate_input_tensors(opx_data);
    /* run all the layers */
    ml_nnops_execute_layers(opx_data);
    /* copy data from output tensors */
    unsigned data_generated = ml_nnops_copy_output_data(opx_data);

    /* transport metadata. since the amount of data consumed from the terminal
     * input buffer is same as the data copied into the terminal output buffer
     * we can use the metadata_strict_transport API.
     */
    
    if(data_generated > 0)
    {
        PL_ASSERT(data_generated == data_processed);
        metadata_strict_transport(opx_data->ip_buffer,
                                  opx_data->op_buffer,
                                  data_generated * OCTETS_PER_SAMPLE);
        /* propagate kick forward */
        touched->sources = TOUCHED_SOURCE_0;
    }
    return;
}

