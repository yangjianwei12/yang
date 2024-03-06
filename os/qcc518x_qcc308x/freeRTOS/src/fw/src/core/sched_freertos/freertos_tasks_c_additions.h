/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/* This file extends the functions defined by FreeRTOS-Kernel/tasks.c
   and should only be included by that file. */

#ifndef FREERTOS_TASKS_C_ADDITIONS_H
#define FREERTOS_TASKS_C_ADDITIONS_H

#include "freertos_tasks_h_additions.h"

#if configUSE_TICKLESS_IDLE

TickType_t xTaskGetExpectedSystemIdleTime(void)
{
    return prvGetExpectedIdleTime();
}

#endif /* configUSE_TICKLESS_IDLE */

#endif /* FREERTOS_TASKS_C_ADDITIONS_H */
