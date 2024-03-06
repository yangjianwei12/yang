/****************************************************************************
 * Copyright (c) 2011 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  kip_mgr.h
 * \ingroup kip_mgr
 *
 */

#ifndef KIP_MGR_H
#define KIP_MGR_H

#include "adaptor/connection_id.h"
#include "proc/proc.h"
#include "status_prim.h"

/* We only support two signals - KICK FWD and BACK - at the moment */
#define KIP_NUM_SIGNALS 2

typedef bool (*KIP_STATUS_CBACK)(CONNECTION_LINK con_id,
                                 STATUS_KYMERA status);

/* Dedicated signal IDs */
typedef enum
{
    KIP_SIGNAL_ID_KICK         = 0x9000,
    KIP_SIGNAL_ID_REVERSE_KICK = 0x9001,
} KIP_SIGNAL_ID_KALIMBA;

/****************************************************************************
Public Function Declarations
*/
/* initialise KIP */
extern void kip_init(void);

/* Init the KIP Mgr */
extern bool kip_mgr_init(void);

/* Shutdown KIP Mgr - just final clean-up steps,
   optionally called at system shutdown only. */
extern void kip_mgr_shutdown(void);

/* Get the message channel ID for a certain destination processor,
   returns 0 for fail. */
extern uint16 kip_get_msg_chn_id_for_processor(PROC_ID_NUM processor_id);

/* Get maximum valid processor ID depending on what image and
   where is it called. */
extern PROC_ID_NUM kip_get_max_processor_id(void);

/* Determine whether an auxiliary processor is up-and-running or not. */
extern bool kip_aux_processor_has_started(PROC_ID_NUM proc_id);

#if defined(SUPPORTS_MULTI_CORE)
/**
 * \brief Check that the secondary processors not hanging.
 *
 * \return TRUE if none of the secondary processors are hanging.
 *
 * \note If there is only one processor in the subsystem then
 *       this function always returns TRUE.
 */                                            
extern bool kip_aux_processor_not_hanging(void);
#else
#define kip_aux_processor_not_hanging() TRUE
#endif /* defined(SUPPORTS_MULTI_CORE) */

#endif /* KIP_MGR_H */
