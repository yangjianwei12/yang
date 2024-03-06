/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 ************************************************************************//**
 * \file pl_profiler.c
 * \ingroup platform
 *
 * Limited C implementation of profiler functionality
 * Most of the code here is just stubs. It exists only to allow unit tests 
 * to build with PROFILER_ON defined.
 *
 ****************************************************************************/

/* This is only needed for host-based unit tests */
#ifdef DESKTOP_TEST_BUILD

/****************************************************************************
Include Files
*/
#include "platform/profiler_c.h"

/****************************************************************************
Private Macro Declarations
*/

/****************************************************************************
Global Variable Definitions
*/

profiler sleep_time = STATIC_PROFILER_INIT(UNINITIALISED_PROFILER, 0, 0, 0);

tTimerId profiler_timer_id = TIMER_ID_INVALID;

profiler* profiler_list = NULL;

/****************************************************************************
Private Type Declarations
*/

/****************************************************************************
Private Constant Declarations
*/

/****************************************************************************
Private Variable Definitions
*/

/****************************************************************************
Private Function Prototypes
*/

/****************************************************************************
Private Function Definitions
*/

/****************************************************************************
Public Function Definitions
*/

void profiler_start(void* address)
{
}

void profiler_stop(void* address)
{
}

void profiler_enable(void)
{
}

void profiler_disable(void)
{
}

bool get_profiler_state(void)
{
    return FALSE;
}

void profiler_deregister_all(void)
{
}

void profiler_initialise(void)
{
    /* Hard-coded value for unit tests */
    sleep_time.cpu_fraction = 700;
}

#endif /* DESKTOP_TEST_BUILD */
