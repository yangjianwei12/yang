/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef GATT_BASS_SERVER_DEBUG_H_
#define GATT_BASS_SERVER_DEBUG_H_

#include "lea_logging.h"

#define LEA_BASS_LOG_ENABLE 0x00000004

#if (defined(LEA_BASS_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_BASS_LOG_ENABLE & LEA_LOG_MASK))

#define GATT_BASS_SERVER_INFO(...) CSR_LOG_TEXT_INFO((CsrBtBassLto, 0, __VA_ARGS__))
#define GATT_BASS_SERVER_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtBassLto, 0, __VA_ARGS__))
#define GATT_BASS_SERVER_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtBassLto, 0, __VA_ARGS__))
#define GATT_BASS_SERVER_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtBassLto, 0, __VA_ARGS__))
#define GATT_BASS_SERVER_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtBassLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define GATT_BASS_SERVER_INFO(...)
#define GATT_BASS_SERVER_DEBUG(...)
#define GATT_BASS_SERVER_WARNING(...)
#define GATT_BASS_SERVER_ERROR(...)
#define GATT_BASS_SERVER_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#endif

#endif /* GATT_BASS_SERVER_DEBUG_H_ */
