/****************************************************************************
 * Copyright (c) 2023 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file sched_ids.h
 * \ingroup sched_oxygen
 *
 * Definition of the scheduler's queue ids and background interrupts ids.
 */

#ifndef SCHED_IDS_H
#define SCHED_IDS_H

/* Use one of the symbols in this enumeration when calling functions
   "put_message_with_routing" or "put_message". */
typedef enum
{
    ACCMD_QUEUE = 0x1,
    OBPM_MSG_ADAPTOR_TASK_QUEUE_ID = 0x2,
    AOV_TASK_QUEUE_ID = 0x3,
    CAP_DOWNLOAD_MGR_TASK_QUEUE_ID = 0x4,
    CLK_MGR_TASK_QUEUE_ID = 0x5,
    DIRECT_ACCESS_TASK_QUEUE_ID = 0x6,
    FILE_MGR_TASK_QUEUE_ID = 0x7,
    AUDIO_DATA_SERVICE_TASK_QUEUE_ID = 0x8,
    sssm_audio_task_msg_queue_id = 0x9,
#if defined(MIBCMD_USES_SCHED_TASK)
    mibcmd_qid = 0xa,
#endif
    IPC_MSG_QUEUE_ID = 0xb,
#if defined(INSTALL_CAPABILITY_CONSTANT_EXPORT)
    MEM_UTILS_QUEUE = 0xc,
#endif
    OPMGR_TASK_QUEUE_ID = 0xd,
    PS_ROUTER_TASK_QUEUE_ID = 0xe,
    PS_SAR_TASK_QUEUE_ID = 0xf,
#if defined(SUPPORTS_LICENSING)
    SECURITY_TASK_QUEUE_ID = 0x401,
#endif
    QUEUE_INVALID = -1
} QUEUE_IDS;

/* Use one of the symbols in this enumeration when calling functions
   "put_message_with_routing" or "put_message". */
typedef enum
{
#if !defined(FAULT_LEAN_AND_MEAN)
    fault_publish_bg_int_id = 0x1000001,
#endif
#if !defined(FAULT_LEAN_AND_MEAN)
    fault_publish_fresh_bg_int_id = 0x1000002,
#endif
    sub_host_wake_bg_int_id = 0x1000003,
    submsg_tx_bg_int_id = 0x1000004,
    submsg_rx_bg_int_id = 0x1000005,
    subwd_bg_int_id = 0x1000006,
    BGINT_INVALID = -1
} BGINT_IDS;
#endif
