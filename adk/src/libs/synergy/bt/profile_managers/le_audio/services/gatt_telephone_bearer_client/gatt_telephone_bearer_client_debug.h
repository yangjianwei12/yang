/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef GATT_TBS_CLIENT_DEBUG_H_
#define GATT_TBS_CLIENT_DEBUG_H_

#include "lea_logging.h"

#define LEA_TBS_LOG_ENABLE 0x00000080

#if (defined(LEA_TBS_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_TBS_LOG_ENABLE & LEA_LOG_MASK))

#define GATT_TBS_CLIENT_INFO(...) CSR_LOG_TEXT_INFO((CsrBtTbsLto, 0, __VA_ARGS__))
#define GATT_TBS_CLIENT_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtTbsLto, 0, __VA_ARGS__))
#define GATT_TBS_CLIENT_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtTbsLto, 0, __VA_ARGS__))
#define GATT_TBS_CLIENT_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtTbsLto, 0, __VA_ARGS__))
#define GATT_TBS_CLIENT_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtTbsLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define GATT_TBS_CLIENT_INFO(...)
#define GATT_TBS_CLIENT_DEBUG(...)
#define GATT_TBS_CLIENT_WARNING(...)
#define GATT_TBS_CLIENT_ERROR(...)
#define GATT_TBS_CLIENT_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#endif

#endif /* GATT_TBS_CLIENT_DEBUG_H_ */
