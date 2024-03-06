/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#ifndef GATT_MICS_CLIENT_DEBUG_H_
#define GATT_MICS_CLIENT_DEBUG_H_


#include "lea_logging.h"

#define LEA_MICS_LOG_ENABLE 0x00000100

#if (defined(LEA_MICS_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_MICS_LOG_ENABLE & LEA_LOG_MASK))

#define GATT_MICS_CLIENT_INFO(...) CSR_LOG_TEXT_INFO((CsrBtMicsLto, 0, __VA_ARGS__))
#define GATT_MICS_CLIENT_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtMicsLto, 0, __VA_ARGS__))
#define GATT_MICS_CLIENT_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtMicsLto, 0, __VA_ARGS__))
#define GATT_MICS_CLIENT_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtMicsLto, 0, __VA_ARGS__))
#define GATT_MICS_CLIENT_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtMicsLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define GATT_MICS_CLIENT_INFO(...)
#define GATT_MICS_CLIENT_DEBUG(...)
#define GATT_MICS_CLIENT_WARNING(...)
#define GATT_MICS_CLIENT_ERROR(...)
#define GATT_MICS_CLIENT_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#endif

#endif /* GATT_MICS_CLIENT_DEBUG_H_ */
