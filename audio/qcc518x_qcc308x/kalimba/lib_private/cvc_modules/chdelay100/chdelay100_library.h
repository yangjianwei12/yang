// *****************************************************************************
// Copyright (c) 2022 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#ifndef _CHDELAY100_LIB_H
#define _CHDELAY100_LIB_H

// ***************************************
// Channel Delay data structure for ASM
// ***************************************
.CONST $MAX_DELAY_FRAMES                            5;
.CONST $chdelay100.INPUT_DATA_PTR                   MK1 * 0;
.CONST $chdelay100.DELAYBUF_START_PTR               MK1 * 1;
.CONST $chdelay100.DELAY_IN_FRAMES                  MK1 * 2;
.CONST $chdelay100.NUM_BINS                         MK1 * 3;
.CONST $chdelay100.DELAYBUF_READ_PTR                MK1 * 4;
.CONST $chdelay100.DELAYBUF_WRITE_PTR               MK1 * 5;
.CONST $chdelay100.DELAYBUF_FRAMESIZE               MK1 * 6;
.CONST $chdelay100.DELAYBUF_LEN                     MK1 * 7;
.CONST $chdelay100.OUTPUT_BUF_PTR                   MK1 * 8;
.CONST $chdelay100.STRUC_SIZE                             9;

#endif // _CHDELAY100_LIB_H