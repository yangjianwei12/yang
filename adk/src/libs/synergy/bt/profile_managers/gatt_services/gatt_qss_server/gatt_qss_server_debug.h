/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

#ifndef GATT_QSS_SERVER_DEBUG_H_
#define GATT_QSS_SERVER_DEBUG_H_

#include<stdio.h>
#include "csr_panic.h"

/* Enable this define to enable debug log */
/* #define GATT_QSS_SERVER_DEBUG_LIB */

/* Macro used to enable debug logs in this library */
#ifdef GATT_QSS_SERVER_DEBUG_LIB


#ifndef DEBUG_PRINT_ENABLED
#define DEBUG_PRINT_ENABLED
#endif

#define PRINT(x) printf x

#define GATT_QSS_SERVER_DEBUG_INFO(x) {PRINT(("%s:%d - -", __FILE__, __LINE__)); PRINT(x);}
#define GATT_QSS_SERVER_ERROR(x) {PRINT(("%s:%d - -", __FILE__, __LINE__)); PRINT(x);}
#define GATT_QSS_SERVER_DEBUG_PANIC(x) {GATT_QSS_SERVER_DEBUG_INFO(x); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, "x");}
#define GATT_QSS_SERVER_PANIC(x) {GATT_QSS_SERVER_DEBUG_INFO(x); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, "x");}

#else /* GATT_QSS_SERVER_DEBUG_LIB */

#define GATT_QSS_SERVER_DEBUG_INFO(x)
#define GATT_QSS_SERVER_ERROR(x)
#define GATT_QSS_SERVER_DEBUG_PANIC(x)
#define GATT_QSS_SERVER_PANIC(x) {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, "x");}

#endif /* GATT_QSS_SERVER_DEBUG_LIB */

#endif /* GATT_QSS_SERVER_DEBUG_H_ */