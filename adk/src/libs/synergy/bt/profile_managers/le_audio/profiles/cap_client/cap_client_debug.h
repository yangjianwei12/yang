/* Copyright (c) 2022 Qualcomm Technologies International, Ltd. */
/* %%version */


#ifndef CAP_CLIENT_DEBUG_H_
#define CAP_CLIENT_DEBUG_H_


#include "lea_logging.h"

#define LEA_CAP_LOG_ENABLE 0x00040000

#if (defined(LEA_CAP_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_CAP_LOG_ENABLE & LEA_LOG_MASK))

#define CAP_CLIENT_INFO(...)  CSR_LOG_TEXT_INFO((CsrBtCapLto, 0, __VA_ARGS__))
#define CAP_CLIENT_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtCapLto, 0, __VA_ARGS__))
#define CAP_CLIENT_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtCapLto, 0, __VA_ARGS__))
#define CAP_CLIENT_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtCapLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define CAP_CLIENT_INFO(...)
#define CAP_CLIENT_WARNING(...)
#define CAP_CLIENT_ERROR(...)
#define CAP_CLIENT_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}


#endif

#endif /* CAP_CLIENT_DEBUG_H_ */
