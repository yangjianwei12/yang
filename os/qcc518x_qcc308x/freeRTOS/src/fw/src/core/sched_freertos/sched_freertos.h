/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */
#ifndef SCHED_FREERTOS_H
#define SCHED_FREERTOS_H

#include "pl_timers/pl_timers.h"
#include "timed_event/timed_event.h"
#include "hydra/hydra_macros.h"

/**
 * Background Interrupt Interface for FreeRTOS
 */

/**
 * \brief A list of background interrupts supported by the FreeRTOS port.
 *
 * BG Ints on Oxygen were a way of signaling to the scheduler that some function
 * should be called in the future. On FreeRTOS there's no exact feature that
 * matches BG Ints so they're implemented by raising event flags on the VM
 * message queue. The VM task will then read these event flags and execute
 * the relevant functions.
 *
 * To add a BG Int to FreeRTOS:
 * 1. An entry should be added to this enum
 * 2. The function protoype for handling the BG Int should be declared below
 * 3. The service_queue_events function in sched_freertos.c should be extended
 *    to call the new handler if the new event bit is set.
 *
 * The BG Int can then be raised the same way it is on Oxygen using
 * GEN_BG_INT(bg_int_name).
 */
typedef enum bg_ints_ids_enum
{
    /**
     * Background interrupt for handling the result of a debounced PIO.
     * Handler: piodebounce_bg_int_handler
     */
    piodebounce_bg,

    /**
     * Background interrupt for handling appcmd messages.
     * Handler: appcmd_background_handler
     */
    appcmd_bg_handler,

    /**
     * Background interrupt for publishing faults over the host interface.
     * Handler: publish_faults_bg
     */
    fault_publish,

    /**
     * Casual timers are handled slightly differently to other BG Ints, they are
     * processed every time the VM task gets a message or event. This entry
     * allows an event to be raised to process casual timers when there may be
     * no other events or messages pending. There is no definition for the BG
     * int handler below as it is implemented entirely within sched_freertos.c.
     */
    service_casual_timers
} bg_int_ids;

/**
 * Background Interrupt Handlers
 */

/**
 * PIO debouce background interrupt handler.
 *
 * BG Int: piodebounce_bg
 */
void piodebounce_bg_int_handler(void);

/**
 * Appcmd background interrupt handler.
 *
 * BG Int: appcmd_bg_handler
 */
void appcmd_background_handler(void);

/**
 * Publish faults background interrupt handler.
 *
 * BG Int: fault_publish
 */
void publish_faults_bg(void);

/**
 * Functions for sending Background Interrupts
 */

/**
 * \brief Raise a background interrupt to required bg interrupt if not already
 * raised.
 *
 * \param[in] bg_int  ID of the bg_int to raise.
 *
 * \note
 * This function is usually called within an interrupt handler to signal an
 * event to a background task.
 */
void raise_bg_int(uint32 bg_int);

/**
 * Generate a background interrupt.
 *
 * \param[in] _bg_int  ID of the bg_int to raise.
 */
#define GEN_BG_INT(_bg_int) raise_bg_int(_bg_int)

/**
 * Generic Sched API Functions
 */

/**
 * Called from dorm on entry to sleep if dorm needs to know the time the
 * scheduler wants us to be awake.
 *
 * \param[out] earliest The earliest time on the microsecond
 * clock at which \c dorm_sleep should return, unless woken by the
 * foreground.  This corresponds to the earliest time of an
 * event scheduled by \c timed_event_at_between().
 *
 * \param[out] latest The last time on the microsecond clock at which
 * \c dorm_sleep_sched() should return.  As usual, this value is compared
 * with the current time in a signed fashion:  if the difference has
 * the top bit (bit 31) set, then \c latest is treated as being in
 * the past and the function returns immediately.  This means that minor
 * delays in the background scheduler are benign.
 *
 * \return TRUE if there is a wakeup deadline, in which case earliest and
 * latest will have been filled in, or FALSE if there is no deadline in
 * which case earliest and latest have undefined values.
 */
bool sched_get_sleep_deadline(TIME *earliest, TIME *latest);

/**
 * \brief Wakeup from timers.
 *
 * Called from the timers block when a background timer is due to expire while
 * the scheduler was asleep.
 *
 * On FreeRTOS a loop through the scheduler is executed each time a strict event
 * (interrupt level timer) occurs. Processing of the casual event queue is
 * prompted by sched_wake() which is called by dorm_wake() at the end of the
 * time_service_routine() strict timer interrupt handler.
 *
 * \note The scheduler armed this by calling timers_scheduler_needs_wakeup().
 */
#define sched_wakeup_from_timers() do { } while(0)

/**
 * \brief Wake up the scheduler to process any outstanding casual timers.
 *
 * Called by dorm_wake() at the end of strict timer interrupt processing.
 *
 * On FreeRTOS strict timer interrupts always raise the service_casual_timers
 * BG Int so that the casual timer queue is checked for anything that might
 * have expired before deciding whether to continue sleeping.
 */
#define sched_wake() GEN_BG_INT(service_casual_timers)

/**
 * \brief Initialise the scheduler.
 *
 * Creates the VM task.
 * The scheduler does not start running until sched() is called.
 */
void init_sched(void);

/**
 * \brief Start the scheduler.
 *
 * Starts the FreeRTOS scheduler running.
 *
 * This function does not return.
 */
void sched(void);

/**
 * \brief Determine whether the scheduler is running.
 *
 * \return TRUE if the scheduler has been started, i.e. sched() has been called.
 */
bool sched_is_running(void);

/**
 * \brief Returns whether running in the interrupt context.
 *
 * \return TRUE if currently executing an interrupt, FALSE otherwise.
 */
bool sched_in_interrupt(void);

/**
 * \brief Determine whether the VM task is currently running.
 *
 * \return TRUE if the VM task is currently running, FALSE otherwise.
 */
bool sched_in_vm_task(void);

/**
 * \brief Looking for the size of the Vm task stack
 *
 * \return Return the size in bytes or 0 if the vm task stack is not created
 */
uint16 sched_get_vm_stack_size(void);

/**
 * \brief Looking for the address of the Vm task stack
 *
 * \return Return The start address of the vm task stack
 */
uint32 sched_get_vm_stack_start_address(void);

/**
 * Priorities of each FreeRTOS task created by HydraOS.
 */
typedef enum sched_task_priority_
{
    /** Lowest possible priority. */
    SCHED_TASK_PRIORITY_IDLE       = 0,

    /** Priority for the VM task, should be at least 2 so applications could
        create low priority tasks that are lower than the VM priority but above
        idle priority. */
    SCHED_TASK_PRIORITY_VM         = 2,

    /** IPC receive task priority. Must be one of the highest priority tasks in
        the system, any task higher priority than this must not use the IPC as
        it's possible to deadlock waiting for a responses as traps that write to
        SQIF do not schedule in lower priority tasks whilst waiting for a
        response. */
    SCHED_TASK_PRIORITY_IPC_RECV   = 4
} sched_task_priority_t;

#endif
