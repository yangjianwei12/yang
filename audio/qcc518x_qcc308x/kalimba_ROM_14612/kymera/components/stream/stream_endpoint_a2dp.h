/****************************************************************************
 * Copyright (c) 2015 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stream_endpoint_a2dp.h
 * \ingroup stream
 */
#ifndef STREAM_ENDPOINT_A2DP_H
#define STREAM_ENDPOINT_A2DP_H
#include "buffer.h"
#include "stream.h"
#include "stream_endpoint.h"

/** The minimum buffer size that this endpoint needs to be connected to */
#define A2DP_BUFFER_MIN_SIZE (256)

bool a2dp_configure(ENDPOINT *endpoint, unsigned key, uint32 value);
bool a2dp_buffer_details(ENDPOINT *endpoint, BUFFER_DETAILS *details);
bool a2dp_connect(ENDPOINT *endpoint, tCbuffer *Cbuffer_ptr,  ENDPOINT *ep_to_kick, bool* start_on_connect);
bool a2dp_disconnect(ENDPOINT *endpoint);
bool a2dp_get_config(ENDPOINT *endpoint, unsigned key, ENDPOINT_GET_CONFIG_RESULT* result);

/**
 * \brief perform part of a2dp get_timing that is common across all platforms
 * \param ep a2dp endpoint
 * \param time_info endpoint timing information
 */
void a2dp_get_timing_common(ENDPOINT *ep, ENDPOINT_TIMING_INFORMATION *time_info);

#endif /* STREAM_ENDPOINT_A2DP_H */

