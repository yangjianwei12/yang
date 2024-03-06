/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

/*
FILE NAME
    gatt_ascs_server_debug.h

DESCRIPTION
    Header file for the GATT ASES library debug functionality.
*/
#ifndef GATT_ASCS_SERVER_DEBUG_H_
#define GATT_ASCS_SERVER_DEBUG_H_

#include <panic.h>

/* Macro used to generate debug version of this library */
#ifdef GATT_ASCS_SERVER_DEBUG_LIB

#include <print.h>

#define GATT_ASCS_SERVER_DEBUG_INFO(x) {PRINT(("%s:%d - ", __FILE__, __LINE__)); PRINT(x);}

#define GATT_ASCS_SERVER_DEBUG_PANIC(x) {GATT_ASCS_SERVER_DEBUG_INFO(x); Panic();}
#define GATT_ASCS_SERVER_PANIC(MSG) {GATT_ASCS_SERVER_DEBUG_INFO(MSG); Panic();}

#else

#define GATT_ASCS_SERVER_DEBUG_INFO(x)
#define GATT_ASCS_SERVER_DEBUG_PANIC(x)
#define GATT_ASCS_SERVER_PANIC(MSG) {Panic();}

#endif /* GATT_ASCS_SERVER_DEBUG_LIB */


#endif /* GATT_ASCS_SERVER_DEBUG_H_ */

