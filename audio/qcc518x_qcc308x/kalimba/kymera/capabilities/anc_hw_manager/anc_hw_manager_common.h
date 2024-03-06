/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  anc_hw_manager_common.h
 * \ingroup anc_hw_manager
 *
 * ANC HW Manager (AHM) operator shared definitions and include files
 *
 */

#ifndef _ANC_HW_MANAGER_COMMON_H_
#define _ANC_HW_MANAGER_COMMON_H_

/******************************************************************************
 * Include Files
 * */

#include "audio_log/audio_log.h"

/****************************************************************************
 * Public Variable Definitions
 * */

#define AHM_NUM_DYNAMIC_FILTERS          3
/* Feedforward and hybrid clock enable bitfields */
#define AHM_HYBRID_ENABLE               (AHM_ANC_ENABLE_FFA_MASK | \
                                         AHM_ANC_ENABLE_FFB_MASK | \
                                         AHM_ANC_ENABLE_FB_MASK |  \
                                         AHM_ANC_ENABLE_OUT_MASK)
#define AHM_FEEDFORWARD_ENABLE          (AHM_ANC_ENABLE_FFA_MASK | \
                                         AHM_ANC_ENABLE_OUT_MASK)
/******************************************************************************
ANC HW type definitions
*/

/* Represent ANC channels */
typedef enum
{
    AHM_ANC_INSTANCE_NONE_ID = 0x0000,
    AHM_ANC_INSTANCE_ANC0_ID = 0x0001,
    AHM_ANC_INSTANCE_ANC1_ID = 0x0002,
    AHM_ANC_INSTANCE_BOTH_ID = 0x0003,
    AHM_ANC_INSTANCE_DUAL_ID = 0x0004
} AHM_ANC_INSTANCE;

/* Represent ANC paths */
typedef enum
{
    AHM_ANC_PATH_NONE_ID = 0x0000,
    AHM_ANC_PATH_FFA_ID = 0x0001,
    AHM_ANC_PATH_FFB_ID = 0x0002,
    AHM_ANC_PATH_FB_ID = 0x0003,
    AHM_ANC_PATH_SM_LPF_ID = 0x0004
} AHM_ANC_PATH;

/* Represent ANC clock enables */
typedef enum
{
    AHM_ANC_ENABLE_FFA_MASK = 0x0001,
    AHM_ANC_ENABLE_FFB_MASK = 0x0002,
    AHM_ANC_ENABLE_FB_MASK = 0x0004,
    AHM_ANC_ENABLE_OUT_MASK = 0x0008
} AHM_ANC_ENABLE;

/* Represent ANC filters */
typedef enum
{
    AHM_ANC_FILTER_FF_ID = 0x0000,
    AHM_ANC_FILTER_FB_ID = 0x0001,
    AHM_ANC_FILTER_EC_ID = 0x0002,
    AHM_ANC_FILTER_RX_FFA_MIX_ID = 0x0003,
    AHM_ANC_FILTER_RX_FFB_MIX_ID = 0x0004,
    AHM_ANC_FILTER_FF_AND_FB_ID = 0x0005
} AHM_ANC_FILTER;

typedef struct _AHM_ANC_CONFIG
{
    AHM_ANC_INSTANCE channel;  /* ANC channel that is controlled */
    AHM_ANC_PATH ff_path;      /* ANC FF path that is controlled */
    AHM_ANC_PATH fb_path;      /* ANC FB path that is controlled */
} AHM_ANC_CONFIG;

/* Represent gain control types */
typedef enum
{
    AHM_GAIN_CONTROL_TYPE_DELTA    = 0x00,
    AHM_GAIN_CONTROL_TYPE_NOMINAL  = 0x01,
    AHM_GAIN_CONTROL_TYPE_MAX_ID   = 0x02
} AHM_GAIN_CONTROL_TYPE;

#endif /* _ANC_HW_MANAGER_COMMON_H_ */
