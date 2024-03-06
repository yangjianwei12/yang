/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

#include "ipc/ipc_private.h"
#include "ipc/ipc_task.h"

static void ipc_recv_kick_process_queue(void);
static void ipc_recv_defer_queue_processing_to_task(void);
static void ipc_recv_process_queue(void);
static void ipc_recv_handle_msg(const IPC_HEADER *msg);
static void ipc_recv_task_handler(void *parameters);

void ipc_interrupt_handler(void)
{
    ipc_recv_kick_process_queue();
}

static void ipc_recv_kick_process_queue(void)
{
    if(sched_is_running())
    {
        ipc_recv_defer_queue_processing_to_task();
    }
    else
    {
        ipc_recv_process_queue();
    }
}

static void ipc_recv_defer_queue_processing_to_task(void)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    assert(ipc_data.recv_task);
    vTaskNotifyGiveFromISR(ipc_data.recv_task, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

static void ipc_recv_process_queue(void)
{
    /* There is no IPC_MAX_RECV_MSGS limit on FreeRTOS.

       If the scheduler is running then this function will be called from the
       IPC receive task. If the scheduler hasn't started then it will be called
       from the ISR. */
    while(ipc_buffer_any_messages(ipc_data.recv))
    {
        const IPC_HEADER *msg = ipc_buffer_map_read(ipc_data.recv);
        ipc_recv_handle_msg(msg);
        ipc_recv_message_free(msg->length_bytes);
    }
}

static void ipc_recv_handle_msg(const IPC_HEADER *msg)
{
    /* Only handles signals received by P1, FreeRTOS is only supported on P1.

       This function is called by ipc_recv_process_queue. If the scheduler hasn't
       started yet then this will be executed from the ISR, otherwise it will be
       from the IPC receive task.
     */
    IPC_SIGNAL_ID id = msg->id;
    switch(id)
    {
        case IPC_SIGNAL_ID_BLUESTACK_PRIM:
            ipc_bluestack_handler(id, msg);
            break;
        case IPC_SIGNAL_ID_TEST_TUNNEL_PRIM:
            ipc_test_tunnel_handler(id, msg, msg->length_bytes);
            break;
        case IPC_SIGNAL_ID_SCHED_MSG_PRIM:
            ipc_sched_handler(id, msg);
            break;
        case IPC_SIGNAL_ID_PFREE:
            ipc_malloc_msg_handler(id, msg);
            break;
        case IPC_SIGNAL_ID_APP_MSG:
        case IPC_SIGNAL_ID_APP_SINK_SOURCE_MSG:
        case IPC_SIGNAL_ID_APP_MSG_TO_HANDLER:
            ipc_trap_api_handler(id, msg, msg->length_bytes);
            break;
        case IPC_SIGNAL_ID_IPC_LEAVE_RECV_BUFFER_PAGES_MAPPED:
            ipc_data.leave_pages_mapped = TRUE;
            break;
        case IPC_SIGNAL_ID_STREAM_DESTROYED:
        case IPC_SIGNAL_ID_OPERATORS_DESTROYED:
            ipc_stream_handler(id, msg);
            break;
        case IPC_SIGNAL_ID_MEMORY_ACCESS_FAULT_INFO:
            ipc_memory_access_fault_handler(id, msg);
            break;
        case IPC_SIGNAL_ID_TRAP_API_VERSION:
            ipc_trap_api_version_prim_handler(id, msg);
            break;
        default:
        {
            /* The remaining signal IDs are trap responses. */
            ipc_task_t *ipc_task = ipc_task_get_from_tag(msg->client);

            /* Deliver the message if the task still exists.
               If the task has been deleted, or for some reason can't be found,
               fault and continue without delivering the message. This could
               result in a memory leak if the message expected the client to
               free any associated memory. */
            if(NULL != ipc_task)
            {
                if(!ipc_task_response_give(ipc_task, id, msg,
                                           msg->length_bytes))
                {
                    panic_diatribe(PANIC_IPC_UNHANDLED_MESSAGE_ID, id);
                }
            }
            else
            {
                fault_diatribe(FAULT_IPC_MESSAGE_FOR_UNKNOWN_TASK, msg->client);
            }
            break;
        }
    }
}

void ipc_recv_task_create(void)
{
    ipc_data.recv_task = xTaskCreateStatic(
        /*pvTaskCode=*/ipc_recv_task_handler,
        /*pcName=*/"IPCR",
        /*ulStackDepth=*/IPC_RECV_TASK_STACK_WORDS,
        /*pvParameters=*/NULL,
        /*uxPriority=*/SCHED_TASK_PRIORITY_IPC_RECV,
        /*puxStackBuffer=*/ipc_data.recv_task_stack,
        /*pxTaskBuffer=*/&ipc_data.recv_task_structure);
    assert(ipc_data.recv_task);
}

static void ipc_recv_task_handler(void *parameters)
{
    UNUSED(parameters);

    /* The IPC receive task handler is never expected to return. */
    for(;;)
    {
        if(ulTaskNotifyTake(pdTRUE, portMAX_DELAY))
        {
            ipc_recv_process_queue();
        }
    }
}

void ipc_recv_messages_sent_before_init(void)
{
    assert(ipc_data.recv_task);

    /* This is called before the FreeRTOS scheduler has started. The IPC receive
       task has been created but is not yet running, so it can't be waiting on a
       notification. If it was possible for it to be waiting on a notification
       this call could provoke a context switch, which would fail. */
    assert_fn_ret(xTaskNotifyGive(ipc_data.recv_task), BaseType_t, pdPASS);
}
