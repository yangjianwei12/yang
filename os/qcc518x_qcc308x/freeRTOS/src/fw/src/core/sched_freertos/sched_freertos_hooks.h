/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * This file contains the prototypes for the FreeRTOS application level hooks
 * that Hydra OS uses.
 */

#ifndef SCHED_FREERTOS_HOOKS_H
#define SCHED_FREERTOS_HOOKS_H

#include "FreeRTOS.h"
#include "task.h"

/**
 * \brief Callback from FreeRTOS just before going in to tickless idle.
 *
 * \param [in] idle_ticks  The expected idle time in ticks. This is based on
 * what FreeRTOS knows about the system, this includes task delay times and
 * FreeRTOS tick times, but not message delivery times which are outside of
 * FreeRTOS. This value is unused as dorm_sleep_sched() can get this information
 * from sched_get_sleep_deadline().
 *
 * \return The amount of time to stay idle for in ticks, always returns 0 as
 * sleep is implemented within this function by calling dorm_sleep_sched().
 */
TickType_t vApplicationPreSleepProcessingHook(TickType_t idle_ticks);

/**
 * \brief Prototoype for the hook used by configPOST_SLEEP_PROCESSING.
 *
 * \param [in] idle_ticks The number of ticks expected to have been idle for.
 */
void vApplicationPostSleepProcessingHook(TickType_t idle_ticks);

/**
 * \brief Called by the FreeRTOS Idle task.
 *
 * Puts the CPU into shallow sleep if there's nothing else to do.
 *
 * NOTE: vApplicationIdleHook() MUST NOT, UNDER ANY CIRCUMSTANCES,
 * CALL A FUNCTION THAT MIGHT BLOCK.
 */
void vApplicationIdleHook(void);

#endif /* !SCHED_FREERTOS_HOOKS_H */
