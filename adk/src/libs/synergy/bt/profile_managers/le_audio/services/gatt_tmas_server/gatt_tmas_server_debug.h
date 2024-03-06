/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#ifndef _GATT_TMAS_SERVER_DEBUG_H_
#define _GATT_TMAS_SERVER_DEBUG_H_

#include "lea_logging.h"

#define LEA_TMAS_LOG_ENABLE 0x00020000

#if (defined(LEA_TMAS_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_TMAS_LOG_ENABLE & LEA_LOG_MASK))

#define GATT_TMAS_SERVER_INFO(...) CSR_LOG_TEXT_INFO((CsrBtTmasLto, 0, __VA_ARGS__))
#define GATT_TMAS_SERVER_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtTmasLto, 0, __VA_ARGS__))
#define GATT_TMAS_SERVER_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtTmasLto, 0, __VA_ARGS__))
#define GATT_TMAS_SERVER_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtTmasLto, 0, __VA_ARGS__))
#define GATT_TMAS_SERVER_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtTmasLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define GATT_TMAS_SERVER_INFO(...)
#define GATT_TMAS_SERVER_DEBUG(...)
#define GATT_TMAS_SERVER_WARNING(...)
#define GATT_TMAS_SERVER_ERROR(...)
#define GATT_TMAS_SERVER_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#endif

#endif /* _GATT_TMAS_SERVER_DEBUG_H_ */
