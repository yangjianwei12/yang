/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef VCP_DEBUG_H_
#define VCP_DEBUG_H_

#include <stdlib.h>

/* Macro used to generate debug version of this library */
#ifdef VCP_DEBUG_LIB


#ifndef DEBUG_PRINT_ENABLED
#define DEBUG_PRINT_ENABLED
#endif

#include <panic.h>
#include <print.h>
#include <stdio.h>

#define VCP_DEBUG_INFO(x) {PRINT(("%s:%d - ", __FILE__, __LINE__)); PRINT(x);}
#define VCP_DEBUG_PANIC(x) {VCP_DEBUG_INFO(x); Panic();}
#define VCP_PANIC(x) {VCP_DEBUG_INFO(x); Panic();}


#else /* VCP_DEBUG_LIB */


#define VCP_DEBUG_INFO(x)
#define VCP_DEBUG_PANIC(x)
#define VCP_PANIC(x) {Panic();}

#endif /* VCP_DEBUG_LIB */


#endif /* VCP_DEBUG_H_ */
