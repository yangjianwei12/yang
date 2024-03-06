/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#ifndef GATT_TELEPHONE_BEARER_SERVER_DEBUG_H_
#define GATT_TELEPHONE_BEARER_SERVER_DEBUG_H_


#include "lea_logging.h"

#define LEA_TBS_LOG_ENABLE 0x00000080

#if (defined(LEA_TBS_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_TBS_LOG_ENABLE & LEA_LOG_MASK))

#define GATT_TBS_SERVER_INFO(...) CSR_LOG_TEXT_INFO((CsrBtTbsLto, 0, __VA_ARGS__))
#define GATT_TBS_SERVER_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtTbsLto, 0, __VA_ARGS__))
#define GATT_TBS_SERVER_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtTbsLto, 0, __VA_ARGS__))
#define GATT_TBS_SERVER_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtTbsLto, 0, __VA_ARGS__))
#define GATT_TBS_SERVER_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtTbsLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define GATT_TBS_SERVER_INFO(...)
#define GATT_TBS_SERVER_DEBUG(...)
#define GATT_TBS_SERVER_WARNING(...)
#define GATT_TBS_SERVER_ERROR(...)
#define GATT_TBS_SERVER_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#endif
#endif /* GATT_TELEPHONE_BEARER_SERVER_DEBUG_H_ */
