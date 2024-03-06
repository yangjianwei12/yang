/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 ****************************************************************************
 * \file pl_timers_private.h
 * \ingroup pl_timers
 *
 * Private header shared by component pl_timers and its unit tests.
 */
#ifndef PL_TIMERS_PRIVATE_H
#define PL_TIMERS_PRIVATE_H

/****************************************************************************
Include Files
*/

#include "pl_timers/pl_timers.h"
#include "pl_timers/pl_timers_for_sched.h"
#include "hydra/hydra_types.h"
#include "platform/pl_assert.h"
#include "platform/pl_interrupt.h"
#include "platform/pl_intrinsics.h"
#include "platform/pl_error.h"
#include "pmalloc/pl_malloc.h"
#include "panic/panic.h"
#include "sched_oxygen/sched_oxygen.h"
#include "sched_oxygen/sched_oxygen_for_timers.h"
#include "patch/patch.h"

/****************************************************************************
Private Type Definitions
*/

/**
 * Defines an abstract timer handle. This is only ever used as a pointer
 */
typedef struct tTimerStuctTag
{
    struct tTimerStuctTag *next; /**<Pointer to the next timer in a linked list */
    /** Timer ID is a unique combination of timer id count and various flags based on
     * event parameters. */
    tTimerId timer_id;
    void *data_pointer; /**< data pointer passed to timer expiry function */
    tTimerEventFunction TimedEventFunction; /**< Timer expiry handler function */
    union
    {
        TIME event_time; /** Strict time deadline */
        struct
        {
            TIME earliest_time;
            TIME latest_time;
        }casual; /** Structure containing casual time data*/
    } variant;
} tTimerStruct;

typedef struct tEventsQueueTag
{
    tTimerStruct *first_event; /**< Head of event queue */

    /** The last time a timer on this message queue fired. This is provided so
     * that a user can request this if they want to schedule a periodic timer
     * during the timer handler. */
    TIME last_fired;

    /* Functions that access the queue */

    /* functions that act on queue elements */
    TIME (*get_expiry_time)(tTimerStruct *t); /**< Get the expiry time */
    TIME (*get_latest_time)(tTimerStruct *t); /**< Get the latest expiry time */
    /**
     * comparison function, which returns TRUE if event expires earlier than
     * given time
     */
    bool (*is_event_time_earlier_than)(tTimerStruct *t, TIME time);
} tEventsQueue;

/****************************************************************************
Private Variables Declarations
*/

extern tEventsQueue casual_events_queue;

/****************************************************************************
Private Function Prototypes
*/

/**
 * \brief Gets scheduled expiry time for the given timer id
 *
 * \param[in] timer_id event Id for which expiry time is required
 *
 * \return The scheduled expiry time for the event
 *
 * \note This function is used mainly from timer test applications. In the case of casual
 * events, the expiry time returned is the earliest time.
 */
extern TIME get_timer_expiry_time(tTimerId timer_id);

/**
 * \brief Gets pointer to the event the given timer id
 *
 * \param[in] timer_id event Id for which pointer is required
 *
 * \return Pointer to the timer structure
 *
 * \note This function is used mainly from timer test applications.
 */
extern tTimerStruct *get_timer_from_id(tTimerId timer_id);

#endif /* PL_TIMERS_PRIVATE_H */
