/****************************************************************************
 * Copyright (c) 2015 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  wbs_dec_data_processing_c.c
 * \ingroup  operators
 *
 *  WBS decode operator
 *
 */
/****************************************************************************
Include Files
*/
#include "wbs_private.h"

/****************************************************************************
Private Constant Definitions
*/
#define INPUT_BLOCK_SIZE                           (30)
#define OUTPUT_BLOCK_SIZE                         (120)
#define OUTPUT_TWO_BLOCK_SIZE                     (240)

/* Threshold for running PLC to compensate for frame alignment */
#define WBS_MIN_ALIGN_FOR_PLC 10

/* Each 60 octet frame encodes 120 samples */
#define WBS_SAMPLES_PER_ENCODED_OCTET 2

/****************************************************************************
Private Type Definitions
*/

/****************************************************************************
Private Function Declarations
*/

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

/* WBS decoder frame format info, used when reading metadata */
static const SCO_ISO_FRAME_INFO wbs_dec_frame_info = 
{
    FALSE,  /* Not required to process whole frames only */
    60,     /* Size in octets */
    7500    /* Duration in microseconds*/
};

/****************************************************************************
Private Function Definitions
*/
static inline WBS_DEC_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (WBS_DEC_OP_DATA *) base_op_get_instance_data(op_data);
}

#ifndef UNIT_TEST_PROCESS_ENCODED_DATA

/**
 * Checks if the codec library has sync.
 *
 * Returns TRUE if codec has sync FALSE if sync is lost.
 */
static inline bool WBS_has_sync(WBS_DEC_OP_DATA* wbs_data)
{
    return wbs_data->codec_data->fields4[WBS_SYNC_FIELD_POS_FIELDS4] != 0;
}


/**
 * Check if the from air records covering a WBS frame has an OK overall status.
 *
 * \param sco_data      Pointer to the common SCO rcv operator data.
 *
 * \return TRUE if status for next WBS frame is OK, FALSE otherwise.
 */
static bool WBS_next_status_OK(SCO_COMMON_RCV_OP_DATA* sco_data)
{

    metadata_tag *in_mtag;
    unsigned b4idx;
    metadata_status status;
    unsigned total_length;
    tCbuffer *ip_buffer = sco_data->buffers.ip_buffer;

    /* Peek the metadata for a lookup.*/
    in_mtag  = buff_metadata_peek_ex(ip_buffer, &b4idx);
    patch_fn_shared(wbs_dec);
    if (b4idx != 0)
    {
        /* Didn't finish last tag. */
        if (sco_data->old_sco_status != OK)
        {
            /* Last tag's status is not OK. */
            return FALSE;
        }
        total_length = b4idx;
    }
    else
    {
        total_length = 0;
    }

    /* Check all the relevant tags status.
     * Note: no need to use get_cumulative_status because the function exits
     * as soon as a status is not OK. */
    while((in_mtag != NULL) && (total_length < WBS_DEFAULT_ENCODED_BLOCK_SIZE_OCTETS))
    {
        /* Read its private metadata */
        if (!sco_common_retrieve_metadata_status(in_mtag, &status))
        {
            /* If no status has been provided, maybe we are not connected to
             * the sco driver. Let's assume the data is good. */
            status = OK;
        }
        /* Exit as soon as the status is not OK.
         * NOTE: There is no reason to check for discarded data because the
         * status is not OK in those cases.*/
        if (status != OK)
        {
            /* Tag's status is not OK. */
            return FALSE;
        }

        /* Go to the next tag.*/
        total_length += in_mtag->length;
        in_mtag = in_mtag->next;
    }

    /* The next status is OK return TRUE. */
    return TRUE;
}

/**
 * Discards data and metadata until the next sync word when
 * the library looses the sync. This is helpful to avoid wrong
 * plc calls when the operator runs in unaligned mode.
 *
 * Returns number of octets discarded (or zero, if none).
 */
static unsigned align_wbs_frame_for_lib(WBS_DEC_OP_DATA* wbs_data, SCO_COMMON_RCV_OP_DATA* sco_data)
{
    tCbuffer *ip_buffer;
    unsigned octets_to_sync;

    /* If the library knows about the sync just exit.
     * We only align if sync is lost. */
    if (WBS_has_sync(wbs_data))
    {
        return 0;
    }
    /* Make sure we only search for sync if the status for the
     * overall records containing a WBS frame is OK. */
    if (!WBS_next_status_OK(sco_data))
    {
        return 0;
    }

    ip_buffer = sco_data->buffers.ip_buffer;
    /* Get the amount of octets to the next sync. */
    octets_to_sync = wbs_octets_to_sync(ip_buffer);

    patch_fn_shared(wbs_dec);
    /* We know that the library only works in words so check if the
     * metadata is already aligned to the sync. */
    if (octets_to_sync == 1)
    {
        unsigned buff_offset = cbuffer_get_usable_data_read_offset(ip_buffer);
        unsigned meta_buff_offset = buff_metadata_get_read_offset(ip_buffer);
        if (meta_buff_offset == buff_offset + 1)
        {
            /* metadata already aligned with WBS frame.
             * Don't discard that extra octet from the buffer because it will be ignored anyways.*/
            SCO_DBG_MSG2("align_wbs_frame_for_lib meta_buff_offset (%d) == buff_offset (%d) + 1",
                    meta_buff_offset, buff_offset);
            return 0;
        }
    }

    /* If sync word found (octets_to_sync < WBS_DEFAULT_ENCODED_BLOCK_SIZE_OCTETS)
     * discard data until it. */
    if ((octets_to_sync > 0) && (octets_to_sync < WBS_DEFAULT_ENCODED_BLOCK_SIZE_OCTETS))
    {
        metadata_status ret_status;
#ifdef SCO_DEBUG
        wbs_data->align_to_sync_count += 1;
        wbs_data->discarded_until_sync = octets_to_sync;
#endif
        L2_DBG_MSG1("align_wbs_frame_for_lib: Discard %d octets of input data to align to WBS frame start",
                octets_to_sync);
        sco_dec_read_metadata(sco_data, octets_to_sync, &ret_status, &wbs_dec_frame_info);
        if (ret_status == OK_WBM)
        {
            SCO_DBG_ERRORMSG1("WBS: Weak bit mask based error correction is unsupported, Frame count %d", wbs_data->sco_rcv_op_data.frame_count);
            fault_diatribe(FAULT_AUDIO_WBM_NOT_SUPPORTED, wbs_data->sco_rcv_op_data.frame_count);
        }
        /* Advance the read pointer. */
        cbuffer_advance_read_ptr_ex(ip_buffer, octets_to_sync);

        return octets_to_sync;
    }
    return 0;
}
#else
extern unsigned align_wbs_frame_for_lib(WBS_DEC_OP_DATA* wbs_data, SCO_COMMON_RCV_OP_DATA* sco_data);
#endif
/****************************************************************************
Public Function Definitions
*/

/**
 * Processing function for the wbs_dec operators.
 *
 * \param op_data  Pointer to the operator data
 * \return The terminals that the operator touched.
 */
unsigned wbs_dec_processing(OPERATOR_DATA *op_data)
{
    WBS_DEC_OP_DATA* wbs_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA* sco_data = &(wbs_data->sco_rcv_op_data);
    unsigned ret_terminals = TOUCHED_NOTHING;
    unsigned frame_bytes;

    /* Init run specific variables for WBS */
    wbs_data->wbs_dec_output_samples = 0;
    frame_bytes = CONVERT_SCO_WORDS_TO_OCTETS(INPUT_BLOCK_SIZE);

    while (cbuffer_enough_data_to_run_ex(sco_data->buffers.ip_buffer,
                                         frame_bytes)
           && enough_space_to_run(sco_data, OUTPUT_BLOCK_SIZE))
    {
        ret_terminals |= wbs_dec_process_packet_payload(op_data, frame_bytes);
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
 * \return The terminals that the operator touched.
 */
unsigned wbs_dec_process_packet_payload(OPERATOR_DATA *op_data,
                                        unsigned frame_bytes)
{
    WBS_DEC_OP_DATA* wbs_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA* sco_data = &(wbs_data->sco_rcv_op_data);
    metadata_status status = OK;
    unsigned align_amount;

    bool data_avail;

    patch_fn_shared(wbs_dec);
    /* Check if the data is aligned for the library.
     * If data was discarded (align_wbs_frame_for_lib returns nonzero) 
     * re-check the input to see if a frame can be decoded, 
     * and run PLC otherwise.
     */
    align_amount = align_wbs_frame_for_lib(wbs_data, sco_data);

    if (align_amount > 0)
    {
        L2_DBG_MSG1("wbs_dec_process_packet_payload remaining data after alignment= %d", 
            cbuffer_calc_amount_data_ex(sco_data->buffers.ip_buffer));

#if defined (INSTALL_PLC100) && !defined (WBS_UNIT_TEST)
        if (align_amount > WBS_MIN_ALIGN_FOR_PLC)
        {
            /* Run PLC to make up for the data discarded */
            unsigned plc_samples = align_amount * WBS_SAMPLES_PER_ENCODED_OCTET;
            if (plc_samples > OUTPUT_BLOCK_SIZE)
            {
                plc_samples = OUTPUT_BLOCK_SIZE;
            }

            sco_fw_generate_output_metadata(sco_data, plc_samples);
            sco_plc100_call_plc(wbs_data->force_plc_off,
                                wbs_data->plc100_struc,
                                NOTHING_RECEIVED,
                                plc_samples);
            return TOUCHED_SOURCE_0;
        }
        else
#endif
        {
            return TOUCHED_NOTHING;
        }
    }

    data_avail = sco_dec_read_metadata(sco_data, frame_bytes, &status, &wbs_dec_frame_info);

    /* No data could be because of a discard. */
    if(!data_avail)
    {
        return TOUCHED_NOTHING;
    }

    /* Increment the frame count. (if something goes wrong frame_error_count is incremented) */
    sco_data->frame_count++;

    if (status == OK_WBM)
    {
        SCO_DBG_ERRORMSG1("WBS: Weak bit mask based error correction is unsupported, Frame count %d", sco_data->frame_count);
        fault_diatribe(FAULT_AUDIO_WBM_NOT_SUPPORTED, sco_data->frame_count);
    }

    if (status != OK)
    {
        sco_common_rcv_print_bad_status(base_op_get_ext_op_id(op_data), status, sco_data->frame_count);
        /* We received some packet. Since the frame is bust, we don't need
         * this packet, so we discard it. */
        discard_data_octets(sco_data, frame_bytes);

        /* All packets composing the encoded frame has been received.
         * The frame is bust so we will invoke PLC.
         * Before that, we reset the frame error status and the codec
         * internal status.
         */

        /* reset the wbs decoder */
        sco_decoder_wbs_initialize(wbs_data->codec_data);

#ifdef INSTALL_PLC100
        /* keep metadata aligned with the buffer */
        sco_fw_generate_output_metadata(sco_data, OUTPUT_BLOCK_SIZE);

        /* Do packet loss concealment (PLC). */
        sco_plc100_call_plc(wbs_data->force_plc_off,
                            wbs_data->plc100_struc,
                            NOTHING_RECEIVED,
                            OUTPUT_BLOCK_SIZE);

        return TOUCHED_SOURCE_0;
#else
        return TOUCHED_NOTHING;
#endif /* INSTALL_PLC100 */
    }
    else
    {
        /* Process the encoded frame. */
        return decode_packet(op_data, frame_bytes);
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
    WBS_DEC_OP_DATA* wbs_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA* sco_data = &(wbs_data->sco_rcv_op_data);
    unsigned wbs_packet_length = 0, amount_to_advance;
    int amount_advanced = 0;
    int validate, result;
#ifdef INSTALL_PLC100
    unsigned bfi;
    unsigned amount_generated;
#endif /* INSTALL_PLC100 */

    /* Validate the current packet. */
    validate = sco_decoder_wbs_validate(op_data,
                                        packet_length,
                                        &wbs_packet_length,
                                        &amount_advanced);

    /* Normally we are in sync and amount_advanced is 0, however if we lose
     * sync wbs_validate function will search for the location of WBS sync. In
     * that case sync normally is found at the beginning of the SCO
     * payload (so amount_advanced = 0), however it is absolutly legal for WBS
     * frames to be unaligned with SCO packets. In that case wbs_validate will
     * adavance the buffer to sync point (so amount_advanced>0).
     *
     * There is also possibility that WBS sync is found to be crossing the
     * boundray of SCO packet. In that case wbs_validate will move the
     * buffer read_ptr back by one word instead (amount_advanced = -1)
     *
     */

    patch_fn_shared(wbs_dec);
    /* Before calling this function we only know that we have one full packet.
     * The validate function cannot go beyond one full packet.
     */
    PL_ASSERT(amount_advanced >= -1 &&
              amount_advanced <=  (int)CONVERT_OCTETS_TO_SCO_WORDS(packet_length));
#ifdef SCO_DEBUG
    /* Save WBS validate return values.*/
    if (amount_advanced > 0)
    {
        wbs_data->wbs_dec_validate_advanced++;
    }
    if ((validate == 0) || (validate == 1) || (validate == 2))
    {
        wbs_data->wbs_dec_validate_ret.error_case[validate]++;
    }
#endif
    if ((validate == 120) || (validate == 240))
    {
        wbs_data->wbs_dec_output_samples += validate;
    }
    else if (!((validate == 0) || (validate == 1) || (validate == 2)))
    {
        fault_diatribe(FAULT_AUDIO_INVALID_WBS_VALIDATE_RESULT, validate);
        /* Increment the number of frames.*/
        sco_data->frame_error_count++;
    }

    result = sco_decoder_wbs_process(op_data, OK, validate, wbs_packet_length);

    patch_fn_shared(wbs_dec);
    print_wbs_validate_and_process_result(validate, amount_advanced, wbs_packet_length, result);

    /* sco_decoder_wbs_process does not increment the read pointer of the input buffer.
     * The sco packet length was checked in read_packet_metadata.
     *
     * Advance the read pointer by one packet length, the input buffer has already
     * been checked that it has one full packet, however part of that might have
     * been consumed by the validate function.
     */
    amount_to_advance = packet_length;

    /* Part of packet might already be advanced by validate function. */
    amount_to_advance -= CONVERT_SCO_WORDS_TO_OCTETS(amount_advanced);
    cbuffer_advance_read_ptr_ex(sco_data->buffers.ip_buffer, amount_to_advance);
    if (result == WBS_DEC_PROCESS_NO_OUTPUT)
    {
#ifdef WBS_UNIT_TEST
        return TOUCHED_NOTHING;
#else
        /* The decoder didn't produce anything, but we still want some output */
        result = WBS_DEC_PROCESS_FAKE_FRAME;
#endif
    }

#ifdef INSTALL_PLC100
    if (result == WBS_DEC_PROCESS_FAKE_FRAME)
    {
        bfi = NOTHING_RECEIVED;
    }
    else
    {
        /* ok, crc error, or nothing received */
        bfi = result;
    }


    /* Check if the validate returned a valid block size,
     * if not use the default one block.
     */
    if ((validate == OUTPUT_BLOCK_SIZE) || (validate == OUTPUT_TWO_BLOCK_SIZE))
    {
        amount_generated = validate;
    }
    else
    {
        amount_generated = OUTPUT_BLOCK_SIZE;

        /* This should never happen if the result is zero. */
        if (result == WBS_DEC_PROCESS_OK)
        {
            wbs_data->wbs_dec_invalid_validate_result += 1;
            L2_DBG_MSG2("Invalid WBS validate result count=%d, validate_value=%d",
                        wbs_data->wbs_dec_invalid_validate_result,
                        validate);
        }
    }

    /* Update metadata first. */
    sco_fw_generate_output_metadata(sco_data, amount_generated);

    /* Do packet loss concealment (PLC). */
    sco_plc100_call_plc(wbs_data->force_plc_off, wbs_data->plc100_struc,
                        bfi, amount_generated);

#else /* No plc */

    /* if result == OK:
     * sco_decoder_wbs_process does not increment the write pointer of the output buffer.
     * That is the plc100 job but now is missing so we should do it now.
     *
     * if result != OK:
     * No output from the decode and no PLC. Just increment the write pointer of the
     * output buffer.
     * 
     * TODO maybe inserting silences is better.
     */

    /* Update metadata first. */
    sco_fw_generate_output_metadata(sco_data, validate);

    cbuffer_advance_write_ptr(sco_data->buffers.op_buffer, validate);
#endif /* INSTALL_PLC100 */

    return TOUCHED_SOURCE_0;
}

/**
 * \brief Reset the internals of the decoder
 *
 * \param op_data  Pointer to the operator data
 */
void wbs_dec_reset_sbc_data(OPERATOR_DATA* op_data)
{
    WBS_DEC_OP_DATA *wbs_data = get_instance_data(op_data);

    /* initialise the wbs decoder */
    sco_decoder_wbs_initialize(wbs_data->codec_data);

    /* reset the sbc decoder */
    sbcdec_reset_decoder(wbs_data->codec_data);

    /* now init the tables */
    sbcdec_init_tables(wbs_data->codec_data);
}

#endif /* UNIT_TEST_PROCESS_ENCODED_DATA */
