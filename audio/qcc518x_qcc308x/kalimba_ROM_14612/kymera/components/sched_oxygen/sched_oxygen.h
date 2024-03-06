/****************************************************************************
 * Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.
 ***************************************************************************/
/**
 * \defgroup sched_oxygen Scheduler
 * \file sched_oxygen.h
 * \ingroup sched_oxygen
 *
 * Header file for the kalimba scheduler
 *
 * \section basic Basic concepts
 * The system scheduler gives structure to the bulk of the code in the
 * system. It organises the code as a set of tasks, and provides means
 * for communication between the tasks.
 *
 * Each task has two main elements: a set of message queues and a
 * message handler function. The message queues accumulate messages to
 * be consumed by the message handler. The message handler consumes
 * messages from its queues and acts upon them.
 *
 * Each message has two elements: a uint16 and a void* pointer. Much
 * of the time the uint16 carries some form of signal ID and the
 * void* points to the message body. If a message body is not
 * required, the void * pointer can be NULL. The scheduler does not examine
 * or alter either of these elements of each message.
 *
 * The core of the scheduler inspects each task's set of message queues
 * and calls the task's handler if there is any message in any
 * of the task's queues.
 *
 * A task's message handler can post to any tasks' queues, but it can
 * consume messages only from its own queues.
 *
 * \section preemption Pre-emption
 *
 * So far, the same as CarlOS. However this scheduler also supports
 * priorities for each task, and "simple pre-emption".
 *
 * "Simple pre-emption" means that at any of a defined set of
 * potential context switch points, the scheduler examines if any task(s)
 * of a higher priority than the current task has been made ready to run.
 * If so, a call will be made to the higher priority task(s) before we
 * return to the currrent task.
 *
 * The only "potential context switch points" are currently:
 *  1) In put_message() after the message has been added to the queue
 *  2) On return from an interrupt, to check if the ISR has signalled
 *     a higher priority task.
 *
 * KEY POINT: Every task handler function must run to completion, else
 * no lower priority task will ever run. Note this means no task should
 * block, as if it does no lower priority task will run till the block
 * is released. Hence mutexes and semaphores are not implemented in
 * this scheduler.
 *
 * The main gain of this approach, rather than a more conventional
 * RTOS, is that the system only requires one stack. This should
 * help constrain the RAM consumption of this embedded application.
 *
 * \section task_creation Task and Background interrupt creation
 *
 * This scheduler supports both statically and dynamically created tasks. Static
 * task generation must somehow arrange for two arrays, \c tasks and \c bg_ints
 * to be populated and for corresponding enumerations of task, bg int and queue
 * IDs to be created.  If task/bg int coupling is not required, CarlOS-style
 * autogeneration is available provided the build system provides suitable
 * support.  This requires modules wishing to register tasks and/or background
 * interrupts to define special macros containing the requisite details.
 * Dynamic task generation uses the \c create_task, \c delete_task
 * interface to dynamically handle coupled and uncoupled tasks and
 * background interrupts.
 *
 * The scheduler stores a void* pointer for the internal use of each
 * task.   The scheduler makes no use of the pointer except to pass it
 * as an argument to each invocation of the task's message handler (or the
 * bg int handler). This allows the system to maintain data between invocations
 * of the handler rather than in static data.  In coupled cases, the bg int
 * stores a pointer to the task's pointer, meaning not only that the memory
 * is shared but that either can safely change its allocation.
 *
 * Each task can have an initialisation function which is called exactly
 * once before any task's main function is called. The initialisation
 * function is passed the same void* pointer that is passed to the main
 * task function.
 *
 */

#ifndef SCHED_OXYGEN_H
#define SCHED_OXYGEN_H

/****************************************************************************
Include Files
*/

#include "pl_timers/pl_timers.h" /* For tTimerId */
#include "sched_oxygen/sched_ids.h"

/****************************************************************************
Public Macro Declarations
*/

/**
 * Special value of taskid used to indicate no task
 */
#define NO_TASK ((taskid) 0)

/****************************************************************************
Public Type Declarations
*/

/**
 * Function pointer matching the prototype of a task's init function and
 * its message hander function.
 */
typedef void (*TASK_FN_PTR)(void **context);

/**
 * Background interrupt handler type.
 *
 * @param priv Pointer to the partner task's private memory area.  This *must*
 * be ignored if the bg int does not have a designated partner task.
 *
 * Note that CarlOS-style background interrupt handlers differ slightly in form
 * and function from Audio-style handlers.  Firstly, they don't take the
 * private memory pointer, since by definition they are not partnered with a
 * task; secondly, they have no notion of multiplexing of logical bg ints - they
 * are either raised or not - and as a result, thirdly, they expect the
 * scheduler to clear their flag and hence don't do it themselves.
 */
typedef void (*bg_int_fn)(void **priv);

/** An identifier issued by the scheduler. */
typedef uint24   scheduler_identifier;

/** Type for storing the task identifier. */
typedef uint32 taskid;

/** An Opaque pointer type for exposing a BGINT structure externally */
typedef void* BGINT_TASK;


/** Line length reducer. */
#define schid   scheduler_identifier
#define NO_SCHID  ((schid)(0))

/**
 * A message identifier.
 */
typedef schid msgid;

/** Active msgids are non-zero, so zero signals "not a message id". */
#define NO_MSGID  ((msgid)NO_TID)

/** Time value indicating sched doesn't care, make up your own value. */
#define SCHED_MAX_DELAY  ((TIME)(10 * (MINUTE)))

/**
 * tRoutingInfo - Message routing details
 */

typedef struct
{
    unsigned int src_id;  /**< Source ID for routing. */
    unsigned int dest_id;  /**< Destination ID for routing. */
} tRoutingInfo;

/**
 * MSG - A message
 */
typedef struct msg_tag
{
    struct msg_tag *next; /**< Next in linked list. */
    uint16 mi; /**< The message's uint16. */
    void *mv; /**< The message's void*. */
    msgid id; /**< The message's identifier. */
    tRoutingInfo routing;
} MSG;

/**
 * MSGQ - A message queue.
 */
typedef struct
{
    MSG *first; /**< Pointer to the first message in the Q. Set to NULL if Q is
                 empty */
} MSGQ;

/**
 * PRIORITY - A priority level used by the scheduler.
 *
 * Used to decide whether a put_message call from one task to another should
 * interrupt the calling task.
 * NOTE - before adding priority levels, ensure there are at least
 * NUM_PRIORITIES bits in CurrentPriorityMask
 */
typedef enum
{
    LOWEST_PRIORITY = 0,
    LOW_PRIORITY,
    MID_PRIORITY,
    HIGH_PRIORITY,
    HIGHEST_PRIORITY,
    NUM_PRIORITIES,
    INVALID_PRIORITY = 0xFF
} PRIORITY;

/**
 * Tasks inherited from CarlOS have no notion of priority, so we define a
 * default priority that they will run at if their definition is not altered.
 */
#define DEFAULT_PRIORITY LOWEST_PRIORITY

/**
 * Generate a background interrupt.
 *
 * Standard way of calling the above.
 */
#define GEN_BG_INT(x) raise_bg_int((taskid)(x ## _bg_int_id))

/****************************************************************************
Public Function Prototypes
*/

/**
 * \brief initialise the scheduler
 */
extern void init_sched(void);

/**
 * Stop the scheduler
 */
extern void sched_stop(void);

/**
 * \brief The main function of the background task scheduler. This invokes
 * tasks as messages become available for them.
 *
 * \return Doesn't return, except for DESKTOP_TEST_BUILD builds
 */
extern void sched(void);

/**
 * \brief Sends a message consisting of the integer mi and the void* pointer
 * mv to the message queue queueId owned by a task. mi and
 * mv are neither inspected nor changed by the scheduler - the task that owns
 * queueId is expected to make sense of the values.
 *
 * \param[in] task_id      ID of the task to send the message to.
 * \param[in] messageInt   Int associated with the message.
 * \param[in] pMessageBody Pointer to the message. May be NULL.
 * \param[in] routing      Pointer to the associated routing information. May be NULL.
 *
 * \return A message identifier.
 *
 * \note If mv is not null then it will typically be a chunk of malloc()ed
 * memory, though there is no need for it to be so.   Tasks should normally obey the
 * convention that when a message built with malloc()ed memory is given to
 * put_message() then ownership of the memory is ceded to the scheduler - and
 * eventually to the recipient task.   I.e., the receiver of the message will be
 * expected to free() the message storage.
 *
 * \note Note also that this function must be re-entrant - a call to this function
 * may cause another task to run which itself may call this function.
 */

extern msgid put_message_with_routing(taskid task_id, 
                                      uint16 messageInt, 
                                      void *pMessageBody,
                                      tRoutingInfo *routing);

#define put_message(q, mi, mb) put_message_with_routing(q, mi, mb, NULL)

/**
 * \brief Send a message at a specific time in the future.
 * This basically wraps put_message, so refer there for information.
 *
 * \param[in] task_id       ID of the queue to send the message to
 * \param[in] mi    Int associated with the message
 * \param[in] mv  Pointer associated with the message. May be NULL.
 * \param[in] deadline      Time at which to send the message.
 *
 * \return  A timer ID. This can be used with timer_cancel_event if necessary.
 */
extern tTimerId put_message_at(TIME deadline,
                               taskid task_id,
                               uint16 mi,
                               void *mv);

/**
 * \brief Send a message after a delay.
 * This basically wraps put_message, so refer there for information.
 *
 * \param[in] delay         Send the message after this amount of time.
 * \param[in] task_id       ID of the queue to send the message to
 * \param[in] mi    Int associated with the message
 * \param[in] mv  Pointer associated with the message. May be NULL.
 *
 * \return  A timer ID, or TIMER_ID_INVALID if the delay was so short that
 * the message was put onto the queue immediately.
 */
extern tTimerId put_message_in(TIME_INTERVAL delay,
                               taskid task_id,
                               uint16 mi,
                               void *mv);

/**
 * \brief Raise a background interrupt to required bg interrupt if not already
 * raised.
 *
 * \param[in] task_id      ID of the bg_int to send the message to.  If the
 * task generation mechanism supports pairing tasks and bg_ints, it is legal to
 * pass the paired task's taskid here instead.
 *
 * \return Nothing.
 *
 * \note
 * This function is usually called within an interrupt handler to signal an
 * event to a background task.
 */
extern void raise_bg_int(taskid task_id);

/**
 * \brief Raise a background interrupt to required bg interrupt if not already
 * raised.
 *
 * \param[in] bg_int      The background interrupt to raise a bg interrupt on.
 *
 * \return Nothing.
 *
 * \note
 * This function is usually called to kick an operator in a very efficient way.
 * This function performs no safety checks and the caller is responsible for
 * managing the validity of bg_int over the lifetime of the operator task.
 */
extern void raise_bg_int_with_bgint(BGINT_TASK bg_int);

/**
 * \brief Obtains a message from the message queue ID queue_id if one is
 * available. The calling task must own this Q. The message consists of one or
 * both of a int and a void*.
 *
 * \param[in,out] *pmi   Integer from the message, if pointer is not NULL
 *
 * \param[in,out] *pmv Pointer associated with the message, if
 * pmv is not NULL.
 *
 * \return TRUE if a message has been obtained from the queue, else FALSE.
 *
 * \note
 * See the note on malloc()ed memory ownership in the description of \ref
 * put_message .
 *
 * \par
 * If a message is taken from the queue, then *pmi and *pmv
 * are set to the mi and mv passed to put_message().
 *
 * \par
 * pmi and/or pmv can be null, in which case the corresponding value from the
 * message is discarded.
 *
 * \par
 * If the function returns TRUE, and if pmv is null, then there is a
 * danger of a memory leak.  The scheduler does not know what is stored
 * in the void*, so it cannot reclaim a chunk of malloc()ed memory
 * there.   In most circumstances pmv should not be null; however,
 * it may be that the application knows that all messages sent on "q" will use
 * the "MessageInt" only, so it may be acceptable for pmv to be null.
 *
 * \par
 * MUST NOT BE CALLED DIRECTLY FROM INTERRUPT CODE - can only be called by the
 * task that owns the queue given by queueId.
 */
extern bool get_message_with_routing(uint16 *pmi,
                                     void **pmv,
                                     tRoutingInfo *routing);

#define get_message(q, mi, mb) get_message_with_routing(mi, mb, NULL)

/**
 * \brief Creates a new task and/or bg int with the given parameters,
 * initialises the new task and returns the task id.
 *
 * If \c bg_int_handler is \c NULL, creates a standalone (uncoupled) task. If
 * \c msg_handler is NULL, the function creates
 * an uncoupled bg int.  If both handlers are supplied, the function creates a
 * coupled bg int/task pair.  In this case the id returned is the task's; the
 * bg int's differs from this only in bit 23.
 *
 * \param[in]     task_priority  Priority of the new task
 * \param[in]     task_data      Task-specific data (see below)
 * \param[in]     msg_handler    Message handler function to handle messages
 * received in the message queues associated with the task
 * \param[in]     bg_int_handler Background interrupts handling function to
 * handle background interrupts
 * \param[in]     flush_msg      Function to flush any queued messages to the
 * task when the task is deleted. Could be NULL
 * \param[in,out] created_task_id   Pointer to return the task id in if the 
 * creation is successful
 *
 * \return TRUE if the creation succeeded, FALSE if there was insufficient 
 * memory.to create the task.
 *
 * \note The task id of the newly created task is used to reference the 
 * task to send messages or background interrupts.  Note that in the coupled
 * case, while the bg int has its own ID, it is possible to raise bg ints using
 * the task's ID.
 *
 * \par \c task_data is specific to the task and is passed to all the handler
 * functions for the task. The scheduler does not do anything with it. The task
 * user should allocate memory if it is required and free it when the task is
 * deleted.
 *
 * \par If \c flush_msg routine is not provided and the scheduler tries to
 * reclaim \c pmv for any messages that are queued to the task that is being
 * deleted, then there is a danger of a memory leak.  The scheduler does not
 * know what is stored in the \c void*, so it cannot reclaim a chunk of
 * \c malloc()ed memory there. It may be that the application knows that all
 * messages sent on "q" will use the "MessageInt" only, or that there will be
 * no messages in the queue when the task delete is called, so it may be
 * acceptable for \c flush_msg to be \c NULL.
 */
extern taskid create_task(PRIORITY task_priority,
                          void *task_data,
                          TASK_FN_PTR msg_handler,
                          bg_int_fn bg_int_handler);

/**
 * Helper macro that automatically sets redundant \c create_task arguments for
 * creating an uncoupled bg int
 */
#define create_uncoupled_bgint(task_priority, task_data, bg_int_handler) \
    create_task(task_priority, task_data, NULL, bg_int_handler)

/**
 * \brief Deletes a scheduler task that was previously created
 *
 * \param[in] id       task Id which is to be deleted
 */
extern void delete_task(taskid id);

#ifdef UNIT_TEST_BUILD
extern void pl_verify_queue(taskid task_id, bool is_empty);

#define verify_queue_is_empty(q) pl_verify_queue(q, TRUE)
#define verify_queue_is_not_empty(q) pl_verify_queue(q, FALSE)
#endif

/**
 * Return the ID of the current task
 */
extern taskid get_current_task(void);

/**
 * Return the priority of the current task
 */
extern PRIORITY get_current_priority(void);

/**
 * \brief Allows a user to get the Background interrupt structure
 * for a task. This is for use with raise_bg_int_with_bgint. The
 * returned bgint becomes invalid when the related task is deleted
 * and should be no longer used.
 * 
 * \param task_id ID of the bg_int to send the message to.  If the
 * task generation mechanism supports pairing tasks and bg_ints, it is legal to
 * pass the paired task's taskid here instead.
 * 
 * \param bgint Location to return the bg structure.
 * 
 * \return TRUE if found. FALSE if not found.
 */
extern bool sched_find_bgint(taskid task_id, BGINT_TASK *bgint);

/**
 * \brief Cleanup cached message(s). Can be called on any processor,
 * but useful when using leak finder on aux (secondary) 
 * processorto not report these memories.
 */
extern void sched_clear_message_cache(void);

/**
 * \brief Returns true if the processor is currently running in
 * interrupt context.
 * \note This is provided to facilitate optimisations which may be possible if
 * the code can't be interrupted in the current context.
 */
extern bool is_current_context_interrupt(void);

/* \brief Does the scheduler have some work to do?
 *
 * \return TRUE when it does
 *
 * \note This is useful to decide whether the subsystem can go to abort
 *       low power consumption state.
 */
extern bool sched_busy(void);

/**
 * NAME
 *   sched_pack_taskid
 *
 * \brief Return the tskid of input parameter 'id'.
 *
 * \return The tskid of of input parameter 'id'.
 */
uint8 sched_pack_taskid(taskid id);

/**
 * NAME
 *   sched_get_packed_task
 *
 * \brief Return the tskid of the current task.
 *
 * \return The tskid of the current task.
 */
uint8 sched_get_packed_task(void);

#endif   /* SCHED_OXYGEN_H */
