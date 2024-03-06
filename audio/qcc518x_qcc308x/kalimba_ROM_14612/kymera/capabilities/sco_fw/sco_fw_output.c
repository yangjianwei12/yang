/****************************************************************************
 * Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \defgroup sco_fw_output Sco
 * \file  sco_fw_output.c
 *
 * SCO framework source for decoded audio output.
 *
 */

/****************************************************************************
Include Files
*/

#include "sco_fw_output.h"
#include "sco_common_funcs.h"
#include "capabilities.h"
#include "ttp/ttp.h"
#include "string.h"

/****************************************************************************
Private Constant Definitions
*/

/****************************************************************************
Private Macro Declarations
*/

/****************************************************************************
Private Variable Definitions
*/

/****************************************************************************
Private Function Definitions
*/
unsigned recovering_after_stall(tCbuffer *buffer,
                                unsigned frame_size,
                                unsigned amount_to_frame_boundary);
void discard_sco_data(tCbuffer *buffer, unsigned bytes_to_discard);
/****************************************************************************
Public Function Definitions
*/

/**
 * \brief  Checks if there is enough data in the input buffer. Number of
 *         required data bytes is taken from the metadata tag(s) length.
 *         Discard the octets before the start tag, to be aligned with the
 *         beginning of the frame.
 *
 * \param  sco_data    Pointer to the common SCO rcv operator data
 * \param  data_bytes  Returned metadata length.
 *
 * \return  TRUE if there is enough data in the input buffer, FALSE otherwise.
 */
bool enough_data_full_tag(SCO_COMMON_RCV_OP_DATA *sco_data,
                          unsigned *data_bytes)
{
    unsigned b4idx = 0;

    *data_bytes = buff_metadata_peek_frame_bytes(sco_data->buffers.ip_buffer,
                                                 &b4idx);

    if (b4idx > 0)
    {
        SCO_DBG_MSG1("LC3 decoder: discard %d bytes from input, before frame "
                     "boundary tag",
                     b4idx);
        discard_sco_data(sco_data->buffers.ip_buffer, b4idx);
    }

    if (*data_bytes == 0)
    {
        return FALSE;
    }

    return cbuffer_enough_data_to_run_ex(sco_data->buffers.ip_buffer,
                                         *data_bytes);
}

/**
 * \brief Checks if there is enough space in the output buffer.
 *
 * \param sco_data - Pointer to the common SCO rcv operator data
 * \param output_words - Words of data which need writing to the output.
 * \return If there is enough space, FALSE otherwise
 */
bool enough_space_to_run(SCO_COMMON_RCV_OP_DATA* sco_data, unsigned output_words)
{
    unsigned output_space;

    /* calc output space */
    output_space = cbuffer_calc_amount_space_in_words(sco_data->buffers.op_buffer);

    SCO_DBG_MSG1("enough_space_to_run: output buffer space |%4d|.", output_space);

    /* Check for run condition.
     * Only one packet is decoded per run.
     */
    if (output_words > output_space)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/**
 * \brief Add kymera metadata tags associated
 *        to the generated output buffer data.
 *
 * \param sco_data         Pointer to the common SCO rcv operator data.
 * \param output_samples   Amount of samples produced.
 * \param tag_info         Time info for the output metadata tag.
 */
void sco_fw_generate_output_metadata(SCO_COMMON_RCV_OP_DATA *sco_data,
                                     unsigned output_samples)
{
    metadata_tag *out_mtag = NULL;
    unsigned b4idx, afteridx;
    unsigned out_len, sp_adjust;
    SCO_COMMON_RCV_METADATA_HIST_OUT *out_tag_info = &sco_data->out_tag;
    tCbuffer *op_buffer = sco_data->buffers.op_buffer;
    TIME timestamp;

    patch_fn_shared(sco_dec_read_metadata);
    if ((output_samples == 0) || !buff_has_metadata(op_buffer))
    {
        /* No output is generated, or output buffer
         * doesn't support metadata at all.
         */
        return;
    }

    /* Length for this tag. */
    out_len = output_samples * OCTETS_PER_SAMPLE;
    /* Set the sample rate adjustment.*/
    sp_adjust = out_tag_info->spa;
    /* Calculate the timestamp */
    if (out_tag_info->nrof_samples == 0)
    {
        /* Transport ToA from input to output.
         * In good days this should always be the case.
         * nrof_samples is cleared when we use the information
         * from the input tag. */
        timestamp = out_tag_info->timestamp;
    }
    else
    {
        /* We have an input tag but we can't use the stored ToA.
         * In two cases this could happen:
         *  - PLC is generating output without any input received
         *  - or less likely, input tag is larger than the amount
         *    processed by the operator
         *  Anyway, in this case we calculate the new time stamp from
         *  the previous tag info (timestamp and number of samples
         *  previously generated).
         */
        timestamp = ttp_get_next_timestamp(out_tag_info->timestamp,
                                           out_tag_info->nrof_samples,
                                           sco_data->sample_rate,
                                           out_tag_info->spa);
    }
#if defined(SCO_DEBUG) && (defined(SCO_DEBUG_PRINTF) || defined(SCO_DEBUG_LOG))
    {
        SCO_DBG_MSG3("SCO_RCV_TRANSPORT_METADATA, Appending output "
                     "tag, with timestamp = 0x%08x"
                     "(%dus in the past), Valid = %d",
                     timestamp,
                     time_sub(time_get_time(), timestamp),
                     out_tag_info->valid_timestamp);
    }
#endif

    /* One tag for each chunk of produced output. */
    if (sco_data->cur_tag != NULL)
    {
        /* Reuse tag from input. */
        out_mtag = sco_data->cur_tag;
        sco_data->cur_tag = NULL;
    }
    else
    {
        /* Create a new tag for the output */
        out_mtag = buff_metadata_new_tag();
    }

    /* make sure we have a tag for the output */
    PL_ASSERT(out_mtag != NULL);

    out_mtag->length = out_len;
    out_mtag->sp_adjust = sp_adjust;

    if (out_tag_info->forward_ttp)
    {
        /* Generate TTP Tag. */
        METADATA_TIMESTAMP_SET(out_mtag,
                                timestamp,
                                METADATA_TIMESTAMP_LOCAL);
    }
    else if (sco_data->generate_timestamp)
    {
        /* Convert ToA to TTP by adding constant offset. */
        TIME ts = time_add(timestamp,
                           sco_data->timestamp_latency);
        METADATA_TIMESTAMP_SET(out_mtag, ts, METADATA_TIMESTAMP_LOCAL);
    }
    else
    {
        /* Generate ToA Tag. */
        METADATA_TIME_OF_ARRIVAL_SET(out_mtag, timestamp);
    }

    /* Append generated metadata to the output buffer. */
    b4idx = 0;
    afteridx = out_len;
    buff_metadata_append(sco_data->buffers.op_buffer,
                         out_mtag,
                         b4idx,
                         afteridx);

    /* Next tag can use current tag info, this is useful
     * when running PLC multiple time or when a packet contains
     * more than 1 frame.*/
    out_tag_info->nrof_samples += output_samples;
}

metadata_status get_cumulative_status(metadata_status old_status,
                                             metadata_status cur_status)
{

    if (old_status == OK)
    {
        /* Any error status in the current tag will be equal or worse than
         * old_status. Be pessimistic and use cur_status */
        return cur_status;
    }

    if ((old_status == CRC_ERROR) && (cur_status != OK))
    {
        /* Any error status in the current tag will be equal or worse than
         * old_status. Be pessimistic and use cur_status */
        return cur_status;
    }

    /* At this point, any error status in the current tag will be
     * equal or less serious than old_status.
     * Be pessimistic and use old_status  */
    return old_status;
}


/**
 * Function check if the buffer has any tags signalling that
 * data were discarded.
 *
 * NOTE: data_before_discard and discarded_data values should be ignored if the
 * function returns false.
 */
static bool was_data_discarded(tCbuffer *buffer, unsigned frame_size, unsigned *data_before_discard, unsigned *discarded_data)
{
    metadata_tag *mtag;
    unsigned struct_length;
    SCO_PRIVATE_METADATA *priv_metadata;

    patch_fn_shared(sco_dec_read_metadata);
    /* Make sure the initial state is cleared.*/
    *data_before_discard = 0;
    *discarded_data = 0;

    /* Search for tags with discarded info. */
    mtag = buff_metadata_peek_ex(buffer, data_before_discard);
    while((mtag != NULL) && ((*data_before_discard) < frame_size))
    {
        if (buff_metadata_find_private_data(mtag, META_PRIV_KEY_SCO_DATA_STATUS, &struct_length, (void *)&priv_metadata))
        {
            /* Sanity check if the tag is indeed a discarded data tag.*/
            if ((priv_metadata->discarded_data != 0) && (struct_length == sizeof(SCO_PRIVATE_METADATA)))
            {
                *discarded_data = priv_metadata->discarded_data;
                /* once the discarded amount is read remove it from the tag.*/
                priv_metadata->discarded_data = 0;
                return TRUE;
            }
        }

        /* Increment the before discard tag data and
         * go to the next tag.*/
        *data_before_discard += mtag->length;
        mtag = mtag->next;
    }
    return FALSE;
}

/**
 * Function discards data and metadata from a buffer.
 *
 */
void discard_sco_data(tCbuffer *buffer, unsigned bytes_to_discard)
{
    unsigned amount_available = cbuffer_calc_amount_data_ex(buffer);

    /* Something is terribly wrong if the bytes_to_discard is bigger than amount_available */
    PL_ASSERT(amount_available >= bytes_to_discard);

    cbuffer_advance_read_ptr_ex(buffer, bytes_to_discard);
    metadata_strict_transport(buffer, NULL, bytes_to_discard);
}

/**
 * Function to align frames and packets after a discard due to chain stalls.
 */
static unsigned align_frames_with_packets(tCbuffer *buffer, unsigned amount_to_frame_boundary, unsigned frame_size)
{
    unsigned octets_b4idx;
    unsigned available_octets = MIN(
                cbuffer_calc_amount_data_ex(buffer),
                buff_metadata_available_octets(buffer));

    while((amount_to_frame_boundary != 0) && (available_octets >= amount_to_frame_boundary))
    {
        metadata_tag *mtag;
        L2_DBG_MSG2("SCO align frames with packet boundary. Frame size %d, amount to discard %d.",
                           frame_size, amount_to_frame_boundary);
        /* First discard to frame boundary */
        discard_sco_data(buffer, amount_to_frame_boundary);
        /* Decrease the available octets */
        available_octets -= amount_to_frame_boundary;
        amount_to_frame_boundary = 0;

        /* Check if the frame is aligned with the packet. */
        mtag = buff_metadata_peek_ex(buffer, &octets_b4idx);

        if (mtag == NULL)
        {
            if (available_octets != 0)
            {
                /* No more tags so we cannot calculate the octets_b4idx,
                 * however there are some data left from the previous tag. */
                amount_to_frame_boundary = frame_size;
            }
        }
        else if(octets_b4idx != 0)
        {
            /* hopefully the next frame boundary will also be a packet boundary */
            amount_to_frame_boundary = frame_size;
        }
    }

    return amount_to_frame_boundary;
}

/**
 * Function used to realign frames and packet after a stalls.
 */
unsigned recovering_after_stall(tCbuffer *buffer,
                                unsigned frame_size,
                                unsigned amount_to_frame_boundary)
{
    unsigned data_before_discard;
    unsigned discarded_data;
    unsigned total_amount_discarded;


    if (was_data_discarded(buffer, frame_size, &data_before_discard, &discarded_data))
    {
        discard_sco_data(buffer, data_before_discard);

        /* Toatal amount of data discarded from the buffer. */
        total_amount_discarded = data_before_discard + discarded_data;
        /* Add the amount to frame boundary which left from a previous
         * discard.
         */
        if(amount_to_frame_boundary > 0)
        {
            total_amount_discarded += frame_size - amount_to_frame_boundary;
        }

        /* Remove entire frames from the discarded data. */
        if (total_amount_discarded > frame_size)
        {
            total_amount_discarded = total_amount_discarded % frame_size;
        }
        /* Calculate the remaining octets to the frame boundary.
         * amount_to_frame_boundary = (frame_size - total_amount_discarded)% frame_size;
         * but we know that total_amount_discarded < frame_size so simplify*/
        if (total_amount_discarded == 0)
        {
            amount_to_frame_boundary = 0;
        }
        else
        {
            amount_to_frame_boundary = frame_size - total_amount_discarded;
        }
        /* Log this event because it is important. */
        L2_DBG_MSG3("SCO Driver stalled and discarded %d octets.\n"
                          "  Discarding %d octets of data before the stall.\n"
                          "  Amount to next next frame boundary is %d octets",
                          discarded_data, data_before_discard, amount_to_frame_boundary);
    }

    patch_fn_shared(sco_dec_read_metadata);
    if (amount_to_frame_boundary != 0)
    {
        amount_to_frame_boundary = align_frames_with_packets(buffer, amount_to_frame_boundary, frame_size);
    }
    return amount_to_frame_boundary;
}

/**
 * \brief Saves tag information if the tag has ToA or TTP flag set.
 *
 * \param sco_data      Pointer to the common SCO rcv operator data
 * \param in_mtag       Pointer to the metadata tag
 * \param b4idx         Number of octets before the current tag, previously
 *                      returned from a buff_metadata_remove call
 * \param afteridx      Number of octets consumed, previously returned
 *                      from a buff_metadata_remove call
 * \param input_octets  Amount of octets consumed from the input
 *
 * \return TRUE if the tag info was saved, FALSE otherwise.
 */
static bool sco_dec_save_tag_info(SCO_COMMON_RCV_OP_DATA *sco_data,
                                  metadata_tag *in_mtag,
                                  unsigned b4idx,
                                  unsigned input_octets,
                                  const SCO_ISO_FRAME_INFO *frame_info)
{
    patch_fn_shared(sco_dec_read_metadata);

    /* If the input tag is TTP then there is no need to convert the
     * TOA tags to TTP. The tag's TTP information will be forwarded.
     */
    sco_data->out_tag.forward_ttp = IS_TIME_TO_PLAY_TAG(in_mtag);

    if ((IS_TIME_OF_ARRIVAL_TAG(in_mtag) || IS_TIME_TO_PLAY_TAG(in_mtag)))
    {
        TIME_INTERVAL ahead_time = 0;

        /* This tag contains timing info that will be propagated to the
         * output.
         * If b4idx > 0, the tag time info doesn't exactly match the
         * requested data. However, as long as the frames are fixed length 
         * we can attempt to reconstruct the correct timing */
        if ((b4idx > 0) && (!frame_info->whole_frame))
        {
            /* The frame size needs to be known and fixed for this to work */
            PL_ASSERT(frame_info->frame_size_octets > 0);
            ahead_time = (frame_info->frame_duration_us * b4idx) / frame_info->frame_size_octets;
        }

        sco_data->out_tag.valid_timestamp = TRUE;
        sco_data->out_tag.timestamp = time_sub(in_mtag->timestamp, ahead_time);
        sco_data->out_tag.spa = in_mtag->sp_adjust;
        sco_data->out_tag.nrof_samples = 0;
        SCO_DBG_MSG5("SCO_RCV_TRANSPORT_METADATA, New tag read, "
                        "valid_timestamp=TRUE, input=%d, toa=%d, "
                        "sp_adjust=%d, before=%d, offset=%d",
                        input_octets,
                        in_mtag->timestamp,
                        in_mtag->sp_adjust,
                        b4idx,
                        ahead_time);

        return TRUE;
    }
    else
    {
        /* This tag has not been generated normally.
         * There shouldn't be any use case where we find such a tag.
         * Anyway, tag info isn't suitable to be passed to the output.
         */
        SCO_DBG_MSG1("SCO_RCV_TRANSPORT_METADATA, Non timestamped tag read, "
                     "input=%d",
                     input_octets);

        return FALSE;
    }
}

/** Save the tag to avoid allocation a new tag */
static void sco_dec_store_cur_tag(SCO_COMMON_RCV_OP_DATA *sco_data,
    metadata_tag *in_mtag)
{
    if ((in_mtag == NULL) || (sco_data == NULL))
    {
        SCO_DBG_MSG("Invalid params");
        return;
    }

    /* It is possible that the decoder will fail to
     * generate output after an input metadata read.
     * So the current tag might stay from a previous packet. */
    if (sco_data->cur_tag == NULL)
    {
        /* Save the tag. */
        sco_data->cur_tag = in_mtag;
    }
    else
    {
        SCO_DBG_MSG1("SCO_RCV_TRANSPORT_METADATA, Input tag deleted, "
            "toa=0x%08x, ",
            in_mtag->timestamp);
        buff_metadata_delete_tag(in_mtag, TRUE);
    }

    /* Private data of the reused tag should not propagate downstream. */
    pdelete(sco_data->cur_tag->xdata);
    /* We have all the data that we need now, we can blank it. */
    memset(sco_data->cur_tag, 0, sizeof(metadata_tag));
}

/**
* sco_dec_read_metadata
*/
bool sco_dec_read_metadata(SCO_COMMON_RCV_OP_DATA *sco_data,
                           unsigned input_octets,
                           metadata_status *ret_status,
                           const SCO_ISO_FRAME_INFO *frame_info)
{
    metadata_tag *in_mtag = NULL, *save_tag = NULL;
    metadata_status frame_status = OK;
    metadata_status status;
    unsigned b4idx, afteridx;
    tCbuffer *ip_buffer = sco_data->buffers.ip_buffer;
    bool whole_frame = frame_info->whole_frame;

    patch_fn_shared(sco_dec_read_metadata);

    if ((input_octets == 0) || !buff_has_metadata(ip_buffer))
    {
        /* No input was processed or input buffer
         * doesn't support metadata at all.
         */
        SCO_DBG_MSG1("Metadata not supported or no data, input_octets=%d",
                     input_octets);
        return FALSE;
    }

    if (!whole_frame)
    {
        /* Check if the driver discarded any data due to a stall and try to
         * sync.
         */
        sco_data->amount_to_frame_boundary =
            recovering_after_stall(ip_buffer,
                                   input_octets,
                                   sco_data->amount_to_frame_boundary);

        if (sco_data->amount_to_frame_boundary > 0)
        {
            return FALSE;
        }
    }

    /* Something has been consumed from input, remove the relevant tag. */
    in_mtag = buff_metadata_remove(ip_buffer,
                                   input_octets,
                                   &b4idx,
                                   &afteridx);

    if (in_mtag == NULL)
    {
        if (whole_frame)
        {
            /* No Metadata. This was not expected */
            SCO_DBG_MSG("No Metadata available");
            return FALSE;
        }
        else
        {
            /* Old tag was finished.*/
            SCO_DBG_MSG3("Null tag read, input=%d, before=%d, after=%d",
                         input_octets,
                         b4idx,
                         afteridx);
            frame_status = sco_data->old_sco_status;
            *ret_status = frame_status;
            return TRUE;
        }
    }

    if (b4idx > 0)
    {
        if (whole_frame)
        {
            /* This was not expected. The previous tag wasn't finished */
            SCO_DBG_MSG1("Discarding %d octets from previous metadata tag",
                         b4idx);
            discard_sco_data(ip_buffer, b4idx);
        }
        else
        {
            /* We started reading a new frame and the previous tag wasn't
             * finished yet, which means that it covers some data from the new
             * frame. Consider the previous status.
             */
            frame_status = sco_data->old_sco_status;
        }
    }

    if (whole_frame && (METADATA_PACKET_START(in_mtag) == 0))
    {
        SCO_DBG_MSG("Tag does not contain the expected frame boundary flag!");
        return FALSE;
    }

    while (in_mtag != NULL)
    {
        metadata_tag *tag = in_mtag;

        /* Remove it from the list */
        in_mtag = in_mtag->next;

        /* Save the tag's information if the tag has ToA flag set.
         * In case of walking through multiples tags for one frame worth of
         * data, if more than one tag would have ToA info, just keep the
         * first one. Otherwise, we expect the current tag to have ToA info.
         */
        if (save_tag == NULL)
        {
            bool res = sco_dec_save_tag_info(sco_data,
                                             tag,
                                             b4idx,
                                             input_octets,
                                             frame_info);
            if (res)
            {
                save_tag = tag;
            }
        }

        /* Read its private metadata */
        if (!sco_common_retrieve_metadata_status(tag, &status))
        {
            /* If no status has been provided, maybe we are not connected to
             * the sco driver. Let's assume the data is good. */
            status = OK;
        }

        /* Update the cumulative status for the frame */
        frame_status = get_cumulative_status(frame_status, status);

        if (tag != save_tag)
        {
            SCO_DBG_MSG1("Input tag deleted, toa=0x%08x", tag->timestamp);
            buff_metadata_delete_tag(tag, TRUE);
        }

        if (whole_frame)
        {
            if (METADATA_PACKET_END(tag))
            {
                /* There shouldn't be more tags left after PACKET_END tag. */
                if (in_mtag != NULL)
                {
                    SCO_DBG_MSG("Metadata tag found after PACKET_END!");
                }

                break;
            }
        }
        else
        {
            /* Update the old_sco_status with the current read status, so that
             * it always contain the value of the latest read tag.
             */
            sco_data->old_sco_status = status;
        }
    }

    /* Save the tag to avoid allocation a new tag when
     * sco_fw_generate_output_metadata is called.
     */
    sco_dec_store_cur_tag(sco_data, save_tag);

    *ret_status = frame_status;

    return TRUE;
}
