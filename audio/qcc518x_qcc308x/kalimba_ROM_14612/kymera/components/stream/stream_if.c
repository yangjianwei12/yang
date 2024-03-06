/****************************************************************************
 * Copyright (c) 2011 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stream_if.c
 * \ingroup stream
 *
 * stream interface file. <br>
 * This file is the interface to the outside world. <br>
 * Whoever that maybe......<br>
 *
 * \section sec1 Contains:
 * stream_if_get_endpoint <br>
 * stream_if_close_endpoint <br>
 * stream_if_configure_sid <br>
 * stream_if_sync_sids <br>
 * stream_if_connect <br>
 * stream_if_transform_connect <br>
 * stream_if_transform_disconnect <br>
 * stream_if_disconnect <br>
 * stream_if_get_source_from_sink <br>
 * stream_if_get_sink_from_source <br>
 *
 *
 */

/****************************************************************************
Include Files
*/

#include "stream_private.h"
#include "opmgr/opmgr.h"
#include "opmgr/opmgr_for_stream.h"

#if defined(SUPPORTS_MULTI_CORE)
#include "stream_kip.h"
#endif

#if defined(INSTALL_UNINTERRUPTABLE_ANC) && defined(INSTALL_ANC_STICKY_ENDPOINTS)
#include "stream_type_alias.h"
#endif /* defined(INSTALL_UNINTERRUPTABLE_ANC) && defined(INSTALL_ANC_STICKY_ENDPOINTS) */

/****************************************************************************
Private Type Declarations
*/

/* Structure for holding creation information for deferred operation */
typedef struct
{
    ENDPOINT *ep;
    CONNECTION_LINK con_id;
    STREAM_ENDPOINT_CBACK callback;
} ep_create_info_struct;

#ifdef INSTALL_MCLK_SUPPORT
/* Structure for holding information for deferred
 * mclk activation operation. The mclk activation
 * will only be deferred if the mclk isn't available
 * at the time of request and has to be claimed first
 * (from curator).
 */
typedef struct
{
    ENDPOINT *ep;
    CONNECTION_LINK con_id;
    STREAM_STATUS_CBACK callback;
} ep_mclk_activate_info_struct;
#endif /* #ifdef INSTALL_MCLK_SUPPORT */


/****************************************************************************
Private Constant Declarations
*/

/****************************************************************************
Private Macro Declarations
*/


/****************************************************************************
Private Variable Definitions
*/

/* Info about a deferred endpoint creation
 * for now assume only one can be in progress at a time
 */
static ep_create_info_struct *ep_create_info = NULL;

#ifdef INSTALL_MCLK_SUPPORT
/* Info about a deferred mclk activation. Only one
 * request can be processed at a time.
 */
static ep_mclk_activate_info_struct *ep_mclk_activate_info = NULL;
#endif
/* Sampling rate at which the streams in the system are
 * configured to work
 */
static uint32 system_stream_rate = STREAM_AUDIO_SAMPLE_RATE_48K;

/* Kick period at which the streams in the system are
 * configured to work
 */
#if defined(INSTALL_MIB)
/* Value will be read from MIB on first use */
#define INITIAL_KICK_PERIOD  0
#else
#define INITIAL_KICK_PERIOD  DEFAULT_KICK_PERIOD_IN_USECS
#endif

static TIME_INTERVAL system_kick_period = (TIME_INTERVAL)INITIAL_KICK_PERIOD;

/****************************************************************************
Private Function Declarations
*/

/****************************************************************************
Public Function Definitions
*/

void stream_if_ep_creation_complete(ENDPOINT *ep, bool success)
{
    CONNECTION_LINK reversed_con_id;

    if ((ep_create_info == NULL) ||
        (ep_create_info->ep != ep))
    {
        /* This isn't the endpoint you're looking for... */
        panic_diatribe(PANIC_AUDIO_ENDPOINT_CREATION_SEQUENCE_ERROR,
                       (DIATRIBE_TYPE)((uintptr_t)ep));
    }

    reversed_con_id = REVERSE_CONNECTION_ID(ep_create_info->con_id);
    if (!success)
    {
        stream_close_endpoint(ep);
        ep_create_info->callback(reversed_con_id,
                                 STATUS_CMD_FAILED,
                                 0);

    }
    else
    {
        unsigned epid;

        epid = stream_external_id_from_endpoint(ep);
        ep_create_info->callback(reversed_con_id,
                                 STATUS_OK,
                                 epid);
    }

    pdelete(ep_create_info);
    ep_create_info = NULL;
}

void stream_if_get_endpoint(CONNECTION_LINK con_id,
                            STREAM_DEVICE device,
                            unsigned num_params,
                            unsigned *params,
                            ENDPOINT_DIRECTION dir,
                            STREAM_ENDPOINT_CBACK callback)
{
    CONNECTION_LINK reversed_con_id;
    STATUS_KYMERA status;
    ENDPOINT *ep;
    bool pending;
    unsigned ext_epid;

    patch_fn_shared(stream_if);

    reversed_con_id = REVERSE_CONNECTION_ID(con_id);
    status = STATUS_CMD_FAILED;
    ep = NULL;
    ext_epid = 0;
    pending = FALSE;

    /* TODO MULTICORE:
       If P0 Stream imposes EP ID, on P1 receiving an extra param with EP ID.
       Pass that EP ID down as extra param to the new endpoint creation macros
       (and the function underneath them).
       If on P0, it would not get this extra param - but if client can impose
       where to create EP, then adaptor may fish out extra param with processor
       ID - and by the time we are here, con_id will say that it is to be
       'remote' EP. */

    /* Right then this is the first interface that is going to get called. */
    switch (device)
    {
        case STREAM_DEVICE_PCM:
        case STREAM_DEVICE_I2S:
        case STREAM_DEVICE_APPDATA:
#ifdef INSTALL_CODEC
        case STREAM_DEVICE_CODEC:
#endif
#ifdef INSTALL_DIGITAL_MIC
        case STREAM_DEVICE_DIGITAL_MIC:
#endif
#ifdef INSTALL_AUDIO_INTERFACE_PWM
        case STREAM_DEVICE_PWM:
#endif
#ifdef INSTALL_MONITOR_OUT
        case STREAM_DEVICE_MON_OUT:
#endif
        {
            ep = stream_audio_get_endpoint(con_id,
                                           dir,
                                           device,
                                           num_params,
                                           params,
                                           &pending);
            break;
        }
#if defined(INSTALL_SPDIF) || defined(INSTALL_AUDIO_INTERFACE_SPDIF)
        case STREAM_DEVICE_SPDIF:
        {
            if (SINK == dir)
            {
                /* For S/PDIF tx always use generic audio endpoint type */
                ep = stream_audio_get_endpoint(con_id,
                                               dir,
                                               device,
                                               num_params,
                                               params,
                                               &pending);
            }
            else
            {
                ep = stream_spdif_get_endpoint(con_id,
                                               dir,
                                               num_params,
                                               params,
                                               &pending);
            }
            break;
        }
#endif /* #if defined(INSTALL_SPDIF) || defined(INSTALL_AUDIO_INTERFACE_SPDIF) */
        case STREAM_DEVICE_OPERATOR:
        {
            EXT_OP_ID ext_op_id;
            CONNECTION_LINK creator_link;

            if (num_params != 2)
            {
                status = STATUS_INVALID_CMD_LENGTH;
                break;
            }

            ext_op_id = (EXT_OP_ID) params[0];
            creator_link = opmgr_con_id_from_opid(ext_op_id);
            if (GET_CON_ID_SEND_ID(creator_link) != GET_CON_ID_SEND_ID(con_id))
            {
                /* Either the operator does not exist or it is owned by
                   another subsystem than the one that made this request.
                   This prevents the application subsystem to mess with the
                   graphs from the Bluetooth subsystem. */
                break;
            }

            /* Unlike other endpoints, the endpoint structure for operators
               is allocated during connection instead of creation. So pass
               the external endpoint id to the rest of the function. */
            ext_epid = stream_operator_get_endpoint_id(ext_op_id,
                                                       params[1],
                                                       dir);
            status = (ext_epid != 0) ? STATUS_OK : status;
            break;
        }
        case STREAM_DEVICE_SCO:
        {
            ep = stream_sco_get_endpoint(con_id, dir, num_params, params);
#if defined(INSTALL_ISO_CHANNELS) && defined(TODO_STREAM_DEVICE_SCO_SUPPORTS_ISO)
            /* FIXME: In early development of isochronous channels, we
             * provided access to their endpoints via STREAM_DEVICE_SCO
             * to avoid the need for inventing new Apps traps, etc.
             * (There is no ambiguity in the request, as the
             * HCI Connection_Handle number space covers both SCO and ISO
             * handles.)
             * We've decided that the production interface will instead be
             * STREAM_DEVICE_ISO (see below), but to ease transition we're
             * also continuing to support access via STREAM_DEVICE_SCO.
             * This is intended to be removed before production.
             */
            if (ep == NULL)
            {
                /* The specified handle didn't correspond to any SCO endpoints.
                 * Check for ISO endpoints. */
                ep = stream_iso_get_endpoint(con_id, dir, num_params, params);
            }
#endif /* INSTALL_ISO_CHANNELS && TODO_STREAM_DEVICE_SCO_SUPPORTS_ISO */
            break;
        }
#ifdef INSTALL_ISO_CHANNELS
        case STREAM_DEVICE_ISO:
        {
            ep = stream_iso_get_endpoint(con_id, dir, num_params, params);
            break;
        }
#endif /* INSTALL_ISO_CHANNELS */
        case STREAM_DEVICE_RAW_BUFFER:
        {
            ep = stream_raw_buffer_get_endpoint(con_id, params[0]);
            break;
        }
        case STREAM_DEVICE_L2CAP:
        {
            /* Kymera assumes that any L2CAP connection has A2DP traffic
             * (since it has no information to go on), and wraps it in an
             * 'a2dp' endpoint, which for instance triggers A2DP-style
             * rate-matching for the sink.
             * This wouldn't necessarily be the right thing for non-A2DP-like
             * uses of L2CAP, but so far we haven't had any of those.
             * FIXME: some way for applications to override this */
            ep = stream_a2dp_get_endpoint(con_id, dir, num_params, params);
            break;
        }
        case STREAM_DEVICE_SHUNT:
        {
            ep = stream_shunt_get_endpoint(con_id, dir, num_params, params);
            break;
        }
        default:
        {
            /* If a platform has no support for a specific device type and
               yet this function is called by accident then ep will be NULL
               and the callback will report the error. */
            break;
        }
    }

    if (ep == NULL)
    {
        /* This branch is taken either creation of an endpoint failed or in the
           case of an operator endpoint. In this last case, while the operator
           already exists, the endpoint structure for its terminal has not
           been created yet and will not be created until it is connected.
           Connecting the terminal of an operator needs the value returned by
           this function in ext_epid. */
        callback(reversed_con_id, status, ext_epid);
        return;
    }

    set_system_event(SYS_EVENT_REAL_EP_CREATE);
    /* Check that the endpoint is tagged with this user - only look at
       "owner", i.e. what is sender ID in the connection ID. If matches,
       send a response and make sure connection ID is reversed. */
    if (GET_CON_ID_SEND_ID(ep->con_id) != GET_CON_ID_SEND_ID(con_id))
    {
        callback(reversed_con_id, status, 0);
        return;
    }

    /* Some endpoints need some time after allocation before being
       usable. This is usually due to the need to exchange some
       information to another subsystem or to acquire resources such
       as clocks. */
    if (!pending)
    {
        unsigned id;

        id = stream_external_id_from_endpoint(ep);
        callback(reversed_con_id, STATUS_OK, id);
        return;
    }

    /* The endpoint returned a pending state, so we have to now
       wait for it to call the completion callback before
       signalling to the client. */
    if (ep_create_info == NULL)
    {
        /* Allocate some data for the deferred-creation context
           We'll panic if this fails, but it's a small structure
           and the endpoint is going to call the callback anyway,
           so we can't do without it... */
        ep_create_info = pnew(ep_create_info_struct);
        ep_create_info->callback = callback;
        ep_create_info->con_id = con_id;
        ep_create_info->ep = ep;
        return;
    }

    /* This point should never be reached as there should be only
       one pending request in flight at a given time. */
    panic_diatribe(PANIC_AUDIO_ENDPOINT_CREATION_SEQUENCE_ERROR,
                   (DIATRIBE_TYPE)((uintptr_t)(ep_create_info->ep)));
}

/****************************************************************************
 *
 * stream_if_close_endpoint
 *
 */
void stream_if_close_endpoint(CONNECTION_LINK con_id,
                              unsigned endpoint_id,
                              STREAM_STATUS_CBACK callback)
{
    CONNECTION_LINK reversed_con_id;
    STATUS_KYMERA status;
    ENDPOINT *endpoint;

    patch_fn_shared(stream_if);

    status = STATUS_CMD_FAILED;
    reversed_con_id = REVERSE_CONNECTION_ID(con_id);
    endpoint = stream_endpoint_from_extern_id(endpoint_id);
    if (endpoint == NULL)
    {
        callback(reversed_con_id, status);
        return;
    }

#if defined(INSTALL_UNINTERRUPTABLE_ANC) && defined(INSTALL_ANC_STICKY_ENDPOINTS)
    STREAM_ANC_INSTANCE instance_id;

    /* Check if the endpoint is being used by ANC
     * if it is then don't close it but return a successful status
     */
    instance_id = stream_audio_anc_get_primary_instance_id(endpoint);
    if (instance_id != STREAM_ANC_INSTANCE_NONE_ID)
    {
        /* ANC is using the endpoint so don't actually do the close */
        status = STATUS_OK;

#if defined(HAVE_ANC_HARDWARE)
        audio_channel channel;

        /* Get the channel to be closed */
        channel = (audio_channel) get_hardware_channel(endpoint);

        /* Shutdown H/W that isn't needed for ANC standalone
         * i.e. disable decimator clocks for sources e.g. ADC/DMIC
         * and disable interpolators for sinks e.g. DAC
         */
        if (endpoint->direction == SOURCE)
        {
            audio_instance instance;

            /* Get the instance and channel to be closed */
            instance = (audio_instance) get_hardware_instance(endpoint);

            /* Disable the decimation chain clocks */
            audio_hwm_anc_disable_decimator(instance, channel);
        }
        else
        {
            /* Disable the interpolation chain. Also, when closing a sink endpoint shared
             * with ANC decouple the DSM and explicitly enable the DSM.
             */
            audio_hwm_anc_disable_interpolator(channel);
        }
#endif /* defined(HAVE_ANC_HARDWARE) */

        /* Indicate that ANC must close the endpoint when tearing down ANC */
        stream_audio_anc_set_close_pending(endpoint, TRUE);
    }
    else
#endif /* defined(INSTALL_UNINTERRUPTABLE_ANC) && defined(INSTALL_ANC_STICKY_ENDPOINTS) */
    {
        /* TODO MULTICORE: based on con_id, act on P0 for local endpoint,
         * or delegate to secondary processor.
         */
        if (stream_close_endpoint(endpoint))
        {
            status = STATUS_OK;
        }
    }

    callback(reversed_con_id, status);
}

/****************************************************************************
 *
 * stream_if_configure_sid
 *
 */
void stream_if_configure_sid(CONNECTION_LINK con_id,
                             unsigned ep_id,
                             unsigned int key,
                             uint32 value,
                             STREAM_STATUS_CBACK callback)
{
    STATUS_KYMERA status = STATUS_CMD_FAILED;

    /* TODO MULTICORE: based on con_id, act on P0 for local endpoint,
     * or delegate to secondary processor.
     */
    ENDPOINT *ep = stream_endpoint_from_extern_id(ep_id);

    if (ep ? ep->functions->configure(ep, key, value) : FALSE)
    {
        status = STATUS_OK;
    }

    callback(REVERSE_CONNECTION_ID(con_id), status);
}

#ifdef INSTALL_MCLK_SUPPORT
/****************************************************************************
 *
 * stream_if_mclk_activate
 *
 */
void stream_if_mclk_activate(CONNECTION_LINK con_id,
                             unsigned ep_id,
                             unsigned activate_output,
                             unsigned enable_mclk,
                             STREAM_STATUS_CBACK callback)
{
    STATUS_KYMERA status = STATUS_CMD_FAILED;
    ENDPOINT *ep = stream_endpoint_from_extern_id(ep_id);
    patch_fn_shared(stream_if);

    if(ep && IS_AUDIO_ENDPOINT(ep))
    {
        bool pending = FALSE;
        if(stream_audio_activate_mclk(ep, activate_output, enable_mclk, &pending))
        {
            if(pending)
            {
                if (ep_mclk_activate_info != NULL)
                {
                    /* Should only have one in progress...? */
                    panic_diatribe(PANIC_AUDIO_MCLK_ACTIVATION_SEQUENCE_ERROR, (DIATRIBE_TYPE)((uintptr_t)ep));
                }
                else
                {
                    /* Allocate some data for the deferred-mclk_enabling context
                     * We'll panic if this fails, but it's a small structure
                     * and the endpoint is going to call the callback anyway,
                     * so we can't do without it...
                     */
                    ep_mclk_activate_info = pnew(ep_mclk_activate_info_struct);
                    ep_mclk_activate_info->callback = callback;
                    ep_mclk_activate_info->con_id = con_id;
                    ep_mclk_activate_info->ep = ep;
                }
                return;
            }
            else
            {
                status = STATUS_OK;
            }
        }
    }
    callback(REVERSE_CONNECTION_ID(con_id), status);
}

/****************************************************************************
 *
 * stream_if_ep_mclk_activate_complete
 *
 */
void stream_if_ep_mclk_activate_complete(ENDPOINT *ep, bool success)
{
    if ((ep_mclk_activate_info == NULL) ||
        (ep_mclk_activate_info->ep != ep))
    {
        /* This isn't the endpoint you're looking for... */
        panic_diatribe(PANIC_AUDIO_MCLK_ACTIVATION_SEQUENCE_ERROR, (DIATRIBE_TYPE)((uintptr_t)ep));
    }
    else if (!success)
    {
        /* We couldn't activate mclk. Send back failure */
        ep_mclk_activate_info->callback(REVERSE_CONNECTION_ID(ep_mclk_activate_info->con_id),
                STATUS_CMD_FAILED);
        /* TODO:any further action here? */
    }
    else
    {
        /* activation successful, report success */
        ep_mclk_activate_info->callback(REVERSE_CONNECTION_ID(ep_mclk_activate_info->con_id),
                                 STATUS_OK);
    }
    pdelete(ep_mclk_activate_info);
    ep_mclk_activate_info = NULL;
}

/****************************************************************************
 *
 * stream_if_set_mclk_source
 *
 */
void stream_if_set_mclk_source(CONNECTION_LINK con_id,
                               unsigned use_external_mclk,
                               uint32 external_mclk_freq,
                               STREAM_STATUS_CBACK callback)
{
    STATUS_KYMERA status = STATUS_OK;

    /* tell the mclk manager to use external mclk or local mpll
     * when using external mclk, it's frequency also needs to be supplied
     */
    if(!audio_mclk_mgr_use_external_mclk(use_external_mclk, external_mclk_freq))
    {
        /* for some reason it couldn't switch to new mclk source
         * this might be for reason like mclk is in-use currently
         */
        status = STATUS_CMD_FAILED;
    }
    callback(REVERSE_CONNECTION_ID(con_id), status);
}

#endif /* #ifdef INSTALL_MCLK_SUPPORT */

/****************************************************************************
 *
 * stream_if_sync_sids
 *
 */
void stream_if_sync_sids(CONNECTION_LINK con_id,
                         unsigned ep_id1,
                         unsigned ep_id2,
                         STREAM_STATUS_CBACK callback)
{
    STATUS_KYMERA status = STATUS_OK;
    ENDPOINT *ep1 = stream_endpoint_from_extern_id(ep_id1);
    ENDPOINT *ep2 = stream_endpoint_from_extern_id(ep_id2);
    patch_fn_shared(stream_if);

    /* TODO MULTICORE: based on con_id, act on P0 for local endpoint,
     * or delegate to secondary processor. Only EPs local to a processor
     * can be sync'ed.
     */

    /* Endpoint used for rate matching comparison*/
    ENDPOINT *r_ep = NULL;

    if(ep1 == NULL)
    {
        if ( (ep2 != NULL) && IS_AUDIO_ENDPOINT(ep2))
        {
            /* Swap ep1 with ep2. Note: ep1 is null */
            ep1 = ep2;
            ep2 = NULL;
        }
        else
        {
            /* If both of them are null or ep2 is not an audio endpoint
             * then return command fail. */
            status = STATUS_CMD_FAILED;
        }
    }
    else if (IS_AUDIO_ENDPOINT(ep1))
    {
        if (ep2 && !IS_AUDIO_ENDPOINT(ep2))
        {
            /* ep2 is not audio return command fail. */
            status = STATUS_CMD_FAILED;
        }
    }
    else
    {
        /* ep1 is not audio return command fail. */
        status = STATUS_CMD_FAILED;
    }

    if(ep2 == NULL)
    {
        /* If ep2 is null we are going to desynchronise so we need to cache
         * the other endpoint in the pair so we can rebuild rate matching */
        if(IS_ENDPOINT_HEAD_OF_SYNC(ep1))
        {
            /* If the endpoint is the head of sync we want to cache the nep
             * in the temporary endpoint for when we rebuild rate matching */
            r_ep = ep1->state.audio.nep_in_sync;
        }
        else
        {
            /* We want to cache the head ep to rebuild with rate matching*/
            r_ep = ep1->state.audio.head_of_sync;
        }
    }
    else
    {
        r_ep = ep2;
    }
#ifdef INSTALL_SPDIF
    /* allow two spidf input streams
     * to get syncronised, this is releavant only
     * in two-channel config
     */
    if(STATUS_OK!=status
       && (ep1 != NULL) && IS_SPDIF_ENDPOINT(ep1)
       && (ep2 != NULL) && IS_SPDIF_ENDPOINT(ep2))
    {
        status = STATUS_OK;
    }
#endif /* #ifdef INSTALL_SPDIF */

    /* If status is still OK then synchronise the endpoints. (ep1 is never null)
     * If the synchronisation failed return command fail. */
    if(status ==STATUS_OK)
    {
        if(!ep1->functions->sync(ep1, ep2))
        {
            status = STATUS_CMD_FAILED;
        }
        else
        {
            /* When use is made of the sync-group information available in Bluecore
             * at the point the endpoint is created. There will no longer be a
             * need to fiddle with ratematching during synchronisation.
             */
            if (r_ep != NULL)
            {
                /* First we remove any pairs that involve either endpoint*/
                cease_ratematching(ep1->id);
                cease_ratematching(r_ep->id);

                /* Then rebuild. The rate matching code should recognise the difference
                * that we either have a single or split rate matching entity.  */
                if (!setup_ratematching(ep1->id))
                {
                    status = STATUS_CMD_FAILED;
                }
                if (!setup_ratematching(r_ep->id))
                {
                    status = STATUS_CMD_FAILED;
                }
            }
        }
    }

    callback(REVERSE_CONNECTION_ID(con_id), status);
}


/* Processor independent stream_connect request. state_info must be NULL
   if the operator endpoints are not already created. For single core
   stream connection, state_info must be NULL. */
void stream_if_transform_connect(CONNECTION_LINK con_id,
                                 unsigned source_id,
                                 unsigned sink_id,
                                 TRANSFORM_ID transform_id,
                                 STREAM_CONNECT_INFO *state_info,
                                 STREAM_TRANSFORM_CBACK callback)
{
    TRANSFORM_ID transform;
    bool success;
    STREAM_CONNECT_INFO info;
    ENDPOINT *source_ep;
    ENDPOINT *sink_ep;

    patch_fn_shared(stream_if);

    /*
     * Whenever this function gets called, get the connect information and
     * decide the starting state of the connect process. If sink and source
     * endpoints are with  either P0 and/or P1, all the states will be executed.
     *
     * Connect can start in stage 3 or in stage 1 depending upon the operator
     * locations.
     *
     * 1. Creating the endpoints if they don't exist
     * 2. Negotiating the buffer details and data format
     * 3. Establishing the transform and updating the kick information
     */

    source_ep = stream_endpoint_from_extern_id(source_id);
    if (source_ep == NULL)
    {
        source_ep = stream_create_endpoint(source_id, con_id);
    }

    sink_ep = stream_endpoint_from_extern_id(sink_id);
    if (sink_ep == NULL)
    {
        sink_ep = stream_create_endpoint(sink_id, con_id);
    }

    success = TRUE;
    if (state_info == NULL)
    {
        success = stream_connect_get_buffer(source_ep, sink_ep, &info);
        state_info = &info;
    }

    /* stage 3 - Establish transform
     * On P0 (as well as on a single core), this transform id will be always 0.
     * On P1, the transform id must not be 0. This must be validated before
     * coming here.
     */
    transform = 0;
    if (success)
    {
        TRANSFORM_INT_ID id;
        TRANSFORM *tfm;

        id = STREAM_TRANSFORM_GET_INT_ID(transform_id);
        tfm = stream_connect_endpoints(source_ep, sink_ep, state_info, id);
        transform = stream_external_id_from_transform(tfm);
        if (transform == 0)
        {
            success = FALSE;
        }
    }

    if (!success)
    {
#ifndef CRESCENDO_TODO_DUAL_CORE
        /* TODO - For dual core, we need to deactivate the data channel */
#endif
        stream_destroy_endpoint_id(source_id);
        stream_destroy_endpoint_id(sink_id);
    }

    callback(REVERSE_CONNECTION_ID(con_id),
             success ? STATUS_OK : STATUS_CMD_FAILED,
             transform);
}

/* This API is being called only in P0. This is not expected to be called on Pn. */
void stream_if_connect(CONNECTION_LINK con_id,
                       unsigned source_id,
                       unsigned sink_id,
                       STREAM_TRANSFORM_CBACK callback)
{
    patch_fn_shared(stream_if);

#if defined(SUPPORTS_MULTI_CORE)
    CONNECTION_LINK reversed_con_id;
    STREAM_EP_LOCATION ep_location;
    PROC_ID_NUM remote_processor_id;

    reversed_con_id = REVERSE_CONNECTION_ID(con_id);
    ep_location = STREAM_EP_REMOTE_NONE;
    remote_processor_id = PROC_PROCESSOR_0;

    /*
     * If dual core support is enabled, the sink and/or the source may be located
     * on a remote processor other than P0.
     *
     *  - If both sink and source are operator eps and not on P0:
     *          - Generate a transform id for remote use
     *          - Send kip_stream_connect_request_req to Pn
     *          - Store the context and call back
     *
     *  - else If either sink or source is an operator ep on Pn
     *          - If other ep is not an audio real endpoint when delegating
     *            audio endpoint is supported.
     *              - Derive the shadow endpoint id for remote and local
     *              - if source is local, create a data channel
     *              - create local endpoints & get the endpoint details(buffer)
     *           - else ( delegating audio endpoint is supported)
     *              - get the real audio endpoint details
     *
     *           - Send kip_create_endpoint_req() to the remote
     *           - Store the context and callback
     *
     *   - else (everything on P0)
     *          - Create local endpoints and get the endpoint details (buffer)
     *          - Call stream_if_transform_connect() and proceed with local connection.
     */

    /* Figure out whether the provided source endpoint is not a local one */
    if (STREAM_EP_IS_OPEP_ID(source_id))
    {
        PROC_ID_NUM proc_id;

        /*
         * There must be an operator associated with this id, if it is not,
         * it will return failure while attempting to create the endpoints.
         */
        proc_id = opmgr_get_processor_id(source_id);
        if ((proc_id != PROC_PROCESSOR_INVALID) &&
            (!PROC_ON_SAME_CORE(proc_id)))
        {
            remote_processor_id = proc_id;
            ep_location |= STREAM_EP_REMOTE_SOURCE;
        }
    }

    /* figure out whether the provided sink endpoint is not a local one */
    if (STREAM_EP_IS_OPEP_ID(sink_id))
    {
        PROC_ID_NUM proc_id;

        /*
         * There must be an operator associated with this id, if it is not,
         * it will return failure while attempting to create the endpoints.
         */
        proc_id = opmgr_get_processor_id(sink_id);
        if ((proc_id != PROC_PROCESSOR_INVALID) &&
            (!PROC_ON_SAME_CORE(proc_id)))
        {
            if (remote_processor_id == PROC_PROCESSOR_0)
            {
                remote_processor_id = proc_id;
            }
            else if (remote_processor_id != proc_id)
            {
                /* Connecting operators running on two different
                 * secondary cores are not supported. For
                 * dual core, it should not happen!
                 */
                callback(reversed_con_id, STATUS_CMD_FAILED, 0);
                return;
            }

            /* sink is also with secondary core */
            ep_location |= STREAM_EP_REMOTE_SINK;
        }
    }

    /*
     * If none of the operator endpoints are located in Pn,
     * just proceed with the single core scenario
     */
    if (ep_location != STREAM_EP_REMOTE_NONE)
    {
        /* create a state information */
        STREAM_KIP_CONNECT_INFO *state_info;
        unsigned remote_source_id = source_id;
        unsigned remote_sink_id = sink_id;
        CONNECTION_LINK con_proc_id;
        bool success;
        bool sync_shadow_eps;

        /* This is a flag that will be used to determine if the internally
         * created & managed shadow EPs are to be synchronised with other
         * similar shadow EPs (currently, this means "shadow EPs using the same
         * port"). By default, all shadow EPs are assumed to be synchronised,
         * hence initialised to TRUE. */
        sync_shadow_eps = TRUE;

        /* Generate the KIP shadow endpoint ids
         * If the source operator endpoint location is remote, then remote sink
         * will be the shadow of local sink and local source will be the shadow
         * of remote source.
         */
        if (ep_location == STREAM_EP_REMOTE_SOURCE)
        {
            source_id = STREAM_GET_SHADOW_EP_ID(source_id);
            remote_sink_id = STREAM_GET_SHADOW_EP_ID(sink_id);

            ENDPOINT *sink_ep;
            sink_ep = stream_endpoint_from_extern_id(sink_id);

            /* Check if the EP on P0 is an audio EP. If it is, check if it
             * is synchronised in order to update the sync_shadow_eps flag
             * accordingly.
             * Audio EPs should have been created by now. */
            if (sink_ep != NULL)
            {
                if (IS_AUDIO_ENDPOINT(sink_ep))
                {
                    if (IS_AUDIO_ENDPOINT_SYNCED(sink_ep) == FALSE)
                    {
                        sync_shadow_eps = FALSE;
                    }
                }
            }
        }
        else if (ep_location == STREAM_EP_REMOTE_SINK)
        {
            sink_id = STREAM_GET_SHADOW_EP_ID(sink_id);
            remote_source_id = STREAM_GET_SHADOW_EP_ID(source_id);

            ENDPOINT *source_ep;
            source_ep = stream_endpoint_from_extern_id(source_id);

            /* Check if the EP on P0 is an audio EP. If it is, check if it
             * is synchronised in order to update the sync_shadow_eps flag
             * accordingly.
             * Audio EPs should have been created by now. */
            if (source_ep != NULL)
            {
                if (IS_AUDIO_ENDPOINT(source_ep))
                {
                    if (IS_AUDIO_ENDPOINT_SYNCED(source_ep) == FALSE)
                    {
                        sync_shadow_eps = FALSE;
                    }
                 }
            }
        }

        /* pack con id and processor id for kip transactions */
        con_proc_id =  PACK_CONID_PROCID(con_id, remote_processor_id);

        /* create the connect state record only for connection */
        /* The data format is updated in stream_kip_create_endpoints().*/
        state_info = stream_kip_create_connect_info_record(con_proc_id,
                                                           source_id,
                                                           sink_id,
                                                           ep_location,
                                                           callback,
                                                           sync_shadow_eps);

        if (state_info == NULL)
        {
            callback(reversed_con_id, STATUS_CMD_FAILED, 0);
            return;
        }

        if (ep_location == STREAM_EP_REMOTE_ALL)
        {

            success = stream_kip_connect_endpoints(con_proc_id,
                                                   remote_source_id,
                                                   remote_sink_id,
                                                   state_info);
        }
        else
        {
            /* Atleast one operator endpoint is in Pn. Create local
             * endpoints and remote endpoints.
             * buffer details must be presented to the remote.
             */
            success = stream_kip_create_endpoints(con_proc_id,
                                                  remote_source_id,
                                                  remote_sink_id,
                                                  state_info);
        }

        if (!success)
        {
            pdelete(state_info);
            callback(reversed_con_id, STATUS_CMD_FAILED, 0);
        }

        /* Done with dual core scenario */
        return;
    }
#endif /* defined(SUPPORTS_MULTI_CORE) */

    /* Comes here only when stream connect happens only at P0 */
    stream_if_transform_connect(con_id, source_id, sink_id, 0, NULL, callback);
}

/* This is the common function for P0 and P1 to disconnect the transform.
   This provides a count of already disconnected transforms for reporting
   in the callback.
   stream_if_transform_disconnect is the P0 only interface function
   being called by ACCMD. */
void stream_if_part_transform_disconnect(CONNECTION_LINK con_id,
                                         unsigned count,
                                         TRANSFORM_ID *transforms,
                                         unsigned success_count,
                                         STREAM_COUNT_CBACK callback)
{
    unsigned i;
    TRANSFORM *tfm;
    STATUS_KYMERA status;

    patch_fn_shared(stream_if);

    status = STATUS_OK;
    for (i = 0; i < count; i++)
    {
        /* Get the endpoints before the disconnect. If they are operator
         * endpoints they need to be destroyed here. */
        tfm = stream_transform_from_external_id(transforms[i]);

        if (tfm == NULL)
        {
            status = STATUS_CMD_FAILED;
            break;
        }

        if (!stream_transform_disconnect(tfm))
        {
            status = STATUS_CMD_FAILED;
            break;
        }
    }

    callback(REVERSE_CONNECTION_ID(con_id), status, i + success_count);
}

/* ACCMD calls this API. If dual core enabled, it filters P1 list first
   before calling stream_if_local_transform_disconnect. */
void stream_if_transform_disconnect(CONNECTION_LINK con_id,
                                    unsigned count,
                                    TRANSFORM_ID *transforms,
                                    STREAM_COUNT_CBACK callback)
{
    unsigned px_tr_count;

    patch_fn_shared(stream_if);

    /* Find the first P1 transform in the list.
       If there are no P1 transforms in the list, px_tr_count returns count. */
    px_tr_count = stream_kip_find_px_transform_start(count, transforms);
    if (px_tr_count < count)
    {
        if (!stream_kip_part_transform_disconnect(con_id,
                                                  count,
                                                  px_tr_count,
                                                  transforms,
                                                  callback))
        {
            callback(REVERSE_CONNECTION_ID(con_id), STATUS_CMD_FAILED, 0);
        }

        return;
    }

    /* It came here only because there is no P1 transforms in the list */
    stream_if_part_transform_disconnect(con_id, count, transforms, 0, callback);
}

void stream_if_disconnect(CONNECTION_LINK con_id,
                          unsigned source_id,
                          unsigned sink_id,
                          STREAM_TRANSFORM_PAIR_CBACK callback)
{
    CONNECTION_LINK reversed_con_id;
    STATUS_KYMERA status;
    unsigned tfid_sink;
    unsigned tfid_source;

    patch_fn_shared(stream_if);

    reversed_con_id = REVERSE_CONNECTION_ID(con_id);

    /* This API must be called only at P0 */
    if (PROC_SECONDARY_CONTEXT())
    {
        callback(reversed_con_id, STATUS_INVALID_CMD_PARAMS, 0, 0);
        return;
    }

    if ((sink_id == 0) && (source_id == 0))
    {
        callback(reversed_con_id, STATUS_INVALID_CMD_PARAMS, 0, 0);
        return;
    }

    if (PROC_PRIMARY_CONTEXT())
    {
        status = stream_kip_disconnect(con_id,
                                       source_id,
                                       sink_id,
                                       callback);
        if (status == STATUS_CMD_FAILED)
        {
            callback(reversed_con_id, STATUS_CMD_FAILED, 0, 0);
            return;
        }
        else if (status == STATUS_CMD_PENDING)
        {
            /* The callback will come later. */
            return;
        }
    }

    /* No second core transforms involved. Just continue handling at
     * single core
     */
    tfid_sink = 0;
    tfid_source = 0;
    status = STATUS_OK;
    if (sink_id != 0)
    {
        ENDPOINT *sink;

        sink = stream_endpoint_from_extern_id(sink_id);
        if (sink == NULL)
        {
            status = STATUS_CMD_FAILED;
        }
        else if (sink->connected_to != NULL)
        {
            /* If the sink_id exists and is connected to *something* ... */
            TRANSFORM *tfm;

            tfm = stream_transform_from_endpoint(sink);
            tfid_sink = stream_external_id_from_transform(tfm);

            /* If the sink is connect to 'our' source... */
            if (sink->connected_to == stream_endpoint_from_extern_id(source_id))
            {
                tfid_source = tfid_sink;
            }

            if (!stream_transform_disconnect(tfm))
            {
                status = STATUS_CMD_FAILED;
            }
        }
        /* Valid sink that is not connected is not a fault */
    }

    if ((source_id != 0) && (tfid_source == 0))
    {
        ENDPOINT *source;

        source = stream_endpoint_from_extern_id(source_id);
        if (source == NULL)
        {
            status = STATUS_CMD_FAILED;
        }
        else if (source->connected_to != NULL)
        {
            /* If the source_id exists and is connected to 'other' sink... */
            TRANSFORM *tfm;

            /* Source id is not connected sink id, we handled that earlier on... */
            tfm = stream_transform_from_endpoint(source);
            tfid_source = stream_external_id_from_transform(tfm);

            if (!stream_transform_disconnect(tfm))
            {
                status = STATUS_CMD_FAILED;
            }
        }
        /* Valid source that is not connected is not a fault */
    }

    callback(reversed_con_id, status, tfid_source, tfid_sink);
}

void stream_if_get_sink_from_source(CONNECTION_LINK con_id,
                                    unsigned source_id,
                                    STREAM_ENDPOINT_ID_CBACK callback)
{
    unsigned sink_id;
    STATUS_KYMERA status;
    ENDPOINT *sink_ep;
    ENDPOINT *source_ep;

    patch_fn_shared(stream_if);

    sink_id = 0;
    source_ep = stream_endpoint_from_extern_id(source_id);
    if (source_ep == NULL)
    {
        status = STATUS_CMD_FAILED;
    }
    else
    {
        sink_ep = stream_get_endpoint_from_key_and_functions(source_ep->key,
                                                             SINK,
                                                             source_ep->functions);
        if (sink_ep != NULL)
        {
            sink_id = stream_external_id_from_endpoint(sink_ep);
            status = STATUS_OK;
        }
        else
        {
            status = STATUS_CMD_FAILED;
        }
    }

    callback(REVERSE_CONNECTION_ID(con_id), status, sink_id);
    return;
}

void stream_if_get_source_from_sink(CONNECTION_LINK con_id,
                                    unsigned sink_id,
                                    STREAM_ENDPOINT_ID_CBACK callback)
{
    unsigned source_id;
    STATUS_KYMERA status;
    ENDPOINT *source_ep;
    ENDPOINT *sink_ep;

    patch_fn_shared(stream_if);

    source_id = 0;
    sink_ep = stream_endpoint_from_extern_id(sink_id);
    if (sink_ep == NULL)
    {
        status = STATUS_CMD_FAILED;
    }
    else
    {
        source_ep = stream_get_endpoint_from_key_and_functions(sink_ep->key,
                                                               SOURCE,
                                                               sink_ep->functions);
        if (source_ep != NULL)
        {
            source_id = stream_external_id_from_endpoint(source_ep);
            status = STATUS_OK;
        }
        else
        {
            status = STATUS_CMD_FAILED;
        }
    }

    callback(REVERSE_CONNECTION_ID(con_id), status, source_id);
    return;
}

void stream_if_transform_from_ep(CONNECTION_LINK con_id,
                                 unsigned sid,
                                 STREAM_TRANSFORM_ID_CBACK callback)
{
    CONNECTION_LINK reversed_con_id;
    ENDPOINT *ep;
    TRANSFORM *tfm;

    patch_fn_shared(stream_if);

    reversed_con_id = REVERSE_CONNECTION_ID(con_id);

    if (sid == 0)
    {
        callback(reversed_con_id, STATUS_INVALID_CMD_PARAMS, 0);
        return;
    }

    if (stream_kip_transform_from_ep(reversed_con_id,
                                     sid,
                                     callback))
    {
        return;
    }

    ep = stream_endpoint_from_extern_id(sid);
    if (ep != NULL)
    {
        tfm = stream_transform_from_endpoint(ep);
    }
    else
    {
        /*
         * This could be a legitimate operator endpoint; these are
         * only created on connection. In that case we need to
         * return OK + transform 0, as if the endpoint exists but
         * is unconnected.
         * (This does mean that we also return 'OK' for completely
         * bogus endpoints. Oh well.)
         */
        tfm = NULL;
    }
    callback(reversed_con_id,
             STATUS_OK,
             stream_external_id_from_transform(tfm));
}

/****************************************************************************
 *
 * stream_if_propagate_kick
 *
 * Passes on a kick from an endpoint to the thing it's connected to.
 * ep_id = endpoint which has generated a kick (identified by external ID).
 * Note the use of stream_endpoint_from_extern_id, which searches through
 * a list of endpoints. This process could be optimised if the API
 * took an ENDPOINT* directly.
 */
void stream_if_propagate_kick(unsigned ep_id)
{
    ENDPOINT *this_ep = stream_endpoint_from_extern_id(ep_id);
    ENDPOINT_KICK_DIRECTION kick_direction;
    patch_fn_shared(stream_if);

    PL_PRINT_P2(TR_STREAM, "stream_if_propagate_kick: ep_id 0x%x = endpoint %p\n", ep_id, this_ep);

    /* TODO MULTICORE: propagation between a local op EP and a shadow EP may happen. Latter needs to
     * act via KIP to reach other processor.
     */

    if (this_ep != NULL)
    {
        if (this_ep->direction == SOURCE)
        {
            kick_direction = STREAM_KICK_FORWARDS;
        }
        else
        {
            kick_direction = STREAM_KICK_BACKWARDS;
        }

        propagate_kick(this_ep, kick_direction);
    }
    else
    {
        /* Uh-oh. The caller must have passed in an iffy endpoint ID.
         * This should only be caused by a programming error, so let's panic. */
        panic_diatribe(PANIC_AUDIO_INVALID_KEY_OR_ENDPOINT_ID, ep_id);
    }
}

/****************************************************************************
 *
 * stream_if_kick_ep
 *
 */
void stream_if_kick_ep(ENDPOINT *ep, ENDPOINT_KICK_DIRECTION kick_dir)
{
    /* TODO MULTICORE: check for shadow EP case. */
    ep->functions->kick(ep, kick_dir);
}


/****************************************************************************
 *
 * stream_if_get_info
 *
 */
bool stream_if_get_info(unsigned id, STREAM_INFO_KEY key, uint32* value)
{
    ENDPOINT *ep = stream_endpoint_from_extern_id(id);

#ifdef STREAM_INFO_KEY_ENDPOINT_EXISTS
    if (key == STREAM_INFO_KEY_ENDPOINT_EXISTS)
    {
        /*
         * Special case: ENDPOINT_EXISTS doesn't need to be dispatched to
         * endpoint-specific code, and *does* need to return TRUE for
         * operator endpoints, even if they haven't yet been connected to
         * (and hence created by Kymera). The meaning of "exists" here
         * is "can potentially be connected to".
         * (Shadow endpoints also have the 'don't exist until connected
         * to' nature, but ACCMD clients won't be asking about those.)
         */
        if (ep)
        {
            /* Endpoint structure already exists, hence obviously
             * connectable. */
            return TRUE;
        }
        else if (STREAM_EP_IS_OPEP_ID(id) && opmgr_is_opidep_valid(id))
        {
            /* Currently unconnected operator terminal for which
             * stream_connect stands a chance of succeeding. */
            return TRUE;
        }
        else
        {
            /* No reason to believe that stream_connect will succeed
             * for this sid. */
            return FALSE;
        }
    }
#endif

    /*
     * For other info keys, the endpoint needs to actually exist as far
     * as Kymera is concerned to return any info.
     */
    if (ep == NULL)
    {
        return FALSE;
    }

    /* STREAM_INFO_KEY is a subset of the possible keys supported by endpoints. */
    return stream_get_endpoint_config(ep, (unsigned) key, value);
}


/****************************************************************************
 *
 * stream_if_get_connection_list
 *
 */
void stream_if_get_connection_list(CONNECTION_LINK con_id,
                                   unsigned source_id,
                                   unsigned sink_id,
                                   STREAM_INFO_LIST_CBACK callback)
{
    unsigned *conn_list;
    unsigned length;
    patch_fn_shared(stream_if);

    /* if the external ID is not zero, meaning no filtering, then make these into internal IDs */
    /* If an operator ID was received instead of endpoint, conversion will leave it intact */
    if(sink_id != 0)
    {
        TOGGLE_EP_ID_BETWEEN_INT_AND_EXT(sink_id);

        /* If an operator ID was sent, ensure all terminal bits are zeroed. If it is an endpoint,
         * it's superfluous but harmless - saves some conditional code. */
        sink_id &= (~STREAM_EP_CHAN_MASK);
    }

    if(source_id != 0)
    {
        TOGGLE_EP_ID_BETWEEN_INT_AND_EXT(source_id);
        source_id &= (~STREAM_EP_CHAN_MASK);
    }

    /* get the stuff */
    stream_get_connection_list(source_id, sink_id, &length, &conn_list);

    /* Pass info to callback with reversed connection ID */
    callback(REVERSE_CONNECTION_ID(con_id), STATUS_OK, length, conn_list);
    pfree(conn_list);
}

/****************************************************************************
 *
 * stream_if_set_system_sampling_rate
 */
bool stream_if_set_system_sampling_rate(uint32 sampling_rate)
{
    if( sampling_rate >  STREAM_AUDIO_SAMPLE_RATE_96K ||
        sampling_rate <  STREAM_AUDIO_SAMPLE_RATE_8K )
    {
        return FALSE;
    }

    L2_DBG_MSG2("stream_if_set_system_sampling_rate: %d -> %d",
                system_stream_rate, sampling_rate);
    system_stream_rate = sampling_rate;
    return TRUE;
}

/****************************************************************************
 *
 * stream_if_get_system_sampling_rate
 */
uint32 stream_if_get_system_sampling_rate(void)
{
    return system_stream_rate;
}

/****************************************************************************
 *
 * stream_if_set_system_kick_period
 */
bool stream_if_set_system_kick_period(TIME_INTERVAL kp)
{
    if ((kp >= MIN_KICK_PERIOD_IN_USECS) && (kp <= MAX_KICK_PERIOD_IN_USECS))
    {
        L2_DBG_MSG2("stream_if_set_system_kick_period: %d -> %d",
                    system_kick_period, kp);
        system_kick_period = kp;
        return TRUE;
    }
    return FALSE;
}

/****************************************************************************
 *
 * stream_if_get_system_kick_period
 */
TIME_INTERVAL stream_if_get_system_kick_period(void)
{
#if defined(INSTALL_MIB)
    if (system_kick_period == 0)
    {
        /* This shouldn't happen on P1, because the P0 value is sent
         * via SYSTEM_KEYS_MSG_KICK_PERIOD at P1 boot
         */
        PL_ASSERT(PROC_PRIMARY_CONTEXT());
        system_kick_period = mibgetrequ16(AUDIOENDPOINTKICKPERIOD);
        L2_DBG_MSG1("Default system_kick_period: %d", system_kick_period);
    }
#endif
    return system_kick_period;
}

