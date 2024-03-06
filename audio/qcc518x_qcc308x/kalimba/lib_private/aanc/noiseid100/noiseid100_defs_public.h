/****************************************************************************
 * Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  noiseid100_defs_public.h
 * \ingroup lib_private\aanc
 *
 * NOISEID100 library header file providing public definitions common to C and
 * ASM code.
 */
#ifndef _NOISEID100_LIB_DEFS_PUBLIC_H_
#define _NOISEID100_LIB_DEFS_PUBLIC_H_

/******************************************************************************
Public Constant Definitions
*/


#define NOISEID100_NUM_BINS                     4 /* Bins used 2,3,4 and 5 */
#define NOISEID100_NUM_IDS                      2

#define NOISEID100_DEFAULT_FRAME_SIZE           64
#define NOISEID100_DEFAULT_FRAME_RATE           250

/* Timer parameter in Q12.N */
#define NOISEID100_TIMER_PARAM_SHIFT            20
/* Max timer value in seconds */
#define NOISEID100_MAX_TIMER_VALUE              20


#endif /* _NOISEID100_LIB_DEFS_PUBLIC_H_ */
