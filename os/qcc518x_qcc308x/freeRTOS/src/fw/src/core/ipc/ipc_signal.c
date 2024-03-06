/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */
#include "ipc/ipc_signal.h"

bool ipc_signal_is_blocking_request(IPC_SIGNAL_ID id)
{
    switch(id)
    {
/*lint -emacro(525,X) The indentation of the resulting code is not a problem. */
#define X(_signal) case _signal:
/*lint -e(961) Including after a declaration is the intention. */
#include "ipc/ipc_signal_blocking_requests.h"
#undef X
        return TRUE;
    default:
        return FALSE;
    }
}

bool ipc_signal_is_blocking_response(IPC_SIGNAL_ID id)
{
    if(ipc_signal_is_autogen_response(id))
    {
        return TRUE;
    }

    switch(id)
    {
    case IPC_SIGNAL_ID_MMU_HANDLE_ALLOC_RSP:
    case IPC_SIGNAL_ID_SMALLOC_RSP:
    case IPC_SIGNAL_ID_PIO_RSP:
    case IPC_SIGNAL_ID_STREAM_UART_SINK_RSP:
    case IPC_SIGNAL_ID_TESTTRAP_BT_RSP:
    case IPC_SIGNAL_ID_SD_MMC_SLOT_INIT_RSP:
    case IPC_SIGNAL_ID_SD_MMC_READ_DATA_RSP:
    case IPC_SIGNAL_ID_VM_GET_FW_VERSION_RSP:
    case IPC_SIGNAL_ID_VM_READ_SECURITY_RSP:
    case IPC_SIGNAL_ID_VM_ENABLE_SECURITY_RSP:
        return TRUE;
    default:
        return FALSE;
    }
}

bool ipc_signal_is_blocking(IPC_SIGNAL_ID id)
{
    return ipc_signal_is_blocking_request(id) ||
        ipc_signal_is_blocking_response(id);
}

bool ipc_signal_is_response_for_blocking_sqif_modify(IPC_SIGNAL_ID id)
{
    switch(id)
    {
/*lint -emacro(525,X) The indentation of the resulting code is not a problem. */
#define X(_signal) case _signal:
/*lint -e(961) Including after a declaration is the intention. */
#include "ipc/gen/ipc_trap_api_sqif_modify_blocking_responses.h"
#undef X
        return TRUE;
    default:
        return FALSE;
    }
}

bool ipc_signal_is_executed_on_audio(IPC_SIGNAL_ID id)
{
    switch(id)
    {
/*lint -emacro(525,X) The indentation of the resulting code is not a problem. */
#define X(_signal) case _signal:
/*lint -e(961) Including after a declaration is the intention. */
#include "ipc/gen/ipc_trap_api_executor_audio.h"
#undef X
        return TRUE;
    default:
        return FALSE;
    }
}
