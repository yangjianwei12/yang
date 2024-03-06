/****************************************************************************
 * Copyright (c) 2015 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  swbs_dec_data_processing_c.c
 * \ingroup  operators
 *
 *  SWBS decode operator
 *
 */
/****************************************************************************
Include Files
*/
#ifdef ADAPTIVE_R2_BUILD
#include "axModContainerDefs_r2.h"
#else
#include "axModContainerDefs.h"
#endif
#include "swbs_private.h"

#include "patch/patch.h"
#include "pl_assert.h"

/****************************************************************************
Private Constant Definitions
*/
#define SWBS_FRAME_SIZE                            (60)
#define INPUT_BLOCK_SIZE                           (30)
#define OUTPUT_BLOCK_SIZE                          (240)
#define OUTPUT_BLOCK_SIZE_24KHZ                    (180)

/****************************************************************************
Private Type Definitions
*/

/****************************************************************************
Private Function Declarations
*/
static unsigned process_packet_payload(OPERATOR_DATA *op_data, unsigned packet_len_bytes);

#ifndef UNIT_TEST_PROCESS_ENCODED_DATA
static unsigned decode_packet(OPERATOR_DATA *op_data,
                              unsigned packet_length);
#ifdef ESCO_SUPPORT_ERRORMASK
static unsigned decode_packet_wbm(OPERATOR_DATA *op_data,
                              unsigned packet_length);
#endif
#else
extern unsigned decode_packet(OPERATOR_DATA *op_data,
                              unsigned packet_length);
#ifdef ESCO_SUPPORT_ERRORMASK
extern unsigned decode_packet_wbm(OPERATOR_DATA *op_data,
                              unsigned packet_length);
#endif
#endif

/* SWBS decoder frame format info, used when reading metadata */
static const SCO_ISO_FRAME_INFO swbs_dec_frame_info =
{
    FALSE,  /* Not required to process whole frames only */
    60,     /* Size in octets */
    7500    /* Duration in microseconds*/
};

/****************************************************************************
Private Function Definitions
*/
static inline SWBS_DEC_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (SWBS_DEC_OP_DATA *) base_op_get_instance_data(op_data);
}

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Processing function for the swbs_dec operator.
 * \param op_data
 * \return touched sources
 */
unsigned swbs_dec_processing(OPERATOR_DATA *op_data)
{
    SWBS_DEC_OP_DATA* swbs_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA* sco_data = &(swbs_data->sco_rcv_op_data);
    unsigned ret_terminals = TOUCHED_NOTHING;
    unsigned frame_bytes;

    swbs_data->swbs_dec_output_samples = 0;

    while (enough_data_full_tag(sco_data, &frame_bytes)
           && enough_space_to_run(sco_data, OUTPUT_BLOCK_SIZE))
    {
        ret_terminals |= process_packet_payload(op_data, frame_bytes);
    }

    return ret_terminals;
}


/****************************************************************************
Private Function Definitions
*/

/**
 * \brief Process a packet from the input buffer.
 *        May produce a decoded frame or run plc on the output.
 *
 * \param op_data  Pointer to the operator data
 * \param packet_len_bytes  Received packet length in bytes
 * \return The terminals that the operator touched.
 */
static unsigned process_packet_payload(OPERATOR_DATA *op_data, unsigned packet_len_bytes)
{
    SWBS_DEC_OP_DATA* swbs_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA* sco_data = &(swbs_data->sco_rcv_op_data);
    metadata_status status = OK;
    bool data_avail = sco_dec_read_metadata(sco_data, packet_len_bytes, &status, &swbs_dec_frame_info);

    /* No data could be because of a discard. */
    if(!data_avail)
    {
        return TOUCHED_NOTHING;
    }

    /* Increment the frame count. (if something goes wrong frame_error_count is incremented) */
    sco_data->frame_count++;

    if (status == OK)
    {
        unsigned bytes_to_process = packet_len_bytes;
        unsigned return_code = TOUCHED_NOTHING;
        /* Process the received packet's audio payload(s). */
        while (bytes_to_process > 0)
        {
            return_code |= decode_packet(op_data, SWBS_FRAME_SIZE);
            bytes_to_process -= SWBS_FRAME_SIZE;
        }
        return return_code;
    }
#ifdef ESCO_SUPPORT_ERRORMASK
    else if (status == OK_WBM)
    {
        /* Process the received packet's audio payload and Weak Bit Mask data. */
        return decode_packet_wbm(op_data, packet_len_bytes);
    }
#endif
    else
    {
        sco_common_rcv_print_bad_status(base_op_get_ext_op_id(op_data), status, sco_data->frame_count);

        /* The current _packet_ is not OK */
        sco_data->frame_error_count++;

        discard_data_octets(sco_data, packet_len_bytes);

        /* All packets composing the encoded frame has been received.
         * The frame is bust so we will invoke PLC.
         * Before that, we reset the frame error status and the codec
         * internal status.
         */
        swbs_data->axInBuf.nRPtr = 0;
        swbs_data->axInBuf.nWPtr = 0;

        fake_packet_swbs(op_data);

        return TOUCHED_SOURCE_0;
    }

}

#ifndef UNIT_TEST_PROCESS_ENCODED_DATA


/**
 * \brief Decode a valid packet to the output buffer. Runs plc100 on the output.
 *
 * \param op_data        Pointer to the operator data
 * \param packet_length  Current packet length in octets
 * \return The terminals that the operator touched.
 */
static unsigned decode_packet(OPERATOR_DATA *op_data,
                              unsigned packet_length)
{
    SWBS_DEC_OP_DATA *swbs_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data = &(swbs_data->sco_rcv_op_data);
    unsigned swbs_packet_length = 0, amount_to_advance;
    int validate, output_packet_size;

    /* Validate the current packet. */
    validate = sco_decoder_swbs_validate(op_data, packet_length, &swbs_packet_length);

    SCO_DBG_MSG1("validate retval = %4d", validate);

    if ((validate == 240) || (validate == 180))
    {
        swbs_data->swbs_dec_output_samples += validate;

#ifdef SCO_DEBUG
        swbs_data->swbs_dec_validate_ret_good++;
#endif
    }
    else if (!((validate == 0) || (validate == 1) || (validate == 2)))
    {
        fault_diatribe(FAULT_AUDIO_INVALID_SWBS_VALIDATE_RESULT, validate);
    }

    patch_fn_shared(swbs_decode_processing);

    output_packet_size = sco_decoder_swbs_process(op_data, swbs_packet_length);

    SCO_DBG_MSG1("sco_decoder_swbs_process result = %4d", output_packet_size);

    /* sco_decoder_swbs_process does not increment the read pointer of the input buffer.
     * The sco packet length was checked in read_packet_metadata.
     *
     * Advance the read pointer by one packet length, the input buffer has already
     * been checked that it has one full packet
     */
    amount_to_advance = CONVERT_OCTETS_TO_SCO_WORDS(packet_length);
    cbuffer_advance_read_ptr(sco_data->buffers.ip_buffer, amount_to_advance);

    if (output_packet_size == 0)
    {
        return TOUCHED_NOTHING;
    }

    /* Update metadata first. */
    sco_fw_generate_output_metadata(sco_data, output_packet_size);

    cbuffer_advance_write_ptr(sco_data->buffers.op_buffer, output_packet_size);

    return TOUCHED_SOURCE_0;
}

#ifdef ESCO_SUPPORT_ERRORMASK
/**
 * \brief Decode a packet with Weak Bit Mask to the output buffer.
 *        Runs plc100 on the output.
 *
 * \param op_data        Pointer to the operator data
 * \param packet_length  Current packet length in octets
 * \return The terminals that the operator touched.
 */
static unsigned decode_packet_wbm(OPERATOR_DATA *op_data,
                              unsigned packet_length)
{
    SWBS_DEC_OP_DATA *swbs_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data = &(swbs_data->sco_rcv_op_data);
    unsigned swbs_packet_length = 0, amount_to_advance;
    int validate, output_packet_size;

    /* Validate the current packet. */
    // packet_length is the received blob size (SDU + WBM)
    // swbs_packet_length is the computed blob size (SDU + WBM)
    // validate is the number of audio samples that will be produced (samples)
    validate = sco_decoder_swbs_validate_wbm(op_data, packet_length, &swbs_packet_length);

    SCO_DBG_MSG1("validate retval = %4d", validate);

    if ((validate == 240) || (validate == 180)      // 60 byte packet
       || (validate == 480) || (validate == 360))   // 120 byte packet
    {
        swbs_data->swbs_dec_output_samples += validate;

#ifdef SCO_DEBUG
        swbs_data->swbs_dec_validate_ret_wbm++;
#endif
    }
    else if (!((validate == 0) || (validate == 1) || (validate == 2)))
    {
        fault_diatribe(FAULT_AUDIO_INVALID_SWBS_VALIDATE_RESULT, validate);
    }

    patch_fn_shared(swbs_decode_processing);

    output_packet_size = sco_decoder_swbs_process_bit_error(op_data, swbs_packet_length);

    SCO_DBG_MSG1("sco_decoder_swbs_process result = %4d", output_packet_size);

    /* sco_decoder_swbs_process does not increment the read pointer of the input buffer.
     * The sco packet length was checked in read_packet_metadata.
     *
     * Advance the read pointer by the size of the complete blob, the input buffer has already
     * been checked that it has one full audio packet
     */
    amount_to_advance = CONVERT_OCTETS_TO_SCO_WORDS(swbs_packet_length);
    cbuffer_advance_read_ptr(sco_data->buffers.ip_buffer, amount_to_advance);

    if (output_packet_size == 0)
    {
        return TOUCHED_NOTHING;
    }

    /* Update metadata first. */
    sco_fw_generate_output_metadata(sco_data, output_packet_size);

    /* Cbuffer write pointer is updated inside the sco_decoder_swbs_process_bit_error() */

    return TOUCHED_SOURCE_0;
}
#endif

/**
 * \brief Fakes a packet with aptX Adaptive PLC.
 *
 * \param op_data  Pointer to the operator data.
 * \return The terminals that the operator touched.
 */
unsigned fake_packet_swbs(OPERATOR_DATA *op_data)
{
    patch_fn(swbs_fake_packet);

#ifdef USE_PLC_MSBC
    SWBS_DEC_OP_DATA *swbs_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data = &(swbs_data->sco_rcv_op_data);
    unsigned packet_size = 0;

    if (swbs_data->codecMode == SWBS_CODEC_MODE4)
    {
        /* Mode 4 packet size is 180 samples */
        packet_size = 180;
    }
    else
    {
        /* Mode 0 packet size is 240 samples */
        packet_size = 240;
    }

    /* Keep metadata aligned with the buffer. */
    sco_fw_generate_output_metadata(sco_data, packet_size);

    /* Do packet loss concealment (PLC). */
    if (swbs_data->force_plc_off == 0)
    {
        swbs_run_plc(op_data, packet_size);
        return TOUCHED_SOURCE_0;
    }
    else
    {
        return TOUCHED_NOTHING;
    }

#else /* USE_PLC_MSBC */

    return TOUCHED_NOTHING;

#endif /* USE_PLC_MSBC */
}

#endif /* UNIT_TEST_PROCESS_ENCODED_DATA */

