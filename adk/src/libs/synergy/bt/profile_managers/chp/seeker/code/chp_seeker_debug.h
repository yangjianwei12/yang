/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef CHP_SEEKER_DEBUG_H_
#define CHP_SEEKER_DEBUG_H_

#include "chp_seeker_private.h"

#ifdef CHP_SEEKER_LOG_ENABLE

#define CHP_INFO(...) CSR_LOG_TEXT_INFO((CsrBtChpsLto, 0, __VA_ARGS__))
#define CHP_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtChpsLto, 0, __VA_ARGS__))
#define CHP_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtChpsLto, 0, __VA_ARGS__))
#define CHP_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtChpsLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define CHP_INFO(...)
#define CHP_WARNING(...)
#define CHP_ERROR(...)
#define CHP_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}
#endif

#endif /* CHP_SEEKER_DEBUG_H_ */
