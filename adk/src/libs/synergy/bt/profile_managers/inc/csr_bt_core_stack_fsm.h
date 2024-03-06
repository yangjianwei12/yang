#ifndef CSR_BT_CORE_STACK_FSM_H__
#define CSR_BT_CORE_STACK_FSM_H__
/******************************************************************************
 Copyright (c) 2008-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_synergy.h"


/* The purpose of this file is to wrap
 * "csr_bt_core_stack_fsm_private.h" which requires that a few
 * compiler defines are setup correctly to work properly with the
 * Synergy logging mechanism. You should ALWAYS include this file and
 * NOT the csr_bt_core_stack_fsm_private.h one!!! */

#ifdef CSR_LOG_ENABLE
#include "csr_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BLUESTACK_FSM_DEBUG
#define BLUESTACK_FSM_DEBUG 1
#endif

#if (CSR_HOST_PLATFORM != QCC5100_HOST)
#ifndef FSM_NAME_LOGGING
#define FSM_NAME_LOGGING 1
#endif

#ifndef FSM_DEBUG_NAMES
#define FSM_DEBUG_NAMES 1
#endif
#endif /* (CSR_HOST_PLATFORM != QCC5100_HOST) */

#ifdef __cplusplus
}
#endif

#endif

#include "csr_bt_core_stack_fsm_private.h"

#endif /* CSR_BT_CORE_STACK_FSM_H__ */
