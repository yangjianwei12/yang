/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup swbs
 * \file  swbs_private.h
 * \ingroup capabilities
 *
 * Super Wide Band Speech operators private header file. <br>
 *
 */

#ifndef SWBS_PRIVATE_H
#define SWBS_PRIVATE_H
/*****************************************************************************
Include Files
*/
#include "types.h"
#include "swbs_struct.h"
#include "capabilities.h"
#include "sco_common_funcs.h"
#include "sco_fw_output.h"
#include "mem_utils/shared_memory_ids.h"
#include "mem_utils/scratch_memory.h"
#include "mem_utils/memory_table.h"
#include "common/interface/util.h"
#include "fault/fault.h"

/****************************************************************************
Private Const Declarations
*/
#define SWBS_DEC_MALLOC_TABLE_LENGTH 1
#define SWBS_ENC_MALLOC_TABLE_LENGTH 1
#define SWBS_DM1_SCRATCH_TABLE_LENGTH (2)                      /// **** CHECK

#define SWBS_CODEC_MODE0 0
#define SWBS_CODEC_MODE4 4

#define DEC_WEAK_HEADER_SIZE  4

#define SWBS_DEFAULT_ENCODED_BLOCK_SIZE  30                // in words
#define SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES  60       // in bytes
#define SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES_WITH_BIT_ERROR ((2*SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES)+DEC_WEAK_HEADER_SIZE)

#define SWBS_MODE0_AUDIO_BLOCK_SIZE 240
#define SWBS_MODE4_AUDIO_BLOCK_SIZE 180

#define SWBS_MODE0_SAMPLE_RATE 32000
#define SWBS_MODE4_SAMPLE_RATE 24000

/** Default buffer size, minimum and default block size for swbs_enc */
#define SWBS_ENC_DEFAULT_OUTPUT_BLOCK_SIZE        SWBS_DEFAULT_ENCODED_BLOCK_SIZE
#define SWBS_ENC_DEFAULT_INPUT_BLOCK_SIZE         SWBS_MODE0_AUDIO_BLOCK_SIZE
#define SWBS_ENC_INPUT_BUFFER_SIZE                (1024)
#define SWBS_ENC_OUTPUT_BUFFER_SIZE               (SCO_DEFAULT_SCO_BUFFER_SIZE)

/** Default buffer sizes, minimum and default block size for swbs_dec */
#define SWBS_DEC_DEFAULT_INPUT_BLOCK_SIZE          0
#define SWBS_DEC_DEFAULT_OUTPUT_BLOCK_SIZE         SWBS_MODE0_AUDIO_BLOCK_SIZE
#define SWBS_DEC_INPUT_BUFFER_SIZE                 (SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES_WITH_BIT_ERROR)
#define SWBS_DEC_OUTPUT_BUFFER_SIZE                (1024)

/* SWBS shared table lengths */
#define SWBS_APTX_SHARED_TABLE_LENGTH     3   /// **** CHECK
#define SWBS_APTX_ENC_SHARED_TABLE_LENGTH 2   /// **** CHECK
#define SWBS_APTX_DEC_SHARED_TABLE_LENGTH 3   /// **** CHECK

#define SWBS_AUDIO_SAMPLE_BUFF_SIZE  240

#ifdef CAPABILITY_DOWNLOAD_BUILD
#define SWBS_ENC_CAP_ID CAP_ID_DOWNLOAD_SWBS_ENC
#define SWBS_DEC_CAP_ID CAP_ID_DOWNLOAD_SWBS_DEC
#else
#define SWBS_ENC_CAP_ID CAP_ID_SWBS_ENC
#define SWBS_DEC_CAP_ID CAP_ID_SWBS_DEC
#endif

#define SWBS_DEC_PROCESS_OK 0
#define SWBS_DEC_PROCESS_NO_OUTPUT -1
#define SWBS_DEC_PROCESS_FAKE_FRAME -2

/****************************************************************************
Private Type Definitions
*/
/*****************************************************************************
SWBS shared tables
*/

/*****************************************************************************
Private Function Declarations
*/
/* Reset for aptX Adaptive codec data */
extern void swbs_enc_reset_aptx_data(OPERATOR_DATA* op_data);

/* Decode processing function */
extern unsigned swbs_dec_processing(OPERATOR_DATA *op_data);

/* Encoder ASM defined functions and C stubs to aptX adaptive encoder/decoder*/
extern void swbsenc_init_encoder(OPERATOR_DATA *op_data);
extern void swbsdec_init_dec_param(OPERATOR_DATA *op_data);
extern void swbsenc_process_frame(OPERATOR_DATA *op_data);

/* Message handlers */
extern bool swbs_enc_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool swbs_enc_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool swbs_enc_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool swbs_enc_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool swbs_enc_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool swbs_enc_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool swbs_enc_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool swbs_enc_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/* Op msg handlers */
extern bool swbs_enc_opmsg_enable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool swbs_enc_opmsg_disable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool swbs_enc_opmsg_codec_mode(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

/* Data processing function */
extern void swbs_enc_process_data(OPERATOR_DATA*, TOUCHED_TERMINALS*);
extern unsigned fake_packet_swbs(OPERATOR_DATA *op_data);

/* Message handlers */
extern bool swbs_dec_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool swbs_dec_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool swbs_dec_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool swbs_dec_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool swbs_dec_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool swbs_dec_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool swbs_dec_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/* Op msg handlers */
extern bool swbs_dec_opmsg_enable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool swbs_dec_opmsg_disable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool swbs_dec_opmsg_force_plc_off(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool swbs_dec_opmsg_codec_mode(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

extern bool swbs_dec_opmsg_frame_counts(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool swbs_dec_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool swbs_dec_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool swbs_dec_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool swbs_dec_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool swbs_dec_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

/* Data processing function */
extern void swbs_dec_process_data(OPERATOR_DATA*, TOUCHED_TERMINALS*);
/**
 * \brief Validates an incoming SWBS packet.
 *
 * \param opdata SWBS operator data.
 * \param payload_length SCO packet payload length.
 * \param swbs_packet_length pointer which will be populated with the SWBS packet length.
 * \param amount_advanced pointer which will be populated with the amount the read pointer advanced.
 *
 * \return Validate returns in r1 240 (for 32khz) or 180 (for 24kHz) (1 frames worth of data) if
 *         everything is alright. If it returns 0, or 1 then there is no data
 *           0 means there is not enough space in output buffer
 *           1 means there is not enough data in the input buffer

 */
extern unsigned sco_decoder_swbs_validate(void* opdata, unsigned payload_length, unsigned *swbs_packet_length);
#ifdef ESCO_SUPPORT_ERRORMASK
extern unsigned sco_decoder_swbs_validate_wbm(void* opdata, unsigned payload_length, unsigned *swbs_packet_length);
#endif

/**
 * \brief Process an incoming SWBS packet.
 *
 * \param opdata SWBS operator data.
 * \param packet The incoming SCO stream packet.
 * \param swbs_packet_length SWBS packet length in the payload.
 *
 * \return output packet status:
 *          -2 output bad, GENERATE_FAKE_FRAME     BFI
 *          -1 no output                           RETCODE_NO_OUTPUT
 *           0 output good no compensation         BFI
 *           1 output bad, needs compensation      BFI
 *           2 output bad, needs compensation      BFI
 */
extern int sco_decoder_swbs_process(void* opdata, unsigned swbs_packet_length);
#ifdef ESCO_SUPPORT_ERRORMASK
extern int sco_decoder_swbs_process_bit_error (void* opdata, unsigned swbs_packet_length);
#endif
/* get current encoder buffer size */
extern bool swbs_enc_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
/* get current decoder buffer size */
extern bool swbs_dec_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);


extern unsigned swbs_run_plc(OPERATOR_DATA *op_data, unsigned packet_size);

/**
 * \brief The SWBS codec works with either 24k or 32k audio streams. This is
 *        selected after creation, setting OPMSG_SWBS_DEC_ID_CODEC_MODE.
 *        This is needed by the sco_fw to process metadata.
 *
 * \param opdata SWBS operator data.
 *
 * \return       sample rate
 */
extern unsigned swbs_get_sample_rate(OPERATOR_DATA *op_data);

#endif /* SWBS_PRIVATE_H */
