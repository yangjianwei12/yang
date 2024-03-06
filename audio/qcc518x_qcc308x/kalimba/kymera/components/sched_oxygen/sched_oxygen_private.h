/****************************************************************************
 * Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.
 ***************************************************************************/
/**
 * \file sched_oxygen_private.h
 * \ingroup sched_oxygen
 *
 * header for the internals of the scheduler
 *
 * This file contains bits of the scheduler that we don't want the
 * scheduler's users to see.
 *
 * \section ident Task and background interrupt identifiers
 *
 * The scheduler has a static array of TASKs and a static array of BG_INTs. Both
 * \c TASK and \c BGINT (background interrupts) have a field "pstk_id", which
 * is a compound identifier, consisting of:
 *  - Bits 0-7: the task/bg_int's "tskid", which is its index
 *  into the static array
 *  - Bits 8-12: the task/bg_int's priority
 *  - Bit 16: set if the task/bg int is coupled to a bg int/task
 *  - Bit 24: set if a bg int, unset if a task
 *  - Other bits should be set to 0.
 *
 * \section bg_ints Background interrupt behaviour
 *
 * For historical reasons, background interrupts are slightly over-engineered.
 * There are basically two modes of operation, the first originating in the
 * Audio subsystem firmware, the second reflecting the behaviour of CarlOS-style
 * bg ints.
 *
 * \subsection audio_bg_ints Audio-style background interrupts
 * In this approach, there is one bg int handler per task, but it is passed a
 * bitmap which allows up to 16 logical bg ints
 * to be multiplexed to the same "physical" bg int.  Previously, this approach
 * was hard-coded into the scheduler's implementation, with the result that
 * bg ints were always raised on a \e task ID.  Now, \c raise_bg_int() logically
 * takes a \e bg \e int ID, but so long as the convention is followed that the
 * bg int coupled to a task has the same ID as the task apart from bit 22
 * (\c BG_INT_FLAG_BIT), it is possible still to pass a task ID to this
 * function and the right thing happen.  Clearly, it is up to the task geneation
 * mechanism to ensure that this "coupling" correspondence holds, if desired.
 * The dynamic task creation functionality does this based on whether both a
 * message and bg int handler are supplied or just one or the other.
 *
 * \subsection carlos_bg_ints CarlOS-style background interrupts
 * In the CarlOS scheme, there is no distinction between logical and physical
 * bg ints; in other words, each logical bg int is explicitly named and has its
 * own handler.  The relationship between bg ints and tasks is purely
 * conventional, and is usually communicated by suitable naming (e.g. the
 * "submsg" task has two associated bg ints - "submsg_rx" and "submsg_tx").
 *
 * \subsection bg_int_handler BG INT handler
 * In any case, there is no constraint on the behaviour of the bg int handler in
 * terms of its ability to pass messages: it can post to any queue, just as
 * any other function can (since in fact it is just another function).
 *
 * \section tasks Task behaviour
 *
 * Each task owns an array of message queues - also statically
 * defined. This array is not null-terminated - its length is held in
 * the TASK structure. Each task must have between 1 and 256 message
 * queues.
 *
 * Any task may send messages to any queue. Only the task which owns
 * a queue may consume messages from it.
 */

#ifndef SCHED_OXYGEN_PRIVATE_H
#define SCHED_OXYGEN_PRIVATE_H

/****************************************************************************
Include Files
*/
#include "sched_oxygen/sched_oxygen.h"
#include "sched_oxygen/sched_count.h"
#if defined(INSTALL_THREAD_OFFLOAD)
#include "sched_oxygen/sched_oxygen_for_thread.h"
#endif
#include "sched_oxygen/sched_oxygen_for_timers.h"
#include "panic/panic.h"
#include "pmalloc/pmalloc.h"
#include "audio_log/audio_log.h"
#include "pl_timers/pl_timers_for_sched.h"
#ifdef DORM_MODULE_PRESENT
#include "dorm/dorm.h"
#endif
#include "patch/patch.h"
#include "platform/pl_interrupt.h"
#include "platform/pl_intrinsics.h"
#include "platform/pl_trace.h"
#include "platform/pl_assert.h"
#include "platform/profiler_c.h"
#include "proc/proc.h"

#ifdef CHIP_BASE_CRESCENDO
#ifndef TODO_CRESCENDO
/* Remove log trace macros.  We'll come up with a better solution for Crescendo
 * when we have to deal with a similar thing in Synergy */
#define PL_PRINT_P1(tr, string, arg)
#define PL_PRINT_P2(tr, string, arg1, arg2)
#define PL_PRINT_P3(tr, string, a1, a2, a3)
#endif /* TODO_CRESCENDO */
#endif /* CHIP_BASE_CRESCENDO */

/****************************************************************************
Public Macro Declarations
*/

/**
 * The maximum number of messages.
 */
#define MAX_NUM_MESSAGES (100)

/* Use the natural word size for the tskid, even though it's only allowed to
 * range up to 127, because it's more efficient in the processor */
typedef uint16f tskid;

/** BG int IDs are distinguished from task IDs by having the uppermost bit of
 * the third octet set (meaning that the priority flag is reduced to 7 bits,
 * but that's more than enough).
 */
#define BG_INT_FLAG_BIT (1U << 24)
#define ID_IS_BG_INT_ID(id) ((id) & BG_INT_FLAG_BIT)

/**
 * For convenience we record whether a task/bg int is coupled with a bg int/
 * task by setting this bit
 */
#define BG_INT_TASK_IS_COUPLED_FLAG_BIT (1 << 16)
#define IS_COUPLED(id) ((id) & BG_INT_TASK_IS_COUPLED_FLAG_BIT)

/**
 * Find priority from given task/queue id
 */
#define PRIORITY_MASK 0x1F
#define PRIORITY_LSB_POS 8
#define GET_TASK_PRIORITY(x) ((uint16f)(((x) >> PRIORITY_LSB_POS) & PRIORITY_MASK))

/**
 * Find a tskid from its taskid.
 */
#define TSKID_MASK 0xffU
#define TASKID_TO_TSKID(q) (tskid)((q) & TSKID_MASK)

extern volatile uint16f TotalNumMessages;

/****************************************************************************
Public Type Declarations
*/

/**
 * TASK - The information for a system task.
 *
 * TASKs defined at compile time (static tasks) are held in an array tasks[]
 * created by autogeneration magic.
 * The scheduler holds all defined TASKs (static and dynamic) in a linked list
 * corresponding to the task priority.
 */
typedef struct _TASK
{
    taskid id; /**< priority + tskid of the task. Used as key to search
                          and identify task */
    bool prunable; /**< State if this is a dynamic task and can be
                                deleted*/
    TASK_FN_PTR init; /**< Initialisation function. May be NULL */
    TASK_FN_PTR handler; /**< Message handler. May be NULL */
    MSGQ mqueue;   /**< Task's message queue. */
    void *priv; /**< Private data for the task handler */
    struct _TASK  *next; /**< Pointer to the next task in a linked list */
} TASK;

/**
 * Definition of a static task.
 */
typedef struct
{
    PROC_BIT_FIELD proc_mask; /**< Bitfield indicating which processors should
                                   run the task. */
    PRIORITY priority;        /**< priority of the task. Used as key to
                                   search and identify task */
    uint8 tskid;              /**< tskid of the task. Used as key to
                                   search and identify task */
    uint8 padding;
    TASK_FN_PTR init;         /**< Initialisation function. May be NULL */
    TASK_FN_PTR handler;      /**< Message handler. May be NULL */
} STATIC_TASK;

extern const STATIC_TASK static_tasks[];

/**
 * Task queue
 */
typedef struct
{
    /** Pointer to first task in the Queue; Set to NULL if Q is empty */
    TASK  *first;
    /** Number of messages in priority */
    volatile int num_msgs;
    /** The number of tasks marked for delete */
    volatile uint16f prunable;
    /** Deletion lock. If this is non-zero, tasks can't be deleted, only marked
     * for deletion */
    uint16f locked;
} TASKQ;

/** Information about a background interrupt. */
typedef struct _BGINT {
    /** BG int ID. Same as a task ID, except that bit 22 is set */
    taskid                  id;
    bool                    prunable; /**< Indicates if this is a dynamic task
                                            that can be deleted*/
    /** bg_int service function. */
    bg_int_fn               handler;
    /** Boolean indicating the BG int has been raised */
    bool                    raised;
    /** Pointer to either the associated task's or the local private memory
     * pointer. */
    void                    **ppriv;
    struct _BGINT           *next;
} BGINT;

typedef struct _UNCOUPLED_BGINT {
    BGINT base; /* The real BGINT struct.  MUST BE THE FIRST ELEMENT IN THIS
    STRUCTURE */
    void *priv; /* Local pointer to private memory */
} UNCOUPLED_BGINT;

/**
 * Definition of a static background interrupt.
 */
typedef struct
{
    PROC_BIT_FIELD proc_mask; /**< Bitfield indicating which processors should
                                   run the bgint. */
    PRIORITY priority;        /**< priority of the task. Used as key to
                                   search and identify bgint */
    uint8 tskid;              /**< tskid of the task. Used as key to
                                   search and identify bgint */
    uint8 padding;
    bg_int_fn handler;        /**< Message handler. May be NULL */
} STATIC_BGINT;

extern const STATIC_BGINT static_bgints[];

typedef struct
{
    BGINT *first;
    volatile int num_raised;
    /** The number of tasks marked for delete */
    volatile uint16f prunable;
    /** Whether the queue is currently locked or not */
    uint16f locked;
} BG_INTQ;

/****************************************************************************
Global Variable Definitions
*/

/**
 * Set by interrupt handler to indicate IRQ is active. Used by scheduler to
 * disable context switch when put_message is called from within IRQ
 */
extern int SchedInterruptActive;

/****************************************************************************
Public Function Prototypes
*/

/* Function called on exit from IRQ in interrupt.asm
 * to check for a possible context switch
 */
extern void exit_irq_check_context_switch(void);

/**
 * Wake up the background, if special action is needed to do
 * so.
 */
#if defined(DORM_MODULE_PRESENT) && !defined(UNIT_TEST_BUILD)
#define sched_wake_up_background() dorm_wake()
#else
#define sched_wake_up_background() ((void)0)
#endif /* DORM_MODULE_PRESENT && !UNIT_TEST_BUILD */

#endif   /* SCHED_OXYGEN_PRIVATE_H */
