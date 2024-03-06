/****************************************************************************
 * Copyright (c) 2015 - 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  anc_compander_cap.c
 * \ingroup  operators
 *
 *  ANC Compander operator
 *
 */
/****************************************************************************
Include Files
*/

#include "capabilities.h"
#include "mem_utils/scratch_memory.h"
#include "anc_compander_cap.h"
#include "compander_c.h"

/****************************************************************************
Private Type Definitions
*/
#ifdef CAPABILITY_DOWNLOAD_BUILD
#define ANC_COMPANDER_CAP_ID CAP_ID_DOWNLOAD_ANC_COMPANDER
#else
#define ANC_COMPANDER_CAP_ID CAP_ID_ANC_COMPANDER
#endif

/*****************************************************************************
Private Constant Declarations
*/
/** The compander capability function handler table */
const handler_lookup_struct anc_compander_handler_table =
{
    anc_compander_create,          /* OPCMD_CREATE */
    anc_compander_destroy,         /* OPCMD_DESTROY */
    aud_cur_start,                 /* OPCMD_START */
    aud_cur_stop,                  /* OPCMD_STOP */
    aud_cur_reset,                 /* OPCMD_RESET */
    aud_cur_connect,               /* OPCMD_CONNECT */
    aud_cur_disconnect,            /* OPCMD_DISCONNECT */
    aud_cur_buffer_details,        /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,       /* OPCMD_DATA_FORMAT */
    aud_cur_get_sched_info         /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table */
const opmsg_handler_lookup_table_entry anc_compander_opmsg_handler_table[] =
    {
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,  base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_SET_CONTROL,             anc_compander_opmsg_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,              aud_cur_opmsg_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,            aud_cur_opmsg_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,              aud_cur_opmsg_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,              anc_compander_opmsg_get_status},
    {OPMSG_COMMON_ID_SET_UCID,                aud_cur_opmsg_set_ucid},
    {OPMSG_COMMON_ID_GET_LOGICAL_PS_ID,       aud_cur_opmsg_get_ps_id},
    {OPMSG_COMMON_SET_SAMPLE_RATE,            anc_compander_opmsg_set_sample_rate},
    {OPMSG_COMMON_ID_SET_BUFFER_SIZE,         aud_cur_opmsg_set_buffer_size},
    {OPMSG_COMMON_ID_LINK_ANC_HW_MANAGER,     anc_compander_opmsg_link_ahm},
    {OPMSG_COMMON_ID_GET_SHARED_GAIN,         anc_compander_opmsg_get_shared_gain_ptr},

    {OPMSG_ADRC_ID_ADRC_GET_ADJUSTED_GAIN,    anc_compander_opmsg_get_adjusted_gain},
    {OPMSG_ADRC_ID_ADRC_LINK_AANC_GAIN,       anc_compander_opmsg_link_aanc_gain},
    {OPMSG_ADRC_ID_ADRC_SET_MAKEUP_GAIN,      anc_compander_opmsg_set_makeup_gain},
    {0, NULL}};

const CAPABILITY_DATA anc_compander_cap_data =
{
    /* Capability ID */
    ANC_COMPANDER_CAP_ID,
    /* Version information - hi part */
    ANC_COMPANDER_ANC_COMPANDER_VERSION_MAJOR,
    /* Version information - lo part */
    ANC_COMPANDER_ANC_COMPANDER_VERSION_MINOR,
    /* Max number of sinks/inputs and sources/outputs */
    ANC_COMPANDER_CAP_MAX_CHANNELS, ANC_COMPANDER_CAP_MAX_CHANNELS,
    /* Pointer to message handler function table */
    &anc_compander_handler_table,
    /* Pointer to operator message handler function table */
    anc_compander_opmsg_handler_table,
    /* Pointer to data processing function */
    anc_compander_process_data,
    /* Reserved */
    0,
    /* Size of capability-specific per-instance data */
    sizeof(ANC_COMPANDER_OP_DATA)
};

#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_ANC_COMPANDER, ANC_COMPANDER_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_ANC_COMPANDER, ANC_COMPANDER_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

/****************************************************************************
Inline functions
*/
static inline ANC_COMPANDER_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (ANC_COMPANDER_OP_DATA *) base_op_get_instance_data(op_data);
}

/****************************************************************************
Static functions
*/
/**
 * \brief  Communicate with the ANC HW Manager to release the shared gain
 *
 * \param  op_data       Pointer to the operator data
 *
 * \return - None
 *
 */
static void anc_compander_release_shared_gain(ANC_COMPANDER_OP_DATA *p_ext_data)
{
    if ((p_ext_data->p_fine_gain == NULL) || (p_ext_data->ahm_op_id == 0))
    {
        /* Nothing to do */
        return;
    }

    aud_cur_release_shared_fine_gain(p_ext_data->p_fine_gain,
                                     p_ext_data->compander_filter_path,
                                     AHM_GAIN_CONTROL_TYPE_NOMINAL,
                                     p_ext_data->ahm_op_id,
                                     AHM_ANC_INSTANCE_ANC0_ID);

    p_ext_data->p_fine_gain = NULL;

    return;
}

static void anc_compander_init_filter_path(ANC_COMPANDER_OP_DATA *p_ext_data)
{
    if ((p_ext_data->compander_cap_params.OFFSET_COMPANDER_CONFIG &
            ANC_COMPANDER_CONFIG_IS_FB_COMPANDER) > 0)
    {
        p_ext_data->compander_filter_path = AHM_ANC_FILTER_FB_ID;
    }
    else
    {
        p_ext_data->compander_filter_path = AHM_ANC_FILTER_FF_ID;
    }
}

static bool anc_compander_opmsg_link_ahm_callback(CONNECTION_LINK con_id,
                                                  STATUS_KYMERA status,
                                                  EXT_OP_ID op_id,
                                                  unsigned num_resp_params,
                                                  unsigned *resp_params)
{
    unsigned raw_ptr, raw_gain;
    ANC_COMPANDER_OP_DATA *p_ext_data;
    AHM_SHARED_FINE_GAIN *p_gain;
    AHM_GAIN_BANK *p_static_gains;

    if (status != ACCMD_STATUS_OK)
    {
        L0_DBG_MSG1("ADRC link response failed: status=%d", status);
        return FALSE;
    }

    raw_ptr = OPMSG_CREATION_FIELD_GET32(resp_params,
                                         OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                         P_EXT_DATA);
    p_ext_data = (ANC_COMPANDER_OP_DATA*)raw_ptr;

    raw_gain = OPMSG_CREATION_FIELD_GET32(resp_params,
                                          OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                          SHARED_GAIN_PTR);
    p_gain = (AHM_SHARED_FINE_GAIN*)raw_gain;
    p_ext_data->p_fine_gain = p_gain;

    /* Get static gain pointer from AHM */
    raw_gain = OPMSG_CREATION_FIELD_GET32(resp_params,
                                          OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                          STATIC_GAINS_PTR);
    p_static_gains = (AHM_GAIN_BANK *)raw_gain;

    switch (p_ext_data->compander_filter_path)
    {
        case AHM_ANC_FILTER_FF_ID:
            p_ext_data->p_static_gain = &p_static_gains->ff;
            break;
        case AHM_ANC_FILTER_FB_ID:
            p_ext_data->p_static_gain = &p_static_gains->fb;
            break;
        default:
            L2_DBG_MSG1("ADRC unsupported filter path for compander: %u",
                        p_ext_data->compander_filter_path);
            break;
    }

    return TRUE;
}
/****************************************************************************
Capability API Handlers
*/

/**
 * \brief  Clean up any memory allocated by the ANC compander.
 *
 * \param  op_data          Pointer to operator data
 *
 * \return  None
 *
 */
static void anc_compander_cleanup(OPERATOR_DATA *op_data)
{
    ANC_COMPANDER_OP_DATA *p_ext_data = get_instance_data(op_data);
    t_compander_object *p_dobject = p_ext_data->compander_object;

    if (p_dobject != NULL)
    {
        /* Free lookahead history buffer */
        pfree(p_dobject->lookahead_hist_buf);
        p_dobject->lookahead_hist_buf = NULL;
        pfree(p_dobject);
        p_dobject = NULL;
    }

    cbuffer_destroy(p_ext_data->internal_buffer);

    anc_compander_release_shared_gain(p_ext_data);

    aud_cur_destroy(op_data);
}

bool anc_compander_create(OPERATOR_DATA *op_data,
                          void *message_data,
                          unsigned *response_id,
                          void **response_data)
{
    ANC_COMPANDER_OP_DATA *p_ext_data = get_instance_data(op_data);
    t_compander_object *compander_object;  /* Pointer to compander obj */
    unsigned *p_default_params;            /* Pointer to default params */
    unsigned *p_cap_params;                /* Pointer to capability params */
    CPS_PARAM_DEF *p_param_def;            /* Pointer to parameter definition */

    L2_DBG_MSG1("ANC Compander Create: p_ext_data at %p", p_ext_data);

    /* Call base_op create, which also allocates and fills response message */
    if (!base_op_create_lite(op_data, response_data))
    {
        return FALSE;
    }

    /* Assume the response to be command FAILED. If we reach the correct
     * termination point in create then change it to STATUS_OK.
     */
    base_op_change_response_status(response_data, STATUS_CMD_FAILED);

    if (!aud_cur_create(op_data,
                        ANC_COMPANDER_CAP_MAX_SOURCES,
                        ANC_COMPANDER_CAP_MAX_SINKS))
    {
        return TRUE;
    }

    aud_cur_set_callbacks(op_data,
                          NULL,
                          NULL,
                          anc_compander_connect_hook,
                          anc_compander_disconnect_hook,
                          anc_compander_param_update_hook);

    aud_cur_set_flags(op_data,
                      ANC_COMPANDER_SUPPORTS_IN_PLACE,
                      ANC_COMPANDER_SUPPORTS_METADATA,
                      ANC_COMPANDER_DYNAMIC_BUFFERS);

    aud_cur_set_min_terminal_masks(op_data,
                                   ANC_COMPANDER_MIN_VALID_SOURCES,
                                   ANC_COMPANDER_MIN_VALID_SINKS);

    aud_cur_set_max_terminal_masks(op_data,
                                   ANC_COMPANDER_MAX_VALID_SOURCES,
                                   ANC_COMPANDER_MAX_VALID_SINKS);


    p_default_params = \
        (unsigned*)ANC_COMPANDER_GetDefaults(ANC_COMPANDER_CAP_ID);
    p_cap_params = (unsigned*) &p_ext_data->compander_cap_params;
    p_param_def = aud_cur_get_cps(op_data);

    if (!cpsInitParameters(p_param_def,
                           p_default_params,
                           p_cap_params,
                           sizeof(ANC_COMPANDER_PARAMETERS)))
    {
        anc_compander_cleanup(op_data);
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* Initialize the compander object.
     * Assuming there will be only one channel for companding */
    compander_object = xzppmalloc(COMPANDER_OBJECT_SIZE , MALLOC_PREFERENCE_DM1);
    if (compander_object == NULL)
    {
        anc_compander_cleanup(op_data);
        L2_DBG_MSG("ANC Compander Create: Failed to allocate compander object");
        return TRUE;
    }

    compander_object->params_ptr = \
        (t_compander_params*)&p_ext_data->compander_cap_params;
    p_ext_data->compander_object = compander_object;

    /* Create internal cBuffer to temporarily hold compander output */
    if (!aud_cur_create_cbuffer(&p_ext_data->internal_buffer,
                                ANC_COMPANDER_INTERNAL_BUFFER_SIZE,
                                MALLOC_PREFERENCE_DM1))
    {
        anc_compander_cleanup(op_data);
        L2_DBG_MSG("ANC Compander Create: failed to allocate internal buffer");
        return TRUE;
    }
    compander_object->channel_output_ptr = (void*)p_ext_data->internal_buffer;

    p_ext_data->host_mode = ANC_COMPANDER_SYSMODE_FULL;
    p_ext_data->cur_mode  = ANC_COMPANDER_SYSMODE_FULL;
    p_ext_data->sample_rate = (unsigned)stream_if_get_system_sampling_rate();

    /* Initialise default compander filter path */
    anc_compander_init_filter_path(p_ext_data);

    base_op_change_response_status(response_data, STATUS_OK);

    L2_DBG_MSG("ANC Compander: Created");

    return TRUE;
}

bool anc_compander_destroy(OPERATOR_DATA *op_data,
                           void *message_data,
                           unsigned *response_id,
                           void **response_data)
{
    /* Call base_op destroy that creates and fills response message */
    if (!base_op_destroy_lite(op_data, response_data))
    {
        return FALSE;
    }

    anc_compander_cleanup(op_data);

    L2_DBG_MSG("ANC Compander Destroyed");

    return TRUE;
}

/****************************************************************************
ANC Compander hook functions
*/
bool anc_compander_connect_hook(OPERATOR_DATA *op_data, unsigned terminal_id)
{
    uint16 terminal_num;
    tCbuffer * p_buffer;

    ANC_COMPANDER_OP_DATA *p_ext_data = get_instance_data(op_data);

    terminal_num = (uint16)(terminal_id & TERMINAL_NUM_MASK);

    if (terminal_num == ANC_COMPANDER_COMPANDING_TERMINAL_NUM)
    {
        if (terminal_id & TERMINAL_SINK_MASK)
        {
            p_buffer = aud_cur_get_sink_terminal(op_data, terminal_num);
            p_ext_data->compander_object->channel_input_ptr = (void*)p_buffer;
        }
    }
    return TRUE;
}

bool anc_compander_disconnect_hook(OPERATOR_DATA *op_data, unsigned terminal_id)
{
    uint16 terminal_num;

    ANC_COMPANDER_OP_DATA *p_ext_data = get_instance_data(op_data);

    terminal_num = (uint16)(terminal_id & TERMINAL_NUM_MASK);

    if (terminal_num == ANC_COMPANDER_COMPANDING_TERMINAL_NUM)
    {
        if (terminal_id & TERMINAL_SINK_MASK)
        {
            p_ext_data->compander_object->channel_input_ptr = NULL;
        }
    }
    return TRUE;
}

bool anc_compander_param_update_hook(OPERATOR_DATA *op_data)
{
    AHM_ANC_FILTER old_filter_path, new_filter_path;
    ANC_COMPANDER_OP_DATA *p_ext_data = get_instance_data(op_data);

    /* Re-link AHM shared gains.
       Ignore if AHM is not linked. This can happen if SET_UCID/SET_PARAMS comes
       before linking AHM. */
    if (p_ext_data->ahm_op_id != 0)
    {
        old_filter_path = p_ext_data->compander_filter_path;
        anc_compander_init_filter_path(p_ext_data);
        new_filter_path = p_ext_data->compander_filter_path;

        /* Re-link if filter path has changed. Else it will
           result in pops in audio due to re-linking */
        if (old_filter_path != new_filter_path)
        {
            anc_compander_release_shared_gain(p_ext_data);
            aud_cur_get_shared_fine_gain((void*)p_ext_data,
                                         new_filter_path,
                                         p_ext_data->ahm_op_id,
                                         AHM_GAIN_CONTROL_TYPE_NOMINAL,
                                         AHM_ANC_INSTANCE_ANC0_ID,
                                         anc_compander_opmsg_link_ahm_callback);
        }
    }

    return TRUE;
}

/****************************************************************************
Operator message handlers
*/
bool anc_compander_opmsg_set_control(OPERATOR_DATA *op_data,
                                     void *message_data,
                                     unsigned *resp_length,
                                     OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    ANC_COMPANDER_OP_DATA *op_extra_data = get_instance_data(op_data);
    unsigned i, num_controls, cntrl_value, cntrl_id;
    CPS_CONTROL_SOURCE cntrl_src;
    OPMSG_RESULT_STATES result = OPMSG_RESULT_STATES_NORMAL_STATE;

    if (!cps_control_setup(message_data, resp_length, resp_data,&num_controls))
    {
       return FALSE;
    }

    for (i = 0; i < num_controls; i++)
    {
        cntrl_id = cps_control_get(message_data, i, &cntrl_value, &cntrl_src);

        /* Only one control message is supported */
        if (cntrl_id != OPMSG_CONTROL_MODE_ID)
        {
            result = OPMSG_RESULT_STATES_UNSUPPORTED_CONTROL;
            break;
        }
        /* Only interested in low 8-bits of value */
        cntrl_value &= 0xFF;
        if (cntrl_value >= ANC_COMPANDER_SYSMODE_MAX_MODES)
        {
            result = OPMSG_RESULT_STATES_INVALID_CONTROL_VALUE;
            break;
        }

        if (cntrl_src == CPS_SOURCE_HOST)
        {
           op_extra_data->host_mode = cntrl_value;
        }
        else
        {
            op_extra_data->ovr_control = (cntrl_src == CPS_SOURCE_OBPM_DISABLE) ? \
                0 : ANC_COMPANDER_CONTROL_MODE_OVERRIDE;
            op_extra_data->qact_mode = cntrl_value;
        }
    }

    if ((op_extra_data->ovr_control & ANC_COMPANDER_CONTROL_MODE_OVERRIDE) > 0)
    {
       op_extra_data->cur_mode  = op_extra_data->qact_mode;
    }
    else
    {
       op_extra_data->cur_mode  = op_extra_data->host_mode;
    }

    cps_response_set_result(resp_data, result);

    /* Set the Reinit flag after setting the paramters */
    if (result == OPMSG_RESULT_STATES_NORMAL_STATE)
    {
        aud_cur_set_reinit(op_data, TRUE);
    }
    return TRUE;
}

bool anc_compander_opmsg_get_status(OPERATOR_DATA *op_data,
                                    void *message_data,
                                    unsigned *resp_length,
                                    OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    ANC_COMPANDER_OP_DATA *p_ext_data = get_instance_data(op_data);
    t_compander_object *compander_obj = p_ext_data->compander_object;
    ParamType *pparam;

    ANC_COMPANDER_STATISTICS stats;
    unsigned *resp;
    unsigned i;

    /* Build the response */
    if (!common_obpm_status_helper(message_data, resp_length,
                                   resp_data,
                                   sizeof(ANC_COMPANDER_STATISTICS),
                                   &resp))
    {
        return FALSE;
    }

    if (resp != NULL)
    {
        stats.OFFSET_CUR_MODE = p_ext_data->cur_mode;
        stats.OFFSET_OVR_CONTROL = p_ext_data->ovr_control;
        stats.OFFSET_OP_STATE = opmgr_op_is_running(op_data) ? 0 : 1;
        stats.OFFSET_LOOKAHEAD_FLAG = p_ext_data->lookahead_status;
        stats.OFFSET_ANC_GAIN = p_ext_data->p_fine_gain->gain;
        stats.OFFSET_BLOCK_SIZE = compander_obj->gain_update_rate;
        stats.OFFSET_LEVEL = compander_obj->level_detect_chn_max_or_mean_log2;

        pparam = (ParamType*)(&stats);
        for (i=0; i<ANC_COMPANDER_N_STAT/2; i++)
        {
            resp = cpsPack2Words(pparam[2*i], pparam[2*i+1], resp);
        }
        if ((ANC_COMPANDER_N_STAT % 2) == 1) /* last one */
        {
            cpsPack1Word(pparam[ANC_COMPANDER_N_STAT-1], resp);
        }
    }

    return TRUE;
}

bool anc_compander_opmsg_set_sample_rate(OPERATOR_DATA *op_data,
                                         void *message_data,
                                         unsigned *resp_length,
                                         OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    ANC_COMPANDER_OP_DATA *p_ext_data = get_instance_data(op_data);
    p_ext_data->sample_rate = SAMPLE_RATE_FROM_COMMON_OPMSG(message_data);
    aud_cur_set_reinit(op_data, TRUE);

    return(TRUE);
}

bool anc_compander_opmsg_link_ahm(OPERATOR_DATA *op_data,
                                  void *message_data,
                                  unsigned *resp_length,
                                  OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    ANC_COMPANDER_OP_DATA *p_ext_data = get_instance_data(op_data);

    unsigned link_data;
    uint16 ahm_op_id;
    bool link;

    link_data = OPMSG_FIELD_GET(message_data,
                                OPMSG_COMMON_MSG_LINK_ANC_HW_MANAGER,
                                LINK);
    ahm_op_id = OPMSG_FIELD_GET(message_data,
                                  OPMSG_COMMON_MSG_LINK_ANC_HW_MANAGER,
                                  AHM_OP_ID);

    p_ext_data->ahm_op_id = ahm_op_id;

    link = (bool)link_data;

    if (link)
    {
        if (p_ext_data->p_fine_gain != NULL)
        {
            L2_DBG_MSG("ADRC: link failed: already linked");
            return FALSE;
        }

        /* It is assumed that UCID will be set before linking AHM gains */
        anc_compander_init_filter_path(p_ext_data);

        aud_cur_get_shared_fine_gain((void*)p_ext_data,
                                     p_ext_data->compander_filter_path,
                                     p_ext_data->ahm_op_id,
                                     AHM_GAIN_CONTROL_TYPE_NOMINAL,
                                     AHM_ANC_INSTANCE_ANC0_ID,
                                     anc_compander_opmsg_link_ahm_callback);
    }
    else
    {
        anc_compander_release_shared_gain(p_ext_data);
    }

    return TRUE;
}

bool anc_compander_opmsg_get_shared_gain_ptr(OPERATOR_DATA *op_data,
                                             void *message_data,
                                             unsigned *resp_length,
                                             OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    ANC_COMPANDER_OP_DATA *p_ext_data = get_instance_data(op_data);

    unsigned *p_resp;
    unsigned is_coarse_data, p_recv_ext_data, msg_id;
    bool is_coarse;

    p_recv_ext_data = OPMSG_FIELD_GET32(message_data,
                                        OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                                        P_EXT_DATA);
    is_coarse_data = OPMSG_FIELD_GET(message_data,
                                     OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                                     COARSE);
    is_coarse = (bool)is_coarse_data;

    /* Only handle fine gains */
    if (is_coarse)
    {
        L2_DBG_MSG("ANC Compander Shared coarse gain not supported");
        return FALSE;
    }

    p_resp = xzpnewn(OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP_WORD_SIZE, unsigned);
    if (p_resp == NULL)
    {
        L0_DBG_MSG("ANC Compander failed to create shared gain response");
        return FALSE;
    }
    *resp_length = OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP_WORD_SIZE;
    msg_id = OPMGR_GET_OPCMD_MESSAGE_MSG_ID((OPMSG_HEADER*)message_data);

    /* Set the message ID */
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                             MESSAGE_ID,
                             msg_id);
    /* Set the extra operator data pointer */
    OPMSG_CREATION_FIELD_SET32(p_resp,
                               OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                               P_EXT_DATA,
                               p_recv_ext_data);
    /* Set shared gain pointer */
    OPMSG_CREATION_FIELD_SET32(p_resp,
                               OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                               SHARED_GAIN_PTR,
                               (unsigned)p_ext_data->p_fine_gain);

    *resp_data = (OP_OPMSG_RSP_PAYLOAD*)p_resp;

    return TRUE;
}

bool anc_compander_opmsg_set_makeup_gain(OPERATOR_DATA *op_data,
                                         void *message_data,
                                         unsigned *resp_length,
                                         OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    ANC_COMPANDER_OP_DATA *p_ext_data = get_instance_data(op_data);
    OPMSG_ADRC_SET_MAKEUP_GAIN *p_msg;

    int tgt_msb, tgt_lsb;

    p_msg = (OPMSG_ADRC_SET_MAKEUP_GAIN*)message_data;

    tgt_msb = OPMSG_FIELD_GET(p_msg,
                              OPMSG_ADRC_SET_MAKEUP_GAIN,
                              MAKEUP_GAIN);
    tgt_lsb = OPMSG_FIELD_GET_FROM_OFFSET(p_msg,
                                          OPMSG_ADRC_SET_MAKEUP_GAIN,
                                          MAKEUP_GAIN,
                                          1);

    p_ext_data->compander_cap_params.OFFSET_MAKEUP_GAIN = (tgt_msb << 16) | tgt_lsb;

    return TRUE;
}

/****************************************************************************
Custom opmsg handlers
*/

bool anc_compander_opmsg_get_adjusted_gain(OPERATOR_DATA *op_data,
                                           void *message_data,
                                           unsigned *resp_length,
                                           OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    ANC_COMPANDER_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned *p_resp;
    uint16 msg_id;
    uint16 static_gain;
    uint16 adjusted_gain;
    int makeup_gain;

    if (p_ext_data->p_static_gain == NULL)
    {
        L2_DBG_MSG("ADRC: get_adjusted_gain - AHM static gain not linked");
        return FALSE;
    }
    static_gain = p_ext_data->p_static_gain->fine;

    *resp_length = OPMSG_ADRC_GET_ADJUSTED_GAIN_RESP_WORD_SIZE;

    p_resp = xzpnewn(OPMSG_ADRC_GET_ADJUSTED_GAIN_RESP_WORD_SIZE, unsigned);
    if (p_resp == NULL)
    {
        L2_DBG_MSG("ADRC: get_adjusted_gain - Failed to allocate response msg");
        return FALSE;
    }

    /* Calculate adjusted gain */
    makeup_gain = p_ext_data->compander_cap_params.OFFSET_MAKEUP_GAIN;
    L2_DBG_MSG1("ADRC: makeup_gain : %d",makeup_gain);
    adjusted_gain = aud_cur_calc_adjusted_gain(static_gain, makeup_gain);

    msg_id = (uint16)OPMGR_GET_OPCMD_MESSAGE_MSG_ID((OPMSG_HEADER*)message_data);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_ADRC_GET_ADJUSTED_GAIN_RESP,
                             MESSAGE_ID,
                             msg_id);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_ADRC_GET_ADJUSTED_GAIN_RESP,
                             ADJUSTED_GAIN,
                             adjusted_gain);

    *resp_data = (OP_OPMSG_RSP_PAYLOAD*)p_resp;

    return TRUE;
}

static bool anc_compander_opmsg_link_aanc_gain_callback(CONNECTION_LINK con_id,
                                                        STATUS_KYMERA status,
                                                        EXT_OP_ID op_id,
                                                        unsigned num_resp_params,
                                                        unsigned *resp_params)
{
    ANC_COMPANDER_OP_DATA *p_ext_data;
    AHM_SHARED_FINE_GAIN *p_aanc_gain;
    unsigned raw_ptr, raw_gain;

    if (status != ACCMD_STATUS_OK)
    {
        L0_DBG_MSG1("ADRC Link AANC gain response failed: status=%d", status);
        return FALSE;
    }
    /* Get p_ext_data pointer */
    raw_ptr = OPMSG_CREATION_FIELD_GET32(resp_params,
                                         OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                         P_EXT_DATA);
    p_ext_data = (ANC_COMPANDER_OP_DATA*)raw_ptr;

    /* Get shared gain pointer */
    raw_gain = OPMSG_CREATION_FIELD_GET32(resp_params,
                                          OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                          SHARED_GAIN_PTR);
    p_aanc_gain = (AHM_SHARED_FINE_GAIN*)raw_gain;
    p_ext_data->p_aanc_gain = p_aanc_gain;

    return TRUE;
}

bool anc_compander_opmsg_link_aanc_gain(OPERATOR_DATA *op_data,
                                        void *message_data,
                                        unsigned *resp_length,
                                        OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    ANC_COMPANDER_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned aanc_op_id;        /* AANC operator ID */

    aanc_op_id = OPMSG_FIELD_GET32(message_data,
                                   OPMSG_ADRC_LINK_AANC_GAIN,
                                   AANC_OP_ID);

    if (aanc_op_id == 0)
    {
        L2_DBG_MSG("ADRC: Link AANC gain - Invalid AANC operator ID");
        return FALSE;
    }

    aud_cur_get_shared_fine_gain((void*)p_ext_data,
                                 0, /* unused */
                                 aanc_op_id,
                                 0, /* unused */
                                 AHM_ANC_INSTANCE_ANC0_ID,
                                 anc_compander_opmsg_link_aanc_gain_callback);

    return TRUE;
}


/****************************************************************************
Data processing functions and wrappers
*/
void anc_compander_process_data(OPERATOR_DATA *op_data,
                                TOUCHED_TERMINALS *touched)
{
    ANC_COMPANDER_OP_DATA *p_ext_data = get_instance_data(op_data);
    t_compander_object * compander_obj = p_ext_data->compander_object;
    AHM_SHARED_FINE_GAIN *p_shared_gain = p_ext_data->p_fine_gain;
    AHM_GAIN * p_static_gain = p_ext_data->p_static_gain;

    tCbuffer * p_input_buf;
    tCbuffer * p_output_buf;
    tCbuffer * p_internal_buf;

    unsigned samples_to_process, samples_processed, samples_pending;
    unsigned terminal_skip_mask;
    unsigned block_size;
    int full_gain, round_gain;
    uint8 anc_hw_gain;
    uint16 nominal_gain;
    bool bypass_compander;

    /* Handle initialization. May change block size */
    if (aud_cur_get_reinit(op_data))
    {
        aud_cur_set_reinit(op_data, FALSE);
        anc_compander_proc_init(p_ext_data);

        block_size = compander_obj->gain_update_rate;
        if (block_size > 1)
        {
            aud_cur_set_block_size(op_data, block_size);
        }

        /* Initialize shared gain object */
        if (p_shared_gain != NULL)
        {
            p_shared_gain->valid = FALSE;
        }
    }

    samples_pending = aud_cur_calc_samples(op_data, touched);
    terminal_skip_mask = 0;
    samples_processed = 0;


    if (samples_pending > 0)
    {
        switch (p_ext_data->cur_mode)
        {
            case ANC_COMPANDER_SYSMODE_FULL:
                while (samples_pending > compander_obj->gain_update_rate)
                {
                    /* Calc samples to process per loop */
                    if (samples_pending > ANC_COMPANDER_WRAPPER_BLOCK_SIZE)
                    {
                        samples_to_process = ANC_COMPANDER_WRAPPER_BLOCK_SIZE;
                    }
                    else
                    {
                        samples_to_process = samples_pending;
                    }

                    compander_obj->samples_to_process = samples_to_process;
                    /* Call Compander data processing function */
                    samples_to_process = anc_compander_processing(compander_obj);

                    p_input_buf = aud_cur_get_sink_terminal(op_data,
                                    ANC_COMPANDER_COMPANDING_TERMINAL_NUM);
                    p_output_buf = aud_cur_get_source_terminal(op_data,
                                    ANC_COMPANDER_COMPANDING_TERMINAL_NUM);
                    p_internal_buf = p_ext_data->internal_buffer;

                    /* Advance compander input terminal buffer pointers
                    * as Comapnder has consumed input data.*/
                    cbuffer_advance_read_ptr(p_input_buf, samples_to_process);

                    /* Advance internal compander buffer write pointer */
                    cbuffer_advance_write_ptr(p_internal_buf, samples_to_process);

                    /* Compander has produced data. Copy to output if terminal is
                    * connected. Else Discard companded data.*/
                    if (p_output_buf == NULL)
                    {
                        /* Discard companded data */
                        cbuffer_advance_read_ptr(p_internal_buf,
                                                samples_to_process);
                    }
                    else
                    {
                        /* Copy companded data to source terminal buffer */
                        cbuffer_copy(p_output_buf,
                                    p_internal_buf,
                                    samples_to_process);
                    }

                    samples_processed += samples_to_process;
                    samples_pending -= samples_to_process;
                }

                /* Update only the final gain computed in AHM shared object. The
                 * intermediate gains computed in each loop are not updated in
                 * AHM shared object as the operator tasks are not interrupted
                 * and only gain computed in the last loop will be applicable */
                bypass_compander = \
                    (p_ext_data->compander_cap_params.OFFSET_COMPANDER_CONFIG &
                     ANC_COMPANDER_CONFIG_BYPASS) > 0;
                if ((p_shared_gain != NULL) && (p_static_gain != NULL))
                {
                    if (!bypass_compander)
                    {
                        /* gain smooth hist linear is in Q5.27. Add headroom to
                         * multiply by fine gain (0-255). Value is Q13.19.
                         */
                        full_gain = compander_obj->gain_smooth_hist_linear >> \
                            ANC_COMPANDER_GAIN_SHIFT;

                        /* Use adaptive gain as nominal ADRC gain if AANC gain
                         * is linked */
                        if (p_ext_data->p_aanc_gain == NULL)
                        {
                            nominal_gain = p_static_gain->fine;
                        }
                        else
                        {
                            nominal_gain = (uint16)p_ext_data->p_aanc_gain->gain;
                        }
                        /* Multiply by fine gain (Q13.19) */
                        full_gain *= nominal_gain;
                        /* Round */
                        round_gain = full_gain + ANC_COMPANDER_GAIN_LSB;
                        /* Test against limit (255 in Q13.19) */
                        if (round_gain > ANC_COMPANDER_MAX_GAIN_Q13)
                        {
                            anc_hw_gain = ANC_COMPANDER_MAX_GAIN;
                        }
                        else if (round_gain < 0)
                        {
                            anc_hw_gain = ANC_COMPANDER_MIN_GAIN;
                        }
                        else {
                            anc_hw_gain = \
                                (uint8)(round_gain >> ANC_COMPANDER_Q_SHIFT);
                        }
                        p_shared_gain->gain = anc_hw_gain;
                        p_shared_gain->valid = TRUE;
                    }
                    else
                    {
                        p_shared_gain->valid = FALSE;
                    }
                }
                else
                {
                    L2_DBG_MSG2("ADRC failed to set gain: invalid gain ptrs \
                                 p_shared_gain = 0x%x, p_static_gain = 0x%x",
                                 p_shared_gain, p_static_gain);
                }

                /* Set terminal_skip_mask as data propagation is handled
                 * manually for compander terminal */
                terminal_skip_mask = ANC_COMPANDER_COMPANDING_TERMINAL_POS;

                break;

            case ANC_COMPANDER_SYSMODE_PASS_THRU:
                /* terminal_skip_mask is initialised to 0.
                 * inputs are copied to outputs in aud_cur_mic_data_transfer()*/

                /* Set ADRC gain validity to false */
                p_shared_gain->valid = FALSE;
                samples_processed = samples_pending;

                break;

            default:
                break;
        }

        /*Transfer data for pass-through mic terminals */
        aud_cur_mic_data_transfer(op_data,
                                  samples_processed,
                                  terminal_skip_mask);
        /* Transfer mic metadata for same number of samples transfered in mic */
        aud_cur_mic_metadata_transfer(op_data, samples_processed);
    }
}
