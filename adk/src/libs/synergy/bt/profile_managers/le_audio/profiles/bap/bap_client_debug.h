/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd.
* %%version
************************************************************************* ***/

#ifndef BAP_CLIENT_DEBUG_H_
#define BAP_CLIENT_DEBUG_H_


#include "lea_logging.h"

#define LEA_BAP_LOG_ENABLE 0x00000008


#if (defined(LEA_BAP_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_BAP_LOG_ENABLE & LEA_LOG_MASK))

#define BAP_CLIENT_INFO(...) CSR_LOG_TEXT_INFO((CsrBtBapLto, 0, __VA_ARGS__))
#define BAP_CLIENT_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtBapLto, 0, __VA_ARGS__))
#define BAP_CLIENT_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtBapLto, 0, __VA_ARGS__))
#define BAP_CLIENT_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtBapLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}
#define BAP_CLIENT_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtBapLto, 0, __VA_ARGS__))
#else

#define BAP_CLIENT_INFO(...)
#define BAP_CLIENT_WARNING(...)
#define BAP_CLIENT_ERROR(...)
#define BAP_CLIENT_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}
#define BAP_CLIENT_DEBUG(...)
#endif


#endif /* BAP_CLIENT_DEBUG_H_ */
