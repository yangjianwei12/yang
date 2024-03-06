/****************************************************************************
 * Copyright (c) 2013 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file hal_time.h
 * \ingroup HAL
 *
 * Public header file for HAL get time function.
 */
#ifndef _HAL_TIME_H_
#define _HAL_TIME_H_

#include "types.h"

/* 32-bit time in 32-bit number */
#define INT_TIME(t) ((int32)(t))
#define hal_time_sub(t1, t2)    INT_TIME(((TIME_INTERVAL)(t1) - (TIME_INTERVAL)(t2)))
#define hal_time_ge(t1, t2) (hal_time_sub((t1), (t2)) >= 0)

/**
 * \brief Tight loop to wait for a given number of microseconds for hal operations.
 *
 * \param delay Number of microseconds to delay
 */
extern void hal_delay_us(TIME delay);

#ifdef DESKTOP_TEST_BUILD
extern void (*hal_delay_us_test_hook)(unsigned delay);
#endif

#endif /* _HAL_TIME_H_ */
