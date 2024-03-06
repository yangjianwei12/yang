/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  svad.c
 * \ingroup  capabilities
 *  svad operator
 */

#include "svad_capability.h"
#include "mem_utils/scratch_memory.h"
#include "mem_utils/shared_memory_ids.h"
#include "capabilities.h"
#include "math/fft_twiddle_alloc_c_stubs.h"

#include "ttp/ttp.h"
#include "ttp_utilities.h"

/****************************************************************************
Private Type Definitions
*/

#ifdef CAPABILITY_DOWNLOAD_BUILD
#define SVAD_CAP_ID                 CAP_ID_DOWNLOAD_SVAD
#else
#define SVAD_CAP_ID                 CAP_ID_SVAD
#endif

/****************************************************************************
Private Constant Declarations
*/

/** The SVAD capability function handler table */
const handler_lookup_struct svad_handler_table =
{
    svad_create,              /* OPCMD_CREATE */
    svad_op_destroy,          /* OPCMD_DESTROY */
    svad_start,               /* OPCMD_START */
    base_op_stop,             /* OPCMD_STOP */
    base_op_reset,            /* OPCMD_RESET */
    svad_connect,             /* OPCMD_CONNECT */
    svad_disconnect,          /* OPCMD_DISCONNECT */
    svad_buffer_details,      /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,  /* OPCMD_DATA_FORMAT */
    base_op_get_sched_info    /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table - this is the set of operator
 * messages that the capability understands and will attempt to service. */
const opmsg_handler_lookup_table_entry svad_opmsg_handler_table[] =
    {
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,             base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_GET_PARAMS,                         svad_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,                       svad_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,                         svad_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,                         svad_opmsg_obpm_get_status},
    {OPMSG_COMMON_ID_SET_UCID,                           svad_opmsg_set_ucid},
    {OPMSG_COMMON_ID_GET_LOGICAL_PS_ID,                  svad_opmsg_get_ps_id},
    {OPMSG_VAD_ID_MODE_CHANGE,                           svad_configure_mode_change}, /* RW decide what to support here for downloadable VAD */
    {OPMSG_COMMON_ID_SET_BUFFER_SIZE,                    svad_set_buffer_size},
    {0, NULL}};


/* Capability data - This is the definition of the capability that Opmgr uses to
 * create the capability from. */
const CAPABILITY_DATA svad_cap_data =
{
    SVAD_CAP_ID,                 /* Capability ID */
    SVAD_SVAD_VERSION_MAJOR, 1,  /* Version information - hi and lo parts */
    2, 1,                        /* Max number of sinks/inputs and sources/outputs */
    &svad_handler_table,         /* Pointer to message handler function table */
    svad_opmsg_handler_table,    /* Pointer to operator message handler function table */
    svad_process_data,           /* Pointer to data processing function */
    0,
    sizeof(SVAD_OP_DATA),        /* Size of capability-specific per-instance data */
};


MAP_INSTANCE_DATA(SVAD_CAP_ID, SVAD_OP_DATA)

static inline SVAD_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (SVAD_OP_DATA *) base_op_get_instance_data(op_data);
}

bool svad_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{

    SVAD_OP_DATA *p_ext_data = get_instance_data(op_data);

    L2_DBG_MSG1("SVAD Create: p_ext_data at 0x%08X", p_ext_data);

    patch_fn_shared(svad_capability);
    
    /* call base_op create, which also allocates and fills response message */
    if (!base_op_create(op_data, message_data, response_id, response_data))
    {
        L2_DBG_MSG("SVAD Create: base_op_create failed");
        return FALSE;
    }

    /* allocate memory */
    if (svad_cap_create(p_ext_data) == FALSE)
    {
        L2_DBG_MSG("SVAD Create: memory allocation failed");
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        svad_destroy(p_ext_data);
        return TRUE;
    }

    /* initialize extended data for operator.  Assume intialized to zero*/
    p_ext_data->ReInitFlag = 1;

    /* initialize to full processing mode (which detects self-speech) */
    p_ext_data->config = FULL_PROC;
    
    if(!cpsInitParameters(&p_ext_data->params_def,SVAD_GetDefaults(SVAD_CAP_ID),(unsigned*)
     p_ext_data->svad_algs_container->svad_object_ptr->params_ptr,sizeof(SVAD_PARAMETERS)))
    {
        L2_DBG_MSG("SVAD Create: cpsInitParameters failed");
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        svad_destroy(p_ext_data);
        return TRUE;
    }

    /* everything created successfully! */
    if (load_svad_handle(&p_ext_data->f_handle) == FALSE)
    {
        L2_DBG_MSG("SVAD Create: load_svad_handle failed");
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        svad_destroy(p_ext_data);
        return TRUE;
    }
    
    L2_DBG_MSG("SVAD Create: create successful");
    return TRUE;
}

bool svad_cap_create(SVAD_OP_DATA* p_ext_data)
{                      
         
    patch_fn_shared(svad_capability);
	
	/* allocate the SVAD_algs_cpontainer object */
    p_ext_data->svad_algs_container = xpnew(SVAD_ALGS_CONTAINER);
    if (p_ext_data->svad_algs_container == NULL)
    {
        return FALSE;
    }
   
    /* allocate the SVAD data object */
    p_ext_data->svad_algs_container->svad_object_ptr = xpnew(t_SVAD_object);
    if (p_ext_data->svad_algs_container->svad_object_ptr == NULL)
    {
        return FALSE;
    }

   /* allocate the OBPM parameter "Curparams" block */     
    p_ext_data->svad_algs_container->svad_object_ptr->params_ptr = xpnew(SVAD_PARAMETERS);
    if (p_ext_data->svad_algs_container->svad_object_ptr->params_ptr == NULL)
    {
        return FALSE;
    }

    p_ext_data->svad_algs_container->svad_object_ptr->ipd_short_avg_ptr = xpnewn(SVAD_IPD_MAX_BINS, int);
    if (p_ext_data->svad_algs_container->svad_object_ptr->ipd_short_avg_ptr == NULL)
    {
        return FALSE;
    }

    p_ext_data->svad_algs_container->svad_object_ptr->ipd_short_avg_other_ptr = xpnewn(SVAD_IPD_MAX_BINS, int);
    if (p_ext_data->svad_algs_container->svad_object_ptr->ipd_short_avg_other_ptr == NULL)
    {
        return FALSE;
    }

    p_ext_data->svad_algs_container->svad_object_ptr->ptr_variant_field = xpnew(unsigned int);
    if (p_ext_data->svad_algs_container->svad_object_ptr->ptr_variant_field == NULL)
    {
        return FALSE;
    }

    *(p_ext_data->svad_algs_container->svad_object_ptr->ptr_variant_field) = SVAD_CVC_VARIANT_WB;

    /* Set default buffer size required by SVAD capability*/
    p_ext_data->buf_size = SVAD_DEFAULT_BUFFER_SIZE;

    /* allocate analysis filter bank object for ext mic */
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr = xpnew(t_analysis_filter_bank_object);
    if (p_ext_data->svad_algs_container->afb_ext_mic_object_ptr == NULL)
    {
        return FALSE;
    }

    /* hfp_enable is the only field that needs to be explicitly set to zero */
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->hfp_enable = 0;

    /* allocate analysis filter bank object for int mic */
    p_ext_data->svad_algs_container->afb_int_mic_object_ptr = xpnew(t_analysis_filter_bank_object);
    if (p_ext_data->svad_algs_container->afb_int_mic_object_ptr == NULL)
    {
        return FALSE;
    }

    /* hfp_enable is the only field that needs to be explicitly set to zero */
    p_ext_data->svad_algs_container->afb_int_mic_object_ptr->hfp_enable = 0;

    /* allocate ext mic filter bank configuration object */
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->config_ptr = xpnew(t_filter_bank_config_object);
    if (p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->config_ptr == NULL)
    {
        return FALSE;
    }

    /* int mic and ext mic use the same configuration data */
    p_ext_data->svad_algs_container->afb_int_mic_object_ptr->config_ptr = p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->config_ptr;

    /* populate filter bank config */
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->config_ptr->frame_size = filter_bank_parameters_frame120;
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->config_ptr->window_size = filter_bank_parameters_proto240;
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->config_ptr->zero_padded_window_size = filter_bank_parameters_proto256;
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->config_ptr->scale = filter_bank_parameters_fft256_scale;
    /* p_ext_data->afb_ext_mic_object_ptr->config_ptr->window_ptr is set in
      $_svad_init function */

    /* allocate memory for int mic input_stream_object_ptr */
    p_ext_data->svad_algs_container->afb_int_mic_object_ptr->input_stream_object_ptr = xpnew(cVcStream);
    if (p_ext_data->svad_algs_container->afb_int_mic_object_ptr->input_stream_object_ptr == NULL)
    {
        return FALSE;
    }

    /* p_ext_data->afb_int_mic_object_ptr->input_stream_object_ptr fields get
     set in $_svad_process */

    /* allocate memory for ext mic input_stream_object_ptr */
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->input_stream_object_ptr = xpnew(cVcStream);
    if (p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->input_stream_object_ptr == NULL)
    {
        return FALSE;
    }

    /* p_ext_data->afb_ext_mic_object_ptr->input_stream_object_ptr fields get
     set in $_svad_process */

    /* allocate ext mic history object */
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->history_ptr = xpnewn(filter_bank_parameters_proto240, int);
    if (p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->history_ptr == NULL)
    {
        return FALSE;
    }

    /* allocate int mic history  object */
    p_ext_data->svad_algs_container->afb_int_mic_object_ptr->history_ptr = xpnewn(filter_bank_parameters_proto240, int);
    if (p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->history_ptr == NULL)
    {
        return FALSE;
    }

    /* allocate ext mic fft data object */
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->fft_object_ptr = xpnew(t_fft_object);
    if (p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->fft_object_ptr == NULL)
    {
        return FALSE;
    }

    /* q_dat_in, q_dat_out, fft_extra_scale, and ifft_extra_scale need to be explicitly set to zero */
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->fft_object_ptr->q_dat_in = 0;
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->fft_object_ptr->q_dat_out = 0;
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->fft_object_ptr->fft_extra_scale = 0;
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->fft_object_ptr->ifft_extra_scale = 0;
    


    /* ext mic and int mic data objects use the same fft data object */
    p_ext_data->svad_algs_container->afb_int_mic_object_ptr->fft_object_ptr = p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->fft_object_ptr;

    /* real_scratch_ptr, imag_scratch_ptr, and fft_scratch_ptr are set in
     svad_process_data function */

    /* split_cos_table_ptr is set in $_svad_init function */

    /* allocate ext mic filter bank channel object */
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->freq_output_object_ptr = xpnew(t_filter_bank_channel_object);
    if (p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->freq_output_object_ptr == NULL)
    {
        return FALSE;
    }

    /* SVAD gets input from afb's freq_output_object_ptr */
    p_ext_data->svad_algs_container->svad_object_ptr->ext_mic_buffer_ptr = p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->freq_output_object_ptr;

    /* allocate int mic filter bank channel object */
    p_ext_data->svad_algs_container->afb_int_mic_object_ptr->freq_output_object_ptr = xpnew(t_filter_bank_channel_object);
    if (p_ext_data->svad_algs_container->afb_int_mic_object_ptr->freq_output_object_ptr == NULL)
    {
        return FALSE;
    }

    /* SVAD gets input from afb's freq_output_object_ptr */
    p_ext_data->svad_algs_container->svad_object_ptr->int_mic_buffer_ptr = p_ext_data->svad_algs_container->afb_int_mic_object_ptr->freq_output_object_ptr;

    /* allocate the ext mic filter bank real buffer */
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->freq_output_object_ptr->real_ptr = xpnewn(filter_bank_parameters_fft256_num_bin, int);
    if (p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->freq_output_object_ptr->real_ptr == NULL)
    {
        return FALSE;
    }

    /* allocate the int mic filter bank real buffer */
    p_ext_data->svad_algs_container->afb_int_mic_object_ptr->freq_output_object_ptr->real_ptr = xpnewn(filter_bank_parameters_fft256_num_bin, int);
    if (p_ext_data->svad_algs_container->afb_int_mic_object_ptr->freq_output_object_ptr->real_ptr == NULL)
    {
        return FALSE;
    }

    /* allocate the ext mic filter bank imag buffer */
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->freq_output_object_ptr->imag_ptr = xpnewn(filter_bank_parameters_fft256_num_bin, int);
    if (p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->freq_output_object_ptr->imag_ptr == NULL)
    {
        return FALSE;
    }

    /* allocate the int mic filter bank imag buffer */
    p_ext_data->svad_algs_container->afb_int_mic_object_ptr->freq_output_object_ptr->imag_ptr = xpnewn(filter_bank_parameters_fft256_num_bin, int);
    if (p_ext_data->svad_algs_container->afb_int_mic_object_ptr->freq_output_object_ptr->imag_ptr == NULL)
    {
        return FALSE;
    }

    /* allocate memory for ext mic exponent */
    p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->freq_output_object_ptr->exp_ptr = xpnew(signed int);
    if (p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->freq_output_object_ptr->exp_ptr == NULL)
    {
        return FALSE;
    }

    /* allocate memory for int mic exponent */
    p_ext_data->svad_algs_container->afb_int_mic_object_ptr->freq_output_object_ptr->exp_ptr = xpnew(signed int);
    if (p_ext_data->svad_algs_container->afb_int_mic_object_ptr->freq_output_object_ptr->exp_ptr == NULL)
    {
        return FALSE;
    }

    /* allocate scratch */
    if (scratch_register())
    {
        
        if (scratch_reserve((filter_bank_parameters_fft256_num_bin)*sizeof(int), MALLOC_PREFERENCE_DM1))
        {
            p_ext_data->scratch_registered = TRUE;
            
            if (scratch_reserve(filter_bank_parameters_fft256_num_bin*sizeof(int), MALLOC_PREFERENCE_DM2))
            {
                if(scratch_reserve(filter_bank_parameters_fft256_num_bin*sizeof(int), MALLOC_PREFERENCE_DM2))
                {
                    if (math_fft_twiddle_alloc(SVAD_FFT_SIZE))
                    {
                        p_ext_data->twiddle_registered = TRUE;
                        /* Successfully allocated everything! */
                        return TRUE;
                    }

                    /* alloc_fft_twiddle_factors failed */
                    return FALSE;
                }
            }
        }
        /* Failed reserving scratch */
        L2_DBG_MSG("svad_cap_create: scratch_reserve failed");
        return FALSE;
    }
    else
    {
        /* scratch_register failed */
        L2_DBG_MSG("svad_cap_create: scratch_register failed");
        return FALSE;
    }

}

void svad_destroy(SVAD_OP_DATA* p_ext_data)
{
    patch_fn_shared(svad_capability);

    if (p_ext_data->scratch_registered)
    {
        scratch_deregister();
        p_ext_data->scratch_registered = FALSE;

        if (p_ext_data->twiddle_registered)
        {
            math_fft_twiddle_release(SVAD_FFT_SIZE);
            p_ext_data->twiddle_registered = FALSE;
        }
    }

    if (p_ext_data->svad_algs_container->svad_object_ptr != NULL)
    {
        /* free the SVAD int mic object->real_ptr */
        pfree(p_ext_data->svad_algs_container->svad_object_ptr->int_mic_buffer_ptr->real_ptr);

        /* free the SVAD int mic object->imag_ptr */
        pfree(p_ext_data->svad_algs_container->svad_object_ptr->int_mic_buffer_ptr->imag_ptr);

        /* free the SVAD int mic object->exp_ptr */
        pfree(p_ext_data->svad_algs_container->svad_object_ptr->int_mic_buffer_ptr->exp_ptr);

        /* free the SVAD int mic object */
        pfree(p_ext_data->svad_algs_container->svad_object_ptr->int_mic_buffer_ptr);

        /* free the SVAD ext mic object->real_ptr */
        pfree(p_ext_data->svad_algs_container->svad_object_ptr->ext_mic_buffer_ptr->real_ptr);

        /* free the SVAD ext mic object->imag_ptr */
        pfree(p_ext_data->svad_algs_container->svad_object_ptr->ext_mic_buffer_ptr->imag_ptr);

        /* free the SVAD ext mic object->exp_ptr */
        pfree(p_ext_data->svad_algs_container->svad_object_ptr->ext_mic_buffer_ptr->exp_ptr);

        /* free the SVAD ext mic object */
        pfree(p_ext_data->svad_algs_container->svad_object_ptr->ext_mic_buffer_ptr);

        /* free the SVAD params object */    
        pfree(p_ext_data->svad_algs_container->svad_object_ptr->params_ptr);

        /* free the SVAD variant field */    
        pfree(p_ext_data->svad_algs_container->svad_object_ptr->ptr_variant_field);

        /* free the SVAD short avg ptr */
        pfree(p_ext_data->svad_algs_container->svad_object_ptr->ipd_short_avg_ptr);

        /* free the SVAD short avg other ptr */
        pfree(p_ext_data->svad_algs_container->svad_object_ptr->ipd_short_avg_other_ptr);

        /* free the SVAD data object */
        pfree(p_ext_data->svad_algs_container->svad_object_ptr);

        /* set the parent object to NULL to avoid needlessly freeing all the child objects */
        p_ext_data->svad_algs_container->svad_object_ptr = NULL;
    }

    if (p_ext_data->svad_algs_container->afb_ext_mic_object_ptr != NULL)
    {
        /* free AFB ext mic config ptr */
        pfree(p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->config_ptr);

        /* free AFB ext mic input stream obj */    
        pfree(p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->input_stream_object_ptr);

        /* free AFB ext mic history ptr */    
        pfree(p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->history_ptr);
    
        /* free AFB ext mic fft */ 
        pfree(p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->fft_object_ptr);

        /* free AFB ext mic object */    
        pfree(p_ext_data->svad_algs_container->afb_ext_mic_object_ptr);

        /* set the parent object to NULL to avoid needlessly freeing all the child objects */
        p_ext_data->svad_algs_container->afb_ext_mic_object_ptr = NULL;
    }      

    if (p_ext_data->svad_algs_container->afb_int_mic_object_ptr != NULL)
    {
        /* free AFB int mic history ptr */    
        pfree(p_ext_data->svad_algs_container->afb_int_mic_object_ptr->history_ptr);

        /* free AFB int mic input stream obj */ 
        pfree(p_ext_data->svad_algs_container->afb_int_mic_object_ptr->input_stream_object_ptr);

        /* free AFB int mic object */    
        pfree(p_ext_data->svad_algs_container->afb_int_mic_object_ptr);

        /* set the parent object to NULL to avoid needlessly freeing all the child objects */
        p_ext_data->svad_algs_container->afb_int_mic_object_ptr = NULL;
    }
	
	if (p_ext_data->svad_algs_container!= NULL)
    {
        /* free svad_algs_container object */    
        pfree(p_ext_data->svad_algs_container);

        /* set the parent object to NULL to avoid needlessly freeing all the child objects */
        p_ext_data->svad_algs_container = NULL;
    }

    unload_svad_handle(p_ext_data->f_handle);
    p_ext_data->f_handle = NULL;

    L2_DBG_MSG("svad_destroy: success");

}

bool svad_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SVAD_OP_DATA* p_ext_data = get_instance_data(op_data);

    patch_fn_shared(svad_capability);

    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        L2_DBG_MSG("svad_start: base_op_build_std_response_ex failed");
        return FALSE;
    }

    if (opmgr_op_is_running(op_data))
    {
        /* Operator already started nothing to do. */
        L2_DBG_MSG("svad_start: already running");
        return TRUE;
    }

    /* Check that all inputs are connected */
    if ( (p_ext_data->ext_mic_buffer == NULL) || (p_ext_data->int_mic_buffer == NULL) )
    {
        L2_DBG_MSG("svad_start: all inputs not connected, therefore failed");
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    return TRUE;
}

bool svad_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{

    unsigned terminal_num, terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    terminal_num = terminal_id & TERMINAL_NUM_MASK;
    SVAD_OP_DATA* p_ext_data = get_instance_data(op_data);
   
    patch_fn_shared(svad_capability);

    if (!base_op_buffer_details(op_data, message_data, response_id, response_data))
    {
        L2_DBG_MSG("svad_buffer_details: base_op_buffer_details failed");
        return FALSE;
    }

    /* Set default buffer size  */
    ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = SVAD_DEFAULT_BUFFER_SIZE;

    /* Only support metadata on the channel that gets copied to the output */
    if (terminal_id & TERMINAL_SINK_MASK)
    {
        /* internal mic */
        if (terminal_num == INTERNAL_MIC_TERMINAL_ID) 
        {
            ((OP_BUF_DETAILS_RSP*)*response_data)->supports_metadata = FALSE;
        }
        else if (terminal_num == EXTERNAL_MIC_TERMINAL_ID)
        {
            /* external mic */
            ((OP_BUF_DETAILS_RSP*)*response_data)->supports_metadata = TRUE;
            
            /* Run in place*/
            ((OP_BUF_DETAILS_RSP*)*response_data)->runs_in_place = TRUE;

            /*input terminal. give the output buffer for the channel */
            ((OP_BUF_DETAILS_RSP*)*response_data)->b.in_place_buff_params.buffer = p_ext_data->output_buffer;

        }
        else
        {
            L2_DBG_MSG("svad_buffer_details: invalid output terminal, only 0 & 1 are supported");
        }
    }
    else
    {
        /* output */
        if (terminal_num == OUTPUT_TERMINAL_ID)
        {
            ((OP_BUF_DETAILS_RSP*)*response_data)->supports_metadata = TRUE;
 
            /* Run in place*/
            ((OP_BUF_DETAILS_RSP*)*response_data)->runs_in_place = TRUE;

            /*input terminal. give the output buffer for the channel */
            ((OP_BUF_DETAILS_RSP*)*response_data)->b.in_place_buff_params.buffer = p_ext_data->ext_mic_buffer;

        }
        else
        {
            L2_DBG_MSG("svad_buffer_details: invalid output terminal, only 0 is supported");
        } 

    }

    ((OP_BUF_DETAILS_RSP*)*response_data)->metadata_buffer = NULL;
    L2_DBG_MSG("svad_buffer_details: successful");
    return TRUE;
}

bool svad_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SVAD_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned terminal_num, terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    terminal_num = terminal_id & TERMINAL_NUM_MASK;
    tCbuffer* pterminal_buf = OPMGR_GET_OP_CONNECT_BUFFER(message_data);
    
    patch_fn_shared(svad_capability);
        
    if (!base_op_connect(op_data, message_data, response_id, response_data))
    {
        L2_DBG_MSG("svad_connect: base_op_connect failed");
        return FALSE;
    }

    if (opmgr_op_is_running(op_data))
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        L2_DBG_MSG("svad_connect: op is running, failed");
        return TRUE;
    }

    /* Check if input terminal */
    if(terminal_id & TERMINAL_SINK_MASK)
    {
        /* connect the terminal */
		if (terminal_num == EXTERNAL_MIC_TERMINAL_ID)
		{
		    p_ext_data->ext_mic_buffer = pterminal_buf;
		}
		
		else if (terminal_num == INTERNAL_MIC_TERMINAL_ID)
		{
		    p_ext_data->int_mic_buffer = pterminal_buf;
		}

        else
        {
            /* invalid terminal id */
            L2_DBG_MSG("svad_connect: invalid input terminal, only 0 & 1 are supported");
            base_op_change_response_status(response_data, STATUS_INVALID_CMD_PARAMS);
            return TRUE;   
        }		
    }
    else
	/* must be output terminal */
    {
		if (terminal_num == OUTPUT_TERMINAL_ID)
		{
		    p_ext_data->output_buffer = pterminal_buf;
		}
        else
        {
            /* invalid terminal id */
            L2_DBG_MSG("svad_connect: invalid output terminal, only 0 is supported");
            base_op_change_response_status(response_data, STATUS_INVALID_CMD_PARAMS);
            return TRUE;
        }
    }

    return TRUE;
}

bool svad_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SVAD_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned terminal_num, terminal_id = OPMGR_GET_OP_DISCONNECT_TERMINAL_ID(message_data);
    terminal_num = terminal_id & TERMINAL_NUM_MASK;

    patch_fn_shared(svad_capability);

    if (!base_op_disconnect(op_data, message_data, response_id, response_data))
    {
        L2_DBG_MSG("svad_disconnect: base_op_disconnect failed");
        return FALSE;
    }

    /* can't disconnect while running */
    if (opmgr_op_is_running(op_data))
    {
        L2_DBG_MSG("svad_disconnect: operator is running, failed");
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* Check if input terminal */
    if(terminal_id & TERMINAL_SINK_MASK)
    {              
       if (terminal_num == EXTERNAL_MIC_TERMINAL_ID)
       {
           p_ext_data->ext_mic_buffer = NULL;
       }

	   else if (terminal_num == INTERNAL_MIC_TERMINAL_ID)
	   {
           p_ext_data->int_mic_buffer = NULL;
	   }

       else
       {
           /* invalid terminal id */
           L2_DBG_MSG("svad_disconnect: invalid input terminal ID, only 0 & 1 supported");
           base_op_change_response_status(response_data, STATUS_INVALID_CMD_PARAMS);
           return TRUE;
       }
    }
    /* Must be an output terminal */
    else
    {
       if (terminal_num == OUTPUT_TERMINAL_ID)
       {
           p_ext_data->output_buffer = NULL;
       }
       else
       {
           /* invalid terminal id */
           L2_DBG_MSG("svad_disconnect: invalid output terminal ID, only 0 is supported");
           base_op_change_response_status(response_data, STATUS_INVALID_CMD_PARAMS);
           return TRUE;
       }
    }

    return TRUE;
}

bool svad_op_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SVAD_OP_DATA *p_ext_data = get_instance_data(op_data);

    patch_fn_shared(svad_capability);

    /* check that we are not trying to destroy a running operator */
    if (opmgr_op_is_running(op_data))
    {
        /* We can't destroy a running operator. */
        L2_DBG_MSG("svad_op_destroy: operator is still running, failed");
        return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
    }
    else
    {
        /* set internal capability state variable to "not_created" */
        svad_destroy(p_ext_data);

        /* call base_op destroy that creates and fills response message, too */
        return base_op_destroy(op_data, message_data, response_id, response_data);
    }
}

/* Check to see if self-speech has been detected */
static bool speech_detected(SVAD_OP_DATA *p_ext_data)
{
    bool speech_changed = FALSE;

    if (p_ext_data->config == FULL_PROC)
    {
        if (p_ext_data->speech_detected != p_ext_data->svad_algs_container->svad_object_ptr->immediate_vadh_field)
        {
            speech_changed = TRUE;
        }
        p_ext_data->speech_detected = p_ext_data->svad_algs_container->svad_object_ptr->immediate_vadh_field;
    }

    return speech_changed;
}

/*
 * Checks if we need to send any notifications to the client.
 */
static bool check_notification(SVAD_OP_DATA *p_ext_data, bool speech_changed,
                               OPMSG_OP_CLIENT_REPLY_ID * p_notification)
{
    bool notify = FALSE;

    /* set notification variable as if self-speech has been detected */
    OPMSG_OP_CLIENT_REPLY_ID notification = OPMSG_REPLY_ID_VA_TRIGGER;

    if (p_ext_data->config == FULL_PROC)
    {
        if (speech_changed)
        {
            notify = TRUE;
            if (p_ext_data->speech_detected == 0)
            {
                /* Self speech not detected */
                notification = OPMSG_REPLY_ID_VA_NEGATIVE_TRIGGER;
            }
            L2_DBG_MSG1("speech_detected: %d", p_ext_data->speech_detected);
        }
    }
    else
    {
        /* In PASS_THRU don't send anything. */
    }

    *p_notification = notification;
    return notify;
}

/* Data processing function */
void svad_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    SVAD_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned available_space = 0, available_data;
    OPMSG_OP_CLIENT_REPLY_ID notification;
    bool speech_changed = FALSE;
    unsigned ip_proc_data;
	
    do {
    patch_fn(svad_process_data);

    /* The input buffers must be connected*/
    if( (p_ext_data->ext_mic_buffer == NULL) || (p_ext_data->int_mic_buffer == NULL) )
    {
        return;
    }

	/* number of samples to process at ext mic input buffer    */
    available_data = cbuffer_calc_amount_data_in_words(p_ext_data->ext_mic_buffer);
	
    /* Check for sufficient data at the input buffer*/
    if(available_data < SVAD_FRAME_SIZE)
    {
        return;
    }

 	/* number of samples available to process at int mic input buffer */
    available_data = cbuffer_calc_amount_data_in_words(p_ext_data->int_mic_buffer);
    
    /* Check for sufficient data at the input buffer*/
    if(available_data < SVAD_FRAME_SIZE)
    {
        return;
    }   
    
    if(p_ext_data->output_buffer != NULL)
	{
        available_space = cbuffer_calc_amount_space_in_words(p_ext_data->output_buffer);
        if(available_space < SVAD_FRAME_SIZE)
        {
            return;
        }
	}

    if (p_ext_data->config == FULL_PROC)
    {
        p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->fft_object_ptr->real_scratch_ptr =
        scratch_commit(filter_bank_parameters_fft256_num_bin*sizeof(int), MALLOC_PREFERENCE_DM1);
        p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->fft_object_ptr->imag_scratch_ptr =
        scratch_commit(filter_bank_parameters_fft256_num_bin*sizeof(int), MALLOC_PREFERENCE_DM2);
        p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->fft_object_ptr->fft_scratch_ptr =
        scratch_commit(filter_bank_parameters_fft256_num_bin*sizeof(int), MALLOC_PREFERENCE_DM2);

        p_ext_data->svad_algs_container->afb_int_mic_object_ptr->fft_object_ptr->real_scratch_ptr =
        p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->fft_object_ptr->real_scratch_ptr;
        p_ext_data->svad_algs_container->afb_int_mic_object_ptr->fft_object_ptr->imag_scratch_ptr =
        p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->fft_object_ptr->imag_scratch_ptr;
        p_ext_data->svad_algs_container->afb_int_mic_object_ptr->fft_object_ptr->fft_scratch_ptr =
        p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->fft_object_ptr->fft_scratch_ptr;    

        /* FFT and SVAD can share a scratch buffer */
        p_ext_data->svad_algs_container->svad_object_ptr->ipd_scratch_ouput_ptr =
        p_ext_data->svad_algs_container->afb_ext_mic_object_ptr->fft_object_ptr->real_scratch_ptr;

        // Perform SVAD processing
       // svad_init(p_ext_data->svad_algs_container->svad_object_ptr, p_ext_data->svad_algs_container->afb_int_mic_object_ptr, p_ext_data->svad_algs_container->afb_ext_mic_object_ptr, p_ext_data);
        svad_init(p_ext_data, p_ext_data->svad_algs_container);
        svad_process(p_ext_data->int_mic_buffer, p_ext_data->ext_mic_buffer, p_ext_data->svad_algs_container, p_ext_data);
        
        /* Free the scratch memory used */
        scratch_free();

        speech_changed = speech_detected(p_ext_data);

        if (check_notification(p_ext_data, speech_changed, &notification))
        {
            common_send_simple_unsolicited_message(op_data, notification);
        }

    }

    if (p_ext_data->output_buffer != NULL )
    {
        /* copy data from external mic to output (copy can be in-place) */
        cbuffer_copy(p_ext_data->output_buffer, p_ext_data->ext_mic_buffer, SVAD_FRAME_SIZE);
    }
    else
    {
        /* No output buffer connected */
        cbuffer_discard_data(p_ext_data->output_buffer, SVAD_FRAME_SIZE);
    }
    
    /* Copy metadata from input terminal to output terminal */
    /* metadata_strict_transport can handle an output buffer not being connected
       and it can handle in-place processing */
    metadata_strict_transport(p_ext_data->ext_mic_buffer,
                                  p_ext_data->output_buffer,
                                  SVAD_FRAME_SIZE * OCTETS_PER_SAMPLE);
    
    /* touched input */
    touched->sinks = TOUCHED_SINK_0 | TOUCHED_SINK_1;

    if( p_ext_data->output_buffer != NULL  )
    {
        touched->sources = TOUCHED_SOURCE_0;
    }

    ip_proc_data = p_ext_data->samples_consumed;

    }while(ip_proc_data!=0);
}

/* **************************** Operator message handlers ******************************** */

bool svad_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SVAD_OP_DATA *p_ext_data = get_instance_data(op_data);

    return cpsGetParameterMsgHandler(&p_ext_data->params_def ,message_data, resp_length,resp_data);
}

bool svad_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SVAD_OP_DATA *p_ext_data = get_instance_data(op_data);
   
    return cpsGetDefaultsMsgHandler(&p_ext_data->params_def ,message_data, resp_length,resp_data);
}

bool svad_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SVAD_OP_DATA *p_ext_data = get_instance_data(op_data);
    bool retval;

    retval = cpsSetParameterMsgHandler(&p_ext_data->params_def ,message_data, resp_length,resp_data);

    /* Set the Reinit flag after setting the paramters */
    p_ext_data->ReInitFlag = 1;

    return retval;
}

bool svad_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SVAD_OP_DATA *p_ext_data = get_instance_data(op_data);
    patch_fn_shared(svad_capability);
    
    unsigned  *resp;

    if(!common_obpm_status_helper(message_data,resp_length,resp_data,sizeof(SVAD_STATISTICS),&resp))
    {
         return FALSE;
    }
    
    if(resp)
    {
        resp = cpsPack2Words(p_ext_data->config, p_ext_data->speech_detected, resp);
    }

    return TRUE;
}

static bool ups_params_svad(void* instance_data,PS_KEY_TYPE key,PERSISTENCE_RANK rank,
                 uint16 length, unsigned* data, STATUS_KYMERA status,uint16 extra_status_info)
{
    SVAD_OP_DATA *p_ext_data = get_instance_data((OPERATOR_DATA *)instance_data);

    cpsSetParameterFromPsStore(&p_ext_data->params_def,length,data,status);

    /* Set the Reinit flag after setting the paramters */
    p_ext_data->ReInitFlag = 1;

    return(TRUE);
}

bool svad_opmsg_set_ucid(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SVAD_OP_DATA *p_ext_data = get_instance_data(op_data);
    PS_KEY_TYPE key;
    bool retval;

    retval = cpsSetUcidMsgHandler(&p_ext_data->params_def,message_data,resp_length,resp_data);
    key = MAP_CAPID_UCID_SBID_TO_PSKEYID(SVAD_CAP_ID,p_ext_data->params_def.ucid,OPMSG_P_STORE_PARAMETER_SUB_ID);
    ps_entry_read((void*)op_data,key,PERSIST_ANY,ups_params_svad);

    return retval;
}

bool svad_opmsg_get_ps_id(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SVAD_OP_DATA *p_ext_data = get_instance_data(op_data);

    return cpsGetUcidMsgHandler(&p_ext_data->params_def,SVAD_CAP_ID,message_data,resp_length,resp_data);
}

bool svad_configure_mode_change(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SVAD_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned new_config = OPMSG_FIELD_GET(message_data, OPMSG_VAD_MODE_CHANGE, WORKING_MODE);
    
    patch_fn_shared(svad_capability);  
           
    if ((new_config == PASS_THRU) || (new_config == FULL_PROC) )
    {
        /* Put SVAD into the requested configuration */
        p_ext_data->config = new_config;
        L2_DBG_MSG1("svad_configure_mode_change: %d", new_config);

        /* Reintialize on mode change*/
        p_ext_data->ReInitFlag = 1; 
        p_ext_data->speech_detected = 0;
        
        return TRUE;
    }
    /* Unknown mode configuration */
    L2_DBG_MSG("SVAD: Invalid mode configuration");
    return FALSE;
}

bool svad_set_buffer_size(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SVAD_OP_DATA *p_ext_data = get_instance_data(op_data);
    patch_fn_shared(svad_capability);
    
    unsigned buf_size = OPMSG_FIELD_GET(message_data, OPMSG_COMMON_SET_BUFFER_SIZE, BUFFER_SIZE);

    if (opmgr_op_is_running(op_data))
    {
        L2_DBG_MSG("svad_set_buffer_size: Cannot set buffer size because the operator is running!");
        return FALSE;
    }

    if  (p_ext_data->output_buffer != NULL)
    {
        L2_DBG_MSG("svad_set_buffer_size: Cannot set the buffer size for an operator with connected outputs!");
        return FALSE;
    }

    if ( (p_ext_data->ext_mic_buffer != NULL) || (p_ext_data->int_mic_buffer != NULL) )
    {
        L2_DBG_MSG("svad_set_buffer_size: Cannot set the buffer size for an operator with connected inputs!");
        return FALSE;
    }

    /*The requested buffer size can't be smaller than the default buffer size. */
    if (buf_size < SVAD_DEFAULT_BUFFER_SIZE)
    {
        L2_DBG_MSG("svad_set_buffer_size: Buffer size too small!");
        return FALSE;
    }
    
    p_ext_data->buf_size = buf_size;
    return TRUE;
}
