/****************************************************************************
 * Copyright (c) 2015 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  sco_rcv_data_processing_c.c
 * \ingroup  operators
 *
 *  SCO receive operator
 *
 */
/****************************************************************************
Include Files
*/
#include "sco_nb_private.h"
#ifdef CVSD_CODEC_SOFTWARE
#include "cvsd.h"
#endif

/****************************************************************************
Private Constant Definitions
*/

/** Minimum and default block size for sco receive */
#define SCO_RCV_MIN_BLOCK_SIZE                       (SCO_MIN_BLOCK_SIZE)
#define SCO_RCV_DEFAULT_BLOCK_SIZE                   (SCO_DEFAULT_BLOCK_SIZE)


#define MAX_SCO_NB_PACKETS_TO_PROCESS 2

/****************************************************************************
Private Type Definitions
*/

/****************************************************************************
Private Function Declarations
*/
static unsigned process_packet_payload(OPERATOR_DATA *op_data,
                                       unsigned input_octets,
                                       unsigned generated_output_words);

#ifndef UNIT_TEST_PROCESS_ENCODED_DATA
static unsigned decode_packet(OPERATOR_DATA *op_data,
                              unsigned packet_length);
#else
extern unsigned decode_packet(OPERATOR_DATA *op_data,
                              unsigned packet_length);
#endif

/*****************************************************************************
Private Constant Declarations
*/

/* Decoder frame format info, used when reading metadata 
 * Although there are no "frames" the relationship between 
 * encoded octets and duration still needs to match
 */
static const SCO_ISO_FRAME_INFO nb_dec_frame_info = 
{
    FALSE,  /* Not required to process whole frames only */
#ifdef CVSD_CODEC_SOFTWARE
    60,     /* Size in octets */
#else
    120,     /* Size in octets */
#endif
    7500    /* Duration in microseconds*/
};


/****************************************************************************
Private Function Definitions
*/
static inline SCO_NB_DEC_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (SCO_NB_DEC_OP_DATA *) base_op_get_instance_data(op_data);
}

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Processing function for the sco_nb_rcv operators.
 * \param op_data  Pointer to the operator data
 * \return The terminals that the operator touched
 */
unsigned sco_rcv_processing(OPERATOR_DATA *op_data)
{
    SCO_NB_DEC_OP_DATA *sco_nb_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data = &sco_nb_data->sco_rcv_op_data;
    unsigned min_input_octets, min_generated_output_words;
    unsigned max_input_octets, max_generated_output_words;
    unsigned ret_terminals = TOUCHED_NOTHING;

    /* Init run specific variables */
    sco_nb_data->sco_rcv_output_samples = 0;

    min_input_octets = CONVERT_SCO_WORDS_TO_OCTETS(SCO_RCV_MIN_BLOCK_SIZE);
    min_generated_output_words = sco_rcv_get_output_size_words(min_input_octets);

    max_input_octets = CONVERT_SCO_WORDS_TO_OCTETS(SCO_RCV_DEFAULT_BLOCK_SIZE);
    max_generated_output_words = sco_rcv_get_output_size_words(max_input_octets);

    /* Check if we can process at least a minimum amount of data:
     * the equivalent of an EV3/HV3 packet. */
    while (cbuffer_enough_data_to_run_ex(sco_data->buffers.ip_buffer,
                                         min_input_octets)
           && enough_space_to_run(sco_data, min_generated_output_words))
    {
        /* Check if we can process more data than the minimum:
         * the equivalent of a 2EV3 packet. */
        if (cbuffer_enough_data_to_run_ex(sco_data->buffers.ip_buffer,
                                          max_input_octets)
           && enough_space_to_run(sco_data, max_generated_output_words))
        {
            /* We can process more data, let's do so. */
            ret_terminals |= process_packet_payload(op_data,
                                                    max_input_octets,
                                                    max_generated_output_words);
        }
        else
        {
            /* Can't process more data, stick to the minimum */
            ret_terminals |= process_packet_payload(op_data,
                                                    min_input_octets,
                                                    min_generated_output_words);
        }
    }

    return ret_terminals;
}

/**
 * \brief Process a packet from the input buffer.
 *        May produce a decoded frame or run plc on the output.
 *
 * \param op_data                 Pointer to the operator data
 * \param input_octets            Packet length in octets
 * \param generated_output_words  Amount of samples generated
 *
 * \return The terminals that the operator touched
 */
static unsigned process_packet_payload(OPERATOR_DATA *op_data,
                                       unsigned input_octets,
                                       unsigned generated_output_words)
{
    SCO_NB_DEC_OP_DATA *sco_nb_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data = &sco_nb_data->sco_rcv_op_data;
    metadata_status status = OK;
    bool data_avail = sco_dec_read_metadata(sco_data, input_octets, &status, &nb_dec_frame_info);

    /* No data could be because of a discard. */
    if(!data_avail)
    {
        return TOUCHED_NOTHING;
    }

    /* Increment the frame count. (if something goes wrong frame_error_count is incremented) */
    sco_data->frame_count++;

    if (status == OK_WBM)
    {
        SCO_DBG_ERRORMSG1("SCO NB: Weak bit mask based error correction is unsupported, Frame count %d",
                sco_data->frame_count);
        fault_diatribe(FAULT_AUDIO_WBM_NOT_SUPPORTED, sco_data->frame_count);
    }

    if (status != OK)
    {
        sco_common_rcv_print_bad_status(base_op_get_ext_op_id(op_data), status, sco_data->frame_count);
        /* The current _packet_ is not OK */
        sco_data->frame_error_count++;

        discard_data_octets(sco_data, input_octets);

#ifdef INSTALL_PLC100
        /* keep metadata aligned with the buffer */
        sco_fw_generate_output_metadata(sco_data, generated_output_words);

        /* Do packet loss concealment (PLC). */
        sco_plc100_call_plc(sco_nb_data->force_plc_off,
                            sco_nb_data->plc100_struc,
                            NOTHING_RECEIVED,
                            generated_output_words);

        return TOUCHED_SOURCE_0;
#else
        return TOUCHED_NOTHING;
#endif /* INSTALL_PLC100 */
    }
    else
    {
        /* Process the received packet's payload. */
        return decode_packet(op_data, input_octets);
    }
}

#ifndef UNIT_TEST_PROCESS_ENCODED_DATA
/****************************************************************************
Private Function Definitions
*/
/**
 * \brief Copies a valid packet to the output buffer. Runs plc100 on the output.
 *
 * \param op_data        Pointer to the operator data struct
 * \param packet_length  Size of the current packet in octets
 * \return The terminals that the operator touched
 */
static unsigned decode_packet(OPERATOR_DATA *op_data,
                              unsigned packet_length)
{
    SCO_NB_DEC_OP_DATA *sco_nb_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data = &sco_nb_data->sco_rcv_op_data;
    unsigned output_words;

#ifdef INSTALL_PLC100
    int *op_buffer_write_address;
#endif /* INSTALL_PLC100 */

    /* The generated output in words. */
    output_words = sco_rcv_get_output_size_words(packet_length);

    sco_nb_data->sco_rcv_output_samples += output_words;

#ifdef INSTALL_PLC100
    /*
     * Because plc100 updates the write pointer we need to reset the original write
     * pointer after the cbops copy.
     */
    op_buffer_write_address = sco_data->buffers.op_buffer->write_ptr;
#endif /* INSTALL_PLC100 */

    sco_fw_generate_output_metadata(sco_data, output_words);

#ifdef CVSD_CODEC_SOFTWARE
    cvsd_receive_asm(&sco_nb_data->cvsd_struct,
                     sco_data->buffers.ip_buffer,
                     sco_data->buffers.op_buffer,
                     sco_nb_data->ptScratch,
                     output_words);
#else
    /* Copy the packet to the output. */
    unsigned amount_copied = cbuffer_copy(sco_data->buffers.op_buffer,
                                          sco_data->buffers.ip_buffer,
                                          output_words);

    /* This really shouldn't happen. Before calling decode_packet we make sure
     *  there is enough space in the output buffer.
     */
    PL_ASSERT(amount_copied == output_words);
#endif /* CVSD_CODEC_SOFTWARE */

#ifdef INSTALL_PLC100
    /* Reset the write pointer for the PLC library. */
    cbuffer_set_write_address(sco_data->buffers.op_buffer,
                              (unsigned int *)op_buffer_write_address);

    /* Do packet loss concealment (PLC). */
    sco_plc100_call_plc(sco_nb_data->force_plc_off, sco_nb_data->plc100_struc,
                        OK, output_words);
#endif /* INSTALL_PLC100 */

    SCO_DBG_MSG2("After copy!  input buffer data |%4d|, output buffer space |%4d|.",
                 cbuffer_calc_amount_data_in_words(sco_data->buffers.ip_buffer),
                 cbuffer_calc_amount_space_in_words(sco_data->buffers.op_buffer));

    /* touch the output */
    return TOUCHED_SOURCE_0;
}
#endif /* UNIT_TEST_PROCESS_ENCODED_DATA */

/**
 * \brief Returns the SCO rcv output words.
 *
 * \param sco_data  Amount of data read from the input [bytes]
 * \return The number of samples produced by SCO_RCV
 */
unsigned sco_rcv_get_output_size_words(unsigned input_bytes)
{
    unsigned packet_size;

    /* Convert SCO packet length from octets to SCO words.
     * SCO input is always unpacked 16-bit.
     */
   packet_size  = CONVERT_OCTETS_TO_SCO_WORDS(input_bytes);

    /*
     * When running a CVSD decoder in FW, the amount of input data received
     * by the SCO Rx FW is halved compared to when it is performed by H/W.
     * (hence the multiplying factor SW_CVSD_RATIO).
     * The input data is read as 16bit words irrespective of whether the data
     * is in PCM or coded CVSD format. However, in the case of CVSD format
     * there is half the amount.
     */
    packet_size = packet_size * SW_CVSD_RATIO;
    if (packet_size == 0)
    {
        /* SCO_PKT_SIZE field is not initialised, use block size for the moment. */
        packet_size = SCO_RCV_MIN_BLOCK_SIZE * SW_CVSD_RATIO;
    }

    return packet_size;
}
