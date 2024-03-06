/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

/*
FILE NAME
    gatt_transmit_power_server_debug.h

DESCRIPTION
    Header file for the GATT Transmit Power Service debug functionality.
*/
#ifndef GATT_TRANSMIT_POWER_SERVER_DEBUG_H_
#define GATT_TRANSMIT_POWER_SERVER_DEBUG_H_
#include "csr_panic.h"

/*#define GATT_TPS_SERVER_DEBUG_LIB */

/* Macro used to generate debug version of this library */
#ifdef GATT_TPS_SERVER_DEBUG_LIB


#include <stdio.h>


#define PRINT(x) printf x

#define GATT_TPS_SERVER_DEBUG_INFO(x) {PRINT(("%s:%d - ", __FILE__, __LINE__)); PRINT(x);}

#define GATT_TPS_SERVER_DEBUG_PANIC(x) {GATT_TPS_SERVER_DEBUG_INFO(x); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, "x");}
#define GATT_TPS_SERVER_PANIC(x) {GATT_TPS_SERVER_DEBUG_INFO(x); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, "x");}

#else

#define GATT_TPS_SERVER_DEBUG_INFO(x)
#define GATT_TPS_SERVER_DEBUG_PANIC(x)
#define GATT_TPS_SERVER_PANIC(x) {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, "x");}

#endif /* GATT_TPS_SERVER_DEBUG_LIB */


#endif /* GATT_TRANSMIT_POWER_SERVER_DEBUG_H_ */

