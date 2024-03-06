/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file sched_count.h
 * \ingroup sched_oxygen
 *
 * Declaration of the number of static tasks and static background interrupts
 * available in the firmware.
 */

#ifndef SCHED_COUNT_H
#define SCHED_COUNT_H

/* The only relevant symbol in this enumeration is N_TASKS. */
typedef enum
{
    sched_count_task_accmd,
    sched_count_task_obpm,
    sched_count_task_aov_task,
    sched_count_task_CAP_DOWNLOAD_MGR,
    sched_count_task_clk_mgr_task,
    sched_count_task_direct_access,
    sched_count_task_FILE_MGR,
    sched_count_task_audio_data_service,
    sched_count_task_hydra_sssm,
#if defined(MIBCMD_USES_SCHED_TASK)
    sched_count_task_mibcmd,
#endif
    sched_count_task_ipc_task,
#if defined(INSTALL_CAPABILITY_CONSTANT_EXPORT)
    sched_count_task_mem_utils,
#endif
    sched_count_task_opmgr,
    sched_count_task_ps_router,
    sched_count_task_ps_sar,
#if defined(SUPPORTS_LICENSING)
    sched_count_task_security_task,
#endif
    N_TASKS
} TASK_COUNT;

/* The only relevant symbol in this enumeration is N_BG_INTS. */
typedef enum
{
#if !defined(FAULT_LEAN_AND_MEAN)
    sched_count_bgint_fault_publish,
#endif
#if !defined(FAULT_LEAN_AND_MEAN)
    sched_count_bgint_fault_publish_fresh,
#endif
    sched_count_bgint_sub_host_wake,
    sched_count_bgint_submsg_tx,
    sched_count_bgint_submsg_rx,
    sched_count_bgint_subwd,
    N_BG_INTS
} BGINT_COUNT;
#endif
