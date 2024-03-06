/**
 * \file  @@@cap_name@@@_cap.h
 * \ingroup capabilities
 *
 *
 */

#ifndef _@@@cap_name^U@@@_CAP_H
#define _@@@cap_name^U@@@_CAP_H

#include "@@@cap_name@@@_defs.h"
#include "@@@cap_name@@@.h"
#include "ml_engine.h"
#include "ml_engine_usecase_manager.h"
#include "pre_passthrough.h"
#include "post_passthrough.h"
#include "preproc_common.h"
#include "postproc_common.h"


/****************************************************************************
Public Type Definitions
*/

typedef struct t_PREPROC_CONTAINER{
    /** Input sample rate */
    unsigned input_sample_rate;
    /** Input block size */
    unsigned input_block_size;
    /** pointer to passthrough data **/
    t_PrePassthrough_Struct * passthrough_data;
    /*!< output to the algorithm */
    ALGO_OUTPUT_INFO algo_output[NUM_IP_TENSORS];  
    /** number of tensors output by this algorithm */
    unsigned num_tensors;  
    signed * linear_input_buffer;
} t_PREPROC_CONTAINER;

typedef struct t_POSTPROC_CONTAINER{
    /** Output sample rate */
    unsigned output_sample_rate;
    /** Output block size */
    unsigned output_block_size;
    t_PostPassthrough_Struct * passthrough_data;
    /*!< output to the algorithm */
    ALGO_INPUT_INFO algo_input[NUM_OP_TENSORS];  
    /** number of tensors output by this algorithm */
    unsigned num_tensors; 
    signed * linear_output_buffer;                      
} t_POSTPROC_CONTAINER;

typedef struct @@@cap_name^U@@@_OP_DATA{
    /* Define here your capability specific data */
    t_PREPROC_CONTAINER      * pre_processing_container;
    ML_ENGINE_OP_DATA        * ml_engine_container;
    t_POSTPROC_CONTAINER     * post_processing_container;

    tCbuffer *ip_buffer;                    /*!< The buffer at the input terminal for this capability */
    tCbuffer *op_buffer;                    /*!< The buffer at the output terminal for this capability */
} @@@cap_name^U@@@_OP_DATA;





/* Operator Message handlers */
/**
 * \brief  @@@cap_name@@@ operator create
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool @@@cap_name@@@_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/**
 * \brief  @@@cap_name@@@ operator start
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool @@@cap_name@@@_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/**
 * \brief  @@@cap_name@@@ operator connect
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool @@@cap_name@@@_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/**
 * \brief  @@@cap_name@@@ operator disconnect
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool @@@cap_name@@@_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/**
 * \brief  @@@cap_name@@@ operator destroy
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool @@@cap_name@@@_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/**
 * \brief  @@@cap_name@@@ buffer details
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool @@@cap_name@@@_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/**
 * \brief  @@@cap_name@@@ pre-processing create  
 *
 * \param  pre_proc_container          Pointer to algo data
 *
 * \return  
 *
 */
extern void @@@cap_name@@@_pre_processing_create(@@@cap_name^U@@@_OP_DATA *op_ext_data);

/**
 * \brief  @@@cap_name@@@ pre-processing destroy  
 *
 * \param  pre_proc_container          Pointer to algo data
 *
 * \return  
 *
 */
extern void @@@cap_name@@@_pre_processing_destroy(t_PREPROC_CONTAINER *pre_proc_container);

/**
 * \brief  @@@cap_name@@@ pre-processing  
 *
 * \param  op_ext_data          Pointer to extra operator data
 *
 * \return  
 *
 */
extern void @@@cap_name@@@_pre_process(@@@cap_name^U@@@_OP_DATA *op_ext_data);

/**
 * \brief  @@@cap_name@@@ machine learning model runner processing  
 *
 * \param  op_ext_data          Pointer to extra operator data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern void @@@cap_name@@@_run_model(@@@cap_name^U@@@_OP_DATA *op_ext_data);

/**
 * \brief  @@@cap_name@@@ post-processing  
 *
 * \param  post_proc_container          Pointer to algo data
 *
 * \return  
 *
 */
void @@@cap_name@@@_post_processing_create(@@@cap_name^U@@@_OP_DATA *op_ext_data);

/**
 * \brief  @@@cap_name@@@ post-processing  destroy
 *
 * \param  post_proc_container          Pointer to algo data
 *
 * \return  
 *
 */
void @@@cap_name@@@_post_processing_destroy(t_POSTPROC_CONTAINER *post_proc_container);

/**
 * \brief  @@@cap_name@@@ post-processing  
 *
 * \param  op_ext_data          Pointer to extra operator data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern void @@@cap_name@@@_post_process(@@@cap_name^U@@@_OP_DATA *op_ext_data);

/* Operator Message Handlers */
extern bool @@@cap_name@@@_opmsg_load_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool @@@cap_name@@@_opmsg_unload_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool @@@cap_name@@@_opmsg_activate_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool @@@cap_name@@@_opmsg_set_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool @@@cap_name@@@_opmsg_set_input_tensor_sequence(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);



#endif /* @@@cap_name^U@@@_CAP_H */
