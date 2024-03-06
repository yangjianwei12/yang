/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/* Declares prototypes for functions exposed by freertos_tasks_c_additions.h.
   These functions extend the capabilities of FreeRTOS-Kernel/tasks.c without
   needing to modify the file directly. */

#ifndef FREERTOS_TASKS_H_ADDITIONS_H
#define FREERTOS_TASKS_H_ADDITIONS_H

#if configUSE_TICKLESS_IDLE

/**
 * Exposes the private prvGetExpectedIdleTime function to our FreeRTOS port.
 *
 * Returns the amount of time, in ticks, that will pass before the kernel will
 * next move a task from the Blocked state to the Running state.
 */
TickType_t xTaskGetExpectedSystemIdleTime(void);

#endif /* configUSE_TICKLESS_IDLE */

#endif /* FREERTOS_TASKS_H_ADDITIONS_H */
