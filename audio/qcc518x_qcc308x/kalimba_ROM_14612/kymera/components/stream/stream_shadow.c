/****************************************************************************
 * Copyright (c) 2011 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stream_shadow.c
 * \ingroup stream
 *
 * stream shadow type file. <br>
 * This file contains stream functions for shadow
 * endpoints. <br>
 *
 */

/****************************************************************************
Include Files
*/

#include "stream_private.h"
#include "opmgr/opmgr_endpoint_override.h"
#include "opmgr/opmgr_for_stream.h"
#include "ipc/ipc.h"
#include "kip_mgr/kip_mgr.h"
#include "platform/pl_assert.h"
#include "buffer/buffer_metadata_kip.h"

/****************************************************************************
Private Type Declarations
*/

/****************************************************************************
Private Constant Declarations
*/
/* Provide the default buffer size in words */
#define DEFAULT_SHADOW_BUFFER_SIZE   128
#define DEFAULT_SHADOW_BLOCK_SIZE      1
#define DEFAULT_SHADOW_PERIOD          0

/****************************************************************************
Private Macro Declarations
*/

#if !defined(COMMON_SHARED_HEAP)
#define SHADOW_EP_CLONE_REMOTE_BUFFER(ep, clone) (clone)? \
                                                  shadow_clone_ipc_data_buffer(ep) : \
                                                  shadow_free_cloned_ipc_data_buffer(ep)
#endif /* !COMMON_SHARED_HEAP */

/****************************************************************************
Private Variable Definitions
*/

/****************************************************************************
Private Function Declarations
*/

static unsigned shadow_create_stream_key(unsigned int epid);
static bool shadow_connect(ENDPOINT *endpoint, tCbuffer *Cbuffer_ptr, ENDPOINT* connected_wants_kicks, bool* start_on_connect);
static bool shadow_disconnect (ENDPOINT *endpoint);
static bool shadow_buffer_details (ENDPOINT *ep, BUFFER_DETAILS *details);
static bool shadow_configure (ENDPOINT *endpoint, unsigned int key, uint32 value);
static bool shadow_get_config (ENDPOINT *endpoint, unsigned int key, ENDPOINT_GET_CONFIG_RESULT* result);
static void shadow_get_timing (ENDPOINT *endpoint, ENDPOINT_TIMING_INFORMATION *time_info);
static void shadow_kick(ENDPOINT *ep, ENDPOINT_KICK_DIRECTION kick_dir);
static bool shadow_start(ENDPOINT *ep, KICK_OBJECT *ko);
static bool shadow_stop(ENDPOINT *ep);
static bool shadow_close(ENDPOINT *ep);
static bool shadow_sync_sids(ENDPOINT *ep1, ENDPOINT *ep2);
#if !defined(COMMON_SHARED_HEAP)
static bool shadow_free_cloned_ipc_data_buffer(ENDPOINT *ep);
static bool shadow_clone_ipc_data_buffer(ENDPOINT *ep);
#endif /* !COMMON_SHARED_HEAP */
static bool shadow_set_state_buffer(ENDPOINT *ep);

DEFINE_ENDPOINT_FUNCTIONS (shadow_functions, shadow_close, shadow_connect,
                           shadow_disconnect, shadow_buffer_details,
                           shadow_kick, stream_sched_kick_dummy,
                           shadow_start, shadow_stop,
                           shadow_configure, shadow_get_config,
                           shadow_get_timing, shadow_sync_sids,
                           stream_have_same_clock_common);

/**
 * \brief Get EP ptr from key - in this case key is the ID
 *
 * \param key                   Key to use.
 *
 * \return If successful, pointer to a shadow endpoint object
 *         that matches key. If error, NULL
 */
static ENDPOINT *stream_shadow_get_endpoint_from_key(unsigned key)
{
    /* Derive the direction from the key which in the case of shadow
     * endpoints is the same as the epid.
     */
    ENDPOINT_DIRECTION dir = (key & STREAM_EP_SINK_BIT) ? SINK : SOURCE;

    return stream_get_endpoint_from_key_and_functions(key, dir,
                                                  &endpoint_shadow_functions);
}

/**
 * \brief   Raise a signal on a channel associated with an endpoint
 *
 * \param   ep          The endpoint
 * \param   channel_id  The channel identifier
 *
 * \return  True if raising the signal was successful.
 */
static bool stream_shadow_raise_ipc_signal(ENDPOINT *ep, uint16 channel_id)
{
    PROC_ID_NUM proc_id;
    KIP_SIGNAL_ID_KALIMBA signal_id;

    if (PROC_PRIMARY_CONTEXT())
    {
        proc_id = PROC_PROCESSOR_1;
    }
    else
    {
        proc_id = PROC_PROCESSOR_0;
    }

    signal_id = (ep->direction == SINK) ? KIP_SIGNAL_ID_KICK :
                                          KIP_SIGNAL_ID_REVERSE_KICK;

    return (ipc_raise_signal(proc_id, signal_id, channel_id) == IPC_SUCCESS);
}


/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Internal function to get KIP endpoint from endpoint ID
 *
 * \param epid                  Endpoint id to use.
 *
 * \return If successful, pointer to a shadow endpoint object.
 *         If error, NULL
 */
ENDPOINT *stream_shadow_get_endpoint(unsigned int epid)
{
    ENDPOINT *ep = NULL;

    unsigned key = shadow_create_stream_key(epid);
    if (key != 0)
    {
        ep = stream_shadow_get_endpoint_from_key(key);
    }

    return ep;
}

/**
 * \brief Internal function to create the KIP endpoint
 *
 * \param epid                  Endpoint id to use.
 * \param con_id                Connection id
 *
 * \return If successful, pointer to a shadow endpoint object.
 *         If error, NULL
 */
ENDPOINT *stream_create_shadow_endpoint(unsigned epid, CONNECTION_LINK con_id)
{
    ENDPOINT *ep;
    ENDPOINT_DIRECTION dir;
    patch_fn_shared(stream_shadow);

    if (epid == 0)
    {
        /* Shadow endpoint must be associated with an existing epid */
        return NULL;
    }

    /* If EP ID is being imposed, get dir from it rather than argument,
     * although latter should match... */
    dir = (epid & STREAM_EP_SINK_BIT) ? SINK : SOURCE;

    if ((ep = STREAM_NEW_ENDPOINT(shadow, epid, dir, con_id)) != NULL)
    {

        PL_PRINT_P2(TR_PL_TEST_TRACE, "Created endpoint: %d for ep id: %d\n",
                                   (unsigned)(uintptr_t) ep, epid);

        /* Shadow endpoints cannot be closed explicitly */
        ep->can_be_closed = FALSE;

        /* it is neither a real endpoint nor a operator endpoint */
        ep->is_real = FALSE;

        /* shadow endpoints must be distroyed always on disconnect */
        ep->destroy_on_disconnect = TRUE;

        /* shadow buffer default size */
        ep->state.shadow.buffer_size = DEFAULT_SHADOW_BUFFER_SIZE;

        /* Point the head_of_sync to itself; could be overwritten later. */
        ep->state.shadow.head_of_sync = ep;

        /* Next endpoint in sync set to NULL. */
        ep->state.shadow.nep_in_sync = NULL;
    }

    return ep;
}

void stream_enable_shadow_endpoint(unsigned epid)
{
    ENDPOINT *ep;
    unsigned shadow_id = STREAM_GET_SHADOW_EP_ID(epid);

    if ((ep = stream_endpoint_from_extern_id(shadow_id)) != NULL)
    {
        /* stream_enable_endpoint will start any real endpoint that ep is connected
         * to. The shadow endpoint needs to be marked as enabled to. */
        stream_enable_endpoint(ep);
        ep->is_enabled = TRUE;
    }
}

void stream_disable_shadow_endpoint(unsigned epid)
{
    ENDPOINT *ep;
    unsigned shadow_id = STREAM_GET_SHADOW_EP_ID(epid);

    if ((ep = stream_endpoint_from_extern_id(shadow_id)) != NULL)
    {
        /* Stream_disable_endpoint will stop any real endpoint that ep
         * is connected to. The shadow endpoint needs to be marked as
         * disabled to. */
        stream_disable_endpoint(ep);
        ep->is_enabled = FALSE;
    }
}

/**
 * \brief Internal function to close the KIP endpoint by disconnecting
 *        and destroying the IPC channel (between P0 and P1).
 *
 * \param ep                    Pointer to the endpoint.
 *
 * \return TRUE if successfull, FALSE if not.
 */
static bool shadow_close(ENDPOINT *ep)
{
    bool success = FALSE;
    endpoint_shadow_state *state = &ep->state.shadow;

    if (ep != NULL)
    {
        success = TRUE;

        /* Overwrite the closeable flag */
        ep->can_be_closed = TRUE;

        if (state->channel_id != 0)
        {
            success = stream_kip_data_channel_destroy(ep->state.shadow.channel_id);
        }
#if !defined(COMMON_SHARED_HEAP)
        if (success)
        {
            if ((state->supports_metadata == TRUE) && (state->meta_channel_id != 0))
            {
                if (stream_kip_is_last_meta_connection(ep))
                {
                    success = stream_kip_data_channel_destroy_ipc(state->meta_channel_id);
                    if (!success)
                    {
                        return success;
                    }
                    else
                    {
                        state->supports_metadata = FALSE;
                        state->meta_channel_id = 0;
                    }
                }
            }
        }
#endif /* !COMMON_SHARED_HEAP */
        /* Make sure we don't try to destroy it twice */
        ep->destroy_on_disconnect = FALSE;
        state->channel_id = 0;
    }

    return success;
}

/**
 * \brief Internal function to destroy the KIP endpoint.
 *
 * \param epid The external endpoint id
 *
 * \return TRUE if successful, FALSE upon error
 */
bool stream_destroy_shadow_endpoint(unsigned epid)
{
    ENDPOINT *ep = stream_endpoint_from_extern_id(epid);
    bool success = shadow_close(ep);

    if (success)
    {
        success = stream_close_endpoint(ep);
    }

    return success;
}

#if !defined(COMMON_SHARED_HEAP)
/**
 * \brief Set the shared metadata buffer for a shadow endpoint
 *
 * \param kip_ep                Pointer to the shadow endpoint
 * \param shared_metadata_buf   The shared metadata buffer for the
 *                              shadow endpoint (kip_ep) to set
 */
void stream_shadow_set_shared_metadata_buffer(ENDPOINT *kip_ep, tCbuffer* shared_metadata_buf)
{
    kip_ep->state.shadow.metadata_shared_buf = shared_metadata_buf;
}

/**
 * \brief Returns the shared metadata buffer for a shadow endpoint
 *
 * \param kip_ep                Pointer to the shadow endpoint
 *
 * \return The shared metadata buffer for the shadow endpoint
 */
tCbuffer* stream_shadow_get_shared_metadata_buffer(ENDPOINT *kip_ep)
{
    return kip_ep->state.shadow.metadata_shared_buf;
}
#endif /* !COMMON_SHARED_HEAP */

/**
 * \brief Returns the local metadata buffer for a shadow endpoint
 *
 * \param kip_ep                Pointer to the shadow endpoint
 *
 * \return The local metadata buffer for the shadow endpoint
 */
tCbuffer* stream_shadow_get_metadata_buffer(ENDPOINT *kip_ep)
{
    return kip_ep->state.shadow.buffer;
}

/**
 * \brief Returns the IPC channel id for a shadow endpoint
 *
 * \param kip_ep                Pointer to the shadow endpoint
 *
 * \return The IPC channel id for the shadow endpoint
 */
uint16 stream_shadow_get_channel_id(ENDPOINT *kip_ep)
{
    return kip_ep->state.shadow.channel_id;
}

/****************************************************************************
Private Function Definitions
*/

/**
 * \brief Connect to the endpoint.
 *
 * \param endpoint         Pointer to the endpoint that is being connected
 * \param cbuffer_ptr      Pointer to the Cbuffer struct for the buffer
 *                         that is being connected.
 * \param ep_to_kick       Pointer to the endpoint which will be kicked
 *                         after a successful run. Note: this can be
 *                         different from the connected to endpoint when
 *                         in-place running is enabled.
 * \param start_on_connect Return flag which indicates if the endpoint
 *                         wants be started on connect. Note: The endpoint
 *                         will only be started if the connected to
 *                         endpoint wants to be started too.
 *
 * \return TRUE if successfull, FALSE if not.
 */
static bool shadow_connect(ENDPOINT *endpoint,
                           tCbuffer *cbuffer_ptr,
                           ENDPOINT *ep_to_kick,
                           bool *start_on_connect)
{
    endpoint_shadow_state *state = &endpoint->state.shadow;

    /* If the buffer is already cloned locally for the data channel
     * the provided buffer is expected to be same
     */
    if (state->buffer == NULL)
    {
        state->buffer = cbuffer_ptr;
    }

    /* The shadow state buffer present must be
     * same as the Cbuffer provided in connect.
     * unexpected error
     */
    if (state->buffer != cbuffer_ptr)
    {
        return FALSE;
    }

    endpoint->ep_to_kick = ep_to_kick;
    *start_on_connect = FALSE;

#ifdef INSTALL_TIMING_TRACE
    timing_trace_instr_instrument_endpoint(endpoint);
#endif /* INSTALL_TIMING_TRACE */

    return TRUE;
}

/*
 * \brief Disconnects from an endpoint and stops the data from flowing
 *
 * \param *endpoint             Pointer to the endpoint that is being disconnected
 *
 * \return TRUE if successfull, FALSE if not.
 */
static bool shadow_disconnect(ENDPOINT *endpoint)
{
    bool success = FALSE;
    endpoint_shadow_state *state = &endpoint->state.shadow;
#if !defined(COMMON_SHARED_HEAP)
    if ((state->supports_metadata == TRUE) &&
       (state->meta_channel_id != 0))
    {
        if (stream_kip_is_last_meta_connection(endpoint))
        {
            success = stream_kip_data_channel_deactivate_ipc(state->meta_channel_id);
            if (!success)
            {
                return success;
            }
            else
            {
                state->metadata_shared_buf = NULL;
            }
        }
    }
#endif /* !COMMON_SHARED_HEAP */

    success = stream_kip_data_channel_deactivate(state->channel_id);
#if !defined(COMMON_SHARED_HEAP)
    if (success)
    {
        success = shadow_free_cloned_ipc_data_buffer(endpoint);
    }
#endif /* COMMON_SHARED_HEAP */
    return success;
}

/*
 * \brief Obtains details of the buffer required for this connection
 *
 * \param *ep                   Pointer to the endpoint from which the buffer
 *                              information is required
 * \param *details              Pointer to buffer in which to deposit buffer
 *                              information
 *
 * \return TRUE if successfull, FALSE if not.
 */
static bool shadow_buffer_details(ENDPOINT *ep, BUFFER_DETAILS *details)
{
    endpoint_shadow_state *state = &ep->state.shadow;

    /* Even though shadow endpoints commits to supply the buffer
     * It doesn't own any buffer now which will be updated while
     * activating the data channel.
     */
    details->can_override = FALSE;
    details->wants_override = FALSE;
    details->runs_in_place = FALSE;

    if (ep->direction == SOURCE)
    {
        /* Activating the data channel happens before connect */
        details->b.buffer = state->buffer;
        /* we don't expect the buffer to be NULL */
        PL_ASSERT(details->b.buffer != NULL);
        /* Failed to provide a buffer, return early */
        if (details->b.buffer == NULL)
        {
            return FALSE;
        }

        details->supplies_buffer = TRUE;
    }
    else
    {
        details->b.buff_params.flags = BUF_DESC_SW_BUFFER;
        details->b.buff_params.size = state->buffer_size;
        details->supplies_buffer = FALSE;
    }
    details->supports_metadata = state->supports_metadata;
    details->metadata_buffer = NULL;

    if (details->supports_metadata)
    {
        details->metadata_buffer = stream_kip_return_metadata_buf(ep);
    }

    return TRUE;
}

#if !defined(COMMON_SHARED_HEAP)
/*
 * \brief Free the cloned ipc data cbuffer structure
 *
 * \param *ep                   Pointer to the shadow endpoint
 *
 * \return TRUE if successfull, FALSE if not.
 */
static bool shadow_free_cloned_ipc_data_buffer(ENDPOINT *ep)
{
    endpoint_shadow_state *state = &ep->state.shadow;

    if (ep->direction == SOURCE && state->buffer != NULL)
    {
        cbuffer_destroy_struct(state->buffer);
    }
    state->buffer = NULL;
    return TRUE;
}

/*
 * \brief Create a clone cbuffer structure of the remote buffer
 *
 * \param *ep                   Pointer to the shadow endpoint
 *
 * \return TRUE if successfull, FALSE if not.
 */
static bool shadow_clone_ipc_data_buffer(ENDPOINT *ep)
{
    endpoint_shadow_state *state = &ep->state.shadow;
    bool retval = FALSE;

    /* Clone the cbuffer structure only if there is no buffer associated */
    if (state->buffer == NULL)
    {
        tCbuffer* cbuffer = ipc_data_channel_get_cbuffer(state->channel_id);

        /* Not expecting the channel buffer to be NULL */
        STREAM_KIP_ASSERT(cbuffer != NULL);

        /* provide a cloned cbuffer struct.
         * The same buffer will be supplied back during stream connect.
         * If it is being overridden at that point, we will use the new
         * supplied buffer.
         */
        if (cbuffer != NULL)
        {
            state->buffer = cbuffer_create(cbuffer->base_addr,
                                             cbuffer_get_size_in_words(cbuffer),
                                             BUF_DESC_SW_BUFFER);
            if (state->buffer != NULL)
            {

                cbuffer_buffer_sync(state->buffer, cbuffer);
                state->buffer_size = (uint16)(cbuffer_get_size_in_words(state->buffer));
                retval = TRUE;
            }
            else
            {
                L2_DBG_MSG("**Failed to create a cloned shadow cbuffer structure**");
            }

        }
        else
        {
            L2_DBG_MSG("**Cannot clone shadow cbuffer structure that does not exist**");
        }

    }
    return retval;
}
#endif /* !COMMON_SHARED_HEAP */

static bool shadow_set_state_buffer(ENDPOINT *ep)
{
    endpoint_shadow_state *state = &ep->state.shadow;
    bool retval = FALSE;

    /* Set the cbuffer structure only if there is no buffer associated */
    if (state->buffer == NULL)
    {
        tCbuffer* cbuffer = ipc_data_channel_get_cbuffer(state->channel_id);

        /* Not expecting the channel buffer to be NULL */
        PL_ASSERT(cbuffer != NULL);

        /* Set the buffer and the buffer size to the shadow state.
         */
        state->buffer = cbuffer;
        state->buffer_size = (uint16)(cbuffer_get_size_in_words(state->buffer));
        retval = TRUE;

    }

    return retval;
}

/*
 * \brief Configure an shadow endpoint with a key and value pair
 *
 * \param endpoint Pointer to the endpoint to be configured.
 * \param key      Denoting what is being configured.
 * \param value    Pointer to a value to which the key is to be configured.
 *
 * \return TRUE if successfull, FALSE if not.
 *
 * \note Not all configurations are currenly accepted.
 */
static bool shadow_configure(ENDPOINT *endpoint,
                             unsigned int key,
                             uint32 value)
{
    switch (key)
    {
        case EP_DATA_FORMAT:
        {
            endpoint->state.shadow.data_format = (AUDIO_DATA_FORMAT) value;
            return TRUE;
        }
        case EP_OVERRIDE_ENDPOINT:
        {
            /* This feature is not supported for the shadow endpoint */
            return FALSE;
        }
        case EP_SET_DATA_CHANNEL:
        {
            endpoint->state.shadow.channel_id = (uint16) value;
            return TRUE;
        }
        case EP_SET_SHADOW_STATE_BUFFER:
        {
            return shadow_set_state_buffer(endpoint);
        }
#if !defined(COMMON_SHARED_HEAP)
        case EP_CLONE_REMOTE_BUFFER:
        {
            return SHADOW_EP_CLONE_REMOTE_BUFFER(endpoint, (value != 0));
        }
#endif /* COMMON_SHARED_HEAP */
        case EP_SET_SHADOW_BUFFER_SIZE:
        {
            endpoint->state.shadow.buffer_size = (uint16) value;
            return TRUE;
        }
        case EP_METADATA_SUPPORT:
        {
            endpoint->state.shadow.supports_metadata = (bool) value;
            return TRUE;
        }
#if !defined(COMMON_SHARED_HEAP)
        case EP_METADATA_CHANNEL_ID:
        {
            tCbuffer *buf;

            endpoint->state.shadow.meta_channel_id = (uint16) value;
            buf = ipc_data_channel_get_cbuffer((uint16) value);
            endpoint->state.shadow.metadata_shared_buf = buf;
            return TRUE;
        }
#endif /* !COMMON_SHARED_HEAP */
        default:
        {
            return FALSE;
        }
    }
}

/*
 * \brief Get shadow endpoint configuration
 *
 * \param *endpoint             Pointer to the endpoint to be configured
 * \param key                   Denoting what is being configured
 * \param value                 Pointer to a value which is populated with
 *                              the current value
 *
 * \return TRUE if successfull, FALSE if not.
 */
static bool shadow_get_config(ENDPOINT *endpoint, unsigned int key, ENDPOINT_GET_CONFIG_RESULT* result)
{
    switch(key)
    {
    case EP_DATA_FORMAT:
        result->u.value = (uint32)endpoint->state.shadow.data_format;
        return TRUE;

    case EP_METADATA_SUPPORT:
        result->u.value = (uint32)endpoint->state.shadow.supports_metadata;
         return TRUE;

#if !defined(COMMON_SHARED_HEAP)
    case EP_METADATA_CHANNEL_ID:
        result->u.value = (uint32)endpoint->state.shadow.meta_channel_id;
         return TRUE;
#endif /* !COMMON_SHARED_HEAP */

    case EP_PROC_TIME:
    case EP_RATEMATCH_ABILITY:
    case EP_RATEMATCH_RATE:
    case EP_RATEMATCH_MEASUREMENT:

    default:
        return FALSE;
    }
}

/*
 * \brief Generates an shadow key (same as an shadow endpoint id) from an
 *        shadow id, index and direction
 *
 * \param epid                  The shadow ep id (NB must be the PUBLIC opid)
 *
 * \return generated shadow key
 */
static unsigned shadow_create_stream_key(unsigned int epid)
{
    return epid;
}

/*
 * \brief Get the timing requirements of this shadow endpoint
 *
 * \param endpoint              The endpoint
 * \param time_info             A pointer to an ENDPOINT_TIMING_INFORMATION
 *                              structure supplied by the caller to populate
 *                              with the endpoint's timing information.
 */
static void shadow_get_timing(ENDPOINT *endpoint, ENDPOINT_TIMING_INFORMATION *time_info)
{
    time_info->block_size = 0;

    /* Locally clocked can only be not true for real op endpoints */
    time_info->locally_clocked = TRUE;
    time_info->is_volatile = FALSE;
    time_info->wants_kicks = TRUE;

    return;
}

/**
 * \brief  Kick the shadow endpoint
 *
 * \param ep                    The endpoint
 * \param kick_dir              The kick direction
 *
 * Kick via KIP signals that yell data produced/consumed to the other end.
 * Latter events will cause data read/write at the other end in the respective
 * handlers. when implicit sync'ing is introduced for terminals of same operator,
 * a kick to any shadow EP data channel in a certain IPC port results in kick
 * across all shadow EPs of all data channels in that port!
 */
static void shadow_kick(ENDPOINT *ep, ENDPOINT_KICK_DIRECTION kick_dir)
#if defined(COMMON_SHARED_HEAP)
{
    patch_fn_shared(stream_shadow);

    endpoint_shadow_state *shadow = &ep->state.shadow;
    uint16 channel_id = shadow->channel_id;

    if (ep != ep->state.shadow.head_of_sync)
    {
        return;
    }

    if (!ep->is_enabled)
    {
        /* There is a potential race between the cores, so we can get kicked
         * when things are getting torn down so check the running state and
         * exit if we shouldn't be here. */
        return;
    }

    /* This can be called from multiple contexts. It's not re-entrant so flag it
     * and return. The kick call that is executing will check when it finishes
     * and will run again. */
    if (shadow->kick_in_progress)
    {
        shadow->kick_blocked = kick_dir;
        return;
    }

#ifdef INSTALL_TIMING_TRACE
    bool is_called =
            kick_dir ==
                    ((ep->direction == SOURCE) ?
                    STREAM_KICK_BACKWARDS :
                    STREAM_KICK_FORWARDS);

    timing_trace_record_called(ep->id, is_called);
#endif /* INSTALL_TIMING_TRACE */

    do
    {
        /* Set kick_blocked to false first so that we know when we finish this
         * iteration if another kick came in whilst running. */
        shadow->kick_blocked = STREAM_KICK_INVALID;
        shadow->kick_in_progress = TRUE;

        if (ep->direction == SINK)
        {
            if (kick_dir == STREAM_KICK_FORWARDS)
            {
                /*  kick forward */
                shadow->remote_kick = TRUE;
            }
            else
            {
                propagate_kick(ep, STREAM_KICK_BACKWARDS);
            }
        }
        else /* source */
        {
            if (kick_dir == STREAM_KICK_BACKWARDS)
            {
                /*  kick backwards */
                shadow->remote_kick = TRUE;
            }
            else
            {
                propagate_kick(ep, STREAM_KICK_FORWARDS);
            }
        }
        shadow->kick_in_progress = FALSE;
        /* It's possible for an interrupt to fire here, and service kick_blocked
         * and the loop will run again with nothing to do. This should happen
         * very rarely. */
        /* We need to update kick direction so that a signal is raised if it
         * should have been given where the blocked kick originated from. */
        kick_dir = shadow->kick_blocked;
    } while (kick_dir != STREAM_KICK_INVALID);

    /* Raise the signal for Kick only if the transform is present in the kip transform
     * list and if it is enabled
     */
    if (shadow->remote_kick)
    {
        /* if raising the signal failed, attempt it again in the next kick */
        if (stream_shadow_raise_ipc_signal(ep, channel_id))
        {
            shadow->remote_kick = FALSE;
        }
    }
#ifdef INSTALL_TIMING_TRACE
    timing_trace_instr_stream_record_end_ep_run(ep, is_called);
#endif /* INSTALL_TIMING_TRACE */
}
#else
{
    patch_fn_shared(stream_shadow);

    endpoint_shadow_state *shadow = &ep->state.shadow;
    tCbuffer *buffer = shadow->buffer;
    uint16 channel_id = shadow->channel_id;

    ENDPOINT *synced_ep;
    tCbuffer *shadow_buffer;
    uint16 sync_chan_id;
    unsigned data_before;
    unsigned data_after;

    if (ep != ep->state.shadow.head_of_sync)
    {
        return;
    }

    if (!ep->is_enabled)
    {
        /* There is a potential race between the cores, so we can get kicked
         * when things are getting torn down so check the running state and
         * exit if we shouldn't be here. */
        return;
    }

    /* This can be called from multiple contexts. It's not re-entrant so flag it
     * and return. The kick call that is executing will check when it finishes
     * and will run again. */
    if (shadow->kick_in_progress)
    {
        shadow->kick_blocked = kick_dir;
        return;
    }

#ifdef INSTALL_TIMING_TRACE
    bool is_called =
            kick_dir ==
                    ((ep->direction == SOURCE) ?
                    STREAM_KICK_BACKWARDS :
                    STREAM_KICK_FORWARDS);

    timing_trace_record_called(ep->id, is_called);
#endif /* INSTALL_TIMING_TRACE */

    do
    {
        /* Set kick_blocked to false first so that we know when we finish this
         * iteration if another kick came in whilst running. */
        shadow->kick_blocked = STREAM_KICK_INVALID;
        shadow->kick_in_progress = TRUE;

        if (ep->direction == SINK)
        {
            if ((shadow->metadata_shared_buf != NULL) && (buffer->metadata != NULL))
            {
                LOCK_INTERRUPTS;
                metadata_tag* tag = buffer->metadata->tags.head;

                /**
                 * In addition to moving tags, we also need to provide an update
                 * of prev_wr_index for the other core. If we succeed in pushing
                 * all the tags to the KIP buffer this will be as simple as
                 * writing the current value prev_wr_index to the shared memory
                 * but when there's not enough space in the KIP buffer, we should
                 * pass a prev_wr_index that reflects the status of tags in the
                 * KIP buffer. This can be achieved by using the return value of
                 * buff_metadata_push_tags_to_KIP(), which is the first tag we
                 * *failed* to copy to the KIP buffer. The index of this tag is
                 * effectively the write index of the tags we have successfully
                 * copied to the KIP buffer. Let's start by assuming success and
                 * adjust in case of failure:
                 */
                unsigned effective_wr_index = buffer->metadata->prev_wr_index;

                if (tag != NULL)
                {
                    metadata_tag* head_tag = buff_metadata_push_tags_to_KIP(shadow->metadata_shared_buf, tag);

                    /**
                     * We only need to update tail if above returns NULL which means
                     * we've successfully pushed ALL tags to KIP buffer.
                     */
                    buffer->metadata->tags.head = head_tag;

                    if (head_tag == NULL)
                    {
                        /** We've successfully copied everything */
                        buffer->metadata->tags.tail = NULL;
                    }
                    else
                    {
                        /**
                         * If here, head_tag points to the tag we failed to copy
                         * to the KIP buffer - we can use its write index for
                         * syncing.
                         */
                        effective_wr_index = head_tag->index;
                    }
                }

                KIP_METADATA_BUFFER* kip_metadata_buf = (KIP_METADATA_BUFFER*)shadow->metadata_shared_buf;

                /* Provide an update to the other side */
                kip_metadata_buf->prev_wr_index = effective_wr_index;

                /* Pick up from the other side */
                buffer->metadata->prev_rd_index = kip_metadata_buf->prev_rd_index;

                UNLOCK_INTERRUPTS;
            }
            unsigned data = cbuffer_calc_amount_data_in_words (buffer);
            /* To determine how much data has been written (for the other side)
             * in order to decide to kick forward or not, the data channel
             * buffer needs to be monitored since the read pointer should be
             * up to date before and after, and only the write pointer is
             * updated. */
            tCbuffer *data_chan_buff;
            data_chan_buff = ipc_data_channel_get_cbuffer(channel_id);

            /* Save the data before the buffer's write pointer is synced. */
            data_before = cbuffer_calc_amount_data_in_words(data_chan_buff);

            /* Sync the buffer pointers for all the members of the sync list. */
            for (synced_ep = ep; synced_ep != NULL; synced_ep = synced_ep->state.shadow.nep_in_sync)
            {
                shadow_buffer = synced_ep->state.shadow.buffer;
                sync_chan_id = synced_ep->state.shadow.channel_id;
                /* write through IPC and update the read pointer as well
                 * if some data was already consumed.
                 * last tag index used to adjust dst wrp if not all tags copied over
                 * (space limitation) */
                if (buffer->metadata->tags.head != NULL)
                {
                    ipc_data_channel_write_sync(shadow_buffer, sync_chan_id,
                                         buffer->metadata->tags.head->index);
                }
                else
                {
                    ipc_data_channel_write_sync(shadow_buffer, sync_chan_id, -1);   /* negative means "no limitation" */
                }
            }

            /* Save the data after the buffer's write pointer is synced.
             * If data has been written, kick forward.*/
            data_after = cbuffer_calc_amount_data_in_words(data_chan_buff);
            if (data_before < data_after)
            {
                /*  kick forward */
                shadow->remote_kick = TRUE;
            }

            /* If there is less data in the buffer after write sync
             * to data channel,
             * means the data has been read as well.
             * So we need to kick backwards and get more data.
             */
            if ((data <= DEFAULT_SHADOW_BLOCK_SIZE) ||
                (data > cbuffer_calc_amount_data_in_words (buffer)))
            {
                propagate_kick(ep, STREAM_KICK_BACKWARDS);
            }
        }
        else /* source */
        {
            /* Source should synchronise the data first and then do metadata */
            /* Save the data before the buffer's write pointer is synced. */
            data_before = cbuffer_calc_amount_data_in_words(buffer);
            /* Sync the buffer pointers for all the members of the sync list. */
            for (synced_ep = ep; synced_ep != NULL; synced_ep = synced_ep->state.shadow.nep_in_sync)
            {
                shadow_buffer = synced_ep->state.shadow.buffer;
                sync_chan_id = synced_ep->state.shadow.channel_id;
                ipc_data_channel_read_sync(sync_chan_id, shadow_buffer);
            }

            /* Save the data after the buffer's write pointer is synced.
             * If data has been written in the buffer, kick forward.*/
            data_after = cbuffer_calc_amount_data_in_words(buffer);

            if (shadow->metadata_shared_buf != NULL)
            {
                KIP_METADATA_BUFFER* kip_metadata_buf = (KIP_METADATA_BUFFER*)shadow->metadata_shared_buf;
                metadata_tag *head_tag, *tail_tag;

                LOCK_INTERRUPTS;
                head_tag = buff_metadata_pop_tags_from_KIP(shadow->metadata_shared_buf, &tail_tag);

                if (buffer->metadata == NULL)
                {
                    /**
                     * This condition is here to make dual-core behave similar to
                     * buff_metadata_append() function which throws tags away if
                     * there's no metadata list in the destination buffer. This
                     * is not great. Ideally we'd like to keep the logic here as
                     * dumb as possible so that it only moves tags across cores.
                     */
                    buff_metadata_tag_list_delete(head_tag);

                    /**
                     * Provide an update to the other side: all tags were
                     * consumed.
                     */
                    kip_metadata_buf->prev_rd_index = kip_metadata_buf->prev_wr_index;
                }
                else
                {
                    /* Pick up from the other side */
                    buffer->metadata->prev_wr_index = kip_metadata_buf->prev_wr_index;

                    /* Provide an update to the other side */
                    kip_metadata_buf->prev_rd_index = buffer->metadata->prev_rd_index;

                    if (head_tag != NULL)
                    {
                        if (buffer->metadata->tags.head == NULL) // Empty list
                        {
                            PL_ASSERT(buffer->metadata->tags.tail == NULL);
                            buffer->metadata->tags.head = head_tag;
                        }
                        else
                        {
                            PL_ASSERT(buffer->metadata->tags.tail != NULL);
                            buffer->metadata->tags.tail->next = head_tag;
                        }

                        buffer->metadata->tags.tail = tail_tag;
                    }
                }
                UNLOCK_INTERRUPTS;

            }
            /* Only kick backwards if there is no new data in the buffer. */
            if (kick_dir == STREAM_KICK_BACKWARDS && data_after <= data_before)
            {
                shadow->remote_kick = TRUE;
            }

            if (data_before < data_after)
            {
                /* Kick forward only if there is new data in the buffer. Otherwise,
                 * there is no need to kick forward.
                 */
                propagate_kick(ep, STREAM_KICK_FORWARDS);
            }
        }
        shadow->kick_in_progress = FALSE;
        /* It's possible for an interrupt to fire here, and service kick_blocked
         * and the loop will run again with nothing to do. This should happen
         * very rarely. */
        /* We need to update kick direction so that a signal is raised if it
         * should have been given where the blocked kick originated from. */
        kick_dir = shadow->kick_blocked;
    } while (kick_dir != STREAM_KICK_INVALID);

    /* Raise the signal for Kick only if the transform is present in the kip transform
     * list and if it is enabled
     */
    if (shadow->remote_kick)
    {
        /* if raising the signal failed, attempt it again in the next kick */
        if (stream_shadow_raise_ipc_signal(ep, channel_id))
        {
            shadow->remote_kick = FALSE;
        }
    }

#ifdef INSTALL_TIMING_TRACE
    timing_trace_instr_stream_record_end_ep_run(ep, is_called);
#endif /* INSTALL_TIMING_TRACE */
}
#endif /* COMMON_SHARED_HEAP */

/*
 * \brief Internal function that starts the shadow endpoint
 *
 * \param *endpoint             Pointer to the endpoint to start
 * \param *ko                   Pointer to kick object
 *
 * \return TRUE if successfull, FALSE if not.
 */
static bool shadow_start(ENDPOINT *ep, KICK_OBJECT *ko)
{
    return TRUE;
}

/*
 * \brief Internal function that stops the shadow endpoint
 *
 * \param *endpoint             Pointer to the endpoint to stop
 *
 * \return TRUE if successfull, FALSE if not.
 */
static bool shadow_stop(ENDPOINT *ep)
{
    if (!ep->is_enabled)
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * \brief Internal function to synchronise two shadow endpoints.
 *
 * \param *ep1                  First source or sink endpoint id
 * \param *ep2                  Second source or sink endpoint id
 *
 * \return TRUE if successfull, FALSE if not.
 */
static bool shadow_sync_sids(ENDPOINT *ep1, ENDPOINT *ep2)
{
    /* If theses are both shadows then we can synchronise them. */
    if (ep1->stream_endpoint_type == endpoint_shadow &&
        ep2->stream_endpoint_type == endpoint_shadow    )
    {
        /* At the moment we are only supporting endpoints that are in the same
         * direction. ie. both Sources or both sinks (Previn allows this in
         * some cases.)
         */
        if (ep1->direction == ep2->direction)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* Find shadow EP from data channel ID and direction. TODO MULTICORE: an optimisation would be if
 * shadow EPs are always kept separately, so less to search, but this then has other overheads.
 */
ENDPOINT* stream_shadow_ep_from_data_channel(unsigned data_chan_id)
{
    /* Get the head of the appropriate list to search. A write channel means it belongs to sink shadow EP and viceversa. */
    ENDPOINT *ep;
    patch_fn_shared(stream_shadow);

    ep = (ipc_get_data_channelid_dir(data_chan_id) == IPC_DATA_CHANNEL_WRITE) ? sink_endpoint_list : source_endpoint_list ;
    /* Search for a shadow endpoint with a matching channel id */
    while ((ep != NULL) && (!STREAM_EP_IS_SHADOW(ep) || (stream_shadow_get_channel_id(ep) != data_chan_id)))
    {
       ep = ep->next;
    }

    return ep;
}
