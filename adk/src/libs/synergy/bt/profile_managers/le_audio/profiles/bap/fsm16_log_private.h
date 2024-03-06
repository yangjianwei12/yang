/*******************************************************************************

Copyright (C) 2008 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef _FSM16_LOG_PRIVATE_H_
#define _FSM16_LOG_PRIVATE_H_

#include "fsm.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BLUESTACK_FSM_DEBUG
void bluestack_fsm_log_entry(const FSM_SPARSE16_DEFINITION_T *p_fsm,
                             fsm16_state_t old_state, 
                             fsm_event_t event, 
                             fsm16_state_t new_state,
                             fsm16_action_t action_index );
void bluestack_fsm_log_exit(const FSM_SPARSE16_DEFINITION_T *p_fsm,
                            fsm_event_t event);
#define FSM_LOG_ENTRY(a,b,c,d,e) \
    (bluestack_fsm_log_entry((a), (b), (c), (d), (e)))
#define FSM_LOG_EXIT(a,b) (bluestack_fsm_log_exit((a), (b)))
#else
#define FSM_LOG_ENTRY(a,b,c,d,e)
#define FSM_LOG_EXIT(a,b)
#endif

#ifdef __cplusplus
}
#endif

#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#endif

