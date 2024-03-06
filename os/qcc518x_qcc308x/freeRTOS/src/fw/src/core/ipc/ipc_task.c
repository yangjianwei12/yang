/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

#include "ipc/ipc_private.h"
#include "ipc/ipc_task.h"
#include "ipc/ipc_signal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/**
 * The IPC task structure.
 *
 * One of these structures is allocated per execution context that sends an IPC
 * message. This may be the main function, an ISR or a task.
 * The structure holds task specific data required for a task to use the IPC.
 * Task structures are stored in a global list so that tasks can be looked up
 * from their IPC tag and vice versa.
 *
 * Note that there is no copy of the OS task's priority in this structure as it
 * is possible for the task priority to change dynamically.
 */
struct ipc_task_
{
    /** A pointer to the next IPC task in the global IPC task list. */
    struct ipc_task_ *next;
    /** The OS specific handle associated with this IPC task. */
    TaskHandle_t handle;
    /** The tag that has been assigned for use with this OS task.
        This value is sent along with the message in the IPC buffer, P0 returns
        the value along with the message's response so that the P1 IPC receive
        code can give the response to the correct task. */
    ipc_task_tag_t tag;
    /** The expected response ID. Should always be set to IPC_SIGNAL_ID_NULL when
        the task is not expecting a response. */
    IPC_SIGNAL_ID expected_response_id;
    /** A pointer to memory to store the response message. Should always be set
        to NULL when the task is not expecting a response. */
    void *response;
    /** The main task cannot use a real semaphore as it runs before the
        scheduler has started and the use of blocking IPC calls is allowed. In
        this case a simple spin lock is used. */
    volatile uint32 spin_lock;
    /** A handle for a semaphore to post when the response data is ready. */
    SemaphoreHandle_t sem;
    /** So that only one dynamic allocation is necessary for each IPC task the
        backing memory for a semaphore is stored within the ipc_task. */
    StaticSemaphore_t sem_buffer;
};

/**
 * A handle representing the main execution context, used before the scheduler
 * has started running.
 */
#define TASK_HANDLE_MAIN ((TaskHandle_t)0)
/**
 * A handle representing the ISR execution context. It is possible to send
 * non-blocking IPC messages from the ISR.
 */
#define TASK_HANDLE_ISR  ((TaskHandle_t)1)

/**
 * Global data structure for the IPC task module.
 */
typedef struct ipc_task_state_
{
    /**
     * A singly linked list of IPC tasks.
     */
    ipc_task_t *tasks;

    /**
     * A set of available task tags.
     *
     * Supports a maximum of 31 simultaneous task tags ranging from 1 to 32.
     * Tag 0 (Bit 31) is reserved to indicate an invalid or NULL tag.
     * Bit N is set if tag N+1 is available to be allocated.
     */
    uint32 available_tags;
} ipc_task_state_t;

/**
 * A global instance of the IPC task state.
 * Kalimba's signdet instruction requires bit 31 to be clear.
 */
static ipc_task_state_t ipc_task_state = {NULL, 0x7FFFFFFF};

/**
 * @brief Create an IPC task for the OS handle.
 * @param [in] handle  The OS handle to create an IPC task structure for.
 * Panics if out of memory.
 * @return A pointer to a new IPC task.
 */
static ipc_task_t *ipc_task_create(TaskHandle_t handle);

/**
 * @brief Look up the IPC task for the OS handle.
 * @warning Must be called with interrupts blocked.
 * @param [in] handle  The OS handle to find the IPC task for.
 * @return The IPC task for @p handle or NULL if one does not exist.
 */
static ipc_task_t *ipc_task_lookup(TaskHandle_t handle);

/**
 * @brief Return the task handle for the currently running task.
 * @return The task handle for the currently running task.
 */
static TaskHandle_t ipc_task_current_task_handle(void);

/**
 * @brief Allocate a tag.
 * @warning Must be called with interrupts blocked.
 */
static ipc_task_tag_t ipc_task_tag_allocate(void);

/**
 * @brief Deallocate a tag.
 * @warning Must be called with interrupts blocked.
 */
static void ipc_task_tag_deallocate(ipc_task_tag_t tag);

ipc_task_t *ipc_task_get_or_create(void)
{
    ipc_task_t *ipc_task;
    TaskHandle_t handle;

    handle = ipc_task_current_task_handle();

    block_interrupts();
    {
        ipc_task = ipc_task_lookup(handle);

        /* If handle doesn't exist create a new entry and return that. */
        if(NULL == ipc_task)
        {
            ipc_task = ipc_task_create(handle);
            ipc_task->next = ipc_task_state.tasks;
            ipc_task_state.tasks = ipc_task;
        }
    }
    unblock_interrupts();

    assert(ipc_task);
    return ipc_task;
}

static TaskHandle_t ipc_task_current_task_handle(void)
{
    TaskHandle_t handle;

    if(!sched_is_running())
    {
        /* Before the scheduler has started there's no current task. */
        handle = TASK_HANDLE_MAIN;
    }
    else if(sched_in_interrupt())
    {
        /* Non-blocking messages can be sent from the ISR. */
        handle = TASK_HANDLE_ISR;
    }
    else
    {
        handle = xTaskGetCurrentTaskHandle();
    }

    return handle;
}

void ipc_task_delete_from_handle(void *handle)
{
    block_interrupts();
    {
        ipc_task_t **piter;

        for(piter = &ipc_task_state.tasks; NULL != *piter;
            piter = &(*piter)->next)
        {
            if((*piter)->handle == handle)
            {
                ipc_task_t *ipc_task = *piter;
                *piter = ipc_task->next;
                ipc_task_tag_deallocate(ipc_task->tag);

                /* Unblock interrupts as early as possible to keep interrupt
                   latency low. */
                unblock_interrupts();

                vSemaphoreDelete(ipc_task->sem);
                pfree(ipc_task);
                return;
            }
        }
    }
    unblock_interrupts();
}

static ipc_task_t *ipc_task_lookup(TaskHandle_t handle)
{
    ipc_task_t *iter;

    for(iter = ipc_task_state.tasks; NULL != iter; iter = iter->next)
    {
        if(iter->handle == handle)
        {
            return iter;
        }
    }

    return NULL;
}

static ipc_task_t *ipc_task_create(TaskHandle_t handle)
{
    ipc_task_t *ipc_task;

    ipc_task = pnew(ipc_task_t);
    ipc_task->handle = handle;
    ipc_task->tag = ipc_task_tag_allocate();
    if(TASK_HANDLE_MAIN == handle || TASK_HANDLE_ISR == handle)
    {
        ipc_task->spin_lock = 0;
        ipc_task->sem = NULL;
    }
    else
    {
        ipc_task->spin_lock = 0;

        /* Binary semaphores created with xSemaphoreCreateBinaryStatic are intially
           in the empty state, so a "Give" must be called before a "Take" will be
           successful. */
        ipc_task->sem = xSemaphoreCreateBinaryStatic(&ipc_task->sem_buffer);
        assert(ipc_task->sem);
    }

    ipc_task->expected_response_id = IPC_SIGNAL_ID_NULL;
    ipc_task->response = NULL;
    ipc_task->next = NULL;

    return ipc_task;
}

ipc_task_t *ipc_task_get_from_tag(ipc_task_tag_t tag)
{
    ipc_task_t *iter;

    for(iter = ipc_task_state.tasks; NULL != iter; iter = iter->next)
    {
        if(iter->tag == tag)
        {
            break;
        }
    }

    /* It's possible iter is NULL if the task associated with 'tag' has been
       deleted. Currently that's the only known case where it's acceptable
       for the requested tag to not have a matching ipc_task. */
    return iter;
}

ipc_task_tag_t ipc_task_tag(const ipc_task_t *ipc_task)
{
    assert(ipc_task);
    return ipc_task->tag;
}

uint8 ipc_task_priority(const ipc_task_t *ipc_task)
{
    uint8 priority;

    assert(ipc_task);

    /* A task's priority is determined dyamically rather than when the IPC task
       is created as it's possible for the task priority to be changed. */

    if(TASK_HANDLE_MAIN == ipc_task->handle)
    {
        /* The 'main' task will use priority 0, although since it's the only 'task'
           running the priority value doesn't actually matter. */
        priority = 0;
    }
    else if(TASK_HANDLE_ISR == ipc_task->handle)
    {
        /* The ISR has higher priority than any task. */
        priority = configMAX_PRIORITIES;
    }
    else
    {
        priority = (uint8) uxTaskPriorityGet(ipc_task->handle);
    }

    return priority;
}

bool ipc_task_response_give(ipc_task_t *ipc_task, IPC_SIGNAL_ID id,
                            const void *response, uint16 length)
{
    assert(ipc_task);
    assert(response);
    assert(length);
    assert(ipc_task->response);

    if(id != ipc_task->expected_response_id)
    {
        return FALSE;
    }

    memcpy(ipc_task->response, response, length);

    if(TASK_HANDLE_MAIN == ipc_task->handle || TASK_HANDLE_ISR == ipc_task->handle)
    {
        ipc_task->spin_lock = 1;
    }
    else
    {
        /* Delivering a blocking response to a task can only happen once the
           scheduler is running, and once the scheduler is running responses
           should only be processed by the IPC receive task, not the ISR. */
        assert(!sched_in_interrupt());

        assert_fn_ret(xSemaphoreGive(ipc_task->sem), BaseType_t, pdPASS);
    }

    return TRUE;
}

void ipc_task_response_set(ipc_task_t *ipc_task, IPC_SIGNAL_ID id, void *response)
{
    assert(ipc_task);
    assert(IPC_SIGNAL_ID_NULL == ipc_task->expected_response_id);
    assert(NULL == ipc_task->response);

    ipc_task->expected_response_id = id;
    ipc_task->response = response;
}

static void ipc_task_response_wait_before_sched_start(ipc_task_t *ipc_task)
{
    /* The spin lock could be given inside an interrupt that happens
       after the check but before sleep, if that happens the CPU might
       never wake up so interrupts must be blocked between the check
       and sleeping, but unblocked so that the interrupt that gives the
       semaphore has a chance to run. */
    while(0 == ipc_task->spin_lock)
    {
        block_interrupts_before_sleep();
        {
            if(0 == ipc_task->spin_lock)
            {
                dorm_shallow_sleep(0);
            }
        }
        unblock_interrupts_after_sleep();
    }
    ipc_task->spin_lock = 0;
}

static void ipc_task_response_wait_allow_lower_priority(ipc_task_t *ipc_task)
{
    assert_fn(xSemaphoreTake(ipc_task->sem, portMAX_DELAY));
}

static void ipc_task_response_wait_disallow_lower_priority(ipc_task_t *ipc_task)
{
    BaseType_t taken;

    /* Waiting on a semaphore would allow lower priority tasks to run.

       This task must remain in the 'running' or 'ready' state so only
       tasks of equal or higher priority can be scheduled. Otherwise we
       risk a lower priority task using P1 resource and blocking P0 from
       executing a SQIF write or erase for a higher priority task.

       Poll the semaphore until it's taken and sleep in-between,
       ensuring the semaphore can't become available between the take
       and the sleep.
    */
    do
    {
        block_interrupts_before_sleep();
        {
            taken = xSemaphoreTake(ipc_task->sem, 0);
            if(!taken)
            {
                dorm_shallow_sleep(0);
            }
        }
        unblock_interrupts_after_sleep();
    }
    while(!taken);
}

void ipc_task_response_wait(ipc_task_t *ipc_task)
{
    assert(ipc_task);
    assert(ipc_task->response);

    /* We must not wait for responses in an ISR. */
    assert(!sched_in_interrupt());

    /* There's no sense deep sleeping if waiting for a response from P0,
       we know someone in the system will be busy executing our request. */
    dorm_disallow_deep_sleep(DORMID_IPC);

    if(TASK_HANDLE_MAIN == ipc_task->handle)
    {
        ipc_task_response_wait_before_sched_start(ipc_task);
    }
    else if(ipc_signal_is_response_for_blocking_sqif_modify(
                ipc_task->expected_response_id))
    {
        ipc_task_response_wait_disallow_lower_priority(ipc_task);
    }
    else
    {
        ipc_task_response_wait_allow_lower_priority(ipc_task);
    }

    dorm_allow_deep_sleep(DORMID_IPC);

    ipc_task->expected_response_id = IPC_SIGNAL_ID_NULL;
    ipc_task->response = NULL;
}

static ipc_task_tag_t ipc_task_tag_allocate(void)
{
    ipc_task_tag_t tag;

    /* It is a programming error to run out of tags for tasks. */
    assert(ipc_task_state.available_tags);

    tag = (ipc_task_tag_t)(MAX_BIT_POS_31(ipc_task_state.available_tags) + 1);
    ipc_task_state.available_tags &= ~(1 << (tag - 1));

    return tag;
}

static void ipc_task_tag_deallocate(ipc_task_tag_t tag)
{
    /* Validate the tag is allocated before freeing it. */
    assert(0 == ((1 << (tag - 1)) & ipc_task_state.available_tags));

    ipc_task_state.available_tags |= 1 << (tag - 1);
}
