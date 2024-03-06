/****************************************************************************
 * Copyright (c) 2011 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stream_kip.c
 * \ingroup stream
 *
 * Private Stream KIP elements.
 */

/****************************************************************************
Include Files
*/

#include "types.h"
#include "proc/proc.h"
#include "ipc/ipc.h"
#include "stream/stream_private.h"
#include "buffer/buffer_metadata.h"
#if !defined(COMMON_SHARED_HEAP)
#include "buffer/buffer_metadata_kip.h"
#endif /* !COMMON_SHARED_HEAP */

/****************************************************************************
Private Type Declarations
*/

/**
 * KIP state is maintained only if there are multiple IPC interactions and/or
 * multiple KIP messages involved to complete once client request over KIP, that
 * requires preserving context.
 */
typedef enum
{
    STREAM_KIP_STATE_NONE = 0,
    STREAM_KIP_STATE_CONNECT,
    STREAM_KIP_STATE_DISCONNECT,
    STREAM_KIP_STATE_DISCONNECT_ENDPOINT,
} STREAM_KIP_STATE;

typedef struct
{
    STREAM_KIP_STATE        state;
    union
    {
        void                                 *any_info;
        STREAM_KIP_CONNECT_INFO              *connect_info;
        STREAM_KIP_TRANSFORM_DISCONNECT_INFO *disconnect_info;
    } context;
} STREAM_KIP_STATE_INFO;

/* Structure for keeping record of connect stages when shadow EPs are involved */
struct STREAM_KIP_CONNECT_INFO
{
    /* Packed connection ID with remote processor id */
    uint16 packed_con_id;

    /* Local endpoint ids. This might be: a external operator endpoint id
       or a real endpoint id or a shadow operator endpoint id. */
    uint16 source_id;
    uint16 sink_id;

    uint16 data_channel_id;
#if !defined(COMMON_SHARED_HEAP)
    uint16 meta_channel_id;

    bool metadata_channel_is_activated : 8;
#endif /* !COMMON_SHARED_HEAP */
    bool data_channel_is_activated : 8;

    /* endpoint locations */
    STREAM_EP_LOCATION ep_location;

    /* internal transform id */
    TRANSFORM_INT_ID tr_id;

    STREAM_CONNECT_INFO connect_info;

    /* The callback to notify connection status */
    STREAM_TRANSFORM_CBACK callback;

    /* The data format of the sink. */
    AUDIO_DATA_FORMAT data_format;
    /* Check if the shadow endpoints should be synchronised. */
    bool sync_shadow_eps;
};

/* Structure for keeping record of transform disconnect
 * when shadow EPs are involved
 **/
struct STREAM_KIP_TRANSFORM_DISCONNECT_INFO
{
    /* Packed connection ID with remote processor id */
    uint16 packed_con_id;

    /* count */
    uint16 count;

    /* success count */
    uint16 success_count;

    /* remote success count */
    uint16 remote_success_count;

    /* flag to show which callback to call */
    bool disc_cb_flag;

    /* The callback to notify transform disconnect status */
    STREAM_KIP_TRANSFORM_DISCONNECT_CB callback;

    /* transform list. Don't change the position from here */
    TRANSFORM_ID tr_list[1];
};

/* Structure used for maintaining remote kip transform information.
 * P0 uses the same to maintain remote transform list as well */
struct STREAM_KIP_TRANSFORM_INFO
{
    /* This is the source id at the secondary core*/
    unsigned source_id;

    /* This is the sink id at the secondary core*/
    unsigned sink_id;

    /* data channel id. 0 if it is not present */
    uint16 data_channel_id;

    /* Metadata channel ID */
    uint16 meta_channel_id;

    /* Internal transform id */
    TRANSFORM_INT_ID id:8;

    /* Remote Processor id */
    unsigned processor_id:8;

    /* Enable to allow using the data channel connected to
     * the transform. Disable it to allow deactivating the
     * data channel.
     */
    bool enabled:8;

    /* Real source ep */
    bool real_source_ep:1;

    /* Real sink ep */
    bool real_sink_ep:1;

    struct STREAM_KIP_TRANSFORM_INFO* next;
};

typedef enum
{
    SOURCE_INDEX = 0,
    SINK_INDEX = 1,
    TOTAL_INDEX = 2
} DIRECTION_INDEX;

/****************************************************************************
Private Constant Declarations
*/

/****************************************************************************
Private Macro Declarations
*/
#define STREAM_KIP_STATE_IN_IDLE() \
        (stream_kip_state.state == STREAM_KIP_STATE_NONE)
#define STREAM_KIP_STATE_IN_CONNECT() \
        (stream_kip_state.state == STREAM_KIP_STATE_CONNECT)
#define STREAM_KIP_STATE_IN_DISCONNECT() \
        (stream_kip_state.state == STREAM_KIP_STATE_DISCONNECT)
#define STREAM_KIP_STATE_IN_DISCONNECT_ENDPOINT() \
        (stream_kip_state.state == STREAM_KIP_STATE_DISCONNECT_ENDPOINT)
#if !defined(COMMON_SHARED_HEAP)
static bool stream_kip_get_metadata_channel(STREAM_KIP_CONNECT_INFO *state,
                                            PROC_ID_NUM proc_id);
#endif /* COMMON_SHARED_HEAP */
static bool stream_kip_set_ep_meta_info(STREAM_KIP_CONNECT_INFO *state,
                                        bool support_metadata);

#if !defined(COMMON_SHARED_HEAP)
#define KIP_METADATA_BUFFER_SIZE (256)

/**
 * Always use the highest channel number in IPC for metadata. -1 because
 * channel numbers start from zero.
 */
#define META_DATA_CHANNEL_NUM (IPC_MAX_DATA_CHANNELS-1)
#define DATA_CHANNEL_IS_META(id) (((id) & META_DATA_CHANNEL_NUM) == META_DATA_CHANNEL_NUM)
#define BOTH_CHANNELS_ARE_ACTIVATED(s) ((s)->data_channel_is_activated && (s)->metadata_channel_is_activated)
#else
#define BOTH_CHANNELS_ARE_ACTIVATED(s) ((s)->data_channel_is_activated)
#endif /* !COMMON_SHARED_HEAP */


/****************************************************************************
Private Variable Definitions
*/

/*
 * There are some instances, there may be multiple KIP messages
 * or IPC activities to complete one client request. especially
 * while activating or deactivating the data channels which
 * requires to store local context and continue the sequence.
 * No other kip messages are entertained during that time.
 */
STREAM_KIP_STATE_INFO stream_kip_state = {STREAM_KIP_STATE_NONE, {NULL}};

/**
 * This keeps information about the remote transforms. On P0, this list contains
 * a copy of P1 transforms. On P1, it contains all the transform created on P1.
 * All P0 related transform local transforms are maintained in its transform_list.
 */
STREAM_KIP_TRANSFORM_INFO *kip_transform_list = NULL;

/****************************************************************************
Private Function Declarations
*/
#if !defined(COMMON_SHARED_HEAP)
static PROC_ID_NUM stream_kip_get_remote_proc_id(CONNECTION_LINK packed_con_id)
{
    PROC_ID_NUM remote_proc_id;

    if (PROC_PRIMARY_CONTEXT())
    {
        remote_proc_id = GET_RECV_PROC_ID(packed_con_id);
    }
    else
    {
        remote_proc_id = GET_SEND_PROC_ID(packed_con_id);
    }
    return remote_proc_id;
}
#endif /* !COMMON_SHARED_HEAP */

/**
 * \brief Set KIP state to 'not active'.
 *
 * \param free_context - Flag that indicates whether to free state context
 */
static void stream_kip_state_to_none(bool free_context)
{
    if ((stream_kip_state.context.any_info != NULL) && free_context)
    {
        /* Free the context only requested */
        pfree(stream_kip_state.context.any_info);
    }

    stream_kip_state.context.any_info = NULL;
    stream_kip_state.state            = STREAM_KIP_STATE_NONE;
}

/**
 * \brief Destroy endpoints of a given state.
 *
 * \param state         - KIP connection state information
 */
static void stream_kip_destroy_endpoint_ids(STREAM_KIP_CONNECT_INFO *state)
{
    if (state != NULL)
    {
        stream_destroy_endpoint_id(state->source_id);
        stream_destroy_endpoint_id(state->sink_id);
        if ((state->ep_location == STREAM_EP_REMOTE_SINK) &&
            (state->data_channel_id != 0))
        {
            ipc_destroy_data_channel(state->data_channel_id);
        }
    }
}

/**
 * \brief Set KIP state to connected.
 *
 * \param info          - Context to remember for this state
 *
 * \return TRUE if successful, FALSE if not
 */
static bool stream_kip_state_to_connect(STREAM_KIP_CONNECT_INFO *info)
{
    if (STREAM_KIP_STATE_IN_IDLE())
    {
        stream_kip_state.context.connect_info = info;
        stream_kip_state.state = STREAM_KIP_STATE_CONNECT;
        return TRUE;
    }
    return FALSE;
}

/**
 * \brief Get KIP connect state info.
 *
 * \return Returns state's connect info.
 */
static inline STREAM_KIP_CONNECT_INFO *stream_kip_state_get_connect_info(void)
{
    return stream_kip_state.context.connect_info;
}

/**
 * \brief Set KIP state to disconnected.
 *
 * \param info          - Context to remember for this state
 *
 * \return TRUE if successful, FALSE if not
 */
static bool stream_kip_state_to_disconnect(STREAM_KIP_TRANSFORM_DISCONNECT_INFO *info)
{
    if (STREAM_KIP_STATE_IN_IDLE())
    {
        stream_kip_state.context.disconnect_info = info;
        stream_kip_state.state = STREAM_KIP_STATE_DISCONNECT;
        return TRUE;
    }
    return FALSE;
}

/**
 * \brief Set KIP state to disconnecting an endpoint
 *
 * \return TRUE if successful, FALSE if not
 */
static bool stream_kip_state_to_disconnect_endpoint(void)
{
    if (STREAM_KIP_STATE_IN_IDLE())
    {
        stream_kip_state.context.disconnect_info = NULL;
        stream_kip_state.state = STREAM_KIP_STATE_DISCONNECT_ENDPOINT;
        return TRUE;
    }
    return FALSE;
}

/**
 * \brief Get KIP disconnect state info.
 *
 * \return Returns state's disconnect info.
 */
static inline STREAM_KIP_TRANSFORM_DISCONNECT_INFO *stream_kip_state_get_disconnect_info(void)
{
    return stream_kip_state.context.disconnect_info;
}

/**
 * \brief Update KIP connection state information.
 *
 * \param state            - KIP connect state information
 * \param channel_id       - The data channel id for this state
 * \param buffer_size      - The buffer size for this state
 * \param flags            - The flags for this state
 */
static inline void stream_kip_update_buffer_info(STREAM_KIP_CONNECT_INFO *state,
                                                 unsigned channel_id,
                                                 unsigned buffer_size,
                                                 unsigned flags)
{
    union _parter_buffer_config_flags
    {
       struct BUFFER_CONFIG_FLAGS bcf;
       unsigned flags;
    } parter_buffer_config_flags;

    parter_buffer_config_flags.flags = flags;
    state->data_channel_id  = (uint16)channel_id;
    state->connect_info.buffer_info.buffer_size = buffer_size;

    /* Update the flag for supports_metadata only */
    state->connect_info.buffer_info.flags.supports_metadata
                            = parter_buffer_config_flags.bcf.supports_metadata;

    /* Currently most flags are ignored and shadow endpoints uses pre-defined
     * flags. Only the 'source_wants_kicks' and 'sink_wants_kicks' flags need
     * to be copied, for adc->P1 and P1->dac transfers.
     * If any other flags need to be configured in the stream shadow
     * endpoint, it needs to call the config() handler to do so.
     *
     */
    UNUSED(parter_buffer_config_flags);
}

/**
 * \brief Get KIP endpoint from connect state
 *
 * \param state            - KIP connect state information
 *
 * \return Pointer to KIP connection state's remote endpoint
 */
static ENDPOINT *stream_kip_get_kip_endpoint_from_state(const STREAM_KIP_CONNECT_INFO *state)
{
    ENDPOINT *ep = NULL;
    uint16 id;

    if ((state->ep_location != STREAM_EP_REMOTE_SOURCE) &&
        (state->ep_location != STREAM_EP_REMOTE_SINK))
    {
        ep = NULL;
    }
    else
    {
        if (state->ep_location == STREAM_EP_REMOTE_SOURCE)
        {
            id = state->source_id;
        }
        else
        {
            /* STREAM_EP_REMOTE_SINK */
            id = state->sink_id;
        }
        ep = stream_endpoint_from_extern_id(id);
    }

    return ep;
}

/**
 * \brief Internal function to get the data format from the state.
 *
 * \param state           - Current KIP state or context to use
 *
 * \return data_format stored in the state
 */
static inline AUDIO_DATA_FORMAT stream_kip_get_data_format_from_state(STREAM_KIP_CONNECT_INFO *state)
{
    return state->data_format;
}

/**
 * \brief Update the data format stored in the state.
 *
 * \param state            - KIP connect state information
 * \param data_format      - The data format to be set
 */
static inline void stream_kip_set_data_format_to_state(STREAM_KIP_CONNECT_INFO *state, AUDIO_DATA_FORMAT data_format)
{
    state->data_format = data_format;
}

/**
 * \brief Set the data format from the state to the shadow endpoint.
 *
 * \param state            - KIP connect state information
 */
static void stream_kip_set_data_format_for_shadow_ep(STREAM_KIP_CONNECT_INFO *state)
{
    ENDPOINT *ep;
    AUDIO_DATA_FORMAT data_format;

    data_format = stream_kip_get_data_format_from_state(state);
    ep = stream_kip_get_kip_endpoint_from_state(state);

    ep->functions->configure(ep, EP_DATA_FORMAT, data_format);
}

/**
 * \brief Internal function to get the data format of the operator connected to
 *        the shadow endpoint.
 *
 * \param ep_id             - The ID of the endpoint connected to a shadow
 *                            endpoint.
 * \return data_format      - The data format of the endpoint ep_id.
 */
static AUDIO_DATA_FORMAT stream_kip_get_data_format_from_ep(unsigned ep_id)
{
    uint32 data_format;
    ENDPOINT *ep;

    ep = stream_endpoint_from_extern_id(ep_id);

    STREAM_KIP_ASSERT(ep != NULL);

    stream_get_endpoint_config(ep, EP_DATA_FORMAT, &data_format);

    return (AUDIO_DATA_FORMAT) data_format;
}

/**
 * \brief Get the data format from the endpoint connected to shadow endpoint
 *        and set it to the shadow endpoint and to the state.
 *
 * \param state            - KIP connect state information
 */
static void stream_kip_get_and_set_data_format_for_shadow_ep(STREAM_KIP_CONNECT_INFO *state)
{
    AUDIO_DATA_FORMAT data_format;
    uint16 ep_id;

    if (state->ep_location == STREAM_EP_REMOTE_SOURCE)
    {
        ep_id = state->sink_id;
    }
    else
    {
        /* STREAM_EP_REMOTE_SINK */
        ep_id = state->source_id;
    }

    data_format = stream_kip_get_data_format_from_ep(ep_id);
    stream_kip_set_data_format_to_state(state, data_format);
    stream_kip_set_data_format_for_shadow_ep(state);
}
/**
 * \brief Get the non-KIP endpoint from connect state
 *
 * \param state            - KIP connect state information
 *
 * \return Pointer to KIP connection state's local endpoint
 */
static inline ENDPOINT *stream_kip_get_local_endpoint_from_state(const STREAM_KIP_CONNECT_INFO *state)
{
    ENDPOINT *ep;
    uint16 id;

    if (state->ep_location == STREAM_EP_REMOTE_SINK)
    {
        id = state->source_id;
    }
    else
    {
        id = state->sink_id;
    }
    ep = stream_endpoint_from_extern_id(id);

    return ep;
}

/**
 * \brief Get buffer and activate data channel. This function is being
 *        called when the endpoint is EP_REMOTE_SINK.
 *
 * \param state            - Connect state information
 * \param proc_id          - Processor ID
 *
 * \return TRUE on success
 */
static bool kip_get_buffer_and_activate_channels(STREAM_KIP_CONNECT_INFO *state,
                                                 PROC_ID_NUM proc_id)
{
    bool result;
    IPC_STATUS activate_result;
    bool get_buff_result;
    ENDPOINT *source_ep = stream_endpoint_from_extern_id(state->source_id);
    ENDPOINT *sink_ep   = stream_endpoint_from_extern_id(state->sink_id);

    /* Configure the required buffer size of the operator from the other core. */
    sink_ep->functions->configure(sink_ep,
                                  EP_SET_SHADOW_BUFFER_SIZE,
                                  state->connect_info.buffer_info.buffer_size);

    activate_result = IPC_ERROR_FAILED;
    get_buff_result = FALSE;
    result = FALSE;

    /* Get the connect buffer. */
    get_buff_result = stream_connect_get_buffer(source_ep, sink_ep, &state->connect_info);
    /* Activate data channel. */
#if defined(COMMON_SHARED_HEAP)
    if (get_buff_result)
    {
        /* Configure the actual buffer size of the shadow endpoint after the
         * buffer was assigned. */
        sink_ep->functions->configure(sink_ep,
                                      EP_SET_SHADOW_BUFFER_SIZE,
                                      state->connect_info.buffer_info.buffer_size);
        activate_result = ipc_activate_data_channel(state->data_channel_id,
                                                    proc_id,
                                             state->connect_info.buffer_info.buffer,
                                                    FALSE);
        if (IPC_SUCCESS == activate_result)
        {
            result = TRUE;
        }
    }
#else
    activate_result = ipc_activate_data_channel(state->data_channel_id,
                                                proc_id,
                                         state->connect_info.buffer_info.buffer,
                                                TRUE);

    if (get_buff_result && (IPC_SUCCESS == activate_result))
    {
        result = TRUE;
    }

    if (state->connect_info.buffer_info.flags.supports_metadata && result)
    {
        if (!stream_kip_get_metadata_channel(state, proc_id))
        {
            STREAM_CONNECT_FAULT(SC_KIP_GET_META_CHANNEL_FAILED,
                                 "Failed to get_metadata_channel");
            /* Disable metadata support */
            stream_kip_set_ep_meta_info(state, FALSE);
        }
    }
#endif /* COMMON_SHARED_HEAP */
    return result;
}

/**
 * \brief Internal function to the local endpoint buffer details.
 *
 * \param state            - KIP connect state information
 * \param buf_details      - Pointer to return the buffer details
 *
 * \return TRUE on successfully retrieving the buffer details
 */
static bool stream_kip_get_local_ep_buffer_details(STREAM_KIP_CONNECT_INFO *state,
                                                   BUFFER_DETAILS *buf_details)
{
    ENDPOINT *ep;
    ep = stream_kip_get_local_endpoint_from_state(state);

    STREAM_KIP_ASSERT(ep != NULL);

    /* Get the local endpoint buffer details */
    return ep->functions->buffer_details(ep, buf_details);
}

/**
 * \brief Internal function to get buffer info for remote information,
 *        the buffer info is stored in the KIP connect info object.
 *
 * \param state            - KIP connect state information
 *
 * \return TRUE on successfully retrieving the buffer info
 */
static bool stream_kip_ep_get_buffer_info(STREAM_KIP_CONNECT_INFO *state)
{
    BUFFER_DETAILS buffer_details;
    unsigned buffer_size;
    STREAM_TRANSFORM_BUFFER_INFO* info = &state->connect_info.buffer_info;

    /* Get the local endpoint buffer details */
    if (!stream_kip_get_local_ep_buffer_details(state, &buffer_details))
    {
        return FALSE;
    }

    /*
     * Any flags that requires sharing with the remote
     * must be set here.
     */

    /* set the buffer size */
    buffer_size = get_buf_size(&buffer_details);

    info->flags.supports_metadata = buffer_details.supports_metadata;

    if (buffer_size > info->buffer_size)
    {
        info->buffer_size = buffer_size;
    }

    return TRUE;
}

/**
 * \brief Find an existing transform created using the same terminal group belongs
 *        to the provided sink and source.
 *
 * \param source_id The source endpoint id
 * \param sink_id   The sink endpoint id
 *
 * \return Any KIP transform info already present with the same base endpoint ids
 */
static STREAM_KIP_TRANSFORM_INFO *stream_kip_get_created_transform(unsigned source_id,
                                                                   unsigned sink_id)
{
    STREAM_KIP_TRANSFORM_INFO *kip_tr = kip_transform_list;

    /* figure out what should be the best base id for search */
    bool source_id_is_base = STREAM_EP_IS_REALEP_ID(sink_id) ?
                                        TRUE : STREAM_EP_IS_OPEP_ID(source_id);

    patch_fn_shared(stream_kip);

    while (kip_tr != NULL)
    {
        /* Either sink or source id must be a KIP endpoint to compare. Otherwise next transform */
        if (STREAM_EP_IS_SHADOW_ID(kip_tr->source_id) || STREAM_EP_IS_SHADOW_ID(kip_tr->sink_id))
        {
            bool source_match;
            bool sink_match;

            source_match = (GET_BASE_EPID_FROM_EPID(kip_tr->source_id) ==
                           GET_BASE_EPID_FROM_EPID(source_id));
            sink_match = (GET_BASE_EPID_FROM_EPID(kip_tr->sink_id) ==
                         GET_BASE_EPID_FROM_EPID(sink_id));

            if (source_id_is_base)
            {
                /* Check the base ids. if the source ids are matching and either both sink ids are
                 * real endpoints or both sink id bases are matching, then we found it.
                 */
                if (source_match &&
                    ((STREAM_EP_IS_REALEP_ID(sink_id) && kip_tr->real_sink_ep) ||
                    sink_match))
                {
                    /* This is another channel connecting same operators */
                    break;
                }
            }
            else
            {
                /* Check the base ids. if the sink base ids are matching and either both source ids are
                 * real endpoints or both source id bases are matching, then we found it.
                 */
                if (sink_match &&
                    ((STREAM_EP_IS_REALEP_ID(source_id) && kip_tr->real_source_ep) ||
                    source_match))
                {
                    /* This is another channel connecting same operators */
                    break;
                }
            }
        }
        kip_tr = kip_tr->next;
    }

    return kip_tr;
}

/**
 * \brief Find the head_of_sync shadow endpoint from another endpoint associated
 *        with the same port.
 *
 * \param data_channel_id     The source endpoint id
 * \param current_ep    The endpoint to be synced
 *
 * \return head_of_sync endpoint of the shadow endpoint found.
 */
static ENDPOINT *stream_kip_get_head_of_sync_endpoint(unsigned data_channel_id,
                                                      ENDPOINT *current_ep)
{
    ENDPOINT *sync_ep = NULL;
    unsigned port_id;

    patch_fn_shared(stream_kip);

    port_id = ipc_get_data_channelid_port(data_channel_id);

    sync_ep = (current_ep->direction == SINK ) ? sink_endpoint_list:
                                                 source_endpoint_list;
    while (sync_ep != NULL)
    {
        if ((sync_ep != current_ep) && STREAM_EP_IS_SHADOW(sync_ep))
        {
            uint16 channel_id;
            uint16 pid;

            channel_id = stream_shadow_get_channel_id(sync_ep);
            pid = ipc_get_data_channelid_port(channel_id);
            if (pid == port_id)
            {
                return sync_ep->state.shadow.head_of_sync;
            }
        }

        sync_ep = sync_ep->next;
    }

    return NULL;
}

/**
 * \brief Internal function to get a used port by another endpoint of the
 *        of the same operator. It searches the kip transform list to find it.
 *
 * \param source_id The source id
 * \param sink_id   sink id
 *
 * \return 0 if not found, otherwise port id (> 0).
 */
static uint16 stream_kip_get_used_port(unsigned source_id, unsigned sink_id)
{
    STREAM_KIP_TRANSFORM_INFO *tr = NULL;
    uint16 port_id = 0;

    tr = stream_kip_get_created_transform(source_id, sink_id);

    if (tr != NULL)
    {
        /* we got the details */
        port_id = ipc_get_data_channelid_port(tr->data_channel_id);
    }

    return port_id;
}

/**
 * \brief Update the metadata information of a KIP endpoint
 *
 * \param state            - Connect state information
 * \param support_metadata - Flag to indicate whether to support metadata
 *
 * \return  TRUE on success
 */
static bool stream_kip_set_ep_meta_info(STREAM_KIP_CONNECT_INFO *state,
                                        bool support_metadata)
{
    ENDPOINT *kip_ep = stream_kip_get_kip_endpoint_from_state(state);

    STREAM_KIP_ASSERT(kip_ep != NULL);

    /* update the meta data channel id in KIP endpoint */
    return (kip_ep->functions->configure(kip_ep, EP_METADATA_SUPPORT,
                                         support_metadata));
}

/**
 * \brief Check if both endpoints support metadata
 *
 * \param state       - Connect state information
 *
 * \return  TRUE on success
 */
static bool stream_kip_endpoints_support_metadata(STREAM_KIP_CONNECT_INFO *state)
{
    BUFFER_DETAILS buffer_details;
    ENDPOINT *local_ep = stream_kip_get_local_endpoint_from_state(state);

    patch_fn_shared(stream_kip);

    if (!local_ep->functions->buffer_details(local_ep, &buffer_details))
    {
        return FALSE;
    }

    if ((state->connect_info.buffer_info.flags.supports_metadata) &&
        (buffer_details.supports_metadata))
    {
        /* Update the metadata information for the KIP endpoint */
        if (!stream_kip_set_ep_meta_info(state, buffer_details.supports_metadata))
        {
            return FALSE;
        }
        return TRUE;
    }

    return FALSE;
}

#if !defined(COMMON_SHARED_HEAP)
/**
 * \brief Update the metadata buffer information based on previous connections
 *
 * \param state       - State information
 */
static void stream_kip_update_metadata_buffer(STREAM_KIP_CONNECT_INFO *state)
{
    patch_fn_shared(stream_kip);

    ENDPOINT *cur_kip_ep = stream_kip_get_kip_endpoint_from_state(state);

    STREAM_KIP_ASSERT(cur_kip_ep != NULL);

    /* Check if we have got a synchronised connection in the same base of source ep and sink ep */
    STREAM_KIP_TRANSFORM_INFO *kip_tr = stream_kip_get_created_transform(state->source_id, state->sink_id);

    if (kip_tr != NULL)
    {
        cur_kip_ep->functions->configure(cur_kip_ep, EP_METADATA_CHANNEL_ID, state->meta_channel_id);
    }
}

/**
 * \brief Get buffer and activate data channel
 *
 * \param state       - State information
 * \param proc_id     - Processor ID
 *
 * \return TRUE on success
 */
static bool stream_kip_activate_metadata_channel(STREAM_KIP_CONNECT_INFO *state,
                                                 PROC_ID_NUM proc_id)
{
    ENDPOINT *source_ep = stream_endpoint_from_extern_id(state->source_id);
    ENDPOINT *sink_ep   = stream_endpoint_from_extern_id(state->sink_id);
    ENDPOINT *kip_ep    = STREAM_EP_GET_SHADOW_EP(source_ep, sink_ep);

    patch_fn_shared(stream_kip);

    /*
     * In addition to tags we need to synchronise a few more values across
     * cores. We have therefore extended the tCbuffer structure with a few extra
     * fields that are only needed by the KIP layer. This is allocated here in
     * KIP so to the rest of the system this structure is an ordinary cbuffer.
     *
     * The reason we're creating a buffer first and then delete it after a copy
     * is limitations in cbuffer API which always allocates the cbuffer
     * structure internally and doesn't allow us to allocate the structure and
     * pass it as a pointer.
     */
    tCbuffer *shared_metadata_buf = cbuffer_create_shared_with_malloc(KIP_METADATA_BUFFER_SIZE,
                                                                      BUF_DESC_SW_BUFFER);

    KIP_METADATA_BUFFER* cbuffer_extra = xzppnew(KIP_METADATA_BUFFER,
                                                 MALLOC_PREFERENCE_SHARED);

    if ((cbuffer_extra == NULL) || (shared_metadata_buf) == NULL)
    {
        return FALSE;
    }

    /* Swap the buffer with the extended version */
    cbuffer_extra->parent = *shared_metadata_buf;
    cbuffer_destroy_struct(shared_metadata_buf);
    shared_metadata_buf = &(cbuffer_extra->parent);

    stream_shadow_set_shared_metadata_buffer(kip_ep, shared_metadata_buf);

    return (IPC_SUCCESS == ipc_activate_data_channel(state->meta_channel_id,
                                                     proc_id,
                                                     shared_metadata_buf,
                                                     FALSE));
}
#endif /* !COMMON_SHARED_HEAP */

/**
 * \brief Internal function to create endpoints and get buffer details
 *        while creating the kip endpoints
 *
 * \param packed_con_id  - Packed connection id
 * \param state          - Current KIP state or context to use if and
 *                         when a KIP reply comes back from P0.
 *
 * \return  TRUE on success
 */
static bool stream_kip_create_eps_for_connect(CONNECTION_LINK packed_con_id,
                                              STREAM_KIP_CONNECT_INFO *state)
{
    patch_fn_shared(stream_kip);

    bool result = TRUE;
    ENDPOINT *source_ep;
    ENDPOINT *sink_ep;

    /* create data channel if local source */
    if (state->ep_location == STREAM_EP_REMOTE_SINK)
    {
        uint16 ch_id = state->data_channel_id;

        /* ch_id may contain the proposed channel id from the remote. If it was
         * not proposed, generate one
         */
        if (ch_id == 0)
        {
            ch_id = (uint16)((STREAM_EP_IS_OPEP_ID(state->source_id)) ?
                           GET_TERMINAL_FROM_OPIDEP(state->source_id) :
              get_hardware_channel(stream_endpoint_from_extern_id(state->source_id)));
        }

        /* Find out if the same operator is being already connect via KIP
         * query the the KIP transform list. If it is already being created
         * use the port ID.
         */
        uint16 port_id =  stream_kip_get_used_port(state->source_id,
                                                   state->sink_id);

        state->data_channel_id = 0;
        result = (ipc_create_data_channel(port_id, ch_id,
                                     IPC_DATA_CHANNEL_WRITE,
                                     &state->data_channel_id) == IPC_SUCCESS);
    }
    else
    if (state->data_channel_id == 0)
    {
        state->data_channel_id = (uint16)((STREAM_EP_IS_OPEP_ID(state->sink_id)) ?
                                        GET_TERMINAL_FROM_OPIDEP(state->sink_id) :
             get_hardware_channel(stream_endpoint_from_extern_id(state->sink_id)));
    }

    source_ep = stream_create_endpoint(state->source_id, packed_con_id);
    sink_ep = stream_create_endpoint(state->sink_id, packed_con_id);

    if ((result) && (source_ep != NULL) && (sink_ep != NULL))
    {
        /* set the context as connect for the multi sequence connect */
        stream_kip_state_to_connect(state);

        /* Set the data format to the local shadow endpoint */
        stream_kip_get_and_set_data_format_for_shadow_ep(state);
    }
    else
    {
        /* Failed to create the endpoints */
        result = FALSE;

        /* clean up */
        stream_destroy_endpoint_id(state->source_id);
        stream_destroy_endpoint_id(state->sink_id);
        ipc_destroy_data_channel(state->data_channel_id);
    }

    return result;
}

/**
 * \brief Go through the transform id list, stop receiving kicks and
 *        deactivate ready. When the remote deactivate the KIP endpoints,
 *        just accept it.
 *
 * \param count       - Number of transform ids in the list
 * \param tr_id_list  - List of transform ids
 */
static void stream_kip_transform_deactivate_ready(unsigned count,
                                                  TRANSFORM_ID *tr_id_list)
{
    unsigned i;

    for (i = 0; (i < count); i++)
    {
        STREAM_KIP_TRANSFORM_INFO *tr;
        TRANSFORM_INT_ID id;

        id = STREAM_TRANSFORM_GET_INT_ID(tr_id_list[i]);
        tr = stream_kip_transform_info_from_id(id);

        if ((tr != NULL) && (ipc_get_data_channelid_port(tr->data_channel_id) != 0))
        {
            /* Disable the transform on KIP. This will stop the kicks through KIP*/
            tr->enabled = FALSE;
#if !defined(COMMON_SHARED_HEAP)
            if (ipc_get_data_channelid_dir(tr->data_channel_id) == IPC_DATA_CHANNEL_READ)
            {
                tCbuffer *buff = ipc_data_channel_get_cbuffer(tr->data_channel_id);
                /* Decremented the reference count and release the metadata information
                 * associated to this buffer */
                buff_metadata_release(buff);
            }
#endif /* COMMON_SHARED_HEAP */
        }
    }
}

/**
 * \brief pack and send a remote transform disconnect request
 *
 * \param packed_con_id Connection id
 * \param count         Number of transform ids in the list to disconnect. This must
 *                      not be 0.
 * \param tr_list       The transform list
 * \param state         This is void because DESTROY as well use this function
 *                      to disconnect the transform.
 *
 * \return  TRUE on success
 */
static bool stream_kip_send_transform_disconnect(CONNECTION_LINK packed_con_id,
                                                 unsigned count,
                                                 TRANSFORM_ID *tr_list)
{
    bool success;
    unsigned kip_length;
    uint16 *kip_msg;
    uint16 *payload;
    KIP_MSG_STREAM_TRANSFORM_DISCONNECT_REQ *req;
    ADAPTOR_MSGID msg_id;

    patch_fn_shared(stream_kip);

    /* Mark the transforms for deactivate, stop receiving kicks and
       ready for deactivate */
    stream_kip_transform_deactivate_ready(count, tr_list);

    /* Allocate the message. */
    kip_length = KIP_MSG_STREAM_TRANSFORM_DISCONNECT_REQ_TRANSFORM_IDS_WORD_OFFSET;
    kip_length += count;
    kip_msg = xpnewn(kip_length, uint16);
    if (kip_msg == NULL)
    {
        return FALSE;
    }

    /* Build the message. */
    req = (KIP_MSG_STREAM_TRANSFORM_DISCONNECT_REQ *) kip_msg;
    KIP_MSG_STREAM_TRANSFORM_DISCONNECT_REQ_COUNT_SET(req, count);
    payload = &KIP_MSG_STREAM_TRANSFORM_DISCONNECT_REQ_TRANSFORM_IDS_GET(req);
    adaptor_pack_list_to_uint16(payload, tr_list, count);

    /* Send the message. */
    msg_id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_STREAM_TRANSFORM_DISCONNECT_REQ);
    success = adaptor_send_message(packed_con_id,
                                   msg_id,
                                   kip_length,
                                   (ADAPTOR_DATA) req);

    /* Free the request. */
    pfree(req);

    return success;
}

/**
 * \brief pack and send a kip_transform_list remove entry request
 *
 * \param packed_con_id  - Packed connection id
 * \param count          - Number of transform ids in the list to remove.
 *                         This must not be 0.
 * \param tr_list        - The transform id list
 * \param state          - Current KIP state or context to use if and
 *                         when a KIP reply comes back from P0.
 *
 * \return  TRUE on success
 */
static bool stream_kip_transform_list_remove_entry(CONNECTION_LINK packed_con_id,
                                                   unsigned count,
                                                   TRANSFORM_ID *tr_list)
{
    bool success;
    unsigned kip_length;
    uint16 *kip_msg;
    uint16 *payload;
    KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_REQ *req;
    ADAPTOR_MSGID msg_id;

    patch_fn_shared(stream_kip);

    /* Allocate the message. */
    kip_length = KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_REQ_TRANSFORM_IDS_WORD_OFFSET;
    kip_length += count;
    kip_msg = xpnewn(kip_length, uint16);
    if (kip_msg == NULL)
    {
        return FALSE;
    }

    /* Build the message. */
    req = (KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_REQ *) kip_msg;
    KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_REQ_COUNT_SET(req, count);
    payload = &KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_REQ_TRANSFORM_IDS_GET(req);
    adaptor_pack_list_to_uint16(payload, tr_list, count);

    /* Send the message. */
    msg_id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_TRANSFORM_LIST_REMOVE_ENTRY_REQ);
    success = adaptor_send_message(packed_con_id,
                                   msg_id,
                                   kip_length,
                                   (ADAPTOR_DATA) req);

    /* Free the request. */
    pfree(req);

    return success;
}

/**
 * \brief Find next P0 only transform
 *
 * \param count       - Number of transform ids in the list
 * \param tr_id_list  - The transform id list
 *
 * \return Index of first P0-only transfers in the KIP transfer info list
 */
static unsigned stream_kip_find_p0_only_transform_start(unsigned count,
                                                        TRANSFORM_ID *tr_id_list)
{
    unsigned i;

    for (i = 0; i < count; i++)
    {
        TRANSFORM_INT_ID id;

        id = STREAM_TRANSFORM_GET_INT_ID(tr_id_list[i]);
        if (stream_kip_transform_info_from_id(id) == NULL)
        {
            break;
        }
    }
    return i;
}

/**
 * \brief Pack and send a remote destroy endpoint request
 *
 * \param packed_con_id    Connection id
 * \param remote_source_id The remote source endpoint id
 * \param remote_sink_id   The remote sink endpoint id
 * \param state            This is void because DESTROY as well use this
 *                         function to disconnect the transform.
 */
static void stream_kip_destroy_endpoints(CONNECTION_LINK packed_con_id,
                                         unsigned remote_source_id,
                                         unsigned remote_sink_id,
                                         STREAM_KIP_CONNECT_INFO *state)
{
    KIP_MSG_STREAM_DESTROY_ENDPOINTS_REQ req;
    ADAPTOR_MSGID msg_id;

    patch_fn_shared(stream_kip);

    KIP_MSG_STREAM_DESTROY_ENDPOINTS_REQ_SOURCE_ID_SET(&req, remote_source_id);
    KIP_MSG_STREAM_DESTROY_ENDPOINTS_REQ_SINK_ID_SET(&req, remote_sink_id);

    msg_id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_STREAM_DESTROY_ENDPOINTS_REQ);
    if (!adaptor_send_message(packed_con_id,
                              msg_id,
                              KIP_MSG_STREAM_DESTROY_ENDPOINTS_REQ_WORD_SIZE,
                              (ADAPTOR_DATA) &req))
    {
        stream_kip_destroy_endpoints_response_handler(packed_con_id,
                                                      STATUS_CMD_FAILED);
    }
}

#if !defined(COMMON_SHARED_HEAP)
/**
 * \brief Send a request to the remote core to set the state as
 *        activated with an existing metadata
 *
 * \param packed_con_id    - Packed send/receive connection ID
 * \param meta_channel_id  - The existing metadata data channel id
 *
 * \return TRUE if successfully sent response, FALSE if not
 */
static bool stream_kip_send_metadata_channel_activated_req(CONNECTION_LINK packed_con_id,
                                                           uint16 meta_channel_id)
{
    KIP_MSG_METADATA_CHANNEL_ACTIVATED_REQ req;
    ADAPTOR_MSGID msg_id;

    KIP_MSG_METADATA_CHANNEL_ACTIVATED_REQ_CHANNEL_ID_SET(&req, meta_channel_id);

    msg_id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_METADATA_CHANNEL_ACTIVATED_REQ);
    return adaptor_send_message(packed_con_id,
                                msg_id,
                                KIP_MSG_METADATA_CHANNEL_ACTIVATED_REQ_WORD_SIZE,
                                (ADAPTOR_DATA) &req);
}

/**
 * \brief Send a response back after setting the state as activated with
 *        an existing metadata data channel
 *
 * \param packed_con_id    - Packed send/receive connection ID
 * \param status           - Status
 * \param meta_channel_id  - The existing metadata data channel id
 *
 * \return TRUE if successfully sent response, FALSE if not
 */
static bool stream_kip_send_metadata_channel_activated_resp(CONNECTION_LINK packed_con_id,
                                                            STATUS_KYMERA status,
                                                            uint16 meta_channel_id)
{
    KIP_MSG_METADATA_CHANNEL_ACTIVATED_RES resp;
    ADAPTOR_MSGID msg_id;

    KIP_MSG_METADATA_CHANNEL_ACTIVATED_RES_STATUS_SET(&resp, status);
    KIP_MSG_METADATA_CHANNEL_ACTIVATED_RES_CHANNEL_ID_SET(&resp, meta_channel_id);

    msg_id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_METADATA_CHANNEL_ACTIVATED_RES);
    return adaptor_send_message(packed_con_id,
                                msg_id,
                                KIP_MSG_METADATA_CHANNEL_ACTIVATED_RES_WORD_SIZE,
                                (ADAPTOR_DATA) &resp);
}

/**
 * \brief Gets the metadata channel from an existing connection or creates
 *        and activates a new channel for metadata if no connection already
 *        exists.
 *
 * \param state       - The state information to be populated
 * \param proc_id     - The processor ID
 *
 * \return TRUE if successful, FALSE if not
 */
static bool stream_kip_get_metadata_channel(STREAM_KIP_CONNECT_INFO *state,
                                            PROC_ID_NUM proc_id)
{
    patch_fn_shared(stream_kip);

    /* Update the KIP endpoint with metadata information */
    stream_kip_set_ep_meta_info(state, TRUE);

    /* Find the existing meta data channel associated with the same port */
    STREAM_KIP_TRANSFORM_INFO *tr = stream_kip_get_created_transform(state->source_id, state->sink_id);

    L4_DBG_MSG("stream_kip_get_metadata_channel - Getting metadata channel");
    if (tr == NULL)
    {
        /* No existing connection, create and activate a channel */

        uint16 meta_channel_id = (state->meta_channel_id == 0) ?
                              META_DATA_CHANNEL_NUM : state->meta_channel_id;


        uint16 port_id = ipc_get_data_channelid_port(state->data_channel_id);

        if (IPC_SUCCESS != ipc_create_data_channel(port_id,
                                                   meta_channel_id,
                                                   IPC_DATA_CHANNEL_WRITE,
                                                   &state->meta_channel_id))
        {
            ipc_destroy_data_channel(state->meta_channel_id);
            return FALSE;
        }

        if (!stream_kip_activate_metadata_channel(state, proc_id))
        {
            /* Send a KIP request to destroy endpoints */
            stream_kip_destroy_endpoints(state->packed_con_id,
                                         STREAM_GET_SHADOW_EP_ID(state->source_id),
                                         STREAM_EP_ID_FROM_SHADOW_ID(state->sink_id),
                                         state);

            return FALSE;
        }
    }
    else
    {
        /* Some connection already exists, use the existing metadata channel */
        state->meta_channel_id = (state->data_channel_id) | META_DATA_CHANNEL_NUM;

        /*
         * Now we only need to set the state to activated for the metadata data
         * channel. Send a request to the remote core first in order to follow
         * the same sequence as a metadata data channel activation.
         */
        stream_kip_send_metadata_channel_activated_req(state->packed_con_id,
                                (uint16)ipc_invert_chanid_dir(state->meta_channel_id));
    }

    L4_DBG_MSG("stream_kip_get_metadata_channel - send metadata channel activation request");
    return TRUE;

}
#endif /* !COMMON_SHARED_HEAP */

/**
 * \brief Remove all remote transforms in the list
 *
 * \param count       - Number of transform ids in the list
 * \param tr_id_list  - The transform id list
 *
 * \return Number of remote-only transfers removed from the
 *         KIP transfer info list
 */
static unsigned stream_kip_remove_px_only_transforms(unsigned count,
                                                     TRANSFORM_ID *tr_id_list)
{
    unsigned i, j;

    patch_fn_shared(stream_kip);

    for (i = 0; i < count; i++)
    {
        TRANSFORM_INT_ID id ;
        STREAM_KIP_TRANSFORM_INFO *tr;

        id = STREAM_TRANSFORM_GET_INT_ID(tr_id_list[i]);
        tr = stream_kip_transform_info_from_id(id);
        if (tr != NULL)
        {
            unsigned ids[TOTAL_INDEX];

            /* Store the endpoint IDs so that we can tidy up ratematching
             * after the disconnect. */
            ids[SOURCE_INDEX] = tr->source_id;
            ids[SINK_INDEX] = tr->sink_id;
            /* Break if the transform is a KIP transform */
            if (STREAM_EP_IS_SHADOW_ID(ids[SOURCE_INDEX]) ||
                STREAM_EP_IS_SHADOW_ID(ids[SINK_INDEX]))
            {
                break;
            }

            /* remove from the transform list */
            stream_kip_remove_transform_info(tr);

            for (j = 0; j < TOTAL_INDEX; j++)
            {
                /* The external ID is stored in the transform but ratematching
                   code expects internal_id so convert it. */
                TOGGLE_EP_ID_BETWEEN_INT_AND_EXT(ids[j]);
                cease_ratematching(ids[j]);
            }

            set_system_event(SYS_EVENT_EP_DISCONNECT);
        }
    }

    return i;
}

/**
 * \brief Do a check on the callback response before calling the final
 *        callback. This function decides to continue next iteration
 *        of disconnect if all transforms are not disconnected.
 *
 * \param con_proc_id Connection id
 * \param status      Status
 * \param count       Total disconnected count
 *
 * \return  TRUE on success
 */
static bool stream_kip_disconnect_callback_handler(CONNECTION_LINK con_proc_id,
                                                   STATUS_KYMERA status,
                                                   unsigned count)
{
    bool loop = FALSE;
    TRANSFORM_ID *tr_list;

    patch_fn_shared(stream_kip);

    /* Context MUST not be NULL at this point. No validation required */
    STREAM_KIP_TRANSFORM_DISCONNECT_INFO *state = stream_kip_state_get_disconnect_info();

    if (status == STATUS_OK)
    {

        /* clean up remote only transforms from the list */
        if (state->remote_success_count > count)
        {
            unsigned px_tr_count;
            tr_list = state->tr_list + count;
            px_tr_count = stream_kip_remove_px_only_transforms(
                                  state->remote_success_count - count,
                                  tr_list);
            count += px_tr_count;
        }

        /* update the total success count */
        state->success_count = (uint16)count;

        if (state->count > count)
        {
            unsigned local_remain = 0;
            unsigned remaining = state->count - count;

            /* There are still more to disconnect */
            tr_list = &state->tr_list[count];

            /* If we have disconnected more remote ones, then start
             * with start with local ones
             */
            if (state->remote_success_count > count)
            {
                local_remain = state->remote_success_count - count;

                if (state->remote_success_count < state->count)
                {
                    /*
                     * Find the next boundary whether Px transforms starts. So we
                     * can disconnect local P0 chunk together with remaining KIP
                     * transforms
                     */
                    local_remain += stream_kip_find_px_transform_start (
                                    state->count - state->remote_success_count,
                                    &state->tr_list[state->remote_success_count]);
                }
            }
            else
            {
                local_remain = stream_kip_find_px_transform_start(remaining,
                                                                   tr_list);
            }

            if (local_remain > 0)
            {
                /* P0 list to process */
                stream_if_part_transform_disconnect(con_proc_id,
                                                     local_remain,
                                                     tr_list,
                                                     state->success_count,
                                            stream_kip_disconnect_callback_handler);
                loop = TRUE;
            }
            else
            {
                /* P1 list to process */
                 count = stream_kip_find_p0_only_transform_start(remaining, tr_list);

                 loop= stream_kip_send_transform_disconnect(
                                                  REVERSE_CONNECTION_ID(con_proc_id),
                                                  count,
                                                  tr_list);
            }
        }
    }
    else
    {
        if (state->remote_success_count > count)
        {
            /* We are in a irrecoverable situation where P1
             * KIP transforms are disconnected but P0 KIP
             * transforms are hanging around while disconnect
             * failed.
             */
             fault_diatribe(FAULT_AUDIO_STREAM_TRANSFORM_DISCONNNECT_ERROR,
                            state->remote_success_count);
        }
    }

    /* no more looping between P0 and P1. Exit now */
    if (!loop)
    {
        CONNECTION_LINK unpacked;

        unpacked = UNPACK_REVERSE_CONID(con_proc_id);
        if (state->disc_cb_flag)
        {
            state->callback.disc_cb(unpacked,
                                    status,
                                    state->tr_list[0],
                                    *((unsigned*)state->tr_list + 1));
        }
        else
        {
            state->callback.tr_disc_cb(unpacked,
                                       status,
                                       count);
        }

        /* Free the global context and unblock KIP */
        stream_kip_state_to_none(TRUE);
    }

    return TRUE;
}

/**
 * \brief Do a check on the callback response before calling the
 *        final callback. This function release the resources
 *        in case of error
 *
 * \param con_id       - The connection id
 * \param status       - Status
 * \param transform_id - Transform
 *
 * \return  TRUE on success
 */
static bool stream_kip_connect_callback_handler(CONNECTION_LINK con_id,
                                                STATUS_KYMERA status,
                                                TRANSFORM_ID transform_id)
{
    STREAM_KIP_CONNECT_INFO *state;

    /* This must be in connect state */
    state = stream_kip_state_get_connect_info();

    STREAM_KIP_ASSERT(state != NULL);

    if ((status != STATUS_OK) && STREAM_KIP_STATE_IN_CONNECT())
    {
        /* failed to establish stream connection at P0
         * Send a disconnect request
         */
        if (stream_kip_send_transform_disconnect(state->packed_con_id,
                                                 1,
                                                 &transform_id))
        {
            /* wait for the remote return */
            return TRUE;
        }
    }

    /* original accmd callback */
    state->callback(UNPACK_REVERSE_CONID(con_id), status, transform_id);

    /* Unblock KIP if it was blocked*/
    stream_kip_state_to_none(TRUE);

    return TRUE;
}

/**
 * \brief Internal function to handle the kip disconnect response. This
 *        function pointer will be provided as a callback function
 *        to streams to send transform disconnect response through KIP.
 *
 * \param con_id      - The connection id
 * \param status      - Status of the request
 * \param count       - The number of transforms got disconnected.
 *
 * \return TRUE on success
 */
static bool stream_kip_send_transform_disconnect_resp(CONNECTION_LINK con_id,
                                                      STATUS_KYMERA status,
                                                      unsigned count)
{
    KIP_MSG_STREAM_TRANSFORM_DISCONNECT_RES resp;
    ADAPTOR_MSGID msg_id;

    if ((count == 0) && (status == STATUS_OK))
    {
        status = STATUS_CMD_FAILED;
    }

    KIP_MSG_STREAM_TRANSFORM_DISCONNECT_RES_STATUS_SET(&resp, status);
    KIP_MSG_STREAM_TRANSFORM_DISCONNECT_RES_COUNT_SET(&resp, count);

    /* Free the global context and unblock KIP only
     * if it was in disconnect
     */
    if (STREAM_KIP_STATE_IN_DISCONNECT())
    {
        stream_kip_state_to_none(TRUE);
    }

    msg_id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_STREAM_TRANSFORM_DISCONNECT_RES);
    return adaptor_send_message(con_id,
                                msg_id,
                                KIP_MSG_STREAM_TRANSFORM_DISCONNECT_RES_WORD_SIZE,
                                (ADAPTOR_DATA) &resp);
}

/**
 * \brief Internal function to handle the KIP P0 transform list remove
 *        entry response. This function pointer will be provided as a
 *        callback function to streams to send KIP P0 transform list
 *        remove response through KIP. This function is only provided
 *        for P0 (to handle requests from Px).
 *
 * \param con_id      - The connection id
 * \param status      - Status of the request
 * \param count       - The number of transforms that were removed.
 *
 * \return TRUE on success
 */
static bool stream_kip_transform_list_remove_entry_resp(CONNECTION_LINK con_id,
                                                        STATUS_KYMERA status,
                                                        unsigned count)
{
    KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_RES resp;
    ADAPTOR_MSGID msg_id;

    if ((count == 0) && (status == STATUS_OK))
    {
        status = STATUS_CMD_FAILED;
    }

    KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_RES_STATUS_SET(&resp, status);
    KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_RES_COUNT_SET(&resp, count);

    /* Free the global context and unblock KIP only
     * if it was in disconnect
     */
    if (STREAM_KIP_STATE_IN_DISCONNECT())
    {
        stream_kip_state_to_none(TRUE);
    }

    msg_id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_TRANSFORM_LIST_REMOVE_ENTRY_RES);
    return adaptor_send_message(con_id,
                                msg_id,
                                KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_RES_WORD_SIZE,
                                (ADAPTOR_DATA) &resp);
}

/**
 * \brief Internal function to send the kip connect response. This
 *        function pointer will be provided as a callback function
 *        to streams to send stream connect response through KIP.
 *
 * \param con_id       - The connection id
 * \param status       - Status of the request
 * \param transform_id - Transform
 *
 * \return TRUE on success
 */
static bool stream_kip_send_connect_resp(CONNECTION_LINK con_id,
                                         STATUS_KYMERA status,
                                         TRANSFORM_ID transform_id)
{
    KIP_MSG_STREAM_CONNECT_RES resp;
    STREAM_KIP_TRANSFORM_INFO *tr;
    TRANSFORM_INT_ID id;
    ADAPTOR_MSGID msg_id;

    patch_fn_shared(stream_kip);

    if (status != STATUS_OK)
    {
        STREAM_CONNECT_FAULT(SC_KIP_CONNECTION_FAILED,
                             "stream_kip_send_connect_resp");
    }

    id = STREAM_TRANSFORM_GET_INT_ID(transform_id);
    tr = stream_kip_transform_info_from_id(id);
    if (tr != NULL)
    {
        if (status != STATUS_OK)
        {
            stream_kip_remove_transform_info(tr);
        }
        else
#if defined(COMMON_SHARED_HEAP)
        {
            tr->enabled = TRUE;
        }
#else
        {
            if (STREAM_EP_IS_SHADOW_ID(tr->sink_id))
            {

                endpoint_shadow_state *state;
                tCbuffer *shared_buffer;
                ENDPOINT *shadow_ep = stream_endpoint_from_extern_id(tr->sink_id);

                /* If connecting to a shadow endpoint */
                state = &shadow_ep->state.shadow;

                /* propagate potential usable_octets TO the shared_buffer */
                shared_buffer = ipc_data_channel_get_cbuffer(state->channel_id);
                cbuffer_set_usable_octets(shared_buffer,
                                    cbuffer_get_usable_octets(state->buffer));

                tr->enabled = TRUE;
            }
            else if (STREAM_EP_IS_SHADOW_ID(tr->source_id))
            {
                tr->enabled = TRUE;
            }
        }
#endif /* COMMON_SHARED_HEAP */
    }

    KIP_MSG_STREAM_CONNECT_RES_STATUS_SET(&resp, status);
    KIP_MSG_STREAM_CONNECT_RES_TRANSFORM_ID_SET(&resp, transform_id);

    /* Add to KIP transform list */
    if (STREAM_KIP_STATE_IN_CONNECT())
    {
        stream_kip_state_to_none(TRUE);
    }

    msg_id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_STREAM_CONNECT_RES);
    return adaptor_send_message(con_id,
                                msg_id,
                                KIP_MSG_STREAM_CONNECT_RES_WORD_SIZE,
                                (ADAPTOR_DATA) &resp);
}

/**
 * \brief Internal function to handle the kip create endpoint response
 *
 * \param packed_con_id The connection id
 * \param status        Status of the request
 *
 * \return TRUE on success
 */
static bool stream_kip_send_create_endpoints_resp(CONNECTION_LINK packed_con_id,
                                                  STATUS_KYMERA status)
{
    KIP_MSG_STREAM_CREATE_ENDPOINTS_RES resp;
    ADAPTOR_MSGID msg_id;

    patch_fn_shared(stream_kip);

    if (status != STATUS_OK)
    {
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_STATUS_SET(&resp, status);
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_CHANNEL_ID_SET(&resp, 0);
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_BUFFER_SIZE_SET(&resp, 0);
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_FLAGS_SET(&resp, 0);
    }
    else
    {
        STREAM_KIP_CONNECT_INFO *state;
        STREAM_TRANSFORM_BUFFER_INFO *buffer_info;
        BUFFER_DETAILS buffer_details;
        unsigned buffer_size;

        state = stream_kip_state_get_connect_info();

        STREAM_KIP_ASSERT(state != NULL);

        buffer_info= &(state->connect_info.buffer_info);
        /* Get the local endpoint buffer details to retrieve the size */
        stream_kip_get_local_ep_buffer_details(state, &buffer_details);
        buffer_size = get_buf_size(&buffer_details);

        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_STATUS_SET(&resp, status);
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_CHANNEL_ID_SET(&resp,
                                                           state->data_channel_id);
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_BUFFER_SIZE_SET(&resp,
                                                            buffer_size);
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_FLAGS_SET(&resp,
                                                      (*((uint16*)&buffer_info->flags)));
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_DATA_FORMAT_SET(&resp,
                                                      ((uint32)state->data_format));
    }

    /* Now send the message */
    msg_id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_STREAM_CREATE_ENDPOINTS_RES);
    return adaptor_send_message(packed_con_id,
                                msg_id,
                                KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_WORD_SIZE,
                                (ADAPTOR_DATA) &resp);
}

/**
 * \brief Add shadow endpoint from the passed state to an existing chain of
 *        synced shadow endpoints, that are connected to the same operator.
 *        If the connection is mono or if it's the first terminal being
 *        connected, the head_of_sync and nep_in_sync remain unchanged.
 *
 * \param state_info The state of the connection
 */
static void stream_kip_add_kip_ep_to_sync_list(STREAM_KIP_CONNECT_INFO* state_info)
{
    ENDPOINT *ep = NULL;
    ENDPOINT *head_of_sync_ep = NULL;

    patch_fn_shared(stream_kip);

    ep = stream_kip_get_kip_endpoint_from_state(state_info);

    if (ep != NULL)
    {
        head_of_sync_ep = stream_kip_get_head_of_sync_endpoint(state_info->data_channel_id, ep);
    }

    if (head_of_sync_ep == NULL)
    {
        /* This means it's either a mono case or it's the first terminal,
         * so just return TRUE */
        return;
    }

    ENDPOINT **p_ep;
    /* Iterate until the end. */
    for(p_ep = &head_of_sync_ep; *p_ep != NULL; p_ep = &((*p_ep)->state.shadow.nep_in_sync));

    /* Populate the nep_in_sync of the last endpoint found in the sync list
     * and populate the head_of_sync field with the head of sync of the list. */
    *p_ep = ep;
    ep->state.shadow.head_of_sync = head_of_sync_ep;
}

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Get the (external) transform id associated with a given endpoint, if any.
 *
 * \param endpoint [IN]  - The KIP endpoint
 * \param tr_id    [OUT] - The (external) transform id
 *
 * \return TRUE if a valid transform (id) was found, FALSE if not.
 */
bool stream_transform_id_from_endpoint(ENDPOINT *endpoint, TRANSFORM_ID *tr_id)
{
    TRANSFORM *tr = stream_transform_from_endpoint(endpoint);

    if (tr == NULL)
    {
        return FALSE;
    }

    *tr_id = STREAM_TRANSFORM_GET_EXT_ID(tr->id);

    return TRUE;
}

/**
 * \brief Request to P0 to remove entry from kip_transform_list
 *        (where P0 keeps a copy/entry/id of each transform on Px).
 *
 * \param tr_id       - The external transform id
 * \param proc_id     - The processor ID
 *
 * \return            TRUE if successful
 */
bool stream_kip_cleanup_endpoint_transform(TRANSFORM_ID tr_id,
                                           PROC_ID_NUM proc_id)
{
    CONNECTION_LINK con_id;

    con_id = PACK_CONID_PROCID(PROC_PROCESSOR_0, proc_id);

    return stream_kip_transform_list_remove_entry(con_id, 1, &tr_id);
}

/**
 * \brief Disconnect the transform associated with a KIP endpoint.
 *
 * \param endpoint The KIP endpoint.
 * \param proc_id  The processor_id.
 *
 * \return TRUE if success, FALSE otherwise
 */
bool stream_kip_disconnect_endpoint(ENDPOINT *endpoint, PROC_ID_NUM proc_id)
{
    TRANSFORM *tr;
    TRANSFORM_ID tr_id;
    CONNECTION_LINK con_id;

    tr = stream_transform_from_endpoint(endpoint);
    if (tr == NULL)
    {
        return FALSE;
    }

    if (PROC_PRIMARY_CONTEXT())
    {
        stream_kip_state_to_disconnect_endpoint();
    }

    tr_id  = STREAM_TRANSFORM_GET_EXT_ID(tr->id);
    con_id = PACK_CONID_PROCID(PROC_PROCESSOR_0, proc_id);

    return stream_kip_send_transform_disconnect(con_id, 1, &tr_id);
}

/**
 * \brief Create a connect info record during connection state and partially
 *        initialise it.
 *
 * \param con_id                - The packed connection id
 * \param source_id             - The source id at the local side
 * \param sink_id               - The sink id at the  local side
 * \param ep_location           - Location of endpoints
 * \param block_size            - Timing info 'block_size' of shadowed operator
 * \param callback              - The callback to be called after handling the
 *                                response
 * \param data_format           - The data format of the endpoint connected to
 *                                shadow
 * \param sync_shadow_eps       - Parameter to tell the other side if the
 *                                shadow endpoints should be synced.
 *
 * \return  A connect information record or NULL
 */
STREAM_KIP_CONNECT_INFO *stream_kip_create_connect_info_record_ex(CONNECTION_LINK con_id,
                                                                  unsigned source_id,
                                                                  unsigned sink_id,
                                                                  STREAM_EP_LOCATION ep_location,
                                                                  STREAM_TRANSFORM_CBACK callback,
                                                                  AUDIO_DATA_FORMAT data_format,
                                                                  bool sync_shadow_eps)
{
    STREAM_KIP_CONNECT_INFO *ep_connect_info;

    ep_connect_info = xzpnew(STREAM_KIP_CONNECT_INFO);
    if (ep_connect_info == NULL)
    {
        return NULL;
    }

    ep_connect_info->packed_con_id      = (uint16)con_id;
    ep_connect_info->source_id          = (uint16)source_id;
    ep_connect_info->sink_id            = (uint16)sink_id;
    ep_connect_info->ep_location        = ep_location;
    ep_connect_info->callback           = callback;
    ep_connect_info->data_format        = data_format;
    ep_connect_info->sync_shadow_eps    = sync_shadow_eps;

    return ep_connect_info;
}

/**
 * \brief Create a connect info record during connection state and partially
 *        initialise it. This is different from the above in that parameters
 *        'block_size' and 'period' are replaced by default values for a
 *        shadow endpoint. This is useful at the start of the shadow
 *        endpoint connection setup sequence.
 *
 * \param con_id                - The packed connection id
 * \param source_id             - The source id at the local side
 * \param sink_id               - The sink id at the  local side
 * \param ep_location           - Location of endpoints
 * \param callback              - The callback to be called after handling the
 *                                response
 * \param sync_shadow_eps       - Parameter to tell the other side if the
 *                                shadow endpoints should be synced
 *
 * \return  A connection information record or NULL
 */
STREAM_KIP_CONNECT_INFO *stream_kip_create_connect_info_record(CONNECTION_LINK con_id,
                                                               unsigned source_id,
                                                               unsigned sink_id,
                                                               STREAM_EP_LOCATION ep_location,
                                                               STREAM_TRANSFORM_CBACK callback,
                                                               bool sync_shadow_eps)
{
    return stream_kip_create_connect_info_record_ex(con_id, source_id, sink_id,
               ep_location, callback, AUDIO_DATA_FORMAT_16_BIT, sync_shadow_eps);
}

/**
 * \brief Get KIP transform info from KIP transform information list
 *
 * \param id          - KIP transform info id
 *
 * \return            Pointer to KIP transform info object
 */
STREAM_KIP_TRANSFORM_INFO* stream_kip_transform_info_from_id(TRANSFORM_INT_ID id)
{
    STREAM_KIP_TRANSFORM_INFO *tr = kip_transform_list;

    while ((tr != NULL) && (tr->id != id))
    {
        tr = tr->next;
    }

    return tr;
}

/**
 * \brief Get KIP transform info from KIP transform information list
 *
 * \param epid        - The ID of the endpoint that is known
 *
 * \return            Pointer to KIP transform info object
 */
STREAM_KIP_TRANSFORM_INFO* stream_kip_transform_info_from_epid(unsigned epid)
{
    STREAM_KIP_TRANSFORM_INFO *tr = kip_transform_list;

    while ((tr != NULL) && (tr->source_id != epid) && (tr->sink_id != epid))
    {
        tr = tr->next;
    }

    return tr;
}

/**
 * \brief Helper function to find the ID of a remote endpoint
 *        connected to a known endpoint.
 *
 * \param epid        - The ID of the endpoint that is known
 *
 * \return The ID of the endpoint connected to endpoint with ID epid.
 *         0 if not found.
 */
unsigned stream_kip_connected_to_epid(unsigned epid)
{
    STREAM_KIP_TRANSFORM_INFO *tr = stream_kip_transform_info_from_epid(epid);

    if ((epid & STREAM_EP_SINK_BIT) == STREAM_EP_SINK_BIT)
    {
        return tr->source_id;
    }
    else
    {
        return tr->sink_id;
    }
}

/**
 * \brief Helper function to create new remote transform info entry
 *        and add it to the respective list.
 *
 * \param id           Internal transform id
 * \param processor_id Remote processor id
 * \param source_id    Remote source id
 * \param sink_id      Remote sink id
 * \param data_chan_id Data channel id
 *
 * \return Pointer to KIP transform info object
 */
STREAM_KIP_TRANSFORM_INFO* stream_kip_add_transform_info(TRANSFORM_INT_ID id,
                                                         PROC_ID_NUM processor_id,
                                                         unsigned source_id,
                                                         unsigned sink_id,
                                                         uint16 data_chan_id)
{
    STREAM_KIP_TRANSFORM_INFO *tr_info;

    tr_info = xpnew(STREAM_KIP_TRANSFORM_INFO);
    if (tr_info != NULL)
    {
        tr_info->next = kip_transform_list;
        kip_transform_list = tr_info;

        tr_info->id = id;
        tr_info->processor_id = proc_serialize(processor_id);
        tr_info->data_channel_id = data_chan_id;
        tr_info->source_id = source_id;
        tr_info->sink_id = sink_id;
        tr_info->enabled = FALSE;
    }

    return tr_info;
}

/**
 * \brief Helper function to remove remote transform info
 *        entry from respective list
 *
 * \param tr_id Internal transform id of transform to be removed
 */
void stream_kip_remove_transform_info_by_id(TRANSFORM_INT_ID tr_id)
{
    STREAM_KIP_TRANSFORM_INFO **tr_p = &kip_transform_list;
    STREAM_KIP_TRANSFORM_INFO *tr;

    while ((tr = *tr_p) != NULL)
    {
        /* Is this the one we're looking for? */
        if (tr->id == tr_id)
        {
            /* Remove entry from list and free it */
            *tr_p = tr->next;
            pfree(tr);

            return;
        }

        tr_p = &tr->next;
    }
}

/**
 * \brief Helper function to remove remote transform info
 *        entry from respective list
 *
 * \param transform - Transform to be removed
 */
void stream_kip_remove_transform_info(STREAM_KIP_TRANSFORM_INFO* transform)
{
    STREAM_KIP_TRANSFORM_INFO *tr, **tr_p = &kip_transform_list;

    while ((tr = *tr_p) != NULL)
    {
        /* Is this the one we're looking for? */
        if (tr == transform)
        {
            /* Remove entry from list and free it */
            *tr_p = tr->next;
            pfree(tr);

            return;
        }

        tr_p = &tr->next;
    }
}

/**
 * \brief Helper function to retrieve entry in remote transform
 *        info list based on data channel ID.
 *
 * \param data_chan_id - data channel id
 *
 * \return             Pointer to KIP transform info object
 */
STREAM_KIP_TRANSFORM_INFO* stream_kip_transform_info_from_chanid(uint16 data_chan_id)
{
    STREAM_KIP_TRANSFORM_INFO *tr = kip_transform_list;

    while ((tr != NULL) && (tr->data_channel_id != data_chan_id))
    {
        tr = tr->next;
    }

    return tr;
}

/**
 * \brief Create disconnect info record
 *
 * \param con_id      - The connection id
 * \param count       - Number of transforms in the list
 * \param ep_disc_cb  - Flag as to which callback to call
 *                      (state->callback.disc_cb or .tr_disc_cb)
 * \param transforms  - The transform list
 * \param callback    - The callback
 *
 * \return            Pointer to an allocated disconnect info object
 */
STREAM_KIP_TRANSFORM_DISCONNECT_INFO *stream_kip_create_disconnect_info_record(CONNECTION_LINK con_id,
                                                                               unsigned count,
                                                                               bool ep_disc_cb,
                                                                               TRANSFORM_ID *transforms,
                                                                               STREAM_KIP_TRANSFORM_DISCONNECT_CB callback)
{
    STREAM_KIP_TRANSFORM_DISCONNECT_INFO *ep_disconnect_info;

    ep_disconnect_info = xpmalloc(sizeof(STREAM_KIP_TRANSFORM_DISCONNECT_INFO) +
                                  (count * sizeof(TRANSFORM_ID)));
    if (ep_disconnect_info == NULL)
    {
        return NULL;
    }

    ep_disconnect_info->packed_con_id = (uint16)con_id;
    ep_disconnect_info->count = (uint16)count;
    ep_disconnect_info->disc_cb_flag = ep_disc_cb;
    if ((count == 2) &&
        ((transforms[0] == 0) || (transforms[0] == transforms[1])))
    {
        /* skip it by marking it as success */
        ep_disconnect_info->success_count = 1;
    }
    else
    {
        ep_disconnect_info->success_count = 0;
    }
    ep_disconnect_info->remote_success_count = 0;
    ep_disconnect_info->callback = callback;

    memcpy(&ep_disconnect_info->tr_list[0], transforms, count * sizeof(TRANSFORM_ID));

    return ep_disconnect_info;
}

/**
 * \brief Find the first Px transform offset from the kip_transform_list
 *
 * \param count       - The total number of transforms in the list
 * \param tr_id_list  - The transform list
 *
 * \return            The first Px transform offset from the list
 */
unsigned stream_kip_find_px_transform_start(unsigned count, TRANSFORM_ID *tr_id_list)
{
    unsigned i;

    for (i = 0; i < count; i++)
    {
        TRANSFORM_INT_ID id;

        id = STREAM_TRANSFORM_GET_INT_ID(tr_id_list[i]);
        if (stream_kip_transform_info_from_id(id) != NULL)
        {
            break;
        }
    }

    return i;
}

/**
 * \brief Handling the incoming destroy endpoint response.
 *
 * \param con_id The connection id
 * \param status Status
 */
void stream_kip_destroy_endpoints_response_handler(CONNECTION_LINK con_id,
                                                   STATUS_KYMERA status)
{
    if (status != STATUS_OK)
    {
        /* P1 may end up in a wrong state */
        fault_diatribe(FAULT_AUDIO_STREAM_ENDPOINT_DESTROY_ERROR , 0);
    }

    if (STREAM_KIP_STATE_IN_CONNECT())
    {
        STREAM_KIP_CONNECT_INFO *state;

        state = stream_kip_state_get_connect_info();
        /* attempt to destroy the endpoints if exists */
        stream_kip_destroy_endpoint_ids(state);

        state->callback(UNPACK_REVERSE_CONID(con_id), STATUS_CMD_FAILED, 0);
        stream_kip_state_to_none(TRUE);
    }
}

/**
 * \brief Handling the incoming create endpoint response.
 *
 * \param con_id      The connection id
 * \param status      Status
 * \param channel_id  The data channel id. This must not be 0.
 * \param buffer_size Negotiated buffer size for the connection
 * \param flags       The buffer related flags
 * \param state       The connection state information.
 * \param data_format The data format of the endpoint connected to shadow on the
 *                    secondary core.
 */
void stream_kip_create_endpoints_response_handler(CONNECTION_LINK con_id,
                                                  STATUS_KYMERA status,
                                                  unsigned channel_id,
                                                  unsigned buffer_size,
                                                  unsigned flags,
                                                  unsigned data_format)
{
    STREAM_KIP_CONNECT_INFO *state;

    patch_fn_shared(stream_kip);

    STREAM_KIP_ASSERT(STREAM_KIP_STATE_IN_CONNECT());
    state = stream_kip_state_get_connect_info();

    if ((status != STATUS_OK) ||
        (channel_id == 0) || (ipc_get_data_channelid_port(state->data_channel_id) != 0 &&
        (ipc_invert_chanid_dir(channel_id) != state->data_channel_id)))
    {
        stream_kip_destroy_endpoint_ids(state);

        state->callback(UNPACK_REVERSE_CONID(con_id), STATUS_CMD_FAILED, 0);
        stream_kip_state_to_none(TRUE);
    }
    else
    {
        bool success;

        if (state->ep_location == STREAM_EP_REMOTE_SOURCE)
        {

            /* update the state buffer details. */
            stream_kip_update_buffer_info(state,
                                          ipc_invert_chanid_dir(channel_id),
                                          buffer_size, flags);

            success = stream_kip_connect_endpoints(state->packed_con_id,
                            STREAM_EP_ID_FROM_SHADOW_ID(state->source_id),
                            STREAM_GET_SHADOW_EP_ID(state->sink_id),
                            state);
        }
        else
        {
            /* Update the data format with the data format of the sink operator,
             * in the state and in the shadow EP. */
            stream_kip_set_data_format_to_state(state, (AUDIO_DATA_FORMAT)data_format);
            stream_kip_set_data_format_for_shadow_ep(state);
            /* update the state buffer details. no channel id changes */
            stream_kip_update_buffer_info(state, state->data_channel_id,
                                          buffer_size, flags);

            PROC_ID_NUM proc_id = GET_RECV_PROC_ID(state->packed_con_id);
            success = kip_get_buffer_and_activate_channels(state, proc_id);
        }

        if (!success)
        {
            STREAM_CONNECT_FAULT(SC_KIP_GET_DATA_CHANNEL_FAILED,
                                 "Failed to get_data_channel");

            /* Send a KIP request to destroy endpoints */
            stream_kip_destroy_endpoints(state->packed_con_id,
                            STREAM_GET_SHADOW_EP_ID(state->source_id),
                            STREAM_EP_ID_FROM_SHADOW_ID(state->sink_id),
                            state);
        }
    }
}

/**
 * \brief Handle the transform disconnect response from the remote
 *
 * \param con_id     - The connection id
 * \param status     - Status of the request
 * \param count      - The number of disconnected transforms
 * \param state      - The disconnect state
 */
void stream_kip_transform_disconnect_response_handler(CONNECTION_LINK con_id,
                                                      STATUS_KYMERA status,
                                                      unsigned count)
{
    patch_fn_shared(stream_kip);

    if (STREAM_KIP_STATE_IN_CONNECT())
    {
        /* This is a disconnect due to a connection failure */
        stream_kip_connect_callback_handler(con_id, status, 0);
    }
    else if (STREAM_KIP_STATE_IN_DISCONNECT())
    {
        STREAM_KIP_TRANSFORM_DISCONNECT_INFO *state;

        state = stream_kip_state_get_disconnect_info();
        state->remote_success_count = (uint16)(state->success_count + count);

        stream_kip_disconnect_callback_handler(con_id, status,
                                               state->success_count);
    }
    else if (STREAM_KIP_STATE_IN_DISCONNECT_ENDPOINT())
    {
        stream_kip_state_to_none(FALSE);
        /*
         * Ignore the response. This happens while cleaning
         * up transforms during endpoint close especially
         * destroying operators.
         */
    }
    else if (STREAM_KIP_STATE_IN_IDLE())
    {
        /*
         * Ignore the response. Might happen after disconnect.
         */
    }
    else
    {
        /* This case should never be reached. */
        STREAM_KIP_ASSERT(FALSE);
    }

    return;
}

/**
 * \brief Handle the connect resp from the secondary core
 *
 * \param con_id       - The connection id
 * \param status       - Status of the request
 * \param transform_id - Transform id returned
 */
void stream_kip_connect_response_handler(CONNECTION_LINK con_id,
                                         STATUS_KYMERA status,
                                         TRANSFORM_ID transform_id)
{
    STREAM_KIP_CONNECT_INFO *state;
    CONNECTION_LINK unpacked_reversed_conid;

    patch_fn_shared(stream_kip);

    STREAM_KIP_ASSERT(STREAM_KIP_STATE_IN_CONNECT());
    state = stream_kip_state_get_connect_info();
    unpacked_reversed_conid = UNPACK_REVERSE_CONID(con_id);

    if (status == STATUS_OK)
    {
        /* The external ID is stored in the transform but ratematching code
         * expects internal_id so convert it */
        unsigned source_id = state->source_id;
        TOGGLE_EP_ID_BETWEEN_INT_AND_EXT(source_id);
        if (!cease_ratematching(source_id))
        {
            status = STATUS_CMD_FAILED;
        }
        else
        if (STREAM_TRANSFORM_GET_INT_ID(transform_id) != state->tr_id)
        {
            /* Unexpected. P1 is not playing by the rules! */
            fault_diatribe(FAULT_AUDIO_MULTICORE_CONNECT_INVALID_STATE, transform_id);

            status = STATUS_CMD_FAILED;
        }
    }

    if (status != STATUS_OK)
    {
        STREAM_CONNECT_FAULT(SC_KIP_CONNECTION_FAILED,
                             "stream_kip_connect_response_handler");
#if !defined(COMMON_SHARED_HEAP)
        /* Deactivating the data channel as well remove the transform info */
        if (state->meta_channel_id != 0)
        {
            bool data_channel_status;
            data_channel_status = stream_kip_data_channel_deactivate_ipc(state->meta_channel_id);
            STREAM_KIP_ASSERT(data_channel_status);
        }
#endif /* COMMON_SHARED_HEAP */
        if (!(stream_kip_data_channel_deactivate(state->data_channel_id)))
        {
            /* remove the transform if deactivation fails */
            stream_kip_remove_transform_info_by_id(transform_id);
        }

        stream_destroy_endpoint_id(state->source_id);
        stream_destroy_endpoint_id(state->sink_id);
    }
    else
    {
        /*  update the transform info list with status */
        STREAM_KIP_TRANSFORM_INFO* kip_tr;

        /* get the preserved remote transform info */
        kip_tr = stream_kip_transform_info_from_id(state->tr_id);

        /* activate the transform */
        kip_tr->enabled = TRUE;

        /* Mark whether the source or sink endpoint is a real endpoint at P0 */
        kip_tr->real_source_ep = STREAM_EP_IS_REALEP_ID(state->source_id);
        kip_tr->real_sink_ep = STREAM_EP_IS_REALEP_ID(state->sink_id);

        STREAM_KIP_ASSERT(kip_tr != NULL);

        if (state->ep_location != STREAM_EP_REMOTE_ALL)
        {
            /* The endpoints must be synced before connecting the transform
             * because buff_metadata_connect_existing() relies heavily on
             * the shadow endpoints being synchronised correctly. */
            if (state->sync_shadow_eps)
            {
                stream_kip_add_kip_ep_to_sync_list(state);
            }

            stream_if_transform_connect(unpacked_reversed_conid,
                                        state->source_id,
                                        state->sink_id,
                                        transform_id, &state->connect_info,
                                        stream_kip_connect_callback_handler);
#if !defined(COMMON_SHARED_HEAP)
            /* propagate potential usable_octets ... */
            if (STREAM_EP_IS_SHADOW_ID(state->sink_id) ||
                STREAM_EP_IS_SHADOW_ID(state->source_id))
            {
                ENDPOINT *source_ep = stream_endpoint_from_extern_id(state->source_id);
                ENDPOINT *sink_ep = stream_endpoint_from_extern_id(state->sink_id);
                ENDPOINT *op_term;
                endpoint_shadow_state *shadow_state;
                tCbuffer *shared_buffer;
                KIP_MSG_STREAM_CONNECT_REQ req;
                EXT_OP_ID ext_opid;
                ADAPTOR_MSGID msg_id;

                if (STREAM_EP_IS_SHADOW_ID(state->sink_id))
                {
                    /* ... to shared_buffer */
                    op_term = source_ep;
                    shadow_state = &sink_ep->state.shadow;
                    shared_buffer = ipc_data_channel_get_cbuffer(shadow_state->channel_id);
                    STREAM_KIP_ASSERT(shared_buffer != NULL);
                    cbuffer_set_usable_octets(shared_buffer,
                             cbuffer_get_usable_octets(shadow_state->buffer));
                }
                else
                {
                    /* ... from shared_buffer */
                    op_term = sink_ep;
                    shadow_state = &source_ep->state.shadow;
                    shared_buffer = ipc_data_channel_get_cbuffer(shadow_state->channel_id);
                    STREAM_KIP_ASSERT(shared_buffer != NULL);
                    cbuffer_set_usable_octets(shadow_state->buffer,
                             cbuffer_get_usable_octets(shared_buffer));
                }

                /* send kip stream_connect_confirm_req message for similar action on second core */
                ext_opid = stream_external_id_from_endpoint(op_term);
                KIP_MSG_STREAM_CONNECT_CONFIRM_REQ_CONN_TO_SET(&req, ext_opid);
                msg_id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_STREAM_CONNECT_CONFIRM_REQ);
                adaptor_send_message(unpacked_reversed_conid,
                                     msg_id,
                                     KIP_MSG_STREAM_CONNECT_CONFIRM_REQ_WORD_SIZE,
                                     (ADAPTOR_DATA) &req);
            }
#endif /* !COMMON_SHARED_HEAP */
            return;
        }
    }

    /* orginal accmd callback */
    state->callback(unpacked_reversed_conid, status, transform_id);

    if (STREAM_KIP_STATE_IN_CONNECT())
    {
        /* Unblock KIP if it was blocked*/
        stream_kip_state_to_none(TRUE);
    }
    else
    {
        /*
         * Stream connection of 2 remote endpoints are forwarded
         * by P0 without blocking KIP using the CONNECT state.
         * In that case, free the context.
         */
        pfree(state);
    }

    set_system_event(SYS_EVENT_EP_CONNECT);

    return;
}

/**
 * \brief Create local endpoints and send a remote request to create
 *        endpoints at secondary core.
 *
 * \param packed_con_id    The packed connection id
 * \param remote_source_id The source endpoint id
 * \param remote_sink_id   The sink endpoint id
 * \param state            The connect state info
 *
 * \return TRUE on success
 */
bool stream_kip_create_endpoints(CONNECTION_LINK packed_con_id,
                                 unsigned remote_source_id,
                                 unsigned remote_sink_id,
                                 STREAM_KIP_CONNECT_INFO *state)
{
    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ req;
    STREAM_TRANSFORM_BUFFER_INFO *buffer_info;
    unsigned data_channel_id;
    ADAPTOR_MSGID msg_id;

    patch_fn_shared(stream_kip);

    /* If KIP is busy, return failure */
    if (!STREAM_KIP_STATE_IN_IDLE())
    {
        return FALSE;
    }

    /* Create endpoints locally and get the buffer details */
    if (!stream_kip_create_eps_for_connect(packed_con_id, state))
    {
        return FALSE;
    }

    /* Get the endpoints buffer details and it is not expected to fail*/
    stream_kip_ep_get_buffer_info(state);

    buffer_info = &(state->connect_info.buffer_info);
    stream_kip_set_ep_meta_info(state, buffer_info->flags.supports_metadata);

    data_channel_id = (ipc_get_data_channelid_port(state->data_channel_id) == 0) ?
                       state->data_channel_id :
                       ipc_invert_chanid_dir(state->data_channel_id);

    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_SOURCE_ID_SET(&req, remote_source_id);
    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_SINK_ID_SET(&req, remote_sink_id);
    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_CHANNEL_ID_SET(&req, data_channel_id);
    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_BUFFER_SIZE_SET(&req, buffer_info->buffer_size);
    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_FLAGS_SET(&req, *((uint16*)&(buffer_info->flags)));
    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_DATA_FORMAT_SET(&req, ((uint32)state->data_format));
    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_SYNC_SHADOW_EPS_SET(&req,
                                        ((uint16)state->sync_shadow_eps));

    /* Now send the message */
    msg_id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_STREAM_CREATE_ENDPOINTS_REQ);
    if (!adaptor_send_message(packed_con_id,
                              msg_id,
                              KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_WORD_SIZE,
                              (ADAPTOR_DATA) &req))
    {
        stream_kip_destroy_endpoint_ids(state);

        /* state info will be free'd by the caller in case of failure*/
        stream_kip_state_to_none(FALSE);
        return FALSE;
    }

    return TRUE;
}

/**
 * \brief  Send a KIP stream disconnect
 *
 * \param state        Disconnect state.
 * \param px_tr_offset Offset to start processing the transform list
 *                     of the state for the secondary core.
 *
 * \return  TRUE on success
 */
bool stream_kip_transform_disconnect(STREAM_KIP_TRANSFORM_DISCONNECT_INFO *state,
                                     unsigned px_tr_offset)
{
    bool result = TRUE;

    patch_fn_shared(stream_kip);

    /*
     * Avoid any other stream operations over KIP
     * while disconnect is in progress.
     */
    stream_kip_state_to_disconnect(state);

    if ((px_tr_offset > 0) && (state->success_count < px_tr_offset))
    {
        /* start with disconnecting local transforms */
        stream_if_part_transform_disconnect(
                          REVERSE_CONNECTION_ID(state->packed_con_id),
                                              px_tr_offset,
                                              state->tr_list,
                                              state->success_count,
                                         stream_kip_disconnect_callback_handler);
    }
    else
    {
        unsigned count;

        /* find the offset to next P0 only transform */
        count = stream_kip_find_p0_only_transform_start((state->count -
                                                         state->success_count),
                                                         state->tr_list);

        /* Request to send remote transform disconnect */
        result = stream_kip_send_transform_disconnect(state->packed_con_id,
                                                      count,
                                                      (state->tr_list +
                                                      state->success_count));
    }

    return result;
}

/**
 * \brief Generate a transform id and send a KIP stream connect request
 *        Sends a KIP connect REQ to secondary core(s) - only used on P0
 *
 * \param packed_con_id     The packed connection id
 * \param remote_source_id  The source endpoint id
 * \param remote_sink_id    The sink endpoint id
 * \param state             The connect state info
 *
 * \return TRUE on success
 */
bool stream_kip_connect_endpoints(CONNECTION_LINK packed_con_id,
                                  unsigned remote_source_id,
                                  unsigned remote_sink_id,
                                  STREAM_KIP_CONNECT_INFO *state)
{
    KIP_MSG_STREAM_CONNECT_REQ req;
    TRANSFORM_INT_ID id;
    unsigned data_channel_id;
    ADAPTOR_MSGID msg_id;
    bool transitioned;

    patch_fn_shared(stream_kip);

    /* Generate a transform id */
    id = stream_get_next_transform_id();

    /* add it in the remote transform list */
    if (stream_kip_add_transform_info(id, GET_RECV_PROC_ID(packed_con_id),
                                      remote_source_id, remote_sink_id,
                                      state->data_channel_id) == NULL)
    {
        return FALSE;
    }

    transitioned = FALSE;
    if (STREAM_KIP_STATE_IN_IDLE())
    {
        stream_kip_state_to_connect(state);
        transitioned = TRUE;
    }

    /* update the connect state info with the transform id */
    state->tr_id = (uint16)id;

    data_channel_id = (state->data_channel_id == 0) ?
                      0 : ipc_invert_chanid_dir(state->data_channel_id);

    /* pack the kip stream connect request */
    KIP_MSG_STREAM_CONNECT_REQ_SOURCE_ID_SET(&req, remote_source_id);
    KIP_MSG_STREAM_CONNECT_REQ_SINK_ID_SET(&req, remote_sink_id);
    KIP_MSG_STREAM_CONNECT_REQ_TRANSFORM_ID_SET(&req,
                                                STREAM_TRANSFORM_GET_EXT_ID(id));
    KIP_MSG_STREAM_CONNECT_REQ_CHANNEL_ID_SET(&req, data_channel_id);

    /* Now send the message */

    msg_id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_STREAM_CONNECT_REQ);
    if (!adaptor_send_message(packed_con_id,
                              msg_id,
                              KIP_MSG_STREAM_CONNECT_REQ_WORD_SIZE,
                              (ADAPTOR_DATA) &req))
    {
        stream_kip_remove_transform_info_by_id(id);
        if (transitioned)
        {
            stream_kip_state_to_none(FALSE);
        }
        return FALSE;
    }

    return TRUE;
}

/****************************************************************************
 *  Audio second core section
 ****************************************************************************/

/**
 * \brief Handling the incoming stream connect request from P0
 *
 * \param con_id       - The connection id
 * \param source_id    - The source endpoint id
 * \param sink_id      - The sink endpoint id
 * \param transform_id - The transform id
 * \param channel_id   - The data channel id
 */
void stream_kip_connect_request_handler(CONNECTION_LINK con_id,
                                        unsigned source_id,
                                        unsigned sink_id,
                                        TRANSFORM_ID transform_id,
                                        unsigned channel_id)
{
    bool valid = FALSE;
    STREAM_KIP_CONNECT_INFO* state_info = NULL;
    STREAM_CONNECT_INFO *state = NULL;

    patch_fn_shared(stream_kip);

    /* If data channel has been created already , it must have been activated*/
    if ((channel_id != 0) && (transform_id != 0))
    {
        /* Get the latest context and verify */
        if ((STREAM_KIP_STATE_IN_CONNECT()              ) &&
            (NULL != stream_kip_state_get_connect_info())    )
        {
            ENDPOINT* ep;
            state_info = stream_kip_state_get_connect_info();
            state = &state_info->connect_info;
            ep = stream_kip_get_kip_endpoint_from_state(state_info);

            valid = ((channel_id == state_info->data_channel_id) &&
                     (state->buffer_info.buffer != NULL) && (ep != NULL));
            if (valid)
            {
                /* Create the transform record */
                valid = (stream_kip_add_transform_info(
                                    STREAM_TRANSFORM_GET_INT_ID(transform_id),
                                    GET_SEND_PROC_ID(con_id),
                                    source_id, sink_id,
                                (uint16) channel_id) != NULL);
            }
        }
    }
    else
    {
        /* Channel id is 0 when the request has received either both operators
         * are on P1 or one of the endpoint in P1 must be an operator endpoint
         * and other endpoint is an audio endpoint when audio endpoints are
         * delegated (Audio endpoint delegation is not supported).
         *
         * The connection context also must be idle.
         */
        valid = STREAM_KIP_VALIDATE_EPS(source_id, sink_id) &&
                stream_kip_state_to_connect(NULL) &&
                (transform_id != 0);
    }

    if (valid)
    {
        if (state_info != NULL && state_info->sync_shadow_eps)
        {
            /* The endpoints must be synced before connecting the transform
             * because buff_metadata_connect_existing() relies heavily on
             * the shadow endpoints being synchronised correctly. */
            stream_kip_add_kip_ep_to_sync_list(state_info);
        }
        /* Connect both operators at P1 */
        stream_if_transform_connect(con_id, source_id, sink_id,
                                    transform_id, state,
                                    stream_kip_send_connect_resp);
    }
    else
    {
        stream_kip_send_connect_resp(REVERSE_CONNECTION_ID(con_id),
                                         STATUS_CMD_FAILED, 0);
    }
}

#if !defined(COMMON_SHARED_HEAP)
/**
 * \brief Handle the incoming stream connect confirm request
 *        from the primary core
 *
 * \param con_id       - The connection id
 * \param conn_to      - The shadow endpoint ID connected to
 */
void stream_kip_connect_confirm_handler(CONNECTION_LINK con_id,
                                        unsigned conn_to)
{
    unsigned shadow_id;
    ENDPOINT *shadow_ep;
    endpoint_shadow_state *state;
    tCbuffer *shared_buffer;

    patch_fn_shared(stream_kip);

    shadow_id = STREAM_GET_SHADOW_EP_ID(conn_to);
    shadow_ep = stream_endpoint_from_extern_id(shadow_id);
    state = &shadow_ep->state.shadow;

    /* Propagate potential usable_octets from shared_buffer */
    shared_buffer = ipc_data_channel_get_cbuffer( state->channel_id);
    STREAM_KIP_ASSERT( shared_buffer != NULL );
    cbuffer_set_usable_octets( state->buffer, cbuffer_get_usable_octets(shared_buffer) );
}
#endif /* !COMMON_SHARED_HEAP */

/**
 * \brief Handling the incoming destroy endpoints request from P0
 *
 * \param con_id     - The connection id
 * \param source_id  - The source endpoint id
 * \param sink_id    - The sink endpoint id
 */
void stream_kip_destroy_endpoints_request_handler(CONNECTION_LINK con_id,
                                                  unsigned source_id,
                                                  unsigned sink_id)
{
    STATUS_KYMERA status;
    KIP_MSG_STREAM_DESTROY_ENDPOINTS_RES resp;
    ADAPTOR_MSGID msg_id;

    patch_fn_shared(stream_kip);

    status = STATUS_CMD_FAILED;

    /* Allowed to destroy the endpoints only during connect state */
    if ((STREAM_KIP_STATE_IN_CONNECT()) &&
        (NULL != stream_kip_state_get_connect_info()))
    {
        STREAM_KIP_CONNECT_INFO *state_info;

        state_info = stream_kip_state_get_connect_info();
        if ((state_info->source_id == source_id) &&
            (state_info->sink_id == sink_id))
        {
            /* clean up */
            stream_kip_destroy_endpoint_ids(state_info);
            status = STATUS_OK;
        }
        stream_kip_state_to_none(TRUE);
    }

    KIP_MSG_STREAM_DESTROY_ENDPOINTS_RES_STATUS_SET(&resp, status);

    msg_id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_STREAM_DESTROY_ENDPOINTS_RES);
    adaptor_send_message(REVERSE_CONNECTION_ID(con_id),
                         msg_id,
                         KIP_MSG_STREAM_DESTROY_ENDPOINTS_RES_WORD_SIZE,
                         (ADAPTOR_DATA) &resp);
}

/**
 * \brief Handling the incoming create endpoints request from P0
 *
 * \param con_id             - The connection id
 * \param source_id          - The source endpoint id
 * \param sink_id            - The sink endpoint id
 * \param channel_id         - The data channel id
 * \param buffer_size        - The buffer size for negotiation
 * \param flags              - The buffer related flags
 * \param data_format        - The data format of the endpoint connected to
 *                             shadow on the primary core
 * \param sync_shadow_eps    - Parameter that informs the secondary core if it
 *                             should synchronise shadow endpoints on the same
 *                             port.
 */
void stream_kip_create_endpoints_request_handler(CONNECTION_LINK con_id,
                                                 unsigned source_id,
                                                 unsigned sink_id,
                                                 unsigned channel_id,
                                                 unsigned buffer_size,
                                                 unsigned flags,
                                                 AUDIO_DATA_FORMAT data_format,
                                                 bool sync_shadow_eps)
{
    bool result = FALSE, resp_result;
    STREAM_KIP_CONNECT_INFO *state = NULL;

    patch_fn_shared(stream_kip);

    /* If Pn is involved in a multi sequence kip command handling
     * it must be busy. Do not handle a new connection when KIP is busy.
     */
    if (STREAM_KIP_STATE_IN_IDLE() &&  STREAM_KIP_VALIDATE_EPS(source_id, sink_id))
    {
        STREAM_EP_LOCATION ep_location = STREAM_EP_IS_SHADOW_ID (source_id) ?
                              STREAM_EP_REMOTE_SOURCE : STREAM_EP_REMOTE_SINK;

        state = stream_kip_create_connect_info_record_ex(con_id,
                                                         source_id,
                                                         sink_id,
                                                         ep_location,
                                                         stream_kip_send_connect_resp,
                                                         data_format,
                                                         sync_shadow_eps);

        /* create the connect state record only for connection */
        if (state != NULL)
        {
            /* update the state buffer details */
            stream_kip_update_buffer_info(state, channel_id,
                                          buffer_size, flags);

            /* create the endpoints and set state */
            result = stream_kip_create_eps_for_connect(con_id, state);

            if (result)
            {
                /*
                 * At this point the flag below is already initialised by the call
                 * to stream_kip_update_buffer_info() but it only includes the
                 * information from the the remote endpoint. The intention of the
                 * flag is to include combined information from both endpoints so
                 * we update it here to reflect the overall result.
                 */
                if (stream_kip_endpoints_support_metadata(state))
                {
                    state->connect_info.buffer_info.flags.supports_metadata = TRUE;
                }
                else
                {
                    state->connect_info.buffer_info.flags.supports_metadata = FALSE;
                }

                /* Source will have to create the data channel and activate it */
                if ((ep_location == STREAM_EP_REMOTE_SINK))
                {
                    /* Set the data format retrieved from the sink on the other
                     * core to the local shadow ep. */
                    stream_kip_set_data_format_to_state(state, data_format);
                    stream_kip_set_data_format_for_shadow_ep(state);

                    PROC_ID_NUM proc_id = GET_SEND_PROC_ID(state->packed_con_id);
                    result = kip_get_buffer_and_activate_channels(state, proc_id);
                    if (result)
                    {
                        /* If channel activation is successful, endpoints
                         * response will be sent after receiving
                         * DATA_CHANNEL_ACTIVATED event.
                         */
                         return;
                    }

                    STREAM_CONNECT_FAULT(SC_KIP_GET_DATA_CHANNEL_FAILED,
                                         "Failed to get_data_channel");
                }
                else
                {
                    AUDIO_DATA_FORMAT sink_data_format;
                    sink_data_format = stream_kip_get_data_format_from_ep(sink_id);
                    /* The sink is on this core (sink_id), update the state.
                     * This data format is sent to the other core via
                     * create_endpoints_rsp.*/
                    stream_kip_set_data_format_to_state(state, sink_data_format);
                    /* Get the endpoints buffer details and
                     * it is not expected to fail*/
                    result = (channel_id != 0) ?
                             stream_kip_ep_get_buffer_info(state) : FALSE;
                }
            }
        }
    }

    resp_result = stream_kip_send_create_endpoints_resp(
                      REVERSE_CONNECTION_ID(con_id),
                      (result) ? STATUS_OK : STATUS_CMD_FAILED);

    if (!resp_result || !result)
    {
        /* free the connect resources */
        stream_kip_destroy_endpoint_ids(state);

        stream_kip_state_to_none(TRUE);
    }
}

/**
 * \brief Handling the incoming stream disconnect request from P0
 *
 * \param con_id     - The connection id
 * \param count      - Number of transforms to disconnect
 * \param tr_list    - The list of transforms
 */
void stream_kip_transform_disconnect_request_handler(CONNECTION_LINK con_id,
                                                     unsigned count,
                                                     TRANSFORM_ID *tr_list)
{
    patch_fn_shared(stream_kip);

    /* Process the request only if the state is idle */
    if (STREAM_KIP_STATE_IN_IDLE())
    {
        /* set the disconnect state */
        stream_kip_state_to_disconnect(NULL);

        /*
         * Make it deactivate ready hence the endpoint will deactivate
         * while disconnecting.
         */
        stream_kip_transform_deactivate_ready(count, tr_list);

        stream_if_part_transform_disconnect(con_id, count, tr_list, 0,
                                stream_kip_send_transform_disconnect_resp);
    }
    else
    {
        stream_kip_send_transform_disconnect_resp(
                                  REVERSE_CONNECTION_ID(con_id),
                                  STATUS_CMD_FAILED, 0);
    }
}

/**
 * \brief Handling the incoming kip_transform_list entry remove
 *        request from secondary core
 *
 * \param con_id     - The connection id
 * \param count      - Number of transforms to cleanup/remove
 * \param tr_list    - The list of transforms
 */
void stream_kip_transform_list_remove_entry_request_handler(CONNECTION_LINK con_id,
                                                            unsigned count,
                                                            TRANSFORM_ID *tr_list)
{
    STATUS_KYMERA status;

    patch_fn_shared(stream_kip);

    /* Process the request only if the state is idle */
    if (STREAM_KIP_STATE_IN_IDLE() && (count != 0))
    {
        unsigned i;

        /* set the disconnect state */
        stream_kip_state_to_disconnect(NULL);

        for (i = 0; i < count; i++)
        {
            TRANSFORM_INT_ID tr;
            tr = STREAM_TRANSFORM_GET_INT_ID(tr_list[i]);
            stream_kip_remove_transform_info_by_id(tr);
        }

        status = STATUS_OK;
    }
    else
    {
        status = STATUS_CMD_FAILED;
        count  = 0;
    }

    stream_kip_transform_list_remove_entry_resp(REVERSE_CONNECTION_ID(con_id),
                                                status, count);
}

/**
 * \brief Indication from IPC when the data channel activated
 *
 * \param status     - STATUS_OK on success
 * \param proc_id    - The remote processor id connected to the data channel
 * \param channel_id - The data channel id
 *
 * \return STATUS_KYMERA
 */
STATUS_KYMERA stream_kip_data_channel_activated(STATUS_KYMERA status,
                                                PROC_ID_NUM proc_id,
                                                uint16 channel_id)
{
    STREAM_KIP_CONNECT_INFO *state;
    ENDPOINT *ep;

    patch_fn_shared(stream_kip);

    state = stream_kip_state_get_connect_info();

    /* Validate it with the active context. If it is
     * not found, there is something wrong. panic
     */
    if (!STREAM_KIP_STATE_IN_CONNECT() || (state == NULL) ||
        PROC_ON_SAME_CORE(proc_id))
    {
        fault_diatribe(FAULT_AUDIO_MULTICORE_CONNECT_INVALID_STATE,
                       (proc_id) | ((stream_kip_state.state)<<2));

        STREAM_CONNECT_FAULT(SC_KIP_INVALID_STATE,
                             "stream_kip_data_channel_activated: Lost state");

        /* We cannot recover this state */
        return STATUS_CMD_FAILED;
    }

    ep = stream_kip_get_kip_endpoint_from_state(state);
    if (ep == NULL)
    {
        /* The ep for the metadata's partner channel (for example with ID
           0x180 for metadata channel ID 0x18F) is not found. This could
           happen when 0x180 wasn't created (e.g. due to a fault) and the
           setup nevertheless carries on trying to activate other channels. */
        fault_diatribe(FAULT_AUDIO_METADATA_PARTNER_EP_MISSING, channel_id);
        return STATUS_CMD_FAILED;
    }
#if !defined(COMMON_SHARED_HEAP)
    if (DATA_CHANNEL_IS_META(channel_id))
    {
        if (!state->data_channel_is_activated)
        {
            STREAM_CONNECT_FAULT(SC_LOG_ONLY,
                                 "inactivated data channel present");
        }

        /* Set metadata channel ID, buffer and flag in the shadow endpoint */
        ep->functions->configure(ep, EP_METADATA_CHANNEL_ID, channel_id);
        ep->functions->configure(ep, EP_METADATA_SUPPORT, TRUE);
    }
    else
#endif /* !COMMON_SHARED_HEAP */
    {
        /* update the data channel id in KIP endpoint */
        ep->functions->configure(ep, EP_SET_DATA_CHANNEL, channel_id);
    }

    /*
     * Data channel can be activated by the local processor or remote
     * processor. This must be activated by the side that supplies
     * buffer. i.e when the remote endpoint is sink
     *
     * There are 4 cases
     * 1. Primary core initiated activation (remote sink case)
     *     - Proceed with stream connect request on success
     *
     * 2. Secondary core initiated activation (remote source case)
     *    - Respond the stream create endpoints request
     *
     * 3. Secondary core notified for case 1
     *     - Primary core will proceed with connection or
     *       disconnection
     *
     * 4. Primary core notified for case 2
     *     - Do Nothing. Secondary core will notify endpoint creation
     *       status.
     *
     */
    if (state->ep_location == STREAM_EP_REMOTE_SOURCE)
    {
        if (status != STATUS_OK)
        {
            return status;
        }
#if !defined(COMMON_SHARED_HEAP)
        /* Activation initiated by remote side. case 3 & 4*/
        if (DATA_CHANNEL_IS_META(channel_id))
        {
            state->meta_channel_id = channel_id;
        }
        else
#endif /* !COMMON_SHARED_HEAP */
        {
            ENDPOINT *source_ep;
            ENDPOINT *sink_ep;
            state->data_channel_id = channel_id;
            source_ep = stream_endpoint_from_extern_id(state->source_id);
            sink_ep = stream_endpoint_from_extern_id(state->sink_id);
            /* Requesting to clone the buffer will create a
               clone of the remote buffer. This is called for
               the remote source endpoint only when data channel
               activation is completed.
               Create a clone buffer every time to enable metadata
               sync if enabled.
               If COMMON_SHARED_HEAP is defined, the buffer is not cloned,
               but the shadow endpoint is set with the buffer from the data
               channel. */

#if defined(COMMON_SHARED_HEAP)
            if (!source_ep->functions->configure(source_ep,
                                                 EP_SET_SHADOW_STATE_BUFFER,
                                                 0) )
            {
                return STATUS_CMD_FAILED;
            }
#else
            if (!source_ep->functions->configure(source_ep,
                                                 EP_CLONE_REMOTE_BUFFER,
                                                 1) )
            {
                return STATUS_CMD_FAILED;
            }
#endif /* COMMON_SHARED_HEAP */
            /* Now get the buffer which will be a cloned one
               if the kip endpoint is connected to a real endpoint. */
            if (!stream_connect_get_buffer(source_ep,
                                           sink_ep,
                                           &state->connect_info))
            {
                return STATUS_CMD_FAILED;
            }
            if (state->data_channel_id == channel_id)
            {
                state->data_channel_is_activated = TRUE;
            }
#if !defined(COMMON_SHARED_HEAP)
            else
            {
                state->metadata_channel_is_activated = TRUE;
            }
#endif /* !COMMON_SHARED_HEAP */
        }

        return STATUS_OK;
    }

    /* Remote Sink only cases reach here */
    if (status != STATUS_OK)
    {
#if !defined(COMMON_SHARED_HEAP)
        /* Activation attempt failed. destroy the data channel */
        if (state->meta_channel_id == channel_id)
        {
            bool data_channel_status;

            ipc_destroy_data_channel(state->meta_channel_id);
            data_channel_status = stream_kip_data_channel_deactivate_ipc(state->data_channel_id);

            STREAM_KIP_ASSERT(data_channel_status);
        }
        else
#endif /* !COMMON_SHARED_HEAP */
        {
            /* Activation attempt failed. destroy the data channel */
            ipc_destroy_data_channel(state->data_channel_id);
        }
        /* don't destroy the endpoints now */
    }

    if (proc_id == PROC_PROCESSOR_0)
    {
        bool supports_metadata;

        /* On Pn - case 2 */
        if (status == STATUS_OK)
        {
            if (state->data_channel_id == channel_id)
            {
                state->data_channel_is_activated = TRUE;
            }
#if !defined(COMMON_SHARED_HEAP)
            else
            {
                state->metadata_channel_is_activated = TRUE;
                state->meta_channel_id = channel_id;
                /* Update metedata_buffer information based on previous connections if any */
                stream_kip_update_metadata_buffer(state);
            }
#endif /* !COMMON_SHARED_HEAP */
        }

        supports_metadata = state->connect_info.buffer_info.flags.supports_metadata;
        if ((supports_metadata && BOTH_CHANNELS_ARE_ACTIVATED(state)) ||
            (!supports_metadata))
        {
            bool success;
            CONNECTION_LINK reversed_con_id;

            reversed_con_id = REVERSE_CONNECTION_ID(state->packed_con_id);
            success = stream_kip_send_create_endpoints_resp(reversed_con_id, status);
            if (!success || (status != STATUS_OK))
            {
                stream_kip_destroy_endpoint_ids(state);

                /* Free the global context and unblock KIP */
                stream_kip_state_to_none(TRUE);
            }
        }
    }

    if (proc_id != PROC_PROCESSOR_0)
    {
        bool success = FALSE;

        /* case 1 : Primary core initiated activation completed.
         * On Success,
         *    -  initiate the stream connect process
         *
         * On Failure
         *      - Send remote request to destroy the endpoints
         */
        if (status == STATUS_OK)
        {
            if (state->data_channel_id == channel_id)
            {
                state->data_channel_is_activated = TRUE;
            }
#if !defined(COMMON_SHARED_HEAP)
            else
            {
                state->metadata_channel_is_activated = TRUE;
                state->meta_channel_id = channel_id;
                /* Update metedata_buffer information based on previous connections if any */
                stream_kip_update_metadata_buffer(state);
            }
            if ((state->connect_info.buffer_info.flags.supports_metadata) &&
                 !BOTH_CHANNELS_ARE_ACTIVATED(state))
            {
                /* If not both data and metadata channels are activated,
                 * wait for the other connection and return STATUS_OK */
                return STATUS_OK;

            }
            else /* If we don't support metadata */
#endif /* !COMMON_SHARED_HEAP */
            {
                /* Send a KIP request to connect endpoints */
                success = stream_kip_connect_endpoints(state->packed_con_id,
                                STREAM_GET_SHADOW_EP_ID(state->source_id),
                                STREAM_EP_ID_FROM_SHADOW_ID(state->sink_id),
                                state);
            }
        }
        if (!success)
        {
            /* Send a KIP request to destroy endpoints */
            stream_kip_destroy_endpoints(state->packed_con_id,
                            STREAM_GET_SHADOW_EP_ID(state->source_id),
                            STREAM_EP_ID_FROM_SHADOW_ID(state->sink_id),
                            state);
        }
    }

    return STATUS_OK;
}

/**
 * \brief Indication from IPC when the data channel deactivated
 *
 * \param status  STATUS_OK on success
 * \param channel The data channel id
 *
 * \return STATUS_KYMERA
 */
STATUS_KYMERA stream_kip_data_channel_deactivated(STATUS_KYMERA status,
                                                  uint16 channel)
{
    STREAM_KIP_TRANSFORM_INFO *tr;

    tr = stream_kip_transform_info_from_chanid(channel);
    if ((tr != NULL) && (status == STATUS_OK))
    {
        tr->enabled = FALSE;
    }
#if !defined(COMMON_SHARED_HEAP)
    /* For data channels, IPC allocates and destroys its own cbuffer structure
       but for metadata channels, we instruct IPC to directly use the passed
       cbuffer which means IPC cannot destroy it. The destroy is done here
       because it's the last place we can retrieve the pointer from IPC. */
    if (DATA_CHANNEL_IS_META(channel) &&
        (IPC_DATA_CHANNEL_WRITE == ipc_get_data_channelid_dir(channel)))
    {
        tCbuffer *cbuf;

        cbuf = ipc_data_channel_get_cbuffer(channel);
        cbuffer_destroy(cbuf);
    }
#endif /* !COMMON_SHARED_HEAP */
    return STATUS_OK;
}

/**
 * \brief  Deactivate the data channel from ipc
 *         Shadow endpoint calls this directly to deactivate meta data channel
 *
 * \param channel - The data channel to be deactivated.
 *
 * \return FALSE if deactivation failed, TRUE if success
 */
bool stream_kip_data_channel_deactivate_ipc(uint16 channel)
{
    if (ipc_get_data_channelid_dir(channel) == IPC_DATA_CHANNEL_WRITE)
    {
        return (ipc_deactivate_data_channel(channel) == IPC_SUCCESS);
    }
    return TRUE;
}

/**
 * \brief  Shadow endpoint calls this to deactivate data channel
 *
 * \param  channel - The data channel to be deactivated.
 *
 * \return FALSE if channel not found, TRUE if channel destroyed
 */
bool stream_kip_data_channel_deactivate(uint16 channel)
{
    STREAM_KIP_TRANSFORM_INFO *tr;
    bool success;

    tr = stream_kip_transform_info_from_chanid(channel);
    if (tr == NULL)
    {
        return TRUE;
    }

    success = stream_kip_data_channel_deactivate_ipc(channel);
    if (success)
    {
        stream_kip_remove_transform_info(tr);
    }

    return success;
}

/**
 * \brief  Destroy the data channel from ipc
 *
 * \param  channel - The data channel to be destroyed.
 *
 * \return FALSE if channel not found, TRUE if channel destroyed
 */
bool stream_kip_data_channel_destroy_ipc(uint16 channel)
{
    if (ipc_get_data_channelid_dir(channel) == IPC_DATA_CHANNEL_WRITE)
    {
        return (ipc_destroy_data_channel(channel) == IPC_SUCCESS);
    }
    return TRUE;
}

/**
 * \brief  Shadow endpoint calls this to destroy the data channel
 *
 * \param  channel - The data channel to be deactivated.
 *
 * \return FALSE if channel not found, TRUE if channel destroyed
 */
bool stream_kip_data_channel_destroy(uint16 channel)
{
    bool success;

    patch_fn_shared(stream_kip);

    /* To destroy the data channel, it must have been already
       removed from the transform list. */
    success = FALSE;
    if (!stream_kip_transform_info_from_chanid(channel))
    {
        success = stream_kip_data_channel_destroy_ipc(channel);
    }

    return success;
}

/**
 * \brief   Get metadata_buffer from the same endpoint base
 *
 * \param   endpoint Pointer to endpoint object
 *
 * \return  metadata_buffer if it found any, otherwise, NULL
 */
tCbuffer *stream_kip_return_metadata_buf(ENDPOINT *endpoint)
{
    patch_fn_shared(stream_kip);
#if !defined(COMMON_SHARED_HEAP)
    tCbuffer *metadata_buffer;
    unsigned endpoint_base;
    ENDPOINT *ep;

    metadata_buffer = NULL;
    endpoint_base = GET_BASE_EPID_FROM_EPID(endpoint->id);
    ep = (STREAM_EP_SINK_BIT & endpoint_base) ? sink_endpoint_list :
                                                source_endpoint_list;
    while (ep != NULL)
    {
        if (STREAM_EP_IS_SHADOW_ID(ep->id) &&
            (GET_BASE_EPID_FROM_EPID(ep->id) == endpoint_base))
        {

            tCbuffer *ep_buffer;
            tCbuffer *endpoint_buffer;

            /* Check if both of the endpoints are sharing the same meta buffer. */
            ep_buffer = stream_shadow_get_shared_metadata_buffer(ep);
            endpoint_buffer = stream_shadow_get_shared_metadata_buffer(endpoint);
            if ((ep_buffer == endpoint_buffer) &&
                (ep != endpoint))
            {
                metadata_buffer = stream_shadow_get_metadata_buffer(ep);
                break;
            }
        }
        ep = ep->next;
    }

    return metadata_buffer;
}
#else
    ENDPOINT *synced_ep = endpoint->state.shadow.head_of_sync;
    tCbuffer *metadata_buffer;

    metadata_buffer = NULL;

    while(synced_ep)
    {
        /* return the first connected buffer*/
        if (synced_ep->state.shadow.buffer)
        {
            metadata_buffer = synced_ep->state.shadow.buffer;
            break;
        }
        synced_ep = synced_ep->state.shadow.nep_in_sync;
    }

    return metadata_buffer;
}
#endif /* !COMMON_SHARED_HEAP */

#if !defined(COMMON_SHARED_HEAP)
/**
 * \brief   Check if this endpoint is in the last metadata data connection
 *
 * \param   endpoint   - Pointer to endpoint object
 *
 * \return  TRUE if it is, otherwise, FALSE;
 */
bool stream_kip_is_last_meta_connection(ENDPOINT *endpoint)
{
    if (stream_kip_return_metadata_buf(endpoint) != NULL)
    {
        /* Another connection exists */
        return FALSE;
    }

    return TRUE;
}

/**
 * \brief   Request remote to set the activated flag in the kip state with
 *          an existing metadata data channel. Then, send a response back
 *
 * \param   packed_con_id   - Packed send/receive connection ID
 * \param   meta_channel_id - The existing metadata data channel id
 */
void stream_kip_metadata_channel_activated_req_handler(CONNECTION_LINK packed_con_id,
                                                       uint16 meta_channel_id)
{
    /* At this point, we must have a existing metadata data channel so the IPC status is SUCCESS. */
    STATUS_KYMERA status;
    PROC_ID_NUM remote_proc_id;
    uint16 inverted_chanid_dir;

    remote_proc_id = stream_kip_get_remote_proc_id(packed_con_id);

    /* Since the channel has been activated, we just need to set the activated flag */
    status = stream_kip_data_channel_activated(STATUS_OK,
                                               remote_proc_id,
                                               meta_channel_id);

    /* We don't expect a failure here */
    STREAM_KIP_ASSERT(status == STATUS_OK);

    inverted_chanid_dir = (uint16) ipc_invert_chanid_dir(meta_channel_id);

    /* Now send a response back to activate the channel on the other core */
    (void) stream_kip_send_metadata_channel_activated_resp(packed_con_id,
                                                           status,
                                                           inverted_chanid_dir);
}

/**
 * \brief   Response to local to set the activated flag in the kip state
 *          with an existing metadata data channel
 *
 * \param   packed_con_id   - Packed send/receive connection ID
 * \param   status          - STATUS_KYMERA
 * \param   meta_channel_id - The existing metadata data channel id
 */
void stream_kip_metadata_channel_activated_resp_handler(CONNECTION_LINK packed_con_id,
                                                        STATUS_KYMERA status,
                                                        uint16 meta_channel_id)
{
    PROC_ID_NUM remote_proc_id;

    remote_proc_id = stream_kip_get_remote_proc_id(packed_con_id);

    /* Since the channel has been activated,
       we just need to set the activated flag. */
    status = stream_kip_data_channel_activated(status,
                                               remote_proc_id,
                                               meta_channel_id);

    /* We don't expect a failure here */
    STREAM_KIP_ASSERT(status == STATUS_OK);
}
#endif /* !COMMON_SHARED_HEAP */

/**
 * \brief kick the kip endpoints on receiving kip signals
 *
 * \param data_chan_id Data channel id
 * \param kick_dir     The kick direction
 */
void stream_kip_kick_eps(unsigned data_chan_id,
                         ENDPOINT_KICK_DIRECTION kick_dir)
{
    ENDPOINT *ep;

    patch_fn_shared(stream_kip);

    ep = stream_shadow_ep_from_data_channel(data_chan_id);
    STREAM_KIP_ASSERT(ep != NULL);
    STREAM_KIP_ASSERT(ep == ep->state.shadow.head_of_sync);

    ep->functions->kick(ep, kick_dir);
}

unsigned stream_kip_get_transform_source_id(STREAM_KIP_TRANSFORM_INFO *tfm)
{
    return tfm->source_id;
}

unsigned stream_kip_get_transform_sink_id(STREAM_KIP_TRANSFORM_INFO *tfm)
{
    return tfm->sink_id;
}

STREAM_KIP_TRANSFORM_INFO *stream_kip_get_next_transform(STREAM_KIP_TRANSFORM_INFO *tfm)
{
    if (tfm == NULL)
    {
        return kip_transform_list;
    }

    return tfm->next;
}

STATUS_KYMERA stream_kip_disconnect(CONNECTION_LINK con_id,
                                    unsigned source_id,
                                    unsigned sink_id,
                                    STREAM_TRANSFORM_PAIR_CBACK callback)
{
    STREAM_KIP_TRANSFORM_INFO *tr;
    unsigned ids[TOTAL_INDEX];
    TRANSFORM_ID transforms[TOTAL_INDEX];
    STREAM_KIP_TRANSFORM_DISCONNECT_INFO *state_info;
    STREAM_KIP_TRANSFORM_DISCONNECT_CB cb;
    CONNECTION_LINK con_proc_id;
    unsigned px_tr_count;
    TRANSFORM_ID remote_trid;
    TRANSFORM_INT_ID id;
    bool found;
    unsigned i;

    patch_fn_shared(stream_kip);

    ids[SOURCE_INDEX] = source_id;
    ids[SINK_INDEX] = sink_id;

    found = FALSE;
    px_tr_count = 0;
    for (i = 0; i < TOTAL_INDEX; i++)
    {
        transforms[i] = 0;
        /* get the remote transform id if exits */
        tr = stream_kip_transform_info_from_epid(ids[i]);
        if (tr != NULL)
        {
            transforms[i] = STREAM_TRANSFORM_GET_EXT_ID(tr->id);
            found = TRUE;
        }
        else
        {
            ENDPOINT *ep;

            /* Find the id in the local transform and add to the list even if
               it is a local transform. */
            ep = stream_endpoint_from_extern_id(ids[i]);
            if ((ep != NULL) && (ep->connected_to != NULL))
            {
                TRANSFORM* tfm;

                tfm = stream_transform_from_endpoint(ep);
                transforms[i] = stream_external_id_from_transform(tfm);

                if ((i == SOURCE_INDEX) &&
                    (STREAM_EP_IS_SHADOW(ep->connected_to)))
                {
                    px_tr_count = 1;
                }
            }
        }
    }

    if (!found)
    {
        /* No transform present so this is not for a secondary
           processor. */
        return STATUS_INVALID_CMD_PARAMS;
    }

    remote_trid = (transforms[SOURCE_INDEX] == 0) ? transforms[SINK_INDEX] :
                                                    transforms[SOURCE_INDEX];
    id = STREAM_TRANSFORM_GET_INT_ID(remote_trid);
    tr = stream_kip_transform_info_from_id(id);
    /* At this point, tr cannot be NULL. */
    con_proc_id = PACK_CONID_PROCID(con_id, tr->processor_id);
    cb.disc_cb = callback;

    /* Create a stream disconnect state record for disconnection */
    state_info = stream_kip_create_disconnect_info_record(con_proc_id,
                                                          TOTAL_INDEX,
                                                          TRUE,
                                                          transforms,
                                                          cb);

    if (state_info == NULL)
    {
        return STATUS_CMD_FAILED;
    }

    if (!stream_kip_transform_disconnect(state_info, px_tr_count))
    {
        return STATUS_CMD_FAILED;
    }

    return STATUS_CMD_PENDING;
}

bool stream_kip_part_transform_disconnect(CONNECTION_LINK con_id,
                                          unsigned count,
                                          unsigned px_tr_count,
                                          TRANSFORM_ID *transforms,
                                          STREAM_COUNT_CBACK callback)
{
    STREAM_KIP_TRANSFORM_DISCONNECT_INFO *state_info;
    STREAM_KIP_TRANSFORM_INFO *tr;
    CONNECTION_LINK con_proc_id;
    STREAM_KIP_TRANSFORM_DISCONNECT_CB cb;
    unsigned id;

    patch_fn_shared(stream_kip);

    id = STREAM_TRANSFORM_GET_INT_ID(transforms[0]);
    tr = stream_kip_transform_info_from_id(id);
    /* tr will not be NULL if we are inside this if condition.
       so not validating tr. */
    con_proc_id = PACK_CONID_PROCID(con_id, tr->processor_id);
    cb.tr_disc_cb = callback;

    /* Create a stream disconnect state record for disconnection */
    state_info = stream_kip_create_disconnect_info_record(con_proc_id,
                                                          count,
                                                          FALSE,
                                                          transforms,
                                                          cb);
    if (state_info == NULL)
    {
        return FALSE;
    }

    if (!stream_kip_transform_disconnect(state_info, px_tr_count))
    {
        return FALSE;
    }

    return TRUE;
}

bool stream_kip_transform_from_ep(CONNECTION_LINK reversed_con_id,
                                  unsigned sid,
                                  STREAM_TRANSFORM_ID_CBACK callback)
{
    STREAM_KIP_TRANSFORM_INFO *tr;

    if (PROC_PRIMARY_CONTEXT())
    {
        tr = stream_kip_transform_info_from_epid(sid);
        if (tr != NULL)
        {
            TRANSFORM_ID id;

            id = STREAM_TRANSFORM_GET_EXT_ID(tr->id);
            callback(reversed_con_id, STATUS_OK, id);
            return TRUE;
        }
    }
    else
    {
        /* This API must be called only at P0 */
        callback(reversed_con_id, STATUS_INVALID_CMD_PARAMS, 0);
        return TRUE;
    }
    return FALSE;
}
