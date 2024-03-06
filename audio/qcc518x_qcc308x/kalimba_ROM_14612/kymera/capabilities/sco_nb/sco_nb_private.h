/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup sco_nb
 * \file  sco_nb_private.h
 * \ingroup opmgr
 *
 * Sco NBS operators private header file. <br>
 *
 */

#ifndef _SCO_NB_PRIVATE_H_
#define _SCO_NB_PRIVATE_H_
/*****************************************************************************
Include Files
*/
#include "capabilities.h"
#include "sco_common_funcs.h"
#include "sco_fw_output.h"
#include "fault/fault.h"
#include "sco_nb_struct.h"
#ifdef INSTALL_PLC100
#include "sco_plc100.h"
#endif /* INSTALL_PLC100 */
/****************************************************************************
Public Constant Definitions
*/
#ifdef CVSD_CODEC_SOFTWARE
    /*  TODO this should go in a config
     * When running a CVSD decoder in FW, the amount of input data received
     * by the SCO Rx FW is halved compared to when it is performed by H/W.
     */
#define SW_CVSD_RATIO 2
#else
#define SW_CVSD_RATIO 1
#endif /* CVSD_CODEC_SOFTWARE */

/* Capability Version */
#define SCO_RCV_NB_VERSION_MINOR            1


/** Minimum block size in words used in SCO */
#define SCO_MIN_BLOCK_SIZE                            15
#define SCO_DEFAULT_BLOCK_SIZE                        30

#define NBS_SAMPLE_RATE 8000

/****************************************************************************
Public Type Declarations
*/

/*****************************************************************************
Private Function Definitions
*/

/* Decode processing function */
extern unsigned sco_rcv_processing(OPERATOR_DATA *op_data);

extern unsigned sco_rcv_get_output_size_words(unsigned input_bytes);

/* Message handlers */
extern bool sco_send_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool sco_send_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool sco_send_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool sco_send_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool sco_send_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool sco_send_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool sco_send_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool sco_send_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool sco_send_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/* Op msg handlers */
extern bool sco_send_opmsg_enable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool sco_send_opmsg_disable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

/* Data processing function */
extern void sco_send_process_data(OPERATOR_DATA*, TOUCHED_TERMINALS*);

/* Message handlers */
extern bool sco_rcv_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool sco_rcv_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool sco_rcv_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool sco_rcv_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool sco_rcv_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool sco_rcv_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool sco_rcv_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool sco_rcv_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/* Op msg handlers */
extern bool sco_rcv_opmsg_enable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool sco_rcv_opmsg_disable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
#ifdef INSTALL_PLC100
extern bool sco_rcv_opmsg_force_plc_off(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
#endif /* INSTALL_PLC100 */
extern bool sco_rcv_opmsg_frame_counts(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool sco_rcv_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool sco_rcv_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool sco_rcv_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool sco_rcv_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool sco_rcv_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

/* Data processing function */
extern void sco_rcv_process_data(OPERATOR_DATA*, TOUCHED_TERMINALS*);

#endif /* _SCO_NB_PRIVATE_H_ */
