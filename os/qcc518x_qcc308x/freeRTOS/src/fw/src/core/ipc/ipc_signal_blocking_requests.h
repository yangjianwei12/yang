/* Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd. */
/*   %%version */
/* This is an X Macro file, it purposely does not have any include guards.

   It should be used by first definig the macro X and #including this file so
   that some code is generated for each signal defined in this file.

   e.g. The following will generate a list of cases for a switch statement:

   //lint -emacro(525,X) The indentation of the resulting code is not a problem
   #define X(_signal) case _signal:
   //lint -e(961) Including after a declaration is the intention
   #include "ipc/ipc_signal_blocking_requests.h"
   #undef X
*/
X(IPC_SIGNAL_ID_MMU_HANDLE_ALLOC_REQ)
X(IPC_SIGNAL_ID_SMALLOC_REQ)
X(IPC_SIGNAL_ID_PIO_SET_OWNER)
X(IPC_SIGNAL_ID_PIO_GET_OWNER)
X(IPC_SIGNAL_ID_PIO_SET_PULL_EN)
X(IPC_SIGNAL_ID_PIO_GET_PULL_EN)
X(IPC_SIGNAL_ID_PIO_SET_PULL_DIR)
X(IPC_SIGNAL_ID_PIO_GET_PULL_DIR)
X(IPC_SIGNAL_ID_PIO_SET_PULL_STR)
X(IPC_SIGNAL_ID_PIO_GET_PULL_STR)
X(IPC_SIGNAL_ID_PIO_GET_UNUSED)
X(IPC_SIGNAL_ID_PIO_SET_PIO_MUX)
X(IPC_SIGNAL_ID_STREAM_UART_SINK)
X(IPC_SIGNAL_ID_PIO_SET_PAD_MUX)
X(IPC_SIGNAL_ID_TESTTRAP_BT_REQ)
X(IPC_SIGNAL_ID_PIO_GET_PAD_MUX)
X(IPC_SIGNAL_ID_PIO_GET_PIO_MUX)
X(IPC_SIGNAL_ID_PIO_SET_DRIVE_STRENGTH)
X(IPC_SIGNAL_ID_PIO_GET_DRIVE_STRENGTH)
X(IPC_SIGNAL_ID_PIO_SET_STICKY)
X(IPC_SIGNAL_ID_PIO_GET_STICKY)
X(IPC_SIGNAL_ID_PIO_SET_SLEW)
X(IPC_SIGNAL_ID_PIO_GET_SLEW)
X(IPC_SIGNAL_ID_PIO_ACQUIRE)
X(IPC_SIGNAL_ID_PIO_RELEASE)
X(IPC_SIGNAL_ID_PIO_SET_XIO_MODE)
X(IPC_SIGNAL_ID_PIO_GET_XIO_MODE)
X(IPC_SIGNAL_ID_PIO_SET_DRIVE_ENABLE)
X(IPC_SIGNAL_ID_PIO_GET_DRIVE_ENABLE)
X(IPC_SIGNAL_ID_PIO_SET_DRIVE)
X(IPC_SIGNAL_ID_PIO_GET_DRIVE)
X(IPC_SIGNAL_ID_PIO_SET_FUNC_BITSERIAL)
X(IPC_SIGNAL_ID_PIO_SET_FUNC_UART)
X(IPC_SIGNAL_ID_SD_MMC_SLOT_INIT_REQ)
X(IPC_SIGNAL_ID_SD_MMC_READ_DATA_REQ)
X(IPC_SIGNAL_ID_VM_GET_FW_VERSION)
X(IPC_SIGNAL_ID_VM_READ_SECURITY)
X(IPC_SIGNAL_ID_VM_ENABLE_SECURITY)
X(IPC_SIGNAL_ID_PIO_SET_FUNC_CHARGER_COMMS)
/*lint -e(961) Including after a declaration is the intention. */
#include "ipc/ipc_bitserial_signal_blocking_requests.h"
/*lint -e(961) Including after a declaration is the intention. */
#include "ipc/gen/ipc_trap_api_blocking_requests.h"
