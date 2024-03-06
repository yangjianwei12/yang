/****************************************************************************
 * Copyright (c) 2011 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  kip_mgr_private.h
 * \ingroup kip_mgr
 *
 */

#ifndef KIP_MGR_PRIVATE_H
#define KIP_MGR_PRIVATE_H

/****************************************************************************
Include Files
*/

#include "kip_mgr/kip_mgr.h"
#include "ipc/ipc_kip.h"
#include "hal/hal_dm_sections.h"
#include "audio_log/audio_log.h"

/****************************************************************************
Type Declarations
*/

typedef struct
{
    /* For now, just data channel ID. It is assumed the initiator of the kick
     * mirrors the data channel ID such that it can be understood by 'receiver' of
     * the kick on the other processor!
     */
    uint16 data_channel_id;

} KICK_SIGNAL_PARAMS;

typedef enum
{
    KIP_IDLE = 0,
    KIP_START,
    KIP_STOP
} KIP_REASON;

/* Info kept for in-progress processor start/stop cmd from some command adaptor */
typedef struct
{
    CONNECTION_LINK con_id;
    KIP_STATUS_CBACK callback;
    KIP_REASON reason;
} AUX_PROC_KIP_IPC_STATE;

/****************************************************************************
Public Constant Declarations
*/
/* Base value of indirect message IDs (resps have 0x8000 added to these) */
#define KIP_INDIRECT_MSG_ID_BASE        0x4000

/****************************************************************************
Public Variable Definitions
*/
extern AUX_PROC_KIP_IPC_STATE aux_proc_kip_ipc_state;

extern uint16 *msg_channel_ids;

extern uint16 kip_kick_signal_chn_id;
extern uint16 kip_reverse_kick_signal_chn_id;

/* Setup message and non-static signal callbacks, keep message
   channel ID in housekeeping table. */
extern IPC_STATUS kip_setup_comms(void);

#endif /* KIP_MGR_PRIVATE_H */
