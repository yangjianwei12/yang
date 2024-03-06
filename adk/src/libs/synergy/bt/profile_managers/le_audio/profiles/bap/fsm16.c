/*******************************************************************************

Copyright (C) 2008 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

*******************************************************************************/

#include "fsm_private.h"
#include "fsm16_log_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
/*
 * The macro READ_ACTION_TABLE loads in data from the specified offset in
 * the specified table (high or low) into the destination word. It
 * encapsulates the different ways of accessing the tables when they are
 * in code space versus data space.
 */
#ifdef COMPRESS_FSM_ACTION_FUNCTIONS
#ifndef REALLY_ON_HOST
#define READ_ACTION_TABLE(dest, table, offset) \
    ((dest) = p_fsm->p_actions_ ## table[offset])
#endif /* ! REALLY_ON_HOST */
#endif /* COMPRESS_FSM_ACTION_FUNCTIONS */



/*! \brief fsm_16bit_run_synergy

    The engine function for a 16 bit bitfiled sparse state machine.
    Adheres to the "shake it until it stops rattling" principle,
    driving the state machine until there are no more events.

    \returns fsm_result_t - FSM_RESULT_OK if the event was processed
    FSM_RESULT_FAIL otherwise
*/
fsm_result_t fsm_16bit_run_synergy(
    const FSM_SPARSE16_DEFINITION_T *p_fsm, /* The FSM definition */
    fsm16_state_t *p_state,                 /* State variable */
    fsm_event_t event,                      /* Event to process */
    void *pv_arg1,                          /* Two void pointer arguments */
    void *pv_arg2
    )
{
    const FSM_SPARSE16_TRANSITION_T *p_transition, *p_end_trans;
    fsm16_action_t action_index;

#ifdef FSM_POINTER_CHECK
    /* Make sure p_state is in RAM as we need to write to it */
    if ((uintptr) p_state < (uintptr) RAM_START)
	panic(PANIC_FSM_BAD_POINTER);
#endif

    while (event != FSM_EVENT_NULL)
    {
        /* Point to the first transition defined for the current state */
        /* Search the current state's transition list for the event,
         * terminating the search at FSM_EVENT_NULL
         */
	for(p_transition = p_fsm->p_table[*p_state].p_transitions,
	    p_end_trans  = p_fsm->p_table[*p_state+1].p_transitions; ;
	    p_transition++)
	{
	    /* Have we reached the end of the list */
	    if (p_transition == p_end_trans)
	    {
		/* Unhandled event */
                FSM_LOG_ENTRY(p_fsm, *p_state, event, *p_state,
                        FSM16_ACTION_UNDEF);
                FSM_LOG_EXIT(p_fsm, event);
		return FSM_RESULT_FAIL;
	    }
	    if (p_transition->event == event)
		break;
	}

        /* Cache the action index, we need it a couple of times. */
        action_index = p_transition->action_index;
		

        /* Log before we perform the transition */
        FSM_LOG_ENTRY(p_fsm, *p_state, event, p_transition->next_state,
                      action_index);
        
        /* State transition */
	*p_state = p_transition->next_state;

        /* Call the action function if one is specified */
        if (action_index != FSM16_ACTION_NULL)
        {
			fsm_action_fn_t p_action;
			
#if defined (COMPRESS_FSM_ACTION_FUNCTIONS) && !defined (REALLY_ON_HOST)
            uint16 word;
            union {
                uint16_t words[2];
                fsm_action_fn_t fn;
            } u;

            COMPILE_TIME_ASSERT(sizeof(uint16_t) == 1, uint16_t_wrong_size);
            COMPILE_TIME_ASSERT(sizeof(u.words) == sizeof(u.fn),
                fsm_action_fn_t_wrong_size);

            /*
             * Norcroft generates a false positive here. The division can't
             * possibly produce a different result if we cast to uint32 any
             * earlier because the result of a division is always smaller than
             * the original value so can't be subject to truncation.
             */
            /* xapncc ignore lower precision in wider context: '/' */
            READ_ACTION_TABLE(word, high, action_index/2);
            if ((action_index % 2) == 0)
                word = word >> 8;
            word = word & 0xFF;
            u.words[0] = word;
            READ_ACTION_TABLE(u.words[1], low, action_index);
            p_action = u.fn;
#else
            p_action = p_fsm->p_actions[action_index];
#endif
			event = p_action(pv_arg1, pv_arg2);
        }
        else
        {
            event = FSM_EVENT_NULL;
        }

        FSM_LOG_EXIT(p_fsm, event);
    }

    return FSM_RESULT_OK;
}
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
