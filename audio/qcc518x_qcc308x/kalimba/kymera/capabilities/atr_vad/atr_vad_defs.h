/****************************************************************************
 * Copyright (c) 2022 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  atr_vad_defs.h
 * \ingroup atr_vad
 *
 * Auto Transparency VAD operator shared definitions and include files
 *
 */


#ifndef _ATR_VAD_DEFS_H_
#define _ATR_VAD_DEFS_H_

/* ATR VAD operation is configured to match CVC WB (16e3, 7.5ms frame) */
#define ATR_VAD_DEFAULT_SAMPLE_RATE 16000
#define ATR_VAD_DEFAULT_FRAME_RATE  133
#define ATR_VAD_DEFAULT_FRAME_SIZE  120
#define ATR_VAD_DEFAULT_BLOCK_SIZE  0.5 * ATR_VAD_DEFAULT_FRAME_SIZE
#define ATR_VAD_DEFAULT_BUFFER_SIZE 2 * ATR_VAD_DEFAULT_FRAME_SIZE

/* ATR timer parameter is Q15.17 */
#define ATR_VAD_TIMER_PARAM_SHIFT   17

#endif /* _ATR_VAD_DEFS_H_ */