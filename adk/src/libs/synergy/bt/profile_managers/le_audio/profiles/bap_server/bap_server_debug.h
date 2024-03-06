/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd.
* %%version
************************************************************************* ***/

#ifndef BAP_SERVER_DEBUG_H_
#define BAP_SERVER_DEBUG_H_

#include "lea_logging.h"

#define LEA_BAP_LOG_ENABLE 0x00000008


#if (defined(LEA_BAP_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_BAP_LOG_ENABLE & LEA_LOG_MASK))

#define BAP_SERVER_INFO(...) CSR_LOG_TEXT_INFO((CsrBtBapLto, 0, __VA_ARGS__))
#define BAP_SERVER_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtBapLto, 0, __VA_ARGS__))
#define BAP_SERVER_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtBapLto, 0, __VA_ARGS__))
#define BAP_SERVER_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtBapLto, 0, __VA_ARGS__))
#define BAP_SERVER_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtBapLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define BAP_SERVER_INFO(...)
#define BAP_SERVER_DEBUG(...)
#define BAP_SERVER_WARNING(...)
#define BAP_SERVER_ERROR(...)
#define BAP_SERVER_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#endif

#endif /* BAP_SERVER_DEBUG_H_ */
