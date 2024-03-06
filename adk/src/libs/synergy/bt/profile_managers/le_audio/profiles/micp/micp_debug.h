/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/

#ifndef MICP_DEBUG_H_
#define MICP_DEBUG_H_


#include "lea_logging.h"

#define LEA_MICP_LOG_ENABLE 0x00000400

#if (defined(LEA_MICP_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_MICP_LOG_ENABLE & LEA_LOG_MASK))

#define MICP_INFO(...) CSR_LOG_TEXT_INFO((CsrBtMicpLto, 0, __VA_ARGS__))
#define MICP_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtMicpLto, 0, __VA_ARGS__))
#define MICP_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtMicpLto, 0, __VA_ARGS__))
#define MICP_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtMicpLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}
#define MICP_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtMicpLto, 0, __VA_ARGS__))
#else

#define MICP_INFO(...)
#define MICP_WARNING(...)
#define MICP_ERROR(...)
#define MICP_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}
#define MICP_DEBUG(...)

#endif

#endif /* MICP_DEBUG_H_ */
