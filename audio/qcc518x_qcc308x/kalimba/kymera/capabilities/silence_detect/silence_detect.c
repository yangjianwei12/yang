/****************************************************************************
 * Copyright (c) 2014 - 2021 Qualcomm Technologies International, Ltd 
****************************************************************************/
/**
 * \file  silence_detect.c
 * \ingroup  capabilities
 *
 *  An implementation of a Capability that can detect the presence of silence or audio
 *
 */


#include "silence_detect.h"
#include "silence_detect_struct.h"

/****************************************************************************
Private Function Definitions
*/
static void silence_detect_process_data(OPERATOR_DATA*, TOUCHED_TERMINALS*);
extern void silence_detector_lib_initialize(silence_detect_algo_object *algo_obj, unsigned frame_size, unsigned sample_rate);
extern void silence_detector_lib_process(silence_detect_algo_object *algo_obj, unsigned frame_size);

static bool silence_detect_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
bool silence_detect_channel_create(OPERATOR_DATA *op_data, MULTI_CHANNEL_CHANNEL_STRUC *chan_ptr, unsigned chan_idx);
bool silence_detect_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
void silence_detect_channel_destroy(OPERATOR_DATA *op_data, MULTI_CHANNEL_CHANNEL_STRUC *chan_ptr);
void send_event_detection_message(OPERATOR_DATA *op_data, unsigned silence_detection_event);

bool sd_opmsg_set_sample_rate(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
bool sd_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
bool sd_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
bool sd_opmsg_get_ps_id(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
bool sd_opmsg_set_ucid(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
static bool sd_ups_params(void* instance_data, PS_KEY_TYPE key, PERSISTENCE_RANK rank, uint16 length, unsigned* data, STATUS_KYMERA status, uint16 extra_status_info);
bool sd_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
bool sd_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
bool sd_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

/****************************************************************************
Private Constant Declarations
*/
#ifdef CAPABILITY_DOWNLOAD_BUILD
#define SILENCE_DETECT_CAP_ID CAP_ID_DOWNLOAD_SILENCE_DETECT
#else
#define SILENCE_DETECT_CAP_ID CAP_ID_SILENCE_DETECT
#endif /* CAPABILITY_DOWNLOAD_BUILD */

/** The stub capability function handler table */
const handler_lookup_struct silence_detect_handler_table =
{
    silence_detect_create,              /* OPCMD_CREATE */
    silence_detect_destroy,             /* OPCMD_DESTROY */
    multi_channel_start,                /* OPCMD_START */
    multi_channel_stop_reset,                 /* OPCMD_STOP */
    multi_channel_stop_reset,                /* OPCMD_RESET */
    multi_channel_connect,             /* OPCMD_CONNECT */
    multi_channel_disconnect,         /* OPCMD_DISCONNECT */
    multi_channel_buffer_details,    /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,  /* OPCMD_DATA_FORMAT */
    base_op_get_sched_info     /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table - this is the set of operator
 * messages that the capability understands and will attempt to service. */
const opmsg_handler_lookup_table_entry silence_detect_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,             base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_SET_CONTROL,                        sd_opmsg_obpm_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,                         sd_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,                       sd_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,                         sd_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,                         sd_opmsg_obpm_get_status},
    {OPMSG_COMMON_ID_SET_UCID,                           sd_opmsg_set_ucid},
    {OPMSG_COMMON_ID_GET_LOGICAL_PS_ID,                  sd_opmsg_get_ps_id},
    {OPMSG_COMMON_SET_SAMPLE_RATE,                       sd_opmsg_set_sample_rate},
    {OPMSG_COMMON_ID_SET_BUFFER_SIZE,                    multi_channel_opmsg_set_buffer_size},
    {0, NULL}
};


/* Capability data - This is the definition of the capability that Opmgr uses to
 * create the capability from. */
const CAPABILITY_DATA silence_detect_cap_data =
{
    SILENCE_DETECT_CAP_ID,                 /* Capability ID */
    1, 1,                                    /* Version information - hi and lo parts */
    SILENCE_DETECTOR_MAX_CHANNELS, SILENCE_DETECTOR_MAX_CHANNELS,    /* Max number of sinks/inputs and sources/outputs */
    &silence_detect_handler_table, /* Pointer to message handler function table */
    silence_detect_opmsg_handler_table,     /* Pointer to operator message handler function table */
    silence_detect_process_data,              /* Pointer to data processing function */
    0,                                        /* Reserved */
    sizeof(SILENCE_DETECT_OP_DATA)         /* Size of capability-specific per-instance data */
};
MAP_INSTANCE_DATA(SILENCE_DETECT_CAP_ID, SILENCE_DETECT_OP_DATA)

/* Accessing the capability-specific per-instance data function */
static inline SILENCE_DETECT_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (SILENCE_DETECT_OP_DATA *) base_op_get_instance_data(op_data);
}


static bool silence_detect_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{

    SILENCE_DETECT_OP_DATA *p_ext_data = get_instance_data(op_data);
    
    patch_fn_shared(silence_detect_cap);

    L3_DBG_MSG1("Silence detect operator create: p_ext_data at 0x%08X", p_ext_data);

    /* call base_op create, which also allocates and fills response message */
    if (!base_op_create_lite(op_data, response_data))
    {
        L2_DBG_MSG("Silence detect operator create: base_op_create failed");
        return FALSE;
    }

    /* Allocate channels, in-place, allow hot connect  */
    if( !multi_channel_create(op_data, (MULTI_INPLACE_FLAG | MULTI_METADATA_FLAG | MULTI_HOT_CONN_FLAG), sizeof(silence_detect_channel_object)) )
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    multi_channel_set_callbacks(op_data, silence_detect_channel_create, silence_detect_channel_destroy);

    multi_channel_set_buffer_size(op_data, 2*SILENCE_DETECT_DEFAULT_FRAME_SIZE);

    p_ext_data->host_mode = SILENCE_DETECT_SYSMODE_FULL;
    p_ext_data->cur_mode  = SILENCE_DETECT_SYSMODE_FULL;
    p_ext_data->sample_rate = SILENCE_DETECT_SD_SAMPLE_RATE;
    p_ext_data->frame_size = SILENCE_DETECT_DEFAULT_FRAME_SIZE;

    if(!cpsInitParameters(&p_ext_data->parms_def, (unsigned*) SILENCE_DETECT_GetDefaults(SILENCE_DETECT_CAP_ID), (unsigned*) &p_ext_data->silence_detector_cap_params, sizeof(SILENCE_DETECT_PARAMETERS)))
    {
       base_op_change_response_status(response_data, STATUS_CMD_FAILED);
       multi_channel_detroy(op_data);
       return TRUE;
    }


    return TRUE;
}

bool silence_detect_channel_create(OPERATOR_DATA *op_data,MULTI_CHANNEL_CHANNEL_STRUC *chan_ptr,unsigned chan_idx)
{
    SILENCE_DETECT_OP_DATA *p_ext_data = get_instance_data(op_data);
    silence_detect_channel_object *sd_chan  = (silence_detect_channel_object*)chan_ptr;
    silence_detect_algo_object *p_sd_dobj;
    
    patch_fn_shared(silence_detect_cap);

    p_sd_dobj = xzppmalloc(sizeof(silence_detect_algo_object) , MALLOC_PREFERENCE_NONE);
    if (p_sd_dobj == NULL)
    {
        /* failed to allocate */
        return FALSE;
    }
    sd_chan->silence_detect_algo_obj = p_sd_dobj;
    sd_chan->chan_mask = 1<<chan_idx;

    p_sd_dobj->params_ptr = (unsigned*) &p_ext_data->silence_detector_cap_params;
    // *************************************************************************
    // Set the input cbuffer pointer for all data objects
    // *************************************************************************
    p_sd_dobj->input_buffer_ptr  = (void*)sd_chan->common.sink_buffer_ptr;

    p_ext_data->ReInitFlag = 1;

    return TRUE;
}

bool silence_detect_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    /* call base_op destroy that creates and fills response message, too */
    if(!base_op_destroy_lite(op_data, response_data))
    {
        return(FALSE);
    }
    multi_channel_detroy(op_data);

    return TRUE;
}

void silence_detect_channel_destroy(OPERATOR_DATA *op_data, MULTI_CHANNEL_CHANNEL_STRUC *chan_ptr)
{
    patch_fn_shared(silence_detect_cap);
    
    silence_detect_channel_object *sd_chan  = (silence_detect_channel_object*)chan_ptr;
    silence_detect_algo_object *p_sd_dobj;

    p_sd_dobj = sd_chan->silence_detect_algo_obj;
    sd_chan->silence_detect_algo_obj = NULL;

    /* free silence detect algorithm object */
    pfree(p_sd_dobj);

    return;
}

/* Data processing function */
static void silence_detect_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    SILENCE_DETECT_OP_DATA *p_ext_data = get_instance_data(op_data);
    MULTI_CHANNEL_CHANNEL_STRUC *first_chan= multi_channel_first_active_channel(op_data);
    MULTI_CHANNEL_CHANNEL_STRUC *chan = first_chan;
    silence_detect_channel_object *sd_chan;
    unsigned samples_to_process;

    patch_fn(silence_detect_process_data);

    /* Make sure there are channels to process */
    if(first_chan == NULL)
    {
        return;
    }

    /* Handle initialization.*/
    if(p_ext_data->ReInitFlag)
    {
        p_ext_data->frame_size = p_ext_data->sample_rate * SILENCE_DETECT_FRAME_PERIOD_MS / 1000;

        chan = first_chan;
        // Initailize all available channels
        while(chan)
        {
            sd_chan  = (silence_detect_channel_object*) chan;
            silence_detector_lib_initialize(sd_chan->silence_detect_algo_obj, p_ext_data->frame_size, p_ext_data->sample_rate);
            chan = chan->next_active;
        }

        //reset channel iterator
        chan  = first_chan;
        /* Set silence detect block size */
        if(p_ext_data->frame_size > 1)
        {
            multi_channel_set_block_size(op_data, p_ext_data->frame_size);
        }
        p_ext_data->all_channels_silent = 2;    /* 2: UNKNOWN */
        p_ext_data->ReInitFlag = 0;
        p_ext_data->frame_count = 0;
    }

    /* Check status of terminals */
    samples_to_process = multi_channel_check_buffers(op_data,touched);

    if(samples_to_process > 0)    
    {
        if(p_ext_data->cur_mode == SILENCE_DETECT_SYSMODE_FULL)
        {
            // Process data on all available channels
            while(chan)
            {
                p_ext_data->frame_count ++;
                sd_chan  = (silence_detect_channel_object*) chan;
                unsigned previous_flag = sd_chan->silence_detect_algo_obj->silence_detection_event;
                silence_detector_lib_process(sd_chan->silence_detect_algo_obj, p_ext_data->frame_size);
                if( sd_chan->silence_detect_algo_obj->silence_detection_event != previous_flag)
                {
                    send_event_detection_message(op_data, sd_chan->silence_detect_algo_obj->silence_detection_event);
                }
                chan = chan->next_active;
            }
        }

        multi_channel_advance_buffers(first_chan, p_ext_data->frame_size, p_ext_data->frame_size);
        multi_channel_metadata_propagate(op_data, p_ext_data->frame_size);

    }
    return;
}

void send_event_detection_message(OPERATOR_DATA *op_data, unsigned silence_detection_event)
{
    unsigned msg_size = OPMSG_UNSOLICITED_SILENCE_DETECT_EVENT_TRIGGER_WORD_SIZE;
    OPMSG_REPLY_ID message_id = OPMSG_REPLY_ID_SILENCE_DETECT_EVENT_TRIGGER;

    patch_fn_shared(silence_detect_cap);

    unsigned *trigger_message = xzpnewn(msg_size, unsigned);
    if (trigger_message == NULL)
    {
        L2_DBG_MSG("SILENCE DETECT: NO MEMORY FOR TRIGGER MESSAGE");
        fault_diatribe(FAULT_AUDIO_INSUFFICIENT_MEMORY, msg_size);
        return;
    }

    SILENCE_DETECT_OP_DATA *p_ext_data = get_instance_data(op_data);

    if (silence_detection_event == 0 && p_ext_data->all_channels_silent)
    {
        p_ext_data->all_channels_silent = 0;
        OPMSG_CREATION_FIELD_SET(trigger_message, OPMSG_UNSOLICITED_SILENCE_DETECT_EVENT_TRIGGER, DETECTED_EVENT, SD_AUDIO_DETECTED);
        common_send_unsolicited_message(op_data, (unsigned)message_id, msg_size, trigger_message);
        pdelete(trigger_message);
        L0_DBG_MSG1("Frame %d: SILENCE DETECT MSG: ATLEAST ONE CHANNEL IS NO LONGER SILENT", p_ext_data->frame_count);
        return;
    }

    unsigned all_channels_silent = 1;
    MULTI_CHANNEL_CHANNEL_STRUC *chan_iterator = multi_channel_first_active_channel(op_data);
    silence_detect_channel_object *sd_chan;

    while(chan_iterator)
    {
        sd_chan  = (silence_detect_channel_object*) chan_iterator;
        if (sd_chan->silence_detect_algo_obj->silence_detection_event != 1)
        {
            all_channels_silent = 0;
        }
        chan_iterator = chan_iterator->next_active;
    }

    p_ext_data->all_channels_silent =  all_channels_silent;

    if(all_channels_silent == 1)
    {
        OPMSG_CREATION_FIELD_SET(trigger_message, OPMSG_UNSOLICITED_SILENCE_DETECT_EVENT_TRIGGER, DETECTED_EVENT, SD_ALL_CHANNELS_SILENT);
        common_send_unsolicited_message(op_data, (unsigned)message_id, msg_size, trigger_message);
        L0_DBG_MSG1("Frame %d: SILENCE DETECT MSG: ALL CHANNELS ARE SILENT", p_ext_data->frame_count);
    }

    pdelete(trigger_message);
    return;
}

/* ********************* Operator Message Handle functions ****************** */

bool sd_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SILENCE_DETECT_OP_DATA *p_ext_data = get_instance_data(op_data);

    unsigned            i, num_controls, cntrl_value;
    CPS_CONTROL_SOURCE  cntrl_src;
    OPMSG_RESULT_STATES result = OPMSG_RESULT_STATES_NORMAL_STATE;
    
    patch_fn_shared(silence_detect_cap);

    if(!cps_control_setup(message_data, resp_length, resp_data, &num_controls))
    {
       return FALSE;
    }

    for(i=0; i<num_controls; i++)
    {
        unsigned cntrl_id = cps_control_get(message_data, i, &cntrl_value,
                                            &cntrl_src);

        if (cntrl_id != OPMSG_CONTROL_MODE_ID)
        {
            result = OPMSG_RESULT_STATES_UNSUPPORTED_CONTROL;
            break;
        }
        /* Only interested in lower 8-bits of value */
        cntrl_value &= 0xFF;
        if (cntrl_value >= SILENCE_DETECT_SYSMODE_MAX_MODES)
        {
            result = OPMSG_RESULT_STATES_INVALID_CONTROL_VALUE;
            break;
        }
        /* Control is Mode */
        if (cntrl_src == CPS_SOURCE_HOST)
        {
            p_ext_data->host_mode = cntrl_value;
        }
        else
        {
            if (cntrl_src == CPS_SOURCE_OBPM_DISABLE)
            {
                p_ext_data->ovr_control = 0;
            }
            else
            {
                p_ext_data->ovr_control = SILENCE_DETECT_CONTROL_MODE_OVERRIDE;
            }
            p_ext_data->obpm_mode = cntrl_value;
        }
    }

    if(p_ext_data->ovr_control & SILENCE_DETECT_CONTROL_MODE_OVERRIDE)
    {
        p_ext_data->cur_mode = p_ext_data->obpm_mode;
    }
    else
    {
        p_ext_data->cur_mode = p_ext_data->host_mode;
    }

    cps_response_set_result(resp_data, result);

    /* Set the Reinit flag after setting the parameters */
    if (result == OPMSG_RESULT_STATES_NORMAL_STATE)
    {
        p_ext_data->ReInitFlag = 1;
    }
    return TRUE;
}

bool sd_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SILENCE_DETECT_OP_DATA *p_ext_data = get_instance_data(op_data);

    return cpsGetParameterMsgHandler(&p_ext_data->parms_def, message_data, resp_length, resp_data);
}

bool sd_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SILENCE_DETECT_OP_DATA *p_ext_data = get_instance_data(op_data);

    return cpsGetDefaultsMsgHandler(&p_ext_data->parms_def, message_data, resp_length, resp_data);
}

bool sd_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SILENCE_DETECT_OP_DATA *p_ext_data = get_instance_data(op_data);

    bool retval = cpsSetParameterMsgHandler(&p_ext_data->parms_def, message_data, resp_length, resp_data);

    p_ext_data->ReInitFlag = TRUE;
    return retval;
}

static bool sd_ups_params(void* instance_data, PS_KEY_TYPE key, PERSISTENCE_RANK rank, uint16 length, unsigned* data, STATUS_KYMERA status, uint16 extra_status_info)
{
    SILENCE_DETECT_OP_DATA *p_ext_data = get_instance_data(instance_data);

    cpsSetParameterFromPsStore(&p_ext_data->parms_def, length, data, status);

    return TRUE;
}

bool sd_opmsg_set_ucid(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SILENCE_DETECT_OP_DATA *p_ext_data = get_instance_data(op_data);
    PS_KEY_TYPE key;

    bool retval = cpsSetUcidMsgHandler(&p_ext_data->parms_def, message_data, resp_length, resp_data);

    key = MAP_CAPID_UCID_SBID_TO_PSKEYID(base_op_get_cap_id(op_data), p_ext_data->parms_def.ucid, OPMSG_P_STORE_PARAMETER_SUB_ID);
    ps_entry_read((void*)op_data, key, PERSIST_ANY, sd_ups_params);

    return retval;
}

bool sd_opmsg_get_ps_id(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SILENCE_DETECT_OP_DATA *p_ext_data = get_instance_data(op_data);

    return cpsGetUcidMsgHandler(&p_ext_data->parms_def, base_op_get_cap_id(op_data),
                                message_data, resp_length, resp_data);
}

bool sd_opmsg_set_sample_rate(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SILENCE_DETECT_OP_DATA *p_ext_data = get_instance_data(op_data);
    
    p_ext_data->sample_rate = SAMPLE_RATE_FROM_COMMON_OPMSG(message_data);
    p_ext_data->ReInitFlag = 1;

    return TRUE;
}

bool sd_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SILENCE_DETECT_OP_DATA *p_ext_data = get_instance_data(op_data);
    MULTI_CHANNEL_CHANNEL_STRUC *chan= multi_channel_first_active_channel(op_data);
    silence_detect_channel_object *sd_chan;
    unsigned *resp;
    int i;

    patch_fn_shared(silence_detect_cap);

    if (!common_obpm_status_helper(message_data, resp_length,resp_data, sizeof(SILENCE_DETECT_STATISTICS), &resp))
    {
        return FALSE;
    }

    if (resp)
    {

        resp = cpsPack2Words(p_ext_data->cur_mode, p_ext_data->all_channels_silent, resp);

        for (i=1;i<=SILENCE_DETECTOR_MAX_CHANNELS;i++)
        {
            if (chan == NULL)
            {
                resp = cpsPack2Words(0, 0, resp);
                continue;
            }
            sd_chan = (silence_detect_channel_object*) chan;
            resp = cpsPack2Words(sd_chan->silence_detect_algo_obj->computed_power_log2, sd_chan->silence_detect_algo_obj->silence_detection_event, resp);
            chan = chan->next_active;
        }

        resp = cpsPack1Word(p_ext_data->ovr_control, resp);
    }

    return TRUE;
}
