/*******************************************************************************

Copyright (C) 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*
FILE NAME
    gatt_hids_server_debug.h

DESCRIPTION
    Header file for the GATT HIDS library debug functionality.
*/
#ifndef GATT_HIDS_SERVER_DEBUG_H_
#define GATT_HIDS_SERVER_DEBUG_H_

#include "lea_logging.h"

#define LEA_HIDS_LOG_ENABLE 0x00200000

#if (defined(LEA_HIDS_LOG_ENABLE) && defined(LEA_LOG_MASK) && (LEA_HIDS_LOG_ENABLE & LEA_LOG_MASK))

#define GATT_HIDS_SERVER_DEBUG(...) CSR_LOG_TEXT_DEBUG((CsrBtHidsLto, 0, __VA_ARGS__))
#define GATT_HIDS_SERVER_INFO(...) CSR_LOG_TEXT_INFO((CsrBtHidsLto, 0, __VA_ARGS__))
#define GATT_HIDS_SERVER_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtHidsLto, 0, __VA_ARGS__))
#define GATT_HIDS_SERVER_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtHidsLto, 0, __VA_ARGS__))
#define GATT_HIDS_SERVER_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtHidsLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define GATT_HIDS_SERVER_DEBUG(...)
#define GATT_HIDS_SERVER_INFO(...)
#define GATT_HIDS_SERVER_WARNING(...)
#define GATT_HIDS_SERVER_ERROR(...)
#define GATT_HIDS_SERVER_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#endif

#endif /* GATT_HIDS_SERVER_DEBUG_H_ */


