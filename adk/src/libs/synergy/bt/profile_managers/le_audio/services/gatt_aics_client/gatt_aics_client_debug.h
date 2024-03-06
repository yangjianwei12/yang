/* Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd. */
/* %%version */


#ifndef GATT_AICS_CLIENT_DEBUG_H_
#define GATT_AICS_CLIENT_DEBUG_H_

#include "lea_logging.h"

#define LEA_AICS_LOG_ENABLE 0x00000800

#if (defined(LEA_AICS_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_AICS_LOG_ENABLE & LEA_LOG_MASK))

#define GATT_AICS_CLIENT_INFO(...) CSR_LOG_TEXT_INFO((CsrBtAicsLto, 0, __VA_ARGS__))
#define GATT_AICS_CLIENT_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtAicsLto, 0, __VA_ARGS__))
#define GATT_AICS_CLIENT_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtAicsLto, 0, __VA_ARGS__))
#define GATT_AICS_CLIENT_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtAicsLto, 0, __VA_ARGS__))
#define GATT_AICS_CLIENT_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtAicsLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define GATT_AICS_CLIENT_INFO(...)
#define GATT_AICS_CLIENT_DEBUG(...)
#define GATT_AICS_CLIENT_WARNING(...)
#define GATT_AICS_CLIENT_ERROR(...)
#define GATT_AICS_CLIENT_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#endif


#endif /* GATT_AICS_CLIENT_DEBUG_H_ */
