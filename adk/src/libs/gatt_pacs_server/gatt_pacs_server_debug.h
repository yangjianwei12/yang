/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

/*
FILE NAME
    gatt_pacs_server_debug.h

DESCRIPTION
    Header file for the GATT PACS server library debug functionality.
*/
#ifndef GATT_PACS_SERVER_DEBUG_H_
#define GATT_PACS_SERVER_DEBUG_H_

#include <panic.h>

/* Macro used to generate debug version of this library */
#ifdef GATT_PACS_SERVER_DEBUG_LIB

#include <print.h>
#include <stdio.h>

#define GATT_PACS_SERVER_DEBUG_INFO(x) {PRINT(("%s:%d - ", __FILE__, __LINE__)); PRINT(x);}

#define GATT_PACS_SERVER_DEBUG_PANIC(x) {GATT_PACS_SERVER_DEBUG_INFO(x); Panic();}
#define GATT_PACS_SERVER_PANIC(x) {GATT_PACS_SERVER_DEBUG_INFO(x); Panic();}

#else

#define GATT_PACS_SERVER_DEBUG_INFO(x)
#define GATT_PACS_SERVER_DEBUG_PANIC(x)
#define GATT_PACS_SERVER_PANIC(x) {Panic();}

#endif /* GATT_PACS_SERVER_DEBUG_LIB */


#endif /* GATT_PACS_SERVER_DEBUG_H_ */

