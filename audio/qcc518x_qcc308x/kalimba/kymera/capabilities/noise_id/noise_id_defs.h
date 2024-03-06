/****************************************************************************
 * Copyright (c) 2022 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  noise_id_defs.h
 * \ingroup noise_id
 *
 * Noise ID operator shared definitions and include files
 *
 */


#include "aanc_afb_public.h"

#ifndef _NOISE_ID_DEFS_H_
#define _NOISE_ID_DEFS_H_

#define NOISE_ID_DEFAULT_SAMPLE_RATE 16000
#define NOISE_ID_DEFAULT_FRAME_SIZE  64
#define NOISE_ID_DEFAULT_BLOCK_SIZE  0.5 * NOISE_ID_DEFAULT_FRAME_SIZE
#define NOISE_ID_DEFAULT_BUFFER_SIZE 2 * NOISE_ID_DEFAULT_FRAME_SIZE

#endif /* _NOISE_ID_DEFS_H_ */
