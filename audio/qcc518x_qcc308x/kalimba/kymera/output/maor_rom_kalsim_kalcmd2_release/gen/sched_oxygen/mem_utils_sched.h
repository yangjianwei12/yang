/****************************************************************************
 * Copyright (c) 2023 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file mem_utils_sched.h
 * \ingroup sched_oxygen
 *
 * Declaration of a component's functions used by the scheduler.
 */

#ifndef MEM_UTILS_SCHED_H
#define MEM_UTILS_SCHED_H

#if defined(INSTALL_CAPABILITY_CONSTANT_EXPORT)
extern void mem_utils_task_init(void **);
extern void mem_utils_task(void **);
#endif
#endif
