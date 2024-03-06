/****************************************************************************
 * Copyright (c) 2019 - 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \ingroup capabilities
 * \file  aanc2_defs.h
 * \ingroup AANC
 *
 * Adaptive ANC (AANC) operator shared definitions and include files
 *
 */

#ifndef _AANC2_DEFS_H_
#define _AANC2_DEFS_H_

#include "fxlms100_public.h"
#include "ed100_public.h"
#include "hc100_public.h"
#include "aanc_afb_public.h"
#include "aanc_security_public.h"

#define AANC2_DEFAULT_FRAME_SIZE    64   /* 4 ms at 16k */
#define AANC2_DEFAULT_BLOCK_SIZE    0.5 * AANC2_DEFAULT_FRAME_SIZE
#define AANC2_DEFAULT_BUFFER_SIZE   2 * AANC2_DEFAULT_FRAME_SIZE
#define AANC2_INTERNAL_BUFFER_SIZE  AANC2_DEFAULT_FRAME_SIZE + 1
#define AANC2_FRAME_RATE            250   /* Fs = 16kHz, frame size = 64 */

/* Timer parameter is Q12.N */
#define AANC2_TIMER_PARAM_SHIFT     20

#endif /* _AANC2_DEFS_H_ */