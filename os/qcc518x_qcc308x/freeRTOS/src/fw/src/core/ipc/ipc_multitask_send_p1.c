/* Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

#include "ipc/ipc_private.h"
#include "ipc_task.h"

static void ipc_try_send(const ipc_task_t *ipc_task, IPC_SIGNAL_ID msg_id,
                         const void *msg, uint16 len_bytes)
{
    IPC_HEADER header;

    header.id = msg_id;
    ipc_header_timestamp_set(&header);
    header.length_bytes = len_bytes;
    header.client = ipc_task_tag(ipc_task);
    header.priority = ipc_task_priority(ipc_task);

    ipc_try_send_common(&header, msg, len_bytes);
}

void ipc_send(IPC_SIGNAL_ID msg_id, const void *msg, uint16 len_bytes)
{
    ipc_task_t *ipc_task = ipc_task_get_or_create();
    ipc_try_send(ipc_task, msg_id, msg, len_bytes);
}

void ipc_transaction(IPC_SIGNAL_ID msg_id, const void *msg, uint16 len_bytes,
                     IPC_SIGNAL_ID rsp_id, void *blocking_msg)
{
    ipc_task_t *ipc_task = ipc_task_get_or_create();
    ipc_task_response_set(ipc_task, rsp_id, blocking_msg);
    ipc_try_send(ipc_task, msg_id, msg, len_bytes);
    ipc_task_response_wait(ipc_task);
}
