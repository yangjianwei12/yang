/****************************************************************************
 * Copyright (c) 2015 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  mixer.c
 * \ingroup  capabilities
 *
 *  Mixer capability.
 *
 */
/****************************************************************************
Include Files
*/
#include "mixer_private.h"
#include "mixer_gen_c.h"
#include "capabilities.h"

/****************************************************************************
Private Macro Definitions
*/

/* Mixer streams legacy for 3-to-1 mix metadata */
typedef enum
{
    OUTPUT,
    STREAM1,
    STREAM2,
    STREAM3,
    STREAM_MAX,
    WRONG_CHANNEL = 0xff
} MIXER_STREAM;

/** Turns a stream enum value into a meta data buffer array index */
#define META_IDX_FROM_STREAM(s) ((s) - STREAM1)

#define MASK_8LSB   (-1 >> (DAWTH-16))

#ifdef CAPABILITY_DOWNLOAD_BUILD
#define MIXER_CAP_ID CAP_ID_DOWNLOAD_MIXER
#else
#define MIXER_CAP_ID CAP_ID_MIXER
#endif

/****************************************************************************
Public Constant Declarations
*/
/** The mixer capability function handler table */
const handler_lookup_struct mixer_handler_table =
{
    mixer_create,             /* OPCMD_CREATE */
    mixer_destroy,            /* OPCMD_DESTROY */
    mixer_start,              /* OPCMD_START */
    base_op_stop,             /* OPCMD_STOP */
    base_op_reset,            /* OPCMD_RESET */
    mixer_connect,            /* OPCMD_CONNECT */
    mixer_disconnect,         /* OPCMD_DISCONNECT */
    mixer_buffer_details,     /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,  /* OPCMD_DATA_FORMAT */
    base_op_get_sched_info    /* OPCMD_GET_SCHED_INFO */
};

/** The mixer capability operator message handler table */
const opmsg_handler_lookup_table_entry mixer_3_to_1_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_MIXER_ID_SET_STREAM_GAINS, mixer_set_stream_gains},
    {OPMSG_MIXER_ID_SET_STREAM_CHANNELS, mixer_set_stream_channels},
    {OPMSG_MIXER_ID_SET_RAMP_NUM_SAMPLES, mixer_set_ramp_num_samples},
    {OPMSG_MIXER_ID_SET_PRIMARY_STREAM, mixer_set_primary_stream},
    {OPMSG_COMMON_SET_SAMPLE_RATE, mixer_set_sample_rate},
    {OPMSG_MIXER_ID_SET_CHANNEL_GAINS, mixer_set_channel_gain},
    {OPMSG_MIXER_ID_GET_CONFIG, mixer_opmsg_get_config},
    {OPMSG_COMMON_ID_SET_BUFFER_SIZE, mixer_opmsg_set_buffer_size},
    {OPMSG_MIXER_ID_SET_METADATA_STREAM, mixer_set_metadata_stream},
    {OPMSG_COMMON_ID_SET_CONTROL,  mixer_opmsg_obpm_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,   mixer_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS, mixer_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,   mixer_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,   mixer_opmsg_obpm_get_status},
    {0, NULL}
};

/** mixer capability data (3 to 1 mixer )*/
const CAPABILITY_DATA mixer_cap_data =
{
    MIXER_CAP_ID,                                       /* Capability ID */
    MIXER_MX_VERSION_MAJOR, 0,                          /* Version information - hi and lo parts */
    MIXER_MAX_INPUT_CHANS, MIXER_MAX_OUTPUT_CHANS,      /* Max number of sinks/inputs and sources/outputs */
    &mixer_handler_table,                               /* Pointer to message handler function table */
    mixer_3_to_1_opmsg_handler_table,                   /* Pointer to operator message handler function table */
    mixer_process_data,                                 /* Pointer to data processing function */
    0,                                                  /* Reserved */
    sizeof(GEN_MIXER_OP_DATA)
};
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_MIXER, GEN_MIXER_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_MIXER, GEN_MIXER_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

/****************************************************************************
Private Function Definitions
*/
static inline GEN_MIXER_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (GEN_MIXER_OP_DATA *) base_op_get_instance_data(op_data);
}

/****************************************************************************
Public Function Definitions
*/
/**
 * \brief Clears up the mixer operator if it's in the correct state.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the destroy request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool mixer_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    if(base_op_destroy_lite(op_data, response_data))
    {
        GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);

        mixer_deallocate_channels(mixer_data);
        return TRUE;
    }

    return FALSE;
}

/**
 * \brief Configures the mixer defaults.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the create request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool mixer_create(OPERATOR_DATA *op_data, void *message_data,
                                        unsigned *response_id, void **response_data)
{
    GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);
    if (!base_op_create_lite(op_data, response_data))
    {
        return FALSE;
    }

    /* Setup - zero sink.  Never part of sink list */
    mixer_data->zero_sink.next = NULL;
    mixer_data->zero_sink.base_addr = (unsigned*)&mixer_data->zero_sink.next;
    mixer_data->zero_sink.read_addr = mixer_data->zero_sink.base_addr;
    mixer_data->zero_sink.length    = sizeof(unsigned);
    mixer_data->sample_rate = (unsigned)stream_if_get_system_sampling_rate();
    mixer_data->mixer_changed = 0;

    /* Memory allocated on stream configuration
       Initiaized zero sinks, sources, and streams
    */

    return TRUE;
}

/**
 * \brief Connects a mixer terminal to a buffer.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the connect request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool mixer_connect(OPERATOR_DATA *op_data, void *message_data,
                   unsigned *response_id, void **response_data)
{
    GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);
    unsigned  terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    unsigned  terminal_num = terminal_id & TERMINAL_NUM_MASK;
    unsigned  terminal_mask;
    tCbuffer* buffer = OPMGR_GET_OP_CONNECT_BUFFER(message_data);

    if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        return FALSE;
    }

    terminal_mask = (1<<terminal_num);

    if(terminal_id & TERMINAL_SINK_MASK)
    {
       if(terminal_num>=mixer_data->max_sinks)
       {
           base_op_change_response_status(response_data, STATUS_CMD_FAILED);
           return TRUE;
       }

       opmgr_op_suspend_processing(op_data);
       mixer_data->sinks[terminal_num].buffer = buffer;
       mixer_data->reset_mixers     = TRUE;
       mixer_data->connected_sinks |= terminal_mask;
    }
    else
    {
       if(terminal_num>=mixer_data->max_sources)
       {
           base_op_change_response_status(response_data, STATUS_CMD_FAILED);
           return TRUE;
       }

       opmgr_op_suspend_processing(op_data);
       mixer_data->sources[terminal_num]->buffer = buffer;
       mixer_data->reset_mixers       = TRUE;
       mixer_data->connected_sources |= terminal_mask;
    }

    set_metadata(mixer_data,terminal_id,buffer);

    opmgr_op_resume_processing(op_data);
    return TRUE;
}


/**
 * \brief Reports the buffer requirements of the mixer terminal
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the buffer size request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool mixer_buffer_details(OPERATOR_DATA *op_data, void *message_data,
                          unsigned *response_id, void **response_data)
{
    GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);
    OP_BUF_DETAILS_RSP   *resp;
    unsigned terminal_id = OPMGR_GET_OP_BUF_DETAILS_TERMINAL_ID(message_data);

    if (!base_op_buffer_details_lite(op_data, response_data))
    {
        return FALSE;
    }

    resp = ((OP_BUF_DETAILS_RSP*)*response_data);

    resp->metadata_buffer = get_metadata(mixer_data, terminal_id);
    resp->supports_metadata = TRUE;

    if (((terminal_id & TERMINAL_SINK_MASK) == 0) && (mixer_data->output_buffer_size != 0))
    {
        resp->b.buffer_size = mixer_data->output_buffer_size;
    }
    else
    {
        /* return the minimum buffer size - based on sampling rate */
        resp->b.buffer_size = frac_mult(mixer_data->sample_rate,FRACTIONAL(0.004));
    }
    return TRUE;
}

/**
 * \brief Disconnects the mixer terminal from a buffer.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the disconnect request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool mixer_disconnect(OPERATOR_DATA *op_data, void *message_data,
                      unsigned *response_id, void **response_data)
{
   GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);
   unsigned  terminal_id = OPMGR_GET_OP_DISCONNECT_TERMINAL_ID(message_data);
   unsigned  terminal_num = terminal_id & TERMINAL_NUM_MASK;
   unsigned  terminal_mask;

   if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
   {
        return FALSE;
   }
   terminal_mask = ~(1<<terminal_num);

    if(terminal_id & TERMINAL_SINK_MASK)
    {
       if(terminal_num>=mixer_data->max_sinks)
       {
           base_op_change_response_status(response_data, STATUS_CMD_FAILED);
           return TRUE;
       }

       opmgr_op_suspend_processing(op_data);
       mixer_data->sinks[terminal_num].buffer = NULL;
       mixer_data->reset_mixers       = TRUE;
       mixer_data->connected_sinks &= terminal_mask;
    }
    else
    {
       if(terminal_num>=mixer_data->max_sources)
       {
           base_op_change_response_status(response_data, STATUS_CMD_FAILED);
           return TRUE;
       }

       opmgr_op_suspend_processing(op_data);
       mixer_data->sources[terminal_num]->buffer = NULL;
       mixer_data->reset_mixers       = TRUE;
       mixer_data->connected_sources &= terminal_mask;
    }

    clr_metadata(mixer_data,terminal_id);

    opmgr_op_resume_processing(op_data);
    return TRUE;
}

/**
 * \brief Starts the mixer capability so that data will flow through this
 * capability.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the start request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool mixer_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);

    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        return FALSE;
    }

    if (opmgr_op_is_running(op_data))
    {
        /* Operator already started nothing to do. */
        return TRUE;
    }

    /* Check that channel setup has occured and at least one source is connected */
    if((mixer_data->sources==NULL)||(mixer_data->connected_sources==0) )
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    return TRUE;
}

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Mixes input channels applying a gain.
 *
 * \param op_data Pointer to the operator instance data.
 * \param touched structure to indicate to Opmgr whether the operator wishes to be
 * scheduled to run again now.
 *
 * \return the bits indicating what input/output channels have been touched.
 */

void mixer_process_data(OPERATOR_DATA* op_data, TOUCHED_TERMINALS *touched)
{
   GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);
   GEN_MIXER_SOURCE_INFO *src_ptr;
   GEN_MIXER_SINK_INFO   *sink_ptr;
   unsigned samples_to_process,amount;

   patch_fn(mixer_process_data);

   /* Handle Connection Change */
   if(mixer_data->reset_mixers || mixer_data->reset_gains)
   {
       if(mixer_data->reset_mixers)
       {
           setup_mixers(mixer_data);
       }
       else
       {
           setup_mixers_gain_change(mixer_data);
       }

       /* Restart transition of gains */
       if(mixer_data->restart_transition)
       {
            mixer_data->transition_count = mixer_data->samples_to_ramp;
            mixer_data->restart_transition = FALSE;
       }

       mixer_data->reset_mixers       = FALSE;
       mixer_data->reset_gains        = FALSE;
       mixer_data->source_wait_buffer = NULL;
       mixer_data->sink_wait_buffer   = NULL;

       mixer_data->mixer_changed = 1;
   }

   /* Check sinks and sources */
   src_ptr  = mixer_data->source_list;
   sink_ptr = mixer_data->sink_list;
   if(!src_ptr || !sink_ptr)
   {
      return;
   }

   /*Accellerator waiting on data */
   if(mixer_data->sink_wait_buffer)
   {
      amount = cbuffer_calc_amount_data_in_words(mixer_data->sink_wait_buffer);
      if(amount < GEN_MIXER_DEFAULT_BLOCK_SIZE)
      {
         touched->sinks = mixer_data->active_sinks;
         return;
      }
      mixer_data->sink_wait_buffer=NULL;
   }

   /*Accelerator waiting on space */
   samples_to_process = UINT_MAX;
   if(mixer_data->source_wait_buffer)
   {
      amount = cbuffer_calc_amount_space_in_words(mixer_data->source_wait_buffer);
      if(amount < GEN_MIXER_DEFAULT_BLOCK_SIZE)
      {
         return;
      }
      mixer_data->source_wait_buffer=NULL;
      samples_to_process = amount;
   }

   /* Minimum space at sources */
   do
   {
      amount = cbuffer_calc_amount_space_in_words(src_ptr->buffer);
      if(amount<samples_to_process)
      {
         if(amount < GEN_MIXER_DEFAULT_BLOCK_SIZE)
         {
            mixer_data->source_wait_buffer=src_ptr->buffer;
            return;
         }
         samples_to_process=amount;
      }
      src_ptr = src_ptr->next;
   }while(src_ptr);


  /* Minimum data at sinks */
  do
  {
     amount = cbuffer_calc_amount_data_in_words(sink_ptr->buffer);
     if(amount<=samples_to_process)
     {
        if(amount < GEN_MIXER_DEFAULT_BLOCK_SIZE)
        {
           touched->sinks = mixer_data->active_sinks;
           mixer_data->sink_wait_buffer = sink_ptr->buffer;
           return;
        }
        samples_to_process=amount;
     }
     sink_ptr = sink_ptr->next;
  }while(sink_ptr);

   /* OK we have something to process.  Update Kick flags */
   touched->sources = mixer_data->connected_sources;
   touched->sinks = mixer_data->active_sinks;

   handle_metadata(mixer_data,samples_to_process);
   gen_mixer_process_channels(mixer_data,samples_to_process);
}

inline static bool dB60GainFrom16bit(int *lpgain)
{
    int gain = *lpgain;

    /* sign extend 16-bit */
    if (gain & 0x8000)
    {
        gain = gain | (UINT_MAX ^ 0xFFFF);
    }
    else if ((gain & 0xFFFF) == 0)
    {
        gain = 0;
    }
    else
    {
        /* Any value > 0dB is not supported by this capability only
        * attenuation is supported */
        return FALSE;
    }
    /* Threshold the attenuation.   Effective zero gain is not mixed */
    if(gain < (-90*60))
    {
        *lpgain = 0;
    }
    else
    {
        *lpgain = dB60toLinearFraction(gain);
    }

    return TRUE;
}

/**
 * \brief Sets the gain for individual channels.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the set channel gain request message
 * \param resp_length Length of the response
 * \param resp_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool mixer_set_channel_gain(OPERATOR_DATA *op_data, void *message_data,
                                        unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);
    GEN_MIXER_GAIN_DEF gain_def;
    int      chain_gains;
    unsigned i;
    unsigned inv_ramp;

    chain_gains = OPMSG_FIELD_GET(message_data, OPMSG_MIXER_SET_CHANNEL_GAINS, NUMBER_CHANNELS_CHANGE);
    /* Check the number of channels to update is the expected. */
    if((OPMGR_GET_OPMSG_LENGTH((OP_MSG_REQ *)message_data) != (chain_gains*2) + OPMSG_MIXER_SET_CHANNEL_GAINS_CHANNEL_NUM_WORD_OFFSET))
    {
        return FALSE;
    }

    /* Is gain transitioned or immediate */
    inv_ramp = opmgr_op_is_running(op_data)? mixer_data->inv_samples_to_ramp : 0;

    /* each sink applies to ony one source */
    gain_def.src_terminal  = -1;
    gain_def.single_source =  1;

    /* gen_mixer_set_gain isn't atomic, so synchronise update */
    opmgr_op_suspend_processing(op_data);
    for(i=0; i < chain_gains; i++)
    {
        int gain,term_idx;

        term_idx = OPMSG_FIELD_GET_FROM_OFFSET(message_data, OPMSG_MIXER_SET_CHANNEL_GAINS, CHANNEL_NUM, 2*i);
        gain     = OPMSG_FIELD_GET_FROM_OFFSET(message_data, OPMSG_MIXER_SET_CHANNEL_GAINS, NEW_CHANNEL_GAIN, 2*i);

        if(!dB60GainFrom16bit(&gain))
        {
            return FALSE;
        }

        gain_def.sink_mask = 1<<term_idx;
        gain_def.gain      = gain;

        gen_mixer_set_gain(mixer_data,&gain_def,inv_ramp);

    }

    if(inv_ramp)
    {
        mixer_data->restart_transition = TRUE;
    }

    mixer_data->reset_gains = TRUE;
    opmgr_op_resume_processing(op_data);
    return TRUE;
}

/**
 * \brief Handles the opmsg that changes the stream gains.
 *
 * \param op_data Pointer to the operator instance data
 * \param message_data Pointer to the destroy request message
 * \param resp_length Length of the response
 * \param resp_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
#define MINIMUM_MIXER_SET_STREAM_GAINS_LENGTH  1

bool mixer_set_stream_gains(OPERATOR_DATA *op_data, void *message_data,
                                        unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);
    GEN_MIXER_GAIN_DEF gain_def;
    unsigned inv_ramp, i, msg_length;

    msg_length = OPMGR_GET_OPMSG_LENGTH((OP_MSG_REQ *)message_data);
    if(msg_length <= MINIMUM_MIXER_SET_STREAM_GAINS_LENGTH)
    {
        return FALSE;
    }
    msg_length -= MINIMUM_MIXER_SET_STREAM_GAINS_LENGTH;
    if(msg_length > mixer_data->num_streams)
    {
        return FALSE;
    }

    /* Apply to all sources with stream */
    gain_def.src_terminal  = -1;
    gain_def.single_source =  0;

    /* Is gain transitioned or immediate */
    inv_ramp = opmgr_op_is_running(op_data)? mixer_data->inv_samples_to_ramp : 0;

    /* gen_mixer_set_gain isn't atomic, so synchronise update */
    opmgr_op_suspend_processing(op_data);
    /* Apply gains to streams */
    for (i = 0; i < msg_length; i++)
    {
        int gain = OPMSG_FIELD_GET_FROM_OFFSET(message_data, OPMSG_MIXER_SET_STREAM_GAINS, STREAM1_GAIN, i);

        if(!dB60GainFrom16bit(&gain))
        {
            return FALSE;
        }

        gain_def.sink_mask = mixer_data->streams[i].sink_mask;
        gain_def.gain      = gain;

        gen_mixer_set_gain(mixer_data, &gain_def, inv_ramp);
    }

    if(inv_ramp)
    {
        mixer_data->restart_transition = TRUE;
    }

    mixer_data->reset_gains = TRUE;
    opmgr_op_resume_processing(op_data);
    return TRUE;
}

/**
 * \brief Handles the opmsg that changes the number of channels per stream.
 *
 * \param op_data Pointer to the operator instance data
 * \param message_data Pointer to the destroy request message
 * \param resp_length Length of the response
 * \param resp_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool mixer_set_stream_channels(OPERATOR_DATA *op_data, void *message_data,
                               unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);
    unsigned       num_group1,num_group2,num_group3,num_sinks,num_sources,i;

    /*Should allow to change the number of channels only if there is nothing connected? */
    if(mixer_data->connected_sources || mixer_data->connected_sinks )
    {
        /* Something is connected, we cannot not change the configuration */
        return FALSE;
    }

    /* Three stream groups */
    num_group1 = OPMSG_FIELD_GET(message_data, OPMSG_MIXER_SET_STREAM_CHANNELS, STREAM1_NUM_CHANNELS);
    num_group2 = OPMSG_FIELD_GET(message_data, OPMSG_MIXER_SET_STREAM_CHANNELS, STREAM2_NUM_CHANNELS);
    num_group3 = OPMSG_FIELD_GET(message_data, OPMSG_MIXER_SET_STREAM_CHANNELS, STREAM3_NUM_CHANNELS);

    /* Total number of sinks */
    num_sinks = num_group1+num_group2+num_group3;
    if(num_sinks > GEN_MIXER_MAX_CHANNELS)
    {
        /* More channels than the allowed */
        return FALSE;
    }
    /* Total number of sources */
    num_sources = (num_group1>num_group2) ? num_group1 : num_group2;
    if(num_sources<num_group3)
    {
        num_sources = num_group3;
    }

    if(!mixer_allocate_channels(mixer_data,num_sinks,num_sources,3))
    {
        /* Memory allocation failure */
        return FALSE;
    }

    /* Need to allocate each source seperately as
       the number of sinks per source is not fixed */
    for(i=0;i<num_sources;i++)
    {
        unsigned config_mask=0;

        /* setup bit-wise flag of sinks to mix into source */
        if(i<num_group1)
        {
            config_mask |= (1<<i);
        }
        if(i<num_group2)
        {
            config_mask |= (1<<(i+num_group1));
        }
        if(i<num_group3)
        {
            config_mask |= (1<<(i+num_group1+num_group2));
        }
        /* Allocate Source and initialize gains to equal mix */
        if(!mixer_allocate_source(mixer_data,i,config_mask,TRUE))
        {
            mixer_deallocate_channels(mixer_data);
            return FALSE;
        }
    }

    /* Setup three groups.   Bit-wise mask of sinks in group */
    if(num_group1>0)
    {
        mixer_data->streams[0].sink_mask = ((1<<num_group1)-1);
    }
    if(num_group2>0)
    {
        mixer_data->streams[1].sink_mask = ((1<<num_group2)-1)<<num_group1;
    }
    if(num_group3>0)
    {
        mixer_data->streams[2].sink_mask = ((1<<num_group3)-1)<<(num_group1+num_group2);
    }

    return TRUE;
}

/**
 * \brief Handles the opmsg that changes the primary stream.
 *
 * \param op_data Pointer to the operator instance data
 * \param message_data Pointer to the destroy request message
 * \param resp_length Length of the response
 * \param resp_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool mixer_set_primary_stream(OPERATOR_DATA *op_data, void *message_data,
                                        unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
   /* primary stream only used for metadata.  Here for legacy only */

    GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);
    MIXER_STREAM new_primary_stream = (MIXER_STREAM) OPMSG_FIELD_GET(message_data, OPMSG_MIXER_SET_PRIMARY_STREAM, STREAM_NUMBER);

    /* Check if the value received is valid. */
    if((new_primary_stream < STREAM1) || (new_primary_stream > STREAM3))
    {
        return FALSE;
    }

    mixer_data->primary_stream = META_IDX_FROM_STREAM(new_primary_stream);
    if (mixer_data->metadata_tracks_primary == TRUE)
    {
        mixer_data->metadata_stream_idx = mixer_data->primary_stream;
    }

    return TRUE;
}

/**
 * \brief Set sample rate message handler. Stores the sample rate in the operator
 * data structure.
 *
 * \param op_data the instance of the mixer to set the sample rate of
 * \param message_data the message containing the new sample rate
 * \param resp_length return value of the length of resp_data
 * \param resp_data return value the response message
 *
 * \return boolean indicating if the message was handled successfully or not
 */
bool mixer_set_sample_rate(OPERATOR_DATA *op_data, void *message_data,
                           unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
   /* Sample rate not used.  Here for legacy only */

   GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);

   /* Check if the sample rate can be changed. */
   if (opmgr_op_is_running(op_data))
   {
       return FALSE;
   }
   /* The message sample format is fs/25 so convert back */
   mixer_data->sample_rate = SAMPLE_RATE_FROM_COMMON_OPMSG(message_data);

   return TRUE;
}

/**
 * \brief Handles the opmsg that changes the number of samples to process on fadeout/in.
 *
 * \param op_data Pointer to the operator instance data
 * \param message_data Pointer to the destroy request message
 * \param resp_length Length of the response
 * \param resp_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool mixer_set_ramp_num_samples(OPERATOR_DATA *op_data, void *message_data,
                                unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);
    unsigned ramp_count = (
            ((OPMSG_FIELD_GET(message_data, OPMSG_MIXER_SET_RAMP_NUM_SAMPLES, NUM_SAMPLES_MSB) & 0xFF) << 16)
            | (OPMSG_FIELD_GET(message_data, OPMSG_MIXER_SET_RAMP_NUM_SAMPLES, NUM_SAMPLES_LSB)&0xFFFF)
            );

    mixer_data->samples_to_ramp     = ramp_count;
    mixer_data->inv_samples_to_ramp = (ramp_count>0) ? pl_fractional_divide(1,ramp_count) : 0;
    return TRUE;
}


/****************************************************************************
Private Function Definitions
*/

void mixer_deallocate_channels(GEN_MIXER_OP_DATA *mixer_data)
{
    unsigned i;

    if (mixer_data->sources == NULL)
    {
        return;
    }

    /* Free sources */
    for (i = 0; i < mixer_data->max_sources; i++)
    {
        pfree(mixer_data->sources[i]);
    }

    /* Free all else */
    pfree(mixer_data->sources);

    /* Clear pointers and amounts */
    mixer_data->max_sources = 0;
    mixer_data->max_sinks   = 0;
    mixer_data->sources     = NULL;
    mixer_data->sinks       = NULL;
    mixer_data->num_streams = 0;
    mixer_data->streams     = NULL;
}

bool mixer_allocate_channels(GEN_MIXER_OP_DATA *mixer_data,unsigned num_sinks,unsigned num_sources,unsigned num_streams)
{
    unsigned alloc_size;

    mixer_deallocate_channels(mixer_data);

    /* Allocate sinks array */
    alloc_size  = num_sinks*sizeof(GEN_MIXER_SINK_INFO);
    /* allocate source pointer array */
    alloc_size += num_sources*sizeof(GEN_MIXER_SOURCE_INFO*);
    /* allocate sink group array */
    alloc_size += num_streams*sizeof(GEN_MIXER_STREAM);

    /* Setup array of sources pointers */
    mixer_data->sources = (GEN_MIXER_SOURCE_INFO**)xzpmalloc(alloc_size);
    if(mixer_data->sources==NULL)
    {
        return FALSE;
    }
    /* Setup array of sinks.  Inlcudes sink buffer */
    mixer_data->sinks = (GEN_MIXER_SINK_INFO*)&mixer_data->sources[num_sources];

    mixer_data->max_sinks   = num_sinks;
    mixer_data->max_sources = num_sources;

    /* setup arrays of stream.   Set of sinks that make up group*/
    mixer_data->num_streams = num_streams;
    mixer_data->streams     = (GEN_MIXER_STREAM*)&mixer_data->sinks[num_sinks];

    return TRUE;
}

GEN_MIXER_SOURCE_INFO* mixer_allocate_source(GEN_MIXER_OP_DATA *mixer_data,unsigned src_idx,unsigned sink_mask,bool bInitGains)
{
    unsigned alloc_size,num_sinks=0;
    GEN_MIXER_SOURCE_INFO *src_ptr;

    /* How many sinks */
    for(alloc_size=sink_mask;alloc_size;alloc_size>>=1)
    {
        if(alloc_size&0x1)
        {
            num_sinks++;
        }
    }

    /* Allocate source memory
    *   Includes source buffer and array of sink mixers (sinks + gains)
    */
    alloc_size = sizeof(GEN_MIXER_SOURCE_INFO) + num_sinks*sizeof(GEN_MIXER_MIX_INFO);
    src_ptr = (GEN_MIXER_SOURCE_INFO*)xzpmalloc(alloc_size);
    if(src_ptr==NULL)
    {
        return NULL;
    }

    src_ptr->configured_sinks = sink_mask;

    if(bInitGains)
    {
        /* Initialize stream gains for equal mix */
        unsigned i,initial_gain = pl_fractional_divide(1,num_sinks);

        for(i=0;i<num_sinks;i++)
        {
            src_ptr->mixes[i].current_gain = initial_gain;
            src_ptr->mixes[i].target_gain  = initial_gain;
        }
    }

    mixer_data->sources[src_idx] = src_ptr;
    return src_ptr;
}

GEN_MIXER_MIX_INFO *setup_mixes(GEN_MIXER_SOURCE_INFO *src_ptr,unsigned sink_mask,GEN_MIXER_SINK_INFO *sink_pptr,GEN_MIXER_OP_DATA *mixer_data)
{
    GEN_MIXER_MIX_INFO    *mix_ptr  = src_ptr->mixes;
    GEN_MIXER_MIX_INFO    *mix_list = NULL;
    GEN_MIXER_SINK_INFO  *zero_sink = &mixer_data->zero_sink;
    unsigned              inv_ramp  = mixer_data->inv_samples_to_ramp;
    unsigned              is_first,sink_count=0,mix_mask;

    /* Update MIX list for source */
    mix_mask = src_ptr->configured_sinks;
    while(mix_mask&sink_mask)
    {
        /* Is source interested in this sink? */
        if(mix_mask&0x1)
        {
            /* Save pointer to sink info */
            mix_ptr->sink = sink_pptr;
            /* Is it connected? */
            if(sink_mask & 0x1)
            {
                /* Handle Gain transition.  Needed here to ensure all channels are in sync with transition */
                mix_ptr->gain_adjust = frac_mult(mix_ptr->target_gain - mix_ptr->current_gain,inv_ramp);
                if(mix_ptr->gain_adjust==0)
                {
                    mix_ptr->current_gain = mix_ptr->target_gain;
                }

                /* Count sinks and append to mix list if gain is not zero */
                if(mix_ptr->current_gain || mix_ptr->target_gain)
                {
                    sink_count++;
                    mix_ptr->next = mix_list;
                    mix_list      = mix_ptr;
                }
            }
            /* Next mix */
            mix_ptr++;
        }

        /* next sink.  Shift bits in mask */
        sink_pptr++;
        sink_mask>>=1;
        mix_mask >>=1;
    }

    /* Check if all no non-zero gain mixes */
    if(sink_count==0)
    {
        mix_ptr  = src_ptr->mixes;
        /* Use a dummy sink for silence */
        sink_count=1;
        mix_ptr->sink = zero_sink;
        mix_ptr->next = NULL;
        mix_list      = mix_ptr;
    }

    /* Define mixer functions.
    The mixer is optimized for a 1,2, or 3 stream mix operation
    To mix more than 3 streams we group the streams and perform
    the mix operations sequentially.   However, each mix operation
    after the first group uses the output of the previous mix
    operation as one of the input streams.   As a result
    each group after the first is a mix of 1 or 2 streams
    */
    is_first=1;
    for(mix_ptr=mix_list;sink_count;)
    {
        /* Limit streams to be mixed to a maximum of three for first
        group and two for each sucessive group */
        unsigned mix_grp = 2+is_first;   /* mix_grp = 3,2,2,... */

        if(mix_grp>sink_count)
        {
            mix_grp = sink_count;
        }
        sink_count -= mix_grp;

        /* mix_grp = 1,2, or 3 is the numbe of streams being mixed

        mix_function is an index into a function tables
        "gen_mixer_no_trans_table" and "gen_mixer_trans_table"
        defined in "mixer_cap.asm"
        */
        mix_ptr->mix_function = (mix_grp-is_first)*sizeof(unsigned);
        is_first = 0;

        /* Advance over linked list streamings being mixed
        to set "mix_ptr" to first stream of next group to be mixed.
        */
        do
        {
            mix_ptr=mix_ptr->next;
            mix_grp--;
        }while(mix_grp);
    }

    /* Save number of sinks to mix into source */
    src_ptr->mix_list        = mix_list;

    return mix_list;
}


void setup_mixers(GEN_MIXER_OP_DATA *mixer_data)
{
    GEN_MIXER_SINK_INFO  *sink_pptr;
    GEN_MIXER_SINK_INFO  *sink_list;
    GEN_MIXER_SOURCE_INFO **source_pptr;
    GEN_MIXER_SOURCE_INFO *source_list;
    unsigned conn_mask,i,active_sinks;

    /* Assume failure */
    mixer_data->source_list = NULL;
    mixer_data->sink_list   = NULL;

    /*Check Sources connected */
    if(mixer_data->connected_sources != ((1<<mixer_data->max_sources)-1) )
    {
        /* Not all sources connected */
        return;
    }

    /*Check Sinks connected */
    if(mixer_data->connected_sinks==0)
    {
        /* No connected sinks */
        return;
    }
    active_sinks = mixer_data->connected_sinks;
    for(i=0;i<mixer_data->num_streams;i++)
    {
        unsigned stream_mask=mixer_data->streams[i].sink_mask;

        conn_mask = stream_mask & active_sinks;
        if((conn_mask!=stream_mask) && (conn_mask!=0) )
        {
            /* Group is incompplete */
            active_sinks &= ~stream_mask;
            if(active_sinks==0)
            {
                /* No complete sink groups connected */
                return;
            }
        }
    }
    mixer_data->active_sinks = active_sinks;

    /* Setup sinks */
    sink_list = NULL;
    sink_pptr = mixer_data->sinks;
    for(conn_mask=mixer_data->active_sinks;conn_mask!=0;conn_mask>>=1,sink_pptr++)
    {
        if(conn_mask&0x1)
        {
            GEN_MIXER_SINK_INFO *sink_ptr = sink_pptr;

            /* Append to sink list */
            sink_ptr->next = sink_list;
            sink_list      = sink_ptr;
        }
    }
    /* Save sink list */
    mixer_data->sink_list = sink_list;

    /* No connected sinks */
    if(sink_list==NULL)
    {
        return;
    }

    /* Setup sources */
    source_list = NULL;
    source_pptr = mixer_data->sources;
    sink_pptr   = mixer_data->sinks;
    for(conn_mask=mixer_data->connected_sources;conn_mask!=0;conn_mask>>=1,source_pptr++)
    {
        if(conn_mask&0x1)
        {
            GEN_MIXER_SOURCE_INFO *source_ptr = *source_pptr;

            if(setup_mixes(source_ptr,active_sinks,sink_pptr,mixer_data)==NULL)
            {
                /* Sources must have at least one connected sink */
                return;
            }

            /* Append to Source list */
            source_ptr->next = source_list;
            source_list      = source_ptr;
        }
    }
    mixer_data->source_list = source_list;
}

void setup_mixers_gain_change(GEN_MIXER_OP_DATA *mixer_data)
{
    GEN_MIXER_SOURCE_INFO *source_ptr = mixer_data->source_list;

    for(;source_ptr;source_ptr=source_ptr->next)
    {
        setup_mixes(source_ptr,mixer_data->active_sinks,mixer_data->sinks,mixer_data);
    }
}

bool mixer_set_metadata_stream(OPERATOR_DATA *op_data, void *message_data,
                                        unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);

    /* Only values 0 and 1 are valid anything else is out of range and is rejected. */
    if (OPMSG_FIELD_GET(message_data, OPMSG_MIXER_SET_METADATA_STREAM, TRACKS_PRIMARY) > 1)
    {
        return FALSE;
    }

    if (OPMSG_FIELD_GET(message_data, OPMSG_MIXER_SET_METADATA_STREAM, TRACKS_PRIMARY) == 1)
    {
        mixer_data->metadata_tracks_primary = TRUE;
        mixer_data->metadata_stream_idx = mixer_data->primary_stream;
    }
    else
    {
        unsigned stream = OPMSG_FIELD_GET(message_data, OPMSG_MIXER_SET_METADATA_STREAM, STREAM_NUMBER);
        /* Make sure stream is in range for the capability */
        if (stream >= STREAM_MAX || stream == OUTPUT)
        {
            return FALSE;
        }
        mixer_data->metadata_tracks_primary = FALSE;
        mixer_data->metadata_stream_idx = META_IDX_FROM_STREAM(stream);
    }
    return TRUE;
}


void set_metadata(GEN_MIXER_OP_DATA *mixer_data,unsigned term_id,tCbuffer *buffer)
{
    if(!buff_has_metadata(buffer))
    {
        return;
    }

    if(term_id & TERMINAL_SINK_MASK)
    {
        GEN_MIXER_STREAM *stream=mixer_data->streams;
        unsigned          sink_mask = (1<<(term_id&TERMINAL_NUM_MASK));
        unsigned          i;

        /* Identify sink supports metadata */
        mixer_data->sinks_with_metadata |= sink_mask;

        for(i=0;i<mixer_data->num_streams;i++,stream++)
        {
            /* Found the group the sink belongs to */
            if(sink_mask & stream->sink_mask)
            {
                /* If group has no master make, this sink the master of the group */
                if(stream->metadata_ip_buffer==NULL)
                {
                    mixer_data->sink_group_masters |= sink_mask;
                    stream->metadata_ip_buffer  = buffer;
                }
                return;
            }
        }
    }
    else
    {
        unsigned src_mask = (1<<term_id);

        mixer_data->source_with_metadata |= src_mask;

        if(mixer_data->metadata_op_buffer == NULL)
        {
            mixer_data->metadata_op_buffer = buffer;
            mixer_data->source_providing_metadata = src_mask;
        }
    }
}

void clr_metadata(GEN_MIXER_OP_DATA *mixer_data,unsigned term_id)
{
    if(term_id & TERMINAL_SINK_MASK)
    {
        unsigned          i;
        unsigned          sink_mask = (1<<(term_id&TERMINAL_NUM_MASK));
        GEN_MIXER_STREAM *stream=mixer_data->streams;

        /* Remove sink from set with metadata */
        mixer_data->sinks_with_metadata &= ~sink_mask;

        /* Is this sink the master of a group? */
        if((mixer_data->sink_group_masters & sink_mask)==0)
        {
            return;
        }

        /* remove sink as master */
        mixer_data->sink_group_masters &= ~sink_mask;

        /* Search for group this sink is part of */
        for(i=0;i<mixer_data->num_streams;i++,stream++)
        {
            unsigned stream_mask = stream->sink_mask;
            unsigned j;

            if(sink_mask & stream_mask)
            {
                /* Remove metadata */
                stream->metadata_ip_buffer =  NULL;

                for(j=0;j<mixer_data->max_sinks;j++)
                {
                   sink_mask = (1<<j);
                   if(stream_mask&sink_mask)
                   {
                      tCbuffer  *lpbuf = mixer_data->sinks[j].buffer;

                      if(lpbuf && buff_has_metadata(lpbuf))
                      {
                          /* Found available sink.  Make it the group master */
                          mixer_data->sink_group_masters |= sink_mask;
                          stream->metadata_ip_buffer  = lpbuf;
                          return;
                      }
                   }
                }
                return;
            }
        }
    }
    else
    {
        unsigned src_mask = (1<<term_id);

        mixer_data->source_with_metadata &= ~src_mask;

        if(src_mask & mixer_data->source_providing_metadata)
        {
            unsigned i;

            /* Remove metadata */
            mixer_data->metadata_op_buffer = NULL;
            mixer_data->source_providing_metadata = 0;

            /* Look for another source for metadata */
            for(i=0;i<mixer_data->max_sources;i++)
            {
               tCbuffer  *lpbuf = mixer_data->sources[i]->buffer;

               if(lpbuf && buff_has_metadata(lpbuf))
               {
                    mixer_data->source_providing_metadata =(1<<i);
                    mixer_data->metadata_op_buffer = lpbuf;
                    return;
               }

            }
        }
    }
}

tCbuffer *get_metadata(GEN_MIXER_OP_DATA *mixer_data,unsigned term_id)
{
    if(term_id & TERMINAL_SINK_MASK)
    {
        unsigned          i;
        unsigned          sink_mask = (1<<(term_id&TERMINAL_NUM_MASK));
        GEN_MIXER_STREAM *stream=mixer_data->streams;

        for(i=0;i<mixer_data->num_streams;i++,stream++)
        {
            unsigned stream_mask = stream->sink_mask;
            /* Found the group the sink belongs to */
            if(sink_mask & stream_mask)
            {
               return stream->metadata_ip_buffer;
            }
        }
        return NULL;
    }
    else
    {
        return mixer_data->metadata_op_buffer;
    }
}

void handle_metadata(GEN_MIXER_OP_DATA *mixer_data, unsigned proc_amount)
{
    unsigned i,meta_idx=mixer_data->metadata_stream_idx;
    GEN_MIXER_STREAM *stream=mixer_data->streams;

    for(i=0;i<mixer_data->num_streams;i++,stream++)
    {
        /* Active group found */
        /* if(stream->sink_mask & mixer_data->sink_group_masters) */ /* wmst */
        {
            if (i == meta_idx)
            {
                /* Propagate any metadata to the output. Any handling of it's presence or
                * not is handled by the metadata library */
                metadata_strict_transport(stream->metadata_ip_buffer,
                mixer_data->metadata_op_buffer,
                proc_amount * OCTETS_PER_SAMPLE);
            }
            else
            {
                /* This metadata isn't going anywhere so transport it to a NULL
                * buffer. */
                metadata_strict_transport(stream->metadata_ip_buffer,
                                          NULL, proc_amount * OCTETS_PER_SAMPLE);
            }
        }
    }
}


/* mixer obpm support */
bool mixer_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* In the case of this capability, nothing is done for control message. Just follow protocol and ignore any content. */
    return cps_control_setup(message_data, resp_length, resp_data,NULL);
}
bool mixer_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    return FALSE;
}
bool mixer_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
     return FALSE;
}
bool mixer_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* Set the parameter(s). For future proofing, it is using the whole mechanism, although currently there is only one field
     * in opdata structure that is a setable parameter. If later there will be more (ever), must follow contiquously the first field,
     * as commented and instructed in the op data definition. Otherwise consider moving them into a dedicated structure.
     */
    return FALSE;
}

bool mixer_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);

    unsigned* resp = NULL;

    if(!common_obpm_status_helper(message_data,resp_length,resp_data,sizeof(MIXER_STATISTICS) ,&resp))
    {
         return FALSE;
    }

    if(resp)
    {
        unsigned pword1,  pword2;

        pword1 = (unsigned) mixer_data->connected_sources;        /* source_mask */
        pword2 = (unsigned) mixer_data->connected_sinks;          /* sink_mask */
        resp = cpsPack2Words(pword1, pword2, resp);

        pword1 = (unsigned) (mixer_data->transition_count?1:0);   /* Transition State */
        pword1 |= mixer_data->mixer_changed<<1;                   /* mixer changed */
        resp = cpsPack1Word(pword1, resp);
        mixer_data->mixer_changed = 0;
    }

	return TRUE;
}

bool mixer_opmsg_set_buffer_size(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    GEN_MIXER_OP_DATA *opx_data = get_instance_data(op_data);

    if (opmgr_op_is_running(op_data))
    {
        L2_DBG_MSG("mixer_opmsg_set_buffer_size: Cannot set the buffer size for a running operator!");
        return FALSE;
    }

    if  (opx_data->connected_sources != 0)
    {
        L2_DBG_MSG("mixer_opmsg_set_buffer_size: Cannot set the buffer size for an operator with connected outputs!");
        return FALSE;
    }
    /* the mixer capability is only in-place on the main input,
     * but for simplicity don't accept any connected input. */
    if  (opx_data->connected_sinks != 0)
    {
        L2_DBG_MSG("mixer_opmsg_set_buffer_size: Cannot set the buffer size for an operator with connected inputs!");
        return FALSE;
    }

    opx_data->output_buffer_size = OPMSG_FIELD_GET(message_data, OPMSG_COMMON_SET_BUFFER_SIZE, BUFFER_SIZE);

    return TRUE;
}


#define OPMSG_MIXER_GET_CONFIG_RESP_HEADER_LENGTH   3 /* msg_id, resp_state, num_mix */

bool mixer_opmsg_get_config(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    GEN_MIXER_OP_DATA *mixer_data = get_instance_data(op_data);
    GEN_MIXER_SOURCE_INFO *src;
    unsigned mix_num = 0;
    unsigned src_idx, sink_idx;
    unsigned mask;
    unsigned i = 0;
    unsigned mix_idx = 0;
    unsigned *resp;

    for(src_idx=0;src_idx<mixer_data->max_sources;src_idx++)
    {
        src = mixer_data->sources[src_idx];

        if (src != NULL)
        {
            mix_num += pl_one_bit_count(src->configured_sinks);
        }
    }

    /* allocate and fill response data */
    *resp_length = mix_num * 2 + OPMSG_MIXER_GET_CONFIG_RESP_HEADER_LENGTH;
    if ((resp = (unsigned*)xpmalloc((*resp_length) * sizeof(unsigned)))==0)
    {
        return (FALSE);
    }
    *resp_data = (OP_OPMSG_RSP_PAYLOAD *)resp;

    /* The first 3 resp_data */
    resp[i++] = OPMGR_GET_OPMSG_MSG_ID((OP_MSG_REQ *)message_data); /* message ID */
    resp[i++] = OPMSG_RESULT_STATES_NORMAL_STATE;        /* result field */
    resp[i++] = mix_num;                                 /* number of mixes */

    /* The rest of the resp_data record for each mix with two 16bit unsigned values:
            (src_idx<<8) | sink_idx
            (gain_dB60)
    */
    for(src_idx=0;src_idx<mixer_data->max_sources;src_idx++)
    {
        src = mixer_data->sources[src_idx];

        if (src != NULL)
        {
            mask = src->configured_sinks;
            for(sink_idx=0, mix_idx=0; mask; mask>>=1,sink_idx++)
            {
                if(mask&0x1)
                {
                    resp[i++] = (src_idx<<8) | sink_idx;
                    resp[i++] = gain_linear2dB60((signed) src->mixes[mix_idx].current_gain);

                    mix_idx++;
                }
            }
        }
    }

    return TRUE;
}

