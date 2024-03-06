/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  anc_hw_manager_event.h
 * \ingroup anc_hw_manager
 *
 * ANC HW Manager (AHM) operator event definitions
 *
 */

#ifndef _ANC_HW_MANAGER_EVENT_H_
#define _ANC_HW_MANAGER_EVENT_H_

/******************************************************************************
Include Files
*/
#include "types.h"
#include "opmgr/opmgr_operator_data.h"
/******************************************************************************
Public Definitions
*/

/* Event IDs */
#define AHM_EVENT_ID_FF_RAMP            0
#define AHM_EVENT_ID_FB_RAMP            1
#define AHM_EVENT_ID_WND_1MIC           2
#define AHM_EVENT_ID_WND_2MIC           3
#define AHM_EVENT_ID_ATR_VAD_1MIC       4
#define AHM_EVENT_ID_ATR_VAD_2MIC       5
#define AHM_EVENT_ID_FILTER_TRANSITION  6
#define AHM_EVENT_ID_EC_RAMP            7
#define AHM_EVENT_ID_NOISE_ID           8

/* Event type encoding */
#define AHM_EVENT_TYPE_TRIGGER      0
#define AHM_EVENT_TYPE_CLEAR        1

/******************************************************************************
Public Type Declarations
*/

/* Represent the state of an AHM event */
typedef enum
{
    AHM_EVENT_CLEAR,
    AHM_EVENT_DETECTED,
    AHM_EVENT_SENT
} AHM_EVENT_STATE;

/* Represent AHM event messaging states */
typedef struct _AHM_EVENT
{
    unsigned frame_counter;
    unsigned set_frames;
    AHM_EVENT_STATE running;
} AHM_EVENT;

typedef struct _AHM_EVENT_MSG
{
    uint16 id;
    uint16 type;
    unsigned payload;
    EXT_OP_ID ext_op_id;
} AHM_EVENT_MSG;

/******************************************************************************
Public Function Definitions
*/

#endif /* _ANC_HW_MANAGER_EVENT_H_ */
