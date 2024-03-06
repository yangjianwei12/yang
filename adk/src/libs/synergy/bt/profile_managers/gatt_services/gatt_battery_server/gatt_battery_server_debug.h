/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_BATTERY_SERVER_DEBUG_H_
#define GATT_BATTERY_SERVER_DEBUG_H_

#include <stdio.h>
#include "csr_panic.h"


/* Enable this define to enable debug log */
/*#define GATT_BATTERY_SERVER_DEBUG_LIB*/

/* Macro used to enable debug logs in this library */
#ifdef GATT_BATTERY_SERVER_DEBUG_LIB


#ifndef DEBUG_PRINT_ENABLED
#define DEBUG_PRINT_ENABLED
#endif

#define PRINT(x) printf x

#define GATT_BATTERY_SERVER_DEBUG_INFO(x) {PRINT(("%s:%d - ", __FILE__, __LINE__)); PRINT(x);}
#define GATT_BATTERY_SERVER_DEBUG_PANIC(x) {GATT_BATTERY_SERVER_DEBUG_INFO(x); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, "x");}
#define GATT_BATTERY_SERVER_PANIC(x) {GATT_BATTERY_SERVER_DEBUG_INFO(x); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, "x");}


#else /* GATT_BATTERY_DEBUG_LIB */


#define GATT_BATTERY_SERVER_DEBUG_INFO(x)
#define GATT_BATTERY_SERVER_DEBUG_PANIC(x)
#define GATT_BATTERY_SERVER_PANIC(x) {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, "x");}

#endif /* GATT_BATTERY_SERVER_DEBUG_LIB */


#endif /* GATT_BATTERY_SERVER_DEBUG_H_ */
