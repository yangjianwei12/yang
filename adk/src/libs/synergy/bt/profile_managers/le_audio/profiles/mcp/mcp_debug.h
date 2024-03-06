/* Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef MCP_DEBUG_H_
#define MCP_DEBUG_H_

#include "lea_logging.h"

#define LEA_MCP_LOG_ENABLE 0x00000020

#if (defined(LEA_MCP_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_MCP_LOG_ENABLE & LEA_LOG_MASK))

#define MCP_INFO(...) CSR_LOG_TEXT_INFO((CsrBtMcpLto, 0, __VA_ARGS__))
#define MCP_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtMcpLto, 0, __VA_ARGS__))
#define MCP_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtMcpLto, 0, __VA_ARGS__))
#define MCP_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtMcpLto, 0, __VA_ARGS__))
#define MCP_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtMcpLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}
#else
#define MCP_INFO(...)
#define MCP_DEBUG(...)
#define MCP_WARNING(...)
#define MCP_ERROR(...)
#define MCP_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}
#endif

#endif /* MCP_DEBUG_H_ */
