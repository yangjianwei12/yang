/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef _GATT_GMAS_SERVER_DEBUG_H_
#define _GATT_GMAS_SERVER_DEBUG_H_

#include "lea_logging.h"

#define LEA_GMAS_LOG_ENABLE 0x00200000

#if (defined(LEA_GMAS_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_GMAS_LOG_ENABLE & LEA_LOG_MASK))

#define GATT_GMAS_SERVER_INFO(...) CSR_LOG_TEXT_INFO((CsrBtGmasLto, 0, __VA_ARGS__))
#define GATT_GMAS_SERVER_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtGmasLto, 0, __VA_ARGS__))
#define GATT_GMAS_SERVER_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtGmasLto, 0, __VA_ARGS__))
#define GATT_GMAS_SERVER_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtGmasLto, 0, __VA_ARGS__))
#define GATT_GMAS_SERVER_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtGmasLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define GATT_GMAS_SERVER_INFO(...)
#define GATT_GMAS_SERVER_DEBUG(...)
#define GATT_GMAS_SERVER_WARNING(...)
#define GATT_GMAS_SERVER_ERROR(...)
#define GATT_GMAS_SERVER_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#endif

#endif /* _GATT_GMAS_SERVER_DEBUG_H_ */
