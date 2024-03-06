/****************************************************************************
 * Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.
 ***************************************************************************/
/**
 * \file sched_oxygen.c
 * \ingroup sched_oxygen
 * Kalimba scheduler - see header file for full description
 */
#include "sched_oxygen/sched_oxygen_private.h"

/****************************************************************************
Private Macro Declarations
*/

/**
 * We use a common source of "unique" identifier numbers.
 */
#define GET_MSGID() (msgid) get_scheduler_identifier()

/**
 * Construct a basic 24-bit taskid from task/bgint index (LS octet) and task
 * priority (lower 6 bits of MS octet).  The middle octet is always zero for
 * task IDs. The MS two bits are used to flag bg ints and coupled tasks.
 */
#define SET_PRIORITY_TASK_ID(task_priority,index) (task_priority << PRIORITY_LSB_POS | index)

/**
 * Clear the bit indicating that the taskid refers to a bg int
 */
#define MARK_AS_TASK(id) ((id) &= ~BG_INT_FLAG_BIT)

/**
 * Set the bit indicating that the taskid refers to a bg int
 */
#define MARK_AS_BG_INT(id) ((id) |= BG_INT_FLAG_BIT)

/**
 * Set the bit indicating that the taskid refers to a task/bg int that is
 * "coupled" to a bg int/task.
 */
#define MARK_AS_COUPLED(id) ((id) |= BG_INT_TASK_IS_COUPLED_FLAG_BIT)

/**
 * Maximum possible task index within a priority level
 */
#define MAX_TASK_INDEX    255

/**
 * \brief Macro to find the priority level of the highest priority task
 * from a priority mask.
 *
 * Assumes Mask is an unsigned int and is greater than 0.
 * Also due to how signdet works Mask cannot have its MSB set. This
 * restriction is fine as long as the number of bits in the mask is greater
 * than NUM_PRIORITIES, which is checked with an assertion in init_sched.
 *
 */
#ifndef DESKTOP_TEST_BUILD
#define MAP_PRIORITY_MASK_TO_HIGHEST_LEVEL(Mask) \
    ((uint16f)((UINT_BIT - 2) - (unsigned)PL_SIGNDET(((int)(Mask)))))
#else
#define MAP_PRIORITY_MASK_TO_HIGHEST_LEVEL(Mask) \
                                    map_priority_mask_to_highest_level(Mask)
static uint16f map_priority_mask_to_highest_level(unsigned int mask)
{
    uint16f highest = UINT_BIT - 1;
    while (! ((1 << highest) & mask))
    {
        highest--;
    }
    return highest;
}

#endif /* DESKTOP_TEST_BUILD */

/**
 * \brief Macro to calculate a priority mask from a priority level.
 * The mask calculated has the bits set for the priority supplied and all
 * lower priorities.
 *
 * Again assumes the number of bits in a unit24 is greater than NUM_PRIORITIES
 * (checked in init_sched)
 *
 */
 #define GET_PRIORITY_MASK(priority) \
                                ((uint16f) ((((uint16f) 1) << (priority + 1)) - 1))

/**
 * \brief Minimum delay for put_message_in() -- if less than this, just
 * PutMessage()
 */
#define MIN_PUT_MESSAGE_IN_DELAY ((INTERVAL) 10)

#define TASK_LIST_WAS_ALREADY_LOCKED(queue) (queue->locked > 1)

#define BGINT_LIST_WAS_ALREADY_LOCKED(queue) (queue->locked > 1)

#ifdef UNIT_TEST_BUILD
#define sched_tag_dm_memory(ptr, id)
#else
#define sched_tag_dm_memory(ptr, id)         tag_dm_memory(ptr, id)
#endif

/****************************************************************************
Private Variable Definitions
*/

/* TODO - make these private variables static. But until Xide can display
 * statics from outside the current scope, its useful having them global */

/**
 * The identifier of the current task or bg_int.  Bit 22 indicates which this
 * is; we use this to check that \c get_message is
 * being called in the right context.
 */
taskid *current_id;

/**
 * Number of messages and background interrupts in the system.
 */
volatile uint16f TotalNumMessages;

/**
 * Bit field representing priority of tasks that are ready to run, but have not
 * yet run. LSB represents LOWEST_PRIORITY from PRIORITY enum.
 * NOTE: The width of the type must be known in order for MAP_PRIORITY_MASK_TO_
 * HIGHEST_LEVEL to work.  But we also want to maximise efficiency.  So we use
 * unsigned int and rely on the macro UINT_BIT (defined in hydra_types.h) to
 * give us the width.
 */
unsigned int CurrentPriorityMask;

/**
 * Source of scheduler identifiers.  Init to 1; value 0 is special (NO_SCHID)
 */
scheduler_identifier NextSchedulerIdentifier = 1;

/**
 * Pointer to a previously relinquished message in order to optimise the
 * next call to put_message(). Avoides repeated calls to malloc and free as
 * messages are created and released
 */
static MSG *pCachedMessage = (MSG *) NULL;

TASKQ tasks_in_priority[NUM_PRIORITIES];
BG_INTQ bg_ints_in_priority[NUM_PRIORITIES];

#ifdef DESKTOP_TEST_BUILD
int SchedInterruptActive = 0;
#endif

/** A flag to allow us to exit a runlevel */
static bool run = FALSE;

/** Record that a casual wakeup timer fired */
static bool casual_wakeup_pending = FALSE;


#if !defined(DISABLE_SHALLOW_SLEEP) && defined(PROFILER_ON)
/* Scheduler specific profiler. Mark the scheduler loop as started because the
   first call for this (and only this) profiler is stop. */
static profiler sched_loop = STATIC_PROFILER_INIT(UNINITIALISED_PROFILER, 0, 0, 1);
#endif

/****************************************************************************
Private Function Prototypes
*/

/****************************************************************************
Private Function Definitions
*/

static void clean_priority(uint16f priority_index)
{
    if (0 == tasks_in_priority[priority_index].num_msgs &&
        0 == bg_ints_in_priority[priority_index].num_raised)
    {
        /* If there are no more messages in this priority, clear the current
           priority masks. */
        CurrentPriorityMask &= (uint16f) (~(((uint16f) 1 << priority_index)));
    }
}

static inline void lock_task_list_ints_blocked(TASKQ *queue)
{
    queue->locked++;
}

static inline void lock_task_list(TASKQ *queue)
{
    interrupt_block();
    lock_task_list_ints_blocked(queue);
    interrupt_unblock();
}

/**
 * \brief Clears all messages associated with the given task.  Note that if
 * a coupled task+bg int is being deleted, \c flush_bg_ints must be called too.
 *
 * \param[in] p_task pointer to the task structure
 *
 * \note This function should only be called with all the interrupts blocked,
 * ie, the whole operation should be atomic.
 */
static void flush_task_messages(TASK *p_task)
{
    MSGQ *pQueue;
    uint16f priority_index;
    TASKQ *taskq;

    priority_index = GET_TASK_PRIORITY(p_task->id);
    taskq = &tasks_in_priority[priority_index];

    pQueue = &p_task->mqueue;
    while (pQueue->first)
    {
        MSG *pMessage = pQueue->first;
        /* If we've found a message, sched_n_messages should be > 0 */
        if ((TotalNumMessages == 0) || (taskq->num_msgs <= 0))
        {
            panic(PANIC_OXYGOS_INVALID_MESSAGE_COUNT);
        }
        /* remove the message from the queue */
        pQueue->first = pMessage->next;
        TotalNumMessages--;
        taskq->num_msgs--;
        if (NULL != pMessage->mv)
        {
            /* Try and reclaim the body as it is. This would at least
             * reduce the memory leak amount
             */
            pfree(pMessage->mv);
        }

        if (pCachedMessage == (MSG *) NULL)
        {
            /* put the message in the cache */
            pCachedMessage = pMessage;
        }
        else
        {
            pfree((void *) pMessage);
        }
    } /* as long as there is a message in the queue */

    clean_priority(priority_index);
}

/**
 * Physically delete tasks that have been logically deleted while
 * their list was locked
 * \param queue A pointer to the queue to prune.
 *
 * \note This function is intended to be called only from the macro
 * \c unlock_task_list.  It must be called with interrupts blocked.
 */
static void prune_tasks(TASKQ *queue)
{
    TASK *t, **tposition;

    tposition = &queue->first;
    while ((t=*tposition) != NULL && queue->prunable)
    {
        if (t->prunable)
        {
            flush_task_messages(t);
            /* update the list */
            *tposition = t->next;
            PL_PRINT_P1(TR_PL_SCHED_TASK_DELETE,
                    "Task 0x%08x deleted\n", t->id);
            pfree(t);
            queue->prunable--;
        }
        else
        {
            tposition = &(*tposition)->next;
        }
    }
    PL_ASSERT(queue->prunable == 0);
}

static inline void unlock_task_list_ints_blocked(TASKQ *queue)
{
    if (queue->locked <= 1)
    {
        prune_tasks(queue);
    }
    queue->locked--;
}

static inline void unlock_task_list(TASKQ *queue)
{
    interrupt_block();
    unlock_task_list_ints_blocked(queue);
    interrupt_unblock();
}

static inline void lock_bgint_list_ints_blocked(BG_INTQ *queue)
{
    queue->locked++;
}

static inline void lock_bgint_list(BG_INTQ *queue)
{
    interrupt_block();
    lock_bgint_list_ints_blocked(queue);
    interrupt_unblock();
}

/**
 * Cleanly remove all traces of a bg int from the scheduler's internals before
 * it is deleted.
 *
 * @param p_bgint Pointer to the \c BGINT to be flushed.
 *
 * \note This function should only be called with all the interrupts blocked,
 * ie, the whole operation should be atomic.
 */
static void flush_bg_ints(BGINT *p_bgint)
{
    uint16f priority_index = GET_TASK_PRIORITY(p_bgint->id);

    /* Check if there are any background interrupts raised for the task
     * and reduce the number of messages in system accordingly
     */
    if (p_bgint->raised)
        {
            /* If we've found a message, sched_n_messages should be > 0 */
            if ((TotalNumMessages == 0) ||
                (bg_ints_in_priority[priority_index].num_raised <= 0))
            {
                panic(PANIC_OXYGOS_INVALID_MESSAGE_COUNT);
            }
            TotalNumMessages--;
            bg_ints_in_priority[priority_index].num_raised--;
        p_bgint->raised = FALSE;
        }

    clean_priority(priority_index);
}

/**
 * Physically delete bg ints that have been logically deleted while
 * their list was locked
 * \param queue A pointer to the queue to prune.
 *
 * \note This function is intended to be called only from the macro
 * \c unlock_bgint_list.  It must be called with interrupts blocked.
 */
static void prune_bg_ints(BG_INTQ *queue)
{
    BGINT *b, **bposition;

    bposition = &queue->first;
    while ((b=*bposition) != NULL && queue->prunable)
    {
        if (b->prunable)
        {
            /* Some one tried to delete this while bg int was running.
             * It is safe to delete now.
             */
            flush_bg_ints(b);
            /* update the list */
            *bposition = b->next;
            PL_PRINT_P1(TR_PL_SCHED_TASK_DELETE,
                        "Bg int 0x%08x deleted\n", b->id);
            pfree(b);
            queue->prunable--;
        }
        else
        {
            bposition = &(*bposition)->next;
        }
    }
    PL_ASSERT(queue->prunable == 0);
}

static inline void unlock_bgint_list_ints_blocked(BG_INTQ *queue)
{
    if (queue->locked <= 1)
    {
        prune_bg_ints(queue);
    }
    queue->locked--;
}

static inline void unlock_bgint_list(BG_INTQ *queue)
{
    interrupt_block();
    unlock_bgint_list_ints_blocked(queue);
    interrupt_unblock();
}

void sched_background_kick_event(void)
{
    /* Record the wakeup so we stay awake until it's been handled */
    if (!casual_wakeup_pending)
    {
        casual_wakeup_pending = TRUE;
        TotalNumMessages++;
        sched_wake_up_background();
    }
}

/**
 * \brief Switch context to higher priority tasks if needed
 *
 *   This function is the core of the scheduler. It checks to see if a context
 *   switch is needed, based on the inputPriorityMask. Calls all tasks that
 *   are pending of HIGHER priority than the priorities set in the mask.
 *
 *   Note that this function must be re-entrant - a call to it may cause a
 *   higher priority task to run which itself may end up calling this function.
 *   But in this case each recursive call will have the inputPriorityMask set
 *   higher than the previous call, and eventually the stack will unwind and
 *   we'll always end up back at the original call again.
 *
 *   This should always be called with interrupts blocked.
 *   It will unblock them if it finds anything to do.
 *
 * \param[in] inputPriorityMask bit field with each bit representing  a priority
 * level, LSB representing LOWEST_PRIORITY, etc.
 *
 */
RUN_FROM_PM_RAM
static void check_context_switch(uint16f inputPriorityMask)
{
    /* Store original task/bg int, so we can restore it before returning to
     * original activity */
    taskid *orig_id = current_id;

    patch_fn(pl_context_switch);

    /*
     * While we have any tasks with priority HIGHER than that indicated by the
     * input mask, run these tasks
     */
    while (CurrentPriorityMask > inputPriorityMask)
    {
        uint16f HighestPriorityLevel;
        BG_INTQ *bg_intq;
        TASKQ *taskq;
        /*
         * Lookup highest priority that is ready to run, and range of tasks to
         * check Qs on. Initially there must be at least one message for this
         * priority level.
         */
        HighestPriorityLevel =
            MAP_PRIORITY_MASK_TO_HIGHEST_LEVEL(CurrentPriorityMask);
        bg_intq = &bg_ints_in_priority[HighestPriorityLevel];
        taskq = &tasks_in_priority[HighestPriorityLevel];

        /* While there is a bg_int or task message at this priority keep
         * servicing them before moving to the next priority.
         */
        while ((bg_intq->num_raised > 0) ||
               (taskq->num_msgs > 0))
        {
            /* Run through all bg ints of this priority.  If any are raised,
             * run their handlers */
            if (bg_intq->num_raised > 0)
            {
                BGINT *b;

                /* Set a flag to prevent pre-empting code from actually removing
                 * bg ints from the list.  It's only strictly needed while
                 * interrupts are unblocked, but this is a more natural place to set
                 * it. */
                lock_bgint_list_ints_blocked(bg_intq);

                for (b = bg_intq->first; b != NULL; b = b->next)
                /* Check if the task has any background interrupt raised */
                {
                    if (b->raised && !b->prunable)
                    {
                        if (NULL == b->handler)
                        {
                            panic(PANIC_OXYGOS_NULL_HANDLER);
                        }

                        /* Switch to the bg_int */
                        current_id = &b->id;

                        /* clear the background interrupt and decrement the Total Messages count.
                         * The bg int count is delayed until later as it indicates whether the
                         * loop can be exited. */
                        b->raised = FALSE;
                        TotalNumMessages--;

                        /* Need to write the appropriate thing for dynamic BG ints */
                        PL_PRINT_P1(TR_PL_SCHED_TASK_RUN,
                                                "Running bg int 0x%08x\n", b->id);
                        /* Unlock IRQs and call the handler function for this task */
                        interrupt_unblock();

                        b->handler(b->ppriv);

                        interrupt_block();

                        PL_PRINT_P1(TR_PL_SCHED_TASK_RUN,
                                              "Bg int 0x%08x completed\n", b->id);

                        /* If this is the last bg_int in this priority break out of the
                         * loop as there are no more interrupts to service. This test is
                         * almost free as we need to decrement
                         * Using a predecrement in the if statement  here to help
                         * the compiler to come up with better code */
                        if (--bg_intq->num_raised == 0)
                        {
                            /* No more bg ints are raised */
                            break;
                        }

                    }
                }

                /* The queue is not in use anymore, so it's open for manipulation */
                unlock_bgint_list_ints_blocked(bg_intq);
            }

            /*
             * Run through all tasks of this priority. For each task check all
             * its Qs to see if there are any messages. If so, run the task
             * handler
             */
            if (taskq->num_msgs > 0)
            {
                TASK *t;

                /* Set a flag to prevent pre-empting code from actually removing
                 * tasks from this list. */
                lock_task_list_ints_blocked(taskq);

                for (t = taskq->first; t != NULL; t = t->next)
                {
                    if (!t->prunable)
                    {
                        MSGQ *q = &t->mqueue;

                        if (q->first != (MSG *) NULL)
                        {
                            /* Switch to the task */
                            current_id = &t->id;

                            PL_PRINT_P1(TR_PL_SCHED_TASK_RUN,
                                            "Running Task 0x%08x\n", t->id);
                            /* Unlock IRQs and call the handler function for this
                             * task */
                            interrupt_unblock();

                            t->handler(&t->priv);

                            interrupt_block();

                            PL_PRINT_P1(TR_PL_SCHED_TASK_RUN,
                                        "Task 0x%08x completed\n", t->id);
                        }
                    }

                } /* end of for loop round tasks of this priority */

                unlock_task_list_ints_blocked(taskq);
            }
        }

        /* We've run all tasks of this priority - clear the bit
         * indicating there are tasks to run */
        CurrentPriorityMask &= (uint16f) (~(((uint16f) 1 << HighestPriorityLevel)));

    }/*End of while loop checking all priorities greater than inputPriorityMask */

    /* Restore the original activity */
    current_id = orig_id;
}


/**
 * Call each task's init function (if it exists)
 */
static void init_tasks(void)
{
    unsigned n;
    TASK *task;

    /* Scan through all the priority levels... */
    for (n = 0; n < NUM_PRIORITIES; n++)
    {
        task = tasks_in_priority[n].first;

        /* ... and through all tasks at each level. */
        while (task != NULL)
        {
            /* If there is an init function for this task, run it */
            if (task->init != (TASK_FN_PTR) NULL)
            {
                current_id = &task->id;
                task->init(&task->priv);
            }
            task = task->next;
        }
    }
    current_id = NULL;
}

/**
 * \fn static scheduler_identifier get_scheduler_identifier(void)
 *
 * \brief obtain an identifier from the scheduler
 *
 * \return A scheduler identifier
 *
 * NOTES
 * Actually, this just returns the next value from an incrementing
 * uint24, but it's pretty unlikely that this will cause problems.
 *
 * It's a bit more subtle than that: if this number wraps (pretty
 * unlikely!) then we wrap to a fraction of the range of a uint24. We
 * can vaguely guess that there may be a few elements of the system that
 * get created when the system starts and which exist until the system
 * reboots.  These might have identifiers. The approach of only giving
 * away the lower identifiers once should circumvent such nasties,
 * but it clearly isn't perfect.
 *
 * Setting NextSchedulerIdentifier initially to one prevents
 * this function returning the value zero. An identifier with value
 * zero has special meanings in some circumstances (NO_SCHID).
 */
#define MAX_SCHED_ID ((scheduler_identifier)0xFFFFFFU)
#define RESTART_SCHED_ID_BASE (MAX_SCHED_ID/32)

static scheduler_identifier get_scheduler_identifier(void)
{
    scheduler_identifier i;

    interrupt_block();
    i = NextSchedulerIdentifier;
    if (++NextSchedulerIdentifier == MAX_SCHED_ID)
    {
        NextSchedulerIdentifier = RESTART_SCHED_ID_BASE;
    }
    interrupt_unblock();

    return(i);
}

/**
 * NAME
 *  put_allocated_message
 *
 * \brief queue up an allocated message
 *
 * \param[in] queueId
 * \param[in] *pMessage
 *
 * FUNCTION
 * Add an allocated message to the appropriate queue.
 * The mi and *mv fields should already have been filled in.
 *
 * \return The msgid for the message.
 */
static msgid put_allocated_message(taskid task_id, MSG *pMessage)
{
    uint16f priority_index;
    MSGQ *pQueue;
    TASK *pTask;
    msgid id;
    TASKQ *taskq;

    /* Check for valid tskid and queue number, and a valid message pointer */
    priority_index = GET_TASK_PRIORITY(task_id);
    if (priority_index >= NUM_PRIORITIES ||
        NULL == tasks_in_priority[priority_index].first)
    {
        panic_diatribe(PANIC_OXYGOS_INVALID_TASK_ID, task_id);
    }
    taskq = &tasks_in_priority[priority_index];
    /* Find task within the list */

    lock_task_list(taskq); /* Prevent physical deletions as we traverse*/
    pTask = taskq->first;
    while ((pTask) && (task_id != pTask->id))
    {
        pTask = pTask->next;
    }

    if (NULL == pTask)
    {
        panic_diatribe(PANIC_OXYGOS_INVALID_TASK_ID, task_id);
    }

    /* if the message pointer is null something has gone wrong so panic */
    if (pMessage == (MSG *) NULL)
    {
        panic(PANIC_OXYGOS_SCHED_MSG_IS_NULL);
    }

    pQueue = &pTask->mqueue;

    pMessage->id = GET_MSGID();
    /* We have to store the message ID locally because if the message is going
     * to a higher-priority task it will have been freed by the time we return*/
    id = pMessage->id;
    pMessage->next = (MSG *) NULL;

    /*
     * Lock IRQs whilst we fiddle with message Qs.
     * TODO - SHOULD WE STORE LENGTH OF Q TO AVOID WHILE WITH IRQs LOCKED
     */
    interrupt_block();

    if (pTask->prunable)
    {
        /* This task is marked for deletion. Just flush the message */
        if (NULL!=pMessage->mv)
        {
            pfree(pMessage->mv);
        }
        /* The list has been updated now so the list can be unlocked */
        unlock_task_list_ints_blocked(taskq);

        if (pCachedMessage == (MSG *) NULL)
        {
            /* put the message in the cache */
            pCachedMessage = pMessage;
            interrupt_unblock();
        }
        else
        {
            interrupt_unblock();
            pfree((void *) pMessage);
        }
        /* There is no such thing as cancel message yet. so return MAX_SCHED_ID */
        return MAX_SCHED_ID;
    }
    else
    {
        /* Store the message on the end of the task's message chain.  */
        MSG **mq = &pQueue->first;
        while (*mq != (MSG *) NULL)
        {
            mq = &(*mq)->next;
        }
        *mq = pMessage;

        /* Increment message counts */
        TotalNumMessages++;
        sched_wake_up_background();
        taskq->num_msgs++;

        /* Set the bit representing that a task of this priority is ready to run */
        CurrentPriorityMask |= (unsigned) (1UL << priority_index);
    }
    unlock_task_list_ints_blocked(taskq);
    interrupt_unblock();

    /*
     * If the put_message was not called from an ISR, check for context switch.
     * For messages sent from an IRQ, context switch is checked on exit from IRQ
     */
    if (!SchedInterruptActive)
    {
            /* The scheduler may be idle at the moment, in which case the
             * context switch should NOT happen now and the message should be
             * handled from scheduler context when the main scheduler loop gets
             * to run.
             * NOTE: This will only happen in the non interrupt context if an
             * init message is sent from main() to get the scheduler rolling,
             * for example in host based tests, before calling the plsched();
             */
        if (NULL != current_id)
        {
            /* It's only worth doing a context switch if this action is sent
             * to a task higher than the current one. */
            uint16f cur_priority = GET_TASK_PRIORITY(*current_id);
            if (cur_priority < priority_index)
            {
                /*
                 * Set mask so all equal or lower priority bits to current task
                 * priority are set, so a HIGHER priority task causes a context
                 * switch to happen
                 */
                uint16f priorityMask = GET_PRIORITY_MASK(cur_priority);
                interrupt_block();
                check_context_switch(priorityMask);
                interrupt_unblock();
            }
        }
    }

    return id;
}

#ifndef DISABLE_SHALLOW_SLEEP
/*
 * Turn down the clock for "shallow sleep" power saving whilst in
 * idle loop.
 *
 * With COAL-based desktop builds for the Crescendo Apps subsystem,
 * sit in a loop waiting for the next itimed event to expire and then service
 * events.  This enables desktop tests to use the itime module with sched_oxygen.
 */
static void sched_sleep(void)
{
#if defined (DORM_MODULE_PRESENT) && !defined (UNIT_TEST_BUILD)
    {
        TIME latest;
        /* OXYGOS support for wakeup time ranges doesn't exist */
        if (timers_get_next_event_time(&latest))
        {
            dorm_sleep(latest, latest);
        }
        else
        {
            TIME throttle_timestamp = time_add(get_time(), SCHED_MAX_DELAY);
            dorm_sleep(throttle_timestamp, throttle_timestamp);
        }
    }
#else /* DORM_MODULE_PRESENT && !UNIT_TEST_BUILD */
    /* Just do shallow sleep if dorm not present */
    safe_enable_shallow_sleep();
#endif /* DORM_MODULE_PRESENT && !UNIT_TEST_BUILD */
}

#endif /* DISABLE_SHALLOW_SLEEP */

/*
 * Search the given list for a given index/task ID
 */
static bool search_list(uint8 task_priority, unsigned index, TASK **temp_pp_task[], BGINT **temp_pp_bgint[])
{
    TASK **pp_task = NULL;
    BGINT **pp_bgint = NULL;
    bool found = FALSE;

    //L2_DBG_MSG2("search_list(): searching task list with priority = %d for index = %d", task_priority, index);

    /* Load the current task list position pointer */
    pp_task = temp_pp_task[task_priority];

    while ((*pp_task) && ((TASKID_TO_TSKID((*pp_task)->id) < index)))
    {
        //L2_DBG_MSG2("search_list(): task list entry = %d, index = %d", TASKID_TO_TSKID((*pp_task)->id), index);

        pp_task = &(*pp_task)->next;
    }

    if ((*pp_task) && (TASKID_TO_TSKID((*pp_task)->id) == index))
    {
        //L2_DBG_MSG1("search_list(): found id = %d in task list", index);

        found = TRUE;
    }

    /* Store the current task list position pointer */
    temp_pp_task[task_priority] = pp_task;

    if (!found)
    {
        /* Load the current bgint list position pointer */
        pp_bgint = temp_pp_bgint[task_priority];

        while ((*pp_bgint) && ((TASKID_TO_TSKID((*pp_bgint)->id) < index)))
        {
            //L2_DBG_MSG2("search_list(): bgint list entry = %d, index = %d", TASKID_TO_TSKID((*pp_bgint)->id), index);

            pp_bgint = &(*pp_bgint)->next;
        }

        if ((*pp_bgint) && (TASKID_TO_TSKID((*pp_bgint)->id) == index))
        {
            //L2_DBG_MSG1("search_list(): found id = %d in bgint list", index);

            found = TRUE;
        }

        /* Store the current bgint list position pointer */
        temp_pp_bgint[task_priority] = pp_bgint;
    }

    return found;
}

/****************************************************************************
Public Function Definitions
*/

/**
 * Exits the scheduler.
 */
void sched_stop(void)
{
    /* On host, signal to main scheduler thread */
    interrupt_block();
    run = FALSE;
    interrupt_unblock();
}

/**
 * initialise the scheduler.
 * Note that task initialisation is not done in this call. It is
 * done at the start of \c sched() instead.
 */
void init_sched(void)
{
    PROC_BIT_FIELD proc_mask;
    TASK *tasks;
    BGINT *bg_ints;
    int i, n;
    unsigned tasks_count = 0, bgints_count = 0;

    struct
    {
        BGINT **ppb;
        TASK **ppt;
    } pcurrent_tail[NUM_PRIORITIES];

    /* Check that the scheduler is set up correctly */
    /* There must be at least NUM_PRIORITIES+1 bits in an unsigned int for the
     * code that uses PL_SIGNDET to determine the highest priority level to work.
     * Check this is true */
    COMPILE_TIME_ASSERT(UINT_BIT > NUM_PRIORITIES,
                        SCHEDULER_HAS_TOO_MANY_PRIORITIES);

    interrupt_block();

    proc_mask = (PROC_BIT_FIELD) (1 << proc_get_processor_id());

    for (n = 0; n < NUM_PRIORITIES; ++n)
    {
        pcurrent_tail[n].ppb = &bg_ints_in_priority[n].first;
        pcurrent_tail[n].ppt = &tasks_in_priority[n].first;
    }

    /* Count all the static bgints running on this core. */
    for (n = 0; n < N_BG_INTS; n++)
    {
        if ((static_bgints[n].proc_mask & proc_mask) != 0)
        {
            bgints_count += 1;
        }
    }

    /* Create an array of static bgints running on this core. */
    bg_ints = NULL;
    if (bgints_count != 0)
    {
        bg_ints = zpnewn(bgints_count, BGINT);
    }
    i = 0;
    for (n = 0; n < N_BG_INTS; n++)
    {
        const STATIC_BGINT *static_bgint;
        BGINT *bg_int;

        static_bgint = &static_bgints[n];
        bg_int = &bg_ints[i];
        if ((static_bgint->proc_mask & proc_mask) != 0)
        {
            PRIORITY pri;

            pri = static_bgint->priority;
            bg_int->id = SET_PRIORITY_TASK_ID(pri,
                                              static_bgint->tskid);
            MARK_AS_BG_INT(bg_int->id);
            bg_int->handler = static_bgint->handler;

            *pcurrent_tail[pri].ppb = bg_int;
            pcurrent_tail[pri].ppb = &bg_int->next;

            i += 1;
        }
    }

    /* Count all the static tasks running on this core. */
    for (n = 0; n < N_TASKS; n++)
    {
        if ((static_tasks[n].proc_mask & proc_mask) != 0)
        {
            tasks_count += 1;
        }
    }

    /* Create an array of static tasks running on this core. */
    tasks = NULL;
    if (tasks_count != 0)
    {
        tasks = zpnewn(tasks_count, TASK);
    }
    i = 0;
    for (n = 0; n < N_TASKS; n++)
    {
        const STATIC_TASK *static_task;
        TASK *task;

        static_task = &static_tasks[n];
        task = &tasks[i];
        if ((static_task->proc_mask & proc_mask) != 0)
        {
            PRIORITY pri;

            pri = static_task->priority;
            task->id = SET_PRIORITY_TASK_ID(pri,
                                               static_task->tskid);
            task->init = static_task->init;
            task->handler = static_task->handler;

            *pcurrent_tail[pri].ppt = task;
            pcurrent_tail[pri].ppt = &task->next;

            i += 1;
        }
    }

    interrupt_unblock();
}

/**
 * The main function of the background task scheduler. This invokes
 * bg ints as they are raised and tasks as messages become available for them.
 */
void sched(void)
{
    /* If this is the first time that the function is called then
       call each task's initialisation function. */
    if (!run)
    {
        init_tasks();
    }

    /* Main scheduling loop - run until the runlevel terminates */
    run = TRUE;
    while (run)
    {
        /* Do we have any messages pending on any tasks ? */
        if (TotalNumMessages != 0)
        {
            /*
             * Check all priority levels, and run any tasks that are ready
             * starting with highest priority task. InputMask is set so tasks
             * of all priorities are run
             */
            interrupt_block();
            check_context_switch(0);

            /* If the wakeup timer fired, check for any casual timed events
             * if there's nothing better to do
             */
            if ((casual_wakeup_pending) && (TotalNumMessages == 1))
            {
                casual_wakeup_pending = FALSE;
                TotalNumMessages--;
                /* There is probably a casual event to handle
                 * Note that the priority isn't actually used here,
                 * but the function / macro needs a parameter
                 */
                timers_service_expired_casual_events();
            }
            interrupt_unblock();
        }
        else
        {
#ifndef DISABLE_SHALLOW_SLEEP
            PROFILER_STOP(&sched_loop);
            PROFILER_START(&sleep_time);
            sched_sleep();
            PROFILER_STOP(&sleep_time);
            PROFILER_START(&sched_loop);
#else

#ifdef DESKTOP_TEST_BUILD
            /* Make sure timers get handled in test builds without shallow sleep */
            test_run_timers();
#endif

#endif /* DISABLE_SHALLOW_SLEEP */

            interrupt_block();
            if (casual_wakeup_pending)
            {
                casual_wakeup_pending = FALSE;
                TotalNumMessages--;
            }
            /* Service expired casual events. */
            timers_service_expired_casual_events();
            interrupt_unblock();

#ifdef DESKTOP_TEST_BUILD
            {
                TIME next_time;
                /* For these builds exit when we have nothing to do.
                 * Before bailing out, check one last time that we really are idle.
                 * In safe_enable_shallow_sleep we could have handled a timer expiry
                 * which queued up a message or bg_int, or perhaps scheduled
                 * another timer. */
                if (TotalNumMessages == 0 &&
                                        !timers_get_next_event_time(&next_time))
                {
                    run = FALSE;
                }
            }
#endif
        }
    } /* end of forever loop */

#ifdef DESKTOP_TEST_BUILD
    unsigned i;

    interrupt_block();
    for (i = 0; i < NUM_PRIORITIES; i++)
    {
        prune_tasks(&tasks_in_priority[i]);
        prune_bg_ints(&bg_ints_in_priority[i]);
    }
    sched_clear_message_cache();
    interrupt_unblock();
#endif
}

/**
 * Helper function that creates and populates a \c TASK object, for dynamic
 * task support.  \c TASKs inherit the current runlevel (if runlevels exist).
 *
 * @param task_data Pointer to private data for the task
 * @param handler The task's message handler.  Must not be NULL!
 * @return Pointer to the allocated task, or NULL if
 * insufficient pool memory is available for the structures to be allocated.
 */
static TASK *new_task(void *task_data,
                      TASK_FN_PTR handler)
{
    TASK *pTask;
    /* Create some memory for the task. */
    pTask = xzpnew(TASK);
    if (pTask != NULL)
    {
        /* Initialise elements of the new task as per input params */
        pTask->init = NULL;
        pTask->handler = handler;
        pTask->priv = task_data;
    }
    return pTask;
}

/**
 * Helper function that creates and populates a \c BGINT, for dynamic task
 * support.
 *
 * \note There is some complexity involved in the handling of the private
 * data area as a result of the shared ownership of the area when a bg int is
 * coupled with a task. If the \c BGINT is coupled with a \c TASK, \c ptask_data
 * must be non-NULL - it is a pointer to the \c TASK's private data pointer,
 * which is stored in the \c BGINT structure in the \c ppriv field. On the other
 * hand, if the BGINT is not associated with a TASK its \c ppriv member will be
 * set to point to a private data pointer, which itself is set equal to
 * \c plocal_data (which may be NULL).
 *
 * \note: In the latter case, the hidden private data pointer is supplied by
 * allocating an \c UNCOUPLED_BGINT instead of a \c BGINT.  However, only the
 * \c BGINT part is returned; we rely upon the fact that the \c BGINT is the
 * first element in the \c UNCOUPLED_BGINT struct to ensure that the memory is
 * subsequently correctly freed.
 *
 * \param ptask_data Pointer to the associated \c TASK's private data pointer.
 * \c NULL if there is no associated \c TASK.
 * \param handler The bg int handler.  Must be supplied.
 * \param plocal_data Direct pointer to the \c BGINT's private memory.  This
 * must be \c NULL if \c ptask_data is not \c NULL.
 * \return Pointer to the newly-allocated \c BGINT structure, or NULL
 * if insufficient pool memory is available for the structures to be
 * allocated.
 */
static BGINT *new_bg_int(void **ptask_data,  void (*handler)(void **),
                         void *plocal_data)
{
    BGINT *p_bgint;

    if (ptask_data == NULL)
    {
        UNCOUPLED_BGINT *p_uncpldbgint = xzpnew(UNCOUPLED_BGINT);
        if (p_uncpldbgint != NULL)
        {
            PL_ASSERT((BGINT *)p_uncpldbgint == &p_uncpldbgint->base);
            p_bgint = (BGINT *)p_uncpldbgint;
            p_uncpldbgint->priv = plocal_data;
            p_bgint->ppriv = &p_uncpldbgint->priv;
        }
        else
        {
            p_bgint = NULL;
        }
    }
    else
    {
        PL_ASSERT(plocal_data == NULL);
        p_bgint = xzpnew(BGINT);
        if (p_bgint != NULL)
        {
            p_bgint->ppriv = ptask_data;
        }
    }

    if (p_bgint != NULL)
    {
        p_bgint->handler = handler;
        p_bgint->raised = FALSE;
    }
    return p_bgint;
}

/**
 * Creates a new task with the given parameters, initialises the new task and
 * returns the task id.
 */
taskid create_task(PRIORITY task_priority,
                  void *task_data,
                  TASK_FN_PTR msg_handler,
                  bg_int_fn bg_int_handler)
{
    PRIORITY pri;
    TASK **pp_task, *p_task = NULL;
    BGINT **pp_bgint, *p_bgint = NULL;
    uint16f index;
    taskid id = NO_TASK;
    TASK **temp_pp_task[NUM_PRIORITIES];
    BGINT **temp_pp_bgint[NUM_PRIORITIES];

    //L2_DBG_MSG3("create_task(): p_task = 0x%08x, p_bgint = 0x%08x, task_priority = %d", p_task, p_bgint, task_priority);

    if (NUM_PRIORITIES<=task_priority             ||
        /* msg handler has to be present if task has any message queues
         * Also a task has to have either background interrupt handler or msg
         * handler */
        (NULL==msg_handler && NULL==bg_int_handler))
    {
        /* Basic checks to make sure that caller is asking for valid priority,
         * container to return task id, atleast 1 queue if msg handler is
         * specified and a valid task, which has atleast either of bg_int
         * handler or a msg_handler */
        panic(PANIC_OXYGOS_DYNAMIC_TASK_INVALID_PARAMS);
    }

    if (msg_handler)
    {
        p_task = new_task(task_data, msg_handler);

        if (p_task == NULL)
        {
            /* Task struct creation failed, so get out */
            return NO_TASK;
        }
    }

    if (bg_int_handler)
    {
        /* If the bg int is coupled to a task, it must store a pointer to the
         * task's priv pointer, so that they are always looking at the same
         * memory space */
        if (p_task)
        {
            p_bgint = new_bg_int(&p_task->priv, bg_int_handler, NULL);
        }
        else
        {
            p_bgint = new_bg_int(NULL, bg_int_handler, task_data);
        }

        if (p_bgint== NULL)
        {
            /* BG struct creation failed, so get out.
             * If we also created a task structure, tidy that up as well
             */
            if (p_task)
            {
                pfree(p_task);
            }
            return NO_TASK;
        }
    }

    PL_ASSERT(p_task || p_bgint);

    interrupt_block();

    /* Aquiring the task ID is a bit involved when there is both a
     * task and a bg int.  They need the same index. */
    /* Note: if, say, there's no task, we set pp_task to point to p_task (which
     * is NULL) because that makes it look like we've got to the end of the
     * task list immediately, and hence the loop just searches for a free bg
     * int index.  The same applies if there's no bgint. */

    pp_task = p_task ? &tasks_in_priority[task_priority].first : &p_task;
    pp_bgint = p_bgint? &bg_ints_in_priority[task_priority].first : &p_bgint;

    /* Reset the task/bgint temporary table pointers to the start of the lists */
    for (pri=LOWEST_PRIORITY; pri<NUM_PRIORITIES; pri++)
    {
        temp_pp_task[pri] = &tasks_in_priority[pri].first;
        temp_pp_bgint[pri] = &bg_ints_in_priority[pri].first;
    }

    /* Loop over tasks in lists, stopping when there is a simultaneous gap in
     * both task and bgint lists (as in the original ROM code)
     */
    for (index = 1; index <= MAX_TASK_INDEX; index++)
    {
        bool task_index_available =
                (!*pp_task) || (index != TASKID_TO_TSKID((*pp_task)->id));
        bool bgint_index_available =
                (!*pp_bgint) || (index != TASKID_TO_TSKID((*pp_bgint)->id));

        /* If the same available index is found in both task and bgint lists, then this
         * is a candidate for the ID provided it is not already used in the other lists
         * Note that if we're only interested in one of the lists, the
         * "available" flag for the other will always be TRUE */
        if(bgint_index_available && task_index_available)
        {
            bool found = FALSE;

            //L2_DBG_MSG1("create_task(): Trying index = %d", index);

            /* Check through all other task priorities (i.e. excluding the desired one!)
             * to ensure that the ID is not already assigned
             */
            for (pri=LOWEST_PRIORITY; pri<NUM_PRIORITIES; pri++)
            {
                if (pri == task_priority)
                {
                    /* Skip checking for the desired task priority */
                    continue;
                }

                /* Look through the list checking if the index/ID has already been used */
                if (search_list(pri, index, temp_pp_task, temp_pp_bgint))
                {
                    found = TRUE;
                    break;
                }
            }

            /* Is the ID already assigned to another list? */
            if (found)
            {
                /* Check the next index/ID */
                continue;
            }
            else
            {
                /* index/ID is not in another list (i.e it's available) break and use it */
                break;
            }
        }

        /* Only advance along the list if the index variable has "caught up with"
         * the tskid of this element */
        if (!bgint_index_available)
        {
            pp_bgint = &(*pp_bgint)->next;
        }
        if (!task_index_available)
        {
            pp_task = &(*pp_task)->next;
        }
    }

    if (index > MAX_TASK_INDEX)
    {
        /* Too many tasks. Caller is unlikely to be able to recover, so panic
         * here */
        interrupt_unblock();
        panic(PANIC_OXYGOS_TOO_MANY_DYNAMIC_TASKS);
    }

    if (p_task)
    {
        /* Insert the task into the list and set its ID */
        p_task->next = *pp_task;
        *pp_task = p_task;
        p_task->id = SET_PRIORITY_TASK_ID(task_priority,index);
        p_task->prunable = FALSE;
        id = p_task->id;

        /* Set owner of operator allocated memory to its task id */
        sched_tag_dm_memory(p_task, p_task->id);
    }

    if (p_bgint)
    {
        /* Insert the bg int into the list and set its ID */
        p_bgint->next = *pp_bgint;
        *pp_bgint = p_bgint;
        p_bgint->id = SET_PRIORITY_TASK_ID(task_priority,index);
        MARK_AS_BG_INT(p_bgint->id);
        p_bgint->prunable = FALSE;
        id = p_bgint->id;

        /* Set owner of operator allocated memory to its task id */
        sched_tag_dm_memory(p_bgint, p_bgint->id);
    }

    interrupt_unblock();

    if (p_bgint && p_task)
    {
        /* If it's a coupled pair, mark as such for quicker deletion */

        MARK_AS_COUPLED(p_bgint->id);
        MARK_AS_COUPLED(p_task->id);
        /* In the coupled case, return the task ID (the bg int ID differs only
         * in one byte) */
        id = p_task->id;
    }

    //L2_DBG_MSG1("create_task(): id = %d", TASKID_TO_TSKID(id));

    return id;
}

/**
 *  Deletes a scheduler task that was previously created
 *
 */
void delete_task(taskid id)
{
    TASK **pp_task;
    BGINT **pp_bgint;
    taskid task_id, bgint_id;
    uint16f priority;
    BG_INTQ *bg_intq;
    TASKQ *taskq;

    patch_fn_shared(pl_dynamic_tasks);

    priority = GET_TASK_PRIORITY(id);
    if (priority >= NUM_PRIORITIES)
    {
        panic(PANIC_OXYGOS_DYNAMIC_TASK_INVALID_PARAMS);
    }
    bg_intq = &bg_ints_in_priority[priority];
    taskq = &tasks_in_priority[priority];

    /* Sort out whether we're dealing with a task, a bg int or a coupled pair */
    task_id = NO_TASK;
    bgint_id = NO_TASK;
    if (ID_IS_BG_INT_ID(id))
    {
        bgint_id = id;
        if (IS_COUPLED(bgint_id))
        {
            task_id = bgint_id;
            MARK_AS_TASK(task_id);
        }
    }
    else
    {
        task_id = id;
        if (IS_COUPLED(task_id))
        {
            bgint_id = task_id;
            MARK_AS_BG_INT(bgint_id);
        }
    }

    if (task_id != NO_TASK)
    {
        /* go through the task in priority list and find the task to be deleted */

        /* Remember if the queue was really running, then set the flag so
         * pre-empting calls don't restructure the list as we're walking it */
        lock_task_list(taskq);
        /* Note that this lock doesn't stop tasks being added.  However, the
         * way we traverse the list is safe under additions: at worst our
         * traversal will skip over a newly-added element if we are interrupted
         * in the process of retrieving the next pointer. But in this function
         * we're not interested in newly-added elements, so that's OK.  The only
         * point at which we have to worry is when we do a physical deletion and
         * are resetting the previous element's next pointer.
         */
        for (pp_task = &taskq->first;
             NULL != *pp_task; pp_task = &(*pp_task)->next)
        {
            if (task_id == (*pp_task)->id)
            {
                TASK *pTask = *pp_task;
                /* We can't safely delete a task if its queue is in the middle
                 * of being processed by the scheduler.  In this case, just mark
                 * it for deletion and the scheduler will delete it when it's
                 * safe to do so */
                if (TASK_LIST_WAS_ALREADY_LOCKED(taskq))
                {
                    if (!pTask->prunable)
                    {
                        interrupt_block();
                        pTask->prunable = TRUE;
                        taskq->prunable++;
                        interrupt_unblock();
                        PL_PRINT_P1(TR_PL_SCHED_TASK_DELETE,
                                "Task 0x%08x is queued for delete\n", pTask->id);
                    }
                    else
                    {
                        PL_PRINT_P1(TR_PL_SCHED_TASK_DELETE,
                                    "Task 0x%08x already queued for delete\n",
                                    pTask->id);
                    }
                }
                else
                {
                    /* We're free to mess with this task list, but mark the
                     * task prunable first so that it's skipped by the main
                     * scheduler loop so we don't end up deleting it under the
                     * scheduler's feet. */
                    pTask->prunable = TRUE;
                    interrupt_block();
                    flush_task_messages(pTask);
                    /* update the list */
                    *pp_task = pTask->next;
                    interrupt_unblock();
                    PL_PRINT_P1(TR_PL_SCHED_TASK_DELETE,
                            "Task 0x%08x deleted\n", pTask->id);
                    pfree(pTask);
                }
                break;
            } /* id matches */
        } /*loop through all tasks in priority */

        unlock_task_list(taskq);
    }

    if (bgint_id != NO_TASK)
    {
        /* go through the task in priority list and find the task to be deleted */
        lock_bgint_list(bg_intq);
        for (pp_bgint = &bg_intq->first;
             NULL != *pp_bgint; pp_bgint = &(*pp_bgint)->next)
        {
            if (bgint_id == (*pp_bgint)->id)
            {
                BGINT *p_bgint = *pp_bgint;
                if (BGINT_LIST_WAS_ALREADY_LOCKED(bg_intq))
                {
                    if (!p_bgint->prunable)
                    {
                        interrupt_block();
                        p_bgint->prunable = TRUE;
                        bg_intq->prunable++;
                        interrupt_unblock();
                        PL_PRINT_P1(TR_PL_SCHED_TASK_DELETE,
                                    "Bg int 0x%08x is queued for delete\n",
                                    p_bgint->id);
                    }
                    else
                    {
                        PL_PRINT_P1(TR_PL_SCHED_TASK_DELETE,
                                    "Bg int 0x%08x already queued for delete\n",
                                    p_bgint->id);
                    }
                }
                else
                {
                    /* We're free to mess with this bg int list, but mark the
                     * bg int prunable first so that it's skipped by the main
                     * scheduler loop so we don't end up deleting it under the
                     * scheduler's feet. */
                    p_bgint->prunable = TRUE;
                    interrupt_block();
                    /* flush_bg_ints must be called with interrupts blocked */
                    flush_bg_ints(p_bgint);
                    /* update the list  */
                    *pp_bgint = p_bgint->next;
                    interrupt_unblock();
                    PL_PRINT_P1(TR_PL_SCHED_TASK_DELETE,
                                "Bg int 0x%08x deleted\n", p_bgint->id);
                    pfree(p_bgint);
                }
                break;
            } /* id matches */
        } /*loop through all bg ints in priority */

        unlock_bgint_list(bg_intq);
    }
}

/**
 * Sends a message consisting of the integer mi and the void* pointer
 * mv to the message queue queueId.
 *
 * mi and mv are neither inspected nor changed by the
 * scheduler - the task that owns queueId is expected to make sense of the
 * values.  mv may be NULL
 *
 * If mv is not null then it will typically be a chunk of malloc()ed
 * memory, though there is no need for it to be so.   tasks should
 * normally obey the convention that when a message built with
 * malloc()ed memory is given to put_message() then ownership of the
 * memory is ceded to the scheduler - and eventually to the recipient
 * task.   I.e., the receiver of the message will be expected to free()
 * the message storage.
 *
 * Note also that this function must be re-entrant - a call to this function
 * may cause another task to run which itself may call this function.
 */

msgid put_message_with_routing(taskid queueId, uint16 mi, void *mv, tRoutingInfo *routing)
{
    MSG *pMessage;

    patch_fn_shared(pl_msgs);

    /* Check there aren't too many messages */
    if (TotalNumMessages >= MAX_NUM_MESSAGES)
    {
      panic_diatribe(PANIC_OXYGOS_TOO_MANY_MESSAGES, TotalNumMessages);
    }

    PL_PRINT_P3(
        TR_PL_PUT_MESSAGE,
        "PL PutMessage called for Queue ID 0x%08x, message int %i, message "
        "pointer is %s\n", queueId,  mi, (NULL==mv)?"NULL":"Not NULL");

    interrupt_block();

    /* use the cached message if available */
    if (pCachedMessage != (MSG *) NULL)
    {
        pMessage = pCachedMessage;
        pCachedMessage = (MSG *) NULL;
        interrupt_unblock();
    }
    else
    {
        interrupt_unblock();
        /* pnew will either succeed or panic */
        pMessage = pnew(MSG);
    }

    /* Set the message parameters */
    pMessage->mi = mi;
    pMessage->mv = mv;

    if (routing == NULL)
    {
        pMessage->routing.src_id = 0;
        pMessage->routing.dest_id = 0;
    }
    else
    {
        pMessage->routing = *routing;
    }

    return put_allocated_message(queueId, pMessage);
}

/**
 * Raises a background interrupt to required task if not already raised.
 *
 * This function is usually called within an interrupt handler to signal an
 * event to a background task.
 */
void raise_bg_int(taskid task_id)
{
    uint16f priority_index;
    BGINT *p_bg_int;
    BG_INTQ *bg_intq;

    patch_fn_shared(pl_bgint);

    /* It is legal to pass a task's taskid on the understanding that there is a
     * directly corresponding bg int. */
    MARK_AS_BG_INT(task_id);

    /* Sanity checks that we have a valid tskid */
    priority_index = GET_TASK_PRIORITY(task_id);
    if (priority_index >= NUM_PRIORITIES)
    {
        panic_diatribe(PANIC_OXYGOS_INVALID_TASK_ID, task_id);
    }
    bg_intq = &bg_ints_in_priority[priority_index];

    /* Check there arent too many messages */
    if (TotalNumMessages >= MAX_NUM_MESSAGES)
    {
      panic_diatribe(PANIC_OXYGOS_TOO_MANY_MESSAGES, TotalNumMessages);
    }

    /* Find task within the list */
    /*lint -e722 There isn't supposed to a body to this for loop. */
    lock_bgint_list(bg_intq);
    for (p_bg_int = bg_intq->first;
         (p_bg_int) && (task_id != p_bg_int->id);
         p_bg_int = p_bg_int->next);

    if (NULL == p_bg_int)
    {
        panic_diatribe(PANIC_OXYGOS_INVALID_TASK_ID, task_id);
    }

    interrupt_block();
    /* Only raise bg interrupt if task is not up for deletion. It's far more
     * likely that the interrupt is already raised than it is prunable so test
     * that first. If either is true there is nothing to do just unwind */
    if (p_bg_int->raised || p_bg_int->prunable)
    {
        unlock_bgint_list_ints_blocked(bg_intq);
        interrupt_unblock();
        return;
    }
    /* The required background interrupt is not set
     * Make sure background interrupt has a handler */
    if (NULL == p_bg_int->handler)
    {
        panic(PANIC_OXYGOS_NULL_HANDLER);
    }
    p_bg_int->raised = TRUE;

    /* Increment message counts */
    TotalNumMessages++;
    sched_wake_up_background();
    bg_intq->num_raised++;

    /* Set the bit representing that a task of this priority is ready to
     * run */
    CurrentPriorityMask |= (unsigned) (1UL << priority_index);

    unlock_bgint_list_ints_blocked(bg_intq);
    interrupt_unblock();
    /*
     * If the raise_bg_int was not called from an ISR, check for context switch.
     * For bg_ints raised from an IRQ, context switch is checked on exit from IRQ
     */
    if (!SchedInterruptActive)
    {
            /* The scheduler may be idle at the moment, in which case the
             * context switch should NOT happen now and the bg_int should be
             * handled from scheduler context when the main scheduler loop gets
             * to run.
             * NOTE: This will only happen in the non interrupt context if an
             * init bg_int is sent from main() to get the scheduler rolling,
             * for example in host based tests, before calling the plsched();
             * So this is unlikely to ever be exercised.
             */
        if (NULL != current_id)
        {
            /* It's only worth doing a context switch if this action is sent
             * to a task higher than the current one. */
            uint16f cur_priority = GET_TASK_PRIORITY(*current_id);
            if (cur_priority < priority_index)
            {
                /*
                 * Set mask so all equal or lower priority bits to current task
                 * priority are set, so a HIGHER priority task causes a context
                 * switch to happen
                 */
                uint16f priorityMask = GET_PRIORITY_MASK(cur_priority);
                interrupt_block();
                check_context_switch(priorityMask);
                interrupt_unblock();
            }
        }
    }
}

/**
 * Raises a background interrupt to required task if not already raised.
 *
 * This function is usually called to cause an operator to do some processing.
 */
RUN_FROM_PM_RAM
void raise_bg_int_with_bgint(BGINT_TASK bg_int)
{
    BGINT *p_bg_int = (BGINT *)bg_int;
    patch_fn_shared(pl_bgint);

    /* Check there arent too many messages */
    if (TotalNumMessages >= MAX_NUM_MESSAGES)
    {
      panic_diatribe(PANIC_OXYGOS_TOO_MANY_MESSAGES, TotalNumMessages);
    }

    /* Only raise bg interrupt if one isn't already raised. If something jumps
     * in and services the already raised one before we leave this call then
     * it's been serviced which was the point of calling this function so we
     * can exit safely. */
    if (!p_bg_int->raised)
    {
        unsigned priority_index;
        if (!is_current_context_interrupt())
        {
            interrupt_block();
        }
        /* Something could have gotten in and raised an bg interrupt so check
         * again before doing the deed. If it did then there is nothing to do
         * except unblock any interrupts that were blocked. */
        if (p_bg_int->raised)
        {
            if (!is_current_context_interrupt())
            {
                interrupt_unblock();
            }
            return;
        }
        priority_index = GET_TASK_PRIORITY(p_bg_int->id);
        /* The required background interrupt is not set. */

        p_bg_int->raised = TRUE;
        /* Increment message counts */
        TotalNumMessages++;
        sched_wake_up_background();
        bg_ints_in_priority[priority_index].num_raised++;

        /* Set the bit representing that a task of this priority is ready to
         * run */
        CurrentPriorityMask |= (unsigned) (1UL << priority_index);

        if (!is_current_context_interrupt())
        {
            /* The scheduler may be idle at the moment, in which case the
             * context switch should NOT happen now and the bg_int should be
             * handled from scheduler context when the main scheduler loop gets
             * to run.
             * NOTE: This will only happen in the non interrupt context if an
             * init bg_int is sent from main() to get the scheduler rolling,
             * for example in host based tests, before calling the plsched();
             * So this is unlikely to ever be exercised.
             */
            if (NULL != current_id)
            {
                /* It's only worth doing a context switch if this action is sent
                 * to a task higher than the current one. */
                uint16f cur_priority = GET_TASK_PRIORITY(*current_id);
                if (cur_priority < priority_index)
                {
                    /*
                     * Set mask so all equal or lower priority bits to current task
                     * priority are set, so a HIGHER priority task causes a context
                     * switch to happen
                     */
                    uint16f priorityMask = GET_PRIORITY_MASK(cur_priority);
                    check_context_switch(priorityMask);
                }
            }
            interrupt_unblock();
        }
    }
}


/** A timed (delayed) message */
typedef struct  timed_msg_t {
    uint16   mi;      /**< The message's uint16. */
    void    *mv;     /**< The message's void*. */
    taskid   queueId; /**< The message's delivery queue. */
} timed_msg;

static void deliver_timed_message(void *stuff)
{
    timed_msg *msg = (timed_msg *) stuff;
    taskid task_id;
    uint16 mi;
    void *mv;

    /*
     * By pulling the parameters out of the pmalloc space onto
     * the stack, we can free the pmalloc pool block earlier,
     * increasing the chance that one will be available for
     * put_message().
     */
    task_id = msg->queueId;
    mi = msg->mi;
    mv = msg->mv;

    /* Recycle thoughtfully. */
    pdelete(msg);

    PL_PRINT_P2(TR_PL_PUT_MESSAGE,
                "Deliver timed message for taskid %d at time %d \n", task_id,
                time_get_time());

    /* Ignore the msgid that gets returned here. We can't really use it
     * for anything anyway. */
    (void) put_message(task_id, mi, mv);
}

/**
 * eventually send a message to a task
 */
tTimerId put_message_at(TIME deadline, taskid task_id, uint16 mi, void *mv)
{
    timed_msg *tm = pnew(timed_msg);

    patch_fn_shared(pl_msgs);

    /* Package the timed message. */
    tm->queueId = task_id;
    tm->mi = mi;
    tm->mv = mv;

    /* Deliver message via a timed event. */
    return timer_schedule_event_at(deadline, deliver_timed_message,
                                   (void *)tm);
}

/**
 * eventually send a message to a task
 */
tTimerId put_message_in(INTERVAL delay, taskid task_id,
                        uint16 mi, void *mv)
{
    patch_fn_shared(pl_msgs);

    /* If the delay is too short, send the message straight away. */
    if (time_lt(delay, MIN_PUT_MESSAGE_IN_DELAY))
    {
        /* Ignore the msgid that gets returned here. We can't really use it
         * for anything anyway. */
        (void) put_message(task_id, mi, mv);
        return TIMER_ID_INVALID; /* No timer was set up */
    }
    else
    {
        return put_message_at(time_add(time_get_time(), delay), task_id,
                              mi, mv);
    }
}

/**
 * Obtains a message from the task's message queue.
 * The message consists of one or both of a int and a void *.
 *
 * If the function is called from a background interrupt then the
 * scheduler asserts. It should also never be called in interrupt
 * handling context.
 *
 * If a message is taken from the queue, then *pmi and *pmv
 * are set to the mi and mv passed to put_message().
 *
 * pMessageInt and/or pmv can be null, in which case the
 * corresponding value from the message is discarded.
 *
 * If the function returns TRUE, and if pmv is null, then there is a
 * danger of a memory leak.  The scheduler does not know what is stored
 * in the void*, so it cannot reclaim a chunk of malloc()ed memory
 * there.   In most circumstances pmv should not be null, however,
 * it may be that the application knows that all messages sent on "q" will use
 * the "MessageInt" only, so it may be acceptable for pmv to be null.
 */
bool get_message_with_routing(uint16 *pmi,
                              void **pmv,
                              tRoutingInfo *routing)
{
    MSG *pMessage;
    MSGQ *pQueue;
    uint16f priority_index;
    TASK *current_task;
    taskid task_id;

    patch_fn_shared(pl_msgs);

    PL_ASSERT(current_id != NULL);

    task_id = *current_id;
    priority_index = GET_TASK_PRIORITY(task_id);

    /* Check for valid taskid and make sure a task is only allowed to read from
     * its own queues. */
    if (priority_index >= NUM_PRIORITIES                     ||
        NULL == tasks_in_priority[priority_index].first      ||
        ID_IS_BG_INT_ID(task_id))
    {
        panic_diatribe(PANIC_OXYGOS_INVALID_TASK_ID, task_id);
    }

    /* Obtain the task structure that the current_id belongs to.*/
    current_task = STRUCT_FROM_MEMBER(TASK, id, current_id);

    pQueue = &current_task->mqueue;

    pMessage = pQueue->first;

    /* If this message queue pointer isnt null, there is a message. Get the
     * unit and void * pointer from the message if required, and remove the
     * message from the queue */
    if (pMessage != (MSG *)(NULL))
    {
        if (pmi != (uint16 *)(NULL))
        {
            *pmi = pMessage->mi;
        }

        if (pmv != (void **)(NULL))
        {
            *pmv = pMessage->mv;
        }

        if (routing != NULL)
        {
            *routing = pMessage->routing;
        }

        /* If we've found a message, sched_n_messages should be > 0 */
        if ((TotalNumMessages == 0) ||
            (tasks_in_priority[priority_index].num_msgs <= 0))
        {
            panic(PANIC_OXYGOS_INVALID_MESSAGE_COUNT);
        }

        /* Need to lock IRQs while we change message counts and fiddle with
         * message Q */
        interrupt_block();

        pQueue->first = pMessage->next;
        TotalNumMessages--;
        tasks_in_priority[priority_index].num_msgs--;

        PL_PRINT_P3(
            TR_PL_GET_MESSAGE,
            "PL Got a message for queue ID 0x%08x, Message ID %i, Message "
            "pointer is %s\n", task_id, pMessage->mi,
            (NULL==pMessage->mv)?"NULL":"Not NULL");

        if (pCachedMessage == (MSG *) NULL)
        {
            /* put the message in the cache */
            pCachedMessage = pMessage;
            interrupt_unblock();
        }
        else
        {
            interrupt_unblock();
            pfree((void *) pMessage);
        }

        return(TRUE);
    }

    /* If we get here there was no message */
    PL_PRINT_P1(
        TR_PL_GET_MESSAGE,
        "PL GetMessage called for queue ID 0x%08x, but no message to get\n",
        task_id);
    return(FALSE);
}

/**
 * \brief Called from interrupt handler when a message has been sent in an ISR
 * Checks if context switch is needed.
 *
 */
void exit_irq_check_context_switch(void)
{
    if (run)
    {
        uint16f priorityMask;
        /* Check if context switch is needed. Set mask so message for task with
         * HIGHER priority than current task will cause context switch */
        if (NULL != current_id)
        {
            /*
             * Set mask so all equal or lower priority bits to current task priority
             * are set, so a HIGHER priority task causes a context switch to happen
             */
            priorityMask = GET_PRIORITY_MASK(GET_TASK_PRIORITY(*current_id));
            /* Don't block / unblock interrupts here
             * It's done in the interrupt handler before calling this function.
             */
            check_context_switch(priorityMask);
        }

    }
}

uint16f current_task_priority(void)
{
    if (!CurrentPriorityMask)
    {
        return 0U;
    }
    return MAP_PRIORITY_MASK_TO_HIGHEST_LEVEL(CurrentPriorityMask);
}

taskid get_current_task(void)
{
    if (current_id != NULL)
    {
        return *current_id;
    }
    else
    {
        return NO_TASK;
    }
}

PRIORITY get_current_priority(void)
{
    if (current_id == NULL)
    {
        return INVALID_PRIORITY;
    }
    return (PRIORITY) GET_TASK_PRIORITY(*current_id);
}

bool is_current_context_interrupt(void)
{
    return SchedInterruptActive != 0;
}

#ifdef UNIT_TEST_BUILD
void pl_verify_queue(taskid task_id, bool is_empty)
{
    TASK *pTask;
    uint16f priority_index = GET_TASK_PRIORITY(task_id);
    TASKQ *taskq;

    /* Check for valid priority and there is a task in priority*/
    PL_ASSERT(priority_index < NUM_PRIORITIES &&
              NULL != tasks_in_priority[priority_index].first);
    taskq = &tasks_in_priority[priority_index];

    /* Find task within the list */
    lock_task_list(taskq);
    for (pTask = taskq->first;
         (pTask) && (task_id != pTask->id); pTask = pTask->next);
    PL_ASSERT(NULL != pTask);

    if (is_empty)
    {
        PL_ASSERT(NULL==pTask->mqueue.first);
    }
    else
    {
        PL_ASSERT(NULL!=pTask->mqueue.first);
    }
    unlock_task_list(taskq);
}
#endif /* UNIT_TEST_BUILD */

/*
 * sched_find_bg_int   - This function is usually used to get a BGINT structure
 * for passing to the streamlined raise_bg_int_with_bgint routine.s
 */
bool sched_find_bgint(taskid task_id, BGINT_TASK *bgint)
{
    BGINT **bposition, *b, **the_bgint = (BGINT **)bgint;
    uint16f priority;
    BG_INTQ *bg_intq;

    /* It is legal to pass a task's taskid on the understanding that there is a
     * directly corresponding bg int. */
    MARK_AS_BG_INT(task_id);
    priority = GET_TASK_PRIORITY(task_id);
    bg_intq = &bg_ints_in_priority[priority];

    lock_bgint_list(bg_intq);
    for (bposition = &bg_intq->first;
            (b = *bposition) != NULL; bposition = &(*bposition)->next)
    {
        if (b->id == task_id)
        {
            if (the_bgint)
            {
                *the_bgint = b;
            }
            unlock_bgint_list(bg_intq);
            return TRUE;
        }
    }
    unlock_bgint_list(bg_intq);
    return FALSE;
}

/*
 * sched_clear_message_cache
 * Can be called on any processor,
 * but useful when using leak finder on aux (secondary)
 * processorto not report these memories.
 */
void sched_clear_message_cache(void)
{
    interrupt_block();

    if (pCachedMessage != (MSG *) NULL)
    {
        pfree((void *) pCachedMessage);
        pCachedMessage = NULL;
    }

    interrupt_unblock();
}

bool sched_busy(void)
{
    return TotalNumMessages != 0;
}

#if defined(INSTALL_THREAD_OFFLOAD)
/* When using thread offload on a secondary processor,
   the processor is either idle or running a remote
   procedure call. The scheduler is made to believe
   that there is a message in its processing queue to
   prevent it to go to sleep. */
void sched_disallow_sleep(void)
{
    TotalNumMessages = 1;
}

void sched_allow_sleep(void)
{
    TotalNumMessages = 0;
}
#endif /* defined(INSTALL_THREAD_OFFLOAD) */

/**
 * NAME
 *   sched_pack_taskid
 *
 * \brief Return the tskid of input parameter 'id'.
 *
 * \return The tskid of of input parameter 'id'.
 */
uint8 sched_pack_taskid(taskid id)
{
    return (uint8) TASKID_TO_TSKID(id);
}

/**
 * NAME
 *   sched_get_packed_task
 *
 * \brief Return the tskid of the current task.
 *
 * \return The tskid of the current task.
 */
uint8 sched_get_packed_task(void)
{
    return (uint8) TASKID_TO_TSKID(get_current_task());
}

