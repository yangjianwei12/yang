/**
 * \file  ml_example_svad_cap.h
 * \ingroup capabilities
 *
 *
 */

#ifndef _ML_EXAMPLE_SVAD_CAP_H
#define _ML_EXAMPLE_SVAD_CAP_H

#include "ml_example_svad_defs.h"
#include "ml_example_svad.h"
#include "ml_engine.h"
#include "ml_engine_usecase_manager.h"
#include "preproc_common.h"
#include "spectral.h"

/* unsolicited message send by ML_EXAMPLE_SVAD - id and length */
#define ML_EXAMPLE_SVAD_UNSOLICITED_MESSAGE_ID  (101)
#define ML_EXAMPLE_SVAD_UNSOLICITED_MESSAGE_LEN (2)

#define FFT_INPUT_SIZE      (640)
#define FFT_SIZE            (1024)
#define SPECTRAL_CHANNELS   (16)

/**
 * SPEC_OUT_IDX, STAT_OUT_IDX and NORM_OUT_IDX represent the tensor indices of the input tensors of the model 
 * Values obtained from the keai file
*/
#define SPEC_OUT_IDX        (4)
#define STAT_OUT_IDX        (0)
#define NORM_OUT_IDX        (2)

#define NORM_ALPHA          (0.00690749753266572)
#define NORM_REQ            (1)
#define NUM_TENSORS         (3)

/****************************************************************************
Public Type Definitions
*/

/** Data Structures required for ML_EXAMPLE_SVAD post-processing */
typedef struct t_VAD_STATUS
{
    /* VAD state change detected */
    bool state_change_det;
    /* VAD current state */
    int current_state;
}t_VAD_STATUS;

/* Buffer structure used by SVAD at its input and output */
typedef struct t_VAD_BUFFER
{
    tCbuffer *cdata;
    signed *data;
}t_VAD_BUFFER;

typedef struct t_PREPROC_CONTAINER{
    /** Input sample rate */
    unsigned input_sample_rate;
    /** Input block size */
    unsigned input_block_size;
    /** Input roll over size */
    unsigned input_roll_size;
    /** Flag to indicate if tensors are ready for the engine to consume */
    bool tensor_formation_complete;
    /** Array of output tensor structures */
    ALGO_OUTPUT_INFO algo_output[NUM_TENSORS];  /*!< output to the algorithm */
    /** number of tensors output by this algorithm */
    unsigned num_tensors;
    /** VAD Input buffer structure */
    t_VAD_BUFFER vad_ip_buffer;
    tEAISpectrum * spectrum_data;
} t_PREPROC_CONTAINER;

typedef struct t_ML_ENGINE_CONTAINER{
    /** Model loaded flag */
    bool model_loaded;
} t_ML_ENGINE_CONTAINER;

typedef struct t_POSTPROC_CONTAINER{
    /** Output sample rate */
    unsigned output_sample_rate;
    /** Output block size */
    unsigned output_block_size;
    /** The remaining fields are specific to SVAD processing */
    /** VAD status */
    t_VAD_STATUS vad_status;
    /** VAD output buffer */
    t_VAD_BUFFER vad_op_buffer;
} t_POSTPROC_CONTAINER;


typedef struct ML_EXAMPLE_SVAD_OP_DATA{
    /* Define here your capability specific data */
    t_PREPROC_CONTAINER      *pre_processing_container;
    ML_ENGINE_OP_DATA        *ml_engine_container;
    t_POSTPROC_CONTAINER     *post_processing_container;

    tCbuffer *ip_buffer;                    /*!< The buffer at the input terminal for this capability */
} ML_EXAMPLE_SVAD_OP_DATA;


/* Operator Message handlers */
/**
 * \brief  ml_example_svad operator create
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool ml_example_svad_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/**
 * \brief  ml_example_svad operator start
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool ml_example_svad_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/**
 * \brief  ml_example_svad operator connect
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool ml_example_svad_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/**
 * \brief  ml_example_svad operator disconnect
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool ml_example_svad_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/**
 * \brief  ml_example_svad operator destroy
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool ml_example_svad_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/**
 * \brief  ml_example_svad buffer details
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool ml_example_svad_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/**
 * \brief  ml_example_svad initialise pre-processing
 *
 * \param  op_ext_data          Pointer to extra operator data
 *
 * \return  boolean indicating success or failure.
 */
extern bool ml_example_svad_init_pre_process(ML_EXAMPLE_SVAD_OP_DATA *op_ext_data, unsigned usecase_id);

/**
 * \brief  ml_example_svad pre-processing
 *
 * \param  op_ext_data          Pointer to extra operator data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern void ml_example_svad_pre_process(ML_EXAMPLE_SVAD_OP_DATA *op_ext_data);

/**
 * \brief  ml_example_svad deinit pre-processing
 *
 * \param  usecase_id          Usecase identifier
 *
 */
extern bool ml_example_svad_deinit_pre_process(ML_EXAMPLE_SVAD_OP_DATA *op_ext_data, unsigned usecase_id);

/**
 * \brief  ml_example_svad machine learning model runner processing
 *
 * \param  op_ext_data          Pointer to extra operator data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern void ml_example_svad_run_model(ML_EXAMPLE_SVAD_OP_DATA *op_ext_data);

/**
 * \brief  ml_example_svad initialise post-processing
 *
 * \param  op_ext_data          Pointer to extra operator data
 *
 */
extern bool ml_example_svad_init_post_process(ML_EXAMPLE_SVAD_OP_DATA *op_ext_data);

/**
 * \brief  ml_example_svad post-processing
 *
 * \param  op_ext_data          Pointer to extra operator data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern void ml_example_svad_post_process(ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data);

/**
 * \brief  ml_example_svad deinit post-processing
 *
 * \param  op_ext_data          Pointer to extra operator data
 * \return none
 *
 */
extern void ml_example_svad_deinit_post_process(ML_EXAMPLE_SVAD_OP_DATA *ml_example_svad_data);

/* Operator Message Handlers */
extern bool ml_example_svad_opmsg_load_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ml_example_svad_opmsg_unload_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ml_example_svad_opmsg_activate_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ml_example_svad_opmsg_set_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ml_example_svad_opmsg_set_input_tensor_sequence(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);



#endif /* ML_EXAMPLE_SVAD_CAP_H */
