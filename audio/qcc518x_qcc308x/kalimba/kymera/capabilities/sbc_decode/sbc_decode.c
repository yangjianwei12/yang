/****************************************************************************
 * Copyright (c) 2014 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  sbc_decode.c
 * \ingroup  capabilities
 *
 *  SBC decode operator
 *
 */

#include "capabilities.h"
#include "mem_utils/scratch_memory.h"
#include "mem_utils/memory_table.h"
#include "codec_c.h"
#include "sbc_c.h"
#include "a2dp_decode/a2dp_common_decode.h"

// add autogen header
#include "sbc_decode_gen_c.h"
#include "sbc_decode.h"
#include "sbc_decode_c.h"


/****************************************************************************
Private Constant Definitions
*/

/****************************************************************************
Private Type Definitions
*/

typedef struct
{
    /** A2DP_DECODER_PARAMS must be the first parameters always */
    A2DP_DECODER_PARAMS    decoder_data;

    /** The sbc_codec specific data */
    sbc_codec codec_data;

	/** The sbc_codec statistics to send to OBPM */
/*  SBC_DEC_STATISTICS statistics; */
} SBC_DEC_OP_DATA;

/****************************************************************************
Private Constant Declarations
*/
/** The maximum number of samples in a single SBC encoded frame */
#define MAX_SBC_BLOCK_SIZE              128

/** The length of the SBC Decoder capability malloc table */
#define SBC_DEC_MALLOC_TABLE_LENGTH 2

#define A2DP_STRIP_SBC_HDR           ( A2DP_STRIP_BFRAME | \
                                       A2DP_STRIP_RTP | \
                                       A2DP_STRIP_MP)
#define A2DP_SBC_HDR_SIZE            ( A2DP_BFRAME_HDR_SIZE + \
                                       A2DP_RTP_HDR_SIZE + \
                                       A2DP_MP_HDR_SIZE)

#define A2DP_STRIP_SBC_CP_HDR         ( A2DP_STRIP_SBC_HDR | \
                                        A2DP_STRIP_CP )

#define A2DP_SBC_CP_HDR_SIZE        ( A2DP_SBC_HDR_SIZE  + \
                                      A2DP_CP_HDR_SIZE )

#ifdef CAPABILITY_DOWNLOAD_BUILD
#define SBC_DECODER_CAP_ID CAP_ID_DOWNLOAD_SBC_DECODER
#else
#define SBC_DECODER_CAP_ID CAP_ID_SBC_DECODER
#endif

/** The SBC decoder capability function handler table */
const handler_lookup_struct sbc_decode_handler_table =
{
    sbc_decode_create,           /* OPCMD_CREATE */
    sbc_decode_destroy,          /* OPCMD_DESTROY */
    a2dp_decode_start,           /* OPCMD_START */
    base_op_stop,                /* OPCMD_STOP */
    sbc_decode_reset,            /* OPCMD_RESET */
    a2dp_decode_connect,         /* OPCMD_CONNECT */
    a2dp_decode_disconnect,      /* OPCMD_DISCONNECT */
    sbc_decode_buffer_details,  /* OPCMD_BUFFER_DETAILS */
    a2dp_decode_get_data_format, /* OPCMD_DATA_FORMAT */
    base_op_get_sched_info       /* OPCMD_GET_SCHED_INFO */
};

/** sbc decode capability data */
#ifdef INSTALL_OPERATOR_SBC_DECODE
const opmsg_handler_lookup_table_entry sbc_decode_opmsg_obpm_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_FADEOUT_ENABLE, a2dp_dec_opmsg_enable_fadeout},
    {OPMSG_COMMON_ID_FADEOUT_DISABLE, a2dp_dec_opmsg_disable_fadeout},
    /* {OPMSG_AD2P_DEC_ID_CONTENT_PROTECTION_ENABLE, sbc_dec_opmsg_content_protection_enable}, */
    {OPMSG_COMMON_ID_SET_CONTROL,                  sbc_dec_opmsg_obpm_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,                   sbc_dec_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,                 sbc_dec_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,                   sbc_dec_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,                   sbc_dec_opmsg_obpm_get_status},
    {0, NULL}
};

const CAPABILITY_DATA sbc_decode_cap_data =
{
    SBC_DECODER_CAP_ID,
    SBC_DECODE_SBCDEC_VERSION_MAJOR, 1, /* Version information - hi and lo parts */
    1, 2,                           /* Max 1 sink and 2 sources */
    &sbc_decode_handler_table,
    /* a2dp_decode_opmsg_handler_table, */
    sbc_decode_opmsg_obpm_handler_table,
    sbc_decode_process_data,        /* Data processing function */
    0,                              /* Reserved */
    sizeof(SBC_DEC_OP_DATA)
};
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_SBC_DECODER, SBC_DEC_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_SBC_DECODER, SBC_DEC_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#endif

#ifdef INSTALL_OPERATOR_SBC_SHUNT_DECODER
const opmsg_handler_lookup_table_entry sbc_strip_decode_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_FADEOUT_ENABLE, a2dp_dec_opmsg_enable_fadeout},
    {OPMSG_COMMON_ID_FADEOUT_DISABLE, a2dp_dec_opmsg_disable_fadeout},
    {OPMSG_AD2P_DEC_ID_CONTENT_PROTECTION_ENABLE, sbc_dec_opmsg_content_protection_enable},
    {OPMSG_COMMON_ID_SET_CONTROL,                  sbc_dec_opmsg_obpm_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,                   sbc_dec_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,                 sbc_dec_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,                   sbc_dec_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,                   sbc_dec_opmsg_obpm_get_status},
    {0, NULL}
};


const CAPABILITY_DATA sbc_a2dp_decoder_cap_data =
{
    CAP_ID_SBC_SHUNT_DECODER,
    0, 1,                           /* Version information - hi and lo parts */
    1, 2,                           /* Max 1 sink and 2 sources */
    &sbc_decode_handler_table,
    sbc_strip_decode_opmsg_handler_table,
    sbc_decode_process_data,        /* Data processing function */
    0,                              /* Reserved */
    (sizeof(SBC_DEC_OP_DATA) + sizeof(A2DP_HEADER_PARAMS))
};
MAP_INSTANCE_DATA(CAP_ID_SBC_SHUNT_DECODER, SBC_DEC_OP_DATA)
#endif

/** Memory owned by an sbc decoder instance */
const malloc_t_entry sbc_dec_malloc_table[SBC_DEC_MALLOC_TABLE_LENGTH] =
{
    {SBC_SYNTHESIS_BUFF_LENGTH, MALLOC_PREFERENCE_DM2, offsetof(sbc_codec, synthesis_vch1)},
    {SBC_SYNTHESIS_BUFF_LENGTH, MALLOC_PREFERENCE_DM2, offsetof(sbc_codec, synthesis_vch2)}
};

const scratch_table decoder_scratch_table =
{
    SBC_DEC_DM1_SCRATCH_TABLE_LENGTH,
    SBC_DEC_DM2_SCRATCH_TABLE_LENGTH,
    0,
    sbc_scratch_table_dm1,
    sbc_scratch_table_dm2,
    NULL
};

/***************************************************************************
Private Function Declarations
*/
static inline SBC_DEC_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (SBC_DEC_OP_DATA *) base_op_get_instance_data(op_data);
}

/**
 * \brief Frees up any data that has been allocated by the instance of the
 * sbc_decode capability.
 *
 * \param op_data Pointer to the operator instance data.
 */
static void free_data(OPERATOR_DATA *op_data)
{
    SBC_DEC_OP_DATA *sbc_data = get_instance_data(op_data);
    /* This can be called when create fails or on destroy so it checks that
     * every allocation has happened before calling free. */
    /* The variables aren't reset to NULL as this is either a failed create or
     * a destroy which means the operator data is about to be freed.
     */

    /* free the shared codec data */
    mem_table_free_shared((void *)(&(sbc_data->codec_data)),
                            sbc_shared_malloc_table, SBC_SHARED_TABLE_LENGTH);

    /* free shared decoder data */
    mem_table_free_shared((void *)(&(sbc_data->codec_data)),
                    sbc_dec_shared_malloc_table, SBC_DEC_SHARED_TABLE_LENGTH);

    /* free non-shared memory */
    mem_table_free((void *)(&(sbc_data->codec_data)), sbc_dec_malloc_table,
                                                SBC_DEC_MALLOC_TABLE_LENGTH);

}

/**
 * \brief Frees up any data that has been allocated by the instnace of the
 * sbc_decode capability and sets the response field to failed.
 *
 * \param op_data Pointer to the operator instance data.
 * \param response Pointer to the response message to give a failed status
 */
static void free_data_and_fail(OPERATOR_DATA *op_data, void **response)
{
    /* Free the data and then override the response message status to fail */
    free_data(op_data);
    base_op_change_response_status(response, STATUS_CMD_FAILED);
}

/**
 * \brief Allocates the sbc_decode specific capability memory and initialises
 * the decoder.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the create request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool sbc_decode_create(OPERATOR_DATA *op_data, void *message_data,
                                unsigned *response_id, void **response_data)
{
    SBC_DEC_OP_DATA *sbc_data = get_instance_data(op_data);
    bool new_allocation;

    /* call base_op create, which also allocates and fills response message */
    if (!base_op_create_lite(op_data, response_data))
    {
        return FALSE;
    }

    /* Stage 1 - Malloc the sbc decoder fields. A lot of this can be shared
     * between any other sbc decoders/encoders in the system. Including WBS
     * capabilities. */

    /* Share memory with decoders. */
    if( !mem_table_zalloc_shared((void *)(&(sbc_data->codec_data)),
                    sbc_dec_shared_malloc_table, SBC_DEC_SHARED_TABLE_LENGTH,
                    &new_allocation))
    {
        free_data_and_fail(op_data, response_data);
        return TRUE;
    }

    /* share memory with other sbc instances */
    if( !mem_table_zalloc_shared((void *)(&(sbc_data->codec_data)),
                            sbc_shared_malloc_table, SBC_SHARED_TABLE_LENGTH,
                            &new_allocation) )
    {
        free_data_and_fail(op_data, response_data);
        return TRUE;
    }

    /* now allocate the non-shareable memory */
    if(!mem_table_zalloc((uintptr_t *)(&(sbc_data->codec_data)), sbc_dec_malloc_table,
                                                SBC_DEC_MALLOC_TABLE_LENGTH))
    {
        free_data_and_fail(op_data, response_data);
        return TRUE;
    }

    /* Now reserve the scratch memory */
    if (scratch_register())
    {
        if (mem_table_scratch_tbl_reserve(&decoder_scratch_table))
        {
            /* Successfully allocated everything! */
            /* Stage 2 populate the DECODER structure */

            /* Tell the codec structure where to find the sbc codec data */
            sbc_data->decoder_data.codec.decoder_data_object = &(sbc_data->codec_data);

            /* Call the sbc decoder init_decode and init_tables functions. */
            sbc_decode_lib_init(sbc_data->decoder_data.codec.decoder_data_object);

#ifdef INSTALL_OPERATOR_SBC_SHUNT_DECODER
            if (base_op_get_cap_id(op_data) == CAP_ID_SBC_SHUNT_DECODER)
            {
                sbc_data->decoder_data.a2dp_header = (A2DP_HEADER_PARAMS*)
                                                     ((unsigned*)sbc_data + sizeof(SBC_DEC_OP_DATA));
                sbc_data->decoder_data.a2dp_header->type = (unsigned)
                                                   A2DP_STRIP_SBC_HDR;
                sbc_data->decoder_data.a2dp_header->hdr_size =
                                         (unsigned) A2DP_SBC_HDR_SIZE;

                /* a2dp_header will be valid here, hence no need to check */
                populate_strip_sbc_asm_funcs(&(sbc_data->decoder_data.decode_frame),
                                           &(sbc_data->decoder_data.a2dp_header->strip_decode_frame),
                                           &(sbc_data->decoder_data.a2dp_header->get_bits));
            }
            else  /* CAP_ID_SBC_DECODE */
#endif /* INSTALL_OPERATOR_SBC_SHUNT_DECODER */
            {
                populate_sbc_asm_funcs(&(sbc_data->decoder_data.decode_frame),
                                       &(sbc_data->decoder_data.silence));
            }
            return TRUE;
        }
        /* Fail free all the scratch memory we reserved */
        scratch_deregister();
    }

    /* Clear up all the allocated memory. */
    free_data_and_fail(op_data, response_data);
    return TRUE;
}


/**
 * \brief Deallocates the sbc_decode specific capability memory.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the destroy request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool sbc_decode_destroy(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *response_id, void **response_data)
{
    /* check that we are not trying to destroy a running operator */
    if (opmgr_op_is_running(op_data))
    {
        /* We can't destroy a running operator. */
        return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
    }
    else if(base_op_destroy_lite(op_data, response_data))
    {
        /* Free all the scratch memory we reserved */
        scratch_deregister();
        /* Clear up the sbc_decode specific work and then let base_op do
         * the grunt work. */
        free_data(op_data);
        return TRUE;
    }

    return FALSE;
}

/**
 * \brief Resets the sbc_decode capability.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the reset request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool sbc_decode_reset(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *response_id, void **response_data)
{
    SBC_DEC_OP_DATA *sbc_data = get_instance_data(op_data);

    if (!base_op_reset(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    sbc_decode_lib_reset(sbc_data->decoder_data.codec.decoder_data_object);

    return TRUE;
}

/**
 * \brief Reports the buffer requirements of the requested capability terminal
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the buffer size request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool sbc_decode_buffer_details(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *response_id, void **response_data)
{
    /* Add one complete SBC frame to the base-op derived output buffer size
     * This helps to ensure stable data flow in voice prompts etc
     */
    return a2dp_decode_buffer_details_core(op_data, message_data,response_id, response_data,A2DP_DECODE_INPUT_BUFFER_SIZE,A2DP_DECODE_OUTPUT_BUFFER_SIZE, MAX_SBC_BLOCK_SIZE);
}


/**
 * \brief process function to decode available input data
 *
 * \param op_data Pointer to the operator instance data.
 * \param touched Structure to return the terminals which this operator wants kicked
 */
void sbc_decode_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    SBC_DEC_OP_DATA *sbc_data = get_instance_data(op_data);
    unsigned output_samples;
    bool input_full = FALSE;
    stereo_ptrs write_ptrs = {NULL, NULL};
    /* amount of input data processed in octets */
    unsigned processed;
    patch_fn(sbc_decode_process_data);

    /* Check the input hasn't gone away, if it has then nothing we can do. It's
     * a radio link so this can happen */
    if (NULL == sbc_data->decoder_data.codec.in_buffer)
    {
        /* One option is to produce silence in this case.*/
        return;
    }

    if (!cbuffer_calc_amount_space_in_words(sbc_data->decoder_data.codec.in_buffer))
    {
        input_full = TRUE;
    }

    /* Commit any scratch memory ideally this should be done later after the
     * decision to decode is made. The call is cheap so it doesn't hurt to
     * do it here currently. */
    mem_table_scratch_tbl_commit(&sbc_data->codec_data, &decoder_scratch_table);

    do
    {
        /* Checks for enough data and enough output space are done at the top of
         * this function so it's not done in this C code as well. */

        a2dp_decode_buffer_get_write_ptrs(&(sbc_data->decoder_data), &write_ptrs);

        a2dp_decoder_decode(&(sbc_data->decoder_data.codec),
                             sbc_data->decoder_data.decode_frame,
                             CODEC_NORMAL_DECODE,
                             sbc_data->decoder_data.a2dp_header);

         output_samples = sbc_data->decoder_data.codec.num_output_samples;
         /* amount of input data processed in octets */
         processed = sbc_data->decoder_data.codec.num_input_octets_consumed;
        if (output_samples > 0 || processed > 0)
        {
            /* Source 0 is always touched */
            unsigned touched_sources = TOUCHED_SOURCE_0;

            unsigned b4idx, afteridx, output_frames, output_framesize;
            metadata_tag *mtag;
            tCbuffer *src, *dst;

            src = sbc_data->decoder_data.codec.in_buffer;
            dst = sbc_data->decoder_data.metadata_op_buffer;
            output_framesize = sbc_data->codec_data.nrof_blocks * sbc_data->codec_data.nrof_subbands;
            output_frames = output_samples / output_framesize;
            mtag = buff_metadata_remove(src, processed, &b4idx, &afteridx);

            if (buff_has_metadata(dst))
            {
                unsigned output_octets, after_octets = output_framesize * OCTETS_PER_SAMPLE;
                metadata_tag *list_tag;

                /* decoded output will have different frame lengths */
                output_octets = output_samples * OCTETS_PER_SAMPLE;

                list_tag = mtag;

                if ((list_tag != NULL) && IS_TIME_TO_PLAY_TAG(list_tag))
                {
                    /* Total length of output tags, in case inputs consumed without generating
                     * output (e.g. when input is corrupt) we make sure the output metadata and
                     * data stay aligned
                     */
                    unsigned tot_output_len_left = output_samples * OCTETS_PER_SAMPLE;
                    /* Incoming metadata has timestamps and must be frame-aligned.
                     * Reuse the existing tags
                     */

                    /* We expect timestamps to be frame-aligned.
                     * there's no attempt to interpolate timestamps here.
                     * Occasional unaligned tags may be due to corrupted frames,
                     * so log this case and carry on processing.
                     */
                    if (b4idx != 0)
                    {
                        L2_DBG_MSG("SBC decode, unaligned timestamped tag (corrupted data?)");
                    }

                    while (list_tag != NULL)
                    {
                        if (list_tag->next == NULL || (list_tag->next->length == 0 && list_tag->next->next == NULL))
                        {
                            /* If this is the last tag, or the tag before an ending zero-length tag (EOF),
                             * anyway it must use the remaining total length */
                            list_tag->length = tot_output_len_left;
                        }
                        else
                        {
                            /* Handle if a tag contain multiple SBC frames. Assume that they are
                             * all the same length.
                             */
                            list_tag->length = (list_tag->length)/sbc_data->codec_data.cur_frame_length;
                            list_tag->length = list_tag->length * output_framesize * OCTETS_PER_SAMPLE;

                            /* make sure we don't append tags more than the total output samples */
                            list_tag->length = MIN(tot_output_len_left, list_tag->length);
                        }

                        /* update the amount left*/
                        tot_output_len_left -= list_tag->length;

                        /* update after_octets */
                        after_octets = list_tag->length;

                        if ((tot_output_len_left == 0) && (list_tag->next != NULL))
                        {
                            L2_DBG_MSG("SBC decode: No more data for remaining tags!");
                            buff_metadata_tag_list_delete(list_tag->next);
                            list_tag->next = NULL;
                        }

                        list_tag = list_tag->next;
                    }
                }
                else
                {
                    /* Incoming metadata is not timestamped (probably from a file)
                     * Create a new list of tags for the output, one per frame
                     */
                    unsigned frame;
                    metadata_tag *eof_tag = NULL;

                    if ((afteridx == 0) && (mtag != NULL))
                    {
                        /* Check if there is an EOF tag at the end.
                         * If there is, copy it before deleting the old list */
                        metadata_tag *end_tag = mtag;
                        while (end_tag->next != NULL)
                        {
                            end_tag = end_tag->next;
                        }
                        if (METADATA_STREAM_END(end_tag))
                        {
                            eof_tag = buff_metadata_copy_tag(end_tag);
                            after_octets = 0;
                        }
                    }

                    buff_metadata_tag_list_delete(mtag);
                    mtag = NULL;
                    list_tag = NULL;

                    for (frame = 0; frame < output_frames; frame++)
                    {
                        metadata_tag *new_tag = buff_metadata_new_tag();
                        if (new_tag != NULL)
                        {
                            new_tag->length = output_framesize * OCTETS_PER_SAMPLE;
                            METADATA_PACKET_START_SET(new_tag);
                            METADATA_PACKET_END_SET(new_tag);
                        }
                        if (list_tag == NULL)
                        {
                            mtag = new_tag;
                        }
                        else
                        {
                            list_tag->next = new_tag;
                        }
                        list_tag = new_tag;
                        /* Add on an EOF tag if we found one at the end */
                        if (list_tag != NULL)
                        {
                            if (eof_tag != NULL)
                            {
                                L2_DBG_MSG("EOF tag appended");
                            }
                            list_tag->next = eof_tag;
                        }
                    }
                }
                if (mtag != NULL)
                {
                    buff_metadata_append(dst, mtag, 0, after_octets);
                }
                else
                {
                    /* Continue adding data to the last tag.*/
                    buff_metadata_append(dst, mtag, output_octets, 0);
                }
            }
            else
            {
                buff_metadata_tag_list_delete(mtag);
            }

            /* Handle mono/stereo fade-out */
            if (a2dp_decode_check_and_perform_fadeout(&(sbc_data->decoder_data), output_samples, &write_ptrs))
            {
                common_send_simple_unsolicited_message(op_data, OPMSG_REPLY_ID_FADEOUT_DONE);
            }

           /* If stereo outputs are connected then source 1 was touched as well */
           if (sbc_data->decoder_data.codec.out_right_buffer != NULL)
           {
               touched_sources |= TOUCHED_SOURCE_1;
           }

           /* Source(s) touched */
           touched->sources = touched_sources;
        }


    }while (sbc_data->decoder_data.codec.mode == CODEC_SUCCESS);

    /* Free the scratch memory used */
    scratch_free();

#ifdef INSTALL_SBC_DATA_TEST
    /* Kick the input to get it going */
    touched->sinks |= TOUCHED_SINK_0;
#else
    /* If there was not enoguh input kick backwards. In case of an error
     * (CODEC_ERROR or CODEC_FRAME_CORRUPT) kick backwards if there is less than
     * the maximum SBC frame size (worst case scenario).
     *
     * If the input buffer was full, it is likely that the endpoint could not
     * copy data into the transform buffer. To solve this problem, SBC decode will kick the endpoint
     * after having consumed from a full buffer. In cases where the endpoint should kick an
     * upstream entity this is useful so the upstream entity can be notified of more space.
     * In other cases it helps to get data into the decoder as quickly as possible.
     */
    if (((sbc_data->decoder_data.codec.mode == CODEC_NOT_ENOUGH_INPUT_DATA) ||
         (((sbc_data->decoder_data.codec.mode == CODEC_ERROR) || (sbc_data->decoder_data.codec.mode == CODEC_FRAME_CORRUPT)) &&
          (cbuffer_calc_amount_data_in_words(sbc_data->decoder_data.codec.in_buffer) < MAX_SBC_FRAME_SIZE_IN_WORDS))) ||
        (input_full && cbuffer_calc_amount_space_in_words(sbc_data->decoder_data.codec.in_buffer)))
    {
        touched->sinks |= TOUCHED_SOURCE_0;
    }
#endif /* INSTALL_SBC_DATA_TEST */
}

#ifdef INSTALL_OPERATOR_SBC_SHUNT_DECODER

/**
 * \brief Enable or disable the A2DP content protection.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the request message payload
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool sbc_dec_opmsg_content_protection_enable(OPERATOR_DATA *op_data,
                    void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SBC_DEC_OP_DATA *sbc_data = get_instance_data(op_data);
    A2DP_HEADER_PARAMS* a2dp_header;

    a2dp_header = (A2DP_HEADER_PARAMS *) sbc_data->decoder_data.a2dp_header;

    if (a2dp_header == NULL || opmgr_op_is_running(op_data))
    {
        return FALSE;
    }

    /* Set the Content protection bit */
    if(OPMSG_FIELD_GET(message_data, OPMSG_A2DP_DEC_CONTENT_PROTECTION_ENABLE, ENABLE_CP) != 0 )
    {
        a2dp_header->type = (unsigned) A2DP_STRIP_SBC_CP_HDR;
        a2dp_header->hdr_size =  (unsigned)A2DP_SBC_CP_HDR_SIZE;
    }
    else
    {
        a2dp_header->type = (unsigned)A2DP_STRIP_SBC_HDR;
        a2dp_header->hdr_size = (unsigned) A2DP_SBC_HDR_SIZE;
    }

    return TRUE;
}

#endif /* INSTALL_OPERATOR_SBC_SHUNT_DECODER */

bool sbc_dec_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* In the case of this capability, nothing is done for control message. Just follow protocol and ignore any content. */
    return cps_control_setup(message_data, resp_length, resp_data,NULL);
}


bool sbc_dec_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    return FALSE;
}

bool sbc_dec_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{

     return FALSE;
}

bool sbc_dec_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* Set the parameter(s). For future proofing, it is using the whole mechanism, although currently there is only one field
     * in opdata structure that is a setable parameter. If later there will be more (ever), must follow contiquously the first field,
     * as commented and instructed in the op data definition. Otherwise consider moving them into a dedicated structure.
     */

    return FALSE;
}

bool sbc_dec_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SBC_DEC_OP_DATA *op_extra_data = get_instance_data(op_data);

    unsigned* resp = NULL;

    if(!common_obpm_status_helper(message_data,resp_length,resp_data,sizeof(SBC_DECODE_STATISTICS) ,&resp))
    {
         return FALSE;
    }

    if(resp)
    {
            resp = cpsPack2Words(op_extra_data->codec_data.sampling_freq, op_extra_data->codec_data.channel_mode, resp);
            resp = cpsPack2Words(op_extra_data->codec_data.bitpool, op_extra_data->codec_data.nrof_blocks, resp);
            resp = cpsPack2Words(op_extra_data->codec_data.nrof_channels, op_extra_data->codec_data.nrof_subbands, resp);
            cpsPack1Word(op_extra_data->codec_data.allocation_method, resp);
    }

	return TRUE;
}
