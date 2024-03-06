/*
 * FreeRTOS Kernel V10.2.1
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "assert.h"

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

/**
 * Pre-emption is supported and the default for Qualcomm Hydra chips.
 */
#define configUSE_PREEMPTION (1)

/**
 * Time slicing is supported and the default for Qualcomm Hydra chips.
 */
#define configUSE_TIME_SLICING (1)

/**
 * The FreeRTOS tick is set to conserve power whilst still having the ability
 * to time slice and delay tasks.
 */
#define configTICK_RATE_HZ (10)

/**
 * Default to using 32-bit ticks.
 * 16-bit ticks haven't been tried yet and are likely of not much benefit.
 */
#define configUSE_16_BIT_TICKS (0)

/**
 * The maximum task name length in characters.
 * FreeRTOS will truncate the name if the provided one is longer than this.
 */
#define configMAX_TASK_NAME_LEN (8)

/**
 * The Idle hook is used for putting the CPU to sleep when FreeRTOS has nothing
 * to do.
 */
#define configUSE_IDLE_HOOK (1)

/**
 * Tickless idle is essential for saving power, it allows the tick to be
 * disabled for long periods of time if there's nothing to do. Without it
 * the CPU would wake for every tick even if there was nothing to do.
 */
#define configUSE_TICKLESS_IDLE (1)

/**
 * Some private functions in tasks.c are exposed to the Qualcomm Hydra library
 * to improve power saving in low power modes. If configUSE_TICKLESS_IDLE is
 * non-zero then this must be set to 1.
 */
#define configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H (1)

/**
 * Qualcomm Hydra chips require the pre-sleep processing hook for determining
 * the wake time.
 */
#define configPRE_SLEEP_PROCESSING(x) do { \
        x = vApplicationPreSleepProcessingHook(x); \
    } while(0)

/**
 * Qualcomm Hydra chips require the post-sleep processing hook for setting the
 * next hardware timer.
 */
#define configPOST_SLEEP_PROCESSING(x) do { \
        vApplicationPostSleepProcessingHook(x); \
    } while(0)

/**
 * Qualcomm Hydra chips do not require a tick hook.
 */
#define configUSE_TICK_HOOK (0)

/**
 * Increasing the number of priorities uses some extra RAM as a list of ready
 * tasks is required per priority level.
 *
 * Qualcomm Hydra chips need at least 2 levels of priority for the Idle Task and
 * the VM + IPC Receive task though this would not be ideal as applications
 * would not be able to define tasks that run at a higher or lower priority that
 * the VM task, so 5 seems a sensible lower limit.
 *
 * FreeRTOS will automatically cap priority values to this value minus 1.
 */
#define configMAX_PRIORITIES (8)

/**
 * Each task must be able to store at least the Kalimba context which is 184
 * bytes. Although with that value there is no room for any local variables.
 * The Idle task uses the minimal stack size so this must be enough to run the
 * Idle task.
 *
 * Running apps1.fw.env.var.idle.stack in pylib should give an idea of how much
 * stack the Idle task has used.
 */
#define configMINIMAL_STACK_SIZE ((unsigned short) 384/sizeof(StackType_t))

/**
 * By default the Idle task yields so that tasks at the idle priority could run.
 * Qualcomm Hydra chips do not define any tasks at the Idle priority other than
 * the Idle task.
 */
#define configIDLE_SHOULD_YIELD (1)

/**
 * This value must be set to 1. Tasks and queues are allocated statically by the
 * Qualcomm Hydra library.
 * Most of these objects live for the life of the program so there's no benefit
 * to dynamically allocating them, switching between dynamic and static
 * allocations would require the pmalloc pool configurations to be modified.
 */
#define configSUPPORT_STATIC_ALLOCATION (1)

/**
 * Enable dynamic allocation in FreeRTOS using pmalloc.
 * If this is enabled and used, the pmalloc pools must be adjusted to account
 * for new allocations performed by FreeRTOS. This may including dynamically
 * allocated task structures and their stacks, queues, semaphores, mutexes etc.
 */
#define configSUPPORT_DYNAMIC_ALLOCATION (1)

/**
 * Map the FreeRTOS assertion macro onto the Hydra assertion macro.
 */
#define configASSERT(x) assert(x)

/**
 * The current task handle is required by the IPC module for prioritising IPC
 * calls.
 */
#define INCLUDE_xTaskGetCurrentTaskHandle (1)

/**
 * The priority of a task is required by the IPC module for prioritising IPC
 * calls.
 */
#define INCLUDE_uxTaskPriorityGet (1)

/**
 * Qualcomm Hydra chips need to be able to determine whether the scheduler has
 * started or not for supporting IPC calls before and after MessageLoop().
 */
#define INCLUDE_xTaskGetSchedulerState (1)

/**
 * Used by Qualcomm Hydra test code only, not strictly required for the firmware
 * to operate.
 */
#define INCLUDE_vTaskDelay (1)

/**
 * Used by Qualcomm Hydra test code only, not strictly required for the firmware
 * to operate.
 */
#define INCLUDE_vTaskSuspend (1)

/**
 * Used by Qualcomm Hydra test code only, not strictly required for the firmware
 * to operate.
 */
#define INCLUDE_vTaskDelete (1)

#endif /* FREERTOS_CONFIG_H */
