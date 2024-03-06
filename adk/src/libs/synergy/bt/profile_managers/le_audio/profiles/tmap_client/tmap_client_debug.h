/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef TMAP_CLIENT_DEBUG_H_
#define TMAP_CLIENT_DEBUG_H_

#include "lea_logging.h"

#define LEA_TMAP_CLIENT_LOG_ENABLE 0x00010000

#if (defined(LEA_TMAP_CLIENT_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_TMAP_CLIENT_LOG_ENABLE & LEA_LOG_MASK))

#define TMAP_CLIENT_INFO(...) CSR_LOG_TEXT_INFO((CsrBtTmapLto, 0, __VA_ARGS__))
#define TMAP_CLIENT_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtTmapLto, 0, __VA_ARGS__))
#define TMAP_CLIENT_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtTmapLto, 0, __VA_ARGS__))
#define TMAP_CLIENT_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtTmapLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}
#define TMAP_CLIENT_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtTmapLto, 0, __VA_ARGS__))

#else

#define TMAP_CLIENT_INFO(...)
#define TMAP_CLIENT_WARNING(...)
#define TMAP_CLIENT_ERROR(...)
#define TMAP_CLIENT_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}
#define TMAP_CLIENT_DEBUG(...)
#endif

#endif /* TMAP_CLIENT_DEBUG_H_ */
