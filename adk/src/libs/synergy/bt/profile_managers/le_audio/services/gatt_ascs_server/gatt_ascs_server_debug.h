/* Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

/*
FILE NAME
    gatt_ascs_server_debug.h

DESCRIPTION
    Header file for the GATT ASES library debug functionality.
*/
#ifndef GATT_ASCS_SERVER_DEBUG_H_
#define GATT_ASCS_SERVER_DEBUG_H_

#include "lea_logging.h"

#define LEA_ASCS_LOG_ENABLE 0x00000002


#if (defined(LEA_ASCS_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_ASCS_LOG_ENABLE & LEA_LOG_MASK))

#define GATT_ASCS_SERVER_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtAscsLto, 0, __VA_ARGS__))
#define GATT_ASCS_SERVER_INFO(...) CSR_LOG_TEXT_INFO((CsrBtAscsLto, 0, __VA_ARGS__))
#define GATT_ASCS_SERVER_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtAscsLto, 0, __VA_ARGS__))
#define GATT_ASCS_SERVER_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtAscsLto, 0, __VA_ARGS__))
#define GATT_ASCS_SERVER_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtAscsLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define GATT_ASCS_SERVER_DEBUG(...)
#define GATT_ASCS_SERVER_INFO(...)
#define GATT_ASCS_SERVER_WARNING(...)
#define GATT_ASCS_SERVER_ERROR(...)
#define GATT_ASCS_SERVER_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#endif

#endif /* GATT_ASCS_SERVER_DEBUG_H_ */


