/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*   %%version */
/* This is an X Macro file, it purposely does not have any include guards.
 * 
 * Bitserial blocking request signals.
 */
#if TRAPSET_BITSERIAL
X(IPC_SIGNAL_ID_BITSERIAL_OPEN)
X(IPC_SIGNAL_ID_BITSERIAL_CLOSE)
X(IPC_SIGNAL_ID_BITSERIAL_TRANSFER)
X(IPC_SIGNAL_ID_BITSERIAL_WRITE)
X(IPC_SIGNAL_ID_BITSERIAL_READ)
X(IPC_SIGNAL_ID_BITSERIAL_CHANGE_PARAM)
#endif /* TRAPSET_BITSERIAL */
