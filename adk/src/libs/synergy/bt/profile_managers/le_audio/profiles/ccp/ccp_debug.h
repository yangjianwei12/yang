/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CCP_DEBUG_H_
#define CCP_DEBUG_H_

#include "lea_logging.h"

#define LEA_CCP_LOG_ENABLE 0x00000010

#if (defined(LEA_CCP_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_CCP_LOG_ENABLE & LEA_LOG_MASK))

#define CCP_INFO(...) CSR_LOG_TEXT_INFO((CsrBtCcpLto, 0, __VA_ARGS__))
#define CCP_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtCcpLto, 0, __VA_ARGS__))
#define CCP_DEBUG(...)  CSR_LOG_TEXT_DEBUG((CsrBtCcpLto, 0, __VA_ARGS__))
#define CCP_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtCcpLto, 0, __VA_ARGS__))
#define CCP_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtCcpLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define CCP_INFO(...)
#define CCP_WARNING(...)
#define CCP_DEBUG(...)
#define CCP_ERROR(...)
#define CCP_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#endif

#endif /* CCP_DEBUG_H_ */
