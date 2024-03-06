/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#ifndef _SDPLIB_PRIVATE_H_
#define _SDPLIB_PRIVATE_H_

#include "qbl_adapter_types.h"
#include "qbl_adapter_pmalloc.h"
#include "qbl_adapter_scheduler.h"

#if defined(USE_BLUESTACK_DLL)
#include "bluestack.h"        /* The standard BlueStack interface */
#else
#include INC_DIR(bluestack,bluetooth.h)
#include INC_DIR(common,common.h)           /* Common Functions - strlog */
#include INC_DIR(mblk,mblk.h)
#endif

#include "sdclib.h"
#include "sdslib.h"
#include INC_DIR(bluestack,sds_prim.h)
#include INC_DIR(common,bluestacklib.h)





#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

#endif
