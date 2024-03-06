/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GMAP_CLIENT_DEBUG_H_
#define GMAP_CLIENT_DEBUG_H_

#include "lea_logging.h"

#define LEA_GMAP_CLIENT_LOG_ENABLE 0x00800000

#if (defined(LEA_GMAP_CLIENT_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_GMAP_CLIENT_LOG_ENABLE & LEA_LOG_MASK))

#define GMAP_CLIENT_INFO(...) CSR_LOG_TEXT_INFO((CsrBtGmapLto, 0, __VA_ARGS__))
#define GMAP_CLIENT_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtGmapLto, 0, __VA_ARGS__))
#define GMAP_CLIENT_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtGmapLto, 0, __VA_ARGS__))
#define GMAP_CLIENT_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtGmapLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}
#define GMAP_CLIENT_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtGmapLto, 0, __VA_ARGS__))

#else

#define GMAP_CLIENT_INFO(...)
#define GMAP_CLIENT_WARNING(...)
#define GMAP_CLIENT_ERROR(...)
#define GMAP_CLIENT_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}
#define GMAP_CLIENT_DEBUG(...)
#endif

#endif /* GMAP_CLIENT_DEBUG_H_ */
