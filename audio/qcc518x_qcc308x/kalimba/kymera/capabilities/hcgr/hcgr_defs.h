/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \ingroup capabilities
 * \file  hcgr.h
 * \ingroup HCGR
 *
 * Howling Control and Gain Recovery operator shared definitions and include files
 *
 */


#ifndef _HCGR_DEFS_H_
#define _HCGR_DEFS_H_

#include "aanc_afb_public.h"
#include "hc100_public.h"
#include "aanc_security_public.h"

#define HCGR_DEFAULT_FRAME_SIZE    64   /* 4 ms at 16k */
#define HCGR_DEFAULT_BLOCK_SIZE    0.5 * HCGR_DEFAULT_FRAME_SIZE
#define HCGR_DEFAULT_BUFFER_SIZE   2 * HCGR_DEFAULT_FRAME_SIZE
#define HCGR_INTERNAL_BUFFER_SIZE  HCGR_DEFAULT_FRAME_SIZE + 1
#define HCGR_FRAME_RATE            250   /* Fs = 16kHz, frame size = 64 */
#define HCGR_FLAGS_HOWLING         0x00000001
#endif /* _HCGR_DEFS_H_ */
