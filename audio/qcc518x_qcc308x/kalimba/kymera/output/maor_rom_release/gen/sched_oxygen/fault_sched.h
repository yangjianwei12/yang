/****************************************************************************
 * Copyright (c) 2023 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file fault_sched.h
 * \ingroup sched_oxygen
 *
 * Declaration of a component's functions used by the scheduler.
 */

#ifndef FAULT_SCHED_H
#define FAULT_SCHED_H

#if !defined(FAULT_LEAN_AND_MEAN)
extern void publish_faults_bg(void **);
#endif
#if !defined(FAULT_LEAN_AND_MEAN)
extern void publish_fresh_faults_bg(void **);
#endif
#endif
