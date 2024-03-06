/****************************************************************************
 * Copyright (c) 2011 - 2020 Qualcomm Technologies International, Ltd.
 ****************************************************************************/
/**
 * \addtogroup Audio Audio endpoint
 * \ingroup endpoints
 * \file  stream_audio_hydra.c
 *
 * stream audio type file. <br>
 * This file contains stream functions for audio endpoints. <br>
 *
 * \section sec1 Contains:
 * stream_audio_get_endpoint <br>
 */

/****************************************************************************
Include Files
*/

#include "stream_private.h"
#include "stream_endpoint_audio.h" /* For protected access to audio ep base class */
#include "opmgr/opmgr_endpoint_override.h"
#include "pl_fractional.h"
#ifdef SUPPORTS_CONTINUOUS_BUFFERING
#include "aov_interface/aov_interface_for_cont_buff.h"
#endif /* SUPPORTS_CONTINUOUS_BUFFERING */

/****************************************************************************
Private Type Declarations
*/

/****************************************************************************
Private Constant Declarations
*/

#define RM_AVG_SHIFT      7
#define RM_AVG_RESOLUTION 20

/** Accumulation of rm_diff needs to happen always.
 *  delta_samples being non-zero is irrelevant.
 */
#define DELTA_SAMPLES_NOT_COMPUTED (unsigned int)(-1)

/* minimum tag length (in words)
 * Normally one tag should be generated for every new chunk of samples,
 * however to avoid generating tags too frequently, we make sure that
 * a tag covers a minimum number of samples. This limitation is a bit
 * relaxed for smaller buffer sizes, in a way full buffer size
 * can covey 4 min-length tags if needed, i.e:
 *
 * min_tag_len: min(AUDIO_SOURCE_TOA_MIN_TAG_LEN, buffer_size/4)
 *
 */
#define AUDIO_SOURCE_TOA_MIN_TAG_LEN (30)

/**
 * \brief Generates metadata for audio source endpoints.
 *
 * \param endpoint          Pointer to audio endpoint structure.
 * \param new_words_written Amount of new words written in the buffer.
 * \param last_sample_time  Time of the last sample written in the buffer.
 */
static void audio_generate_metadata(ENDPOINT *endpoint,
                                    unsigned new_words_written,
                                    TIME last_sample_time);


/****************************************************************************
Private Macro Declarations
*/

#define STREAM_AUDIO_HW_FRM_KEY(k)                 ((audio_hardware)(((k)>>8)&0xFF))

/*#define STREAM_AUDIO_HYDRA_WARP_VERBOSE*/

/****************************************************************************
Private Variable Definitions
*/

/****************************************************************************
Private Function Declarations
*/
static void destroy_audio_endpoint(ENDPOINT *endpoint);
static bool audio_close (ENDPOINT *endpoint);
static bool audio_connect (ENDPOINT *endpoint, tCbuffer *Cbuffer_ptr, ENDPOINT *ep_to_kick, bool* start_on_connect);
static bool audio_disconnect (ENDPOINT *endpoint);
static bool audio_buffer_details (ENDPOINT *endpoint, BUFFER_DETAILS *details);
static bool audio_start (ENDPOINT *endpoint, KICK_OBJECT *ko);
static bool audio_stop (ENDPOINT *endpoint);
static bool audio_configure (ENDPOINT *endpoint, unsigned int key, uint32 value);
static bool audio_get_config (ENDPOINT *endpoint, unsigned int key, ENDPOINT_GET_CONFIG_RESULT* result);
static void audio_get_timing (ENDPOINT *endpoint, ENDPOINT_TIMING_INFORMATION *time_info);
static bool audio_set_data_format (ENDPOINT *endpoint, AUDIO_DATA_FORMAT format);
static void adjust_audio_rate(ENDPOINT *ep, int32 adjust_val);
static bool enact_audio_rm(ENDPOINT* endpoint, uint32 value);
#if defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM)
static bool audio_init_warp_update_descs(ENDPOINT* ep);
static void adjust_audio_rate_warp(ENDPOINT *ep, int32 adjust_val);
static void set_rate_warp(ENDPOINT *ep, int use_val);
static void enact_audio_rm_warp(ENDPOINT *ep, uint32 val);
static int get_measured_sp_deviation(ENDPOINT *ep);
static int get_sp_deviation(ENDPOINT *ep);
static unsigned get_audio_rate(ENDPOINT* ep);
#endif /* defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM) */

#ifdef INSTALL_MCLK_SUPPORT
static bool stream_audio_activate_mclk_callback(unsigned sid, MCLK_MGR_RESPONSE status);
#endif
static bool is_locally_clocked(ENDPOINT *endpoint);

DEFINE_ENDPOINT_FUNCTIONS (audio_functions, audio_close, audio_connect,
                           audio_disconnect, audio_buffer_details,
                           stream_audio_kick, stream_sched_kick_dummy,
                           audio_start, audio_stop,
                           audio_configure, audio_get_config,
                           audio_get_timing, sync_endpoints,
                           stream_audio_have_same_clock);

/****************************************************************************
Private Function Declarations
*/

/****************************************************************************
Public Function Definitions
*/

/*
 * \brief Get the HW buffer size, depending on the size of the ping pong buffers
 *        (max_entries) if continuous buffering (AOV mode) is enabled for the EP.
 *
 * \param id The unique ID of the EP.

 * \return hw_buff_size The size that should be set for the HW buffer, in words.
 */
#ifdef SUPPORTS_CONTINUOUS_BUFFERING
static inline unsigned get_continuous_buff_hw_buff_size(unsigned id, tCbuffer *out_cbuffer_ptr)
{
    bool aov_mode;
    unsigned hw_buff_size;

    hw_buff_size = 0;
    /* aov_mode can only be enabled for a source endpoint, therefore no need
     * to check if EP is source or sink. */
    aov_mode = audio_vsm_get_aov_mode_from_sid(id);
    if (aov_mode)
    {

#ifdef AOV_EOD_ENABLED
        bool aov_runs_in_eod_mode = FALSE;
        mibgetbool(USEAOVEODMODE, &aov_runs_in_eod_mode);
        if (aov_runs_in_eod_mode)
        {
            /* If Empty on Demand is supported, the input buffer size is given by:
             * - non-zero value of MIB key EODAUDIOINPUTBUFFERSIZE or
             * - next power-of-two(size of transform buffer) */
            hw_buff_size = mibgetrequ16(EODAUDIOINPUTBUFFERSIZE);
            if (hw_buff_size == 0)
            {
                hw_buff_size = cbuffer_get_size_in_words(out_cbuffer_ptr);
            }
        }
        else
        {
            unsigned max_entries;
            max_entries = audio_vsm_get_max_entries_from_sid((SID) id);
            hw_buff_size = stream_audio_get_hw_buff_size(max_entries);
        }
#else
        unsigned max_entries;
        max_entries = audio_vsm_get_max_entries_from_sid((SID) id);
        hw_buff_size = stream_audio_get_hw_buff_size(max_entries);
#endif /* AOV_EOD_ENABLED */
    }

    return hw_buff_size;
}

#ifdef AOV_EOD_ENABLED
static inline bool enable_aov_eod_event(ENDPOINT *ep, KICK_OBJECT *ko)
{
    ENDPOINT *synced_ep;
    bool all_eps_aov_enabled = TRUE;
    bool all_aov_eod_enables_succeeded = TRUE;
    bool aov_eod_enable = FALSE;

    if(IS_ENDPOINT_HEAD_OF_SYNC(ep) && (ep->direction == SOURCE))
    {
        /* AoV-enabled endpoints in sync should all have aov_mode ON.
         * If one doesn't, the AoV EoD event will not be enabled for
         * any of them.
         */
        for (synced_ep = ep; synced_ep != NULL; synced_ep = synced_ep->state.audio.nep_in_sync)
        {
            all_eps_aov_enabled &= audio_vsm_get_aov_mode_from_sid(stream_external_id_from_endpoint(synced_ep));
        }
        if (all_eps_aov_enabled)
        {
            for (synced_ep = ep; synced_ep != NULL; synced_ep = synced_ep->state.audio.nep_in_sync)
            {
                bool evt_enable_result;
                evt_enable_result = stream_aov_eod_event_enable(synced_ep, ko);
                all_aov_eod_enables_succeeded &= evt_enable_result;
                PL_PRINT_P3(TR_STREAM, "audio_start enable EoD event for EP 0x%04X Kick Object 0x%08X with result %u",
                            stream_external_id_from_endpoint(synced_ep), ko, evt_enable_result);
            }
            /* Enabling eod event should succeed for all the endpoints in sync. */
            if (all_aov_eod_enables_succeeded)
            {
                aov_eod_enable = TRUE;
            }
            else
            {
                /* If any eod event enable doesn't succeed,
                 * we disable the event for all the endpoints. */
                for (synced_ep = ep; synced_ep != NULL; synced_ep = synced_ep->state.audio.nep_in_sync)
                {
                    stream_aov_eod_event_disable(synced_ep);
                }
            }
        }
    }

    return aov_eod_enable;
}
#endif /* AOV_EOD_ENABLED */
#else
static inline unsigned get_continuous_buff_hw_buff_size(unsigned id, tCbuffer *out_cbuffer_ptr)
{
    return 0;
}
#endif /* SUPPORTS_CONTINUOUS_BUFFERING */

/****************************************************************************
 *
 * stream_audio_get_endpoint
 *
 */
ENDPOINT *stream_audio_get_endpoint(CONNECTION_LINK con_id,
                                    ENDPOINT_DIRECTION dir,
                                    unsigned int hardware,
                                    unsigned num_params,
                                    unsigned *params,
                                    bool *pending)
{
    patch_fn_shared(stream_audio_hydra);

    if (num_params!=HYDRA_AUDIO_PARAMS_NUM)
    {
        return NULL;
    }

    int instance = params[HYDRA_AUDIO_PARAMS_INSTANCE];
    int channel  = params[HYDRA_AUDIO_PARAMS_CHANNEL];

    /* First go and find a stream on the same audio hardware, if not found
     * then create a new stream. */
    unsigned key = create_stream_key(hardware, instance, channel);
    ENDPOINT *endpoint = stream_get_endpoint_from_key_and_functions(key, dir,
                                                  &endpoint_audio_functions);
    if(!endpoint)
    {
        endpoint_audio_state* ep_audio;

        if ((endpoint = STREAM_NEW_ENDPOINT(audio, key, dir, con_id)) == NULL)
        {
            return NULL;
        }

        ep_audio = &endpoint->state.audio;

        /* All is well */
        endpoint->can_be_closed = TRUE;
        endpoint->can_be_destroyed = FALSE;
        /* Audio endpoints are always at the end of a chain */
        endpoint->is_real = TRUE;
        ep_audio->is_overridden = FALSE;

        /* Initialise endpoint synchronisation values */
        ep_audio->head_of_sync = endpoint;
        ep_audio->nep_in_sync = NULL;

        /* Initialise rate matching values */
        endpoint->deferred.config_deferred_kick = TRUE;
        ep_audio->rm_support = RATEMATCHING_SUPPORT_NONE;
        ep_audio->rm_adjust_amount = 0;
        ep_audio->rm_adjust_prev = 0;
        ep_audio->rm_report_sp_deviation = 0;
        ep_audio->rm_enable_sw_rate_adjust = TRUE;
        ep_audio->rm_enable_hw_rate_adjust = TRUE;
        ep_audio->rm_enable_clrm_measure = TRUE;

        ep_audio->direct_hw_warp_apply = FALSE;

#if defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM)
        ep_audio->rm_update_desc.channel_mask = 0;
#endif /* defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM) */

        /* By default we expect to produce FIXP audio, so activate the necessary HW shift and cbops */
        ep_audio->shift = DAWTH - 16;

        if(SOURCE == endpoint->direction)
        {
            /* support metadata by default */
            endpoint->state.audio.generate_metadata = TRUE;
        }

#ifdef INSTALL_UNINTERRUPTABLE_ANC
        /* ANC isn't using this newly created endpoint */
        ep_audio->anc.instance_id = STREAM_ANC_INSTANCE_NONE_ID;
        ep_audio->anc.input_path_id = STREAM_ANC_PATH_NONE_ID;
#endif /* INSTALL_UNINTERRUPTABLE_ANC */

        if(!stream_audio_post_create_check(hardware, instance, channel, dir,
                           endpoint, pending))
        {
            destroy_audio_endpoint(endpoint);
            return NULL;
        }

        if (!*pending)
        {
            endpoint->state.audio.hw_allocated = TRUE;
            /* Endpoint setup is complete, so create the endpoint cbops graph now */
            if (!stream_audio_create_endpoint_cbops(endpoint))
            {
                destroy_audio_endpoint(endpoint);
                return NULL;
            }
        }
        else
        {
            /* Endpoint cbops graph will be created when the pending setup completes */
            endpoint->state.audio.hw_allocated = FALSE;
        }
    }

#if defined(INSTALL_UNINTERRUPTABLE_ANC) && defined(INSTALL_ANC_STICKY_ENDPOINTS)
    endpoint_audio_state* ep_audio = &endpoint->state.audio;

    /* Newly opened/re-opened so indicate that ANC should not close the endpoint */
    ep_audio->anc.close_pending = FALSE;
#endif /* defined(INSTALL_UNINTERRUPTABLE_ANC) && defined(INSTALL_ANC_STICKY_ENDPOINTS) */

    return endpoint;
}

/****************************************************************************
Private Function Definitions
*/
/* ******************************* Helper functions ************************************ */

/* remove_from_synchronisation */
bool remove_from_synchronisation(ENDPOINT *ep)
{
    patch_fn_shared(stream_audio_hydra);

    if ( IS_AUDIO_ENDPOINT_SYNCED(ep) )
    {
        remove_from_sync_list(ep);
        /* Call into audio to reset the synchronisation groups in hardware */
        audio_vsm_sync_sids(stream_external_id_from_endpoint(ep), 0);
    }
    return TRUE;
}

/* add_to_synchronisation */
bool add_to_synchronisation(ENDPOINT *ep1, ENDPOINT *ep2)
{
    patch_fn_shared(stream_audio_hydra);

    /* If the two endpoint is the same or they are already synced return TRUE. */
    if ((ep1 == ep2) || (ALREADY_SYNCHRONISED(ep1,ep2)))
    {
        return TRUE;
    }

    /* Cannot synchronise with a running group of endpoints.*/
    if (SYNC_GROUP_IS_RUNNING(ep1) || SYNC_GROUP_IS_RUNNING(ep2))
    {
        return FALSE;
    }

    /* Call into audio to set the synchronisation groups in hardware */
    if(!audio_vsm_sync_sids(stream_external_id_from_endpoint(ep1), stream_external_id_from_endpoint(ep2)))
    {
        /* Hardware sync failed. So nothing else to do */
        return FALSE;
    }

    /* add new endpoint to the synchronisation list */

    return add_to_sync_list(ep1,ep2);
}

static void destroy_audio_endpoint(ENDPOINT *endpoint)
{
    /* Failed to get everything, give back what we might have asked for */
    if(endpoint->cbops != NULL)
    {
        /* This gets called on create audio endpoint. The cbops is still single slot */
        cbops_mgr_destroy(endpoint->cbops);
        endpoint->cbops = NULL;
    }
    stream_destroy_endpoint(endpoint);
}

#ifdef INSTALL_MCLK_SUPPORT
/**
 * \brief Activate mclk output for an audio interface

 * \param ep endpoint
 * \param activate_output if not 0, user wants to activate mclk OUTPUT for this endpoint, otherwise it will
 *        de-activate the output. Activation/De-activation request will only be done if:
 *        - the endpoint can have mclk output (e.g i2s master)
 *        - interface wants to route the MCLK output via GPIO, Note that the MCLK output can be generated from
 *          internal clock too.
 * \param enable_mclk makes the mclk available to use by the endpoint (instead of root clock). For an interface
 *        to use MCLK we need to make sure that the MCLK is available and stable this should be able to be done
 *        automatically before an interface gets activated(normally at connection point), so we might deprecate
 *        this flag in future.
 * \param pending set to TRUE by the endpoint if it needs to wait before the mclk becomes available
 */
bool stream_audio_activate_mclk(ENDPOINT *ep, unsigned activate_output, unsigned enable_mclk, bool *pending)
{
    return audio_vsm_activate_mclk(stream_external_id_from_endpoint(ep),
                                   (bool) activate_output,
                                   (bool) enable_mclk,
                                   &ep->state.audio.mclk_claimed,
                                   pending,
                                   stream_audio_activate_mclk_callback);
}
#endif /* #ifdef INSTALL_MCLK_SUPPORT */

/**
 * \brief closes the audio endpoint by requesting the release method on the
 *        audio hardware
 *
 * \param *endpoint pointer to the endpoint that is being closed.
 *
 * \return success or failure
 */
static bool audio_close (ENDPOINT *endpoint)
{
    patch_fn_shared(stream_audio_hydra);

#ifdef INSTALL_MCLK_SUPPORT
    if(endpoint->state.audio.mclk_claimed)
    {
        /* Endpoint has claimed mclk, it needs
         * to release it before we can close
         * the endpoint.
         */
        L2_DBG_MSG1("Failed to close audio endpoint (0x%x), MCLK needs to be released first", (unsigned)(uintptr_t)endpoint);

        return FALSE;
    }
#endif /* INSTALL_MCLK_SUPPORT */

    /* If endpoint is synced, remove it from sync list */
    if(!sync_endpoints(endpoint,NULL))
    {
        /* Fail if the sync failed */
        return FALSE;
    }
    if(endpoint->cbops != NULL)
    {
        /* Endpoint's cbops manager object will be independent of other endpoints since
         * it has been taken of a synchronisation list. So it is either a single slot
         * cbops manager object or a multichannel one with all the other channels marked
         * as not in use, because this is the last endpoint to call "desync".
         * In all cases, it is safe to call destroy.
         */
        cbops_mgr_destroy(endpoint->cbops);
        endpoint->cbops = NULL;
    }

    timed_playback_destroy(endpoint->state.audio.timed_playback);

#ifdef INSTALL_DELEGATE_RATE_ADJUST_SUPPORT
    if(0 != endpoint->state.audio.external_rate_adjust_opid)
    {
        /* The endpoint will close soon, set its standalone RATE_ADJUST
         * operator to passthrough mode (if it still exists of course).
         */
        stream_delegate_rate_adjust_set_passthrough_mode(endpoint->state.audio.external_rate_adjust_opid, TRUE);
    }
#endif
    /* By the time we reach this function we will have stopped everything from
     * running, so all we need to do is call the close hw method on audio, if it has
     * been allocated
     */
    return (endpoint->state.audio.hw_allocated)
                ? audio_vsm_release_hardware(stream_external_id_from_endpoint(endpoint))
                : TRUE;
}

#ifdef INSTALL_MCLK_SUPPORT
/**
 * \brief call back function for when endpoint needs to wait mclk claiming before
 *        activation takes place
 *
 * \param sid interface sid
 * \param status the result from mclk manager
 *
 * \return success or failure
 */
static bool stream_audio_activate_mclk_callback(unsigned sid, MCLK_MGR_RESPONSE status)
{
    ENDPOINT *ep = stream_endpoint_from_extern_id(sid);

    patch_fn_shared(stream_audio_hydra);

    if(MCLK_MGR_RESPONSE_CLAIM_SUCCESS == status)
    {
        /* we have claimed MCLK before using it */
        ep->state.audio.mclk_claimed = TRUE;
        /* activate mclk output if needed */
        audio_vsm_complete_mclk_output_activation(sid);
        /* report completion of mclk activation process  */
        stream_if_ep_mclk_activate_complete(ep, TRUE);
    }
    else if(MCLK_MGR_RESPONSE_RELEASE_SUCCESS == status)
    {
        /* we have released mclk,
         * the mclk output must already be de-activated
         * so everything is done now */
        ep->state.audio.mclk_claimed = FALSE;
        stream_if_ep_mclk_activate_complete(ep, TRUE);
    }
    else
    {
        /* failing to claim/release mclk */
        stream_if_ep_mclk_activate_complete(ep, FALSE);
    }

    return TRUE;
}
#endif /* #ifdef INSTALL_MCLK_SUPPORT */

/**
 * \brief Connect to the endpoint.
 *
 * \param *endpoint pointer to the endpoint that is being connected
 * \param *Cbuffer_ptr pointer to the Cbuffer struct for the buffer that is being connected.
 * \param *ep_to_kick pointer to the endpoint which will be kicked after a successful
 *              run. Note: this can be different from the connected to endpoint when
 *              in-place running is enabled.
 * \param *start_on_connect return flag which indicates if the endpoint wants be started
 *              on connect. Note: The endpoint will only be started if the connected
 *              to endpoint wants to be started too.
 *
 * \return success or failure
 */
static bool audio_connect(ENDPOINT *endpoint, tCbuffer *Cbuffer_ptr, ENDPOINT *ep_to_kick, bool* start_on_connect)
{
    unsigned int hw_buf_size;    /* in samples */
    unsigned flags;
    unsigned id;
    uint32 rate;

    patch_fn_shared(stream_audio_hydra);

    hw_buf_size = 0;
    id = stream_external_id_from_endpoint(endpoint);

    hw_buf_size = get_continuous_buff_hw_buff_size(id, Cbuffer_ptr);

    rate = audio_vsm_get_sample_rate_from_sid(id);

    if (hw_buf_size == 0)
    {
        hw_buf_size = stream_audio_get_buffer_length(rate,
                                                     endpoint->direction,
                                                     TRUE);
    }
    /* cbuffer size is in words, but we use octets from here on in. */
    endpoint->ep_to_kick = ep_to_kick;

    /* Calculate the monitor interrupt threshold based on system kick period and sample rate
     * This only gets used if the endpoint is head of sync
     */
    endpoint->state.audio.monitor_threshold = (uint32)(((uint64)stream_if_get_system_kick_period() * rate) / SECOND);

    if (endpoint->state.audio.monitor_threshold > hw_buf_size / 2)
    {
        endpoint->state.audio.monitor_threshold = hw_buf_size / 2;
    }

    L3_DBG_MSG2("audio_connect ep = %04X threshold = %u", stream_external_id_from_endpoint(endpoint),
        endpoint->state.audio.monitor_threshold);

    /* If this endpoint is synchronised and connected to the same operator then
     * we only need one kick to that operator. */
    if ( (ep_to_kick != NULL) && IS_AUDIO_ENDPOINT_SYNCED(endpoint))
    {
        ENDPOINT *p_ep;
        bool this_ep_reached_first = FALSE;
        for (p_ep = endpoint->state.audio.head_of_sync; p_ep != NULL; p_ep = p_ep->state.audio.nep_in_sync)
        {
            if (p_ep == endpoint)
            {
                this_ep_reached_first = TRUE;
            }
            else if (stream_is_connected_to_same_entity(p_ep, endpoint))
            {
                /* The first endpoint in the list that is connected to the operator should do the kicking */
                if (this_ep_reached_first)
                {
                    p_ep->ep_to_kick = NULL;
                }
                else
                {
                    endpoint->ep_to_kick = NULL;
                }
                break;
            }
        }
    }

    /* Set MMU handle sample size to 32-bit unpacked until we teach audio endpoints
     * to decide to use specific sample types.
     */
    flags = 0;
#ifdef BAC32
#ifdef TODO_CRESCENDO_STREAMS_SAMPLE_SIZE_SUPPORT
    flags = MMU_UNPACKED_32BIT_MASK;
#else /* TODO_CRESCENDO_STREAMS_SAMPLE_SIZE_SUPPORT */
#error "sample size support in streams is not implemented"
#endif /* TODO_CRESCENDO_STREAMS_SAMPLE_SIZE_SUPPORT */
#endif /* BAC32 */

    if(stream_direction_from_endpoint(endpoint) == SOURCE)
    {
        if (!endpoint->state.audio.is_overridden)
        {
            endpoint->state.audio.sink_buf = Cbuffer_ptr;
            endpoint->state.audio.source_buf = cbuffer_create_mmu_buffer(flags | BUF_DESC_MMU_BUFFER_HW_WR,
                    hw_buf_size);

            if (endpoint->state.audio.source_buf == NULL)
            {
                return FALSE;
            }
        }
        else
        {
            /* The input buffer is already wrapped because the buffer_details is
             * changed to BUF_DESC_MMU_BUFFER_HW_WR when the endpoint is overridden*/
            endpoint->state.audio.sink_buf = NULL;
            endpoint->state.audio.source_buf = Cbuffer_ptr;
        }

        /* Configure hardware transformation flags, to produce desired data format */
        cbuffer_set_write_shift(endpoint->state.audio.source_buf, endpoint->state.audio.shift);
    }
    else
    {
        if (!endpoint->state.audio.is_overridden)
        {
            endpoint->state.audio.source_buf = Cbuffer_ptr;
            endpoint->state.audio.sink_buf = cbuffer_create_mmu_buffer(flags | BUF_DESC_MMU_BUFFER_HW_RD,
                    hw_buf_size);
            if (endpoint->state.audio.sink_buf == NULL)
            {
                return FALSE;
            }
        }
        else
        {
            /* The input buffer is already wrapped because the buffer_details is
             * changed to BUF_DESC_MMU_BUFFER_HW_WR when the endpoint is overridden*/
            endpoint->state.audio.source_buf = NULL;
            endpoint->state.audio.sink_buf = Cbuffer_ptr;
        }
        /* Fill the buffer with silence so that the audio hardware reads silence
         * when it is connected. */
        cbuffer_flush_and_fill(endpoint->state.audio.sink_buf, get_ep_buffer_zero_value(endpoint));
        /* Configure hardware transformation flags, to produce desired data format */
        cbuffer_set_read_shift(endpoint->state.audio.sink_buf, endpoint->state.audio.shift);
    }

    if (!endpoint->state.audio.is_overridden)
    {
        if(stream_direction_from_endpoint(endpoint) == SINK)
        {

            /* Every ep created single-channel cbops. Now we arrived at the point when the cbops in the chain
               get created and set up, with buffer and alg params hooked in.

             1. If connect() is done before sync for this endpoint having been done, then
                connect is done on just the "original" single-channel cbops,
                adding buffer info etc. as in the old days...

                When later on the sync is done, we consolidate the various single-channel cbops into
                a multi-channel cbops, based on existing single-channel cbops information on buffers - but
                we only use sync head's information for various alg parameters!

             2. If sync was done before connect (so currently connected EP is found in a sync group, i.e.
                it has non-NULL nep_in_sync or the ptr to sync head is not pointing to itself)
                then populate the already existing multi-channel cbops with the connect information as per above;
                the single-channel cbops in that case was already destroyed for this endpoint and
                its cbop ptr field already points to the multi-channel cbops chain!

             */

            if((endpoint->state.audio.head_of_sync != endpoint) || (endpoint->state.audio.nep_in_sync != NULL))
            {
                /* This EP has already been added to a sync group, so "plug" its connection info into the multi-channel
                 * cbops chain created by the "add to sync" operation, which removed the single-channel cbops originally created for
                 * this endpoint.
                 */

                /* This just connects channel information up, cbops chain was already created! */
                if (!cbops_mgr_connect_channel(endpoint->cbops, endpoint->state.audio.channel,
                        endpoint->state.audio.source_buf, endpoint->state.audio.sink_buf))
                {
                    cbuffer_destroy(endpoint->state.audio.sink_buf);
                    endpoint->state.audio.sink_buf = NULL;
                    return FALSE;
                }
            }
            else
            {
                /* It's still a standalone endpoint, so connect as a single-channel cbops chain from scratch as in the older days.
                 * Fill in initial parameters cbops will use to communicate between themselves and with the endpoint(s).
                 */
                CBOP_VALS vals;
                stream_audio_set_cbops_param_vals(endpoint, &vals);

                if (!cbops_mgr_connect(endpoint->cbops, 1, &endpoint->state.audio.source_buf, &endpoint->state.audio.sink_buf, &vals))
                {
                    cbuffer_destroy(endpoint->state.audio.sink_buf);
                    endpoint->state.audio.sink_buf = NULL;
                    return FALSE;
                }
            }
        }
        else /* It is a source EP */
        {
            /* If source already in a sync group, then connect info in multi-channel cbops */
            if((endpoint->state.audio.head_of_sync != endpoint) || (endpoint->state.audio.nep_in_sync != NULL))
            {
                /* This ep has already been added to a sync group, so "plug" its connection info into the multi-channel
                 * cbops chain created by the "add to sync" operation, which removed the single-channel cbops originally created for
                 * this endpoint.
                 */

                /* This just connects channel information up, cbops chain was already created! */
                if (!cbops_mgr_connect_channel(endpoint->cbops, endpoint->state.audio.channel,
                        endpoint->state.audio.source_buf, endpoint->state.audio.sink_buf))
                {
                    cbuffer_destroy(endpoint->state.audio.source_buf);
                    endpoint->state.audio.source_buf = NULL;
                    return FALSE;
                }
            }
            else
            {
                /* Standalone endpoint, with its single-channel cbops created as in the older days.
                 * Using a barely populated common vals struct, the only values currently used by in-chain is
                 * rate adjust amount ptr and shift amount (also set inside cbops mgr). In future if vals carries any relevant value for whatever
                 * input chain operator, then fill fields here.
                 */
                CBOP_VALS vals;
                vals.rate_adjustment_amount = &(endpoint->state.audio.rm_adjust_amount);
                vals.shift_amount = 0;

                if (!cbops_mgr_connect(endpoint->cbops, 1, &endpoint->state.audio.source_buf, &endpoint->state.audio.sink_buf, &vals))
                {
                    cbuffer_destroy(endpoint->state.audio.source_buf);
                    endpoint->state.audio.source_buf = NULL;
                    return FALSE;
                }
            }
        }
        /* Initialise any rateadjust to passthrough as haven't been asked to rateadjust yet */
        cbops_mgr_rateadjust_passthrough_mode(endpoint->cbops, TRUE);
    }

    *start_on_connect = TRUE;
    return TRUE;
}
/**
 * \brief Disconnects from an endpoint and stops the data from flowing
 *
 * \param *endpoint pointer to the endpoint that is being disconnected
 *
 * \return success or failure
 */
static bool audio_disconnect(ENDPOINT *endpoint)
{
    patch_fn_shared(stream_audio_hydra);
    if (!endpoint->state.audio.is_overridden)
    {
        if((endpoint->state.audio.head_of_sync != endpoint) || (endpoint->state.audio.nep_in_sync != NULL))
        {
            /* If endpoint in a sync group, then mark the channel as unused in cbops.
             * The endpoint will keep hold of the same channel within the multichannel
             * cbop as the channel number was allocated in the synchronise routine */
            cbops_mgr_set_unused_channel(endpoint->cbops, endpoint->state.audio.channel);
        }
        else
        {
            /* For stand alone endpoints, disconnect the cbops */
            cbops_mgr_disconnect(endpoint->cbops);
        }
    }


    /* Wipe the cbuffer ptrs held in audio state just in case someone treats it as valid */
    if (SOURCE == endpoint->direction)
    {
        endpoint->state.audio.sink_buf = NULL;
        if (!endpoint->state.audio.is_overridden)
        {
            cbuffer_destroy(endpoint->state.audio.source_buf);
        }
        endpoint->state.audio.source_buf = NULL;
    }
    else
    {
        endpoint->state.audio.source_buf = NULL;
        if (!endpoint->state.audio.is_overridden)
        {
            cbuffer_destroy(endpoint->state.audio.sink_buf);
        }
        else
        {
            /* If the sink is the head of the sync group, it silences all the endpoints
             * in the group. Otherwise the endpoint silences itself only.
             */
            if(IS_ENDPOINT_HEAD_OF_SYNC(endpoint))
            {
                ENDPOINT *synced;
                /* At stop the output buffer is filled with silence. This doesn't get
                 * called when we're overridden so ensure it happens here in case the
                 * responsible party didn't do it.
                 */
                for (synced = endpoint; synced != NULL;
                                        synced = synced->state.audio.nep_in_sync)
                {
                    /* The buffer may have been NULLified already, if the endpoint in the group
                     * was disconnected. In that case, avoid trying to silence it.
                     */
                    if(synced->state.audio.sink_buf != NULL)
                    {
                        cbuffer_flush_and_fill(synced->state.audio.sink_buf,
                                               get_ep_buffer_zero_value(synced));
                    }
                }
            }
            else if(endpoint->state.audio.sink_buf != NULL)
            {
                cbuffer_flush_and_fill(endpoint->state.audio.sink_buf,
                                       get_ep_buffer_zero_value(endpoint));
            }
        }

        endpoint->state.audio.sink_buf = NULL;
    }

    /* Reset ep_to_kick to it's default state of no endpoint to kick*/
    endpoint->ep_to_kick = NULL;
    /* Clear the override flag */
    endpoint->state.audio.is_overridden = FALSE;
    return TRUE;
}

/**
 * \brief Obtains details of the buffer required for this connection
 *
 * \param endpoint pointer to the endpoint from which the buffer
 *        information is required
 * \param details pointer to the BUFFER_DETAILS structure to be populated.
 *
 * \return TRUE/FALSE success or failure
 *
 */
static bool audio_buffer_details(ENDPOINT *endpoint, BUFFER_DETAILS *details)
{
    patch_fn_shared(stream_audio_hydra);
    if (endpoint == NULL || details == NULL)
    {
        return FALSE;
    }

    /* Get required buffer size based on sample rate and kick period */
    details->b.buff_params.size = stream_audio_get_buffer_length(
            audio_vsm_get_sample_rate_from_sid(stream_external_id_from_endpoint(endpoint)), endpoint->direction, FALSE);

    if (endpoint->state.audio.is_overridden)
    {
        if (SOURCE == endpoint->direction)
        {
            details->b.buff_params.flags = BUF_DESC_MMU_BUFFER_HW_WR;
        }
        else
        {
            details->b.buff_params.flags = BUF_DESC_MMU_BUFFER_HW_RD;
        }
        /* Set MMU handle sample size to 32-bit unpacked until we teach audio endpoints
         * to decide to use specific sample types.
         */
#ifdef BAC32
#ifdef TODO_CRESCENDO_STREAMS_SAMPLE_SIZE_SUPPORT
        details->b.buff_params.flags |= MMU_UNPACKED_32BIT_MASK;
#else /* TODO_CRESCENDO_STREAMS_SAMPLE_SIZE_SUPPORT */
#error "sample size support in streams is not implemented"
#endif /* TODO_CRESCENDO_STREAMS_SAMPLE_SIZE_SUPPORT */
#endif /* BAC32 */
    }
    else
    {
        if (SINK == endpoint->direction)
        {
            /* check if any of the synced endpoints are already connected.*/
            ENDPOINT *synced_ep = endpoint->state.audio.head_of_sync;
            details->supports_metadata = TRUE;
            details->metadata_buffer = NULL;

            while(synced_ep)
            {
                /* return the first connected buffer*/
                if (synced_ep->state.audio.source_buf)
                {
                    details->metadata_buffer = synced_ep->state.audio.source_buf;
                    break;
                }
                synced_ep = synced_ep->state.audio.nep_in_sync;
            }
        }
        if (SOURCE == endpoint->direction)
        {
            /* check if any of the synced endpoints are already connected.
             * metadata is supported only if it has been enabled by the
             * user, user should enable it for all the synchronised endpoint
             * for this purpose we only look at head of sync group.
             */
            ENDPOINT *synced_ep = endpoint->state.audio.head_of_sync;
            if(synced_ep->state.audio.generate_metadata)
            {
                details->supports_metadata = TRUE;
                details->metadata_buffer = NULL;
                while(synced_ep)
                {
                    /* return the first connected buffer*/
                    if (synced_ep->state.audio.sink_buf)
                    {
                        details->metadata_buffer = synced_ep->state.audio.sink_buf;
                        break;
                    }
                    synced_ep = synced_ep->state.audio.nep_in_sync;
                }
            }
        }
        details->b.buff_params.flags = BUF_DESC_SW_BUFFER;
    }


    details->supplies_buffer = FALSE;
    details->runs_in_place = FALSE;
    details->can_override = TRUE;
    details->wants_override = TRUE;

    return TRUE;
}

/**
 * \brief Starts a kick interrupt source based off this audio endpoint.
 *
 * \param ep pointer to the endpoint which is responsible for scheduling
 * the kick.
 * \param ko pointer to the KICK_OBJECT that called start.
 *
 *  \return success or failure
 */
static bool audio_start (ENDPOINT *ep, KICK_OBJECT *ko)
{
    endpoint_audio_state* ep_audio;
    mmu_handle handle;
    /* in samples */
    unsigned int offset = 0;
    unsigned int max_offset;

    bool stream_mon_enable;
#ifdef AOV_EOD_ENABLED
    bool aov_runs_in_eod_mode = FALSE;
    bool aov_eod_enable;
    mibgetbool(USEAOVEODMODE, &aov_runs_in_eod_mode);
#endif /* AOV_EOD_ENABLED */

    ep_audio = &ep->state.audio;
    patch_fn_shared(stream_audio_hydra);

    stream_mon_enable = FALSE;
#ifdef AOV_EOD_ENABLED
    aov_eod_enable = FALSE;
#endif /* AOV_EOD_ENABLED */
    ep_audio->internal_kick_timer_id = TIMER_ID_INVALID;

    /* If endpoint is already running don't do anything more */
    if (ep_audio->running)
    {
        return TRUE;
    }

    /* Retrieve the buffer handle, max offset and initial offset for the endpoint */
    if (ep->direction == SOURCE)
    {
        handle = cbuffer_get_write_mmu_handle(ep->state.audio.source_buf);
        max_offset = mmu_buffer_get_size(handle);
        /* Set the offset to halfway through the buffer,
         * accounting for the fact that the read offset may have moved
         */
        offset = (mmu_buffer_get_handle_offset(handle) + (max_offset / 2)) % max_offset;
    }
    else
    {
        handle = cbuffer_get_read_mmu_handle(ep->state.audio.sink_buf);
        max_offset = mmu_buffer_get_size(handle);
    }

    if (IS_AUDIO_ENDPOINT_SYNCED(ep) && ep->direction == SOURCE)
    {
        /* If starting a source endpoint that is part of sync group, update the
         * same offset for all the connected source endpoints in the sync group */
        ENDPOINT *synced;
        for (synced = ep->state.audio.head_of_sync; synced != NULL;  synced = synced->state.audio.nep_in_sync)
        {
            if (synced->connected_to != NULL && synced->direction == SOURCE)
            {
                audio_vsm_update_initial_buffer_offset(stream_external_id_from_endpoint(synced), offset);
            }
        }
    }

    L3_DBG_MSG2("Activating EP: %08X with offset %d", stream_external_id_from_endpoint(ep), offset);
    if (!audio_vsm_activate_sid(stream_external_id_from_endpoint(ep), handle, offset, max_offset))
    {
        return FALSE;
    }

#ifdef AOV_EOD_ENABLED
    if (aov_runs_in_eod_mode)
    {
        aov_eod_enable = enable_aov_eod_event(ep, ko);
    }
#endif /* AOV_EOD_ENABLED */

    /* Set the running flag */
    ep_audio->running = TRUE;

    /* If endpoint is overridden, don't do anything else */
    if (ep_audio->is_overridden)
    {
        return TRUE;
    }

    /* No hard deadline, so we kick as and when data arrives */
    if (IS_ENDPOINT_HEAD_OF_SYNC(ep) && (ep_audio->monitor_threshold != 0))
    {
        unsigned int sample_rate;
        /* Set up a monitor to kick us whenever a certain number of
         * samples have arrived. This needs to be the same as the input
         * block size of the attached operator. Handily, that should
         * already be stashed in source->state.audio.monitor_threshold
         * (assuming that the far side *is* an operator, which it should be). */
        PL_PRINT_P1(TR_STREAM, "stream_IS_AUDIO_ENDPOINT_start: starting audio monitor every %d samples\n",
                        ep_audio->monitor_threshold);

        sample_rate = (unsigned int)
                      audio_vsm_get_sample_rate_from_sid(stream_external_id_from_endpoint(ep));

        /* Configure kick interrupts' handling to be deferred,
         * before enabling them */
        stream_set_deferred_kick(ep, ep->deferred.config_deferred_kick);

        if(ep->direction == SOURCE)
        {
            unsigned buf_offset;
            ENDPOINT *synced;

            /* Get read and write pointers sync'ed. When the first monitor interrupt
             * arrives, HW wrote a block worth of data. We always move the
             * pointer that we own. */
            /* Include an extra bit of headroom to give time to ensure the cbop
             * processing doesn't trample the write pointer. Start the interrupt
             * source first so that an interrupt here doesn't erode the priming
             * level. */

#ifdef AOV_EOD_ENABLED
            if(!aov_eod_enable)
#endif /* AOV_EOD_ENABLED */
            {
                stream_mon_enable = stream_monitor_int_wr_enable(ep,
                                 cbuffer_get_write_mmu_handle(ep_audio->source_buf),
                                 ko);
            }
            cbuffer_move_read_to_write_point(ep_audio->source_buf, CBOP_MIN_HEADROOM_SAMPLES);
            buf_offset = cbuffer_get_read_mmu_offset(ep_audio->source_buf);

            for (synced = ep; synced != NULL;  synced = synced->state.audio.nep_in_sync)
            {
                int *addr = synced->state.audio.source_buf->base_addr + buf_offset;
                synced->state.audio.source_buf->read_ptr = addr;
                cbuffer_move_write_to_read_point(synced->state.audio.sink_buf, 0);

                synced->state.audio.sample_rate = sample_rate;

                /* Align any metadata indexes with the pointers that have been moved.
                 * This probably only needs to happen for the head, but it's
                 * easier to do it for each channel. Only align the write pointer
                 * as that's the one that is moved here.
                 */
                buff_metadata_align_to_buff_write_ptr(ep_audio->sink_buf);
                ep_audio->last_tag_left_words = 0;

                /* set the minimum length of tags, in case the monitor level has been set to a very
                 * small value we make sure that the tag lengths aren't very small,
                 * this is set by AUDIO_SOURCE_TOA_MIN_TAG_LEN.
                 * Note: we also make sure that the full buffer size can convey 4 min-length tags,
                 * so the limitation will be further relaxed for low buffer sizes.
                 */
                ep_audio->min_tag_len = (cbuffer_get_size_in_words(ep_audio->sink_buf) -1) >> 2;
                if(ep_audio->min_tag_len > AUDIO_SOURCE_TOA_MIN_TAG_LEN)
                {
                    ep_audio->min_tag_len = AUDIO_SOURCE_TOA_MIN_TAG_LEN;
                }

                /* Clear down the buffer and metadata to ensure a consistent restart */
                LOCK_INTERRUPTS;
                cbuffer_empty_buffer_and_metadata(ep_audio->sink_buf);
                UNLOCK_INTERRUPTS;

                synced->state.audio.sync_started = FALSE;
            }
        }
        else
        {
            unsigned buf_offset;
            ENDPOINT *synced;

            /* Calculate a cbop headroom amount based on sampling rate and a default processing time allowance */
            unsigned initial_amount = ep_audio->monitor_threshold +
                                      AUDIO_RM_HEADROOM_AMOUNT +
                                      (CBOP_PROCESSING_TIME_ALLOWANCE_IN_USECS * sample_rate) / SECOND;

            /* get read and write pointers set such that by first interrupt,
             * the HW consumes a block of silence. After that, it will
             * silence insert until real data turns up. We move the pointers that
             * we own.
             */
            /* Include an extra bit of headroom to give time to perform the cbop processing before the read pointer
             * catches up. Start the interrupt source first so that an interrupt
             * here doesn't erode the priming level. */
            stream_mon_enable = stream_monitor_int_rd_enable(ep,
                         cbuffer_get_read_mmu_handle(ep_audio->sink_buf),
                         ko);
            cbuffer_move_write_to_read_point(ep_audio->sink_buf, initial_amount);
            buf_offset = cbuffer_get_write_mmu_offset(ep_audio->sink_buf);

            for (synced = ep; synced != NULL;  synced = synced->state.audio.nep_in_sync)
            {
                int *addr = synced->state.audio.sink_buf->base_addr + buf_offset;
                synced->state.audio.sink_buf->write_ptr = addr;
                cbuffer_move_read_to_write_point(synced->state.audio.source_buf, 0);

                synced->state.audio.sample_rate = sample_rate;

                /* Align any metadata indexes with the pointers that have been moved.
                 * This probably only needs to happen for the head, but it's
                 * easier to do it for each channel. Only align the read pointer
                 * as that's the one that is moved here. It's dangerous
                 * to align the write pointer as the operator may have added a delay
                 * to the write index already, and we'll trash that. */
                buff_metadata_align_to_buff_read_ptr(ep_audio->source_buf);

                synced->state.audio.sync_started = FALSE;

                /* Initialise the rate matching features. */
                synced->state.audio.rm_adjust_amount = 0;
                synced->state.audio.rm_diff = 0;
            }
        }
        /* Re-init the cbops with the Cbuffers as they are at this moment!
           Otherwise they would carry on from where they left off
           (if there was a previous stop, then start of the endpoint).
         */
        cbops_mgr_buffer_reinit(ep->cbops);

        rate_measure_set_nominal_rate(&ep_audio->rm_measure, sample_rate);
        rate_measure_stop(&ep_audio->rm_measure);
        /* TODO higher precision */
        ep_audio->rm_expected_time = ( (RATE_TIME)ep_audio->rm_measure.sample_period
                                  << RATE_SAMPLE_PERIOD_TO_TIME_SHIFT)
                                * ep_audio->monitor_threshold;
        ep_audio->rm_int_time = ep_audio->rm_expected_time;
        ep_audio->rm_period_start_time = (RATE_TIME)time_get_time()
                                         << RATE_TIME_EXTRA_RESOLUTION;

#ifdef SUPPORTS_CONTINUOUS_BUFFERING
        ep_audio->initialise_toa = TRUE;
        if(ep->direction == SINK)
        {
            ep_audio->initialise_toa = FALSE;
        }
        ep_audio->previous_toa = 0;
#endif /* SUPPORTS_CONTINUOUS_BUFFERING */

        if (ep_audio->source_buf->metadata)
        {
            ep_audio->data_flow_started = FALSE;
            ep_audio->use_timed_playback = FALSE;
        }

        if (stream_mon_enable)
        {
            ep_audio->monitor_enabled = TRUE;
        }
#ifdef AOV_EOD_ENABLED
        else if(aov_eod_enable)
        {
            ep_audio->aov_eod_enabled = TRUE;
        }
#endif /* AOV_EOD_ENABLED */
        else
        {
            /* We don't have any free monitors. Since the HW provides one monitor
             * per PCM slot, it implies that we probably shouldn't have ever
             * created this audio endpoint in the first place. */
            panic_diatribe(PANIC_AUDIO_STREAM_MONITOR_TOO_FEW_RESOURCES, stream_external_id_from_endpoint(ep));
        }
    }

    return TRUE;
}

/**
 * \brief Stops the kick interrupt source that was started by this endpoint.
 *
 * \param ep Pointer to the endpoint this function is being called on.
 *
 * \return success or failure
 */
static bool audio_stop (ENDPOINT *ep)
{
    KICK_OBJECT *kick_object;

    patch_fn_shared(stream_audio_hydra);
    if(!ep->state.audio.running)
    {
        /* The kick source is already stopped */
        return FALSE;
    }

    /* Deactivate the audio before unwrapping the mmu buffer as the audio code
     * does some tidying up of the buffer. */
    /* If the deactivation failed then don't complete the stop as endpoint
     * hardware may still be in use. */
    if (!audio_vsm_deactivate_sid(stream_external_id_from_endpoint(ep)))
    {
        return FALSE;
    }

    /* Update the running flag */
    ep->state.audio.running = FALSE;

    /* Make sure the timed playback struct is freed,
     * even if the endpoint is restarted
     */
    timed_playback_destroy(ep->state.audio.timed_playback);
    ep->state.audio.timed_playback = NULL;
    ep->state.audio.data_flow_started = FALSE;

    /* If endpoint is overridden, don't do anything else */
    if (ep->state.audio.is_overridden)
    {
        /*Until disconnect the ep is overridden*/
        return TRUE;
    }

    if (IS_ENDPOINT_HEAD_OF_SYNC(ep))
    {
        /* For audio endpoints we don't have hard deadlines
         * which means kick_object->ep_kick == kick_object->ep_sched.
         */
        kick_object = kick_obj_from_sched_endpoint(ep);
        if (NULL == kick_object)
        {
            /* The function call above is looking up the KICK_OBJECT that
             * called this function, so it's very bad if it doesn't exist. */
            panic_diatribe(PANIC_AUDIO_STREAM_MONITOR_STOP_FAILED,
                                stream_external_id_from_endpoint(ep));
        }

        if(ep->state.audio.monitor_enabled)
        {
            bool ok;
            if(ep->direction == SOURCE)
            {
                ok = stream_monitor_int_wr_disable(kick_object);
            }
            else
            {
                ok = stream_monitor_int_rd_disable(kick_object);
            }
            if (!ok)
            {
                panic_diatribe(PANIC_AUDIO_STREAM_MONITOR_STOP_FAILED,
                        stream_external_id_from_endpoint(ep));
            }
            ep->state.audio.monitor_enabled = FALSE;
            stream_set_deferred_kick(ep, FALSE);
        }
#ifdef AOV_EOD_ENABLED
        else if (ep->state.audio.aov_eod_enabled)
        {
            if(ep->direction == SOURCE)
            {
                ENDPOINT *synced_ep;
                for (synced_ep = ep; synced_ep != NULL; synced_ep = synced_ep->state.audio.nep_in_sync)
                {
                    PL_PRINT_P1(TR_STREAM, "audio_stop disable EoD event for EP 0x%04X",
                                stream_external_id_from_endpoint(synced_ep));
                    if(stream_aov_eod_event_disable(synced_ep))
                    {
                        ep->state.audio.aov_eod_enabled = FALSE;
                        stream_set_deferred_kick(ep, FALSE);
                    }
                    else
                    {
                        fault_diatribe(FAULT_AUDIO_AOV_EOD_DISABLE_FAILURE,
                                       stream_external_id_from_endpoint(synced_ep));
                    }
                }
            }
        }
#endif /* AOV_EOD_ENABLED */
    }

#if defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM)
    if (ep->state.audio.rm_support == RATEMATCHING_SUPPORT_HW)
    {
        enact_audio_rm_warp(ep, 0);
    }
#endif /* defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM) */

    if(ep->direction == SINK)
    {
        ENDPOINT *synced;
        /* Leave behind a pristine buffer if this is a sink - so only silence is pumped out.
         * The pointers are not bothered with, HW can free-run and start will snap the pointers
         * to proper places as per priming.
         */
        for (synced = ep; synced != NULL; synced = synced->state.audio.nep_in_sync)
        {
            cbuffer_flush_and_fill(synced->state.audio.sink_buf, get_ep_buffer_zero_value(synced));
        }
    }

    return TRUE;
}

#if defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM)
/**
 * Prepare to apply a HW ratematch related setting to all KCODEC channels
 * belonging to endpoints in the sync group. This should really match
 * the response given in stream_ratematch_mgr.c: audio_clock_source_same().
 * Limit to the same direction as the endpoint ep. Chris Avery says that
 * currently (Jun'16) you can't put a source and sink device in the same
 * sync group.
 */
static bool audio_init_warp_update_descs(ENDPOINT* ep)
{
    ENDPOINT* eps;
    WARP_UPDATE_DESC* warp_desc = &ep->state.audio.rm_update_desc;
    bool success;

    patch_fn_shared(stream_audio_hydra);

    audio_vsm_init_warp_update_desc(warp_desc, ep->direction == SINK);

    success = audio_vsm_add_warp_update_desc(warp_desc, stream_external_id_from_endpoint(ep));

    for (eps = ep->state.audio.head_of_sync;
            success && (eps != NULL);
            eps = eps->state.audio.nep_in_sync)
    {
        /* A FALSE return here would only mean that there is a device
         * in the sync group which is not covered by the same HW warp
         * mechanism. Not sure if that should fail the whole setup;
         * proceed for now.
         */
        audio_vsm_add_warp_update_desc(warp_desc, stream_external_id_from_endpoint(eps));
    }

    return success;
}

/**
 * adjust_audio_rate_warp
 */
static void adjust_audio_rate_warp(ENDPOINT *ep, int32 adjust_val)
{
    int use_val;
    endpoint_audio_state* audio = &ep->state.audio;

#ifdef STREAM_AUDIO_HYDRA_WARP_VERBOSE
    L2_DBG_MSG5("adjust_audio_rate((%d,%d,%d,%d,0x%02x) HW",
            stream_get_device_type(ep), get_hardware_instance(ep), get_hardware_channel(ep), ep->direction,
            audio->rm_update_desc.channel_mask
        );
#endif

    /*
     * A positive adjust_val means increase the source sampling
     * frequency, or decrease the sink sampling frequency.
     * Subsequently, deal in positive values for increases.
     */
    if (ep->direction == SINK)
    {
        adjust_val = - adjust_val;
    }

    /* Determine the actual bitfield required for the warp adjustment. The HW warp has +-3.125% adjustment available,
     * the adjust_val is for a wider range (and is sign-extended in a 32-bit word). The upshot is that (for the 32-bit
     * input representation), we need to arithmetic-shift "adjust_val" to give a 13-bit warp value in the LSBs.
     * Further info is in CS-236709-SP-E-Marco_Audio_Analog_APB_Register_Map
     * Note: the new warp value must be corrected by adding the current warp to the new warp setting, it is
     *       a rough approximation to: w_corr = 1 - (1-w_prev)*(1-w_new).
     */
    if (ep->state.audio.direct_hw_warp_apply
        || ep->state.audio.use_timed_playback)
    {
        /* Timed playback uses a PID controller, so for error feedback to work properly
         * the computed warp values must be applied directly(More accurately any
         * "strictly increasing function" of computed values would be ok as long as
         * it keeps the warp in suitable range), adding the needed change to warp
         * values is the responsibility of PID controller.
         */
        use_val = adjust_val;
    }
    else
    {
        /* The warp value has been computed based on the currently applied HW warp
         * rate, so we need to add that on top current value.
         */
        use_val = audio->rm_adjust_prev + (int)adjust_val;
    }
    set_rate_warp(ep, use_val);
}

static void set_rate_warp(ENDPOINT *ep, int use_val)
{
    endpoint_audio_state* audio = &ep->state.audio;

    patch_fn_shared(stream_audio_hydra);
    if (use_val >= FRACTIONAL(0.031250))
    {
        use_val = FRACTIONAL(0.031250) - 1;
    }
    else if (use_val < - FRACTIONAL(0.031250))
    {
        use_val = - FRACTIONAL(0.031250);
    }

    if (use_val != audio->rm_adjust_prev)
    {
        audio_vsm_set_warp(
                &audio->rm_update_desc,
                use_val,
                &audio->rm_hw_sp_deviation);
#ifdef STREAM_AUDIO_HYDRA_WARP_VERBOSE
        L3_DBG_MSG3("set_rate_warp rm_adjust_prev: %d -> %d, report: %d",
                    audio->rm_adjust_prev, use_val, audio->rm_hw_sp_deviation);
#endif
        audio->rm_adjust_prev = use_val;
    }
}
#endif /* defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM) */

static void adjust_audio_rate(ENDPOINT *ep, int32 adjust_val)
{
    patch_fn_shared(stream_audio_hydra);

    if (pl_abs_i32(adjust_val) >= FRACTIONAL(0.031250))
    {
        L3_DBG_MSG2("Audio adjustment : %d EP: %08X : discarded", (int)(adjust_val), (uintptr_t)ep);
        return;
    }

    L3_DBG_MSG2("Audio adjustment : %d EP: %08X", (int)(adjust_val), (uintptr_t)ep);

    /* The parameter adjust_val is a Q0.23 value (24bit arch) resp. Q0.31
     * (32bit arch), representing (real sink rate/real source rate) - 1.
     * I.e. for a SW rate adjuster:
     *  >0 means fractionally interpolate, <0 means fractionally decimate.
     * For a HW RM source:
     *  >0 means increase sampling frequency, <0 means decrease.
     * For a HW RM sink:
     *  >0 means decrease sampling frequency, <0 means increase.
     */
    switch (ep->state.audio.rm_support)
    {
        case RATEMATCHING_SUPPORT_SW:
#ifdef STREAM_AUDIO_HYDRA_WARP_VERBOSE
            L3_DBG_MSG5("adjust_audio_rate((%d,%d,%d,%d), %d) SW",
                    stream_get_device_type(ep), get_hardware_instance(ep), get_hardware_channel(ep), ep->direction,
                    (int)adjust_val
                );
#endif
            ep->state.audio.rm_adjust_amount = ((int)adjust_val);
#ifdef INSTALL_DELEGATE_RATE_ADJUST_SUPPORT
            if(0 != ep->state.audio.external_rate_adjust_opid
               && !ep->state.audio.use_timed_playback)
            {
                /* set the target rate value of the standalone rate adjust operator
                 * not required (will be ignored) if endpoint is in timed playback mode
                 */
                stream_delegate_rate_adjust_set_target_rate(ep->state.audio.external_rate_adjust_opid,
                                                            ep->state.audio.rm_adjust_amount);

            }
#endif /* INSTALL_DELEGATE_RATE_ADJUST_SUPPORT */

            break;

#if defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM)
        case RATEMATCHING_SUPPORT_HW:
            adjust_audio_rate_warp(ep, adjust_val);
            break;
#endif /* defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM) */

        default:
            break;
    }
    return;
}

#if defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM)
static void enact_audio_rm_warp(ENDPOINT *ep, uint32 val)
{
#ifdef STREAM_AUDIO_HYDRA_WARP_VERBOSE
    L3_DBG_MSG5("enact_audio_rm_warp (%d,%d,%d,%d) %d",
            stream_get_device_type(ep), get_hardware_instance(ep),
            get_hardware_channel(ep), ep->direction,
            val);
#endif
    if (val)
    {
        /* Convert the sync group into a channel bitmask.
         * This is safe because sync groups do not change while a
         * chain is running, and lazy because it will only be done
         * for endpoints which subsequently perform HW RM.
         */
        audio_init_warp_update_descs(ep);
    }
    else
    {
        endpoint_audio_state* audio = &ep->state.audio;

        /* Stop hardware rate adjustment on this endpoint */
        audio_vsm_set_warp(&audio->rm_update_desc, 0, &audio->rm_hw_sp_deviation);
        audio->rm_adjust_prev = 0;
        audio->rm_update_desc.channel_mask = 0;
    }
}
#endif /* defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM) */

static bool enact_audio_rm(ENDPOINT* endpoint, uint32 value)
{
    patch_fn_shared(stream_audio_hydra);

#if defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM)
    if (endpoint->state.audio.rm_support == RATEMATCHING_SUPPORT_HW)
    {
        enact_audio_rm_warp(endpoint, value);
        return TRUE;
    }
    else
#endif /* defined(INSTALL_CODEC) || defined(INSTALL_DIGITAL_MIC) || defined(INSTALL_AUDIO_INTERFACE_PWM) */
    {
        return audio_configure_rm_enacting(endpoint, value);
    }
}

/* KCODEC_ADC_GAIN_FINE values for digital mics mapping the kymera monotonic gain levels
 * 0-17 to go from -30dB to +24dB supported by the KCODEC ADC digital gain block
 * Bit 15 is set to enable the fine gain KCODEC_ADC_GAIN_SELECT_FINE */
static const uint16 digmic_digital_gain_word[] =
{
    0x8001,/* (0)-30.10299957 dB */
    0x8002,/* (1)-24.08239965 dB */
    0x8003,/* (2)-20.56057447 dB */
    0x8004,/* (3)-18.06179974 dB */
    0x8006,/* (4)-14.53997456 dB */
    0x8008,/* (5)-12.04119983 dB */
    0x800B,/* (6)-9.275145863 dB */
    0x8010,/* (7)-6.020599913 dB */
    0x8017,/* (8)-2.868442846 dB */
    0x8020,/* (9)0.0000000000 dB */
    0x802D,/* (10)2.961250709 dB */
    0x8040,/* (11)6.020599913 dB */
    0x805B,/* (12)9.077828280 dB */
    0x8080,/* (13)12.04119983 dB */
    0x80B4,/* (14)15.00245054 dB */
    0x80FF,/* (15)18.02780404 dB */
    0x8168,/* (16)21.02305045 dB */
    0x81FF /* (17)24.06541844 dB */
};

/**
 * \brief Translate between gain scales
 *
 * Operators overriding endpoints think about gains in a Kymera-defined
 * scale with 1/60 dB steps. The Hydra audio driver code inherits from
 * BlueCore a 'friendly gain scale' with 3dB steps. This function
 * translates from the former to the latter.
 *
 * \param hardware Hardware type for which the gain scale is required.
 * \param kymera_gain Gain in 2's complement 32-bit format, 1/60 dB steps
 * \param is_dac Whether this for ADC (towards DSP) or DAC (away from DSP) --
 *   influences zero point of gain scale
 *
 * \return An unsigned number on the traditional 0-22 gain scale (3dB steps)
 */
static unsigned int kymera_to_bluecore_gain(STREAM_DEVICE hardware, uint32 kymera_gain, bool is_dac)
{
    unsigned traditional_gain;
    int32 signed_gain = (int32)kymera_gain; /* XXX hope this works */

    /* The absolute values are a bit arbitrary. We make some sort of
     * effort to fit in with previous conventions, although those
     * conventions are mostly rumour. */

    signed_gain /= 60*3; /* 1/60dB steps -> 3dB steps */

    /* 'Zero points' on these scales are rumour. Source:
     * http://ukbugdb/B-215628#h10433714 */
    if (is_dac)
    {
        /* There's a pretty clear consensus that 15 is the zero point
         * for DACs. See for instance B-220681 */
        signed_gain += 15;
    }
    else
    {
        /* It's less clear what the zero point is for ADCs.
         * Pick a popular value. */
        signed_gain += 9;
    }

    /* The BlueCore gain scale traditionally has values 0-22.
     * We leave clipping at the high end to the audio driver, in case
     * higher levels are ever supported.
     * However, the interface is unsigned, so we clip at the low end. */
    traditional_gain = signed_gain > 0 ? (unsigned int)signed_gain : 0;
    if(hardware == STREAM_DEVICE_DIGITAL_MIC)
    {
        /* digital mics expect the raw gain value. Other hardware devices can manage
         * the platform dependent value on their own.
         */
        if(traditional_gain>=ARRAY_LENGTH(digmic_digital_gain_word))
        {
            traditional_gain = ARRAY_LENGTH(digmic_digital_gain_word)-1;
        }
        return digmic_digital_gain_word[traditional_gain];
    }
    return traditional_gain;
}

/**
 * \brief configure an audio endpoint with a key and value pair
 *
 * \param *endpoint pointer to the endpoint to be configured
 * \param key denoting what is being configured
 * \param value value to which the key is to be configured
 *
 * \return Whether the request succeeded.
 *
 */
static bool audio_configure(ENDPOINT *endpoint, unsigned int key, uint32 value)
{
    endpoint_audio_state* ep_audio = &endpoint->state.audio;

    patch_fn_shared(stream_audio_hydra);
    if((key & ENDPOINT_INT_CONFIGURE_KEYS_MASK) != 0)
    {
        switch (key)
        {
        case EP_DATA_FORMAT:
            return audio_set_data_format(endpoint, (AUDIO_DATA_FORMAT)value);

        case EP_OVERRIDE_ENDPOINT:
        {
            /* Set the logical value of override flag. */
            ep_audio->is_overridden = (bool)value;

            return TRUE;
        }

        case EP_CBOPS_PARAMETERS:
        {
            bool retval;
            CBOPS_PARAMETERS *parameters = (CBOPS_PARAMETERS *)(uintptr_t) value;

            if (parameters)
            {
                /* cbops_mgr should not be updated when endpoint is running. */
                if (!endpoint->is_enabled &&  endpoint->cbops && \
                    opmgr_override_pass_cbops_parameters(parameters, endpoint->cbops,
                                                         ep_audio->source_buf,
                                                         ep_audio->sink_buf))
                {
                    retval = TRUE;
                }
                else
                {
                    retval = FALSE;
                }
            }
            else
            {
                /* Panic can return in unit test*/
                retval = FALSE;
                panic_diatribe(PANIC_AUDIO_STREAM_INVALID_CONFIGURE_KEY,
                                                                endpoint->id);
            }

            free_cbops_parameters(parameters);
            return retval;
        }

        case EP_HW_WARP_APPLY_MODE:
            /* set hw warp apply mode */
            ep_audio->direct_hw_warp_apply = (value != 0);
            return TRUE;

        case EP_RATEMATCH_ADJUSTMENT:
            adjust_audio_rate(endpoint, (int32)value);
            return TRUE;
        case EP_RATEMATCH_REFERENCE:
            return FALSE;
        case EP_RATEMATCH_ENACTING:
            return enact_audio_rm(endpoint, value);
        case EP_SET_INPUT_GAIN:
        {
            stream_config_key cfg_key;
            STREAM_DEVICE hardware = stream_get_device_type(endpoint);

            switch(hardware)
            {
            case STREAM_DEVICE_CODEC:
                cfg_key = STREAM_CONFIG_KEY_STREAM_CODEC_INPUT_GAIN;
                break;
            case STREAM_DEVICE_DIGITAL_MIC:
                cfg_key = STREAM_CONFIG_KEY_STREAM_DIGITAL_MIC_INPUT_GAIN;
                break;
            default:
                return FALSE;
            }
            return audio_vsm_configure_sid(stream_external_id_from_endpoint(endpoint),
                                      cfg_key, kymera_to_bluecore_gain(hardware, value, FALSE));
        }
        case EP_SET_OUTPUT_GAIN:
            if (stream_get_device_type(endpoint) == STREAM_DEVICE_CODEC)
            {
                return audio_vsm_configure_sid(stream_external_id_from_endpoint(endpoint),
                                           STREAM_CONFIG_KEY_STREAM_CODEC_OUTPUT_GAIN,
                                           kymera_to_bluecore_gain(STREAM_DEVICE_CODEC, value, TRUE));
            }
            else
            {
                return FALSE;
            }

        default:
            return FALSE;
        }
    }
    else
    {
        switch (key)
        {
#ifdef INSTALL_UNINTERRUPTABLE_ANC
            /* Set the ANC instance associated with the endpoint */
            case STREAM_CONFIG_KEY_STREAM_ANC_INSTANCE:

            /* Set the ANC input path associated with the endpoint */
            case STREAM_CONFIG_KEY_STREAM_ANC_INPUT:

            /* Configure the ANC DC filter/SM LPF */
            case STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_ENABLE:
            case STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_ENABLE:
            case STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_ENABLE:
            case STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_SHIFT:
            case STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_SHIFT:
            case STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_SHIFT:

            /* Configure the ANC path gains */
            case STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN:
            case STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN:
            case STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN:
            case STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN_SHIFT:
            case STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN_SHIFT:
            case STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN_SHIFT:

#ifdef INSTALL_ANC_V2P0
            /* Configure the ANC RxMix path gains and shift */
            case STREAM_CONFIG_KEY_STREAM_ANC_RX_MIX_FFA_GAIN:
            case STREAM_CONFIG_KEY_STREAM_ANC_RX_MIX_FFB_GAIN:
            case STREAM_CONFIG_KEY_STREAM_ANC_RX_MIX_FFA_SHIFT:
            case STREAM_CONFIG_KEY_STREAM_ANC_RX_MIX_FFB_SHIFT:
#endif
            /* Enable adaptive ANC */
            case STREAM_CONFIG_KEY_STREAM_ANC_FFA_ADAPT_ENABLE:
            case STREAM_CONFIG_KEY_STREAM_ANC_FFB_ADAPT_ENABLE:
            case STREAM_CONFIG_KEY_STREAM_ANC_FB_ADAPT_ENABLE:

            /* Set ANC controls */
#ifdef INSTALL_ANC_V2P0
            case STREAM_CONFIG_KEY_STREAM_ANC_CONTROL_1:
#endif
            case STREAM_CONFIG_KEY_STREAM_ANC_CONTROL:
            {
                bool success;

                /* Is this a source or sink endpoint? */
                if (stream_direction_from_endpoint(endpoint) == SOURCE)
                {
                    success = stream_anc_source_configure(endpoint, (STREAM_CONFIG_KEY)key, value);
                }
                else
                {
                    success = stream_anc_sink_configure(endpoint, (STREAM_CONFIG_KEY)key, value);
                }

                return success;
            }
#endif /* INSTALL_UNINTERRUPTABLE_ANC*/

            case STREAM_CONFIG_KEY_STREAM_RM_ENABLE_SW_ADJUST:
                ep_audio->rm_enable_sw_rate_adjust = (value != 0);
                return TRUE;
            case STREAM_CONFIG_KEY_STREAM_RM_ENABLE_HW_ADJUST:
                ep_audio->rm_enable_hw_rate_adjust = (value != 0);
                return TRUE;
            case STREAM_CONFIG_KEY_STREAM_RM_ENABLE_DEFERRED_KICK:
                endpoint->deferred.config_deferred_kick = (value != 0);
                return TRUE;

            case STREAM_CONFIG_KEY_STREAM_AUDIO_SINK_DELAY:
            {
                if(SINK == endpoint->direction)
                {
                    endpoint->state.audio.endpoint_delay_us = value;
                    if (endpoint->state.audio.timed_playback != NULL)
                    {
                        timed_playback_set_delay(endpoint->state.audio.timed_playback, value);
                    }
                    return TRUE;
                }
                return FALSE;
            }
            case STREAM_CONFIG_KEY_STREAM_AUDIO_SOURCE_METADATA_ENABLE:
            {
                /* only when the source endpoint is disconnected can we
                 * change the generate_metadata flag.
                 */
                if(SOURCE == endpoint->direction &&
                   NULL == endpoint->connected_to)
                {
                    endpoint->state.audio.generate_metadata = (bool) value;
                    return TRUE;
                }
                return FALSE;
            }
            case STREAM_CONFIG_KEY_STREAM_AUDIO_SAMPLE_PERIOD_DEVIATION:
            {
                if (! endpoint->state.audio.running)
                {
#if DAWTH<32
                    endpoint->state.audio.rm_report_sp_deviation = (int)((int32)value >> (32-DAWTH));
#else
                    endpoint->state.audio.rm_report_sp_deviation = value;
#endif
                    return TRUE;
                }
                else
                {
                    return FALSE;
                }
            }
#ifdef INSTALL_DELEGATE_RATE_ADJUST_SUPPORT
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
                }
                else
                {
                    /* opid == 0 intends to disassociate the endpoint from standalone
                     * rate adjust operator, set the operator to pass-through mode
                     * if it is still being used
                     */
                    unsigned cur_opid = endpoint->state.audio.head_of_sync->state.audio.external_rate_adjust_opid;
                    if(cur_opid != 0 )
                    {
                        stream_delegate_rate_adjust_set_passthrough_mode(cur_opid, TRUE);
                    }
                }

                /* set the rate adjust op that is available to this endpoint
                 */
                endpoint->state.audio.external_rate_adjust_opid = opid;

                /* also use the same value for head of sync group */
                endpoint->state.audio.head_of_sync->state.audio.external_rate_adjust_opid = opid;

                return TRUE;
            }
#endif /* INSTALL_DELEGATE_RATE_ADJUST_SUPPORT */
            case STREAM_CONFIG_KEY_STREAM_AUDIO_DISABLE_ENDPOINT_PROCESSING:
            {
                /* cbops_mgr should not be updated when endpoint is running. */
                if (!endpoint->is_enabled)
                {
                    switch (value)
                    {
                        case ACCMD_AUDIO_ENDPOINT_DISABLE_PROCESSING_ALL:
                        {
                            /* Clear all Cbops flag for this end point */
                            endpoint->cbops->req_ops = CBOPS_COPY_ONLY;
                            return TRUE;
                        }
                        case ACCMD_AUDIO_ENDPOINT_DISABLE_PROCESSING_DC_REMOVE:
                        {
                            /* Clear DC remove Cbops flag for this end point */
                            endpoint->cbops->req_ops &= ~CBOPS_DC_REMOVE;
                            return TRUE;
                        }
                        default:
                            /* Non-valid key */
                            return FALSE;
                    }
                }
                else
                {
                    return FALSE;
                }
            }
#ifdef SUPPORTS_CONTINUOUS_BUFFERING
            case STREAM_CONFIG_KEY_STREAM_CODEC_AOV_MODE_ON:
            case STREAM_CONFIG_KEY_STREAM_DIGITAL_MIC_AOV_MODE_ON:
            {
                SID sid = stream_external_id_from_endpoint(endpoint);
                if (audio_vsm_get_max_entries_from_sid(sid) == 0 &&
                    (bool)value == TRUE)
                {
                   fault_diatribe(FAULT_AUDIO_AOV_BUFFER_SIZE_NOT_CONFIGURED, sid);
                   return FALSE;
                }
                if (audio_vsm_configure_sid(sid, (stream_config_key)key, value))
                {
                    aov_set_continuous_buff_mode((bool)value);
                    return TRUE;
                }
                else
                {
                    return FALSE;
                }
            }
#endif /* SUPPORTS_CONTINUOUS_BUFFERING */
            default:
                return audio_vsm_configure_sid( stream_external_id_from_endpoint(endpoint),
                                                (stream_config_key)key, value);
        }
    }
}

/**
 * Return a relative deviation of the sample period to the nominal
 * period, i.e.
 *
 *   measured sample period
 *   ---------------------- - 1
 *   nominal sample period
 *
 * as a signed fractional number.
 */
static int get_measured_sp_deviation(ENDPOINT *ep)
{
    int val;
    endpoint_audio_state* audio;

    patch_fn_shared(stream_audio_hydra);

    audio = &ep->state.audio;

    RATE_STIME diff_time = (RATE_STIME)audio->rm_int_time
                           - (RATE_STIME)audio->rm_expected_time;

    /* Right shift and pl_fractional_divide should be done on
     * non-negative values.
     */
    RATE_SHORT_INTERVAL diff_interval;
    if (diff_time >= 0)
    {
        diff_interval = (RATE_SHORT_INTERVAL)(diff_time >> RATE_SAMPLE_PERIOD_TO_TIME_SHIFT);
        val = pl_fractional_divide(diff_interval,
                                   audio->monitor_threshold
                                   * audio->rm_measure.sample_period);
    }
    else
    {
        diff_interval = (RATE_SHORT_INTERVAL)(- diff_time >> RATE_SAMPLE_PERIOD_TO_TIME_SHIFT);
        val = - pl_fractional_divide(diff_interval,
                                     audio->monitor_threshold
                                     * audio->rm_measure.sample_period);
    }

    return val;
}

/**
 * \brief Return a relative deviation of the sample period to the nominal
 * sample period, as a signed fractional.
 *
 * \note See also get_measured_sp_deviation.
 */
static int get_sp_deviation(ENDPOINT *ep)
{
    int val = 0;
    endpoint_audio_state* audio;

    patch_fn_shared(stream_audio_hydra);

    audio = &ep->state.audio;

    /* If the endpoint isn't running or the DSP owns the clock then there is
     * nothing to compensate for so indicate that the rate is perfect. */
    if (!audio->running || is_locally_clocked(ep))
    {
        if (audio->rm_support == RATEMATCHING_SUPPORT_HW)
        {
            val = audio->rm_hw_sp_deviation;
        }
    }
    else
    {
        val = get_measured_sp_deviation(ep);
    }

    if (val != audio->rm_report_sp_deviation)
    {
        audio->rm_report_sp_deviation = val;
#ifdef STREAM_AUDIO_HYDRA_WARP_VERBOSE
        L3_DBG_MSG4("get_sp_deviation(0x%04x,%d) %d * 2^-31 = %d * 10e-6",
                    ep->key, ep->direction, val, frac_mult(val, 1000000));
#endif
    }
    return val;
}

/**
 * Return an approximate quotient (measured sample rate)/(nominal sample rate),
 * in Qm.22
 */
static unsigned get_audio_rate(ENDPOINT* ep)
{
    int sp_deviation = get_sp_deviation(ep);
    int rel_rate;

    /* RM_PERFECT_RATE is 1.0 in Qm.22 */
    rel_rate = RM_PERFECT_RATE - frac_mult(sp_deviation, RM_PERFECT_RATE);

    return (unsigned)rel_rate;
}


static bool is_locally_clocked(ENDPOINT *endpoint)
{
    SID sid = stream_external_id_from_endpoint(endpoint);
#ifdef INSTALL_MCLK_SUPPORT
    /* The case where an interface is clocked from MPLL but using a
     * non-standard rate, is expected to only occur when using MPLL
     * to rate match under SW control. In this case this interface
     * reports locally clocked, and should report HW rate match capable.
     */
    if (audio_vsm_get_master_mode_from_sid(sid))
    {
        if (! audio_vsm_mclk_is_in_use())
        {
            /* No interface is using external MCLK, so this one can't */
            return TRUE;
        }

        /* If any endpoint in the same sync group has claimed MCLK,
         * that is assumed to be the clock for this endpoint.
         * Only nonsense cases would not fulfil this assumption.
         */
        ENDPOINT* ep_in_sync;
        for (ep_in_sync = endpoint->state.audio.head_of_sync;
             ep_in_sync != NULL;
             ep_in_sync = ep_in_sync->state.audio.nep_in_sync)
        {
            if (ep_in_sync->state.audio.mclk_claimed)
            {
                return FALSE;
            }
        }
        /* Something is using MCLK, but the sync group of which
         * this endpoint is a member is not.
         */
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#else
    return audio_vsm_get_master_mode_from_sid(sid);
#endif
}

/*
 * \brief get audio endpoint configuration
 *
 * \param *endpoint pointer to the endpoint to be configured
 * \param key denoting what is being configured
 * \param value pointer to a value which is populated with the current value
 *
 * \return Whether the request succeeded.
 *
 */
static bool audio_get_config(ENDPOINT *endpoint, unsigned int key, ENDPOINT_GET_CONFIG_RESULT* result)
{
    endpoint_audio_state* audio = &endpoint->state.audio;

    patch_fn_shared(stream_audio_hydra);
    switch (key)
    {
    case EP_DATA_FORMAT:
        result->u.value = audio_get_data_format(endpoint);
        return TRUE;
    case STREAM_INFO_KEY_AUDIO_SAMPLE_RATE:
        result->u.value = audio_vsm_get_sample_rate_from_sid(stream_external_id_from_endpoint(endpoint));
        return TRUE;
    case EP_CBOPS_PARAMETERS:
    {
        CBOPS_PARAMETERS *parameters;
        unsigned cbops_flags = cbops_get_flags(endpoint->cbops);
        parameters = create_cbops_parameters(cbops_flags, EMPTY_FLAG);

        if (!parameters)
        {
            return FALSE;
        }

        /* Set additional parameters if needed.*/
        result->u.value = (uint32)(uintptr_t) parameters;
        return TRUE;
    }

    case EP_CURRENT_HW_WARP:
    {
        /* return the latest hw warp value */
        result->u.value = (uint32) audio->rm_hw_sp_deviation;
        return TRUE;
    }

    case EP_RATEMATCH_ABILITY:
    {
        bool success;
#ifdef INSTALL_DELEGATE_RATE_ADJUST_SUPPORT
            if(0 != audio->external_rate_adjust_opid)
            {
                /* report HW rate adjust ability, this way most likely that
                 * this endpoint will be chosen as enacting side.
                 * TODO: new support type could be added to rate match manager
                 */
                result->u.value = (uint32)RATEMATCHING_SUPPORT_HW;

                /* support type is SW though */
                audio->rm_support = RATEMATCHING_SUPPORT_SW;
                success = TRUE;
            }
            else
#endif /* INSTALL_DELEGATE_RATE_ADJUST_SUPPORT */
        if (stream_audio_hw_rate_adjustment_supported(endpoint))
        {
            result->u.value = (uint32)RATEMATCHING_SUPPORT_HW;
            audio->rm_support = RATEMATCHING_SUPPORT_HW;
            success = TRUE;
        }
        else if (audio->rm_enable_sw_rate_adjust)
        {
            success = audio_get_config_rm_ability(endpoint, &result->u.value);
            if (success)
            {
                audio->rm_support = (unsigned)(result->u.value);
            }
        }
        else
        {
            result->u.value = RATEMATCHING_SUPPORT_NONE;
            audio->rm_support = RATEMATCHING_SUPPORT_NONE;
            success = TRUE;
        }
#ifdef STREAM_AUDIO_HYDRA_WARP_VERBOSE
        L3_DBG_MSG4("str_aud_hyd aud_get_cfg ((%d,%d,%d,%d), RM_ABIL)",
                stream_get_device_type(endpoint), get_hardware_instance(endpoint),
                get_hardware_channel(endpoint), endpoint->direction);
        L3_DBG_MSG2("... %d, %d", success, result->u.value);
#endif
        return success;
    }
    case EP_RATEMATCH_RATE:
        result->u.value = get_audio_rate(endpoint);
        L3_DBG_MSG2("Audio rate : %d EP: %08X", result->u.value, (uintptr_t)endpoint);
        return TRUE;
    case EP_RATEMATCH_MEASUREMENT:
        result->u.rm_meas.sp_deviation = get_sp_deviation(endpoint);
        if (audio->rm_enable_clrm_measure)
        {
            result->u.rm_meas.measurement.nominal_rate_div25 =
                    (uint16)audio->rm_measure.sample_rate_div25;
            result->u.rm_meas.measurement.q.num_samples =
                    (uint16)audio->rm_measure.sample_rate_div25;
            result->u.rm_meas.measurement.q.delta_usec = SECOND / 25;
            result->u.rm_meas.measurement.valid =
                    rate_measure_take_measurement(&audio->rm_measure,
                                                  &result->u.rm_meas.measurement.q,
                                                  &rate_measurement_validity_default,
                                                  time_get_time());
        }
        else
        {
            result->u.rm_meas.measurement.valid = FALSE;
        }
        return TRUE;
    case STREAM_INFO_KEY_AUDIO_SAMPLE_PERIOD_DEVIATION:
        /* Return the last sp_deviation without triggering a new measurement */
        result->u.value = audio->rm_report_sp_deviation;
        return TRUE;
    case STREAM_INFO_KEY_AUDIO_LOCALLY_CLOCKED:
        result->u.value = is_locally_clocked(endpoint);
        return TRUE;
#ifdef INSTALL_DELEGATE_RATE_ADJUST_SUPPORT
    case EP_RATE_ADJUST_OP:
    {
        /* get the rate adjust op from head of sync */
        ENDPOINT *ep = endpoint->state.audio.head_of_sync;
        result->u.value = (uint32)(uintptr_t)ep->state.audio.external_rate_adjust_opid;
        return TRUE;
    }
#endif /* INSTALL_DELEGATE_RATE_ADJUST_SUPPORT */
    case EP_LATENCY_AMOUNT:
        if (audio->use_timed_playback)
        {
            result->u.value =
                    timed_playback_get_latency_amount(audio->timed_playback);
        }
        else
        {
            /* TODO for non-timed-playback operation */
            result->u.value = 0;
        }
        return TRUE;

    default:
        break;
    }
    return FALSE;
}

/**
 * \brief Get the timing requirements of this audio endpoint
 *
 * \param endpoint pointer to the endpoint to get the timing info for
 * \param time_info a pointer to an ENDPOINT_TIMING_INFORMATION structure to
 * populate with the endpoint's timing information
 */
static void audio_get_timing (ENDPOINT *endpoint, ENDPOINT_TIMING_INFORMATION *time_info)
{
#ifdef SUPPORTS_CONTINUOUS_BUFFERING
    SID sid = stream_external_id_from_endpoint(endpoint);
#endif

    time_info->block_size = 0;
    time_info->is_volatile = FALSE;
    time_info->locally_clocked = is_locally_clocked(endpoint);
    /* Effort is only done on this endpoint's interrupt so no kicks are
     * desired from the rest of the chain. */
    time_info->wants_kicks = FALSE;
#ifdef SUPPORTS_CONTINUOUS_BUFFERING
    /* Only in AoV mode we override the setting because we want backwards
     * kicks from the chain to move more data out of the endpoint. */
    if (endpoint->direction == SOURCE)
    {
        time_info->wants_kicks = audio_vsm_get_aov_mode_from_sid(sid);
    }
#endif
    return;
}

/*
 * audio_get_data_format
 */
AUDIO_DATA_FORMAT audio_get_data_format (ENDPOINT *endpoint)
{
    patch_fn_shared(stream_audio_hydra);

    /* Audio always supplies FIXP data to connected endpoints */
    return AUDIO_DATA_FORMAT_FIXP;
}

/**
 * \brief set the audio data format that the endpoint will place in/consume from
 * the buffer
 *
 * \param endpoint pointer to the endpoint to set the data format of.
 * \param format AUDIO_DATA_FORMAT requested to be produced/consumed by the endpoint
 *
 * \return whether the set operation was successful
 */
static bool audio_set_data_format (ENDPOINT *endpoint, AUDIO_DATA_FORMAT format)
{
    AUDIO_DATA_FORMAT hw_format;
    tCbuffer **in_buffs, **out_buffs;
    unsigned nr_chans;
    CBOP_VALS vals;
    bool head_setup = TRUE;
    bool success = FALSE;

    patch_fn_shared(stream_audio_hydra);

    /* The data format can only be set before connect. Also audio only supports
     * FIXP for now. Fail if an attempt is made to set format to any thing else */
    if (NULL != endpoint->connected_to || format != AUDIO_DATA_FORMAT_FIXP)
    {
        return success;
    }

    /* Read the hardware format to determine the amount of shift required to
     * operate with FIXP data */
    hw_format = audio_vsm_get_data_format_from_sid(stream_external_id_from_endpoint(endpoint));
    /* For now there is no conversion we can on top of how the hardware was
     * configured. So if the requested set isn't what we already produce fail. */
    switch (hw_format)
    {
    case AUDIO_DATA_FORMAT_24_BIT:
        endpoint->state.audio.shift = DAWTH - 24;
        break;
    case AUDIO_DATA_FORMAT_16_BIT:
        endpoint->state.audio.shift = DAWTH - 16;
        break;
    case AUDIO_DATA_FORMAT_13_BIT:
        endpoint->state.audio.shift = DAWTH - 13;
        break;
    case AUDIO_DATA_FORMAT_8_BIT:
        endpoint->state.audio.shift = DAWTH - 8;
        break;
    default:
        /* We can't achieve any other format so fail */
        return success;
    }

    if(!cbops_mgr_alloc_buffer_info(endpoint->cbops, &nr_chans, &in_buffs, &out_buffs))
    {
        return success;
    }

    if(nr_chans > 0)
    {
        /* Snapshot cbuffer into at this point in the cbops. It may return fail if number of channels
         * is out of kilter, but we just created things properly so it can't whine.
         */
        cbops_mgr_get_buffer_info(endpoint->cbops, nr_chans, in_buffs, out_buffs);

        /* The head's cbuffer info may have changed since it was last seen in cbops down under...
         * Hook in the buffer info before re-creation of chain happens, for bullet proofing against
         * lifecycle phase-related mishaps.
         * Update in the received buffer table, too that we use later.
         */
        in_buffs[endpoint->state.audio.channel] = endpoint->state.audio.source_buf;
        out_buffs[endpoint->state.audio.channel] =  endpoint->state.audio.sink_buf;

        cbops_mgr_connect_channel(endpoint->cbops, endpoint->state.audio.channel,
                endpoint->state.audio.source_buf, endpoint->state.audio.sink_buf);
    }
    else
    {
        /* Even section head hasn't yet been set up - this is therefore a set format before chain gets properly
         * created. Hence it can't belong to an existing sync group, therefore chain is single channel chain
         * for this endpoint in question.
         */
        nr_chans = 1;
        head_setup = FALSE;
        in_buffs = &endpoint->state.audio.source_buf;
        out_buffs = &endpoint->state.audio.sink_buf;
    }


    /* NOTE: In all subsequent changes to the cbops chain, the Hydra case has to tell the cbops_mgr_remove/append
     * functions that they should change the chain regardless of all buffer information being present. This is because
     * in Hydra case, the set data format can occur after the sync groups have been built up, and before connect() setting
     * up individual channels' buffer information. So at this point in time, we may have no or only some buffer pointers,
     * and we need to avoid common functionality in cbops_mgr taking the (in all other cases correct) decision to not touch
     * the chain itself until all buffer info is provided.
     */

    /* Ensure that what gets "recreated" is having the sync group head's parameters!
     * Of course, on some platforms may not have yet sync'ed, in which case it takes its own params.
     * Add to sync list operation will later anyway set things up with sync head driving the show.
     */
    stream_audio_set_cbops_param_vals(endpoint->state.audio.head_of_sync , &vals);
    vals.rate_adjustment_amount = &(endpoint->state.audio.rm_adjust_amount);

    success = cbops_mgr_append(endpoint->cbops, endpoint->cbops->req_ops , nr_chans, in_buffs, out_buffs, &vals, TRUE);
    /* Initialise rateadjust to passthrough as haven't been asked to rateadjust yet */
    cbops_mgr_rateadjust_passthrough_mode(endpoint->cbops, TRUE);

    if(head_setup)
    {
        cbops_mgr_free_buffer_info(in_buffs, out_buffs);
    }

    return success;
}

/*
 * \brief This is called in response to the endpoint's monitor interrupt,
 * i.e. every monitor\_threshold * T_{sample}
 */
unsigned stream_audio_get_rm_data(ENDPOINT *endpoint)
{
    RATE_TIME curr_time;
    RATE_TIME delta_time;

    patch_fn(stream_audio_hydra_get_rm_data);

    endpoint_audio_state *audio = &endpoint->state.audio;

    if (endpoint->deferred.kick_is_deferred)
    {
        curr_time = endpoint->deferred.interrupt_handled_time;
    }
    else
    {
        curr_time = time_get_time();
    }
    curr_time <<= RATE_TIME_EXTRA_RESOLUTION;
    /* Wrapping subtraction; curr_time is always later than rm_period_start_time */
    delta_time = curr_time - audio->rm_period_start_time;
    audio->rm_period_start_time = curr_time;

    /* Integrate over 128 samples */
    audio->rm_int_time -= audio->rm_int_time >> RM_AVG_SHIFT;
    audio->rm_int_time += delta_time >> RM_AVG_SHIFT;

    /* Hardware sample count currently always equals the monitor threshold,
     * i.e. there are no adjustments.
     */
    return audio->monitor_threshold;
}

/**
 * Process data collected by stream_audio_get_rm_data
 */
void stream_audio_process_rm_data(ENDPOINT *endpoint,
                                  unsigned num_cbops_read,
                                  unsigned num_cbops_written)
{
    endpoint_audio_state* ep_audio = &endpoint->state.audio;

    patch_fn_shared(stream_audio_hydra);
    if (endpoint->connected_to != NULL)
    {
        TIME last_sample_time =
                (TIME)(endpoint->state.audio.rm_period_start_time
                >> RATE_TIME_EXTRA_RESOLUTION);

        if ((endpoint->direction == SOURCE)
            && buff_has_metadata(ep_audio->sink_buf))
        {
            audio_generate_metadata(endpoint, num_cbops_written, last_sample_time);
        }
    }
}

/* Set cbops values */
void stream_audio_set_cbops_param_vals(ENDPOINT* ep, CBOP_VALS *vals)
{
    patch_fn_shared(stream_audio_hydra);

    vals->data_block_size_ptr = &(ep->state.audio.latency_ctrl_info.data_block);

    /* The silence counter is to become the single sync group counter (when synced) */
    vals->total_inserts_ptr = &(ep->state.audio.latency_ctrl_info.silence_samples);

    /* The rm_diff is to become the single sync group rm_diff (when synced) */
    vals->rm_diff_ptr = &(ep->state.audio.rm_diff);

    vals->rate_adjustment_amount = &(ep->state.audio.rm_adjust_amount);

    /* TODO: using silence insertion only for now */
    vals->insertion_vals_ptr = NULL;

    /* Delta samples is not in use on Hydra - ptr to it must be set to NULL */
    vals->delta_samples_ptr = NULL;

    /* Endpoint block size (that equates to the endpoint kick period's data amount).
     * This "arrives" later on, and is owned, possibly updated, by endpoint only.
     */
    vals->block_size_ptr = (unsigned*)&(ep->state.audio.monitor_threshold);

    vals->rm_headroom = AUDIO_RM_HEADROOM_AMOUNT;

    vals->sync_started_ptr = &ep->state.audio.sync_started;
}

/**
 * \brief Generates metadata for audio source endpoints.
 *
 * \param endpoint          Pointer to audio endpoint structure.
 * \param new_words_written Amount of new words written in the buffer.
 * \param last_sample_time  Time of the last sample written in the buffer.
 */
static void audio_generate_metadata(ENDPOINT *endpoint,
                                    unsigned new_words_written,
                                    TIME last_sample_time)
{
    endpoint_audio_state *audio = &endpoint->state.audio;
    metadata_tag *mtag;
    unsigned b4idx, afteridx;

    /* nothing required if the buffer doesn't have metadata enabled,
     * or no new sample has been written into the buffer */
    if(!buff_has_metadata(audio->sink_buf) || new_words_written == 0)
    {
        return;
    }

    patch_fn_shared(stream_audio_hydra);

    /* A metadata tag that we create should cover a minimum amount
     * of samples (min_tag_len). Normally audio samples are copied
     * in chunks larger than that value, so most of the cases one
     * tag is created per each 'new_words_written'. However, if that's
     * less than min_tag_len, a tag with minimum length is created, which
     * means it will cover some samples that will be received later.
     */

    /* if previous tag was incomplete, we need first to complete the tag */
    if(audio->last_tag_left_words > 0)
    {
        /* last written tag was incomplete, we keep adding
         * Null tag until full length of incomplete tag is
         * covered.
         */
        unsigned null_tag_len = audio->last_tag_left_words;
        if(null_tag_len > new_words_written)
        {
            null_tag_len = new_words_written;
        }

        /* append Null tag, with length = null_tag_len */
        b4idx = 0;
        afteridx = null_tag_len*OCTETS_PER_SAMPLE;
        buff_metadata_append(audio->sink_buf, NULL, b4idx, afteridx);

        /* update amount left */
        audio->last_tag_left_words -= null_tag_len;
        new_words_written -= null_tag_len;
        STREAM_METADATA_DBG_MSG2("AUDIO_SOURCE_GENERATE_METADATA, NULL TAG ADDED to complete old written tag, time=0x%08x, tag_len=%d",
                                 time_get_time(), afteridx);
        if(new_words_written == 0)
        {
            /* all new words used for completing old tag */
            return;
        }
    }

    /* create a new tag to append */
    b4idx = 0;
    afteridx = new_words_written*OCTETS_PER_SAMPLE;
    mtag = buff_metadata_new_tag();
    if (mtag != NULL)
    {
        /* Calculating time of arrival, here it is the time that first sample of this tag
         * arrives in the source buffer. We use current time and go back by the duration
         * of total samples ahead of first sample of the tag.
         * Notes:
         *    - Since we assume the last sample in source buffer just arrived, there could
         *      up to 1-sample duration bias.
         *
         *    - if we use HW/SW rate matching, for better accuracy we might want to apply an adjustment
         *      to 'amount_in_buffer' and/or 'new_words_written' below.
         *
         *    - There will be more inaccuracy if cbops inserts silence or trashes input.
         *
         *    None of the above concerns causes accumulation error, and in practice the jitter is negligible.
         */

        TIME time_of_arrival;
        INTERVAL time_passed;
        unsigned amount_in_buffer = cbuffer_calc_amount_data_in_words(audio->source_buf);
#ifdef SUPPORTS_CONTINUOUS_BUFFERING
        if (audio_vsm_get_aov_mode_from_sid(stream_external_id_from_endpoint(endpoint)) == TRUE)
        {
            time_passed = (INTERVAL) (((uint48)new_words_written*SECOND)/audio->sample_rate);
            time_of_arrival = time_add(endpoint->state.audio.previous_toa, time_passed);
            endpoint->state.audio.previous_toa = time_of_arrival;
        }
        else
        {
            time_passed = (INTERVAL) (((uint48)(amount_in_buffer+new_words_written)*SECOND)/audio->sample_rate);
            time_of_arrival = time_sub(last_sample_time, time_passed);
        }
#else
        time_passed = (INTERVAL) (((uint48)(amount_in_buffer+new_words_written)*SECOND)/audio->sample_rate);
        time_of_arrival = time_sub(last_sample_time, time_passed);
#endif /* SUPPORTS_CONTINUOUS_BUFFERING */

        /* Pass on the integrated relative sample period deviation */
        mtag->sp_adjust = get_sp_deviation(endpoint);

        /* see if we have minimum amount for tag */
        if(new_words_written >= audio->min_tag_len)
        {
            /* we have enough new samples to append a complete tag */
            mtag->length = new_words_written*OCTETS_PER_SAMPLE;
        }
        else
        {
            /* new received samples aren't enough to form a
             * new complete tag, we append a new tag with
             * minimum length, this tag is incomplete and
             * will be completed in next calls when we receive
             * new samples by appending Null tags.
             */
            mtag->length = audio->min_tag_len*OCTETS_PER_SAMPLE;
            audio->last_tag_left_words = audio->min_tag_len - new_words_written;
        }

        /* set the time of arrival */
        METADATA_TIME_OF_ARRIVAL_SET(mtag, time_of_arrival);
        STREAM_METADATA_DBG_MSG5("AUDIO_SOURCE_GENERATE_METADATA, NEW TAG ADDED,"
                                 "adjust=0x%08x, toa=0x%08x(%dus in the past), new_words=%d, tag_length=%d",
                                 (unsigned)mtag->sp_adjust,
                                 mtag->timestamp,
                                 time_sub(time_get_time(), mtag->timestamp),
                                 new_words_written,
                                 mtag->length);
    }
    else
    {
        STREAM_METADATA_DBG_MSG1("AUDIO_SOURCE_GENERATE_METADATA, NULL TAG ADDED, time=0x%08x", time_get_time());
    }
    /* append generated metadata to the output buffer */
    buff_metadata_append(audio->sink_buf, mtag, b4idx, afteridx);

}
