/****************************************************************************
 * Copyright (c) 2018 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file stream_for_ops.h
 * \ingroup stream
 *
 * This file contains public stream functions that are used by operators
 * and capabilities.
 */

#ifndef STREAM_FOR_OPS_H
#define STREAM_FOR_OPS_H

#include "types.h"
#include "status_prim.h"
#include "stream_prim.h"
#include "opmgr/opmgr_common.h"
#include "stream/stream_common.h"
#include "stream/stream_audio_data_format.h"
#include "stream/stream_downstream_probe.h"
#include "buffer/buffer.h"

/**
 * \brief Get the system streaming rate
 *
 * \return system sampling_rate
 */
extern uint32 stream_if_get_system_sampling_rate(void);

/**
 * \brief Get the system kick period
 *
 * This is the maximum value across all endpoints which have configurable kick periods
 *
 * \return kick period in microseconds
 */
extern TIME_INTERVAL stream_if_get_system_kick_period(void);

/**
 * \brief get the endpoint that is connected to an operator's terminal
 *
 * \param opid operators id
 * \param terminal_id terminal ID
 *
 * \return connected operator to the terminal, NULL if not connected
 */
extern ENDPOINT *stream_get_connected_endpoint_from_terminal_id(INT_OP_ID opid,
                                                                unsigned terminal_id);

/**
 * \brief get current sample rate of an endpoint
 *
 * \param ep pointer to endpoint structure
 * \param value pointer to be populated with endpoint's sample rate
 *
 * \return TRUE if *value is populated with sample rate else FALSE.
 *
 * NOTE: Only endpoint types that that support EP_SAMPLE_RATE config key
 *       may return success.
 */
extern bool stream_get_sample_rate(ENDPOINT *ep, uint32 *value);

/**
 * \brief get the cbuffer associated with a transform
 *
 * \param transform_id transform extrenal ID
 *
 * \return cbuffer associated with the transfor or NULL if not found
 *
 * Important notes:
 *
 * - Operators shall not use this function to retreive their own terminal
 *   buffer (which they receive it at connection time). The main purpose 
 *   of this function is to allow peeking the status and/or content of a
 *   connection buffer.
 *
 * - Note that the return value is read only, caller shall never try to modify
 *   the status or content of the connection buffer using returned handle.
 *
 * - Do not use this function for inter-core transforms (connection between
 *   operators running in different audio cores).
 *
 * - The return of this function shall always NULL checked and if you wanted
 *   to peek the content of the buffer do that only if it is local SW or
 *   local MMU buffer.
 */
extern tCbuffer* stream_get_buffer_from_external_transform_id(TRANSFORM_ID transform_id);

/**
 * \brief get the operator associated with a terminal endpoint
 *
 * \param ep endpoint pointer
 *
 * \return associated operator, or NULL if not found
 */
extern OPERATOR_DATA *stream_get_op_data_from_endpoint(ENDPOINT *ep);

/**
 * stream_get_opdata_from_external_transform_id
 * \brief given transform_id finds the operator data in one side
 * \param tid external transform ID
 * \param get_sink if TRUE returns operator data for op in sink side of the transform else
 *        for source side
 * \returns operator data if op is found else NULL
 */
extern OPERATOR_DATA *stream_get_opdata_from_external_transform_id(TRANSFORM_ID tid, bool get_sink);

#endif /* STREAM_FOR_OPS_H */
