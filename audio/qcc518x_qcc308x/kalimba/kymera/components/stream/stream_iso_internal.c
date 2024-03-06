/****************************************************************************
 * Copyright (c) 2018 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stream_iso_internal.c
 * \ingroup stream
 *
 * stream iso type file. <br>
 * This file contains stream functions for iso endpoints. <br>
 *
 */

/****************************************************************************
Include Files
*/

#include "stream_private.h"
#include "stream_endpoint_iso.h"
#include "sco_drv/sco_src_drv.h"
#include "sco_drv/sco_sink_drv.h"

/****************************************************************************
Private Type Declarations
*/

/****************************************************************************
Private Constant Declarations
*/

/****************************************************************************
Private Macro Declarations
*/
#ifdef INSTALL_ISO_CHANNELS

/****************************************************************************
Private Function Declarations
*/

/****************************************************************************
Private Variable Definitions
*/

DEFINE_ENDPOINT_FUNCTIONS(iso_functions, iso_close, iso_connect,
                          iso_disconnect, iso_buffer_details,
                          iso_kick, iso_sched_kick, iso_start,
                          iso_stop, iso_configure, iso_get_config,
                          iso_get_timing, iso_sync_endpoints,
                          iso_have_same_clock);

/****************************************************************************
Protected Functions
*/

/**
 * Close the endpoint
 */
bool iso_close(ENDPOINT *endpoint)
{
    patch_fn_shared(stream_iso);
    return sco_close(endpoint);
}

/**
 * \brief Connects the ISO source/sink endpoint.
 *
 * \param  endpoint          Pointer to the endpoint that is being connected.
 * \param  Cbuffer_ptr       Pointer to the buffer created by the transform.
 * \param  ep_to_kick        Pointer to the endpoint which will be kicked after
 *                           a successful run.
 *                           Note: this can be different from the connected to
 *                           endpoint when in-place running is enabled.
 * \param  start_on_connect  Return flag which indicates if the endpoint wants
 *                           to be started on connect.
 *                           Note: The endpoint will only be started if the
 *                           connected to endpoint wants to be started too.
 *
 * \return TRUE on success.
 */
bool iso_connect(ENDPOINT *endpoint,
                 tCbuffer *Cbuffer_ptr,
                 ENDPOINT *ep_to_kick,
                 bool *start_on_connect)
{
    ENDPOINT_GET_CONFIG_RESULT result;
    bool ret;

    patch_fn_shared(stream_iso);

    ret = iso_get_config(endpoint, EP_DATA_FORMAT, &result);
    if (!ret)
    {
         return ret;
    }

    return sco_iso_connect(endpoint,
                           Cbuffer_ptr,
                           ep_to_kick,
                           start_on_connect,
                           (AUDIO_DATA_FORMAT)result.u.value);
}

/**
 * Disconnect from the endpoint
 */
bool iso_disconnect(ENDPOINT *endpoint)
{
    patch_fn_shared(stream_iso);
    return sco_disconnect(endpoint);
}

/**
 * Retrieve the buffer details from the endpoint
 */
bool iso_buffer_details(ENDPOINT *endpoint, BUFFER_DETAILS *details)
{
    patch_fn_shared(stream_iso);
    return sco_buffer_details(endpoint, details);
}

#if defined(PROFILER_ON) && defined(INSTALL_SCOISO_EP_PROFILING)
/* Debug log string to allow profiler entries to be recognised by ACAT */
LOG_STRING(endpoint_name, "ISO EP");
#endif

/**
 * Make the endpoint run any data processing and propagate kick
 */
void iso_kick(ENDPOINT *endpoint, ENDPOINT_KICK_DIRECTION kick_dir)
{
    patch_fn_shared(stream_iso);

    endpoint_iso_state *iso_state = &endpoint->state.iso;
    unsigned hci_handle = endpoint->key;

#if defined(PROFILER_ON) && defined(INSTALL_SCOISO_EP_PROFILING)
    if (iso_state->profiler != NULL)
    {
        profiler_start(iso_state->profiler);
        iso_state->profiler->kick_inc++;
    }
#endif

    /* This endpoint is a hard deadline so the operator is tied to the
     * endpoint scheduling and expects an INTERNAL kick to the connected to endpoint. */
    if (SOURCE == endpoint->direction)
    {
        SCO_SRC_DRV_DATA *sco_drv = sco_get_sco_src_drv(endpoint);
        sco_src_timing_info timing_info;

        int sp = STREAM_RATEMATCHING_RATE_TO_FRAC(iso_state->rate_measurement);

        /* calculate time of arrival in microseconds */
        unsigned toa = time_add(iso_state->current_slot_time,
                sco_iso_transport_offset_from_air_get(hci_handle));

        /* We should have made sure that during creation and destruction
         * there is no race condition for which we end up serving the kick with
         * invalid data.
         */
        PL_ASSERT(sco_drv);

        timing_info.toa_ep = toa;
        timing_info.sp_adjust = sp;

        if (sco_iso_framed_get(hci_handle))
        {
            sco_src_drv_processing_iso_framed(sco_drv, 
                sco_iso_sdu_interval_from_air_get(hci_handle),
                sco_tesco_get(hci_handle),
                iso_state->current_slot_time);
        }
        else
        {
            /* Run the sco drv instance */
            sco_src_drv_processing(sco_drv, &timing_info);
        }
    }
    else
    {
        SCO_SINK_DRV_DATA *sco_drv = sco_get_sco_sink_drv(endpoint);
        unsigned ttp_max_deviaton = sco_iso_sdu_interval_to_air_get(hci_handle);

        TIME ttp_reference_time;
        
        if (sco_iso_framed_get(hci_handle))
        {
            /* Make reference time the same as the actual kick time ? */
            ttp_reference_time = time_sub(time_sub(iso_state->current_slot_time,
                                    sco_to_air_latency_get(hci_handle)),
                                    endpoint->state.sco.proc_time);
        }
        else
        {
            ttp_reference_time = time_sub(iso_state->current_slot_time,
                sco_iso_transport_offset_to_air_get(hci_handle));
        }

        /* We should have made sure that during creation and destruction
         * there is no race condition for which we end up serving the kick with
         * invalid data.
         */
        PL_ASSERT(sco_drv);

        /* Run the sco drv instance */
        sco_sink_drv_processing(sco_drv, ttp_reference_time, ttp_max_deviaton);

        /* If TTP is enabled we have to run the corresponding PID controller
         * here and send the resulting value to the rate adjust op
         * */
        if (sco_sink_drv_ttp_enabled(sco_drv))
        {
            int warp;
            warp = sco_sink_drv_ttp_ctrl_get_warp(sco_drv);
            stream_delegate_rate_adjust_set_current_rate(
                    iso_state->external_rate_adjust_opid, warp);
        }
    }

#if defined(PROFILER_ON) && defined(INSTALL_SCOISO_EP_PROFILING)
    if (iso_state->profiler != NULL)
    {
        profiler_stop(iso_state->profiler);
    }
#endif

    if (endpoint->direction == SOURCE)
    {
        propagate_kick(endpoint, STREAM_KICK_FORWARDS);
    }
    else
    {
        propagate_kick(endpoint, STREAM_KICK_BACKWARDS);
    }
}

/**
 * Perform any real-time scheduling that needs to occur per kick
 */
void iso_sched_kick(ENDPOINT *endpoint, KICK_OBJECT *ko)
{
    patch_fn_shared(stream_iso);
    sco_sched_kick(endpoint, ko);
}


static void iso_start_from_air(ENDPOINT *endpoint)
{
    /* The source is the SCO side here as it is from air */
    unsigned hci_handle = stream_sco_get_hci_handle(endpoint);

    sco_from_air_endpoint_run_state_set(hci_handle, CHAIN_RUNNING);

    /* We need to have received valid parameters by now, 
     * to correctly set the timing information
     */
    PL_ASSERT(sco_params_received_get(hci_handle));

    /* BT doesn't really have any use for frame length so we'll send 0.
        * If we come up with a use for it in the future then we'll change
        * the code to send something constructive. */
    sco_send_frame_length_and_run_state(hci_handle, SCO_DIR_FROM_AIR, 0);
}

/**
 * Initiate a kick interrupt source to start producing kicks
 */
bool iso_start(ENDPOINT *endpoint, KICK_OBJECT *ko)
{
    patch_fn_shared(stream_iso);

    unsigned hci_handle = stream_sco_get_hci_handle(endpoint);
 
    /* Start the timers needed to run the endpoint at the correct timing.
     * Only do this if we have the relevant parameters to know that timing.
     */
    if (!sco_params_received_get(hci_handle))
    {
        L2_DBG_MSG("ISO endpoint starting without valid params, start will be deferred");
        endpoint->state.iso.start_pending = TRUE;
    }
    else
    {
        bool is_framed = sco_iso_framed_get(hci_handle);

        /* Set the endpoint kick period. */
        if (SOURCE == endpoint->direction)
        {
            if (is_framed)
            {
                /* From-air framed: kick at ISO interval */
                endpoint->state.iso.kick_period_us = sco_tesco_get(hci_handle);
            }
            else
            {
                /* From-air unframed: kick at SDU interval */
                endpoint->state.iso.kick_period_us = sco_iso_sdu_interval_from_air_get(hci_handle);
            }
        }
        else
        {
            /* To-air: kick at SDU interval for both framed and unframed */
            endpoint->state.iso.kick_period_us = sco_iso_sdu_interval_to_air_get(hci_handle);
        }

        endpoint->state.sco.proc_time = SCOISO_SINK_PROCESSING_TIME();

        L2_DBG_MSG1("iso_start kp = %d", endpoint->state.iso.kick_period_us);

        if (endpoint->direction == SOURCE)
        {
            unsigned ts_step, from_air_length, exp_ts;

            /* TODO exp_ts is defined an unsigned and set to UNKNONW (-1). */
            /* Initialize the expected timestamp (SDU number). */
            exp_ts = SCO_DRV_EXPECTED_TS_UNKNOWN;
            /* The amount to incremement the exp_ts with, at every kick. */
            ts_step = 1;
            /* from_air_length is stored in octets */
            from_air_length = sco_from_air_length_get(hci_handle);

            sco_src_drv_set_from_air_info(sco_get_sco_src_drv(endpoint),
                                          from_air_length,
                                          exp_ts,
                                          ts_step,
                                          is_framed,
                                          TRUE); 
        }
        else
        {
            /* enable ttp logic only if rate_adjust OPID has been configured */
            bool ttp_enable = (0 == endpoint->state.iso.external_rate_adjust_opid)?FALSE:TRUE;

            sco_sink_drv_set_to_air_info(sco_get_sco_sink_drv(endpoint),
                                     sco_to_air_length_get(hci_handle),
                                     is_framed,
                                     ttp_enable);
        }

        if (sco_start_timers(endpoint, ko))
        {
            /* update the chains running state in case the other side needs
             * timing info to schedule in relation to this chain */
            if (SOURCE == endpoint->direction)
            {
                /* Setup the from-air endpoint */
                iso_start_from_air(endpoint);

                ENDPOINT *synced_ep = endpoint->state.iso.synced_ep;
                if ((synced_ep != NULL) && (!synced_ep->state.iso.start_pending))
                {
                    /* Start synchronized ISO source endpoints in the same
                     * SDU Interval. This is only applicable to unframed ISOAL.
                     * At this point, both endpoints should have just scheduled
                     * their first kick.
                     */
                    iso_start_from_air(synced_ep);
                    flush_endpoint_buffers(synced_ep);
                }
            }
            else
            {
                /* Setup the to-air endpoint */
                sco_start_to_air(endpoint);
            }

            /* empty the buffer, show is about to start */
            flush_endpoint_buffers(endpoint);
        }
#if defined(PROFILER_ON) && defined(INSTALL_SCOISO_EP_PROFILING)
        endpoint->state.iso.profiler = create_dynamic_profiler(endpoint_name, stream_external_id_from_endpoint(endpoint));
#endif
    }

    return TRUE;
}

/**
 * Cancel the associated kick interrupt source
 */
bool iso_stop(ENDPOINT *endpoint)
{
    patch_fn_shared(stream_iso);
    return sco_stop(endpoint);
}

/**
 * Configure the endpoint
 */
bool iso_configure(ENDPOINT *endpoint, unsigned int key, uint32 value)
{
    patch_fn_shared(stream_iso);

    if ((key == EP_DATA_FORMAT) && (value != AUDIO_DATA_FORMAT_ENCODED_DATA))
    {
        return FALSE;
    }

    return sco_configure(endpoint, key, value);
}

/**
 * Get endpoint configuration
 */
bool iso_get_config(ENDPOINT *endpoint, unsigned int key, ENDPOINT_GET_CONFIG_RESULT* result)
{
    patch_fn_shared(stream_iso);

    switch (key)
    {
        case EP_DATA_FORMAT:
        {
            /* Override the ISO ep data format. When connecting the ISO ep and
             * resolving the data format of the two ep being connected, the
             * data format returned is relevant to the transform buffer, and
             * not the MMU buffer. Overwrite the ISO ep data format to set the
             * usable octets for the ISO transform buffer correctly.
             */
            result->u.value = AUDIO_DATA_FORMAT_ENCODED_DATA;
            return TRUE;
        }
        default:
        {
            return sco_get_config(endpoint, key, result);
        }
    }
}

/**
 * Obtain the timing information from the endpoint
 */
void iso_get_timing (ENDPOINT *endpoint, ENDPOINT_TIMING_INFORMATION *time_info)
{
    patch_fn_shared(stream_iso);
    sco_get_timing(endpoint, time_info);
}

/**
 * Report whether two endpoints have the same clock source
 */
bool iso_have_same_clock(ENDPOINT *ep1, ENDPOINT *ep2, bool both_local)
{
    patch_fn_shared(stream_iso);
    return sco_have_same_clock(ep1, ep2, both_local);
}

/**
 * Create a new ISO endpoint
 */
ENDPOINT * iso_create_endpoint(unsigned key, ENDPOINT_DIRECTION dir)
{
    patch_fn_shared(stream_iso);
    return STREAM_NEW_ENDPOINT(iso, key, dir, INVALID_CON_ID);
}

/**
 * Get pointer to an existing ISO endpoint
 */
ENDPOINT * iso_get_endpoint(unsigned key, ENDPOINT_DIRECTION dir)
{
    patch_fn_shared(stream_iso);
    return stream_get_endpoint_from_key_and_functions(key, dir,
            &endpoint_iso_functions);
}


bool iso_sync_endpoints(ENDPOINT *ep1, ENDPOINT *ep2)
{
    patch_fn_shared(stream_iso);

    unsigned hci_handle_ep1, hci_handle_ep2;

    if (ep1 == NULL)
    {
        /* If one of the endpoints is NULL we've made sure it is not ep1. This
         * can only happen if both ep1 and ep2 are NULL.
         */
        return FALSE;
    }

    if ((ep1->state.iso.kick_id != TIMER_ID_INVALID) ||
        (ep1->direction != SOURCE))
    {
        /* Cannot remove synchronize for an endpoint while it's running.
         * kick_id will be set when scheduling the first kick.
         */
        return FALSE;
    }

    if (ep2 == NULL)
    {
        /* Since we can only synchronize 2 endpoints, if we remove
         * synchronization for one of them, remove it for both, if neither of
         * them is running.
         */
        ep2 = ep1->state.iso.synced_ep;
        if ((ep2 != NULL) &&
            (ep2->state.iso.synced_ep == ep1))
        {
            if (ep2->state.iso.kick_id == TIMER_ID_INVALID)
            {
                ep2->state.iso.synced_ep = NULL;
            }
            else
            {
                return FALSE;
            }
        }

        ep1->state.iso.synced_ep = NULL;

        return TRUE;
    }

    if ((ep1 == ep2) ||
        ((ep1->state.iso.synced_ep == ep2) &&
         (ep2->state.iso.synced_ep == ep1)))
    {
        /* If the endpoints are the same or they are already sync-ed,
         * nothing more to do.
         */
        return TRUE;
    }

    if ((ep2->state.iso.kick_id != TIMER_ID_INVALID) ||
        (ep2->direction != SOURCE))
    {
        /* Cannot remove synchronize for an endpoint while it's running.
         * kick_id will be set when scheduling the first kick.
         */
        return FALSE;
    }

    /* At this point try to synchornize ep1 and ep2. */
    hci_handle_ep1 = stream_sco_get_hci_handle(ep1);
    hci_handle_ep2 = stream_sco_get_hci_handle(ep2);

    if (!sco_params_received_get(hci_handle_ep1) ||
        !sco_params_received_get(hci_handle_ep2))
    {
        /* Cannot synchronize endpoints for which we have not received the
         * ISO_PARAMS message, since there isn't any timing info available.
         */
        return FALSE;
    }

    if (sco_iso_framed_get(hci_handle_ep1) ||
        sco_iso_framed_get(hci_handle_ep2))
    {
        /* Cannot synchronize endpoints for framed ISOAL */
        return FALSE;
    }

    if ((ep1->state.iso.kick_period_us != ep2->state.iso.kick_period_us) ||
        (sco_iso_interval_get(hci_handle_ep1) !=
         sco_iso_interval_get(hci_handle_ep2)))
    {
        /* Cannot synchronize endpoints with different SDU Intervals or
         * ISO Intervals. The latter is relevant when calculating BN (burst
         * number) and setting the expected sequence number for the SCO src
         * drv.
         */
        return FALSE;
    }

    /* Cannot synchornize two endpoints if at least one of them was already
     * synchronized to a third one. Prior synchornizations should first be
     * removed.
     */
    if ((ep1->state.iso.synced_ep != NULL) ||
        (ep2->state.iso.synced_ep != NULL))
    {
        return FALSE;
    }

    ep1->state.iso.synced_ep = ep2;
    ep2->state.iso.synced_ep = ep1;

    return TRUE;
}

#endif /* INSTALL_ISO_CHANNELS */

