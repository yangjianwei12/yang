/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */
#include <assert.h>

#include "sched_freertos.h"
#include "sched_freertos_hooks.h"
#include "freertos_tasks_h_additions.h"
#include "hydra_log/hydra_log.h"
#include "pmalloc/pmalloc.h"
#include "dorm/dorm.h"
#include "pl_timers/pl_timers_private.h"
#include "trap_api/messaging.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define LOG_PREFIX "sched_freertos: "

/**
 * \brief The size of the VM task's stack in bytes.
 *
 * If the build system hasn't already defined this value it should default to
 * something sensible. Allowing the build system to define this value lets
 * application writers modify the size of the VM stack via MDE build options.
 */
#ifndef VM_TASK_STACK_BYTES
#define VM_TASK_STACK_BYTES (1024 + 512)
#endif

/**
 * \brief The size of the VM task's stack in 32-bit words.
 *
 * FreeRTOS stack sizes are specified as a number of StackType_t's, not bytes.
 */
#define VM_TASK_STACK_WORDS \
    ((VM_TASK_STACK_BYTES + sizeof(StackType_t) - 1) / sizeof(StackType_t))

/**
 * VM task data structure type.
 */
typedef struct vm_task_data_
{
    /**
     * Memory area for the VM task's stack.
     */
    StackType_t stack[VM_TASK_STACK_WORDS];

    /**
     * The FreeRTOS VM task data structure.
     */
    StaticTask_t task_structure;

    /**
     * A handle to the FreeRTOS VM Task.
     */
    TaskHandle_t task;

    /**
     * A message queue for posting messages and events to the VM task.
     */
    message_queue_t *queue;
} vm_task_data_t;

/**
 * The one VM task state.
 */
static vm_task_data_t vm;

/**
 * \brief Create the VM task.
 *
 * Doesn't start the task running, the call to sched() does that.
 */
static void vm_task_create(void);

/**
 * \brief Entrypoint for the FreeRTOS VM task.
 *
 * Does not return.
 *
 * \param [in] parameters Unused.
 */
static void vm_task_handler(void *parameters);

/**
 * Idle task data structure type.
 */
typedef struct idle_task_data_
{
    /**
     * Memory area for the Idle task's stack.
     */
    StackType_t stack[configMINIMAL_STACK_SIZE];

    /**
     * The FreeRTOS Idle task data structure.
     */
    StaticTask_t task_structure;
} idle_task_data_t;

/**
 * The one Idle task state.
 */
static idle_task_data_t idle;

/**
 * \brief Process any expired casual events.
 */
static void service_casual_timer(void);

/**
 * \brief Process any raised background interrupts.
 *
 * When new background interrupts are added this function needs extending to
 * handle them.
 *
 * \param [in] events  A set of events that need servicing. The index of each
 * set bit corresponds to the background interrupt to execute.
 */
static void service_queue_events(message_queue_events_t events);

void init_sched(void)
{
    /* FreeRTOS sleeps:
       1. In the Idle task.
       2. Waiting for an IPC response before the scheduler has started.
       3. Waiting for an IPC response for a SQIF write trap.

       In all cases interrupts are blocked before sleeping and unblocked after.
       After unblocking the scheduler will execute the interrupt that woke us
       from sleep and decide which task to execute.

       The allow goto shallow sleep register isn't required as interrupts are
       blocked whilst sleeping, if an interrupt line is raised after interrupts
       have been blocked but before sleeping the CPU will immediately return
       from the sleep instruction.
    */
    hal_set_reg_allow_goto_shallow_sleep(1);

    vm_task_create();
}

static void vm_task_create(void)
{
    vm.queue = messaging_queue_create_vm();
    vm.task = xTaskCreateStatic(/*pvTaskCode=*/vm_task_handler,
                                /*pcName=*/"VM",
                                /*ulStackDepth=*/VM_TASK_STACK_WORDS,
                                /*pvParameters=*/NULL,
                                /*uxPriority=*/SCHED_TASK_PRIORITY_VM,
                                /*puxStackBuffer=*/vm.stack,
                                /*pxTaskBuffer=*/&vm.task_structure);
    assert(vm.task);
}

void sched(void)
{
    vTaskStartScheduler();
}

static void vm_task_handler(void *parameters)
{
    UNUSED(parameters);

    for(;;)
    {
        message_queue_events_t events;

        /* Wait for a message and / or any background interrupts to process. */
        /* Messages are processed first within the wait function so they're
           delivered as close as possible to the expected time. */
        events = messaging_queue_wait_vm(vm.queue);
        /* Casual timers second, as they're casual they shouldn't need high
           accuracy. These are processed every time a message is delivered
           regardless of whether the service_casual_timers BG int has been set
           or not. */
        service_casual_timer();
        /* Queue events third, events have no time deadlines associated with
           them. */
        service_queue_events(events);
    }
}

static void service_casual_timer(void)
{
    block_interrupts();
    {
        timers_service_expired_casual_events();
    }
    unblock_interrupts();
}

static void service_queue_events(message_queue_events_t events)
{
    if(events & (1 << piodebounce_bg))
    {
        piodebounce_bg_int_handler();
    }

    if(events & (1 << appcmd_bg_handler))
    {
        appcmd_background_handler();
    }

    if(events & (1 << fault_publish))
    {
        publish_faults_bg();
    }
}

void raise_bg_int(uint32 bg_int)
{
    assert(bg_int <= MESSAGE_QUEUE_EVENTS_MAX);

    /* Background interrupts are implemented by setting a bit in the VM queue's
       set of events. */
    message_queue_event_raise(vm.queue, (message_queue_event_bit_t)bg_int);
}

#if configCHECK_FOR_STACK_OVERFLOW > 0
/**
 * @brief Called when a stack overflow has been detected by FreeRTOS.
 *
 * This is not the same as the Kalimba HW stack overflow detection, it is used
 * when configCHECK_FOR_STACK_OVERFLOW is enabled
 *
 * See FreeRTOS-Kernel/include/task.h for more documentation.
 */
void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    UNUSED(task);
    UNUSED(name);

    panic(PANIC_HYDRA_STACK_OVERFLOW);
}
#endif

/**
 * See sched_freertos_hooks.h for documentation
 */
void vApplicationIdleHook(void)
{
    /* This function is called in the Idle task if configUSE_IDLE_HOOK is
       enabled. If configUSE_TICKLESS_IDLE is enabled, the port implementation
       handles tickless idle sleeping in vApplicationPreSleepProcessingHook
       and vApplicationPostSleepProcessingHook. */

#if configUSE_TICKLESS_IDLE
    /* When tickless idle mode is enabled FreeRTOS won't disable the tick
       unless it is going to be idle for a configurable number of ticks,
       however this number must be at least two. Our system has a very long
       default tick duration to save power. Busy looping for two ticks wastes
       power and always sleeping here only to be woken by a tick before going
       into tickless idle also wastes power. This port therefore exposes the
       private prvGetExpectedIdleTime function in FreeRTOS-Kernel/tasks.c so
       it can check if it's going into tickless idle before deciding whether to
       sleep in the idle hook. */
    TickType_t expected_idle_ticks = xTaskGetExpectedSystemIdleTime();
    L4_DBG_MSG1(LOG_PREFIX
                "Expected idle ticks:%u threshold:" CSR_EXPAND_AND_STRINGIFY(
                    configEXPECTED_IDLE_TIME_BEFORE_SLEEP),
                expected_idle_ticks);
    if (expected_idle_ticks >= configEXPECTED_IDLE_TIME_BEFORE_SLEEP)
    {
        return;
    }
#endif /*configUSE_TICKLESS_IDLE */

    /* Block interrupts so that on wake from sleep we have a chance to update
       the next timer before another task is scheduled. */
    block_interrupts_before_sleep();
    {
        L4_DBG_MSG(LOG_PREFIX "Sleep tick:on");

        /* Sets the timer so that the scheduler wakes up before the next casual
           event expires. */
        timers_scheduler_needs_wakeup();

        /* Enter shallow or deep sleep. */
        dorm_sleep_sched();

        /* Updates the next hardware timer. */
        timers_scheduler_has_awoken();

        L4_DBG_MSG(LOG_PREFIX "Wake  tick:on");
    }
    unblock_interrupts_after_sleep();
}

/**
 * See sched_freertos_hooks.h for documentation
 */
TickType_t vApplicationPreSleepProcessingHook(TickType_t idle_ticks)
{
    UNUSED(idle_ticks);

    L4_DBG_MSG(LOG_PREFIX "Sleep tick:off");

    /* Sets the timer so that the scheduler wakes up before the next casual
       event expires. */
    timers_scheduler_needs_wakeup();

    /* Enter shallow or deep sleep. */
    dorm_sleep_sched();

    /* Return a new sleep duration of 0 since we've already performed the sleep
       in this function. */
    return 0;
}

/**
 * See sched_freertos_hooks.h for documentation
 */
void vApplicationPostSleepProcessingHook(TickType_t idle_ticks)
{
    UNUSED(idle_ticks);

    /* Updates the next hardware timer. */
    timers_scheduler_has_awoken();

    L4_DBG_MSG(LOG_PREFIX "Wake  tick:off");
}

/**
 * See FreeRTOS-Kernel/include/task.h for documentation
 */
void vApplicationGetIdleTaskMemory(StaticTask_t **tcb,
                                   StackType_t **stack,
                                   uint32_t *stack_size)
{
    *tcb = &idle.task_structure;
    *stack = idle.stack;
    *stack_size = configMINIMAL_STACK_SIZE;
}

bool sched_get_sleep_deadline(TIME *earliest, TIME *latest)
{
    TIME next_timer;
    TIME deadline;

    /* FreeRTOS next needs to do something either on the next OS tick interrupt,
       or when the next strict timer expires. Whichever is earlier.
       In the case of tickless idle the next tick will have already been set to
       the time we need to wake up rather than one tick duration in the future.
    */
    deadline = hal_get_reg_timer2_trigger();

    if (timers_get_next_event_time(&next_timer))
    {
        if(time_lt(next_timer, deadline))
        {
            deadline = next_timer;
        }
    }

    *latest = deadline;
    *earliest = *latest;
    return TRUE;
}

bool sched_is_running(void)
{
    return taskSCHEDULER_NOT_STARTED != xTaskGetSchedulerState();
}

bool sched_in_interrupt(void)
{
    return xPortInIsrContext();
}

bool sched_in_vm_task(void)
{
    return vm.task == xTaskGetCurrentTaskHandle();
}

uint16 sched_get_vm_stack_size(void)
{
    if (vm.task != NULL)
    {
        return VM_TASK_STACK_WORDS * sizeof(StackType_t);
    }
    /* Vm task stack is not yet created */
    return 0;
}

uint32 sched_get_vm_stack_start_address(void)
{
  return (uint32)vm.stack;
}
