/****************************************************************************
 * Copyright (c) 2023 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file security_sched.h
 * \ingroup sched_oxygen
 *
 * Declaration of a component's functions used by the scheduler.
 */

#ifndef SECURITY_SCHED_H
#define SECURITY_SCHED_H

#if defined(SUPPORTS_LICENSING)
extern void security_task_init(void **);
extern void security_task_handler(void **);
#endif
#endif
