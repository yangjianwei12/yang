/****************************************************************************
 * Copyright (c) 2011 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stream_sco_hydra.c
 * \ingroup stream
 *
 * stream sco type file. <br>
 * This file contains stream functions for sco endpoints. <br>
 *
 * \section sec1 Contains:
 * stream_sco_get_endpoint <br>
 * stream_create_sco_endpoints_and_cbuffers <br>
 * stream_delete_sco_endpoints_and_cbuffers <br>
 */

/****************************************************************************
Include Files
*/

#include "stream_private.h"
#include "stream_endpoint_sco.h"
#include "stream/stream_for_sco_processing_service.h"
#include "pl_assert.h"
#include "sco_drv/sco_src_drv.h"
#include "sco_drv/sco_sink_drv.h"


/****************************************************************************
Private Type Declarations
*/

/****************************************************************************
Private Constant Declarations
*/
/** The location of the hci handle in the sco_get_endpoint params */
#define HCI_HANDLE  0

/****************************************************************************
Private Macro Declarations
*/
/* Macros for bt clock manipulation and comparison */
/* This file deals with 32 bit bt clocks; outside they're 28 bits. */
#define BTCLKMASK       (0x0FFFFFFFUL)
#define uint32_to_uint28(c)     ((c) & BTCLKMASK)
#define btclock_rawadd(x, y) ((x) + (y))
#define btclock_add(x, y) uint32_to_uint28(btclock_rawadd((x), (y)))


/****************************************************************************
Private Function Declarations
*/

/****************************************************************************
Private Variable Definitions
*/

DEFINE_ENDPOINT_FUNCTIONS(sco_functions, sco_close, sco_connect,
                          sco_disconnect, sco_buffer_details,
                          sco_kick, sco_sched_kick, sco_start,
                          sco_stop, sco_configure, sco_get_config,
                          sco_get_timing, stream_sync_sids_dummy,
                          sco_have_same_clock);

/****************************************************************************
Public Function Definitions
*/

/****************************************************************************
 *
 * stream_sco_get_endpoint
 *
 */
ENDPOINT *stream_sco_get_endpoint(CONNECTION_LINK con_id,
                                  ENDPOINT_DIRECTION dir,
                                  unsigned num_params,
                                  unsigned *params)
{
    ENDPOINT *ep;
    unsigned key;

    /* Expect an hci handle and potentially some padding */
    if (num_params < 1)
    {
        L3_DBG_MSG("hydra stream_sco_get_endpoint (num_params < 1) return NULL");
        return NULL;
    }
    /* The hci handle forms the key (unique for the type and direction) */
    key = params[HCI_HANDLE];

    L3_DBG_MSG1("hydra stream_sco_get_endpoint hci handle: %d", key);

    /* Return the requested endpoint (NULL if not found) */
    ep = stream_get_endpoint_from_key_and_functions(key, dir,
                                                  &endpoint_sco_functions);
    if (ep)
    {
        /* The endpoint has been created, however we now need to check
           the ID */
        if (ep->con_id == INVALID_CON_ID)
        {
            ep->con_id = con_id;
        }
        /* If the client does not own the endpoint they can't access it. */
        else if (ep->con_id != con_id)
        {
            L3_DBG_MSG("hydra stream_sco_get_endpoint (ep->con_id != con_id) return NULL");
            return NULL;
        }
    }
    L3_DBG_MSG1("hydra stream_sco_get_endpoint normal execution ep: %d", ep);
    return ep;
}

/****************************************************************************
 *
 * stream_create_sco_endpoints_and_cbuffers
 *
 */
bool stream_create_sco_endpoints_and_cbuffers(unsigned int hci_handle,
                                              unsigned int source_buf_size,
                                              unsigned int sink_buf_size,
                                              tCbuffer **source_cbuffer,
                                              tCbuffer **sink_cbuffer)
{
    ENDPOINT *source_ep, *sink_ep = NULL;
    SCO_SRC_DRV_DATA *sco_src_drv;
    SCO_SINK_DRV_DATA *sco_sink_drv;

    patch_fn_shared(stream_sco);

    /* Clear contents of source and sink cbuffer pointer parameters as a
     * precaution to minimise the risk of a caller treating them as valid
     * pointers in the event of the function failing and returning FALSE.
     */
    *source_cbuffer = NULL;
    *sink_cbuffer = NULL;

    /* The hci handle is the key (unique for the type and direction) */
    unsigned key = hci_handle;


    L3_DBG_MSG1("!!!!!!!!hydra stream_create_sco_endpoints_and_cbuffers hci handle: %d", key);

    /* Check that we don't already have a source endpoint for the
     * specified hci handle.
     */
    if (stream_get_endpoint_from_key_and_functions(key, SOURCE,
                                            &endpoint_sco_functions) != NULL)
    {
        /* Caller should not have called us for a second time without
         * deleting the existing buffers first.
         */
        panic(PANIC_AUDIO_SCO_BUFFERS_ALREADY_EXIST);
    }


    /* Create and initialise a source endpoint
     * ---------------------------------------
     */
    if ((source_ep = STREAM_NEW_ENDPOINT(sco, key, SOURCE, INVALID_CON_ID)) == NULL)
    {
        goto handle_error;
    }
    source_ep->can_be_closed = FALSE;
    source_ep->can_be_destroyed = FALSE;
    /* SCO endpoints are always at the end of a chain */
    source_ep->is_real = TRUE;
    source_ep->deferred.config_deferred_kick = TRUE;

    /* We're hosting the buffer so create the source buffer and handles.
     * This includes a third auxiliary handle which is a write handle.
     */
    source_ep->state.sco.cbuffer = cbuffer_create_mmu_buffer(
                                      MMU_UNPACKED_16BIT_MASK | BUF_DESC_MMU_BUFFER_AUX_WR,
                                      source_buf_size);

    if (source_ep->state.sco.cbuffer == NULL)
    {
        goto handle_error;
    }

    /* Set 16-bit shift for unpacked buffers */
    cbuffer_set_write_shift(source_ep->state.sco.cbuffer, 16);
    /* Default the from-air MMU buffer configuration for
     * AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP, to be consistent with the to-air
     * MMU buffer.
     */
    cbuffer_set_write_byte_swap(source_ep->state.sco.cbuffer, TRUE);

    /* Create the sco_src_drv as part of the endpoint. */
    sco_src_drv = sco_src_drv_data_create();
    if (sco_src_drv == NULL)
    {
        goto handle_error;
    }

   /* Connecting the sco_src_drv output buffer to the MMU buffer will happen
    * when the transform creates that output buffer.
    */

    /* Install the sco drv. */
    source_ep->state.sco.sco_drv.sco_src_drv = sco_src_drv;

    /* Create and initialise a sink endpoint
     * -------------------------------------
     */
    if ((sink_ep = STREAM_NEW_ENDPOINT(sco, key, SINK, INVALID_CON_ID)) == NULL)
    {
        goto handle_error;
    }
    sink_ep->can_be_closed = FALSE;
    sink_ep->can_be_destroyed = FALSE;
    /* SCO endpoints are always at the end of a chain */
    sink_ep->is_real = TRUE;
    sink_ep->deferred.config_deferred_kick = TRUE;

    /* We're hosting the buffer so create the sink buffer and handles.
     * This includes a third auxiliary handle which is a read handle.
     */
    sink_ep->state.sco.cbuffer = cbuffer_create_mmu_buffer(
                                          MMU_UNPACKED_16BIT_MASK | BUF_DESC_MMU_BUFFER_AUX_RD,
                                          sink_buf_size);

    if (sink_ep->state.sco.cbuffer == NULL)
    {
        goto handle_error;
    }

    /* Set 16-bit shift for unpacked buffers */
    cbuffer_set_read_shift(sink_ep->state.sco.cbuffer, 16);

    /* Configure the MMU buffer so that sco_get_data_format() can return
     * AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP. This is useful when the SCO
     * encoders are running on P1 and we need to resolve the data formats of
     * the shadow sink ep connected to the encoder source ep.
     * 
     * When connecting ENC (P1) -> SCO sink (P0):
     * - on P1: find out the data format of ENC source ep, and that is
     *   advertised in the operator's _get_data_format()
     * - on P0: get the data format of the SCO sink ep, and that is given
     *   by sco_get_data_format(). If byte swap is not set, this will return
     *   AUDIO_DATA_FORMAT_16 = 0, leading to an unresolvable data format for
     *   operators which do not addvertise this data format. To allow operators
     *   using AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP on P1 to connect to the SCO
     *   sink, set this byte swap to TRUE.
     */
    cbuffer_set_read_byte_swap(sink_ep->state.sco.cbuffer, TRUE);

    /* Create the sco_sink_drv as part of the endpoint.
     * So far SCO does not support to air record framing. */
    sco_sink_drv = sco_sink_drv_data_create(FALSE);
    if (sco_sink_drv == NULL)
    {
        goto handle_error;
    }

   /* Connecting the sco_sink_drv input buffer to the MMU buffer will happen
    * when the transform creates that input buffer.
    */

    /* Install the sco drv. */
    sink_ep->state.sco.sco_drv.sco_sink_drv = sco_sink_drv;

    /* initialise measured rate for both sides */
    source_ep->state.sco.rate_measurement = 1<<STREAM_RATEMATCHING_FIX_POINT_SHIFT;
    sink_ep->state.sco.rate_measurement = 1<<STREAM_RATEMATCHING_FIX_POINT_SHIFT;

    /* Update incoming pointer parameters to give caller access to the
     * created source and sink cbuffer structures.
     */
    *source_cbuffer = source_ep->state.sco.cbuffer;
    *sink_cbuffer = sink_ep->state.sco.cbuffer;

    /* Succeeded */
    return TRUE;

handle_error:
    /* Cleanup source endpoint along with cbuffer and SCO driver instance
     * if they exist
     */
    if (source_ep != NULL)
    {
        if (source_ep->state.sco.cbuffer != NULL)
        {
            /* Free up the buffer and associated data space */
            cbuffer_destroy(source_ep->state.sco.cbuffer);
        }

        /* Destroy SCO driver instance */
        sco_clean_up_sco_drv(source_ep);

        source_ep->can_be_destroyed = TRUE;
        stream_destroy_endpoint(source_ep);
    }

    /* Cleanup sink endpoint and cbuffer if they exist */
    if (sink_ep != NULL)
    {
        if (sink_ep->state.sco.cbuffer != NULL)
        {
            /* Free up the buffer and associated data space */
            cbuffer_destroy(sink_ep->state.sco.cbuffer);
        }

        /* Destroy SCO driver instance */
        sco_clean_up_sco_drv(sink_ep);

        sink_ep->can_be_destroyed = TRUE;
        stream_destroy_endpoint(sink_ep);
    }

    /* Failed */
    return FALSE;
}

/****************************************************************************
 *
 * stream_delete_sco_endpoints_and_cbuffers
 *
 */
void stream_delete_sco_endpoints_and_cbuffers(unsigned int hci_handle)
{
    ENDPOINT *ep;

    /* The hci handle is the key (unique for the type and direction) */
    unsigned key = hci_handle;
    patch_fn_shared(stream_sco);

    /* Get and close the source endpoint associated with the hci handle */
    if ((ep = stream_get_endpoint_from_key_and_functions(key, SOURCE,
                                           &endpoint_sco_functions)) != NULL)
    {
        sco_iso_clean_up_endpoint(ep);
    }

    /* Get and close the sink endpoint associated with the hci handle */
    if ((ep = stream_get_endpoint_from_key_and_functions(key, SINK,
                                           &endpoint_sco_functions)) != NULL)
    {
        sco_iso_clean_up_endpoint(ep);
    }
}

/****************************************************************************
Private Function Definitions
*/

bool sco_close(ENDPOINT *endpoint)
{
    /* The endpoint still persists, but it has been released so potentially
     * another user can do something with it */
    endpoint->con_id = INVALID_CON_ID;

    return TRUE;
}

/**
 * \brief Connects the SCO source/sink endpoint.
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
bool sco_connect(ENDPOINT *endpoint,
                 tCbuffer *Cbuffer_ptr,
                 ENDPOINT *ep_to_kick,
                 bool *start_on_connect)
{
    ENDPOINT_GET_CONFIG_RESULT result;
    bool ret;

    ret = sco_get_config(endpoint, EP_DATA_FORMAT, &result);
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

bool sco_disconnect(ENDPOINT *endpoint)
{
    /* Clear out any configured hardware bit shift; this is necessary if
     * the SCO endpoint is to be subsequently reconnected.
     */
    cbuffer_set_read_shift(endpoint->state.sco.cbuffer, 0);
    cbuffer_set_write_shift(endpoint->state.sco.cbuffer, 0);

    buff_metadata_release(endpoint->state.sco.cbuffer);

    /* disconnect the sco_drv */
    if (endpoint->direction == SOURCE)
    {
        sco_src_drv_data_disconnect(sco_get_sco_src_drv(endpoint));
    }
    else
    {
        sco_sink_drv_data_disconnect(sco_get_sco_sink_drv(endpoint));
    }

    return TRUE;
}

/**
 * \brief Kicks down the chain only if this is a source.
 *
 * \param endpoint pointer to the endpoint that received a kick
 * \param kick_dir kick direction
 */
void sco_kick(ENDPOINT *endpoint, ENDPOINT_KICK_DIRECTION kick_dir)
{
    patch_fn_shared(stream_sco);

    endpoint_sco_state *sco_state = &endpoint->state.sco;
    unsigned hci_handle = endpoint->key;

#if defined(PROFILER_ON) && defined(INSTALL_SCOISO_EP_PROFILING)
    if (sco_state->profiler != NULL)
    {
        profiler_start(sco_state->profiler);
        sco_state->profiler->kick_inc++;
    }
#endif

    if (SOURCE == endpoint->direction)
    {
        TIME toa;
        SCO_SRC_DRV_DATA *sco_drv = sco_get_sco_src_drv(endpoint);
        sco_src_timing_info timing_info;
        int sp = STREAM_RATEMATCHING_RATE_TO_FRAC(sco_state->rate_measurement);

        /* We should have made sure that during creation and destruction
         * there is no race condition for which we end up serving the kick with
         * invalid data.  */
        PL_ASSERT(sco_drv);

        toa = time_add(sco_state->current_slot_time,
                       sco_from_air_latency_get(hci_handle));

        timing_info.toa_ep = toa;
        timing_info.sp_adjust = sp;

        /* Run the sco drv instance */
        sco_src_drv_processing(sco_drv, &timing_info);
    }
    else
    {
        SCO_SINK_DRV_DATA *sco_drv = sco_get_sco_sink_drv(endpoint);

        /* We should have made sure that during creation and destruction
         * there is no race condition for which we end up serving the kick with
         * invalid data.  */
        PL_ASSERT(sco_drv);

        /* Run the sco drv instance
         * SCO to-air reference time is sco_state->current_slot_time - to-air latency
         */
        sco_sink_drv_processing(sco_drv,
                                time_sub(sco_state->current_slot_time,
                                         sco_to_air_latency_get(hci_handle)), 
                                sco_tesco_get(hci_handle));

        /* If TTP is enabled we have to run the corresponding PID controller
         * here and send the resulting value to the rate adjust op
         * */
        if (sco_sink_drv_ttp_enabled(sco_drv))
        {
            int warp;
            warp = sco_sink_drv_ttp_ctrl_get_warp(sco_drv);
            stream_delegate_rate_adjust_set_current_rate(sco_state->external_rate_adjust_opid, warp);
        }
    }

#if defined(PROFILER_ON) && defined(INSTALL_SCOISO_EP_PROFILING)
    if (sco_state->profiler != NULL)
    {
        profiler_stop(sco_state->profiler);
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

void sco_start_from_air(ENDPOINT *endpoint)
{
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

void sco_start_to_air(ENDPOINT *endpoint)
{
    unsigned frame_size;
    unsigned hci_handle = stream_sco_get_hci_handle(endpoint);

    sco_to_air_endpoint_run_state_set(hci_handle, CHAIN_RUNNING);

    /* We need to have received valid parameters by now, 
     * to correctly set the timing information
     */
    PL_ASSERT(sco_params_received_get(hci_handle));

#if CHIP_HAS_CSR_BT || defined(TODO_SCO_USE_OPERATOR_FRAME_SIZE)
    unsigned block_size;

    /* The to-air frame length is specified by the encoder operator as the
     * block size, and is needed by BT to perform frame-based rate-matching:
     * if needed BTSS throws away a full encoded frame (and not part of it).
     */
    opmgr_get_block_size(endpoint->connected_to->id, &block_size);

    if (block_size == 0)
    {
        /* If we don't have a fixed size (i.e. SCO NB), make the frame size
         * the same as the packet size.
         */
        frame_size = sco_to_air_length_get(hci_handle);
    }
    else
    {
        /* For BT, the frame length needs to be specified in octets so turn
         * our word based definition into octets.
         */
        frame_size = CONVERT_SCO_WORDS_TO_OCTETS(block_size);
    }
#else
    frame_size = 0;
#endif /* CHIP_HAS_CSR_BT || defined(TODO_SCO_USE_OPERATOR_FRAME_SIZE) */

    sco_send_frame_length_and_run_state(hci_handle, SCO_DIR_TO_AIR,
                                        (uint16)frame_size);
}

#if defined(PROFILER_ON) && defined(INSTALL_SCOISO_EP_PROFILING)
/* Debug log string to allow profiler entries to be recognised by ACAT */
LOG_STRING(endpoint_name, "SCO EP");
#endif

bool sco_start(ENDPOINT *endpoint, KICK_OBJECT *ko)
{
    unsigned hci_handle = stream_sco_get_hci_handle(endpoint);
    
    /* Start the timers needed to run the endpoint at the correct timing.
     * Only do this if we have the relevant parameters to know that timing.
     */
    if (!sco_params_received_get(hci_handle))
    {
        L2_DBG_MSG("SCO endpoint starting without valid params, start will be deferred");
        endpoint->state.sco.start_pending = TRUE;
    }
    else
    {
        /* Set the endpoint kick period. Always kick at TeSCO for SCO endpoints */
        endpoint->state.sco.kick_period_us = sco_tesco_get(hci_handle);

        endpoint->state.sco.proc_time = SCOISO_SINK_PROCESSING_TIME();

        if (SOURCE == endpoint->direction)
        {
            unsigned ts_step, from_air_length, exp_ts;

            /* TODO exp_ts is defined an unsigned and set to UNKNONW (-1). */
            /* Initialize the expected timestamp (the 16 LSBs of the BT clock
             * in units of 312.5 us).
             */
            exp_ts = SCO_DRV_EXPECTED_TS_UNKNOWN;
            /* The amount to incremement the exp_ts with, at every kick
             * (converted to BT ticks).
             */
            ts_step = endpoint->state.sco.kick_period_us * 2 / US_PER_SLOT;
            /* from_air_length is stored in octets */
            from_air_length = sco_from_air_length_get(hci_handle);

            sco_src_drv_set_from_air_info(sco_get_sco_src_drv(endpoint),
                                          from_air_length,
                                          exp_ts,
                                          ts_step,
                                          FALSE,
                                          FALSE);
        }
        else
        {
            /* enable ttp logic only if rate_adjust OPID has been configured */
            bool ttp_enable = (0 == endpoint->state.sco.external_rate_adjust_opid)?FALSE:TRUE;

            sco_sink_drv_set_to_air_info(sco_get_sco_sink_drv(endpoint),
                                         sco_to_air_length_get(hci_handle),
                                         FALSE,
                                         ttp_enable);
        }

        if (sco_start_timers(endpoint, ko))
        {
            /* update the chains running state in case the other side needs
             * timing info to schedule in relation to this chain */
            if (SOURCE == endpoint->direction)
            {
                /* Setup the from-air endpoint */
                sco_start_from_air(endpoint);
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
        endpoint->state.sco.profiler = create_dynamic_profiler(endpoint_name, stream_external_id_from_endpoint(endpoint));
#endif
    }
    return TRUE;
}

void sco_get_timing (ENDPOINT *endpoint, ENDPOINT_TIMING_INFORMATION *time_info)
{
    /* Most of the work here is the same on all platforms, the wants kicks field
     * differs and is populated by the platform specific code. */
    sco_common_get_timing(endpoint, time_info);

    time_info->wants_kicks = FALSE;
}

bool sco_buffer_details(ENDPOINT *endpoint, BUFFER_DETAILS *details)
{
    if (endpoint == NULL || details == NULL)
    {
        return FALSE;
    }

    details->supports_metadata = TRUE;
    details->metadata_buffer = NULL;
    details->supplies_buffer = FALSE;
    details->b.buff_params.size = SCO_DEFAULT_SCO_BUFFER_SIZE;
    details->b.buff_params.flags = BUF_DESC_SW_BUFFER;

    details->runs_in_place = FALSE;
    return TRUE;
}

/**
 * sco_get_data_format
 */
AUDIO_DATA_FORMAT sco_get_data_format (ENDPOINT *endpoint)
{
    patch_fn_shared(stream_sco);
    if(stream_direction_from_endpoint(endpoint) == SOURCE)
    {
#ifdef CVSD_CODEC_SOFTWARE
        if (cbuffer_get_write_byte_swap(endpoint->state.sco.cbuffer) == FALSE)
        {
            return AUDIO_DATA_FORMAT_16_BIT;
        }
        return AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP;
#else /* CVSD_CODEC_SOFTWARE */
        if (cbuffer_get_write_shift(endpoint->state.sco.cbuffer) == (DAWTH- 16))
        {
            return AUDIO_DATA_FORMAT_FIXP;
        }
        return AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP;
#endif /* CVSD_CODEC_SOFTWARE */
    }
    else
    {
#ifdef CVSD_CODEC_SOFTWARE
        if (cbuffer_get_read_byte_swap(endpoint->state.sco.cbuffer) == FALSE)
        {
            return AUDIO_DATA_FORMAT_16_BIT;
        }
        return AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP;
#else /* CVSD_CODEC_SOFTWARE */
        if (cbuffer_get_read_shift(endpoint->state.sco.cbuffer) == (DAWTH- 16))
        {
            return AUDIO_DATA_FORMAT_FIXP;
        }
        return AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP;
#endif /* CVSD_CODEC_SOFTWARE */
    }
}

/**
 * sco_set_data_format
 */
bool sco_set_data_format (ENDPOINT *endpoint, AUDIO_DATA_FORMAT format)
{
    patch_fn_shared(stream_sco);
    /* The data format can only be set before connect */
    if (NULL != endpoint->connected_to)
    {
        return FALSE;
    }

    /* Sources and sinks have different data formats due to metadata */
    if(stream_direction_from_endpoint(endpoint) == SOURCE)
    {
        /* The sco drv needs to know how to read the data. */
        sco_src_drv_set_sco_data_format(sco_get_sco_src_drv(endpoint), format);

        switch(format)
        {
#ifdef CVSD_CODEC_SOFTWARE
        case AUDIO_DATA_FORMAT_16_BIT:
            cbuffer_set_write_shift(endpoint->state.sco.cbuffer, 0);
            cbuffer_set_write_byte_swap(endpoint->state.sco.cbuffer, FALSE);
            return TRUE;
#else /* CVSD_CODEC_SOFTWARE */
        case AUDIO_DATA_FORMAT_FIXP:
            cbuffer_set_write_shift(endpoint->state.sco.cbuffer, DAWTH - 16);
            cbuffer_set_write_byte_swap(endpoint->state.sco.cbuffer, FALSE);
            return TRUE;
#endif /* CVSD_CODEC_SOFTWARE */
        case AUDIO_DATA_FORMAT_ENCODED_DATA:        
        case AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP:
            cbuffer_set_write_shift(endpoint->state.sco.cbuffer, 0);
            cbuffer_set_write_byte_swap(endpoint->state.sco.cbuffer, TRUE);
            return TRUE;
        default:
            return FALSE;
        }
    }
    else
    {
        /* The sco drv needs to know how to read the data. */
        sco_sink_drv_set_sco_data_format(sco_get_sco_sink_drv(endpoint), format);

        switch(format)
        {
#ifdef CVSD_CODEC_SOFTWARE
        case AUDIO_DATA_FORMAT_16_BIT:
            cbuffer_set_read_shift(endpoint->state.sco.cbuffer, 0);
            cbuffer_set_read_byte_swap(endpoint->state.sco.cbuffer, FALSE);
            return TRUE;
#else /* CVSD_CODEC_SOFTWARE */
        case AUDIO_DATA_FORMAT_FIXP:
            cbuffer_set_read_shift(endpoint->state.sco.cbuffer, DAWTH - 16);
            cbuffer_set_read_byte_swap(endpoint->state.sco.cbuffer, FALSE);
            return TRUE;
#endif /* CVSD_CODEC_SOFTWARE */
        case AUDIO_DATA_FORMAT_ENCODED_DATA:
        case AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP:
            cbuffer_set_read_shift(endpoint->state.sco.cbuffer, 0);
            cbuffer_set_read_byte_swap(endpoint->state.sco.cbuffer, TRUE);
            return TRUE;
        default:
            return FALSE;
        }
    }
}

/**
 * flush_endpoint_buffers
 */
void flush_endpoint_buffers(ENDPOINT *ep)
{
    tCbuffer* cbuffer = ep->state.sco.cbuffer;
    patch_fn_shared(stream_sco);

    /* Wipe the buffer completely - history starts now, nobody knows what ended up in the buffer so far.
     * As it is SCO endpoint, the zero value used is... zero.
     */
    if(ep->direction == SOURCE)
    {
        cbuffer_empty_buffer(cbuffer);
    }
    else
    {
        cbuffer_flush_and_fill(cbuffer, 0);
    }
}


/**
 * \brief Connects the SCO/ISO source/sink endpoint.
 *        Called from both sco_connect and iso_connect.
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
 * \param  format            Endpoint data format needed to set the usable
 *                           octets for the transform buffer.
 *
 * \return TRUE on success.
 */
bool sco_iso_connect(ENDPOINT *endpoint,
                     tCbuffer *Cbuffer_ptr,
                     ENDPOINT *ep_to_kick,
                     bool *start_on_connect,
                     AUDIO_DATA_FORMAT format)
{
    unsigned usable_octets;

    endpoint->ep_to_kick = ep_to_kick;

    if (SOURCE == endpoint->direction)
    {
        if (!buff_has_metadata(Cbuffer_ptr))
        {
            /* The connected operator is expected to support metadata, so at
             * this point buffer should have metadata enabled, generate fault
             * if it isn't enabled.
             */
            L2_DBG_MSG("From-air transform buffer doesn't support metadata");
            return FALSE;
        }
        else
        {
            L2_DBG_MSG("From-air transform buffer connect with metadata");
        }

        /* Make a record of the SCO source endpoint in the sps structure for
         * first kick scheduling calculations.
         */
        sco_from_air_endpoint_set(stream_sco_get_hci_handle(endpoint), endpoint);

        /* Connect the sco_drv to the MMU buffer and to the transform
         * buffer. 
         */
        if (!sco_src_drv_data_connect(sco_get_sco_src_drv(endpoint),
                                      endpoint->state.sco.cbuffer,
                                      Cbuffer_ptr))
        {
            /* Something went wrong. This could have happened at the wrong
             * moment, or we missed something during EP creation.
             */
            /* The caller (connect_transform) checks for errors.
             * Let's use them
             */
            return FALSE;
        }
    }
    else
    {
        /* Make a record of the SCO sink endpoint in the sps structure for
         * first kick scheduling calculations. */
        sco_to_air_endpoint_set(stream_sco_get_hci_handle(endpoint), endpoint);

        /* Connect the sco_drv to the MMU buffer and to the transform
         * buffer. 
         */
        if (!sco_sink_drv_data_connect(sco_get_sco_sink_drv(endpoint),
                                       Cbuffer_ptr,
                                       endpoint->state.sco.cbuffer))
        {
            /* Something went wrong. This could have happened at the wrong
             * moment, or we missed something during EP creation.
             */
            /* The caller (connect_transform) checks for errors.
             * Let's use them
             */
            return FALSE;
        }
    }

    usable_octets = sco_iso_get_transf_buf_usable_octets(format);
    cbuffer_set_usable_octets(Cbuffer_ptr, usable_octets);

    *start_on_connect = FALSE;
    return TRUE;
}

