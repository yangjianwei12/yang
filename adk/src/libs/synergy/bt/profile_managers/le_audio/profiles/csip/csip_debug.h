/* Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CSIP_DEBUG_H_
#define CSIP_DEBUG_H_


#include "lea_logging.h"

#define LEA_CSIP_LOG_ENABLE 0x00000200

#if (defined(LEA_CSIP_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_CSIP_LOG_ENABLE & LEA_LOG_MASK))

#define CSIP_INFO(...) CSR_LOG_TEXT_INFO((CsrBtCsipLto, 0, __VA_ARGS__))
#define CSIP_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtCsipLto, 0, __VA_ARGS__))
#define CSIP_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtCsipLto, 0, __VA_ARGS__))
#define CSIP_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtCsipLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}
#define CSIP_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtCsipLto, 0, __VA_ARGS__))

#else

#define CSIP_INFO(...)
#define CSIP_WARNING(...)
#define CSIP_ERROR(...)
#define CSIP_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}
#define CSIP_DEBUG(...)
#endif

#endif /* csip_DEBUG_H_ */
