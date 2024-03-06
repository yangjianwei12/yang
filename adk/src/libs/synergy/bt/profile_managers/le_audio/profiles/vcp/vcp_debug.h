/* Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef VCP_DEBUG_H_
#define VCP_DEBUG_H_


#include "lea_logging.h"

#define LEA_VCP_LOG_ENABLE 0x00000400

#if (defined(LEA_VCP_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_VCP_LOG_ENABLE & LEA_LOG_MASK))

#define VCP_INFO(...) CSR_LOG_TEXT_INFO((CsrBtVcpLto, 0, __VA_ARGS__))
#define VCP_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtVcpLto, 0, __VA_ARGS__))
#define VCP_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtVcpLto, 0, __VA_ARGS__))
#define VCP_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtVcpLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}
#define VCP_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtVcpLto, 0, __VA_ARGS__))
#else

#define VCP_INFO(...)
#define VCP_WARNING(...)
#define VCP_ERROR(...)
#define VCP_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}
#define VCP_DEBUG(...)

#endif

#endif /* VCP_DEBUG_H_ */
