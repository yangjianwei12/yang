/****************************************************************************
 * Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
#include "aac/aac_c.h"
#include "rtp_decode_private.h"
#include "rtp_decode_struct.h"
#include "unpack_cbuff_to_array.h"
#include "opmgr/opmgr_for_ops.h"

/**
 * Decodes the AAC frame and populates the frame data structure with the necessary
 * information to generate a timestamped tags.
 *
 * \param opx_data Pointer to the RTP operator data.
 * \param payload_size Pointer to the RTP operator data.
 * \param frame_data Pointer to the frame data structure.
 */
static void aac_sample_count(RTP_DECODE_OP_DATA *opx_data, unsigned payload_size,
        RTP_FRAME_DECODE_DATA *frame_decode_data)
{
    tCbuffer *clone_buffer;
    aac_codec *aac_codec_struc = (aac_codec *)opx_data->aac_codec;
    dummy_decoder aac_decoder;
    unsigned int offset;

    patch_fn_shared(rtp_decode);

    /* Select the internal buffers based on the packing */
    if (opx_data->pack_latency_buffer)
    {
        clone_buffer = opx_data->u.pack.clone_frame_buffer;
    }
    else
    {
        clone_buffer = opx_data->u.clone_op_buffer;
    }


    /* Initialise the input parameters for the codec.*/
    /* Set the  main decoder's structure.*/
    aac_decoder.aac_decoder_struc = aac_codec_struc;
    /* Set the return structure. */
    aac_decoder.frame_dec_struc = frame_decode_data;

    /* Set the payload size which is all the available data. */
    aac_decoder.payload_size = payload_size;

    /* Set the input buffer. */
    aac_decoder.in_cbuffer = clone_buffer;

    offset = ((uintptr_t)(clone_buffer->read_ptr)) & 0x3;
    /* Set the bit position. */
    switch(offset)
    {
    case 0:
        aac_decoder.bit_position = 32;
        break;
    case 1:
        aac_decoder.bit_position = 24;
        break;
    case 2:
        aac_decoder.bit_position = 16;
        break;
    case 3:
        aac_decoder.bit_position = 8;
        break;
    }
    aacdec_samples_in_packet_lc(&aac_decoder);
}

/**
 * Decodes the sbc frame header and populates the frame data structure with the necessary
 * information to generate a timestamped tags.
 *
 * \param opx_data Pointer to the RTP operator data.
 * \param payload_size Pointer to the RTP operator data.
 * \param frame_data Pointer to the frame data structure.
 */
static void sbc_sample_count(RTP_DECODE_OP_DATA* opx_data, unsigned payload_size,
        RTP_FRAME_DECODE_DATA* frame_data)
{
    tCbuffer *clone_buffer;
    unsigned sbc_header[SBC_SAMPLE_COUNT_HEADER_SIZE];

    patch_fn_shared(rtp_decode);

    /* Select the internal buffers based on the packing */
    if (opx_data->pack_latency_buffer)
    {
        clone_buffer = opx_data->u.pack.clone_frame_buffer;
    }
    else
    {
        clone_buffer = opx_data->u.clone_op_buffer;
    }

    unsigned mode;
    unsigned nrof_blocks = 0;
    unsigned nrof_subbands = 0;
    unsigned bitpool;
    unsigned offset = 0;
    unsigned sbc_frames_in_payload;
    unsigned payload_left = payload_size;

    for (sbc_frames_in_payload = 0; sbc_frames_in_payload < frame_data->nr_of_frames_rtp_header; sbc_frames_in_payload++)
    {
        unsigned frame_length = 0;
        /* Read and unpack the sbc header. */
        unpack_cbuff_to_array_from_offset((int*) &sbc_header, clone_buffer,
                offset, SBC_SAMPLE_COUNT_HEADER_SIZE);

        mode = sbc_header[1] & SBC_HEADER1_CHANNEL_MASK;
        nrof_blocks = SBC_HEADER1_GET_NROF_BLOCKS(sbc_header[1]);
        nrof_subbands = SBC_HEADER1_GET_NROF_SUBBANDS(sbc_header[1]);
        bitpool = SBC_HEADER2_BITPOOL(sbc_header[2]);

        if (sbc_header[0] == SBC_HEADER0_SYNC)
        {

            switch (mode)
            {
                case SBC_HEADER1_CHANNEL_MONO:
                {
                    /* nrof_channels = 1;
                     * frame_length = 4 + (4 * nrof_subbands * nrof_channels) / 8 +  ceil((nrof_blocks * nrof_channels * bitpool) / 8); */
                    frame_length = 4 + (nrof_subbands) / 2
                            + ((nrof_blocks * bitpool) + 7) / 8;
                    break;
                }
                case SBC_HEADER1_CHANNEL_DUAL:
                {
                    /* nrof_channels = 2;
                     * frame_length = 4 + (4 * nrof_subbands * nrof_channels) / 8 +  ceil((nrof_blocks * nrof_channels * bitpool) / 8); */
                    frame_length = 4 + nrof_subbands
                            + ((nrof_blocks * 2 * bitpool) + 7) / 8;
                    break;
                }
                case SBC_HEADER1_CHANNEL_STEREO:
                {
                    /* nrof_channels = 2;
                     * frame_length = 4 + (4 * nrof_subbands * nrof_channels) / 8 +  ceil((nrof_blocks * bitpool) / 8); */
                    frame_length = 4 + nrof_subbands
                            + ((nrof_blocks * bitpool) + 7) / 8;
                    break;
                }
                case SBC_HEADER1_CHANNEL_JOINT:
                {
                    /* nrof_channels = 2;
                     * frame_length = 4 + (4 * nrof_subbands * nrof_channels) / 8 +  ceil((nrof_subbands + nrof_blocks *  bitpool)) / 8); */
                    frame_length = 4 + nrof_subbands
                            + ((nrof_subbands + nrof_blocks * bitpool) + 7) / 8;
                    break;
                }
                default:
                {
                    break;
                }
            }
            if (frame_length <= payload_left)
            {
                frame_data->frame_length_extra[sbc_frames_in_payload] = (uint16)frame_length;
                payload_left -= frame_length;
                offset += frame_length;
            }
            else
            {
                /* Probably the frame length calculation is wrong or the
                 * current frame_length exceeds the amount of payload left. */
                sbc_frames_in_payload = 0;
                break;
            }
        }
        else
        {
            fault_diatribe(FAULT_RTP_SBC_SYNC_LOST, sbc_header[0]);
            sbc_frames_in_payload = 0;
            break;
        }
    }

    if (payload_left == 0)
    {
        frame_data->valid = TRUE;
        frame_data->nr_of_frames = sbc_frames_in_payload;
        /* frame_samples =  nrof_subbands * nrof_blocks. */
        frame_data->frame_samples = nrof_subbands * nrof_blocks;
        frame_data->total_encoded_data = (uint16)offset;
    }
}

void get_samples_in_packet(RTP_DECODE_OP_DATA *opx_data, RTP_FRAME_DECODE_DATA* frame_data)
{
    tCbuffer *clone_buffer;
    unsigned payload_size;

    patch_fn_shared(rtp_decode);
    /* Select the internal buffers based on the packing */
    if (opx_data->pack_latency_buffer)
    {
        clone_buffer = opx_data->u.pack.clone_frame_buffer;
    }
    else
    {
        clone_buffer = opx_data->u.clone_op_buffer;
    }

    payload_size = cbuffer_calc_amount_data_ex(clone_buffer);
    frame_data->valid = FALSE;

    switch (opx_data->codec_type)
    {
        case SBC:
        {
            sbc_sample_count(opx_data, payload_size, frame_data);
            break;
        }

        case ATRAC:
        {
            break;
        }
        case MP3:
        {
            break;
        }
        case APTX:
        {
            frame_data->valid = TRUE;
            /* For aptX, the generated samples is the same as the number of encoded octets,
             * as long as the input number of octets is a multiple of 4.
             * This is assumed to always be the case here. */
            payload_size = payload_size >> 2;
            payload_size = payload_size << 2;
            if (payload_size != 0)
            {
                frame_data->frame_length = payload_size;
                frame_data->nr_of_frames = 1;
                frame_data->frame_samples = payload_size;
            }
            else
            {
                L4_DBG_MSG("RTP decode APTX classic: Not enough data to create a tag.");
                frame_data->nr_of_frames = 0;
            }
            break;
        }
        case APTXHD:
        {
            frame_data->valid = TRUE;
            /* For aptXHD, every 6 input octets generates 4 output samples. */
            frame_data->frame_samples = payload_size / 3;
            frame_data->frame_samples >>= 1;
            if (frame_data->frame_samples != 0)
            {
                frame_data->frame_length = frame_data->frame_samples * 6;
                frame_data->frame_samples <<= 2;
                frame_data->nr_of_frames = 1;
                /* If the size of the payload received is not a multiple of 6 the remaining
                 * octets will be part of the next payload. */
            }
            else
            {
                L4_DBG_MSG("RTP decode APTX hd: Not enough data to create a tag.");
                frame_data->nr_of_frames = 0;
            }
            break;
        }
        case APTXADAPTIVE:
        {
            frame_data->valid = TRUE;
            /* Extract the number of samples from the RTP Timestamp */
            frame_data->frame_samples = RTP_TIMESTAMP_APTX_AD_GET_SAMPLES(frame_data->rtp_timestamp);
            frame_data->frame_length = payload_size;
            frame_data->nr_of_frames = 1;
            break;
        }
        case AAC:
        {
            aac_sample_count(opx_data, payload_size, frame_data);
            break;
        }
        default:
        {
            /*mode not supported yet*/
            panic(PANIC_AUDIO_RTP_UNSUPPORTED_CODEC);
            break;
        }
    }
}
