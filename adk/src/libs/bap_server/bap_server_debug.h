/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd.
* 
************************************************************************* ***/

#ifndef BAP_SERVER_DEBUG_H_
#define BAP_SERVER_DEBUG_H_

#include <stdio.h>

/* Macro used to generate debug version of this library */
/*#define BAP_SERVER_DEBUG_LIB*/

#ifdef BAP_SERVER_DEBUG_LIB

#include <panic.h>
#include <print.h>
#include <stdio.h>


#define BAP_DEBUG_INFO(x) {PRINT(("%s:%d - ", __FILE__, __LINE__)); PRINT(x);}
#define BAP_DEBUG_PANIC(x) {BAP_DEBUG_INFO(x);  Panic();}

#else /* BAP_SERVER_DEBUG_LIB */

#define BAP_DEBUG_INFO(x)
#define BAP_DEBUG_PANIC(x)
#define BAP_PANIC(x) {Panic();}

#endif /* BAP_DEBUG_LIB */


#endif /* BAP_SERVER_DEBUG_H_ */
