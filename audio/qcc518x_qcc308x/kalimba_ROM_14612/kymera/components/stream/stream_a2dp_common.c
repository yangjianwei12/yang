/****************************************************************************
 * Copyright (c) 2015 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
#include "adaptor/adaptor.h"
#include "audio_log/audio_log.h"
#include "buffer.h"
#include "stream.h"
#include "stream_audio_data_format.h"
#include "stream_endpoint.h"
#include "stream_endpoint_a2dp.h"
#include "stream_private.h"
#include "types.h"

/**
 * a2dp_set_data_format
 */
static bool a2dp_set_data_format(ENDPOINT *endpoint, AUDIO_DATA_FORMAT format)
{
    if (AUDIO_DATA_FORMAT_ENCODED_DATA == format)
    {
        return TRUE;
    }

    return FALSE;
}

bool a2dp_configure(ENDPOINT *endpoint, unsigned key, uint32 value)
{
    /* A2DP specific endpoint configuration code.
     *
     * There is nothing about an a2dp endpoint that can be configured externally.
     * However there are some internal configuration commands that are supported.
     */
    switch(key)
    {
    case EP_DATA_FORMAT:
        return a2dp_set_data_format(endpoint, (AUDIO_DATA_FORMAT)value);
    default:
        return FALSE;
    }
}

bool a2dp_buffer_details(ENDPOINT *endpoint, BUFFER_DETAILS *details)
{
    if (endpoint == NULL || details == NULL)
    {
        return FALSE;
    }

    /* This endpoint type may produce/consume metadata. */
    endpoint_a2dp_state *a2dp = &endpoint->state.a2dp;

    details->supports_metadata = FALSE;
    if (SOURCE == endpoint->direction)
    {
        if (buff_has_metadata(a2dp->source_buf))
        {
            details->supports_metadata = TRUE;
        }
    }
    else
    {
        if (buff_has_metadata(a2dp->sink_buf))
        {
            details->supports_metadata = TRUE;
        }
    }
    details->metadata_buffer = NULL;

    details->b.buff_params.size = A2DP_BUFFER_MIN_SIZE;
    details->b.buff_params.flags = BUF_DESC_SW_BUFFER;
    details->supplies_buffer = FALSE;
    details->runs_in_place = FALSE;
    details->wants_override = details->can_override = FALSE;
    return TRUE;
}

/**
 * \brief Connect to the endpoint.
 *
 * \param endpoint         Pointer to the endpoint that is being connected.
 * \param cbuffer_ptr      Pointer to the Cbuffer struct for the buffer that
 *                         is being connected.
 * \param ep_to_kick       Pointer to the endpoint which will be kicked after
 *                         a successful run. Note: this can be different from
 *                         the connected to endpoint when in-place running is
 *                         enabled.
 * \param start_on_connect Return flag which indicates if the endpoint wants be
 *                         started on connect.
 *
 * \return success or failure
 *
 * \note The endpoint will only be started if the connected endpoint wants
 *       to be started too.
 */
bool a2dp_connect(ENDPOINT *endpoint,
                  tCbuffer *cbuffer_ptr,
                  ENDPOINT *ep_to_kick,
                  bool* start_on_connect)
{
    endpoint->ep_to_kick = ep_to_kick;

    cbuffer_set_usable_octets(cbuffer_ptr, ENCODED_DATA_OCTETS_IN_WORD);

    if (SOURCE == endpoint->direction)
    {
        endpoint->state.a2dp.sink_buf = cbuffer_ptr;
    }
    else
    {
        endpoint->state.a2dp.source_buf = cbuffer_ptr;
    }
    *start_on_connect = FALSE;
    return TRUE;
}

bool a2dp_disconnect(ENDPOINT *endpoint)
{
    if (SOURCE == endpoint->direction)
    {
        endpoint->state.a2dp.sink_buf = NULL;
    }
    else
    {
        endpoint->state.a2dp.source_buf = NULL;
    }

    return TRUE;
}

void a2dp_get_timing_common(ENDPOINT *ep, ENDPOINT_TIMING_INFORMATION *time_info)
{
    time_info->block_size = 0;
    time_info->is_volatile = TRUE;
    if (ep->direction == SINK)
    {
        /* To-air A2DP is just sent as and when we dictate so this end owns the
         * clock source. */
        time_info->locally_clocked = TRUE;
    }
    else
    {
        /* Disable endpoint-to-endpoint rate matching, timed playback is used instead */
        time_info->locally_clocked = TRUE;
    }
}

bool a2dp_get_config(ENDPOINT *endpoint, unsigned key, ENDPOINT_GET_CONFIG_RESULT* result)
{
    /* A2DP specific endpoint configuration code.
     */
    switch(key)
    {
    case EP_DATA_FORMAT:
        result->u.value = AUDIO_DATA_FORMAT_ENCODED_DATA;
        return TRUE;
    case EP_RATEMATCH_ABILITY:
        result->u.value = (uint32)RATEMATCHING_SUPPORT_NONE;
        return TRUE;
    case EP_RATEMATCH_RATE:
        result->u.value = RM_PERFECT_RATE;
        return TRUE;
    case EP_RATEMATCH_MEASUREMENT:
        result->u.rm_meas.sp_deviation = 0;
        result->u.rm_meas.measurement.valid = FALSE;
        return TRUE;
    default:
        return FALSE;
    }
}
