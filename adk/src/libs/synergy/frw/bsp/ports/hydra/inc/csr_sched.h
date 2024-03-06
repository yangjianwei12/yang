#ifndef CSR_SCHED_H__
#define CSR_SCHED_H__
/******************************************************************************
 Copyright (c) 2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "platform/csr_hydra_sched.h"

#ifdef __cplusplus
extern "C" {
#endif

/* An identifier issued by the scheduler. */
typedef CsrUint8 CsrSchedIdentifier;

#include "csr_gsched.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      SynergySchedMessagesPendingForTask
 *
 *  DESCRIPTION
 *      Get the number of messages pending on a given task.
 *
 *  RETURNS
 *      Returns number of messages which are pending in the queue for a given
 *      task.
 *
 *----------------------------------------------------------------------------*/
CsrUint16 SynergySchedMessagesPendingForTask(CsrSchedQid dst, CsrInt32 *first_due);

/*----------------------------------------------------------------------------*
 *  NAME
 *      SynergySchedMessagePendingMatch
 *
 *  DESCRIPTION
 *      Gets the number of messages pending on a given task based on
 *      the provided callback function to determine whether the pending message
 *      matches the criteria or not.
 *
 *  RETURNS
 *      Returns number of messages which are pending in the queue based
 *      on the matchFunc callback, for a given task.
 *
 *----------------------------------------------------------------------------*/
CsrUint16 SynergySchedMessagePendingMatch(CsrSchedQid dst, CsrBool once, void *matchFunc);

#ifdef __cplusplus
}
#endif

#endif /* CSR_SCHED_H__ */
