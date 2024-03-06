/****************************************************************************
 * Copyright (c) 2014 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  sco_common_funcs.h
 * \ingroup  capabilities
 *
 *  Common functions, used by NB and WB SCO capabilities.
 *  Functions "sco_common_rcv_..." are for receive capabilities (SCO_RCV & WBS_DEC).
 *  "sco_common_send_..." are for send capabilities (SCO_SEND, WBS_ENC).
 *  "sco_common_..." are for any SCO capability.
 *
 */

#include "capabilities.h"
#include "sco_common_struct.h"


/* initialise some common parts of SCO operator data - it trusts that everything
 * referenced here was allocated before call */


/**
 * \brief   Initialise SCO_TERMINAL_BUFFERS info on additional terminals.
 *          ISO capabilities can support multiple audio channels which are
 *          multiplexed over a single encoded data channel.
 *          This function is needed only if the capability supports multiple
 *          inputs or multiple outputs (can also be called with only one
 *          input/output).
 *          This mechanism does NOT support BOTH multiple inputs and multiple
 *          outputs (asserts for not supported inputs).
 *          The number of inputs/outputs is taken from CAPABILITY_DATA.
 *
 *          Note: sco_common_destroy_terminals() must be called at operator
 *          destruction.
 *
 * \param  op_data    Pointer to the operator structure.
 * \param  terminals  Pointer to input/output buffer struct
 *
 * \return TRUE if completed successfully, FALSE if no memory.
 */
extern bool sco_common_init_terminals(OPERATOR_DATA *op_data,
                                      SCO_TERMINAL_BUFFERS *terminals);

/**
 * \brief   Releases any allocated memory for additional terminals.
 *
 * \param  terminals  Pointer to input/output buffer struct
 */
extern void sco_common_destroy_terminals(SCO_TERMINAL_BUFFERS *terminals);


/** \brief  Get buffer for input terminal with specified index.
 *          If capability doesn't support multiple inputs, and index != 0,
 *          returns NULL.
 *
 * \param  terminals  Pointer to input/output buffer struct
 * \param  index      Input terminal number
 *
 * \return Requested input terminal buffer pointer if successfully, NULL on fail.
 */
extern tCbuffer *sco_common_get_input_buffer(SCO_TERMINAL_BUFFERS *terminals,
                                             unsigned index);
/** \brief  Get buffer for output terminal with specified index.
 *          If capability doesn't support multiple inputs, and index != 0,
 *          returns NULL.
 *
 * \param  terminals  Pointer to input/output buffer struct
 * \param  index      Output terminal number
 *
 * \return Requested output terminal buffer pointer if successfully, NULL on fail.
 */
extern tCbuffer *sco_common_get_output_buffer(SCO_TERMINAL_BUFFERS *terminals,
                                              unsigned index);

/**
 * \brief Initialise NB or WB SCO receive operators' certain parameters during
 *        creation. It differs from sco_common_rcv_reset_working_data() as
 *        latter can be called during operator reset.
 *        The function can only be called once the capability-specific
 *        and (if present in the build) the PLC data structures
 *        are allocated!
 *        Also assumes that SCO_COMMON_RCV_OP_DATA is zero-initialised on
 *        allocation.
 *
 * \param  sco_rcv_op_data  Pointer to the common SCO receive operator data structure.
 *
 * \return TRUE if completed successfully, FALSE on fail.
 */
extern void sco_common_rcv_initialise(SCO_COMMON_RCV_OP_DATA *sco_rcv_op_data);


/**
 * \brief Destroys the common SCO receive operator's allocations.
 *
 * \param  sco_rcv_op_data  Pointer to the common SCO receive operator data structure.
 */
extern void sco_common_rcv_destroy(SCO_COMMON_RCV_OP_DATA *sco_rcv_op_data);

/**
 * \brief Initialise various working data params of the NB or WB SCO receive operators.
 *
 * \param  sco_rcv_op_data  Pointer to the common SCO receive operator data structure.
 *
 * \return TRUE if completed successfully, FALSE on fail.
 */
/*
 * The part of the extra_op_data that is referenced to
 * by this is identical for both capabilities, hence pointer casts are safe
 * unless someone re-arranges the operator data structs and ignores comments :)
 */
extern bool sco_common_rcv_reset_working_data(SCO_COMMON_RCV_OP_DATA *sco_rcv_op_data);

/**
 * \brief Helper function for opmsg handlers that  get the frame count statistics
 *
 * \param  sco_rcv_op_data  Pointer to the common SCO receive operator data structure.
 * \param  message_data  Pointer to message structure.
 * \param  resp_length  Pointer to response message length in words
 * \param  resp_data  Pointer to a pointer to response message structure, allocated in this function.
 *
 * \return TRUE if completed successfully, FALSE on fail.
 */
extern bool sco_common_rcv_frame_counts_helper(SCO_COMMON_RCV_OP_DATA *sco_rcv_op_data, void *message_data,
                                               unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

/**
 * \brief Operator message handler for getting the terminal data formats
 *
 * \param  op_data  Pointer to the operator structure.
 * \param  message_data  Pointer to message structure.
 * \param  resp_length  Pointer to response message length in words
 * \param  resp_data  Pointer to a pointer to response message structure, allocated in this function.
 * \param  input_format  The data format of the capability input terminal.
 * \param  output_format  The data format of the capability output terminal.
 *
 * \return TRUE if completed successfully, FALSE on fail.
 */
extern bool sco_common_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id,
        void **response_data, AUDIO_DATA_FORMAT input_format, AUDIO_DATA_FORMAT output_format);

/**
 * \brief Operator message handler for getting the block size of the operator
 *        This is only used for send / to-air operators, and only the output 
 *        (encoded frame) size is interesting. 
 *
 * \param  op_data  Pointer to the operator structure.
 * \param  message_data  Pointer to message structure.
 * \param  resp_length  Pointer to response message length in words
 * \param  resp_data  Pointer to a pointer to response message structure, allocated in this function.
 * \param  output_block_size  the output terminal block size of the capability.
 *
 * \return TRUE if completed successfully, FALSE on fail.
 */
extern bool sco_common_send_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id,
                                            void **response_data, unsigned output_block_size);

/**
 * \brief Operator message handler for connecting operator terminal
 *
 * \param  op_data  Pointer to the operator structure.
 * \param  message_data  Pointer to message structure.
 * \param  resp_length  Pointer to response message length in words
 * \param  resp_data  Pointer to a pointer to response message structure, allocated in this function.
 *
 * \return TRUE if completed successfully, FALSE on fail.
 */
extern bool sco_common_connect(OPERATOR_DATA *op_data, void *message_data,
        unsigned *response_id, void **response_data, SCO_TERMINAL_BUFFERS *bufs,
        unsigned *terminal_id);

/**
 * \brief Operator message handler for disconnecting operator terminal
 *
 * \param  op_data  Pointer to the operator structure.
 * \param  message_data  Pointer to message structure.
 * \param  resp_length  Pointer to response message length in words
 * \param  resp_data  Pointer to a pointer to response message structure, allocated in this function.
 *
 * \return TRUE if completed successfully, FALSE on fail.
 */
extern bool sco_common_disconnect(OPERATOR_DATA *op_data, void *message_data,
        unsigned *response_id, void **response_data, SCO_TERMINAL_BUFFERS *bufs,
        unsigned *terminal);

/**
 * sco_common_rcv_opmsg_set_ttp_latency
 * \brief sets the rcv op to generate timestamp tags instead of default toa
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the start request message
 * \param resp_length pointer to location to write the response message length
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
extern bool sco_common_rcv_opmsg_set_ttp_latency(OPERATOR_DATA *op_data, void *message_data,
                                     unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
/**
 * sco_common_rcv_opmsg_set_buffer_size
 * \brief message handler to set required sco rcv output buffer size
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the start request message
 * \param resp_length pointer to location to write the response message length
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
extern bool sco_common_rcv_opmsg_set_buffer_size(OPERATOR_DATA *op_data, void *message_data,
                                          unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

/**
 * sco_common_rcv_opmsg_set_terminal_buffer_size
 * \brief message handler to set required sco rcv output buffer size
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the start request message
 * \param resp_length pointer to location to write the response message length
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
extern bool sco_common_rcv_opmsg_set_terminal_buffer_size(OPERATOR_DATA *op_data, void *message_data,
    unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

/**
 * sco_common_send_opmsg_set_terminal_buffer_size
 * \brief message handler to set required sco rcv output buffer size
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the start request message
 * \param resp_length pointer to location to write the response message length
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
extern bool sco_common_send_opmsg_set_terminal_buffer_size(OPERATOR_DATA *op_data, void *message_data,
    unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);


/**
 * \brief Discard a certain amount of words from the SCO input buffer.
 *
 * \param sco_rcv_op_data     Pointer to the common sco operator structure.
 * \param amount_to_discard   Number of octets to discard from the SCO
 *                            input buffer.
 */
extern void discard_data_octets(SCO_COMMON_RCV_OP_DATA *sco_rcv_op_data,
                                unsigned amount_to_discard);
/**
 * \brief Retrieve the status of the SCO packet data which is
 *        populated by the sco_drv
 *
 * \param mtag            cbuffer metadata tag
 * \param status          status flag extracted
 */
extern bool sco_common_retrieve_metadata_status(metadata_tag *mtag, metadata_status *status);

/**
 * \brief Initialise metadata transport for SCO / ISO encoder capability
 *
 * \param enc_ttp  Encoder TTP info
 *
 */
void sco_common_send_init_metadata(SCO_ENC_TTP *enc_ttp);

/**
 * \brief Transfer metadata for SCO / ISO encoder capability
 * This is assumed to be either a single frame or a chunk of stream-based data
 * which can be treated as a frame.
 *
 * \param buffers  Pointer to input/output buffer struct
 * \param enc_ttp  Encoder TTP info
 * \param ip_proc_samples  Samples consumed from input
 * \param out_data_octets  Output octets produced
 * \param sample_rate  Input sample rate
 * 
 */
void sco_common_send_transport_metadata(SCO_TERMINAL_BUFFERS *buffers, 
                                        SCO_ENC_TTP *enc_ttp, 
                                        unsigned ip_proc_samples, 
                                        unsigned out_data_octets, 
                                        unsigned sample_rate);


/**
 * \brief Write status message to audio log
 *
 * \param status  Received status value
 * 
 */
void sco_common_rcv_print_bad_status(EXT_OP_ID opid, metadata_status status, unsigned frame_count);

