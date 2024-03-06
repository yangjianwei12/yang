/****************************************************************************
 * Copyright (c) 2014 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  aac_decode.c
 * \ingroup  capabilities
 *
 *  AAC decode operator
 *
 */

#include "capabilities.h"
#include "mem_utils/scratch_memory.h"
#include "mem_utils/memory_table.h"
#include "codec_c.h"
#include "aac_c.h"
#include "a2dp_decode/a2dp_common_decode.h"

// add autogen header
#include "aac_decode_gen_c.h"
#include "aac_decode_c.h"
#include "aac_decode.h"
/****************************************************************************
Private Constant Declarations
*/
/** The maximum number of samples in a single AAC encoded frame */

#define AAC_DECODE_INPUT_BUFFER_SIZE    MAX_AAC_FRAME_SIZE_IN_WORDS
#define SRA_HEADROOM                    64
#if defined (AACDEC_SBR_ADDITIONS) || defined(AACDEC_PS_ADDITIONS) || defined(AACDEC_ELD_ADDITIONS)
    #define AAC_DECODE_OUTPUT_BUFFER_SIZE   2*AAC_FRAME_BUF_LENGTH + SRA_HEADROOM
#else
    #define AAC_DECODE_OUTPUT_BUFFER_SIZE   AAC_FRAME_BUF_LENGTH + SRA_HEADROOM
#endif
#define AAC_MP4                         0
#define AAC_ADTS                        1
#define AAC_LATM                        2
/****************************************************************************
Private Constant Definitions
*/
/** The scratch buffer size required if performing ratematching inside the capability
 * This has to be AAC frame size + some SRA headroom (already included above)*/
#define AAC_SCRATCH_BUFFER_SIZE  AAC_DECODE_OUTPUT_BUFFER_SIZE * sizeof(unsigned)

/****************************************************************************
Private Type Definitions
*/

/* THIS IS THIEVED FROM an old capabilities.h */
typedef struct
{
    /** A2DP_DECODER_PARAMS must be the first parameters always */
    A2DP_DECODER_PARAMS decoder_data;

    /** The aac_codec specific data */
    aac_codec codec_data;

    /** Terminal input buffer for the AAC decoder */
    tCbuffer *terminal_ip_buff;
} AAC_DEC_OP_DATA;

#define A2DP_STRIP_AAC_HDR           A2DP_STRIP_BFRAME
#define A2DP_AAC_HDR_SIZE            A2DP_BFRAME_HDR_SIZE
#define A2DP_STRIP_AAC_CP_HDR        ( A2DP_STRIP_BFRAME | \
                                        A2DP_STRIP_RTP | \
                                        A2DP_STRIP_CP )
#define A2DP_AAC_CP_HDR_SIZE        ( A2DP_BFRAME_HDR_SIZE + \
                                        A2DP_RTP_HDR_SIZE + \
                                        A2DP_CP_HDR_SIZE )

#ifdef CAPABILITY_DOWNLOAD_BUILD
#define AAC_DECODER_CAP_ID CAP_ID_DOWNLOAD_AAC_DECODER
#define AAC_SHUNT_DECODER_CAP_ID CAP_ID_DOWNLOAD_AAC_SHUNT_DECODER
#else
#define AAC_DECODER_CAP_ID CAP_ID_AAC_DECODER
#define AAC_SHUNT_DECODER_CAP_ID CAP_ID_AAC_SHUNT_DECODER
#endif

/** The aac_decoder capability function handler table */
const handler_lookup_struct aac_decode_handler_table =
{
    aac_decode_create,           /* OPCMD_CREATE */
    aac_decode_destroy,          /* OPCMD_DESTROY */
    a2dp_decode_start,           /* OPCMD_START */
    base_op_stop,                /* OPCMD_STOP */
    aac_decode_reset,            /* OPCMD_RESET */
    aac_decode_connect,          /* OPCMD_CONNECT */
    aac_decode_disconnect,       /* OPCMD_DISCONNECT */
    aac_decode_buffer_details,   /* OPCMD_BUFFER_DETAILS */
    a2dp_decode_get_data_format, /* OPCMD_DATA_FORMAT */
    base_op_get_sched_info       /* OPCMD_GET_SCHED_INFO */
};
const opmsg_handler_lookup_table_entry aac_decode_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_FADEOUT_ENABLE, a2dp_dec_opmsg_enable_fadeout},
    {OPMSG_COMMON_ID_FADEOUT_DISABLE, a2dp_dec_opmsg_disable_fadeout},
    {OPMSG_AAC_ID_SET_FRAME_TYPE, aac_set_frame_type},
    {OPMSG_AAC_ID_SET_HUFFMAN_TABLES_LOCATION, aac_set_huffman_location},

    {OPMSG_COMMON_ID_SET_CONTROL,                  aac_dec_opmsg_obpm_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,                   aac_dec_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,                 aac_dec_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,                   aac_dec_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,                   aac_dec_opmsg_obpm_get_status},
    {0, NULL}
};
/** aac decode capability data */
#ifdef INSTALL_OPERATOR_AAC_DECODE
const CAPABILITY_DATA aac_decode_cap_data =
{
    AAC_DECODER_CAP_ID,
    AAC_DECODE_AACD_VERSION_MAJOR, 1, /* Version information - hi and lo parts */
    1, 2,                           /* Max 1 sink and 2 sources */
    &aac_decode_handler_table,
    aac_decode_opmsg_handler_table,
    aac_decode_process_data,        /* Data processing function */
    0,                              /* Reserved */
    sizeof(AAC_DEC_OP_DATA)
};
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_AAC_DECODER, AAC_DEC_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_AAC_DECODER, AAC_DEC_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_AAC_SHUNT_DECODER

const opmsg_handler_lookup_table_entry aac_strip_decode_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_FADEOUT_ENABLE, a2dp_dec_opmsg_enable_fadeout},
    {OPMSG_COMMON_ID_FADEOUT_DISABLE, a2dp_dec_opmsg_disable_fadeout},
    {OPMSG_AD2P_DEC_ID_CONTENT_PROTECTION_ENABLE, aac_dec_opmsg_content_protection_enable},
    {OPMSG_COMMON_ID_SET_CONTROL,                  aac_dec_opmsg_obpm_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,                   aac_dec_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,                 aac_dec_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,                   aac_dec_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,                   aac_dec_opmsg_obpm_get_status},
    {0, NULL}
};

const CAPABILITY_DATA aac_a2dp_decoder_cap_data =
{
    AAC_SHUNT_DECODER_CAP_ID,
    0, 1,                           /* Version information - hi and lo parts */
    1, 2,                           /* Max 1 sink and 2 sources */
    &aac_decode_handler_table,
    a2dp_decode_opmsg_handler_table,
    aac_decode_process_data,        /* Data processing function */
    0,                              /* Reserved */
    sizeof(AAC_DEC_OP_DATA) + sizeof(A2DP_HEADER_PARAMS)
};
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_AAC_SHUNT_DECODER, AAC_DEC_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_AAC_SHUNT_DECODER, AAC_DEC_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

/** Memory owned by an aac decoder instance */
    const malloc_t_entry aac_dec_malloc_table[] =
    {
#ifdef AACDEC_ELD_ADDITIONS
        {AAC_OVERLAP_ADD_LENGTH,                  MALLOC_PREFERENCE_DM2,  offsetof(aac_codec, overlap_add_left_ptr)},
        {AAC_OVERLAP_ADD_LENGTH,                  MALLOC_PREFERENCE_DM2,  offsetof(aac_codec, overlap_add_right_ptr)},
        {AAC_TNS_INPUT_HISTORY_LENGTH,            MALLOC_PREFERENCE_NONE, offsetof(aac_codec, tns_fir_input_history_ptr)},
        {AAC_DEC_SBR_X_IMAG_LENGTH,               MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_x_imag_ptr)},
        {AAC_DEC_SBR_X_REAL_LENGTH,               MALLOC_PREFERENCE_DM2,  offsetof(aac_codec, SBR_x_real_ptr)},
        {AAC_DEC_SBR_V_BUF_LENGTH,                MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_v_buffer_left_ptr)},
        {AAC_DEC_SBR_V_BUF_LENGTH,                MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_v_buffer_right_ptr)},
        {AAC_DEC_SBR_X_INPUT_BUF_LENGTH,          MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_x_input_buffer_left_ptr)},
        {AAC_DEC_SBR_X_INPUT_BUF_LENGTH,          MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_x_input_buffer_right_ptr)},
        {AAC_DEC_SBR_OTHER_IMAG_LENGTH,           MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_X_sbr_other_imag_ptr)},
        {AAC_DEC_SBR_INFO_LENGTH,                 MALLOC_PREFERENCE_NONE, offsetof(aac_codec, SBR_info_ptr)},
        {AAC_DEC_SBR_SYNTH_TMP_LENGTH,            MALLOC_PREFERENCE_NONE, offsetof(aac_codec, SBR_synth_temp_ptr)},
        {AAC_DEC_PS_INFO_LENGTH,                  MALLOC_PREFERENCE_NONE, offsetof(aac_codec, PS_info_ptr)},
        {AAC_DEC_PS_X_HYBRID_RIGHT_IMAG_LENGTH,   MALLOC_PREFERENCE_NONE, offsetof(aac_codec, PS_X_hybrid_imag_address[1])},
        {AAC_DEC_PS_HYBRID_ALLPASS_BUFFER_LENGTH, MALLOC_PREFERENCE_NONE, offsetof(aac_codec, PS_hybrid_allpass_feedback_buffer_ptr)},
        {AAC_DEC_PS_QMF_ALLPASS_BUFFER_LENGTH,    MALLOC_PREFERENCE_NONE, offsetof(aac_codec, PS_qmf_allpass_feedback_buffer_ptr)},
        {AAC_DEC_PS_HYBRID_TYPE_A_BUFFER_LENGTH,  MALLOC_PREFERENCE_NONE, offsetof(aac_codec, PS_hybrid_type_a_fir_filter_input_buffer_ptr)},
        {AAC_DEC_PS_LONG_DELAY_BUFFER_LENGTH,     MALLOC_PREFERENCE_NONE, offsetof(aac_codec, PS_long_delay_band_buffer_real_ptr)},
        {AAC_DEC_PS_LONG_DELAY_BUFFER_LENGTH,     MALLOC_PREFERENCE_NONE, offsetof(aac_codec, PS_long_delay_band_buffer_imag_ptr)},
        {AAC_DEC_ELD_TEMP_U_LENGTH,               MALLOC_PREFERENCE_NONE, offsetof(aac_codec, ELD_temp_u_ptr)},
        {AAC_DEC_ELD_SYNTH_TEMP1_LENGTH,          MALLOC_PREFERENCE_NONE, offsetof(aac_codec, ELD_synthesis_temp1_ptr)},
        {AAC_DEC_ELD_SYNTH_TEMP2_LENGTH,          MALLOC_PREFERENCE_NONE, offsetof(aac_codec, ELD_synthesis_temp2_ptr)},
        {AAC_DEC_ELD_SYNTH_TEMP3_LENGTH,          MALLOC_PREFERENCE_NONE, offsetof(aac_codec, ELD_synthesis_temp3_ptr)},
        {AAC_DEC_ELD_GW_BUF_LENGTH,               MALLOC_PREFERENCE_NONE, offsetof(aac_codec, ELD_synthesis_g_w_buffer_ptr)},
        {AAC_DEC_ELD_SBR_TEMP_LENGTH,             MALLOC_PREFERENCE_DM2,  offsetof(aac_codec, ELD_sbr_temp_5_ptr)},
        {AAC_DEC_ELD_SBR_TEMP_LENGTH,             MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, ELD_sbr_temp_6_ptr)},
        {AAC_DEC_ELD_SBR_TEMP_LENGTH,             MALLOC_PREFERENCE_DM2,  offsetof(aac_codec, ELD_sbr_temp_7_ptr)},
        {AAC_DEC_ELD_SBR_TEMP_LENGTH,             MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, ELD_sbr_temp_8_ptr)},
        {AAC_DEC_ELD_IFFT_RE_LENGTH,              MALLOC_PREFERENCE_NONE, offsetof(aac_codec, ELD_ifft_re_ptr)},
    };
#else
#ifdef AACDEC_PS_ADDITIONS
        {AAC_OVERLAP_ADD_LENGTH,                  MALLOC_PREFERENCE_DM2,  offsetof(aac_codec, overlap_add_left_ptr)},
        {AAC_OVERLAP_ADD_LENGTH,                  MALLOC_PREFERENCE_DM2,  offsetof(aac_codec, overlap_add_right_ptr)},
        {AAC_TNS_INPUT_HISTORY_LENGTH,            MALLOC_PREFERENCE_NONE, offsetof(aac_codec, tns_fir_input_history_ptr)},
        {AAC_DEC_SBR_X_IMAG_LENGTH,               MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_x_imag_ptr)},
        {AAC_DEC_SBR_X_REAL_LENGTH,               MALLOC_PREFERENCE_DM2,  offsetof(aac_codec, SBR_x_real_ptr)},
        {AAC_DEC_SBR_V_BUF_LENGTH,                MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_v_buffer_left_ptr)},
        {AAC_DEC_SBR_V_BUF_LENGTH,                MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_v_buffer_right_ptr)},
        {AAC_DEC_SBR_X_INPUT_BUF_LENGTH,          MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_x_input_buffer_left_ptr)},
        {AAC_DEC_SBR_X_INPUT_BUF_LENGTH,          MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_x_input_buffer_right_ptr)},
        {AAC_DEC_SBR_OTHER_IMAG_LENGTH,           MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_X_sbr_other_imag_ptr)},
        {AAC_DEC_SBR_INFO_LENGTH,                 MALLOC_PREFERENCE_NONE, offsetof(aac_codec, SBR_info_ptr)},
        {AAC_DEC_SBR_SYNTH_TMP_LENGTH,            MALLOC_PREFERENCE_NONE, offsetof(aac_codec, SBR_synth_temp_ptr)},
        {AAC_DEC_PS_INFO_LENGTH,                  MALLOC_PREFERENCE_NONE, offsetof(aac_codec, PS_info_ptr)},
        {AAC_DEC_PS_X_HYBRID_RIGHT_IMAG_LENGTH,   MALLOC_PREFERENCE_NONE, offsetof(aac_codec, PS_X_hybrid_imag_address[1])},
        {AAC_DEC_PS_HYBRID_ALLPASS_BUFFER_LENGTH, MALLOC_PREFERENCE_NONE, offsetof(aac_codec, PS_hybrid_allpass_feedback_buffer_ptr)},
        {AAC_DEC_PS_QMF_ALLPASS_BUFFER_LENGTH,    MALLOC_PREFERENCE_NONE, offsetof(aac_codec, PS_qmf_allpass_feedback_buffer_ptr)},
        {AAC_DEC_PS_HYBRID_TYPE_A_BUFFER_LENGTH,  MALLOC_PREFERENCE_NONE, offsetof(aac_codec, PS_hybrid_type_a_fir_filter_input_buffer_ptr)},
        {AAC_DEC_PS_LONG_DELAY_BUFFER_LENGTH,     MALLOC_PREFERENCE_NONE, offsetof(aac_codec, PS_long_delay_band_buffer_real_ptr)},
        {AAC_DEC_PS_LONG_DELAY_BUFFER_LENGTH,     MALLOC_PREFERENCE_NONE, offsetof(aac_codec, PS_long_delay_band_buffer_imag_ptr)},
    };
#else
#ifdef AACDEC_SBR_ADDITIONS
        {AAC_OVERLAP_ADD_LENGTH,                  MALLOC_PREFERENCE_DM2,  offsetof(aac_codec, overlap_add_left_ptr)},
        {AAC_OVERLAP_ADD_LENGTH,                  MALLOC_PREFERENCE_DM2,  offsetof(aac_codec, overlap_add_right_ptr)},
        {AAC_TNS_INPUT_HISTORY_LENGTH,            MALLOC_PREFERENCE_NONE, offsetof(aac_codec, tns_fir_input_history_ptr)},
        {AAC_DEC_SBR_X_IMAG_LENGTH,               MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_x_imag_ptr)},
        {AAC_DEC_SBR_X_REAL_LENGTH,               MALLOC_PREFERENCE_DM2,  offsetof(aac_codec, SBR_x_real_ptr)},
        {AAC_DEC_SBR_V_BUF_LENGTH,                MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_v_buffer_left_ptr)},
        {AAC_DEC_SBR_V_BUF_LENGTH,                MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_v_buffer_right_ptr)},
        {AAC_DEC_SBR_X_INPUT_BUF_LENGTH,          MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_x_input_buffer_left_ptr)},
        {AAC_DEC_SBR_X_INPUT_BUF_LENGTH,          MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_x_input_buffer_right_ptr)},
        {AAC_DEC_SBR_OTHER_IMAG_LENGTH,           MALLOC_PREFERENCE_DM1,  offsetof(aac_codec, SBR_X_sbr_other_imag_ptr)},
        {AAC_DEC_SBR_INFO_LENGTH,                 MALLOC_PREFERENCE_NONE, offsetof(aac_codec, SBR_info_ptr)},
        {AAC_DEC_SBR_SYNTH_TMP_LENGTH,            MALLOC_PREFERENCE_NONE, offsetof(aac_codec, SBR_synth_temp_ptr)},
    };
#else
        {AAC_FRAME_BUF_LENGTH,                    MALLOC_PREFERENCE_DM2,  offsetof(aac_codec, buf_left_ptr)},
        {AAC_FRAME_BUF_LENGTH,                    MALLOC_PREFERENCE_DM2,  offsetof(aac_codec, buf_right_ptr)},
        {AAC_OVERLAP_ADD_LENGTH,                  MALLOC_PREFERENCE_DM2,  offsetof(aac_codec, overlap_add_left_ptr)},
        {AAC_OVERLAP_ADD_LENGTH,                  MALLOC_PREFERENCE_DM2,  offsetof(aac_codec, overlap_add_right_ptr)},
        {AAC_TNS_INPUT_HISTORY_LENGTH,            MALLOC_PREFERENCE_NONE, offsetof(aac_codec, tns_fir_input_history_ptr)},
    };
#endif
#endif
#endif

/***************************************************************************
Private Function Declarations
*/
static inline AAC_DEC_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (AAC_DEC_OP_DATA *) base_op_get_instance_data(op_data);
}

/**
 * \brief Frees up any data that has been allocated by the instance of the
 * aac_decode capability.
 *
 * \param op_data Pointer to the operator instance data.
 */
static void free_data(OPERATOR_DATA *op_data)
{
    AAC_DEC_OP_DATA *aac_data = get_instance_data(op_data);
    /* This can be called when create fails or on destroy so it checks that
     * every allocation has happened before calling free. */
    /* The variables aren't reset to NULL as this is either a failed create or
     * a destroy which means the operator data is about to be freed.
     */
#ifndef AACDEC_TABLE_NO_FLASH_COPY
    /* free shared decoder data */
    mem_table_free_shared((void *)(&(aac_data->codec_data)),
                    aac_dec_shared_malloc_table, sizeof(aac_dec_shared_malloc_table) / sizeof(share_malloc_t_entry));
#endif
    /* free non-shared memory */
    mem_table_free((void *)(&(aac_data->codec_data)),
                    aac_dec_malloc_table, sizeof(aac_dec_malloc_table) / sizeof(malloc_t_entry));

    aac_decode_free_decoder_twiddle();
}

/**
 * \brief Frees up any data that has been allocated by the instnace of the
 * aac_decode capability and sets the response field to failed.
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
 * \brief Allocates the aac_decode specific capability memory and initialises
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
bool aac_decode_create(OPERATOR_DATA *op_data, void *message_data,
                                unsigned *response_id, void **response_data)
{

    AAC_DEC_OP_DATA *aac_data = get_instance_data(op_data);
    tCbuffer *library_input_buffer;
    A2DP_DECODER_PARAMS *decoder_data;

    decoder_data = &aac_data->decoder_data;

    /* call base_op create, which also allocates and fills response message */
    if (!base_op_create_lite(op_data, response_data))
    {
        return FALSE;
    }
    /* create a buffer to be used for the kalimba AAC library */
    library_input_buffer = cbuffer_create_with_malloc(MAX_AAC_FRAME_SIZE_IN_WORDS,
                                                      BUF_DESC_SW_BUFFER);
    if(NULL == library_input_buffer)
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }
    /* Enable metadata for the library_input_buffer  */
    metadata_list *mlist = buff_metadata_enable(library_input_buffer);
    if(NULL == mlist)
    {
       base_op_change_response_status(response_data, STATUS_CMD_FAILED);
       return TRUE;
    }
    decoder_data->codec.in_buffer = library_input_buffer;

#ifndef AACDEC_TABLE_NO_FLASH_COPY
    bool new_allocation;
         /* Share memory with decoders. */
    if( !mem_table_zalloc_shared((void *)(&(aac_data->codec_data)),
                    aac_dec_shared_malloc_table, sizeof(aac_dec_shared_malloc_table) / sizeof(share_malloc_t_entry),
                    &new_allocation))
    {
        free_data_and_fail(op_data, response_data);
        return TRUE;
    }
#endif
    /* now allocate the non-shareable memory */
    if(!mem_table_zalloc((uintptr_t *)(&(aac_data->codec_data)),
                    aac_dec_malloc_table, sizeof(aac_dec_malloc_table) / sizeof(malloc_t_entry)))
    {
        free_data_and_fail(op_data, response_data);
        return TRUE;
    }

    if (scratch_register())
    {
        if (scratch_reserve(AAC_FRAME_MEM_POOL_LENGTH*sizeof(int), MALLOC_PREFERENCE_DM2))
        {
            if (scratch_reserve(AAC_TMP_MEM_POOL_LENGTH*sizeof(int), MALLOC_PREFERENCE_DM1))
            {

                /* Successfully allocated everything! */
                /* Stage 2 populate the DECODER structure */

                /* Tell the codec structure where to find the aac codec data */
                aac_data->decoder_data.codec.decoder_data_object = &(aac_data->codec_data);

#ifdef AACDEC_SBR_ADDITIONS
                aac_data->codec_data.buf_left_ptr                        = aac_data->codec_data.SBR_x_real_ptr     + 1024;
                aac_data->codec_data.buf_right_ptr                       = aac_data->codec_data.SBR_x_real_ptr     + 2048;
                aac_data->codec_data.tmp_mem_pool_ptr                    = aac_data->codec_data.SBR_x_imag_ptr     + 512;
                aac_data->codec_data.tmp_mem_pool_end_ptr                = aac_data->codec_data.tmp_mem_pool_ptr;
                aac_data->codec_data.SBR_synthesis_post_process_imag_ptr = aac_data->codec_data.SBR_x_real_ptr;
                aac_data->codec_data.SBR_X_2env_imag_ptr                 = aac_data->codec_data.SBR_x_imag_ptr;
                aac_data->codec_data.SBR_X_curr_imag_ptr                 = aac_data->codec_data.SBR_x_imag_ptr     + 128;
                aac_data->codec_data.SBR_X_2env_real_ptr                 = aac_data->codec_data.SBR_x_real_ptr     + 512;
                aac_data->codec_data.SBR_X_curr_real_ptr                 = aac_data->codec_data.SBR_x_real_ptr     + 640;
                aac_data->codec_data.SBR_X_sbr_other_real_ptr            = aac_data->codec_data.SBR_x_real_ptr     + 128;
                aac_data->codec_data.SBR_temp_2_ptr                      = aac_data->codec_data.SBR_x_imag_ptr     + 3072;
                aac_data->codec_data.SBR_temp_4_ptr                      = aac_data->codec_data.SBR_x_imag_ptr     + 3200;
#endif

#ifdef AACDEC_PS_ADDITIONS
                aac_data->codec_data.PS_X_hybrid_real_address[0]         = aac_data->codec_data.SBR_synth_temp_ptr;
                aac_data->codec_data.PS_X_hybrid_real_address[1]         = aac_data->codec_data.SBR_synth_temp_ptr + 640;
                aac_data->codec_data.PS_X_hybrid_imag_address[0]         = aac_data->codec_data.SBR_synth_temp_ptr + 320;
#endif

                /* Call the aac decoder init_decode and init_tables functions. */
                aac_data->codec_data.read_frame_function = AAC_LATM;
                aac_decode_lib_init(&(aac_data->decoder_data.codec));


#ifdef INSTALL_OPERATOR_AAC_SHUNT_DECODER
                if (base_op_get_cap_id(op_data) == AAC_SHUNT_DECODER_CAP_ID)
                {
                    aac_data->decoder_data.a2dp_header =(A2DP_HEADER_PARAMS*)
                              ((unsigned*)aac_data + sizeof(AAC_DEC_OP_DATA));
                    aac_data->decoder_data.a2dp_header->type = A2DP_STRIP_AAC_HDR;
                    aac_data->decoder_data.a2dp_header->hdr_size = A2DP_AAC_HDR_SIZE;
                    /* a2dp_header will be valid here, hence no need to check */
                    populate_strip_aac_asm_funcs(&(aac_data->decoder_data.decode_frame),
                               &(aac_data->decoder_data.silence),
                               &(aac_data->decoder_data.a2dp_header->get_bits));
                }
                else  /* CAP_ID_AAC_DECODE */
#endif /* INSTALL_OPERATOR_AAC_SHUNT_DECODER */
                {
                    populate_aac_asm_funcs(&(aac_data->decoder_data.decode_frame),
                               &(aac_data->decoder_data.silence));
                }
                return TRUE;
            }
        }

       /* Fail free all the scratch memory we reserved */
        scratch_deregister();
    }
            /* Clear up all the allocated memory. */
    free_data_and_fail(op_data, response_data);
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

bool aac_decode_buffer_details(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *response_id, void **response_data)
{
    /* Add one complete AAC frame to the base-op derived output buffer size
     * This helps to ensure stable data flow with longer kick periods
     */
    AAC_DEC_OP_DATA *aac_data;
    unsigned terminal_id;
    OP_BUF_DETAILS_RSP *resp;

    terminal_id = OPMGR_GET_OP_BUF_DETAILS_TERMINAL_ID(message_data);
    aac_data = get_instance_data(op_data);

    if (!a2dp_decode_buffer_details_core(op_data, message_data,response_id, response_data,
                                         AAC_DECODE_INPUT_BUFFER_SIZE,
                                         A2DP_DECODE_OUTPUT_BUFFER_SIZE,
                                         AAC_FRAME_BUF_LENGTH))
    {
        return FALSE;
    }
    if ((terminal_id & TERMINAL_SINK_MASK) == TERMINAL_SINK_MASK)
    {
         resp = (OP_BUF_DETAILS_RSP*) *response_data;
        /* The input terminal buffer has changed from codec.in_buffer to terminal_ip_buff
         * Change the metadata buffer accordingly */
         if(aac_data->terminal_ip_buff != NULL &&
             buff_has_metadata(aac_data->terminal_ip_buff))
         {
             resp->metadata_buffer = aac_data->terminal_ip_buff;
         }
         else
         {
             resp->metadata_buffer = NULL;
         }
    }
    return TRUE;
}

/**
 * \brief Deallocates the aac_decode specific capability memory.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the destroy request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool aac_decode_destroy(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *response_id, void **response_data)
{
    AAC_DEC_OP_DATA *aac_data;
    A2DP_DECODER_PARAMS * decoder_data;
    tCbuffer *library_buffer;

    aac_data = get_instance_data(op_data);
    decoder_data = &aac_data->decoder_data;
    library_buffer = decoder_data->codec.in_buffer;

    if(base_op_destroy_lite(op_data, response_data))
    {
        /* Destroy the library buffer */
        cbuffer_destroy(library_buffer);
        library_buffer = NULL;
        /* Free all the scratch memory we reserved */
        scratch_deregister();
        /* Clear up the aac_decode specific work and then let base_op do
         * the grunt work. */
        free_data(op_data);
        return TRUE;
    }

    return FALSE;
}

/**
 * \brief set the aac frame type needed by AAC decoder
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the destroy request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * Mp4 -->0 , adts --->1 , latm --->2
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool aac_set_frame_type(OPERATOR_DATA *op_data, void *message_data,
                                        unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    uint16 frame_type;

    AAC_DEC_OP_DATA *aac_data = get_instance_data(op_data);

    if (opmgr_op_is_running(op_data))
    {
     /* Can't change Frame type  while running */
       return (FALSE);
    }

    frame_type = OPMSG_FIELD_GET(message_data, OPMSG_AAC_SET_FRAME_TYPE, FRAME_TYPE);

#if !defined(AACDEC_MP4_FILE_TYPE_SUPPORTED)
    if (frame_type == AAC_MP4)
    {
     /* Reject a request for a non-configured frame type */
       return (FALSE);
    }
#endif
#if !defined(AACDEC_ADTS_FILE_TYPE_SUPPORTED)
    if (frame_type == AAC_ADTS)
    {
     /* Reject a request for a non-configured frame type */
       return (FALSE);
    }
#endif
#if !defined(AACDEC_LATM_FILE_TYPE_SUPPORTED)
    if (frame_type == AAC_LATM)
    {
     /* Reject a request for a non-configured frame type */
       return (FALSE);
    }
#endif

     /* Call the aac decoder init_decode and init_tables functions. */
    aac_data->codec_data.read_frame_function = frame_type;
    aac_decode_lib_init(&(aac_data->decoder_data.codec));
    return TRUE;
}

/**
 * \brief set the aac Huffman table location needed by AAC decoder
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the destroy request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * RAM -->0 , flash/ROM --->1
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool aac_set_huffman_location(OPERATOR_DATA *op_data, void *message_data,
                                        unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{

    AAC_DEC_OP_DATA *aac_data = get_instance_data(op_data);

    if (opmgr_op_is_running(op_data))
    {
     /* Can't change Huffman location while running */
       return (FALSE);
    }

    aac_data->codec_data.use_huffman_tables_from_rom = OPMSG_FIELD_GET(message_data,
                                OPMSG_AAC_SET_HUFFMAN_TABLES_LOCATION, HUFFMAN_LOCATION);
    return TRUE;
}


/**
 * \brief Resets the aac_decode capability.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the reset request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool aac_decode_reset(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *response_id, void **response_data)
{
    AAC_DEC_OP_DATA *aac_data = get_instance_data(op_data);

    if (!base_op_reset(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    aac_decode_lib_reset(&(aac_data->decoder_data.codec));

    return TRUE;
}

/**
 * \brief Connects the aac_decode capability to terminal buffers
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the reset request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool aac_decode_connect(OPERATOR_DATA *op_data, void *message_data,
                        unsigned *response_id, void **response_data)
{
    unsigned terminal_id;
    AAC_DEC_OP_DATA *aac_data;
    tCbuffer *terminal_buffer;

    aac_data = get_instance_data(op_data);

    /* Check that the capability is not running */
    if (opmgr_op_is_running(op_data))
    {
        return base_op_build_std_response_ex(op_data,
                                             STATUS_CMD_FAILED,
                                             response_data);
    }

    terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);

    if(terminal_id == INPUT_TERMINAL_ID)
    {
        /* At this point, the terminal input buffer should be NULL.
         * Raise an error if that is not the case
         */
        PL_ASSERT(aac_data->terminal_ip_buff == NULL);
        terminal_buffer = OPMGR_GET_OP_CONNECT_BUFFER(message_data);
        aac_data->terminal_ip_buff = terminal_buffer;
        return base_op_build_std_response_ex(op_data, STATUS_OK, response_data);
    }
    else
    {
        return a2dp_decode_connect(op_data, message_data,
                                   response_id, response_data);
    }
}

/**
 * \brief Disconnects the aac_decode capability from the terminal buffers
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the reset request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool aac_decode_disconnect(OPERATOR_DATA *op_data, void *message_data,
                           unsigned *response_id, void **response_data)
{
    AAC_DEC_OP_DATA *aac_data;
    unsigned terminal_id;

    terminal_id = OPMGR_GET_OP_DISCONNECT_TERMINAL_ID(message_data);
    aac_data = get_instance_data(op_data);

    /* Check that the capability is not running, Only the sink can be
     * disconnected whilst running!*/
    if (opmgr_op_is_running(op_data))
    {
        if (terminal_id != INPUT_TERMINAL_ID )
        {
            return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
        }
    }

    if(terminal_id == INPUT_TERMINAL_ID)
    {
        /* Associate the terminal_ip_buffer struct to NULL. The memory allocated
         * to the terminal buffer will be cleared by the framework.
         */
        aac_data->terminal_ip_buff = NULL;
        return base_op_build_std_response_ex(op_data, STATUS_OK, response_data);
    }
    else
    {
        return a2dp_decode_disconnect(op_data, message_data, response_id, response_data);
    }
}

/**
 * \brief Generates zero frame and sets next new tag to end of file tag.
 *
 * \param aac_data Pointer to the aac codec data.
 * \param end_tag Pointer to the meta data tag
 */
static void aac_decode_generate_zero_frame(AAC_DEC_OP_DATA * aac_data, metadata_tag *end_tag)
{
    /* EOF tag found in a corrupt packet or a short packet.
    * We do not have any bytes left in the input to provide
    */
    L2_DBG_MSG("AAC_DEC: Generating zero frame");
    cbuffer_block_fill(aac_data->decoder_data.codec.out_left_buffer,
                       AAC_FRAME_BUF_LENGTH, 0);
    if(aac_data->decoder_data.codec.out_right_buffer != NULL)
    {
        cbuffer_block_fill(aac_data->decoder_data.codec.out_right_buffer,
                           AAC_FRAME_BUF_LENGTH, 0);
    }
    metadata_tag *eof_tag = buff_metadata_copy_tag(end_tag);
    metadata_tag *new_tag = buff_metadata_new_tag();
    tCbuffer *dst = aac_data->decoder_data.metadata_op_buffer;
    if(new_tag != NULL)
    {
        new_tag->length = AAC_FRAME_BUF_LENGTH * OCTETS_PER_SAMPLE;
        METADATA_PACKET_START_SET(new_tag);
        METADATA_PACKET_END_SET(new_tag);
        new_tag->next = eof_tag;
        buff_metadata_append(dst,new_tag, 0, 0);
    }
}

/**
 * \brief Copies data from the terminal input buffer of the capability into
 *        the Kalimba AAC decoder's input buffer
 *
 * \param aac_data Pointer to the aac codec data.
 *
 * \returns TRUE if EOF flag was seen at the terminal buffer, else FALSE
 */
static void aac_decode_copy_input_data(AAC_DEC_OP_DATA *aac_data)
{
    tCbuffer *terminal_buffer, *library_buffer;
    unsigned amt_data_octets, amt_data_words;
    unsigned amt_consumed_octets, amt_words_copied;
    unsigned octets_b4idx, octets_afteridx;
    unsigned amt_space_octets;
    metadata_tag *mtag;
    bool frame_aligned_stream = FALSE;
    bool eof_tag_found = FALSE;

    terminal_buffer = aac_data->terminal_ip_buff;
    library_buffer = aac_data->decoder_data.codec.in_buffer;

    amt_data_octets = cbuffer_calc_amount_data_ex(terminal_buffer);
    amt_space_octets = cbuffer_calc_amount_space_ex(library_buffer);

    /* If there is an EOF tag present in the terminal input buffer, then we copy
     * the entire data present in the input buffer. This may lead to the Kalimba
     * AAC decoder library not able to understand that its input buffer has octet
     * aligned addressing but since this is the last frame, we are mostly safe.
     *
     * If there are no EOF tag present,we only copy word-aligned amount of data
     * thus ensuring that the input buffer seen by the library is always
     * word-aligned. From the computation done above, that would be
     * "amt_data_words" data samples.
     */

    mtag = buff_metadata_peek(terminal_buffer);
    if(mtag != NULL)
    {
        /* Look for the EOF tag */
        metadata_tag *end_tag = mtag;
        if(IS_TIME_TO_PLAY_TAG(end_tag))
        {
            /* Found frame aligned tag */
            frame_aligned_stream = TRUE;
            /* For frame aligned streams - copy just one encoded packet. This is to ensure that
             * there is only one tag corresponding to that packet.
             */
            amt_data_octets = end_tag->length;
            if(amt_data_octets > amt_space_octets)
            {
                /* Do not copy anything if we do not have space for a complete
                 * encoded packet.
                 */
                 amt_data_octets = 0;
            }
        }
        else
        {
            while (end_tag->next != NULL)
            {
                end_tag = end_tag->next;
            }
            eof_tag_found = METADATA_STREAM_END(end_tag);
        }
        if (eof_tag_found || frame_aligned_stream)
        {
            /* The buffer at the input has the EOF marker. Copy the entire data
             * so that the marker is also copied.
             */
             /* Also, if the incoming data is timestamped it means that it is
              * an A2DP source - in which case, each input frame aligns with
              * exactly one AAC frame. In that case also, we copy the entire
              * input data. Since we present a complete frame to the library,
              * it will not read junk data from memory locations which are not
              * written until now */
            L4_DBG_MSG3("aac_decode:copying %d octets from terminal buffer.eof_tag_found: %d,\
                         frame_aligned_stream: %d", amt_data_octets, eof_tag_found, frame_aligned_stream);
            amt_consumed_octets = cbuffer_copy_ex(library_buffer,
                                                  terminal_buffer,
                                                  amt_data_octets);
            metadata_tag *tag = buff_metadata_remove(terminal_buffer,
                                                     amt_consumed_octets,
                                                     &octets_b4idx,
                                                     &octets_afteridx);
            if(NULL != tag)
            {
                buff_metadata_append(library_buffer, tag, octets_b4idx, octets_afteridx);
            }
            return;
        }
    }

    /* There is no EOF/TTP tag present in the terminal buffer.compute the amount of
     * FULL words that can be copied from the terminal buffer
     * into the library's input buffer.
     */
    amt_data_words = amt_data_octets >> 2;
    amt_consumed_octets = amt_data_words * OCTETS_PER_SAMPLE;

    if(amt_data_words > 0)
    {
        L4_DBG_MSG3("aac_decode:amt_data_octets - %d,amt_data_words - %d,amt_consumed_octets - %d",\
                     amt_data_octets,
                     amt_data_words,
                     amt_consumed_octets);
        amt_words_copied = cbuffer_copy(library_buffer, terminal_buffer, amt_data_words);
        if(amt_words_copied != amt_data_words)
        {
            /* Could not copy completly */
            L4_DBG_MSG2("aac_decode: copied %d words instead of %d words",
                        amt_words_copied,
                        amt_data_words);
            amt_data_words = amt_words_copied;
            amt_consumed_octets = amt_data_words * OCTETS_PER_SAMPLE;
        }
        if(amt_data_words > 0)
        {
            /* Extract metadata from the terminal buffer and attach it to the library buffer*/
            metadata_tag *tag = buff_metadata_remove(terminal_buffer,
                                                     amt_consumed_octets,
                                                     &octets_b4idx,
                                                     &octets_afteridx);
            buff_metadata_append(library_buffer, tag, octets_b4idx, octets_afteridx);
        }
    }
}

/**
 * \brief process function to decode available input data
 *
 * \param op_data Pointer to the operator instance data.
 * \param touched Structure to return the terminals which this operator wants kicked
 */
void aac_decode_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    AAC_DEC_OP_DATA *aac_data = get_instance_data(op_data);
    unsigned output_samples;
    stereo_ptrs write_ptrs = {NULL, NULL};

    patch_fn_shared(aac_decode);

    /* Check the input hasn't gone away, if it has then nothing we can do. It's
     * a radio link so this can happen */
    if ((NULL == aac_data->decoder_data.codec.in_buffer) || (NULL == aac_data->terminal_ip_buff))
    {
        /* One option is to produce silence in this case.*/
        return;
    }

    /* Commit any scratch memory ideally this should be done later after the
     * decision to decode is made. The call is cheap so it doesn't hurt to
     * do it here currently. */
    aac_data->codec_data.frame_mem_pool_ptr =
            scratch_commit(AAC_FRAME_MEM_POOL_LENGTH*sizeof(int), MALLOC_PREFERENCE_DM2);
    aac_data->codec_data.tmp_mem_pool_ptr =
            scratch_commit(AAC_TMP_MEM_POOL_LENGTH*sizeof(int), MALLOC_PREFERENCE_DM1);

    aac_data->codec_data.frame_mem_pool_end_ptr =  aac_data->codec_data.frame_mem_pool_ptr;
    aac_data->codec_data.tmp_mem_pool_end_ptr =  aac_data->codec_data.tmp_mem_pool_ptr;


#ifdef AACDEC_SBR_ADDITIONS
    aac_data->codec_data.SBR_temp_1_ptr        = aac_data->codec_data.frame_mem_pool_ptr + 256;
    aac_data->codec_data.SBR_temp_3_ptr        = aac_data->codec_data.frame_mem_pool_ptr + 384;
#endif

#ifdef AACDEC_PS_ADDITIONS
    aac_data->codec_data.PS_fmp_remains_ptr    = aac_data->codec_data.frame_mem_pool_ptr + 512;
#endif

    aac_decode_copy_input_data(aac_data);

    do
    {
        patch_fn_shared(aac_decode);

        unsigned pre_run_bit_position = aac_data->codec_data.get_bitpos;
        unsigned pre_run_bits_consumed = 32 - pre_run_bit_position;
        /* cbuffer_get_read_offset() returns the offset in words (data in 32-bit unpacked mode),
         * (* 4) to convert to octets */
        unsigned pre_run_read_offset = cbuffer_get_read_offset(aac_data->decoder_data.codec.in_buffer) << 2;

        /* Checks for enough data and enough output space are done at the top of
         * this function so it's not done in this C code as well. */

        a2dp_decode_buffer_get_write_ptrs(&(aac_data->decoder_data), &write_ptrs);

        a2dp_decoder_decode(&(aac_data->decoder_data.codec),
                             aac_data->decoder_data.decode_frame,
                             CODEC_NORMAL_DECODE,
                             aac_data->decoder_data.a2dp_header);

        unsigned post_run_bits_consumed = 32 - aac_data->codec_data.get_bitpos;
        /* cbuffer_get_read_offset() returns the offset in words (data in 32-bit unpacked mode),
         * (* 4) to convert to octets */
        unsigned post_run_read_offset = cbuffer_get_read_offset(aac_data->decoder_data.codec.in_buffer) << 2;
        /* cbuffer_get_size_in_words() returns the size in words (data in 32-bit unpacked mode),
         * (* 4) to convert the buffer size to the number of octets actually used */
        unsigned real_buff_size = cbuffer_get_size_in_words(aac_data->decoder_data.codec.in_buffer) << 2;

        int amount_consumed;
        tCbuffer *src, *dst;
        unsigned b4idx, afteridx;
        metadata_tag *mtag;
        bool corrupted_frame = FALSE;
        /* first estimate based on offsets (in octets but with a 16-bit/32 -bit unpacked word resolution),
         * check and correct if read offset wrapped */
        amount_consumed = post_run_read_offset - pre_run_read_offset;
        if (amount_consumed < 0)
        {
             amount_consumed = amount_consumed + real_buff_size;
        }
        /* Adjust calculated amount by the number of bits within a 16-bit/32 bit word consumed
         * pre- and post-processing, byte-align in both cases (the decoder does the same):
         * - for pre-process, the decoder does byte alignment first and starts decoding a new frame
         * - for post-process, we don't consider any partial octets for the amount calculation -
         * the sub-octet amount will be discarded at the start of next frame decode. */
        amount_consumed = amount_consumed - (pre_run_bits_consumed >> 3);
        amount_consumed = amount_consumed + (post_run_bits_consumed >> 3);

        output_samples = aac_data->decoder_data.codec.num_output_samples;

        patch_fn_shared(aac_decode);

        src = aac_data->decoder_data.codec.in_buffer;
        dst = aac_data->decoder_data.metadata_op_buffer;

        if((amount_consumed == 0) &&
           ((aac_data->decoder_data.codec.mode == CODEC_NOT_ENOUGH_INPUT_DATA) ||
            (aac_data->decoder_data.codec.mode == CODEC_FRAME_CORRUPT)))
        {
            mtag = buff_metadata_peek(src);
            if(mtag != NULL)
            {
                /* Look for the EOF tag */
                metadata_tag *end_tag = mtag;
                while (end_tag->next != NULL)
                {
                    end_tag = end_tag->next;
                }
                if METADATA_STREAM_END(end_tag)
                {
                    /* Before generating the zero frame, we need not check
                     * if there is space in the output buffer. This is because
                     * the AAC Decoder library will check for available space
                     * in the output beofre it checks for the amount of data
                     * in the input. And in that case, we would not end up here
                     */
                    aac_decode_generate_zero_frame(aac_data, end_tag);
                    /* Mark the packet as corrupt and output a dummy kick */
                    corrupted_frame = TRUE;
                    touched->sources = TOUCHED_SOURCE_0;
                    if(aac_data->decoder_data.codec.out_right_buffer != NULL)
                    {
                        touched->sources |= TOUCHED_SOURCE_1;
                    }
                }
            }
        }

        /* Metadata handling logic:
         * - remove the input buffer tags if we've consumed encoded data
         * - add new output buffer tags if we've produced output
         * These changes have been prompted by the decoder's behaviour:
         * - consume input stream and produce output audio (normal decode)
         * - consume less than a frame and produce no output audio (corrupted frame)
         * - consume more than a frame and produce no output audio (corrupted frame)
         * - consume and produce nothing (condition known as 'buffer underflow' - there
         *   is less than a frame worth of data in the input buffer)
         */
        /* consumed but not produced? that should be frame corrupt. */
        if (((amount_consumed > 0) && (output_samples == 0)) ||
            (aac_data->decoder_data.codec.mode == CODEC_FRAME_CORRUPT))
        {
            corrupted_frame = TRUE;
        }
        /* A frame is also marked corrupt if it consumes more data
         * than a frame. This is possible in case the link is bad
         */
        else
        {
            mtag = buff_metadata_peek(src);
            if ((mtag != NULL) && (IS_TIME_TO_PLAY_TAG(mtag)))
            {
                if((amount_consumed > 0) && (amount_consumed != mtag->length))
                {
                    corrupted_frame = TRUE;
                }
            }
        }
        /* Handle good frames */
        if ((amount_consumed > 0) &&(!corrupted_frame))
        {
            unsigned output_octets = output_samples * OCTETS_PER_SAMPLE;
            L4_DBG_MSG2("AAC Decode: Consumed %d octets. Produced %d samples", amount_consumed, output_samples);

            mtag = buff_metadata_remove(src, amount_consumed, &b4idx, &afteridx);

            if (buff_has_metadata(dst))
            {
                unsigned output_afteridx = output_octets;

                if ((mtag != NULL) && (IS_TIME_TO_PLAY_TAG(mtag)))
                {
                    /* The incoming metadata tags are frame-aligned, this means
                     * that we will have only one tag per decoded frame and
                     * b4idx should be 0
                     */
                    PL_ASSERT(b4idx == 0);
                    PL_ASSERT(mtag->next == NULL);
                    /* Update the length field of the tag */
                    mtag->length = output_octets;
                }
                else
                {
                    /* The incoming metadata tags are not frame-aligned.
                     * Maybe we are getting data from a file-endpoint where the
                     * application does not know about the frame boundaries.
                     * There can be more than one tag in the processed buffer.
                     * Delete the old tag list and create a new tag for each decoded
                     * frame and align them here.But before that save the EOF tag.
                     */
                    metadata_tag *eof_tag = NULL;
                    if ((afteridx == 0) && (mtag != NULL))
                    {
                        metadata_tag *end_tag = mtag;
                        while (end_tag->next != NULL)
                        {
                            end_tag = end_tag->next;
                        }
                        if METADATA_STREAM_END(end_tag)
                        {
                            eof_tag = buff_metadata_copy_tag(end_tag);
                        }
                    }
                    /* delete the non-aligned tag */
                    buff_metadata_tag_list_delete(mtag);
                    /* Create a new metadata tag */
                    metadata_tag *new_tag = buff_metadata_new_tag();
                    if (new_tag != NULL)
                    {
                        new_tag->length = output_octets;
                        METADATA_PACKET_START_SET(new_tag);
                        METADATA_PACKET_END_SET(new_tag);
                    }
                     mtag = new_tag;
                    /* Append the EOF Tag */
                    if (new_tag != NULL)
                    {
                        if (eof_tag != NULL)
                        {
                            new_tag->next = eof_tag;
                            output_afteridx = 0; /* length of eof_tag */
                        }
                    }
                }
                if (mtag != NULL)
                {
                    buff_metadata_append(dst, mtag, 0, output_afteridx);
                }
                else
                {
                    /* No new tags. Continue adding data to the last tag.*/
                    buff_metadata_append(dst, NULL, output_octets, 0);
                }
            }
            else
            {
                buff_metadata_delete_tag(mtag, TRUE);
            }
        }
        else if(corrupted_frame)
        {
            /* In case the frame is detected as corrupt. We will discard all data in the
             * library buffer. This means resetting the library buffer and setting the
             * bitpos to 32 (first octet in the new word)
             */
            unsigned discard_amt;

            discard_amt = cbuffer_calc_amount_data_in_words(src);
            L2_DBG_MSG1("AAC DECODER: Corrupt frame detected - discarding the library ip buffer containing %d words", discard_amt);
            cbuffer_empty_buffer_and_metadata(src);
            aac_data->codec_data.get_bitpos = 32;


            /* In case the frame was marked as corrupt by the wrapper
             * and the library produced some output, we will discard that
             * data by rolling back the write pointer to its original
             * value before the decode happened and set the output samples
             * to zero.
             * If we do not do this, the output will not be copied into
             * the next transform buffer and will end up with NOT_ENOUGH_OUTPUT_SPACE
             * continuously and a stall.
             */

            if (output_samples > 0)
            {
                tCbuffer *left_out_buffer, *right_out_buffer;
                left_out_buffer = aac_data->decoder_data.codec.out_left_buffer;
                right_out_buffer = aac_data->decoder_data.codec.out_right_buffer;
                if (left_out_buffer)
                {
                    left_out_buffer->write_ptr = write_ptrs.left;
                }
                if (right_out_buffer)
                {
                    right_out_buffer->write_ptr = write_ptrs.right;
                }
                output_samples = 0;
            }
        }

        patch_fn_shared(aac_decode);
        if (output_samples > 0)
        {
            /* Source 0 is always touched */
            unsigned touched_sources = TOUCHED_SOURCE_0;

            /* Handle mono/stereo fade-out */
            if (a2dp_decode_check_and_perform_fadeout(&(aac_data->decoder_data), output_samples, &write_ptrs))
            {
                common_send_simple_unsolicited_message(op_data, OPMSG_REPLY_ID_FADEOUT_DONE);
            }

           /* If stereo outputs are connected then source 1 was touched as well */
           if (aac_data->decoder_data.codec.out_right_buffer != NULL)
           {
               touched_sources |= TOUCHED_SOURCE_1;
           }

           /* Source(s) touched */
           touched->sources = touched_sources;
        }

    }while(aac_data->decoder_data.codec.mode == CODEC_SUCCESS);

    /* Free the scratch memory used */
    scratch_free();


#ifdef INSTALL_AAC_DATA_TEST
    /* Kick the input to get it going */
    touched->sinks |= TOUCHED_SINK_0;
#else
    /* In case of an error (CODEC_ERROR or CODEC_FRAME_CORRUPT) kick backwards
     * if there is less than the maximum AAC frame size (worst case scenario).
     */
    if ((aac_data->decoder_data.codec.mode == CODEC_NOT_ENOUGH_INPUT_DATA) ||
        (((aac_data->decoder_data.codec.mode == CODEC_ERROR) || (aac_data->decoder_data.codec.mode == CODEC_FRAME_CORRUPT)) &&
         (cbuffer_calc_amount_data_in_words(aac_data->decoder_data.codec.in_buffer) < MAX_AAC_FRAME_SIZE_IN_WORDS)))
        {
            touched->sinks |= TOUCHED_SOURCE_0;
        }
#endif /* INSTALL_AAC_DATA_TEST */
}
#ifdef INSTALL_OPERATOR_AAC_SHUNT_DECODER

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
bool aac_dec_opmsg_content_protection_enable(OPERATOR_DATA *op_data,
                    void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AAC_DEC_OP_DATA *op_extra_data = get_instance_data(op_data);
    A2DP_HEADER_PARAMS* a2dp_header;

    a2dp_header = (A2DP_HEADER_PARAMS*) op_extra_data->decoder_data.a2dp_header;


    if (a2dp_header == NULL || opmgr_op_is_running(op_data))
    {
        return FALSE;
    }

    /* Set the Content protection bit */
    if(OPMSG_FIELD_GET(message_data, OPMSG_A2DP_DEC_CONTENT_PROTECTION_ENABLE, ENABLE_CP) != 0 )
    {
        a2dp_header->type = (unsigned) A2DP_STRIP_AAC_CP_HDR;
        a2dp_header->hdr_size =  (unsigned)A2DP_AAC_CP_HDR_SIZE;
    }
    else
    {
        a2dp_header->type = (unsigned)A2DP_STRIP_AAC_HDR;
        a2dp_header->hdr_size = (unsigned) A2DP_AAC_HDR_SIZE;
    }

    return TRUE;
}

#endif /* INSTALL_OPERATOR_AAC_SHUNT_DECODER */
bool aac_dec_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* In the case of this capability, nothing is done for control message. Just follow protocol and ignore any content. */
    return cps_control_setup(message_data, resp_length, resp_data,NULL);

}


bool aac_dec_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    return FALSE;
}

bool aac_dec_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{

     return FALSE;
}

bool aac_dec_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* Set the parameter(s). For future proofing, it is using the whole mechanism, although currently there is only one field
     * in opdata structure that is a setable parameter. If later there will be more (ever), must follow contiquously the first field,
     * as commented and instructed in the op data definition. Otherwise consider moving them into a dedicated structure.
     */

    return FALSE;
}

bool aac_dec_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AAC_DEC_OP_DATA *op_extra_data = get_instance_data(op_data);

    unsigned* resp = NULL;



    if(!common_obpm_status_helper(message_data,resp_length,resp_data,sizeof(AAC_DECODE_STATISTICS) ,&resp))
    {
         return FALSE;
    }
    // TBD: check for NULL pointers
    if(resp)
    {
            resp = cpsPack2Words(op_extra_data->codec_data.sf_index_field, op_extra_data->codec_data.channel_configuration_field, resp);
            resp = cpsPack2Words(op_extra_data->codec_data.audio_object_type_field, op_extra_data->codec_data.extension_audio_object_type_field, resp);
            cpsPack2Words(op_extra_data->codec_data.sbr_present_field, op_extra_data->codec_data.mp4_frame_count, resp);
    }

    return TRUE;
}
