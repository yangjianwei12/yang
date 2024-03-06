/****************************************************************************
 * Copyright (c) 2023 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file sched_static.c
 * \ingroup sched_oxygen
 *
 * Definition of the scheduler's tasks array and scheduler's background
 * interrupts array.
 */

#include "sched_oxygen/sched_oxygen_private.h"

/* Define a macro to declare the mapping between a task handler function
   and the type used by this function.
   The information is stored in the ELF and used by ACAT. */
#ifdef __KCC__
#define GLOBAL_DEBUG_STRING_ATTR _Pragma("datasection GLOBAL_DEBUG_STRINGS")
#define SCHED_MAP_HANDLER_DATA(HANDLER, HTYPE) GLOBAL_DEBUG_STRING_ATTR \
     const char ACAT_SCHED_TYPE_##HANDLER[] = "" #HTYPE "";
#else /* __KCC__ */
#define SCHED_MAP_HANDLER_DATA(HANDLER, HTYPE)
#endif /* __KCC__ */

#include "sched_oxygen/accmd_sched.h"
#include "sched_oxygen/obpm_sched.h"
#include "sched_oxygen/cap_download_mgr_sched.h"
#include "sched_oxygen/clk_mgr_task_sched.h"
#include "sched_oxygen/direct_access_sched.h"
#include "sched_oxygen/fault_sched.h"
#include "sched_oxygen/file_mgr_sched.h"
#include "sched_oxygen/audio_data_service_sched.h"
#include "sched_oxygen/hydra_sssm_sched.h"
#include "sched_oxygen/mibcmd_sched.h"
#include "sched_oxygen/sub_host_wake_sched.h"
#include "sched_oxygen/submsg_sched.h"
#include "sched_oxygen/subwd_sched.h"
#include "sched_oxygen/mem_utils_sched.h"
#include "sched_oxygen/opmgr_sched.h"
#include "sched_oxygen/ps_router_sched.h"
#include "sched_oxygen/ps_sar_sched.h"
#include "sched_oxygen/security_sched.h"

const STATIC_TASK static_tasks[] =
{
    /* accmd */
    {
        PROC_BIT_PROCESSOR_0,
        DEFAULT_PRIORITY,
        1,
        0,
        accmd_task_init,
        accmd_task,
    },
    /* obpm */
    {
        PROC_BIT_PROCESSOR_0,
        DEFAULT_PRIORITY,
        2,
        0,
        obpm_adaptor_init,
        obpm_adaptor_msg_handler,
    },
    /* CAP_DOWNLOAD_MGR */
    {
        PROC_BIT_PROCESSOR_ALL,
        DEFAULT_PRIORITY,
        3,
        0,
        cap_download_mgr_task_init,
        cap_download_mgr_task_handler,
    },
    /* clk_mgr_task */
    {
        PROC_BIT_PROCESSOR_0,
        DEFAULT_PRIORITY,
        4,
        0,
        clk_mgr_task_init,
        clk_mgr_task_msg_handler,
    },
    /* direct_access */
    {
        PROC_BIT_PROCESSOR_ALL,
        DEFAULT_PRIORITY,
        5,
        0,
        direct_access_task_init,
        direct_access_handler,
    },
    /* FILE_MGR */
    {
        PROC_BIT_PROCESSOR_0,
        DEFAULT_PRIORITY,
        6,
        0,
        file_mgr_task_init,
        file_mgr_task_handler,
    },
    /* audio_data_service */
    {
        PROC_BIT_PROCESSOR_0,
        DEFAULT_PRIORITY,
        7,
        0,
        audio_data_service_task_init,
        audio_data_service_task_handler,
    },
    /* hydra_sssm */
    {
        PROC_BIT_PROCESSOR_0,
        DEFAULT_PRIORITY,
        8,
        0,
        sssm_audio_task_init,
        sssm_audio_task_msg_handler,
    },
#if defined(MIBCMD_USES_SCHED_TASK)
    /* mibcmd */
    {
        PROC_BIT_PROCESSOR_0,
        DEFAULT_PRIORITY,
        9,
        0,
        init_mibcmd,
        mibcmd_task_handler,
    },
#endif
#if defined(INSTALL_CAPABILITY_CONSTANT_EXPORT)
    /* mem_utils */
    {
        PROC_BIT_PROCESSOR_0,
        DEFAULT_PRIORITY,
        10,
        0,
        mem_utils_task_init,
        mem_utils_task,
    },
#endif
    /* opmgr */
    {
        PROC_BIT_PROCESSOR_ALL,
        DEFAULT_PRIORITY,
        11,
        0,
        opmgr_task_init,
        opmgr_task_handler,
    },
    /* ps_router */
    {
        PROC_BIT_PROCESSOR_0,
        DEFAULT_PRIORITY,
        12,
        0,
        ps_router_task_init,
        ps_router_handler,
    },
    /* ps_sar */
    {
        PROC_BIT_PROCESSOR_ALL,
        DEFAULT_PRIORITY,
        13,
        0,
        ps_sar_task_init,
        ps_sar_handler,
    },
#if defined(SUPPORTS_LICENSING)
    /* security_task */
    {
        PROC_BIT_PROCESSOR_0,
        HIGHEST_PRIORITY,
        1,
        0,
        security_task_init,
        security_task_handler,
    },
#endif
};

SCHED_MAP_HANDLER_DATA(direct_access_handler, DIRECT_ACCESS_CTX)
SCHED_MAP_HANDLER_DATA(ps_router_handler, PS_ROUTER_CTX)
SCHED_MAP_HANDLER_DATA(ps_sar_handler, PS_SAR_CTX)
#if defined(SUPPORTS_LICENSING)
SCHED_MAP_HANDLER_DATA(security_task_handler, SECURITY_TASK_DATA)
#endif

const STATIC_BGINT static_bgints[] =
{
#if !defined(FAULT_LEAN_AND_MEAN)
    /* fault_publish */
    {
        PROC_BIT_PROCESSOR_ALL,
        DEFAULT_PRIORITY,
        1,
        0,
        publish_faults_bg,
    },
#endif
#if !defined(FAULT_LEAN_AND_MEAN)
    /* fault_publish_fresh */
    {
        PROC_BIT_PROCESSOR_ALL,
        DEFAULT_PRIORITY,
        2,
        0,
        publish_fresh_faults_bg,
    },
#endif
    /* sub_host_wake */
    {
        PROC_BIT_PROCESSOR_0,
        DEFAULT_PRIORITY,
        3,
        0,
        sub_host_wake_bg_int_handler,
    },
    /* submsg_tx */
    {
        PROC_BIT_PROCESSOR_0,
        DEFAULT_PRIORITY,
        4,
        0,
        submsg_tx_bg,
    },
    /* submsg_rx */
    {
        PROC_BIT_PROCESSOR_0,
        DEFAULT_PRIORITY,
        5,
        0,
        submsg_rx_bg,
    },
    /* subwd */
    {
        PROC_BIT_PROCESSOR_0,
        DEFAULT_PRIORITY,
        6,
        0,
        subwd_bg_int_handler,
    },
};
