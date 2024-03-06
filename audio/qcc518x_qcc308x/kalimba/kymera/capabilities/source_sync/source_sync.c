/****************************************************************************
 * Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  source_sync.c
 * \ingroup  capabilities
 *
 *  src_sync operator
 *
 */
/*
   The capability ensures that time syncronized data is
   aligned from multiple sources.

   Ideally this would just process a fixed amount from
   each source at a fixed period.   This presents several
   issues.
      1) The capability does not know the clock rate of each source.
         To ensure continuous flow the capabaility has no choise
         but to rely on space a the source leting the external source
         throttle the stream.
      2) By block the data into frames at a fixed period a mismatch
         may occur which reduced available MIPs.


   Instead, the capability uses the output space to set its polling
   period.   The result is non-optimal latency as the intermidiate
   buffers fill.  However, it avoids the above issues.

   A minimum polling period is specified to handle stall transitions
   and to somewaht control the data flow.

 */
/****************************************************************************
Include Files
*/

#include "source_sync_defs.h"
#include "capabilities.h"

/****************************************************************************
Private Constant Definitions
*/

/* #define SOSY_CHECK_BLOCK_FILL */

/*****************************************************************************
Private Constant Declarations
*/
/* Null terminated handler table for SOURCE_SYNC capability*/
static const handler_lookup_struct src_sync_handler_table =
{
    src_sync_create,            /* OPCMD_CREATE */
    src_sync_destroy,           /* OPCMD_DESTROY */
    src_sync_start,             /* OPCMD_START */
    src_sync_stop,              /* OPCMD_STOP */
    src_sync_reset,             /* OPCMD_RESET */
    src_sync_connect,           /* OPCMD_CONNECT */
    src_sync_disconnect,        /* OPCMD_DISCONNECT */
    src_sync_buffer_details,    /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,    /* OPCMD_DATA_FORMAT */
    base_op_get_sched_info      /* OPCMD_GET_SCHED_INFO */
};

#ifdef CAPABILITY_DOWNLOAD_BUILD
#define SOURCE_SYNC_ID CAP_ID_DOWNLOAD_SOURCE_SYNC
#else
#define SOURCE_SYNC_ID CAP_ID_SOURCE_SYNC
#endif


const CAPABILITY_DATA source_sync_cap_data =
{
    SOURCE_SYNC_ID,                     /* Capability ID */
    SOURCE_SYNC_SRC_SYNC_VERSION_MAJOR, /* Version information */
    SRC_SYNC_VERSION_MINOR,             /* Version information */
    SRC_SYNC_CAP_MAX_CHANNELS,          /* Max number of sinks/inputs */
    SRC_SYNC_CAP_MAX_CHANNELS,          /* Max number of sources/outputs */
    &src_sync_handler_table,            /* Pointer to entry table */
    src_sync_opmsg_handler_table,       /* Pointer to operator message table */
    src_sync_process_data,              /* Pointer to processing function */
    0,                                  /* Reserved */
    sizeof(SRC_SYNC_OP_DATA)            /* Size of capability-specific per-instance data */
};
#ifndef CAPABILITY_DOWNLOAD_BUILD
MAP_INSTANCE_DATA(CAP_ID_SOURCE_SYNC, SRC_SYNC_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_SOURCE_SYNC, SRC_SYNC_OP_DATA)
#endif

#ifdef SOSY_VERBOSE

#ifdef SOSY_NUMBERED_LOG_MESSAGES
unsigned src_sync_trace_serial = 0;
#ifdef SOSY_LOG_MESSAGE_LIMIT
unsigned src_sync_trace_limit = SOSY_LOG_MESSAGE_LIMIT;
#endif
#endif /* SOSY_NUMBERED_LOG_MESSAGES */

/* Define a debug string for each state */
#define SINK_STATE_NAME(S) AUDIO_LOG_STRING(SOSY_SINK_STATE_NAME_##S, #S);
SRC_SYNC_FOR_EACH_SINK_STATE(SINK_STATE_NAME)
#undef SINK_STATE_NAME

static const char* src_sync_sink_state_names[SRC_SYNC_NUM_SINK_STATES+1] =
{
#define SINK_STATE_NAME_TABLE(S) SOSY_SINK_STATE_NAME_##S,
    SRC_SYNC_FOR_EACH_SINK_STATE(SINK_STATE_NAME_TABLE)
#undef SINK_STATE_NAME_TABLE
    NULL
};
#else /* SOSY_VERBOSE */

#define SINK_STATE_NAME_CHARS 28

#ifdef AUDIO_LOG_STRING_ATTR
AUDIO_LOG_STRING_ATTR
#endif
static const char src_sync_sink_state_names
    [SRC_SYNC_NUM_SINK_STATES+1]
    [SINK_STATE_NAME_CHARS] =
{
#define SINK_STATE_NAME(S) #S,
SRC_SYNC_FOR_EACH_SINK_STATE(SINK_STATE_NAME)
#undef SINK_STATE_NAME
    { 0 }
};

#endif /* SOSY_VERBOSE */


/****************************************************************************
Private Function Declarations
*/
/* ******************************* Helper functions ************************************ */

static inline SRC_SYNC_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (SRC_SYNC_OP_DATA *) base_op_get_instance_data(op_data);
}

void src_sync_cleanup(SRC_SYNC_OP_DATA *op_extra_data)
{
    unsigned i;

    /* Kill timer Task */
    timer_cancel_event_atomic(&op_extra_data->kick_id);

    src_sync_free_buffer_histories(op_extra_data);

    op_extra_data->sinks_connected = 0;
    op_extra_data->sources_connected = 0;
    op_extra_data->source_group_mask = 0;
    op_extra_data->sink_group_mask = 0;

    pfree(op_extra_data->sink_groups);
    op_extra_data->sink_groups=NULL;

    SRC_SYNC_SOURCE_GROUP* src_grp;
    for ( src_grp = op_extra_data->source_groups;
          src_grp != NULL;
          src_grp = next_source_group(src_grp))
    {
        pfree(src_grp->metadata_dest.eof_tag);
    }
    pfree(op_extra_data->source_groups);
    op_extra_data->source_groups = NULL;

    for(i=0;i<SRC_SYNC_CAP_MAX_CHANNELS;i++)
    {
        pfree(op_extra_data->sinks[i]);
        op_extra_data->sinks[i] = NULL;

        pfree(op_extra_data->sources[i]);
        op_extra_data->sources[i] = NULL;
    }
}

SRC_SYNC_SINK_ENTRY *src_sync_alloc_sink(SRC_SYNC_OP_DATA   *op_extra_data,unsigned term_idx)
{
    SRC_SYNC_SINK_ENTRY     *sink_data = op_extra_data->sinks[term_idx];

    patch_fn_shared(src_sync);

    if (sink_data == NULL)
    {
        sink_data = xzpnew(SRC_SYNC_SINK_ENTRY);
        if (sink_data != NULL)
        {
            sink_data->common.idx          = term_idx;
            op_extra_data->sinks[term_idx] = sink_data;
        }
    }
    return sink_data;
}

SRC_SYNC_SOURCE_ENTRY *src_sync_alloc_source(SRC_SYNC_OP_DATA *op_extra_data,unsigned term_idx)
{
    SRC_SYNC_SOURCE_ENTRY   *src_data  = op_extra_data->sources[term_idx];

    patch_fn_shared(src_sync);

    if (src_data == NULL)
    {
        src_data = xzpnew(SRC_SYNC_SOURCE_ENTRY);
        if (src_data != NULL)
        {
            src_data->common.idx             = term_idx;
            op_extra_data->sources[term_idx] = src_data;
            op_extra_data->src_route_switch_pending_mask &=
                    ~ (1 << term_idx);
        }
    }

    return src_data;
}

/**
 * A buffer history structure is used for a heuristic to tell when
 * the downstream buffers have been filled. Here this is done by checking
 * that after all kicks during an approximate kick period the output
 * buffers have not been emptied. Thus the number of output buffer levels
 * which need to be recorded is the estimated max number of kicks to this
 * operator during a kick period. Assume that every source/sink group
 * is connected to a different operator and thus the operator can
 * receive an independent series of kicks for every group.
 */
bool src_sync_alloc_buffer_histories(
        OPERATOR_DATA *op_data, SRC_SYNC_SINK_GROUP* sink_groups,
        SRC_SYNC_SOURCE_GROUP* source_groups)
{
    SRC_SYNC_OP_DATA *op_extra_data = get_instance_data(op_data);
    unsigned num_entries =
            src_sync_get_num_groups(&sink_groups->common)
            + src_sync_get_num_groups(&source_groups->common);

    PL_ASSERT(opmgr_op_is_processing_suspended(op_data));
    if (! src_sync_alloc_buffer_history( &op_extra_data->source_buffer_history,
                                         num_entries))
    {
        return FALSE;
    }

    return TRUE;
}

void src_sync_free_buffer_histories(SRC_SYNC_OP_DATA *op_extra_data)
{
    src_sync_free_buffer_history(&op_extra_data->source_buffer_history);
}


SRC_SYNC_TERMINAL_GROUP* src_sync_find_group(
        SRC_SYNC_TERMINAL_GROUP* groups, unsigned channel_num)
{
    unsigned channel_bit = 1 << channel_num;
    SRC_SYNC_TERMINAL_GROUP* grp;

    for ( grp = groups; grp != NULL; grp = grp->next )
    {
        if ((grp->channel_mask & channel_bit) != 0)
        {
            return grp;
        }
    }
    return NULL;
}

SRC_SYNC_SINK_GROUP* src_sync_find_sink_group(
        SRC_SYNC_OP_DATA *op_extra_data, unsigned channel_num)
{
    return cast_sink_group(
               src_sync_find_group( &(op_extra_data->sink_groups->common),
                   channel_num) );
}

SRC_SYNC_SOURCE_GROUP* src_sync_find_source_group(
        SRC_SYNC_OP_DATA *op_extra_data, unsigned channel_num)
{
    return cast_source_group(
               src_sync_find_group( &(op_extra_data->source_groups->common),
                   channel_num) );
}

void src_sync_notify_sink_state_change(
        SRC_SYNC_OP_DATA* op_extra_data, SRC_SYNC_SINK_GROUP* sink_grp,
        src_sync_sink_state new_state)
{
    bool prev_stalled, new_stalled;
    unsigned stall_changed_msg[OPMSG_SOURCE_SYNC_STALL_CHANGED_WORD_SIZE];
    TIME now;

    prev_stalled = sink_grp->stall_state == SRC_SYNC_SINK_STALLED;
    new_stalled = new_state == SRC_SYNC_SINK_STALLED;
    if (prev_stalled == new_stalled)
    {
        return;
    }

    now = time_get_time();

    OPMSG_CREATION_FIELD_SET(stall_changed_msg,
                             OPMSG_SOURCE_SYNC_STALL_CHANGED, SINK_GROUP,
                             sink_grp->common.idx);
    OPMSG_CREATION_FIELD_SET(stall_changed_msg,
                             OPMSG_SOURCE_SYNC_STALL_CHANGED, STALL_STATE,
                             new_stalled);
    OPMSG_CREATION_FIELD_SET32(stall_changed_msg,
                               OPMSG_SOURCE_SYNC_STALL_CHANGED, TIMESTAMP,
                               now);

    common_send_unsolicited_message(
            op_extra_data->op_data,
            (unsigned)OPMSG_REPLY_ID_SOURCE_SYNC_STALL_CHANGED,
            OPMSG_SOURCE_SYNC_STALL_CHANGED_WORD_SIZE,
            (unsigned*)stall_changed_msg);
}

void src_sync_set_sink_state( SRC_SYNC_OP_DATA *op_extra_data,
                              SRC_SYNC_SINK_GROUP* sink_grp,
                              src_sync_sink_state new_state )
{
    patch_fn_shared(src_sync);

    if (sink_grp->stall_state != new_state)
    {
#ifdef SOSY_VERBOSE
        PL_ASSERT(new_state < SRC_SYNC_NUM_SINK_STATES);
        SOSY_MSG4( SRC_SYNC_TRACE_SINK_STATE,
                   "t %d sink_g%d %s -> %s",
                   time_get_time(),
                   sink_grp->common.idx,
                   src_sync_sink_state_names[sink_grp->stall_state],
                   src_sync_sink_state_names[new_state] );
#else
        if ( (pl_min(sink_grp->stall_state, new_state) < SRC_SYNC_SINK_FLOWING) ||
             (pl_max(sink_grp->stall_state, new_state) > SRC_SYNC_SINK_PENDING) )
        {
            L2_DBG_MSG4( "src_sync t %d sink_g%d %s -> %s",
                         time_get_time(),
                         sink_grp->common.idx,
                         src_sync_sink_state_names[sink_grp->stall_state],
                         src_sync_sink_state_names[new_state] );
        }
#endif
#ifdef INSTALL_TIMING_TRACE
        opmgr_record_timing_trace_op_term_event(
            op_extra_data->id | STREAM_EP_OP_SINK | sink_grp->common.terminals->idx,
            SRC_SYNC_TIMING_EVENT_SINK_STATE,
            sink_grp->stall_state, new_state);
#endif
        if (op_extra_data->enable_sink_state_notification)
        {
            src_sync_notify_sink_state_change(op_extra_data, sink_grp, new_state);
        }
    }

    sink_grp->stall_state = new_state;
}

bool src_sync_connect_metadata_buffer( SRC_SYNC_TERMINAL_ENTRY* entry,
                                       SRC_SYNC_TERMINAL_GROUP* group)
{
    if (group->metadata_enabled
        && (group->metadata_buffer == NULL)
        && buff_has_metadata(entry->buffer))
    {
        group->metadata_buffer = entry->buffer;
        return TRUE;
    }
    return FALSE;
}

void src_sync_find_alternate_metadata_buffer(
        unsigned connected, SRC_SYNC_TERMINAL_ENTRY* entry,
        SRC_SYNC_TERMINAL_ENTRY** all_entries)
{
    SRC_SYNC_TERMINAL_GROUP* grp;
    unsigned ch;

    grp = entry->group;
    if (grp != NULL
        && grp->metadata_buffer == entry->buffer)
    {
        unsigned channels = grp->channel_mask & connected;
        bool found_alternative = FALSE;

        /* Try to find a replacement in the same group */
        for (ch = 0; ch < SRC_SYNC_CAP_MAX_CHANNELS; ++ ch)
        {
            if ((ch != entry->idx) && ((channels & (1 << ch)) != 0))
            {
                SRC_SYNC_TERMINAL_ENTRY* alt = all_entries[ch];

                /* Bit set in the connected bitmask should guarantee
                 * that the alt != NULL and alt->buffer != NULL
                 */
                if (buff_has_metadata(alt->buffer))
                {
                    grp->metadata_buffer = alt->buffer;
                    found_alternative = TRUE;
                    break;
                }
            }
        }
        if (!found_alternative)
        {
            grp->metadata_buffer = NULL;
        }
    }
}

bool src_sync_valid_route(const SRC_SYNC_ROUTE_ENTRY* route)
{
    /* It is not necessary to check route != NULL; the argument
     * is always the address of the field current_route or switch_route
     */
    return route->is_valid && (route->sink != NULL);
}

unsigned src_sync_get_num_groups(const SRC_SYNC_TERMINAL_GROUP* groups)
{
    unsigned count = 0;
    while (groups != NULL)
    {
        count += 1;
        groups = groups->next;
    }
    return count;
}

/* ********************************** API functions ************************************* */


bool src_sync_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SRC_SYNC_OP_DATA *op_extra_data = get_instance_data(op_data);

    patch_fn_shared(src_sync);

    SOSY_MSG1( SRC_SYNC_TRACE_ALWAYS, "0x%04x create", base_op_get_ext_op_id(op_data));

    /* call base_op create, which also allocates and fills response message */
    if (!base_op_create_lite(op_data, response_data))
    {
        return FALSE;
    }

    /* Initialize Data */
    op_extra_data->id = (uint16) base_op_get_ext_op_id(op_data);
    op_extra_data->Dirty_flag = SRC_SYNC_CAP_CHANNELS_MASK;
    op_extra_data->stat_sink_stalled = 0;
    op_extra_data->stat_sink_stall_occurred = 0;
    op_extra_data->kick_id = TIMER_ID_INVALID;
    op_extra_data->buffer_size = SRC_SYNC_DEFAULT_BUFFER_SIZE_CONFIG;
    op_extra_data->sample_rate = stream_if_get_system_sampling_rate();
    op_extra_data->src_route_switch_pending_mask = 0;
    op_extra_data->op_data = op_data;
#ifdef SOSY_VERBOSE
    op_extra_data->trace_enable = SRC_SYNC_DEFAULT_TRACE_ENABLE;
#ifdef SOSY_NUMBERED_LOG_MESSAGES
    src_sync_trace_serial = 0;
#endif /* SOSY_NUMBERED_LOG_MESSAGES */
#endif /* SOSY_VERBOSE */

    /* Setup CPS */
    if (!cpsInitParameters( &op_extra_data->parms_def,
                            (unsigned*)SOURCE_SYNC_GetDefaults(base_op_get_cap_id(op_data)),
                            (unsigned*)&op_extra_data->cur_params,
                            sizeof(SOURCE_SYNC_PARAMETERS)))
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    src_sync_trace_params(op_extra_data);

    return TRUE;
}

bool src_sync_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SRC_SYNC_OP_DATA *op_extra_data = get_instance_data(op_data);

    patch_fn_shared(src_sync);

    SOSY_MSG1( SRC_SYNC_TRACE_ALWAYS, "0x%04x destroy", op_extra_data->id);

    /* check that we are not trying to destroy a running operator */
    if (opmgr_op_is_running(op_data))
    {
        /* We can't destroy a running operator. */
        return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
    }
    else
    {
        /* set internal capability state variable to "not_created" */
        src_sync_cleanup(op_extra_data);
        /* call base_op destroy that creates and fills response message, too */
        return base_op_destroy_lite(op_data, response_data);
    }
}

bool src_sync_start( OPERATOR_DATA *op_data, void *message_data,
                     unsigned *response_id, void **response_data)
{
    SRC_SYNC_OP_DATA *op_extra_data = get_instance_data(op_data);
    bool start_success;

    patch_fn_shared(src_sync);

    /* do something only if the current state is not "RUNNING" */
    if (!opmgr_op_is_running(op_data))
    {
        start_success = base_op_start( op_data, message_data,
                                       response_id,response_data);
        if (start_success)
        {
            /* Re-Init routes */
            src_sync_suspend_processing(op_data);

            /* Startup values */
            op_extra_data->time_stamp = time_get_time();
            op_extra_data->system_kick_period = rate_signed_usec_to_second_interval(stream_if_get_system_kick_period());
            op_extra_data->est_latency = src_sync_get_max_period(op_extra_data);
            op_extra_data->primary_sp_adjust = 0;

            src_sync_resume_processing(op_data);
        }
    }
    else
    {
        start_success = base_op_start( op_data,message_data,
                                       response_id,response_data);
    }
    SOSY_MSG2( SRC_SYNC_TRACE_ALWAYS, "0x%04x start %d", op_extra_data->id, start_success);
    return start_success;
}

bool src_sync_stop_reset(OPERATOR_DATA *op_data,void **response_data)
{
    SRC_SYNC_OP_DATA *op_extra_data = get_instance_data(op_data);

    timer_cancel_event_atomic(&op_extra_data->kick_id);

    if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        return(FALSE);
    }
    return TRUE;
}

bool src_sync_stop(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
#ifdef SOSY_VERBOSE
    SRC_SYNC_OP_DATA *op_extra_data = get_instance_data(op_data);
#endif /* SOSY_VERBOSE */
    SOSY_MSG1(SRC_SYNC_TRACE_ALWAYS, "0x%04x stop", base_op_get_ext_op_id(op_data));

    patch_fn_shared(src_sync);

    /* Setup Response to Stop Request.   Assume Failure*/
    return(src_sync_stop_reset(op_data,response_data));
}

bool src_sync_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
#ifdef SOSY_VERBOSE
    SRC_SYNC_OP_DATA *op_extra_data = get_instance_data(op_data);
#endif /* SOSY_VERBOSE */
    SOSY_MSG1(SRC_SYNC_TRACE_ALWAYS, "0x%04x reset", base_op_get_ext_op_id(op_data));

    patch_fn_shared(src_sync);

    /* Setup Response to Reset Request.   Assume Failure*/
    return(src_sync_stop_reset(op_data,response_data));
}


bool src_sync_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SRC_SYNC_OP_DATA *op_extra_data = get_instance_data(op_data);
    unsigned terminal_num;
    unsigned terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    unsigned terminal_num_mask;

    patch_fn_shared(src_sync);

    terminal_num = terminal_id & TERMINAL_NUM_MASK;

    if (!base_op_connect(op_data, message_data, response_id, response_data))
    {
        L2_DBG_MSG1("src_sync 0x%04x connect FAILED base_op_connect failed",
                    op_extra_data->id);
        return FALSE;
    }

    if ( terminal_num >= SRC_SYNC_CAP_MAX_CHANNELS )
    {
        /* invalid terminal id */
        L2_DBG_MSG2("src_sync 0x%04x connect REJECTED invalid terminal %d",
                    op_extra_data->id, terminal_num);
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }
    terminal_num_mask = (1 << terminal_num);

    if ((terminal_id & TERMINAL_SINK_MASK) != 0)
    {
        SRC_SYNC_SINK_ENTRY *sink_data;
        SRC_SYNC_SINK_GROUP* sink_grp;

        if ((op_extra_data->sink_group_mask & terminal_num_mask) == 0)
        {
            L2_DBG_MSG2("src_sync 0x%04x connect REJECTED sink %d: no group",
                        op_extra_data->id, terminal_num);
            base_op_change_response_status(response_data, STATUS_CMD_FAILED);
            return TRUE;
        }

        sink_grp = src_sync_find_sink_group(op_extra_data, terminal_num);
        if (sink_grp == NULL)
        {
            /* Must not happen */
            L2_DBG_MSG2("src_sync 0x%04x connect REJECTED sink %d no group",
                        op_extra_data->id, terminal_num);
            base_op_change_response_status(response_data, STATUS_CMD_FAILED);
            return TRUE;
        }

        sink_data = op_extra_data->sinks[terminal_num];
        if (sink_data == NULL)
        {
            /* shouldn't happen */
            L2_DBG_MSG2("src_sync 0x%04x connect REJECTED sink %d no entry",
                        op_extra_data->id, terminal_num);
            base_op_change_response_status(response_data, STATUS_CMD_FAILED);
            return TRUE;
        }

        src_sync_suspend_processing(op_data);

        sink_data->common.buffer = OPMGR_GET_OP_CONNECT_BUFFER(message_data);
        sink_data->input_buffer = sink_data->common.buffer;
        sink_data->common.group = &sink_grp->common;

        if (src_sync_connect_metadata_buffer(&sink_data->common,
                                             &sink_grp->common))
        {
            sink_grp->metadata_input_buffer = sink_grp->common.metadata_buffer;

            SOSY_MSG4( SRC_SYNC_TRACE_ALWAYS, "0x%04x connect sink %d: "
                        "using 0x%08x as metadata buffer on grp %d",
                        op_extra_data->id, terminal_num,
                        (unsigned)(uintptr_t)sink_data->common.buffer,
                        sink_grp->common.idx );
        }

        op_extra_data->sinks_connected |= terminal_num_mask;
        if (! sink_grp->common.connected)
        {
            sink_grp->common.connected =
                    (op_extra_data->sinks_connected & sink_grp->common.channel_mask)
                    == sink_grp->common.channel_mask;

            if (sink_grp->common.connected && sink_grp->rate_adjust_enable)
            {
                src_sync_rm_init(op_extra_data, sink_grp);
            }
        }

        SOSY_MSG4( SRC_SYNC_TRACE_ALWAYS, "0x%04x connect sink %d: "
                    "grp %d grp_conn %d",
                    op_extra_data->id, terminal_num,
                    sink_grp->common.idx, sink_grp->common.connected);

        src_sync_resume_processing(op_data);
    }
    else
    {
        SRC_SYNC_SOURCE_ENTRY *src_data;
        SRC_SYNC_SOURCE_GROUP* source_grp;

        if ((op_extra_data->source_group_mask & terminal_num_mask) == 0)
        {
            L2_DBG_MSG2("src_sync 0x%04x connect REJECTED source %d: no group",
                        op_extra_data->id, terminal_num);
            base_op_change_response_status(response_data, STATUS_CMD_FAILED);
            return TRUE;
        }

        source_grp = src_sync_find_source_group(op_extra_data, terminal_num);
        if (source_grp == NULL)
        {
            /* Must not happen */
            L2_DBG_MSG2("src_sync 0x%04x connect REJECTED source %d no group",
                        op_extra_data->id, terminal_num);
            base_op_change_response_status(response_data, STATUS_CMD_FAILED);
            return TRUE;
        }

        src_data = op_extra_data->sources[terminal_num];
        if (src_data == NULL)
        {
            /* Must not happen */
            L2_DBG_MSG2("src_sync 0x%04x connect REJECTED source %d no entry",
                        op_extra_data->id, terminal_num);
            base_op_change_response_status(response_data, STATUS_CMD_FAILED);
            return TRUE;
        }

        src_sync_suspend_processing(op_data);

        src_data->common.buffer = OPMGR_GET_OP_CONNECT_BUFFER(message_data);
        src_data->common.group = &source_grp->common;

        op_extra_data->sources_connected |= terminal_num_mask;
        source_grp->common.connected =
                (op_extra_data->sources_connected & source_grp->common.channel_mask)
                == source_grp->common.channel_mask;

        SOSY_MSG4( SRC_SYNC_TRACE_ALWAYS, "0x%04x connect source %d: "
                   "grp %d grp_conn %d",
                   op_extra_data->id, terminal_num,
                   source_grp->common.idx,
                   source_grp->common.connected);

        if (src_sync_connect_metadata_buffer(&src_data->common,
                                             &source_grp->common))
        {
            SOSY_MSG4( SRC_SYNC_TRACE_ALWAYS, "0x%04x connect source %d: "
                       "using 0x%08x as metadata buffer on grp %d",
                       op_extra_data->id, terminal_num,
                       (unsigned)(uintptr_t)src_data->common.buffer,
                       source_grp->common.idx );
            cbuffer_set_usable_octets(source_grp->common.metadata_buffer,
                get_octets_per_word(AUDIO_DATA_FORMAT_FIXP));
        }

        src_sync_resume_processing(op_data);
    }

    return TRUE;
}


bool src_sync_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SRC_SYNC_OP_DATA *op_extra_data = get_instance_data(op_data);
    unsigned terminal_num;
    unsigned terminal_id = OPMGR_GET_OP_DISCONNECT_TERMINAL_ID(message_data);

    patch_fn_shared(src_sync);

    terminal_num = terminal_id & TERMINAL_NUM_MASK;

    if (!base_op_disconnect(op_data, message_data, response_id, response_data))
    {
        L2_DBG_MSG1("src_sync 0x%04x disconnect FAILED base_op_disconnect failed",
                    op_extra_data->id);
        return FALSE;
    }
    if ( terminal_num >= SRC_SYNC_CAP_MAX_CHANNELS )
    {
        L2_DBG_MSG2("src_sync 0x%04x disconnect REJECTED invalid terminal %d",
                    op_extra_data->id, terminal_num);
        /* invalid terminal id */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    if ((terminal_id & TERMINAL_SINK_MASK) != 0)
    {
        SRC_SYNC_SINK_ENTRY *sink_data = op_extra_data->sinks[terminal_num];
        if ((sink_data != NULL) && (sink_data->common.buffer != NULL))
        {
            SRC_SYNC_SINK_GROUP* sink_grp;

            src_sync_suspend_processing(op_data);

            op_extra_data->sinks_connected &= ~(1<<terminal_num);

            /* In casting the third argument, each array element is cast
             * from SRC_SYNC_SINK_ENTRY* to SRC_SYNC_TERMINAL_ENTRY*,
             * to point to its first member. This is valid even
             * if the entry is NULL.
             */
            src_sync_find_alternate_metadata_buffer(
                    op_extra_data->sinks_connected, &sink_data->common,
                    (SRC_SYNC_TERMINAL_ENTRY**)(op_extra_data->sinks));

            sink_data->common.buffer = NULL;
            sink_data->input_buffer = NULL;

            /* set_route, then disconnect, could lead
             * to a NULL group pointer */
            sink_grp = sink_group_from_entry(sink_data);
            if (sink_grp != NULL)
            {
                sink_grp->metadata_input_buffer = sink_grp->common.metadata_buffer;

                if (sink_grp->common.connected && sink_grp->rate_adjust_enable)
                {
                    src_sync_rm_fini(op_extra_data, sink_grp);
                }
                sink_grp->common.connected = FALSE;
            }
            src_sync_check_primary_clock_connected(op_extra_data);

            src_sync_resume_processing(op_data);

            SOSY_MSG2( SRC_SYNC_TRACE_ALWAYS, "0x%04x disconnect sink %d",
                       op_extra_data->id, terminal_num);
        }
        else
        {
            L2_DBG_MSG2("src_sync 0x%04x disconnect REJECTED sink %d not connected",
                        op_extra_data->id, terminal_num);
            /* not connected */
            base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        }
    }
    else
    {
        SRC_SYNC_SOURCE_ENTRY *src_data = op_extra_data->sources[terminal_num];
        if ((src_data != NULL) && (src_data->common.buffer != NULL))
        {
            src_sync_suspend_processing(op_data);

            op_extra_data->sources_connected &= ~(1<<terminal_num);

            /* In casting the third argument, each array element is cast
             * from SRC_SYNC_SOURCE_ENTRY* to SRC_SYNC_TERMINAL_ENTRY*,
             * to point to its first member. This is valid even
             * if the entry is NULL.
             */
            src_sync_find_alternate_metadata_buffer(
                    op_extra_data->sinks_connected, &src_data->common,
                    (SRC_SYNC_TERMINAL_ENTRY**)(op_extra_data->sources));

            src_data->common.buffer = NULL;

            /* set_route, then disconnect, could lead
             * to a NULL group pointer */
            if (src_data->common.group != NULL)
            {
                src_data->common.group->connected = FALSE;
            }
            src_sync_check_primary_clock_connected(op_extra_data);

            src_sync_resume_processing(op_data);

            SOSY_MSG2( SRC_SYNC_TRACE_ALWAYS, "0x%04x disconnect source %d",
                       op_extra_data->id, terminal_num);
        }
        else
        {
            L2_DBG_MSG2("src_sync 0x%04x disconnect REJECTED source %d not connected",
                        op_extra_data->id, terminal_num);
            /* not connected */
            base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        }
    }

    return TRUE;
}


bool src_sync_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SRC_SYNC_OP_DATA *op_extra_data = get_instance_data(op_data);
    unsigned terminal_id;
    unsigned terminal_num;
    SRC_SYNC_TERMINAL_ENTRY* terminal;
    int buffer_size_config;
    unsigned config_buffer_w;

    patch_fn_shared(src_sync);

    if (!base_op_buffer_details_lite(op_data, response_data))
    {
        L2_DBG_MSG1("src_sync 0x%04x buffer_details FAILED base_op_buffer_details failed",
                    op_extra_data->id);
        return FALSE;
    }

    terminal_id = OPMGR_GET_OP_BUF_DETAILS_TERMINAL_ID(message_data);
    terminal_num = terminal_id & TERMINAL_NUM_MASK;

    if (terminal_num >= SRC_SYNC_CAP_MAX_CHANNELS)
    {
        base_op_change_response_status(response_data, STATUS_INVALID_CMD_PARAMS);

        L2_DBG_MSG2("src_sync 0x%04x buffer_details REJECTED invalid terminal 0x%06x",
                    op_extra_data->id, terminal_id);
        return TRUE;
    }

    ((OP_BUF_DETAILS_RSP*)*response_data)->runs_in_place = FALSE;

    /* If an input/output connection is already present and has metadata then
     * we are obliged to return that buffer so that metadata can be shared
     * between channels. */

    ((OP_BUF_DETAILS_RSP*)*response_data)->supports_metadata = FALSE;

    if ((terminal_id & TERMINAL_SINK_MASK) == TERMINAL_SINK_MASK)
    {
        /* Taking the address of the first struct member is a type-safer
         * pointer cast, even if the pointer is NULL */
        terminal = &(op_extra_data->sinks[terminal_num]->common);
    }
    else
    {
        /* Taking the address of the first struct member is a type-safer
         * pointer cast, even if the pointer is NULL */
        terminal = &(op_extra_data->sources[terminal_num]->common);
    }

    buffer_size_config = op_extra_data->buffer_size;

    if ((terminal != NULL)
        && (terminal->group != NULL))
    {
        ((OP_BUF_DETAILS_RSP*)*response_data)->supports_metadata =
                terminal->group->metadata_enabled;
        ((OP_BUF_DETAILS_RSP*)*response_data)->metadata_buffer =
                terminal->group->metadata_buffer;
        if (terminal->group->has_buffer_size)
        {
            buffer_size_config = terminal->group->buffer_size;
        }
    }

    if (buffer_size_config > 0)
    {
        config_buffer_w = buffer_size_config;
    }
    else
    {
        unsigned config_buffer_us, buffer_kick_mult;
        if (buffer_size_config == 0)
        {
            buffer_kick_mult = SRC_SYNC_AUTO_BUFFER_SS_PERIOD_MULT;
        }
        else
        {
            buffer_kick_mult = - buffer_size_config;
        }
        config_buffer_us = frac_mult(buffer_kick_mult, (unsigned)stream_if_get_system_kick_period() << 5);
        config_buffer_w = frac_mult(src_sync_usec_to_sec_frac(config_buffer_us), op_extra_data->sample_rate);
    }

    ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = config_buffer_w;

    SOSY_MSG4( SRC_SYNC_TRACE_ALWAYS, "0x%04x buffer_details 0x%06x bsz_w %d "
                "supp_md %d",
                op_extra_data->id, terminal_id,
                ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size,
                ((OP_BUF_DETAILS_RSP*)*response_data)->supports_metadata);

    return TRUE;
}


/**
 * \brief Calculate cached values of commonly used expressions
 */
void src_sync_update_derived(SRC_SYNC_OP_DATA *op_extra_data)
{
    op_extra_data->sample_period = rate_sample_rate_to_sample_period(op_extra_data->sample_rate);
    op_extra_data->system_kick_period =
            rate_signed_usec_to_second_interval(stream_if_get_system_kick_period());
    op_extra_data->ss_period_w =
            rate_second_interval_to_samples(src_sync_get_period(op_extra_data),
                                            op_extra_data->sample_rate);
}

/**
 * \brief Check whether parameters allow a sink to enter RECOVERING_RESTARTING
 * state. That state is intended to prime downstream buffers with a predictable
 * amount of latency (MAX_LATENCY). If all the target latency can be contained
 * in the directly connected output buffers, go to FLOWING state instead.
 * The output buffer size condition is simplified to use the common output
 * buffer space, saved in max_space_t, rather than only the output buffers
 * affecting a sink group.
 */
void src_sync_set_sink_recovering_restarting(SRC_SYNC_OP_DATA *op_extra_data,
                                             SRC_SYNC_SINK_GROUP *sink_grp)
{
    RATE_SECOND_INTERVAL max_space_t =
            rate_samples_to_second_interval(op_extra_data->max_space_w,
                                            op_extra_data->sample_period);
    if (max_space_t >= src_sync_get_max_latency(op_extra_data))
    {
        src_sync_set_sink_state(op_extra_data, sink_grp,
                                SRC_SYNC_SINK_FLOWING);
    }
    else
    {
        sink_grp->inserted_silence_words = 0;
        src_sync_set_sink_state(op_extra_data, sink_grp,
                                SRC_SYNC_SINK_RECOVERING_RESTARTING );
    }
}

void src_sync_refresh_sink_list(SRC_SYNC_OP_DATA *op_extra_data)
{
    unsigned i;
    SRC_SYNC_SINK_ENTRY  **lpp_sinks;
    SRC_SYNC_SINK_GROUP  *group;
    bool have_rm_master = FALSE;

    for (group = op_extra_data->sink_groups;
         group != NULL;
         group = next_sink_group(group))
    {
        SRC_SYNC_TERMINAL_ENTRY** ppentry = &(group->common.terminals);
        unsigned channels = group->common.channel_mask;

        for (i = 0, lpp_sinks = op_extra_data->sinks;
             i < SRC_SYNC_CAP_MAX_CHANNELS;
             i++, lpp_sinks++)
        {
            SRC_SYNC_SINK_ENTRY  *sink = *lpp_sinks;
            unsigned ch_bit = 1 << i;

            if ( ((channels & ch_bit) != 0)
                 && (sink != NULL) )
            {
                /* Add sink to list */
                *ppentry = &sink->common;
                ppentry = &(*ppentry)->next;

                SOSY_MSG2( SRC_SYNC_TRACE_REFRESH,
                           "rsil add sink %d to grp %d list",
                           sink->common.idx, group->common.idx );

                sink->input_buffer = src_sync_get_input_buffer(group, sink);
            }
        }
        *ppentry = NULL;

        if (! group->common.connected)
        {
            src_sync_set_sink_state( op_extra_data, group,
                                     SRC_SYNC_SINK_NOT_CONNECTED);
        }

        /*
         * Timestamped rate references:
         * Build up list of measurement contexts
         * Determine which sink group becomes timestamped rate master
         * i.e. provides tags from which to derive the measurement
         */
        else if (! have_rm_master && group->common.connected
                && group->common.metadata_enabled
                && (group->common.metadata_buffer != NULL))
        {
            group->ts_rate_master = TRUE;
            have_rm_master = TRUE;

            src_sync_ra_set_primary_rate(op_extra_data, op_extra_data->sample_rate);

            SOSY_MSG1(SRC_SYNC_TRACE_RATE_MATCH, "rm sink_g%d becomes ts_rate_master",
                      group->common.idx);
        }
    }
}

void src_sync_refresh_source_list(SRC_SYNC_OP_DATA *op_extra_data)
{
    unsigned src_kick;
    unsigned min_size_w;
    SRC_SYNC_SOURCE_ENTRY  **lpp_sources;
    SRC_SYNC_SOURCE_GROUP* src_grp;

    /* collect minimum of connected source buffer sizes */
    min_size_w = MAXINT;
    /* collect bitmap of sources to kick */
    src_kick = 0;

    /*
     * For each source group, link the connected and routed sources
     * for quick traversal. These are the sources for which output
     * needs to be produced.
     *
     * It is not necessary for the routed sinks to be connected;
     * silence will be inserted for unconnected routed sinks.
     */
    for ( src_grp = op_extra_data->source_groups;
          src_grp != NULL;
          src_grp = next_source_group(src_grp) )
    {
        SRC_SYNC_TERMINAL_ENTRY** ppentry = &(src_grp->common.terminals);
        unsigned i;
        unsigned channels = src_grp->common.channel_mask;

        for ( i = 0, lpp_sources = op_extra_data->sources;
              i < SRC_SYNC_CAP_MAX_CHANNELS;
              i++, lpp_sources++, channels >>= 1 )
        {
            SRC_SYNC_SOURCE_ENTRY  *src_ptr = *lpp_sources;
            if ( ((channels & 1) != 0)
                 && (src_ptr != NULL)
                 && (src_ptr->common.buffer != NULL) )
            {
                SRC_SYNC_ROUTE_ENTRY* route = &src_ptr->current_route;

                if (src_sync_valid_route(route))
                {
                    unsigned buffer_size_words;

                    /* link into the source group's terminal list */
                    *ppentry = &src_ptr->common;
                    ppentry = &(*ppentry)->next;

                    /* Setup Source */
                    src_kick |= (1<<src_ptr->common.idx);

                    /* Get minimum buffer size (minus two) */
                    buffer_size_words =
                            cbuffer_get_size_in_words(src_ptr->common.buffer);
                    buffer_size_words -= 2;

                    min_size_w = pl_min(min_size_w, buffer_size_words);

                    SOSY_MSG3( SRC_SYNC_TRACE_REFRESH,
                               "rsol grp %d src %d bsw %d",
                               src_grp->common.idx, i, buffer_size_words);
                }
            }
        }
        *ppentry = NULL;
    }

    SOSY_MSG2( SRC_SYNC_TRACE_REFRESH,
               "rsol max_space_w %d fwd_kick 0x%06x",
               min_size_w, src_kick);

    /* Save State Info */
    op_extra_data->max_space_w = min_size_w;
    op_extra_data->forward_kicks = src_kick;
}


/* ************************************* Data processing-related functions and wrappers **********************************/
void src_sync_timer_task(void *kick_object)
{
    OPERATOR_DATA    *op_data = (OPERATOR_DATA*) kick_object;
    SRC_SYNC_OP_DATA *op_extra_data = get_instance_data(op_data);

    patch_fn_shared(src_sync);

    base_op_profiler_start(op_data);

    op_extra_data->kick_id = TIMER_ID_INVALID;

    /* Raise a bg int to process */
    opmgr_kick_operator(op_data);

    base_op_profiler_stop(op_data);
}

bool src_sync_perform_transitions(SRC_SYNC_OP_DATA* op_extra_data)
{
    SRC_SYNC_SOURCE_GROUP* src_grp;
    unsigned i;
    bool routes_changed = FALSE;

    for ( src_grp = op_extra_data->source_groups;
          src_grp != NULL;
          src_grp = next_source_group(src_grp))
    {
        /* is transition_pt == 0 for all sources with a switch_route? */
        unsigned grp_switch_route_mask =
                ( src_grp->common.channel_mask
                  & op_extra_data->src_route_switch_pending_mask);

        if (0 == grp_switch_route_mask)
        {
            /* No sources in this group have a pending transition */
            continue;
        }

        bool unfinished_transition_out = FALSE;
        for (i = 0; i < SRC_SYNC_CAP_MAX_CHANNELS; ++ i)
        {
            if ((grp_switch_route_mask & (1<<i)) != 0)
            {
                SRC_SYNC_SOURCE_ENTRY* src_ptr = op_extra_data->sources[i];

                PL_ASSERT((src_ptr != NULL) && (src_ptr->switch_route.sink != NULL));

                if (src_ptr->transition_pt != 0)
                {
                    unfinished_transition_out = TRUE;
                    break;
                }
            }
        }

        if (! unfinished_transition_out)
        {
            SOSY_MSG2( SRC_SYNC_TRACE_ALWAYS, "src_g_%d switch route mask 0x%06x",
                src_grp->common.idx, grp_switch_route_mask);

            /* Change over now */
            for (i = 0; i < SRC_SYNC_CAP_MAX_CHANNELS; ++ i)
            {
                if ((grp_switch_route_mask & (1<<i)) != 0)
                {
                    SRC_SYNC_SOURCE_ENTRY* src_ptr = op_extra_data->sources[i];
                    PL_ASSERT((src_ptr != NULL) && (src_ptr->switch_route.sink != NULL));

                    /* Change direction of transition */
                    src_ptr->inv_transition = - src_ptr->inv_transition;

                    /* Switch Sinks */
                    src_ptr->current_route = src_ptr->switch_route;
                    src_ptr->switch_route.sink = NULL;
                    op_extra_data->src_route_switch_pending_mask &= ~(1<<i);

                    /* Update submodules which need to have the current sample rate */
                    SRC_SYNC_SINK_GROUP* sink_grp =
                            sink_group_from_entry(src_ptr->current_route.sink);
                    src_sync_ra_set_rate(op_extra_data, sink_grp, op_extra_data->sample_rate);
                    if (sink_grp->ts_rate_master)
                    {
                        src_sync_ra_set_primary_rate(op_extra_data, op_extra_data->sample_rate);
                    }

                    routes_changed = TRUE;

                    op_extra_data->Dirty_flag |= (1<<i);

                    SOSY_MSG3( SRC_SYNC_TRACE_TRANSITION,
                               "route sink %d -> src %d g %d/60db",
                               src_ptr->current_route.sink->common.idx,
                               src_ptr->common.idx,
                               src_ptr->current_route.gain_dB);
                }
            }
        }
    }
    return routes_changed;
}

void src_sync_refresh_forward_routes(SRC_SYNC_OP_DATA* op_extra_data)
{
    SRC_SYNC_SINK_GROUP* sink_grp;
    unsigned i;

    /* Clear, then recreate forward route linked lists
     * (sink_ptr->source and src_ptr->next_split_route,
     * sink_grp->route_dest)
     */
    for (i = 0; i < SRC_SYNC_CAP_MAX_CHANNELS; ++ i)
    {
        SRC_SYNC_SINK_ENTRY* sink_ptr = op_extra_data->sinks[i];
        if (sink_ptr != NULL)
        {
            sink_ptr->source = NULL;
        }
    }
    for (i = 0; i < SRC_SYNC_CAP_MAX_CHANNELS; ++ i)
    {
        SRC_SYNC_SOURCE_ENTRY* src_ptr = op_extra_data->sources[i];
        if (src_ptr != NULL)
        {
            src_ptr->next_split_source = NULL;
        }
    }
    for ( sink_grp = op_extra_data->sink_groups;
          sink_grp != NULL;
          sink_grp = next_sink_group(sink_grp))
    {
        sink_grp->route_dest = NULL;
        sink_grp->have_route_dest = FALSE;
    }
    for (i = 0; i < SRC_SYNC_CAP_MAX_CHANNELS; ++ i)
    {
        SRC_SYNC_SOURCE_ENTRY* src_ptr = op_extra_data->sources[i];
        if ( (src_ptr != NULL)
             && src_sync_valid_route(&src_ptr->current_route)
             && (src_ptr->common.buffer != NULL) )
        {
            SRC_SYNC_SINK_ENTRY* route_origin =
                    src_ptr->current_route.sink;
            src_ptr->next_split_source = route_origin->source;
            route_origin->source = src_ptr;

            SRC_SYNC_SINK_GROUP* route_origin_grp =
                    sink_group_from_entry(route_origin);
            SRC_SYNC_SOURCE_GROUP* dest_grp =
                    source_group_from_entry(src_ptr);

            PL_ASSERT((route_origin_grp->route_dest == NULL) || (route_origin_grp->route_dest == dest_grp));
            route_origin_grp->route_dest = dest_grp;
            route_origin_grp->have_route_dest = TRUE;

#ifdef SOSY_VERBOSE
            if (src_ptr->next_split_source != NULL)
            {
                SOSY_MSG3( SRC_SYNC_TRACE_TRANSITION,
                           "route changed: sink_%d -> src_%d (split src_%d)",
                           route_origin->common.idx,
                           src_ptr->common.idx,
                           src_ptr->next_split_source->common.idx);
            }
            else
            {
                SOSY_MSG2( SRC_SYNC_TRACE_TRANSITION,
                           "route changed: sink_%d -> src_%d",
                           route_origin->common.idx, src_ptr->common.idx);
            }
#endif /* SOSY_VERBOSE */
        }
    }
}

/**
 * Break out the outermost decision of src_sync_perform_transitions
 * as a performance optimization. Still, the caller should not need
 * to know about the src_route_switch_pending_mask field, so
 * keep this in an inline function.
 */
static inline bool src_sync_perform_transitions_if_needed(SRC_SYNC_OP_DATA* op_extra_data)
{
    if (op_extra_data->src_route_switch_pending_mask != 0)
    {
        return src_sync_perform_transitions(op_extra_data);
    }
    else
    {
        return FALSE;
    }
}

void src_sync_refresh_connections(SRC_SYNC_OP_DATA* op_extra_data)
{
    SRC_SYNC_SINK_GROUP* sink_grp;
    bool all_routed_sinks_are_unconnected = TRUE;

    for ( sink_grp = op_extra_data->sink_groups;
          sink_grp != NULL;
          sink_grp = next_sink_group(sink_grp))
    {
        if (sink_grp->have_route_dest && sink_grp->common.connected)
        {
            all_routed_sinks_are_unconnected = FALSE;
        }
    }

#ifdef SOSY_VERBOSE
    if (op_extra_data->all_routed_sinks_unconnected != all_routed_sinks_are_unconnected)
    {
        SOSY_MSG2( SRC_SYNC_TRACE_SINK_STATE | SRC_SYNC_TRACE_TRANSITION,
                   "route/connection changed: all_routed_sinks_unconnected %d -> %d",
                   op_extra_data->all_routed_sinks_unconnected,
                   all_routed_sinks_are_unconnected);
    }
#endif /* SOSY_VERBOSE */
    op_extra_data->all_routed_sinks_unconnected = all_routed_sinks_are_unconnected;
}

void src_sync_refresh_metadata_routes(SRC_SYNC_OP_DATA* op_extra_data)
{
    SRC_SYNC_SINK_GROUP* sink_grp;
    SRC_SYNC_SOURCE_GROUP* src_grp;

    /* Recalculate metadata routes */

    /* 1. Clear metadata routes */
    for ( sink_grp = op_extra_data->sink_groups;
          sink_grp != NULL;
          sink_grp = next_sink_group(sink_grp) )
    {
        sink_grp->metadata_dest = NULL;
    }
    for ( src_grp = op_extra_data->source_groups;
          src_grp != NULL;
          src_grp = next_source_group(src_grp) )
    {
        src_grp->metadata_in = NULL;
    }

    /* 2. From metadata routes along audio routes,
     *    chose the lowest numbered input group
     */
    for ( src_grp = op_extra_data->source_groups;
          src_grp != NULL;
          src_grp = next_source_group(src_grp) )
    {
        if ( src_grp->common.metadata_enabled &&
             (src_grp->common.metadata_buffer != NULL) )
        {
            SRC_SYNC_SOURCE_ENTRY* src_ptr;

            for ( src_ptr = source_entries_from_group(src_grp);
                  src_ptr != NULL;
                  src_ptr = next_source_entry(src_ptr) )
            {
                if (src_sync_valid_route( &(src_ptr->current_route) ))
                {
                    sink_grp = cast_sink_group(src_ptr->current_route.sink->common.group);

                    /* pointer arithmetic can be used because
                     * all sink group structs are in the array
                     * op_extra_data->sink_groups
                     */
                    if ( (src_grp->metadata_in == NULL)
                         || (sink_grp < src_grp->metadata_in) )
                    {
                        src_grp->metadata_in = sink_grp;
                    }
                }
            }
        }
    }

    /* 3. Build forward linked lists from backward pointers
     */
    for ( src_grp = op_extra_data->source_groups;
          src_grp != NULL;
          src_grp = next_source_group(src_grp) )
    {
        if (src_grp->metadata_in != NULL)
        {
            src_grp->metadata_in->metadata_dest = src_grp;

            SOSY_MSG2( SRC_SYNC_TRACE_TRANSITION,
                       "metadata route sink_g%d -> src_g%d",
                       src_grp->metadata_in->common.idx,
                       src_grp->common.idx );
        }
    }
}

void src_sync_refresh_downstream_probe(SRC_SYNC_OP_DATA* op_extra_data)
{
    SRC_SYNC_SOURCE_GROUP* src_grp = op_extra_data->source_groups;
    bool single_downstream_hop;

    if ((src_grp != NULL) &&
        (src_grp->common.next == NULL) &&
        (src_grp->common.terminals != NULL))
    {
        /* Single, non-empty source group */
        unsigned terminal_id =
                op_extra_data->id | STREAM_EP_OP_SOURCE |
                src_grp->common.terminals->idx;

        single_downstream_hop =
                stream_short_downstream_probe(
                        terminal_id,
                        &op_extra_data->downstream_probe) &&
                op_extra_data->downstream_probe.constant_rate_buffer_consumer;

        /* Fixup opportunity */
        patch_fn_shared(src_sync);
#ifndef TODO_DEPRECATE_BACKWARDS_COMPATIBILITY
        if (single_downstream_hop)
        {
            /* Try to set MAX_LATENCY and possibly MAX_PERIOD to ensure
             * * MAX_LATENCY matches buffer size,
             * * MAX_LATENCY >= MAX_PERIOD + MIN_PERIOD,
             * * MAX_PERIOD >= 1.5.
             * This is not possible if the buffer size is less than MIN_PERIOD + 1.5.
             */
            unsigned kick_period = (unsigned)stream_if_get_system_kick_period();

            /* There is an assert(kick_period <= 20000) in
             * base_op_buffer_details which ensures that the following
             * multiplication doesn't overflow.
             */
            unsigned buffer_unit = (kick_period * op_extra_data->sample_rate) / SECOND;

            unsigned buffer_sane_min =
                    buffer_unit * 3 / 2 +
                    frac_mult(buffer_unit, op_extra_data->cur_params.OFFSET_SS_PERIOD);

            unsigned buffer_size = cbuffer_get_size_in_words(
                    op_extra_data->downstream_probe.first_cbuffer);

            if (buffer_size < buffer_sane_min)
            {
                single_downstream_hop = FALSE;
                L2_DBG_MSG5("src_sync WARNING cannot configure single downstream buffer, size %d, min_size %d, kp %d, fs %d, MIN_PERIOD 0.%06d",
                            buffer_size, buffer_sane_min, kick_period, op_extra_data->sample_rate,
                            frac_mult(1000000, op_extra_data->cur_params.OFFSET_SS_PERIOD));
            }
            else
            {
                op_extra_data->cur_params.OFFSET_SS_MAX_LATENCY =
                    pl_fractional_divide(
                        rate_samples_to_usec(buffer_size,
                                             op_extra_data->sample_period),
                        kick_period << 5);
#ifdef SOSY_VERBOSE
                unsigned prt_max_latency = frac_mult(op_extra_data->cur_params.OFFSET_SS_MAX_LATENCY, 1000<<5);
                SOSY_MSG2(SRC_SYNC_TRACE_ALWAYS,
                          "autoconfigure MAX_LATENCY %2d.%03d",
                          prt_max_latency / 1000, prt_max_latency % 1000);
#endif /* SOSY_VERBOSE */
                unsigned new_max_period =
                        op_extra_data->cur_params.OFFSET_SS_MAX_LATENCY -
                        (op_extra_data->cur_params.OFFSET_SS_PERIOD >> 5);
                if (new_max_period < op_extra_data->cur_params.OFFSET_SS_MAX_PERIOD)
                {
                    op_extra_data->cur_params.OFFSET_SS_MAX_PERIOD = new_max_period;
#ifdef SOSY_VERBOSE
                    unsigned prt_max_period = frac_mult(op_extra_data->cur_params.OFFSET_SS_MAX_PERIOD, 1000<<5);
                    SOSY_MSG2(SRC_SYNC_TRACE_ALWAYS,
                              "autoconfigure MAX_PERIOD %2d.%03d",
                              prt_max_period / 1000, prt_max_period % 1000);
#endif /* SOSY_VERBOSE */
                }
            }
        }
#endif /* TODO_DEPRECATE_BACKWARDS_COMPATIBILITY */

    }
    else
    {
        single_downstream_hop = FALSE;
    }

    op_extra_data->single_downstream_hop = single_downstream_hop;
    L2_DBG_MSG1("src_sync single downstream buffer %d", single_downstream_hop);
}

/**
 * \brief Return TRUE if the buffer level of the group is above the threshold:
 *        more data than threshold value if the latter is positive,
 *        or less space than negative threshold value if the latter is
 *        negative. If the threshold value is 0, return FALSE.
 * \param sink_grp Sink group
 * \param amount_data_w The amount of data to use if threshold value > 0.
 *                      Ignored otherwise.
 * \return TRUE if buffer level is above threshold.
 */
static bool src_sync_cmp_back_kick_threshold(
        SRC_SYNC_SINK_GROUP* sink_grp,
        unsigned amount_data_w)
{
    int back_kick_threshold = sink_grp->back_kick_threshold;

    if (back_kick_threshold == 0)
    {
        return FALSE;
    }

    if (back_kick_threshold > 0)
    {
        return (int)amount_data_w > back_kick_threshold;
    }

    int amount_grp_space_w =
            (int)src_sync_any_group_space(
#ifdef SOSY_VERBOSE
                    NULL,
#endif
                    &sink_grp->common);

    /* space < -threshold <=> space + threshold < 0 */
    return (amount_grp_space_w + back_kick_threshold) < 0;
}

/**
 * \brief Check sink group levels, record amount of data
 *        and whether initial conditions for configured back kick
 *        are met.
 *        If no terminals are connected,
 *        the back kick condition is FALSE (it won't matter)
 *        and transfer_w is undefined (it won't be used).
 * \param op_extra_data Operator state
 */
void src_sync_pre_check_back_kick(SRC_SYNC_OP_DATA* op_extra_data)
{
    SRC_SYNC_SINK_GROUP* sink_grp;

    for (sink_grp = op_extra_data->sink_groups;
         sink_grp != NULL;
         sink_grp = next_sink_group(sink_grp))
    {
        /* Fixup opportunity */
        patch_fn_shared(src_sync);
        if (sink_grp->common.terminals == NULL)
        {
            sink_grp->recheck_back_kick = FALSE;
            continue;
        }

        unsigned sink_grp_data_w = src_sync_calc_sink_group_available_data(sink_grp);

        SOSY_MSG2( SRC_SYNC_TRACE_SINK_AVAIL,
                   "sink_g%d avail_w %d",
                   sink_grp->common.idx, sink_grp_data_w );

        sink_grp->transfer_w = sink_grp_data_w;

        /* In level mode, the back kick only depends on the buffer level
         * at the end of the run. In edge mode, the buffer level must
         * be above the threshold at the start of the run.
         */
        sink_grp->recheck_back_kick =
                (sink_grp->back_kick_mode == OPMSG_COMMON_BACK_KICK_MODE_LEVEL)
                || src_sync_cmp_back_kick_threshold(sink_grp, sink_grp_data_w);
    }
}

/**
 * \brief For each sink group, if the conditions for back kick
 *        were TRUE at the start of the run, evaluate the conditions
 *        now (at the end of the run).
 * \param op_extra_data Operator state
 * \return Bitmap of sink terminals to back kick
 */
unsigned src_sync_post_check_back_kick(SRC_SYNC_OP_DATA* op_extra_data)
{
    SRC_SYNC_SINK_GROUP* sink_grp;
    unsigned touched_sinks = 0;

    for (sink_grp = op_extra_data->sink_groups;
         sink_grp != NULL;
         sink_grp = next_sink_group(sink_grp))
    {
        /* Fixup opportunity */
        patch_fn_shared(src_sync);
        if (! sink_grp->recheck_back_kick)
        {
            continue;
        }

        unsigned amount_data_w = 0;
        if (sink_grp->back_kick_threshold > 0)
        {
            amount_data_w = src_sync_calc_sink_group_available_data(sink_grp);
        }

        if (! src_sync_cmp_back_kick_threshold(sink_grp, amount_data_w))
        {
            touched_sinks |= sink_grp->common.channel_mask;
        }
    }

    return touched_sinks;
}

/**
 * \brief Return the amount of data available in a sink group, in words.
 *        If not all terminals of the group are connected, return 0,
 *        to prevent transfers from part of a group's terminals in the
 *        middle of connecting a multi-channel group.
 *        If the sink group has no output routes, it won't constrain
 *        transfers, so return MAXINT.
 * \param sink_grp Sink group
 * \return Amount of data available, in words.
 */
unsigned src_sync_calc_sink_group_available_data(SRC_SYNC_SINK_GROUP* sink_grp)
{
    SRC_SYNC_SINK_ENTRY* sink_ptr;
    unsigned sink_grp_data_w = MAXINT;

    if (! sink_grp->common.connected)
    {
        return 0;
    }
    if ((! sink_grp->have_route_dest) && !sink_grp->purge)
    {
        return sink_grp_data_w;
    }

    if (sink_grp->common.metadata_enabled &&
        (sink_grp->common.metadata_buffer != NULL))
    {
        sink_grp_data_w =
                buff_metadata_available_octets(sink_grp->common.metadata_buffer)
                / OCTETS_PER_SAMPLE;
    }

    for ( sink_ptr = sink_entries_from_group(sink_grp);
          sink_ptr != NULL;
          sink_ptr = next_sink_entry(sink_ptr) )
    {
        unsigned data_w;

        data_w = cbuffer_calc_amount_data_in_words(sink_ptr->input_buffer);

        sink_grp_data_w = pl_min(sink_grp_data_w, data_w);
        if (sink_grp_data_w == 0)
        {
            break;
        }
    }

    return sink_grp_data_w;
}

/**
 * \brief Return the space in words in a terminal group.
 *        If no terminals are connected, return MAXINT.
 * \param op_extra_data Operator state, only present when SOSY_VERBOSE is
 *                      defined as it is only needed then. The log messages
 *                      are suitable for source groups. Pass NULL
 *                      to disable debug log.
 * \param any_grp Pointer to group
 * \return Space in words
 */
unsigned src_sync_any_group_space(
#ifdef SOSY_VERBOSE
        SRC_SYNC_OP_DATA* op_extra_data,
#endif
        SRC_SYNC_TERMINAL_GROUP* any_grp
)
{
    SRC_SYNC_TERMINAL_ENTRY* any_term;
    unsigned space_w = MAXINT;

    for (any_term = any_grp->terminals;
         (any_term != NULL) && (space_w > 0);
         any_term = any_term->next)
    {
        unsigned term_space_w;
        term_space_w = cbuffer_calc_amount_space_in_words(any_term->buffer);

        /* Fixup opportunity */
        patch_fn_shared(src_sync);
#ifdef SOSY_VERBOSE
        if (op_extra_data != NULL)
        {
            SOSY_MSG2( SRC_SYNC_TRACE_SRC_TERM_SPACE,
                       "calc source %d space_w %d",
                       any_term->idx, term_space_w);
        }
#endif /* SOSY_VERBOSE */

        space_w = pl_min(space_w, term_space_w);
    }

    if ( space_w > 0
         && any_grp->metadata_enabled
         && (any_grp->metadata_buffer != NULL)
         && (buff_has_metadata(any_grp->metadata_buffer)))
    {
        unsigned md_space_o =
                buff_metadata_available_space(
                        any_grp->metadata_buffer);
        unsigned md_space_w = md_space_o / OCTETS_PER_SAMPLE;

        if (md_space_w < space_w)
        {
#ifdef SOSY_VERBOSE
            if (op_extra_data != NULL)
            {
                SOSY_MSG4( SRC_SYNC_TRACE_SRC_SPACE,
                           "calc src_g%d space limited "
                           "by metadata buffer: "
                           "cb_sp_w %d > md_sp_o %d md_sp_w %d",
                           any_grp->idx, space_w,
                           md_space_o, md_space_w);
            }
#endif /* SOSY_VERBOSE */
            space_w = md_space_w;
        }
    }

    return space_w;
}

/**
 * \brief Return the space in words available across all
 *        source groups
 * \param op_extra_data Operator state
 */
unsigned src_sync_compute_space(SRC_SYNC_OP_DATA* op_extra_data)
{
    SRC_SYNC_SOURCE_GROUP* src_grp;
    unsigned src_space_w = MAXINT;

    for ( src_grp = op_extra_data->source_groups;
          src_grp != NULL;
          src_grp = next_source_group(src_grp))
    {
        if (src_grp->common.terminals != NULL)
        {
            unsigned src_grp_space_w;

            src_grp_space_w = src_sync_any_group_space(
#ifdef SOSY_VERBOSE
                    op_extra_data,
#endif
                    &src_grp->common
                );

            SOSY_MSG2( SRC_SYNC_TRACE_SRC_SPACE,
                       "calc src_g%d sp_w %d",
                       src_grp->common.idx, src_grp_space_w);

            if (src_grp_space_w == 0)
            {
                return 0;
            }
            src_space_w = pl_min(src_space_w, src_grp_space_w);
        }
    }
    return src_space_w;
}

/**
 * \note Variables whose names end with _t are in fractional seconds,
 *       those with names ending with _w are in words.
 * \return a negative value if computing and processing transfers should continue,
 *         a positive time value in order to reschedule,
 *         0 if there is not enough output space to transfer this time.
 */
RATE_SECOND_INTERVAL src_sync_compute_transfer_space(SRC_SYNC_OP_DATA* op_extra_data,
                                                     SRC_SYNC_COMP_CONTEXT* comp)
{
    TIME now;
    TIME_INTERVAL us_since;

    patch_fn_shared(src_sync);

    comp->max_transfer_w = 0;
    comp->min_transfer_w = 0;
    comp->downstream_filled = FALSE;

    now = time_get_time();
    /* The subtraction will wrap when the time_get_time() value wraps,
     * every ca. 71 minutes. The result is always expected to be a
     * small positive number (up to low thousands) due to scheduling.
     */
    us_since = time_sub(now, op_extra_data->time_stamp);
    op_extra_data->time_stamp = now;

    comp->est_latency_t =
            pl_max( op_extra_data->est_latency
                    - rate_signed_usec_to_second_interval(us_since),
                    0 );

    SOSY_MSG4( SRC_SYNC_TRACE_KICK,
               "calc: t %d est_latency(in) 0.%06d us_since %d el 0.%06d",
               now, rate_second_interval_to_signed_usec_round(op_extra_data->est_latency),
               us_since, rate_second_interval_to_signed_usec_round(comp->est_latency_t));

    const RATE_SECOND_INTERVAL SS_PERIOD = src_sync_get_period(op_extra_data);
    const RATE_SECOND_INTERVAL SS_MAX_PERIOD = src_sync_get_max_period(op_extra_data);
    const RATE_SECOND_INTERVAL SS_MAX_LATENCY = src_sync_get_max_latency(op_extra_data);

    /* Check Sources.
     * This only includes sources that are connected with a route defined.
     *
     *  1) Check for sink switch (MUX)
     *  2) Mark associated sink as active
     *  3) Convert space to time and verify greater than system time period
     *      Otherwise, wait for next period.
     *
     *  Note:  If any source route stalls, all source routes are stalled.
     *         This is the intended design and consistent with the
     *         purpose of this capability of syncronizing sources
     */
    if (op_extra_data->forward_kicks == 0)
    {
        SOSY_MSG1( SRC_SYNC_TRACE_COMPUTE_TRANSFER,
                   "calc: nothing to do, est_latency 0.%06d",
                   rate_second_interval_to_signed_usec_round(comp->est_latency_t));

        op_extra_data->est_latency = comp->est_latency_t;
        return 0; /* means, nothing to do, wait for next kick */
    }

    unsigned src_space_w;

    src_space_w = src_sync_compute_space(op_extra_data);

    SOSY_MSG1( SRC_SYNC_TRACE_SRC_SPACE,
               "calc srcs sp_w %d",
               src_space_w);

    if (src_space_w < op_extra_data->ss_period_w)
    {
        /* Strict timing (i.e. non-stallable) source groups don't have
         * enough space to process. Skip to next period.
         */
        SOSY_MSG2( SRC_SYNC_TRACE_COMPUTE_TRANSFER,
                   "calc transfer space %d < ss_period_w %d not enough to do",
                   src_space_w, op_extra_data->ss_period_w);
        op_extra_data->est_latency = comp->est_latency_t;
        return 0; /* means, nothing to do, wait for next kick */
    }

    /*
     * A special case which occurs frequently enough:
     * If all inputs are unconnected, i.e. this operator only
     * generates silence, limit the estimated amount to MAX_LATENCY
     * and thus limit the rate to approx. the nominal rate plus margin
     * for rate deviations
     */
    if (op_extra_data->all_routed_sinks_unconnected)
    {
        if (comp->est_latency_t > (SS_MAX_LATENCY - SS_PERIOD))
        {
            SOSY_MSG1( SRC_SYNC_TRACE_COMPUTE_TRANSFER,
                       "calc transfer, all inputs unconnected, est 0.%06d wait",
                       rate_second_interval_to_signed_usec_round(comp->est_latency_t));
            op_extra_data->est_latency = comp->est_latency_t;

            return SS_PERIOD; /* means, check periodically while in this state */
        }

        RATE_SECOND_INTERVAL silence_amount_t = SS_MAX_LATENCY - comp->est_latency_t;
        /* 5% margin for non-ideal rates */
        silence_amount_t += frac_mult(silence_amount_t, FRACTIONAL(0.05));
        unsigned silence_amount_w = rate_second_interval_to_samples(silence_amount_t,
                                                                    op_extra_data->sample_rate);
        src_space_w = pl_min(src_space_w, silence_amount_w);

        SOSY_MSG3( SRC_SYNC_TRACE_COMPUTE_TRANSFER,
                   "calc transfer, all inputs unconnected, est 0.%06d, silence_w %d, fixup space_w %d",
                   rate_second_interval_to_signed_usec_round(comp->est_latency_t),
                   silence_amount_w, src_space_w);
    }

    /* Potential fixups: may decide to change the heuristic for detecting
     * downstream buffer full (B-243778)
     */
    patch_fn_shared(src_sync);

    if (op_extra_data->single_downstream_hop)
    {
        /* When setting single_downstream_hop to TRUE, it was checked that
         * there is a non-empty source group.
         */
        tCbuffer* first_cbuffer =
                op_extra_data->source_groups->common.terminals->buffer;

        /* Directly measure the buffered amount and convert to time */
        comp->est_latency_t =
            rate_samples_to_second_interval(
                stream_calc_downstream_amount_words_ex(
                        first_cbuffer,
                        &op_extra_data->downstream_probe),
                op_extra_data->sample_period);

        /* Some state transition still need downstream_filled */
        comp->downstream_filled =
                (comp->est_latency_t >= (SS_MAX_LATENCY - SS_PERIOD));
    }
    else
    {
        /*
         * The downstream buffer not emptied condition is used to reset
         * the estimated latency to the configured upper threshold (MAX_LATENCY).
         * To avoid triggering this by a kick between writing to downstream
         * and the downstream operator consuming, use the running history maximum.
         */
        src_sync_put_buffer_history( &op_extra_data->source_buffer_history,
                                     op_extra_data->time_stamp, src_space_w);
        unsigned max_src_space_over_history_w =
                src_sync_get_buffer_history_max(
                        &op_extra_data->source_buffer_history,
                        op_extra_data->time_stamp,
                        stream_if_get_system_kick_period() +
                        SRC_SYNC_BUFFER_LEVEL_HISTORY_PERIOD_MARGIN );

        if (max_src_space_over_history_w < op_extra_data->max_space_w)
        {
            /* max_space was calculated to be the minimum of the buffer sizes
            ** connected to the sources.
            ** If the minimum of the space available on all sources falls below this
            ** threshold, it means that all source buffers contain unconsumed data.
            ** This is interpreted as meaning that the downstream chains
            ** have been filled. Then we take SS_MAX_LATENCY as a given amount of
            ** buffered data, something like: minimum over all downstream chains,
            ** of sum of buffer size for the source endpoint, and average buffer
            ** contents for all further buffers, scaled as time. Subtract min_period
            ** (unused buffer space still included in the calculation so far)
            ** to get a new est_latency.
            */
            RATE_SECOND_INTERVAL max_src_space_over_history_t =
                    rate_samples_to_second_interval(max_src_space_over_history_w,
                                                    op_extra_data->sample_period);
            if (pl_min(SS_MAX_LATENCY,SS_MAX_PERIOD) > max_src_space_over_history_t)
            {
                RATE_SECOND_INTERVAL new_latency_t = SS_MAX_LATENCY - max_src_space_over_history_t;
                if (new_latency_t != comp->est_latency_t)
                {
                    comp->est_latency_t = new_latency_t;
                    SOSY_MSG3( SRC_SYNC_TRACE_SRC_SPACE_FILLED,
                               "calc source buffers non-empty "
                               "(hist_space_w %d < max_space_w %d), "
                               "set est_latency 0.%06d",
                               max_src_space_over_history_w,
                               op_extra_data->max_space_w,
                               rate_second_interval_to_signed_usec_round(comp->est_latency_t));
                }

                comp->downstream_filled = TRUE;
            }
        }
    }

    /* Initialize the limit on the global transfer based on space
     * at (non-stallable) sources. This will be capped by other
     * amounts such as data available at non-stalled sinks later.
     */
    comp->max_transfer_w = src_space_w;

    /* Minimum amount to write to sources in order to avoid underrun for
     * the next SS_PERIOD
     */
    if (comp->est_latency_t >= (SS_MAX_PERIOD + SS_PERIOD))
    {
        comp->min_transfer_w = 0;
    }
    else
    {
        RATE_SECOND_INTERVAL min_transfer_t = (SS_MAX_PERIOD + SS_PERIOD) -
                                              comp->est_latency_t;
        comp->min_transfer_w =
                rate_second_interval_to_samples_trunc(min_transfer_t,
                                                      op_extra_data->sample_rate);

        /* Avoid writing tiny amounts of silence while
         * scraping along near minimum buffer fill
         */
        comp->min_transfer_w = pl_max(comp->min_transfer_w,
                                      pl_min(op_extra_data->ss_period_w,
                                             comp->max_transfer_w));
    }

    if (comp->min_transfer_w > comp->max_transfer_w)
    {
        /* Make the values consistent. This should not happen with
         * sane parameters: when source buffers are full, est_latency
         * should have been set to nearly SS_MAX_LATENCY above.
         *
         * Sanity check:
         * SS_MAX_PERIOD + SS_PERIOD <= SS_MAX_LATENCY
         *
         * The way to get here may be: output buffers suddenly filled
         * but history still contains some recent not-full records
         * and meanwhile passing time brought est_latency below threshold.
         */
        L2_DBG_MSG("src_sync WARNING cannot write minimum "
                   "to avoid underrun (check parameters)");

        comp->min_transfer_w = comp->max_transfer_w;
    }

    /* Return a negative value to proceed.
     * Note only the sign matters here.
     */
    return -1;
}

RATE_SECOND_INTERVAL src_sync_compute_transfer_sinks(SRC_SYNC_OP_DATA* op_extra_data,
                                                     SRC_SYNC_COMP_CONTEXT* comp)
{
    const RATE_SECOND_INTERVAL SS_PERIOD = src_sync_get_period(op_extra_data);
    const RATE_SECOND_INTERVAL SS_MAX_PERIOD = src_sync_get_max_period(op_extra_data);
    SRC_SYNC_SOURCE_GROUP* src_grp;
    SRC_SYNC_SINK_GROUP* sink_grp;
    bool have_pending_sinks;

    patch_fn_shared(src_sync);

    /* Check Sinks.   Sink list is all connected sinks.
       Only interested in sinks for active routes or with purge flag set

       1) Get amount of data at terminal
       2) Limit transfer based on data available.   Check for stalls  */
    have_pending_sinks = FALSE;

    for ( sink_grp = op_extra_data->sink_groups;
          sink_grp != NULL;
          sink_grp = next_sink_group(sink_grp))
    {
        /* Modify loop body */
        patch_fn_shared(src_sync);

        if (sink_grp->common.terminals == NULL)
        {
            continue;
        }
        unsigned sink_grp_data_w = sink_grp->transfer_w;

        if (sink_grp_data_w == MAXINT)
        {
            continue;
        }
        bool have_data;

        have_data = (sink_grp_data_w >= op_extra_data->ss_period_w);

        sink_grp->copy_before_silence = FALSE;

        if ( (sink_grp->stall_state != SRC_SYNC_SINK_NOT_CONNECTED)
             && ! sink_grp->common.connected )
        {
            SOSY_MSG1( SRC_SYNC_TRACE_COMPUTE_TRANSFER,
                       "sink_g%d disconnected",
                       sink_grp->common.idx);
            src_sync_set_sink_state( op_extra_data, sink_grp,
                                     SRC_SYNC_SINK_NOT_CONNECTED);
        }

        /* Transition based on input buffer or downstream_filled */
        switch (sink_grp->stall_state)
        {
        case SRC_SYNC_SINK_FLOWING:
            {
#ifdef SOSY_VERBOSE
                int sink_grp_data_usec =
                        rate_samples_to_usec(sink_grp_data_w,
                                             op_extra_data->sample_period);
#endif /* SOSY_VERBOSE */
                if (! have_data)
                {
                    if (comp->min_transfer_w == 0)
                    {
                        SOSY_MSG3( SRC_SYNC_TRACE_PENDING,
                                   "calc sink_g%d sg_w %d "
                                   "sg_t 0.%06d: late",
                                   sink_grp->common.idx, sink_grp_data_w,
                                   SOSY_IF_VERBOSE(sink_grp_data_usec));

                        src_sync_set_sink_state( op_extra_data, sink_grp,
                                                 SRC_SYNC_SINK_PENDING);
                        have_pending_sinks = TRUE;
                        /* No transfers */
                    }
                    else
                    {
                        SOSY_MSG3( SRC_SYNC_TRACE_STALLED,
                                   "calc sink_grp %d sg_w %d "
                                   "sg_t 0.%06d: stalled",
                                   sink_grp->common.idx, sink_grp_data_w,
                                   SOSY_IF_VERBOSE(sink_grp_data_usec));

                        op_extra_data->stat_sink_stalled |=
                                sink_grp->common.channel_mask;
                        op_extra_data->stat_sink_stall_occurred |=
                                sink_grp->common.channel_mask;
                        src_sync_set_sink_state( op_extra_data, sink_grp,
                                                 SRC_SYNC_SINK_STALLED );

                        /* Enter stalled state: reset silence counter */
                        sink_grp->inserted_silence_words = 0;

                        /* copy sink_grp_data_t, then silence
                         * up to min_transfer_t == max_transfer_t
                         */
                        sink_grp->copy_before_silence = TRUE;
                    }
                }
                else
                {
                    SOSY_MSG4( SRC_SYNC_TRACE_FLOWING,
                               "calc sink_g%d sg_w %d sg_t 0.%06d "
                               "flowing max_w %d",
                               sink_grp->common.idx,
                               sink_grp_data_w,
                               SOSY_IF_VERBOSE(sink_grp_data_usec),
                               comp->max_transfer_w);
                    /* only copy */
                }
            }
            break;

        case SRC_SYNC_SINK_PENDING:
            if (! have_data)
            {
                /* not enough data */
                /* Have we waited too long? */
                if (comp->min_transfer_w != 0)
                {
                    op_extra_data->stat_sink_stalled |=
                            sink_grp->common.channel_mask;
                    op_extra_data->stat_sink_stall_occurred |=
                            sink_grp->common.channel_mask;
                    src_sync_set_sink_state( op_extra_data, sink_grp,
                                             SRC_SYNC_SINK_STALLED);

                    /* Enter stalled state: reset silence counter */
                    sink_grp->inserted_silence_words = 0;

                    /* copy sink_grp_data_t,
                     * then silence up to min_transfer_t == max_transfer_t
                     */
                    sink_grp->copy_before_silence = TRUE;
                }
                else
                {
                    SOSY_MSG1( SRC_SYNC_TRACE_PENDING,
                               "calc src_g%d still pending",
                               sink_grp->common.idx);
                    have_pending_sinks = TRUE;
                    /* No transfers */
                }
            }
            else
            {
                /* When certain streams with metadata start in state
                 * PENDING, after initial silence-fill latency has
                 * run down, more silence fill is needed to reach
                 * the intended latency.
                 * Part of the condition for the start of such streams
                 * is that up to now no timestamps have been observed.
                 */
                if ( (sink_grp->timestamp_state.timestamp_type
                      == RATE_TIMESTAMP_TYPE_NONE)
                    && (sink_grp->metadata_input_buffer != NULL)
                    && (comp->est_latency_t < src_sync_get_max_latency(op_extra_data)) )
                {
                    metadata_tag* peeked = buff_metadata_peek(
                            sink_grp->metadata_input_buffer);
                    /* These are the tag types which later
                     * preserve latency across stalls, i.e.
                     * for which src_sync_peek_resume can return
                     * SRC_SYNC_STALL_RECOVERY_GAP, so streams
                     * containing them don't automatically recover
                     * from insufficient latency margin.
                     */
                    if ( (peeked != NULL)
                         && ( IS_TIME_TO_PLAY_TAG(peeked)
                             || IS_TIME_OF_ARRIVAL_TAG(peeked)) )
                    {
                        unsigned available_data_w;
                        RATE_SECOND_INTERVAL required_data_t;
                        unsigned required_data_w;

                        /* Number of samples required to reach
                         * max latency
                         */
                        required_data_t =
                                src_sync_get_max_latency(op_extra_data)
                                - comp->est_latency_t;
                        required_data_w = rate_second_interval_to_samples(
                                required_data_t,
                                op_extra_data->sample_rate);

                        /* If the data appears to be packetized,
                         * count the whole block as available.
                         */
                        if (METADATA_PACKET_START(peeked))
                        {
                            available_data_w = peeked->length / OCTETS_PER_SAMPLE;
                        }
                        else
                        {
                            available_data_w = sink_grp_data_w;
                        }

                        if (available_data_w < required_data_w)
                        {
#ifdef SOSY_VERBOSE
                            if (METADATA_PACKET_START(peeked))
                            {
                                SOSY_MSG4(SRC_SYNC_TRACE_PEEK_RESUME,
                                          "sink_g%d pending, start of packeted tagged stream, "
                                          "est_latency_t 0.%06d "
                                          "reqd_w %d avail_w %d",
                                          sink_grp->common.idx,
                                          rate_second_interval_to_signed_usec_round(comp->est_latency_t),
                                          required_data_w, available_data_w);
                            }
                            else
                            {
                                SOSY_MSG4(SRC_SYNC_TRACE_PEEK_RESUME,
                                          "sink_g%d pending, start of unpacketed tagged stream, "
                                          "est_latency_t 0.%06d "
                                          "reqd_w %d avail_w %d",
                                          sink_grp->common.idx,
                                          rate_second_interval_to_signed_usec_round(comp->est_latency_t),
                                          required_data_w, available_data_w);
                            }
#endif
                            /* Add silence fill to reach max latency */
                            sink_grp->stall_recovery_silence_words =
                                    required_data_w - available_data_w;
                            sink_grp->filling_until_full = FALSE;
                            sink_grp->inserted_silence_words = 0;
                            src_sync_set_sink_state(
                                    op_extra_data, sink_grp,
                                    SRC_SYNC_SINK_RECOVERING_FILLING );
                            break;
                        }
                    }
                }

                op_extra_data->stat_sink_stalled &=
                        ~ sink_grp->common.channel_mask;
                src_sync_set_sink_state( op_extra_data, sink_grp,
                                         SRC_SYNC_SINK_FLOWING);
            }
            break;

        case SRC_SYNC_SINK_STALLED:
        case SRC_SYNC_SINK_RECOVERING_WAITING_FOR_TAG:
            /* Look at data when there is more than SS_PERIOD of it */
            if (have_data)
            {
                if (sink_grp->common.metadata_enabled)
                {
                    unsigned peek_beforeidx;
                    unsigned gap_samples;
                    metadata_tag* peeked = buff_metadata_peek_ex(
                            sink_grp->metadata_input_buffer,
                            &peek_beforeidx);

                    src_sync_stall_recovery_type recovery =
                        src_sync_peek_resume(
                            op_extra_data,
                            peeked, &sink_grp->timestamp_state,
                            peek_beforeidx, sink_grp_data_w,
                            &gap_samples, sink_grp->common.idx );

                    switch (recovery)
                    {
                    default:
                        /* src_sync_peek_resume does not return other values */
                    case SRC_SYNC_STALL_RECOVERY_UNKNOWN:
                        /* Fill downstream with a configured amount
                         * of silence. Don't consume data or metadata
                         * until filled.
                         */
                        sink_grp->stall_recovery_silence_words =
                                rate_second_interval_to_samples(
                                        src_sync_get_stall_recovery_default_fill(op_extra_data),
                                        op_extra_data->sample_rate );
                        sink_grp->filling_until_full = FALSE;
                        sink_grp->inserted_silence_words = 0;
                        src_sync_set_sink_state(
                                op_extra_data, sink_grp,
                                SRC_SYNC_SINK_RECOVERING_FILLING );
                        break;

                    case SRC_SYNC_STALL_RECOVERY_WAITING_FOR_TAG:
                        /* Consume the data and metadata,
                         * send minimum amount of silence downstream
                         * to prevent underruns
                         */
                        src_sync_set_sink_state(
                                op_extra_data, sink_grp,
                                SRC_SYNC_SINK_RECOVERING_WAITING_FOR_TAG );
                        /* only silence */
                        break;

                    case SRC_SYNC_STALL_RECOVERY_RESTART:
                        /* Fill downstream until full. Don't
                         * consume data or metadata until then.
                         */
                        src_sync_set_sink_recovering_restarting(
                                op_extra_data, sink_grp );
                        break;

                    case SRC_SYNC_STALL_RECOVERY_GAP:
                        /* gap_samples is valid only in this case */
                        if ( gap_samples
                             < sink_grp->inserted_silence_words)
                        {
                            /* Already sent more silence than
                             * the gap turned out to be.
                             * Discard some data to re-align.
                             * Insert minimum amount of silence; add this
                             * to the amount of input that needs to be
                             * discarded.
                             */
                            sink_grp->stall_recovery_discard_words =
                                sink_grp->inserted_silence_words
                                - gap_samples;
                            src_sync_set_sink_state(
                                    op_extra_data, sink_grp,
                                    SRC_SYNC_SINK_RECOVERING_DISCARDING );

                            /* Limit on waiting for input to catch up,
                             * expressed in length of silence sent while
                             * waiting
                             */
                            sink_grp->stall_recovery_discard_remaining =
                                    rate_time_to_samples(
                                            op_extra_data->cur_params
                                            .OFFSET_SS_STALL_RECOVERY_CATCHUP_LIMIT,
                                            op_extra_data->sample_rate);
                        }
                        else if ( gap_samples
                                  > sink_grp->inserted_silence_words )
                        {
                            /* Gap is longer than the silence sent so far.
                             * Send more silence as fast as the output
                             * accepts it.
                             */
                            sink_grp->stall_recovery_silence_words =
                                gap_samples
                                - sink_grp->inserted_silence_words;
                            sink_grp->filling_until_full = FALSE;
                            src_sync_set_sink_state(
                                    op_extra_data, sink_grp,
                                    SRC_SYNC_SINK_RECOVERING_FILLING );

                        }
                        else
                        {
                            /* Gap and silence happen to match. */
                            /* Forward data and metadata. */
                            op_extra_data->stat_sink_stalled &=
                                    ~ sink_grp->common.channel_mask;
                            src_sync_set_sink_state(
                                    op_extra_data, sink_grp,
                                    SRC_SYNC_SINK_FLOWING );
                        }
                        break;
                    }
                }
                else
                {
                    /* Fill downstream with a configured amount
                     * of silence. Don't consume data or metadata
                     * until filled.
                     */
                    sink_grp->stall_recovery_silence_words =
                            rate_second_interval_to_samples(
                                    src_sync_get_stall_recovery_default_fill(op_extra_data),
                                    op_extra_data->sample_rate );
                    sink_grp->filling_until_full = TRUE;
                    sink_grp->inserted_silence_words = 0;
                    src_sync_set_sink_state(
                            op_extra_data, sink_grp,
                            SRC_SYNC_SINK_RECOVERING_FILLING );
                    /* Recheck after overall max_transfer_t is known,
                     * whether the inserted silence fits within
                     * this kick.
                     */
                }
            }
            break;

        case SRC_SYNC_SINK_RECOVERING_RESTARTING:
            /* If downstream buffers have been filled, start copying.
             * Until then, write as much silence as the outputs
             * accept.
             */
            if (comp->downstream_filled)
            {
                SOSY_MSG1( SRC_SYNC_TRACE_RECOVERED,
                           "restarted after fill_w %d",
                           sink_grp->inserted_silence_words);
                op_extra_data->stat_sink_stalled &=
                        ~ sink_grp->common.channel_mask;
                src_sync_set_sink_state( op_extra_data, sink_grp,
                                         SRC_SYNC_SINK_FLOWING);
            }
            break;

        case SRC_SYNC_SINK_RECOVERING_FILLING:
            /* If we get here at the start of process_data, it means
             * the next step is sending silence downstream, and not
             * consuming data; so not dependent on sink state.
             */
            /* Recheck after overall max_transfer_t is known,
             * whether the inserted silence fits within
             * this kick.
             */
            /* If filling_until_full is set, check if downstream
             * buffers have been filled once, and if so,
             * switch to copying.
             */
            if (sink_grp->filling_until_full && comp->downstream_filled)
            {
                SOSY_MSG1( SRC_SYNC_TRACE_RECOVERED,
                           "recovered after fill_w %d",
                           sink_grp->inserted_silence_words);
                op_extra_data->stat_sink_stalled &=
                        ~ sink_grp->common.channel_mask;
                src_sync_set_sink_state( op_extra_data, sink_grp,
                                         SRC_SYNC_SINK_FLOWING);
            }
            break;

        case SRC_SYNC_SINK_RECOVERING_DISCARDING:
            /* If we get here at the start of process_data, it means
             * that at the end of the last process_data, still more
             * samples had to be discarded from input.
             * If additional silence samples have to be sent in the
             * meantime to prevent underruns, add that amount to
             * stall_recovery_discard_words.
             */
            break;

        case SRC_SYNC_SINK_NOT_CONNECTED:
            /* sink_grp_data_w < MAXINT implied connected,
             * so transition
             */
            if (sink_grp->common.connected)
            {
                SOSY_MSG1( SRC_SYNC_TRACE_COMPUTE_TRANSFER,
                           "sink_g%d connected",
                           sink_grp->common.idx);

                src_sync_set_sink_recovering_restarting(
                        op_extra_data, sink_grp );
            }
            break;

        default:
            break;
        }

        switch (sink_grp->stall_state)
        {
        case SRC_SYNC_SINK_FLOWING:
            /* Limit Transfer to available data */
            comp->max_transfer_w = pl_min(comp->max_transfer_w, sink_grp_data_w);
            break;

        case SRC_SYNC_SINK_STALLED:
        case SRC_SYNC_SINK_RECOVERING_WAITING_FOR_TAG:
        case SRC_SYNC_SINK_RECOVERING_DISCARDING:
            /* While stalled, transfer minimum */
            comp->max_transfer_w = comp->min_transfer_w;
            break;

        case SRC_SYNC_SINK_RECOVERING_FILLING:
            {
                /* Limit current transfer to what could
                 * be sent immediately
                 */
                unsigned available_now_w =
                    sink_grp->stall_recovery_silence_words
                    + sink_grp_data_w;
                comp->max_transfer_w = pl_min(comp->max_transfer_w,
                                              available_now_w);
            }
            break;

        case SRC_SYNC_SINK_RECOVERING_RESTARTING:
        case SRC_SYNC_SINK_NOT_CONNECTED:
            /* Transfer is not limited by this route;
             * means produce as much silence
             * as needed.
             */
            break;

        case SRC_SYNC_SINK_PENDING:
            /* Nothing to adjust, will exit below and
             * not transfer anything
             */
        default:
            break;
        }
    }

    /* Fixups, initial conditions have been modified */
    patch_fn_shared(src_sync);

    if (have_pending_sinks)
    {
        /* have_pending_sinks is only set when min_transfer_t == 0,
         * that guarantees est_latency_t >= (int)(SS_MAX_PERIOD + SS_PERIOD),
         * thus wait_period >= SS_PERIOD.
         */
        /* time left until underrun */
        RATE_SECOND_INTERVAL wait_period = comp->est_latency_t - SS_MAX_PERIOD;

        SOSY_MSG4( SRC_SYNC_TRACE_PENDING,
                   "calc pending est_lat_t 0.%06d max_per_t 0.%06d ss_period 0.%06d still time 0.%06d",
                   rate_second_interval_to_signed_usec_round(comp->est_latency_t),
                   rate_second_interval_to_signed_usec_round(SS_MAX_PERIOD),
                   rate_second_interval_to_signed_usec_round(SS_PERIOD),
                   rate_second_interval_to_signed_usec_round(wait_period));

        op_extra_data->est_latency = comp->est_latency_t;
        return wait_period;
    }

    SOSY_MSG1( SRC_SYNC_TRACE_COMPUTE_TRANSFER,
               "calc after sinks, max_transfer_w %d",
               comp->max_transfer_w);

    /* Actual transferred amount is rounded down to whole samples */
    RATE_SECOND_INTERVAL act_transfer_t = 0;

    op_extra_data->min_transfer_w = comp->min_transfer_w;
    unsigned src_transfer_w = comp->max_transfer_w;

    bool have_source_terminals = FALSE;
    for ( src_grp = op_extra_data->source_groups;
          src_grp != NULL;
          src_grp = next_source_group(src_grp))
    {
        /* Limit source transfer amounts to the overall
         * amount, converted to samples.
         * The result may be smaller than the overall amount,
         * if a source is stalled.
         */

        if (src_grp->common.terminals != NULL)
        {
            have_source_terminals = TRUE;
            break;
        }
    }

    if (have_source_terminals)
    {
        SOSY_MSG2( SRC_SYNC_TRACE_COMPUTE_TRANSFER,
                   "calc src: transfer space %d min %d",
                   src_transfer_w, op_extra_data->min_transfer_w);

        op_extra_data->src_transfer_w = src_transfer_w;

        RATE_SECOND_INTERVAL src_transfer_t =
                rate_samples_to_second_interval(src_transfer_w,
                                                op_extra_data->sample_period);

        /* Use the approximate actual sample period of the output,
         * rather than the nominal one, to reduce latency drift
         */
        src_transfer_t += frac_mult(src_transfer_t,
                                    op_extra_data->primary_sp_adjust);

        act_transfer_t = src_transfer_t;
    }

    if (act_transfer_t != 0)
    {
        comp->est_latency_t += act_transfer_t;
        op_extra_data->est_latency = comp->est_latency_t;
        SOSY_MSG3( SRC_SYNC_TRACE_COMPUTE_TRANSFER,
                   "calc max_transfer_w %d act_transfer 0.%06d el 0.%06d",
                   comp->max_transfer_w,
                   rate_second_interval_to_signed_usec_round(act_transfer_t),
                   rate_second_interval_to_signed_usec_round(comp->est_latency_t));

        /* Save transfer (negative to differentiate from stall timer */
        /* note the magnitude is not used, only the sign */
        return (- act_transfer_t);
    }
    else
    {
        /* After rounding down to whole samples, there was nothing to do.
         * Recheck soon. */
        op_extra_data->est_latency = comp->est_latency_t;
        return SS_PERIOD;
    }
}

unsigned src_sync_route_write_silence( SRC_SYNC_OP_DATA *op_extra_data,
                                       SRC_SYNC_SINK_GROUP *sink_grp,
                                       unsigned words )
{
    unsigned written = 0;
    SRC_SYNC_SINK_ENTRY* sink_ptr;

    patch_fn_shared(src_sync);

    if (sink_grp->metadata_dest != NULL)
    {
        src_sync_metadata_silence( op_extra_data,
                                   &(sink_grp->metadata_dest->common),
                                   &(sink_grp->metadata_dest->metadata_dest),
                                   words * OCTETS_PER_SAMPLE);
    }

    for ( sink_ptr = sink_entries_from_group(sink_grp);
          sink_ptr != NULL;
          sink_ptr = next_sink_entry(sink_ptr) )
    {
        SRC_SYNC_SOURCE_ENTRY* src_ptr = sink_ptr->source;

        if (src_ptr != NULL)
        {
            do
            {
#ifdef SOSY_CHECK_BLOCK_FILL
                unsigned space_before_w, space_after_w;
                space_before_w =
                    cbuffer_calc_amount_space_in_words(src_ptr->common.buffer);
#endif /* SOSY_CHECK_BLOCK_FILL */

                if (src_ptr->common.buffer != NULL)
                {
                    cbuffer_block_fill( src_ptr->common.buffer, words, 0);
                }
                else
                {
                    fault_diatribe(FAULT_AUDIO_SRC_SYNC_WRITE_UNCONNECTED_TERMINAL,
                                   src_ptr->common.idx);
                }

#ifdef SOSY_CHECK_BLOCK_FILL
                space_after_w =
                    cbuffer_calc_amount_space_in_words(src_ptr->common.buffer);
                SOSY_MSG3( SRC_SYNC_TRACE_PERFORM_TRANSFER,
                           "src_sync block fill before_w %d w %d after_w %d",
                           space_before_w, words, space_after_w);
#endif /* SOSY_CHECK_BLOCK_FILL */

                src_ptr = src_ptr->next_split_source;
            }
            while (src_ptr != NULL);

            written = words;
        }
    }

    /* Keep state */
    sink_grp->inserted_silence_words += written;

    SOSY_MSG4( SRC_SYNC_TRACE_PERFORM_TRANSFER,
               "pt g%d->g%d silence_w %d tot %d",
               sink_grp->common.idx, sink_grp->route_dest->common.idx,
               words, sink_grp->inserted_silence_words);

    return written;
}

/**
 * Discard specified number of words from input buffer,
 * only on the inputs which have a route
 * (because the unrouted ones are handled separately)
 */
unsigned src_sync_route_discard_input( SRC_SYNC_OP_DATA *op_extra_data,
                                       SRC_SYNC_SINK_GROUP *sink_grp,
                                       unsigned words )
{
    unsigned consumed = 0;
    SRC_SYNC_SINK_METADATA* md = &sink_grp->timestamp_state;

    patch_fn_shared(src_sync);

    /* Shall not be called in SINK_NOT_CONNECTED state */
    PL_ASSERT(sink_grp->common.connected);

    src_sync_get_sink_metadata( op_extra_data, sink_grp,
                                words*OCTETS_PER_SAMPLE );

    if (sink_grp->metadata_dest != NULL)
    {
        /* Cache any EOF tags */
        src_sync_metadata_drop_tags( op_extra_data, md->received,
                                     &sink_grp->metadata_dest->metadata_dest);
    }
    else
    {
#ifdef SOSY_CHECK_METADATA_TAG_COUNTS
        op_extra_data->num_tags_deleted +=
                src_sync_metadata_count_tags(md->received);
#endif /* SOSY_CHECK_METADATA_TAG_COUNTS */
        buff_metadata_tag_list_delete(md->received);
    }
    md->received = NULL;

    SRC_SYNC_SINK_ENTRY* sink_ptr;
    for ( sink_ptr = sink_entries_from_group(sink_grp);
          sink_ptr != NULL;
          sink_ptr = next_sink_entry(sink_ptr) )
    {
        SRC_SYNC_SOURCE_ENTRY* src_ptr = sink_ptr->source;

        if (src_ptr != NULL)
        {
            PL_ASSERT(sink_ptr->input_buffer != NULL);

            cbuffer_advance_read_ptr( sink_ptr->input_buffer, words );
            consumed = words;
        }
    }

    SOSY_MSG3( SRC_SYNC_TRACE_PERFORM_TRANSFER,
               "pt g%d->g%d discard_w %d",
               sink_grp->common.idx, sink_grp->route_dest->common.idx, words);

    return consumed;
}

unsigned src_sync_route_copy( SRC_SYNC_OP_DATA *op_extra_data,
                              SRC_SYNC_SINK_GROUP *sink_grp,
                              unsigned words )
{
    unsigned written = 0;

    patch_fn_shared(src_sync);

    if (words > 0)
    {
        PL_ASSERT(sink_grp->common.connected);

        SRC_SYNC_SINK_METADATA* md = &sink_grp->timestamp_state;
        unsigned copy_octets = words*OCTETS_PER_SAMPLE;

        src_sync_get_sink_metadata( op_extra_data, sink_grp, copy_octets );

        if (sink_grp->metadata_dest != NULL)
        {
            src_sync_metadata_forward(
                    op_extra_data, md, copy_octets,
                    &(sink_grp->metadata_dest->common),
                    &(sink_grp->metadata_dest->metadata_dest));
        }
        else
        {
#ifdef SOSY_CHECK_METADATA_TAG_COUNTS
            op_extra_data->num_tags_deleted +=
                    src_sync_metadata_count_tags(md->received);
#endif /* SOSY_CHECK_METADATA_TAG_COUNTS */
            buff_metadata_tag_list_delete(md->received);
        }
        md->received = NULL;

        SRC_SYNC_SINK_ENTRY* sink_ptr;
        for ( sink_ptr = sink_entries_from_group(sink_grp);
              sink_ptr != NULL;
              sink_ptr = next_sink_entry(sink_ptr) )
        {
            SRC_SYNC_SOURCE_ENTRY* src_ptr = sink_ptr->source;

            if (src_ptr != NULL)
            {
                PL_ASSERT(sink_ptr->input_buffer != NULL);
                PL_ASSERT(src_ptr->current_route.sink == sink_ptr);

                /* This takes care of gain but doesn't move
                 * input buffer pointers
                 */
                if (src_ptr->common.buffer != NULL)
                {
                    src_sync_transfer_route(src_ptr, sink_ptr->input_buffer, words);
                }
                else
                {
                    fault_diatribe(FAULT_AUDIO_SRC_SYNC_WRITE_UNCONNECTED_TERMINAL,
                                   src_ptr->common.idx);
                }

                while (src_ptr->next_split_source != NULL)
                {
                    src_ptr = src_ptr->next_split_source;
                    PL_ASSERT(src_ptr->current_route.sink == sink_ptr);
                    if (src_ptr->common.buffer != NULL)
                    {
                        src_sync_transfer_route(src_ptr, sink_ptr->input_buffer, words);
                    }
                    else
                    {
                        fault_diatribe(FAULT_AUDIO_SRC_SYNC_WRITE_UNCONNECTED_TERMINAL,
                                       src_ptr->common.idx);
                    }
                }

                cbuffer_advance_read_ptr( sink_ptr->input_buffer, words);

                written = words;
            }
        }
    }

    SOSY_MSG3( SRC_SYNC_TRACE_PERFORM_TRANSFER,
               "pt g%d->g%d copy_w %d",
               sink_grp->common.idx, sink_grp->route_dest->common.idx, words);

    return written;
}

unsigned src_sync_perform_transfer(SRC_SYNC_OP_DATA *op_extra_data)
{
    unsigned sink_kicks = 0;
    SRC_SYNC_SINK_GROUP* sink_grp;

#ifdef SOSY_CHECK_METADATA_TRANSPORT_POINTERS
    src_sync_check_md_transport_pre(op_extra_data);
#endif /* SOSY_CHECK_METADATA_TRANSPORT_POINTERS */
#ifdef SOSY_CHECK_METADATA_TAG_COUNTS
    src_sync_clear_tag_counts(op_extra_data);
#endif /* SOSY_CHECK_METADATA_TAG_COUNTS */

    for ( sink_grp = op_extra_data->sink_groups;
          sink_grp != NULL;
          sink_grp = next_sink_group(sink_grp))
    {
        /* Fixup opportunity */
        patch_fn_shared(src_sync);
        /* This state should not get here */
        PL_ASSERT(sink_grp->stall_state != SRC_SYNC_SINK_PENDING);

        /* Any other state may expect all sink buffers to be connected */
        PL_ASSERT( sink_grp->common.connected
                   || (sink_grp->stall_state == SRC_SYNC_SINK_NOT_CONNECTED));

        if (sink_grp->have_route_dest)
        {
            /* Keep track of number of words which were copied
             * to a route destination; afterwards, unrouted sinks
             * have to discard data to match.
             */
            unsigned consumed_via_route_w = 0;
            unsigned sink_w = sink_grp->transfer_w;
            unsigned src_w = op_extra_data->src_transfer_w;
            unsigned src_min_w = op_extra_data->min_transfer_w;

            switch (sink_grp->stall_state)
            {
            case SRC_SYNC_SINK_FLOWING:
                /* copy to source */
                consumed_via_route_w =
                        src_sync_route_copy( op_extra_data, sink_grp,
                                             pl_min(sink_w, src_w));
                break;

            case SRC_SYNC_SINK_STALLED:
                if (sink_grp->copy_before_silence)
                {
                    /* copy remaining sink data to source */
                    sink_grp->copy_before_silence = FALSE;

                    consumed_via_route_w =
                            src_sync_route_copy( op_extra_data,
                                                 sink_grp,
                                                 pl_min(sink_w, src_w));

                    /* write silence up to minimum transfer */
                    if (consumed_via_route_w < src_min_w)
                    {
                        src_sync_route_write_silence(
                                op_extra_data, sink_grp,
                                src_min_w - consumed_via_route_w);
                    }
                }
                else
                {
                    /* write minimum silence */
                    src_sync_route_write_silence(
                            op_extra_data, sink_grp, src_min_w );
                }
                break;

            case SRC_SYNC_SINK_NOT_CONNECTED:
                /* There is no input data to consume. Write silence,
                 * limited to MAX_LATENCY (see all_routed_sinks_unconnected). */
                src_sync_route_write_silence( op_extra_data, sink_grp, src_w );
                break;

            case SRC_SYNC_SINK_RECOVERING_RESTARTING:
                /* Do not consume input, write maximum silence */
                src_sync_route_write_silence( op_extra_data, sink_grp, src_w );

                /* est_latency has already been updated to include this silence.
                 * Limit amount of initial silence: switch this sink to FLOWING
                 * before the next output buffer full of silence would put
                 * the latency above the target.
                 * src_sync_set_sink_recovering_restarting() ensures that
                 * when in this state, SS_MAX_LATENCY > max_space_t. */
                RATE_SECOND_INTERVAL max_space_t =
                        rate_samples_to_second_interval(op_extra_data->max_space_w,
                                                        op_extra_data->sample_period);
                if (op_extra_data->est_latency
                    >= (src_sync_get_max_latency(op_extra_data) - max_space_t))
                {
                    SOSY_MSG4( SRC_SYNC_TRACE_COMPUTE_TRANSFER,
                               "sink_g%d recovering_restarting el reached 0.%06d target 0.%06d threshold 0.%06d",
                               sink_grp->common.idx,
                               rate_second_interval_to_signed_usec_round(op_extra_data->est_latency),
                               rate_second_interval_to_signed_usec_round(src_sync_get_max_latency(op_extra_data)),
                               rate_second_interval_to_signed_usec_round(src_sync_get_max_latency(op_extra_data)
                                                           - max_space_t));
                    src_sync_set_sink_state( op_extra_data, sink_grp,
                                             SRC_SYNC_SINK_FLOWING);
                }
                break;

            case SRC_SYNC_SINK_RECOVERING_FILLING:
                {
                    /* Write silence up to stall_recovery_silence_words,
                     * subtract silence words written from
                     * stall_recovery_silence_words,
                     * if zero switch to flowing,
                     * if that was less than overall transfer, copy from input
                     */
                    unsigned silence_words =
                            pl_min( sink_grp->stall_recovery_silence_words,
                                    src_w );

                    sink_grp->inserted_silence_words +=
                            src_sync_route_write_silence(
                                    op_extra_data, sink_grp, silence_words );

                    sink_grp->stall_recovery_silence_words -= silence_words;
                    if (sink_grp->stall_recovery_silence_words == 0)
                    {
                        op_extra_data->stat_sink_stalled &=
                                ~ sink_grp->common.channel_mask;
                        src_sync_set_sink_state( op_extra_data, sink_grp,
                                                 SRC_SYNC_SINK_FLOWING );
                    }

                    if (silence_words < src_w)
                    {
                        consumed_via_route_w =
                                src_sync_route_copy(
                                        op_extra_data, sink_grp,
                                        src_w - silence_words );
                    }
                }
                break;

            case SRC_SYNC_SINK_RECOVERING_WAITING_FOR_TAG:
                /* Discard input */

                consumed_via_route_w =
                        src_sync_route_discard_input(
                                op_extra_data, sink_grp, sink_w );

                /* write minimum silence */
                src_sync_route_write_silence( op_extra_data, sink_grp,
                                              src_min_w);
                break;

            case SRC_SYNC_SINK_RECOVERING_DISCARDING:
                /* Discard input up to stall_recovery_discard_words
                 * subtract words discarded from stall_recovery_discard_words
                 * if that becomes zero, and there is still input, and that is
                 * at least minimum transfer, switch to flowing, and copy from
                 * input.
                 * Else write minimum silence, add words written to
                 * stall_recovery_discard_words, subtract silence words
                 * written from stall_recovery_discard_remaining,
                 * if that runs out, switch to restarting
                 */
                {
                    unsigned input_remaining_w;
                    unsigned discard_w =
                            pl_min( sink_w,
                                    sink_grp->stall_recovery_discard_words );

                    consumed_via_route_w =
                            src_sync_route_discard_input(
                                    op_extra_data, sink_grp, discard_w );

                    sink_grp->stall_recovery_discard_words -=
                            consumed_via_route_w;

                    input_remaining_w = sink_w - consumed_via_route_w;

                    if ( (sink_grp->stall_recovery_discard_words == 0)
                         && (input_remaining_w >= src_min_w) )
                    {
                        unsigned copy_w = pl_min(input_remaining_w, src_w);

                        SOSY_MSG1( SRC_SYNC_TRACE_RECOVERED,
                                   "input caught up, copy_w %d",
                                   copy_w);

                        consumed_via_route_w +=
                                src_sync_route_copy( op_extra_data, sink_grp,
                                                     copy_w );

                        op_extra_data->stat_sink_stalled &=
                                ~ sink_grp->common.channel_mask;
                        src_sync_set_sink_state( op_extra_data, sink_grp,
                                                 SRC_SYNC_SINK_FLOWING);
                    }
                    else
                    {
                        src_sync_route_write_silence( op_extra_data, sink_grp,
                                                      src_min_w );

                        sink_grp->stall_recovery_discard_words += src_min_w;

                        if ( sink_grp->stall_recovery_discard_remaining
                             <= src_min_w )
                        {
                            SOSY_MSG( SRC_SYNC_TRACE_RECOVERED,
                                      "input did not catch up, restart");

                            sink_grp->stall_recovery_discard_remaining = 0;
                            src_sync_set_sink_recovering_restarting(
                                    op_extra_data, sink_grp);
                        }
                        else
                        {
                            sink_grp->stall_recovery_discard_remaining -=
                                    src_min_w;

                            SOSY_MSG4( SRC_SYNC_TRACE_RECOVER_DISCARD,
                                       "catchup consume_w %d silence_w %d "
                                       "balance_w %d remain_w %d",
                                       consumed_via_route_w, src_min_w,
                                       sink_grp->stall_recovery_discard_words,
                                       sink_grp
                                           ->stall_recovery_discard_remaining);
                        }
                    }
                }
                break;

            default:
                L2_DBG_MSG1( "src_sync: unhandled sink state %d",
                             sink_grp->stall_state);
                break;
            }

            /* Consume equal amount on unrouted sinks */
            if (consumed_via_route_w > 0)
            {
                SRC_SYNC_SINK_ENTRY* sink_ptr;

                PL_ASSERT(sink_grp->common.connected);

                for ( sink_ptr = sink_entries_from_group(sink_grp);
                      sink_ptr != NULL;
                      sink_ptr = next_sink_entry(sink_ptr) )
                {
                    if (sink_ptr->source == NULL)
                    {
                        PL_ASSERT(sink_ptr->input_buffer != NULL);

                        cbuffer_advance_read_ptr( sink_ptr->input_buffer,
                                                  consumed_via_route_w );
                    }
                }
            }

            if ((sink_grp->back_kick_threshold == 0) && (sink_w < 2*src_w))
            {
                sink_kicks |= sink_grp->common.channel_mask;
            }
        }
        else
        {
            /* Connected and no route destination */
            if (sink_grp->common.connected && sink_grp->purge)
            {
                SRC_SYNC_SINK_ENTRY* sink_ptr;
                unsigned sink_w = sink_grp->transfer_w;
                unsigned sink_octets = sink_w * OCTETS_PER_SAMPLE;

                /* TODO cache as "metadata_connected" */
                if ( sink_grp->common.metadata_enabled
                     && (sink_grp->common.metadata_buffer != NULL))
                {
                    metadata_tag* tags;
                    unsigned before, after;

                    tags = buff_metadata_remove(
                            sink_grp->metadata_input_buffer,
                            sink_octets, &before, &after);

#ifdef SOSY_CHECK_METADATA_TAG_COUNTS
                    unsigned num_tags =
                            src_sync_metadata_count_tags(tags);
                    op_extra_data->num_tags_received += num_tags;
                    op_extra_data->num_tags_deleted += num_tags;
#endif /* SOSY_CHECK_METADATA_TAG_COUNTS */

                    buff_metadata_tag_list_delete(tags);
                }

                for ( sink_ptr = sink_entries_from_group(sink_grp);
                      sink_ptr != NULL;
                      sink_ptr = next_sink_entry(sink_ptr) )
                {
                    PL_ASSERT(sink_ptr->input_buffer != NULL);

                    cbuffer_advance_read_ptr( sink_ptr->input_buffer, sink_w );
                }
            }
        }
    }

#ifdef SOSY_CHECK_METADATA_TRANSPORT_POINTERS
    src_sync_check_md_transport_post(op_extra_data);
#endif /* SOSY_CHECK_METADATA_TRANSPORT_POINTERS */
#ifdef SOSY_CHECK_METADATA_TAG_COUNTS
    src_sync_check_tag_counts(op_extra_data);
#endif /* SOSY_CHECK_METADATA_TAG_COUNTS */

    return sink_kicks & op_extra_data->sinks_connected;
}

/* Allow src_sync_process_data to process again,
 * after updating control data.
 */
void src_sync_resume_processing(OPERATOR_DATA* op_data)
{
    SRC_SYNC_OP_DATA *op_extra_data = get_instance_data(op_data);
    op_extra_data->bRinit = TRUE;
    opmgr_op_resume_processing(op_data);
}

void src_sync_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    SRC_SYNC_OP_DATA *op_extra_data = get_instance_data(op_data);
    RATE_SECOND_INTERVAL min_period;
    bool routes_changed;

    patch_fn_shared(src_sync);

    timer_cancel_event_atomic(&op_extra_data->kick_id);

#ifdef VERBOSE_PROCESS_DATA
    static unsigned int serial = 0;
    SOSY_PD_MSG5("src_sync 0x%04x pd #%d t %d el 0.%05d rinit %d",
                 op_extra_data->id, serial, time_get_time(),
                 src_sync_sec_frac_to_10usec(op_extra_data->est_latency),
                 op_extra_data->bRinit);
    serial += 1;
#endif /* VERBOSE_PROCESS_DATA */

    routes_changed = src_sync_perform_transitions_if_needed(op_extra_data);

    /* Check for Configuration change */
    if (routes_changed || op_extra_data->bRinit)
    {
        op_extra_data->bRinit = FALSE;

        src_sync_update_derived(op_extra_data);

        /* Update state of all routes */
        src_sync_refresh_forward_routes(op_extra_data);
        src_sync_refresh_sink_list(op_extra_data);
        src_sync_refresh_source_list(op_extra_data);
        src_sync_refresh_metadata_routes(op_extra_data);
        src_sync_refresh_connections(op_extra_data);
        src_sync_refresh_downstream_probe(op_extra_data);

        /* Wakeup sinks just in case */
        touched->sinks = op_extra_data->sinks_connected;

        /* Fixups */
        patch_fn_shared(src_sync);
    }

    src_sync_pre_check_back_kick(op_extra_data);

    SRC_SYNC_COMP_CONTEXT compute_context;
    /* src_sync_compute_transfer_space returns 0 there
     * is not enough to do this time (limited by output space),
     * or a positive value to reschedule, or a negative one to proceed
     */
    min_period = src_sync_compute_transfer_space(op_extra_data, &compute_context);
    if (min_period != 0)
    {
        if (min_period < 0)
        {
            /* Pre-process rate-adjusted sink groups.
             * For those groups, the later steps work with
             * the output of rate adjustment.
             */
            src_sync_rm_process(op_extra_data, &touched->sinks);

            /* Check sinks and compute the transfer amount
            min_period == 0 if insufficient space
            min_period < 0 if data to transfer
            min_period > 0 if waiting for data
            */
            min_period = src_sync_compute_transfer_sinks(op_extra_data, &compute_context);
        }

        if (min_period > 0)
        {
            SOSY_MSG2( SRC_SYNC_TRACE_PERFORM_TRANSFER,
                       "calc min_p 0.%06d el 0.%06d",
                       rate_second_interval_to_signed_usec_round(min_period),
                       rate_second_interval_to_signed_usec_round(op_extra_data->est_latency));
            /* Set a timer to kick task before stall causes glitch in channels */
            timer_schedule_event_in_atomic(
                rate_second_interval_to_signed_usec_round(min_period),
                src_sync_timer_task,
                (void*)op_data,
                &(op_extra_data->kick_id));

            /* If we are waiting for a sink, lets kick it */
            SRC_SYNC_SINK_GROUP* sink_grp;
            for ( sink_grp = op_extra_data->sink_groups;
                  sink_grp != NULL;
                  sink_grp = next_sink_group(sink_grp) )
            {
                if (sink_grp->stall_state == SRC_SYNC_SINK_PENDING)
                {
                    touched->sinks |= sink_grp->common.channel_mask;
                }
            }

            /* If source buffers are nearly full, remind consumer */
            if (compute_context.downstream_filled)
            {
                touched->sources |= op_extra_data->sources_connected;
            }
        }
        else if (min_period < 0)
        {
            SOSY_MSG2( SRC_SYNC_TRACE_PERFORM_TRANSFER,
                       "calc min_p -0.%06d el 0.%06d",
                       rate_second_interval_to_signed_usec_round(-min_period),
                       rate_second_interval_to_signed_usec_round(op_extra_data->est_latency));
            /* Perform sink to source transfers */
            touched->sinks   |= src_sync_perform_transfer(op_extra_data);
            touched->sources = op_extra_data->forward_kicks;
        }
    }

    touched->sinks |= src_sync_post_check_back_kick(op_extra_data);

    SOSY_MSG3( SRC_SYNC_TRACE_KICK, "at exit el 0.%06d kick bwd 0x%06x fwd 0x%06x",
               rate_second_interval_to_signed_usec_round(op_extra_data->est_latency),
               touched->sinks, touched->sources);
#ifdef INSTALL_TIMING_TRACE
    opmgr_record_timing_trace_op_event(
        op_extra_data->id,
        SRC_SYNC_TIMING_EVENT_SOURCE_STATE,
        op_extra_data->est_latency,
        ((compute_context.max_transfer_w >> 16) << 16) |
        (compute_context.min_transfer_w >> 16) );
#endif
}

