/****************************************************************************
 * Copyright (c) 2011 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stream_sco_common.c
 * \ingroup stream
 *
 * stream sco type file. <br>
 * This file contains stream functions for sco endpoints. <br>
 *
 */

/****************************************************************************
Include Files
*/

#include "stream_private.h"
#include "stream_endpoint_sco.h"
#include "cbops_mgr/cbops_mgr.h"
#include "opmgr/opmgr_endpoint_override.h"

/****************************************************************************
Private Type Declarations
*/

/****************************************************************************
Private Constant Declarations
*/

/****************************************************************************
Private Variable Definitions
*/

/****************************************************************************
Private Function Declarations
*/
static int get_sco_rate(ENDPOINT *ep);
static bool is_locally_clocked(ENDPOINT *ep);

/****************************************************************************
Private Function Definitions
*/

/****************************************************************************
Public Function Definitions
*/


/****************************************************************************
 *
 * stream_sco_get_hci_handle
 *
 */
unsigned int stream_sco_get_hci_handle(ENDPOINT *endpoint)
{
    /* Internally called so not checking if the pointer is NULL */
    if (!endpoint_is_sco_type(endpoint))
    {
        panic_diatribe(PANIC_AUDIO_STREAM_INVALID_SCO_ENDPOINT,
                       stream_external_id_from_endpoint(endpoint));
    }
    /* the hci handle is the key */
    return endpoint->key;
}


/****************************************************************************
 *
 * stream_sco_params_update
 *
 */
void stream_sco_params_update(unsigned int hci_handle)
{
    ENDPOINT *sco_ep_source, *sco_ep_sink;
    KICK_OBJECT *ko;
    bool source_stopped = FALSE;
    bool sink_stopped = FALSE;
    bool source_connected = TRUE;
    bool sink_connected = TRUE;
    patch_fn_shared(stream_sco);
    /* Both endpoints must be stopped before moving to the new
     * parameters, otherwise the second may see the wallclock change vastly
     * under its feet.
     *
     * This also restarts ratematching.
     */
    sco_ep_source = sco_from_air_endpoint_get(hci_handle);
    /* Stop the source endpoint if it exists and is connected */
    if (NULL == sco_ep_source || NULL == sco_ep_source->connected_to)
    {
        source_connected = FALSE;
    }
    else
    {
        source_stopped = sco_ep_source->functions->stop(sco_ep_source);
    }

    /* Stop the sink endpoint if it exists and is connected. Then update it to
     * the new parameters, which is now safe as the source is also stopped. */
    sco_ep_sink = sco_to_air_endpoint_get(hci_handle);
    if (NULL == sco_ep_sink || NULL == sco_ep_sink->connected_to)
    {
        sink_connected = FALSE;
    }
    else
    {
        sink_stopped = sco_ep_sink->functions->stop(sco_ep_sink);
    }
    if ((!source_connected) && (!sink_connected))
    {
        /* If neither is connected up yet then there is nothing worth doing.
         * Hopefully the compiler optimises this collection of if statements
         * somewhat.
         */
        return;
    }

    /* reschedule the source now, it's safe because the sink is stopped. */
    if (source_connected)
    {

    /* Start the endpoints that we stopped. They are only marked as stopped if
     * they exist and are connected in the first place so don't need to check
     * that too */
        if (source_stopped)
        {
            ko = kick_obj_from_sched_endpoint(sco_ep_source);
            sco_ep_source->functions->start(sco_ep_source, ko);
        }
    }
    if (sink_stopped)
    {
        ko = kick_obj_from_sched_endpoint(sco_ep_sink);
        sco_ep_sink->functions->start(sco_ep_sink, ko);
    }

    return;
}


/****************************************************************************
Private Function Definitions
*/

static void sco_reinit_ratematching(ENDPOINT *endpoint, TIME kick_time)
{
    endpoint_sco_state* ep_sco = &endpoint->state.sco;
    ep_sco->rate_measurement = 1<<STREAM_RATEMATCHING_FIX_POINT_SHIFT;
    ep_sco->rm_start_time = kick_time;
    ep_sco->rm_bt_clock = ep_sco->data_avail_bt_ticks;
}

#ifdef __KCC__
asm int sco_calc_rate(int expected, int measured)
{
    @[    .change rMACB
          .restrict expected:bank1_with_rmac, measured:bank1_with_rmac
     ]
    /* Make use of rMACB as the C compiler doesn't */
    rMACB = @{expected} ASHIFT -2 (56bit);
    Div = rMACB / @{measured};
    @{} = DivResult;
    @{} = @{} ASHIFT -(DAWTH-24);
}
#else /* __KCC__ */
static int sco_calc_rate(int expected, int measured)
{
    return 1<<STREAM_RATEMATCHING_FIX_POINT_SHIFT;
}
#endif /* __KCC__ */

static TIME sco_sched_slot_time(unsigned hci_handle, endpoint_sco_state* ep_sco)
{
    /* Get next slot time including rolling microsecond remainder */
    return time_add(sco_wallclock_get_time_from_ticks(hci_handle, ep_sco->data_avail_bt_ticks),
        ep_sco->data_avail_us_remainder);
}

static void sco_sched_next_kick(endpoint_sco_state* ep_sco)
{
    /* Update kick time (BT ticks and rolling microsecond remainder */
    unsigned interval_ticks = US_TO_BT_TICKS(ep_sco->kick_period_us);

    ep_sco->data_avail_us_remainder += ep_sco->kick_period_us - BT_TICKS_TO_US(interval_ticks);

    if (ep_sco->data_avail_us_remainder > BT_TICKS_TO_US(2))
    {
        ep_sco->data_avail_us_remainder -= BT_TICKS_TO_US(2);
        interval_ticks += 2;
    }
        /* Schedule to kick for when the next data packet is due to be received. */
    ep_sco->data_avail_bt_ticks = (ep_sco->data_avail_bt_ticks + interval_ticks) & BT_TICK_MASK;
}


void sco_sched_kick(ENDPOINT *endpoint, KICK_OBJECT *ko)
{
    unsigned hci_handle = endpoint->key; /* The endpoint key is the hci-handle */
    TIME new_kick_time;
    unsigned delta_time;

    /* Stash the local time of the current slot. Will be used to compute ToA. */
    endpoint->state.sco.current_slot_time = sco_sched_slot_time(hci_handle, &endpoint->state.sco);

    /* Schedule to kick for when the next data packet is due to be received. */
    sco_sched_next_kick(&endpoint->state.sco);

    if (SOURCE == endpoint->direction)
    {
         new_kick_time = time_add(sco_sched_slot_time(hci_handle, &endpoint->state.sco),
                                  sco_from_air_latency_get(hci_handle));
    }
    else
    {
        new_kick_time = time_sub(time_sub(sco_sched_slot_time(hci_handle, &endpoint->state.sco),
                                          sco_to_air_latency_get(hci_handle)),
                                          endpoint->state.sco.proc_time);
    }

    delta_time = (unsigned)time_sub(new_kick_time, endpoint->state.sco.rm_start_time);

    if(delta_time > SECOND)
    {
        /* For ratematching, calculate the expected time 
         * based on the BT clock change over this interval 
         */
        unsigned bt_ticks = endpoint->state.sco.data_avail_bt_ticks;

        TIME_INTERVAL expected = sco_time_between_bt_clocks(endpoint->state.sco.rm_bt_clock, bt_ticks) +
            endpoint->state.sco.data_avail_us_remainder - endpoint->state.sco.rm_us_remainder;

        L3_DBG_MSG2("sco_sched_kick rm delta %d expected %d", delta_time, expected);

        endpoint->state.sco.rm_start_time = new_kick_time;
        endpoint->state.sco.rm_bt_clock = bt_ticks;
        endpoint->state.sco.rm_us_remainder = endpoint->state.sco.data_avail_us_remainder;

        /* Rate is 2^22*((accumulated_time/measured_time)) */
        endpoint->state.sco.rate_measurement = (int)sco_calc_rate(expected, delta_time);
     }

    endpoint->state.sco.kick_id = timer_schedule_event_at(new_kick_time, kick_obj_kick, (void*)ko);

    return;
}

/**
 * \brief Perform timing calculations and setups, and start timers.
 *
 * \return TRUE if the timer was not active,
 *         FALSE if it was already active (and nothing needs doing)
 */
bool sco_start_timers(ENDPOINT *endpoint, KICK_OBJECT *ko)
{
    TIME fire_time, now;
    unsigned n = 0;
    /* The source is the SCO side here as it is from air */
    unsigned hci_handle = stream_sco_get_hci_handle(endpoint);

#if defined(INSTALL_ISO_CHANNELS) && defined(ACTIVATE_SDU_NUMBERING)
    unsigned sdus_per_iso = 0, init_sdu_number = SCO_DRV_EXPECTED_TS_UNKNOWN;
#endif /* defined(INSTALL_ISO_CHANNELS) && defined(ACTIVATE_SDU_NUMBERING) */

    /* This might not be needed however, if we are already running
       then don't do anything */
    if (TIMER_ID_INVALID != endpoint->state.sco.kick_id)
    {
        return FALSE;
    }

    /* Configure kick interrupts' handling to be deferred,
     * before enabling them */
    stream_set_deferred_kick(endpoint, endpoint->deferred.config_deferred_kick);

    endpoint->state.sco.data_avail_bt_ticks = sco_next_slot_get(hci_handle);

    /*
     * The kick is based on the timing information that is/will be provided
     * by the sco_params and the wallclock. This calculation takes account
     * of clock drift and from-air latency.
     */
    if (SOURCE == endpoint->direction)
    {
        /* The maths performed to calculate the first kick is:
         *
         * data_available_us = next_slot_time_us + from_air_latency
         *
         * If this time has passed increment by Tesco (in us) until the kick is
         * in the future.
         */
         fire_time = time_add(sco_wallclock_get_time_from_ticks(hci_handle, endpoint->state.sco.data_avail_bt_ticks),
                     sco_from_air_latency_get(hci_handle));
    }
    else
    {
        /* The maths performed to calculate the first kick is:
         *
         * data_due_us = next_slot_time_us - to_air_latency - audio_processing_time
         *
         * If this time has passed, increment by Tesco (in us) until the kick is
         * in the future.
         */
        fire_time = time_sub(time_sub(sco_wallclock_get_time_from_ticks(hci_handle, endpoint->state.sco.data_avail_bt_ticks),
                    sco_to_air_latency_get(hci_handle)),
                    endpoint->state.sco.proc_time);
    }
    /* Adjust to run just in time for the next SCO packet */

    now = time_get_time();

    patch_fn_shared(stream_sco);

    /* If the time is in the past then bring it into the future, calculate
     * the required delta here */
    if (time_le(fire_time, now) )
    {
        unsigned tesco = sco_tesco_get(hci_handle);
        TIME_INTERVAL project_fwd;
        TIME_INTERVAL delta = time_sub(now, fire_time);

        /* Work out how many tesco periods will put the fire time into the
         * future. Add 1 to the result as it probably wasn't an exact
         * division and even if it was we need a bit of time to schedule
         * the timer. */

        /* delta is a 32-bit time interval, and tesco is at least 1250us
         * so the result is guaranteed to fit in 24 bits
         */
        n = (unsigned)(delta / tesco + 1);

        /* How far into the future does the fire time need to be projected
         * N.B. (delta/tesco) * tesco != delta */
        project_fwd = n * (TIME_INTERVAL)tesco;
        /* Increment both the calculated fire time and the BT clock value */
        endpoint->state.sco.data_avail_bt_ticks += n * US_TO_BT_TICKS(tesco);
        endpoint->state.sco.data_avail_bt_ticks &= BT_TICK_MASK;
        fire_time = time_add(fire_time, project_fwd);
    }

    sco_reinit_ratematching(endpoint, fire_time);

#if defined(INSTALL_ISO_CHANNELS) && defined(ACTIVATE_SDU_NUMBERING)
    if ((endpoint->stream_endpoint_type == endpoint_iso) )
    {
        /* If using SDU numbering, update the from-air expected_ts,
         * or to-air sequence number after obtaining the final fire_time value.
         */

        if (SOURCE == endpoint->direction)
        {
            if (!sco_iso_framed_get(hci_handle))
            { /* On the source EP we can only expect a predictable SDU number for unframed ISO*/
                sdus_per_iso = (sco_iso_interval_get(hci_handle)* US_PER_SLOT * 2) /
                        sco_iso_sdu_interval_from_air_get(hci_handle);
                init_sdu_number = (sco_iso_sdu_number_from_air_get(hci_handle) +
                        n * sdus_per_iso) & 0xffff;
                sco_src_drv_update_expected_ts(sco_get_sco_src_drv(endpoint), init_sdu_number);
            }
        }
        else
        {
            if (sco_iso_framed_get(hci_handle))
            { /* For framed ISO we start counting the SDU number with what we got from BTSS*/
                sco_sink_drv_update_initial_seq_num(sco_get_sco_sink_drv(endpoint),
                        (sco_iso_sdu_number_to_air_get(hci_handle)  & 0xffff));
            }
            else
            {
                sdus_per_iso = (sco_iso_interval_get(hci_handle)* US_PER_SLOT * 2) /
                        sco_iso_sdu_interval_to_air_get(hci_handle);
                init_sdu_number = (sco_iso_sdu_number_to_air_get(hci_handle) +
                        n * sdus_per_iso) & 0xffff;
                sco_sink_drv_update_initial_seq_num(sco_get_sco_sink_drv(endpoint), init_sdu_number);
            }
        }
    }
#endif /* defined(INSTALL_ISO_CHANNELS) && defined(ACTIVATE_SDU_NUMBERING) */

    timer_schedule_event_at_atomic(fire_time,
                                   kick_obj_kick,
                                   (void*)ko,
                                   &endpoint->state.sco.kick_id);

    return TRUE;
}

bool sco_stop(ENDPOINT *endpoint)
{
    unsigned hci_handle;

    /* update the running state to indicate the chain is no longer running */
    hci_handle = endpoint->key;
    if (SOURCE == endpoint->direction)
    {
        sco_from_air_endpoint_run_state_set(hci_handle, CHAIN_NOT_RUNNING);
    }
    else
    {
        sco_to_air_endpoint_run_state_set(hci_handle, CHAIN_NOT_RUNNING);
    }

    /* Cancel deferred kick */
    stream_set_deferred_kick(endpoint, FALSE);

    /* Block interrupts to make sure the timer ID can't change
     * while we're trying to cancel it
     */
    interrupt_block();
    /* This transform owns the kick timer. Cancel the timer if it is running */
    if(TIMER_ID_INVALID != endpoint->state.sco.kick_id)
    {
        timer_cancel_event(endpoint->state.sco.kick_id);
        /* The transform is no longer running */
        endpoint->state.sco.kick_id = TIMER_ID_INVALID;
        interrupt_unblock();
        return TRUE;
    }
    interrupt_unblock();

#if defined(PROFILER_ON) && defined(INSTALL_SCOISO_EP_PROFILING)
    if (endpoint->state.sco.profiler != NULL)
    {
        profiler *old_profiler = endpoint->state.sco.profiler;
        endpoint->state.sco.profiler = NULL;
        PROFILER_DEREGISTER(old_profiler);
        pdelete(old_profiler);
    }
#endif

    /* If we tried to start the endpoint, but didn't have enough information
     * to start the kick timers, start_pending will be TRUE. Report successful
     * stop so the endpoint will be (re)started.
     */

    if (endpoint->state.sco.start_pending)
    {
        endpoint->state.sco.start_pending = FALSE;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

bool sco_configure(ENDPOINT *endpoint, unsigned int key, uint32 value)
{
    /* SCO specific endpoint configuration code to go here.
     */
    if ((key & ENDPOINT_INT_CONFIGURE_KEYS_MASK) != 0)
    {
        switch(key)
        {
        case EP_CBOPS_PARAMETERS:
            /* Cbops parameter passing is not supported be SCO.
             * Free the cbops_parameters to avoid any potential memory leak and fail. */
            free_cbops_parameters((CBOPS_PARAMETERS *)(uintptr_t) value);
            return FALSE;
        case EP_DATA_FORMAT:
            return sco_set_data_format(endpoint, (AUDIO_DATA_FORMAT)value);
        case EP_PROC_TIME:
            endpoint->state.sco.proc_time = (unsigned int)value;
            return TRUE;

        default:
            return FALSE;
        }
    }
    else
    {
        switch(key)
        {
        case STREAM_CONFIG_KEY_STREAM_RM_USE_RATE_ADJUST_OPERATOR:
        {
           EXT_OP_ID opid = (EXT_OP_ID) value;
           CAP_ID capid;

           /* Allow configuration only when endpoint is disconnected */
           if(NULL != endpoint->connected_to)
           {
               return FALSE;
           }
           if(0 != opid)
           {
               /* Check that the operator exists. */
               if (!opmgr_get_capid_from_opid(opid, &capid))
               {
                   return FALSE;
               }
               stream_delegate_rate_adjust_set_passthrough_mode(opid, FALSE);
           }
           else
           {
               /* opid == 0 intends to disassociate the endpoint from standalone
                * rate adjust operator, set the operator to pass-through mode
                * if it is still being used
                */
               unsigned cur_opid = endpoint->state.sco.external_rate_adjust_opid;
               if(cur_opid != 0 )
               {
                   stream_delegate_rate_adjust_set_passthrough_mode(cur_opid, TRUE);
               }
           }
           /* set the rate adjust op that is available to this endpoint
            */
           endpoint->state.sco.external_rate_adjust_opid = opid;

           return TRUE;
       }
       default:
           return FALSE;
       }
    }
}

/**
 *
 */
static int get_sco_rate(ENDPOINT *ep)
{
    patch_fn_shared(stream_sco);

    return ep->state.sco.rate_measurement;
}

#if CHIP_HAS_CSR_BT
static bool is_locally_clocked(ENDPOINT *ep)
{
    patch_fn_shared(stream_sco);
    /* All Kymera firmware is post B-111491 so we can tell if we are master
     * or slave in the link by looking at the to-air latency parameter. If the
     * to-air latency is negative then this device is slave. */
    /* FIXME: This approach is VERY fragile BT can change the behaviour of
     * sco_to_air_latengy unilaterally and break this approach at any time. */
    if (sco_to_air_latency_get(ep->key) < 0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
#else
static bool is_locally_clocked(ENDPOINT *ep)
{
    patch_fn_shared(stream_sco);
    /* When running with a Zeagle IP BT there is not way to know if we are
     * master or Slave so we've decided to say always that we are slave
     * and bite the bullet of additional (minimal) MIPS consumption. */
    return FALSE;
}
#endif /* CHIP_HAS_CSR_BT */

bool sco_get_config(ENDPOINT *endpoint, unsigned int key, ENDPOINT_GET_CONFIG_RESULT* result)
{
    /* SCO specific endpoint configuration code to go here.
     */
    endpoint_sco_state* ep_sco = &endpoint->state.sco;
    switch(key)
    {
    case EP_DATA_FORMAT:
        result->u.value = sco_get_data_format(endpoint);
        return TRUE;
    case EP_PROC_TIME:
        result->u.value = ep_sco->proc_time;
        return TRUE;
    case EP_RATEMATCH_ABILITY:
        /* Sco endpoints can't ratematch as the data might be encoded. */
        result->u.value = (uint32)RATEMATCHING_SUPPORT_NONE;
        return TRUE;
    case EP_RATEMATCH_RATE:
        result->u.value = (uint32)(int32)get_sco_rate(endpoint);
        L3_DBG_MSG2("SCO rate : %d EP: %06X", result->u.value, (uintptr_t)endpoint);
        return TRUE;

    case EP_RATEMATCH_MEASUREMENT:
        result->u.rm_meas.sp_deviation =
                STREAM_RATEMATCHING_RATE_TO_FRAC(get_sco_rate(endpoint));
        result->u.rm_meas.measurement.valid = FALSE;
        return TRUE;

#ifdef STREAM_INFO_KEY_AUDIO_SAMPLE_PERIOD_DEVIATION
    case STREAM_INFO_KEY_AUDIO_SAMPLE_PERIOD_DEVIATION:
        result->u.value =
                STREAM_RATEMATCHING_RATE_TO_FRAC(get_sco_rate(endpoint));
        return TRUE;
#endif /* STREAM_INFO_KEY_AUDIO_SAMPLE_PERIOD_DEVIATION */

#ifdef STREAM_INFO_KEY_AUDIO_LOCALLY_CLOCKED
    case STREAM_INFO_KEY_AUDIO_LOCALLY_CLOCKED:
        result->u.value = is_locally_clocked(endpoint);
        return TRUE;
#endif /* STREAM_INFO_KEY_AUDIO_LOCALLY_CLOCKED */
     default:
        return FALSE;
    }
}

void sco_common_get_timing (ENDPOINT *endpoint, ENDPOINT_TIMING_INFORMATION *time_info)
{
    patch_fn_shared(stream_sco);

    time_info->block_size = 0;
    time_info->is_volatile = TRUE;
    time_info->locally_clocked = is_locally_clocked(endpoint);

    return;
}

unsigned stream_sco_get_wallclock_id(ENDPOINT *ep)
{
    unsigned hci_handle = ep->key;

    return sco_get_wallclock_id(hci_handle);
}

/**
 * \brief Check whether two endpoints share a clock source
 *
 * \param ep1 Endpoint to compare with ep2.
 * \param ep2 Endpoint to compare with ep1.
 * \param both_local boolean indicating if both endpoints are locally clocked.
 *
 * \return TRUE if ep1 and ep2 share a clock source, otherwise FALSE.
 */
bool sco_have_same_clock(ENDPOINT *ep1, ENDPOINT *ep2, bool both_local)
{
    /* If these endpoints use the same wallclock then they have the
     * same clock source. Or we are the master of both links in which
     * case they are both locally clocked and can save the function call
     * effort. */
    if (both_local || stream_sco_get_wallclock_id(ep1) == stream_sco_get_wallclock_id(ep2))
    {
        return TRUE;
    }
    return FALSE;
}

/**
 * \brief Get the sco source driver instance associated to the endpoint.
 *
 * \param ep  Pointer to the ENDPOINT structure.
 *
 * \return SCO_SRC_DRV_DATA pointer to the instance.
 */
SCO_SRC_DRV_DATA* sco_get_sco_src_drv(ENDPOINT *ep)
{
    if ((ep == NULL) ||
        ((ep->stream_endpoint_type != endpoint_sco) &&
         (ep->stream_endpoint_type != endpoint_iso)) ||
        (ep->direction != SOURCE))
    {
        return NULL;
    }

    return ep->state.sco.sco_drv.sco_src_drv;
}

/**
 * \brief Get the sco sink driver instance associated to the endpoint.
 *
 * \param ep  Pointer to the ENDPOINT structure.
 *
 * \return SCO_SINK_DRV_DATA pointer to the instance.
 */
SCO_SINK_DRV_DATA* sco_get_sco_sink_drv(ENDPOINT *ep)
{
    if ((ep == NULL) ||
        ((ep->stream_endpoint_type != endpoint_sco) &&
        (ep->stream_endpoint_type != endpoint_iso)) ||
        (ep->direction != SINK))
    {
        return NULL;
    }

    return ep->state.sco.sco_drv.sco_sink_drv;
}

/**
 * \brief Destroy the sco (source or sink) driver instance
 *        associated to the endpoint.
 *
 * \param ep  Pointer to the ENDPOINT structure.
 */
void sco_clean_up_sco_drv(ENDPOINT *ep)
{
    if (sco_get_sco_src_drv(ep))
    {
        /* Destroy SCO src driver instance */
        pdelete(ep->state.sco.sco_drv.sco_src_drv);
        ep->state.sco.sco_drv.sco_src_drv = NULL;
    }

    if (sco_get_sco_sink_drv(ep))
    {
        /* Destroy SCO sink driver instance */
        sco_sink_drv_data_destroy(ep->state.sco.sco_drv.sco_sink_drv);
    }
}

/**
 * \brief Free the memory associated to the endpoint.
 *
 * \param ep  Pointer to the ENDPOINT structure.
 */
void sco_iso_clean_up_endpoint(ENDPOINT *ep)
{
    tCbuffer *temp_buffer_ptr;
    union SCO_DRV sco_drv;
    ENDPOINT_DIRECTION dir;

    /* Remember the buffer that we need to free */
    temp_buffer_ptr = ep->state.sco.cbuffer;
    sco_drv = ep->state.sco.sco_drv;
    dir = ep->direction;

    ep->can_be_closed = TRUE;
    ep->can_be_destroyed = TRUE;
    stream_close_endpoint(ep);

    /* Free up the buffer and associated data space */
    cbuffer_destroy(temp_buffer_ptr);

    /* Destroy SCO driver instance */
    if (dir == SINK)
    {
        sco_sink_drv_data_destroy(sco_drv.sco_sink_drv);
    }
    else
    {
        pdelete(sco_drv.sco_src_drv);
    }
}

/**
 * \brief Obtain the usable octets to be further set for the transform buffer
 *        for SCO/ISO endpoints according to the endpoint data format. SCO
 *        encoders and decoders output data in 16-bit format, as opposed to
 *        LC3 which outputs data in 32-bit format.
 *
 * \param  format    Format of the endpoint.
 *
 * \return The number of usable octets to be set for the transform buffer.
 */
unsigned sco_iso_get_transf_buf_usable_octets(AUDIO_DATA_FORMAT format)
{
    switch (format)
    {
#ifdef CVSD_CODEC_SOFTWARE
        case AUDIO_DATA_FORMAT_16_BIT:
#else
        case AUDIO_DATA_FORMAT_FIXP:
#endif /* CVSD_CODEC_SOFTWARE */
        case AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP:
            return 2;

        case AUDIO_DATA_FORMAT_ENCODED_DATA:
            return 4;

        default:
            L2_DBG_MSG1("Unknown EP format %d, transf buffer usable octets: 0",
                        format);
            return 0;
    }
}
