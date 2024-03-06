#ifndef CSR_HYDRA_SCHED_H__
#define CSR_HYDRA_SCHED_H__

/*****************************************************************************
Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #56 $
*****************************************************************************/

#include "csr_hydra_rtime.h"
#include "csr_hydra_types.h"
#include "csr_synergy.h"


#ifdef __cplusplus
extern "C" {
#endif

/** External task mask */
#define EXT_TASK_ID_MASK                    0xFF00

/** Converts a queue ID into external Task ID */
#define GET_EXT_TASK_ID(_queueId)           ((_queueId) | EXT_TASK_ID_MASK)

/** Checks if a Task ID belongs to an external task */
#define IS_EXT_TASK(_taskId)                (((_taskId) & EXT_TASK_ID_MASK) == EXT_TASK_ID_MASK)

/* Maximum number of Trap tasks which may communicate with Synergy tasks */
#ifndef APP_TRAP_TASKS_MAX
#define APP_TRAP_TASKS_MAX                  32
#endif
/* NULL trap task ID */
#define APP_TRAP_TASK_NULL                  (APP_TRAP_TASKS_MAX + 1)

/* ipc_send_bluestack was declared here. Replaced by VMTrap calls locally in qbl_adapter_scheduler.h */

#ifdef __cplusplus
}
#endif

#endif /* CSR_HYDRA_SCHED_H__ */
