/****************************************************************************
 * Copyright (c) 2014 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup wbs
 * \file  async_wbs_private.h
 * \ingroup capabilities
 *
 * Wide Band Speech operators private header file. <br>
 *
 */

#ifndef ASYNC_WBS_PRIVATE_H
#define ASYNC_WBS_PRIVATE_H
/*****************************************************************************
Include Files
*/
#include "async_wbs_struct.h"
#include "wbs_private.h"

/****************************************************************************
Private Const Declarations
*/

#ifdef CAPABILITY_DOWNLOAD_BUILD
#define ASYNC_WBS_ENC_CAP_ID CAP_ID_DOWNLOAD_ASYNC_WBS_ENC
#define ASYNC_WBS_DEC_CAP_ID CAP_ID_DOWNLOAD_ASYNC_WBS_DEC
#else
#define ASYNC_WBS_ENC_CAP_ID CAP_ID_ASYNC_WBS_ENC
#define ASYNC_WBS_DEC_CAP_ID CAP_ID_ASYNC_WBS_DEC
#endif

/* Apps will fake BT records and will assume Tesco=12. */
#define ASYNC_WBS_DEFAULT_TESCO (12)

/* Default step in the timestamp field of the BT record headers.
 * In SCO, BT record header's timestamp field increments by 2*Tesco. */
#define ASYNC_WBS_DEFAULT_TS_STEP (2*ASYNC_WBS_DEFAULT_TESCO)

#define ASYNC_WBS_MAX_BITPOOL_VALUE (26)
#define ASYNC_WBS_MIN_BITPOOL_VALUE (10) /* Below this value audio is not understandable*/
#define ASYNC_WBS_MIN_OUTPUT_BLOCK_SIZE (15)

/****************************************************************************
Private Type Definitions
*/

/****************************************************************************
Private Macro Declarations
*/
/**
 * Convert a quantity in octets (as seen by the BTSS) to words
 * (as seen by the audio subsystem), assuming the buffer mode is 16-bit
 * unpacked. If the length in octets is uneven the words number will
 * be rounded up.
 */
#define CONVERT_OCTETS_TO_WORDS(x) (((x) + 1) / 2)

/**
 * Convert a quantity in SCO words back to octets.
 */
#define CONVERT_WORDS_TO_OCTETS(x) ((x) * 2)

/*****************************************************************************
Private Function Declarations
*/
extern unsigned awbs_encode_frame_size(sbc_codec *codec_data);
extern bool awbs_enc_opmsg_set_encoding_params(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool awbs_enc_opmsg_set_frames_per_packet(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool awbs_dec_opmsg_set_encoding_params(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

/* Message handlers */
extern bool async_wbs_enc_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool async_wbs_enc_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool async_wbs_enc_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool async_wbs_enc_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool async_wbs_enc_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/* Data processing function */
extern void async_wbs_enc_process_data(OPERATOR_DATA*, TOUCHED_TERMINALS*);

/* Message handlers */
extern bool async_wbs_dec_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool async_wbs_dec_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool async_wbs_dec_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool async_wbs_dec_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool async_wbs_dec_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool async_wbs_dec_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool async_wbs_dec_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/* Data processing function */
extern void async_wbs_dec_process_data(OPERATOR_DATA*, TOUCHED_TERMINALS*);

/* get current encoder/decoder buffer size */
extern bool async_wbs_enc_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool async_wbs_dec_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
#endif /* ASYNC_WBS_PRIVATE_H */
