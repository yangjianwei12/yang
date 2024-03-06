/* Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd. */
/* %%version */


#ifndef GATT_VOCS_CLIENT_DEBUG_H_
#define GATT_VOCS_CLIENT_DEBUG_H_


#include "lea_logging.h"

#define LEA_VOCS_LOG_ENABLE 0x00008000

#if (defined(LEA_VOCS_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_VOCS_LOG_ENABLE & LEA_LOG_MASK))

#define GATT_VOCS_CLIENT_INFO(...) CSR_LOG_TEXT_INFO((CsrBtVocsLto, 0, __VA_ARGS__))
#define GATT_VOCS_CLIENT_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtVocsLto, 0, __VA_ARGS__))
#define GATT_VOCS_CLIENT_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtVocsLto, 0, __VA_ARGS__))
#define GATT_VOCS_CLIENT_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtVocsLto, 0, __VA_ARGS__))
#define GATT_VOCS_CLIENT_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtVocsLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define GATT_VOCS_CLIENT_INFO(...)
#define GATT_VOCS_CLIENT_DEBUG(...)
#define GATT_VOCS_CLIENT_WARNING(...)
#define GATT_VOCS_CLIENT_ERROR(...)
#define GATT_VOCS_CLIENT_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#endif

#endif /* GATT_VCS_CLIENT_DEBUG_H_ */
