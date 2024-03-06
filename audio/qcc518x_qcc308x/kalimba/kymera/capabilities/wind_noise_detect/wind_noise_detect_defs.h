/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  wind_noise_detect_defs.h
 * \ingroup wind_noise_detect
 *
 * Wind noise detect operator shared definitions and include files
 *
 */


#ifndef _WIND_NOISE_DETECT_DEFS_H_
#define _WIND_NOISE_DETECT_DEFS_H_

#define WND_DEFAULT_FRAME_SIZE  64
#define WND_DEFAULT_FRAME_RATE  250
#define WND_DEFAULT_BLOCK_SIZE  0.5 * WND_DEFAULT_FRAME_SIZE
#define WND_DEFAULT_BUFFER_SIZE 2 * WND_DEFAULT_FRAME_SIZE

#define WND_1MIC_BLOCK_SIZE     128
#define WND_2MIC_BLOCK_SIZE     16

/* Timer parameter is Q12.N */
#define WND_TIMER_PARAM_SHIFT   20

#endif /* _WIND_NOISE_DETECT_DEFS_H_ */