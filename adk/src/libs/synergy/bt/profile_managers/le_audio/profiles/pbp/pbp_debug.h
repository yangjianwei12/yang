/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #2 $
******************************************************************************/

#ifndef PBP_DEBUG_H_
#define PBP_DEBUG_H_

#include "lea_logging.h"

#define LEA_PBP_LOG_ENABLE 0x00010000

#if (defined(LEA_PBP_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_PBP_LOG_ENABLE & LEA_LOG_MASK))

#define PBP_INFO(...) CSR_LOG_TEXT_INFO((CsrBtPbpLto, 0, __VA_ARGS__))
#define PBP_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtPbpLto, 0, __VA_ARGS__))
#define PBP_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtPbpLto, 0, __VA_ARGS__))
#define PBP_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtPbpLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);} 

#else

#define PBP_INFO(...)
#define PBP_WARNING(...)
#define PBP_ERROR(...)
#define PBP_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);} 
#endif

#endif /* PBP_DEBUG_H_ */
