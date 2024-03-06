/****************************************************************************
 * Copyright (c) 2015 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup lib_private\aanc
 *
 * \file  hc100_defs_public.h
 * \ingroup lib_private\aanc
 *
 * HC100 library header file providing public definitions common to C and
 * ASM code.
 */
#ifndef _HC100_LIB_DEFS_PUBLIC_H_
#define _HC100_LIB_DEFS_PUBLIC_H_

/******************************************************************************
Public Constant Definitions
*/

/* Frame size  */
#define HC100_FRAME_SIZE                        64
#define HC100_NUM_FREQ_BIN                      65
#define HC100_FFT_SIZE                          128
#define HC100_FFT_SIZE_HALF                     64
#define HC100_FFT_SIZE_POW2                     7
#define HC100_COUNTER_TABLE_SIZE                8
#define HC100_NEIGHBOUR_BIN_OFFSET              2
#define HC100_BIN2_OFFSET                       (2 * ADDR_PER_WORD)
#define HC100_BIN3_OFFSET                       (3 * ADDR_PER_WORD)
/* First three and last two are special bins as their neighbour bins are
 * calcualted differently. (5 = 3 + 2) */
#define HC100_NUM_SPECIAL_BINS                  5

#endif /* _HC100_LIB_DEFS_PUBLIC_H_ */
