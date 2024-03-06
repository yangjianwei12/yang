/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  anc_compander_cap.h
 * \ingroup anc_hw_manager
 *
 * ANC Compander operator private header file.
 *
 */

#ifndef _ANC_COMPANDER_CAP_H_
#define _ANC_COMPANDER_CAP_H_
/*****************************************************************************
Include Files
*/
#include "capabilities.h"
#include "opmgr/opmgr.h"
#include "adaptor/connection_id.h"
#include "anc_compander_gen_c.h"
#include "compander_c.h"
#include "anc_compander_defs.h"
#include "op_msg_utilities.h"
#include "base_aud_cur_op/base_aud_cur_op.h"
#include "anc_hw_manager/anc_hw_manager_gain.h"
#include "anc_hw_manager/anc_hw_manager_common.h"

/****************************************************************************
Private Constant Definitions
*/
/* Capability minor version. Capability versions:
 *
 * Version | Description
 * --------|--------------------
 *   1.1   | Initial Release
 *   1.2   | Remove support for shared gain priority
 */
#define ANC_COMPANDER_ANC_COMPANDER_VERSION_MINOR 2
#define ANC_COMPANDER_NUM_PARAMETERS \
            (sizeof(ANC_COMPANDER_PARAMETERS) >> LOG2_ADDR_PER_WORD)
#define ANC_COMPANDER_GAIN_SHIFT 8
#define ANC_COMPANDER_Q_SHIFT 19
#define ANC_COMPANDER_MAX_GAIN 255
#define ANC_COMPANDER_MAX_GAIN_Q13 (ANC_COMPANDER_MAX_GAIN << ANC_COMPANDER_Q_SHIFT)
#define ANC_COMPANDER_GAIN_LSB (1 << (ANC_COMPANDER_Q_SHIFT - 1))
#define ANC_COMPANDER_MIN_GAIN 0

/* capability-specific extra operator data */
typedef struct anc_compander_exop
{
    /* Pointer to compander object*/
    t_compander_object *compander_object;

    /* Compander parameters */
    ANC_COMPANDER_PARAMETERS compander_cap_params;

    /* Capability sample rate */
    unsigned sample_rate;

    /* Mode control */
    unsigned cur_mode;
    unsigned host_mode;
    unsigned qact_mode;
    unsigned ovr_control;

    /* Lookahead status*/
    unsigned lookahead_status;

    /* Shared FF or FB fine gain for controlling the ANC HW */
    AHM_SHARED_FINE_GAIN *p_fine_gain;
    /* Pointer to shared AANC gain */
    AHM_SHARED_FINE_GAIN * p_aanc_gain;
    /* Pointer to FF or FB static gain from AHM */
    AHM_GAIN * p_static_gain;
    /* Nominal gain for compander */
    uint16 * p_nominal_gain;
    /* FF/FB filter path used for companding */
    AHM_ANC_FILTER compander_filter_path;
    uint16 ahm_op_id;

    /* Internal cBuffer to temporarily hold compander output */
    tCbuffer * internal_buffer;
} ANC_COMPANDER_OP_DATA;

/*****************************************************************************
Private Function Definitions
*/

/* Standard Capability API handlers */
extern bool anc_compander_create(OPERATOR_DATA *op_data,
                                 void *message_data,
                                 unsigned *response_id,
                                 void **response_data);
extern bool anc_compander_destroy(OPERATOR_DATA *op_data,
                                  void *message_data,
                                  unsigned *response_id,
                                  void **response_data);
extern bool anc_compander_buffer_details(OPERATOR_DATA *op_data,
                                         void *message_data,
                                         unsigned *response_id,
                                         void **response_data);

/* Standard Opmsg handlers */
extern bool anc_compander_opmsg_set_control(OPERATOR_DATA *op_data,
                                            void *message_data,
                                            unsigned *resp_length,
                                            OP_OPMSG_RSP_PAYLOAD **resp_data);

extern bool anc_compander_opmsg_get_status(OPERATOR_DATA *op_data,
                                           void *message_data,
                                           unsigned *resp_length,
                                           OP_OPMSG_RSP_PAYLOAD **resp_data);

extern bool anc_compander_opmsg_set_sample_rate(OPERATOR_DATA *op_data,
                                                void *message_data,
                                                unsigned *resp_length,
                                                OP_OPMSG_RSP_PAYLOAD **resp_data);

extern bool anc_compander_opmsg_link_ahm(OPERATOR_DATA *op_data,
                                         void *message_data,
                                         unsigned *resp_length,
                                         OP_OPMSG_RSP_PAYLOAD **resp_data);

extern bool anc_compander_opmsg_get_shared_gain_ptr(OPERATOR_DATA *op_data,
                                                    void *message_data,
                                                    unsigned *resp_length,
                                                    OP_OPMSG_RSP_PAYLOAD **resp_data);
/* Custom Opmsg handlers */
/**
 * \brief  Operator message to get adjusted gain.
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  resp_length      Length of response
 * \param  resp_data        Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool anc_compander_opmsg_get_adjusted_gain(OPERATOR_DATA *op_data,
                                                  void *message_data,
                                                  unsigned *resp_length,
                                                  OP_OPMSG_RSP_PAYLOAD **resp_data);

/**
 * \brief  Operator message to get AANC shared gain pointer.
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  resp_length      Length of response
 * \param  resp_data        Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool anc_compander_opmsg_link_aanc_gain(OPERATOR_DATA *op_data,
                                               void *message_data,
                                               unsigned *resp_length,
                                               OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool anc_compander_opmsg_set_makeup_gain(OPERATOR_DATA *op_data,
                                                void *message_data,
                                                unsigned *resp_length,
                                                OP_OPMSG_RSP_PAYLOAD **resp_data);
/* Standard data processing function */
extern void anc_compander_process_data(OPERATOR_DATA*, TOUCHED_TERMINALS*);

/* Capability hook functinos to work with base_aud_cur_op */

/**
 * \brief  Operator connect hook function
 *
 * \param  op_data  Address of operator data
 * \param  terminal_id ID of terminal to be connected
 *
 * \return  boolean indicating success or failure.
 */
bool anc_compander_connect_hook(OPERATOR_DATA *op_data, unsigned terminal_id);

/**
 * \brief  Operator disconnect hook function
 *
 * \param  op_data  Address of operator data
 * \param  terminal_id ID of terminal to be dis-connected
 *
 * \return  boolean indicating success or failure.
 */
bool anc_compander_disconnect_hook(OPERATOR_DATA *op_data, unsigned terminal_id);

/**
 * \brief  Param update/SET_UCID hook function
 *
 * \param  op_data  Address of operator data
 *
 * \return  boolean indicating success or failure.
 */
bool anc_compander_param_update_hook(OPERATOR_DATA *op_data);

/* ASM Function defs */
extern unsigned anc_compander_processing(t_compander_object *compander_obj);
extern void anc_compander_proc_init(ANC_COMPANDER_OP_DATA *op_data);

#endif /* _ANC_COMPANDER_CAP_H_ */

