/****************************************************************************
 * Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.
 ************************************************************************//**
 * \file pl_assert.h
 * \ingroup platform
 *
 * Definition of assert macros
 *
 ****************************************************************************/

#if !defined(PL_ASSERT_H)
#define PL_ASSERT_H

/****************************************************************************
Include Files
*/

#include "platform/pl_trace.h"
#ifdef DESKTOP_TEST_BUILD
#include <stdio.h>
#elif !defined(NDEBUG)
#include "panic/panic.h"
#endif

/****************************************************************************
Public Macro Declarations
*/

/**
 * Use our own assert function that doesn't take any
 * parameters to save code space
 */

#ifdef DESKTOP_TEST_BUILD
#define PL_ASSERT(cond) (!(cond) ? printf("Warning : Assertion failed in %s : Line %d\n", __FILE__, __LINE__) : (void) 0)
#elif defined(NDEBUG)
#define PL_ASSERT(cond) ((void)0)
#else
#define PL_ASSERT(x) ( (x) ? ((void)0) : panic_diatribe(PANIC_HYDRA_ASSERTION_FAILED, __LINE__) )
#endif

#define COMPILE_TIME_ASSERT(expr, msg) struct compile_time_assert_ ## msg { \
    int compile_time_assert_ ## msg [1 - (!(expr))*2]; \
}

/****************************************************************************
Public Type Declarations
*/

/****************************************************************************
Global Variable Definitions
*/

/****************************************************************************
Public Function Prototypes
*/

extern void exit( int);

extern void abort( void);

#endif /* TEMPLATE_H */
