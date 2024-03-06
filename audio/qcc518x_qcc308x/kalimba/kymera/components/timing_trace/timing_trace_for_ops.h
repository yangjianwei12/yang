/****************************************************************************
 * Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file timing_trace_for_ops.h
 * \ingroup timing_trace
 *
 * Public event recorder API for capabilities
 */
#ifndef TIMING_TRACE_FOR_OPS_H
#define TIMING_TRACE_FOR_OPS_H

#include "timing_trace_types_for_ops.h"


/******************************
 * Public Function Declarations / Inline Definitions
 */

/**
 * \brief Check before costly preparations of parameters
 *        for timing trace recorder functions
 *        like timing_trace_record_op_event.
 *        The record event functions themselves also check this,
 *        so for isolated calls with readily available parameters
 *        it is not necessary to call this first.
 *
 * \return TRUE if the timing trace recorder functions can be
 *         called and the trace data transport is active.
 */
#if defined(INSTALL_TIMING_TRACE) && !defined(UNIT_TEST_BUILD) && !defined(DESKTOP_TEST_BUILD)
extern bool timing_trace_is_enabled(void);
#else /* defined INSTALL_TIMING_TRACE */
static inline bool timing_trace_is_enabled(void)
{
    return FALSE;
}
#endif /* defined INSTALL_TIMING_TRACE */


#endif /* TIMING_TRACE_FOR_OPS_H */
