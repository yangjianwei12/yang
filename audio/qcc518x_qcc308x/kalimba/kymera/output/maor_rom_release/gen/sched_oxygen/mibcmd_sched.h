/****************************************************************************
 * Copyright (c) 2023 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file mibcmd_sched.h
 * \ingroup sched_oxygen
 *
 * Declaration of a component's functions used by the scheduler.
 */

#ifndef MIBCMD_SCHED_H
#define MIBCMD_SCHED_H

#if defined(MIBCMD_USES_SCHED_TASK)
extern void init_mibcmd(void **);
extern void mibcmd_task_handler(void **);
#endif
#endif
