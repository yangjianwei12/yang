/****************************************************************************
 * Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.
 ****************************************************************************
 * \file pl_timers_for_sched.h
 * \ingroup pl_timers
 *
 * header for the internals of the timer, which is shared between timer and
 * scheduler
 *
 * NOTES:
 * This file contains bits of the timer that only scheduler code should see
 *
 * The timer module has 2 internal lists for casual and strict timed events
 *
 * The casual timed events are stored in a list, which is accessed and
 * serviced from the scheduler loop. The list and access functions are
 * declared here.
 *
 */
#ifndef PL_TIMERS_FOR_SCHED_H
#define PL_TIMERS_FOR_SCHED_H

/****************************************************************************
Include Files
*/

#include "pl_timers/pl_timers.h"

/****************************************************************************
Public Function Prototypes
*/

/**
 * \brief Timer service routine, which traverses through the casual event list
 * and services expired events.

 *
 * \note This function is internal to platform and is called by the scheduler.
 * This function expects to be called with interrupts blocked. This is for
 * efficiency as the scheduler always has interrupts blocked immediately prior
 * to calling. Interrupts are unblocked when an event handler is called. The
 * function exits with interrupts in the same state as when called.
 */
extern void timers_service_expired_casual_events(void);

/**
 * \brief Finds the deadline of the next timer (strict or casual) that is set to expire.
 *
 * \param[out] next_time Holds the expiry time of the first event to expire, if there
 * are any.
 *
 * \return Returns TRUE if a valid timer was found, else FALSE
 */
extern bool timers_get_next_event_time(TIME *next_time);

/**
 * \brief The Timer2 hardware interrupt handler which causes the scheduler to check for
 * a casual timer to service.
 */
extern void casual_kick_event(void);

#endif /* PL_TIMERS_FOR_SCHED_H */
