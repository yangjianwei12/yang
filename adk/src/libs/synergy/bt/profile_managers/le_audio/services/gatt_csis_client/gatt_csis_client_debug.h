/* Copyright (c) 2019 - 2022 Qualcomm Technologies International, Ltd. */
/*  */

/*
FILE NAME
    gatt_csis_client_debug.h

DESCRIPTION
    Header file for the GATT CSIS client library debug functionality.
*/
#ifndef GATT_CSIS_CLIENT_DEBUG_H
#define GATT_CSIS_CLIENT_DEBUG_H

#include "lea_logging.h"

#define LEA_CSIS_LOG_ENABLE 0x00002000


#if (defined(LEA_CSIS_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_CSIS_LOG_ENABLE & LEA_LOG_MASK))

#define GATT_CSIS_CLIENT_INFO(...) CSR_LOG_TEXT_INFO((CsrBtCsisLto, 0, __VA_ARGS__))
#define GATT_CSIS_CLIENT_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtCsisLto, 0, __VA_ARGS__))
#define GATT_CSIS_CLIENT_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtCsisLto, 0, __VA_ARGS__))
#define GATT_CSIS_CLIENT_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtCsisLto, 0, __VA_ARGS__))
#define GATT_CSIS_CLIENT_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtCsisLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define GATT_CSIS_CLIENT_INFO(...)
#define GATT_CSIS_CLIENT_DEBUG(...)
#define GATT_CSIS_CLIENT_WARNING(...)
#define GATT_CSIS_CLIENT_ERROR(...)
#define GATT_CSIS_CLIENT_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#endif
#endif /* GATT_CSIS_CLIENT_DEBUG_H */

