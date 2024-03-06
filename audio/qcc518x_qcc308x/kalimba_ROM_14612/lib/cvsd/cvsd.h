/****************************************************************************
 * Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup cvsd Continuously Variable Slope Delta modulation
 *
 * Library to compress and decompress audio using the CVSD codec.
 */
/**
 * \file  cvsd.h
 * \ingroup cvsd 
 *
 * CVSD codec public header file.
 */

#ifndef CVSD_LIB_H
#define CVSD_LIB_H

#include "cvsd_struct.h"

#define SCRATCH_SIZE_WORDS 256*8

void cvsd_receive_asm(sCvsdState_t* cvsd_struct, tCbuffer* bufIn, tCbuffer* bufOut, int* ptScratch, int knSamples);
void cvsd_send_asm(sCvsdState_t* cvsd_struct, tCbuffer* bufIn, tCbuffer* bufOut, int* ptScratch, int knSamples);

#endif /* CVSD_LIB_H */
