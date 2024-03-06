/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
/*
FILE NAME
    gatt_pacs_server_debug.h

DESCRIPTION
    Header file for the GATT PACS server library debug functionality.
*/
#ifndef GATT_PACS_SERVER_DEBUG_H_
#define GATT_PACS_SERVER_DEBUG_H_

#include "lea_logging.h"

#define LEA_PACS_LOG_ENABLE 0x00000001

#if (defined(LEA_PACS_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_PACS_LOG_ENABLE & LEA_LOG_MASK))

#define GATT_PACS_SERVER_INFO(...) CSR_LOG_TEXT_INFO((CsrBtPacsLto, 0, __VA_ARGS__))
#define GATT_PACS_SERVER_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtPacsLto, 0, __VA_ARGS__))
#define GATT_PACS_SERVER_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtPacsLto, 0, __VA_ARGS__))
#define GATT_PACS_SERVER_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtPacsLto, 0, __VA_ARGS__))
#define GATT_PACS_SERVER_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtPacsLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define GATT_PACS_SERVER_INFO(...)
#define GATT_PACS_SERVER_DEBUG(...)
#define GATT_PACS_SERVER_WARNING(...)
#define GATT_PACS_SERVER_ERROR(...)
#define GATT_PACS_SERVER_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#endif

#endif /* GATT_PACS_SERVER_DEBUG_H_ */
