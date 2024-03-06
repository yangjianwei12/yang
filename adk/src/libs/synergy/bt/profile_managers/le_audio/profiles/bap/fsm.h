/*******************************************************************************

Copyright (C) 2008 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

\brief  This file defines the interface for two state machine
        engines. The most appropriate one should be chosen for each
        state machine based upon criteria such as code size and speed
        of execution. Both FSMs are table-driven.

        For in-depth information on how this works, see fsm_private.h.

*******************************************************************************/

#ifndef _BAP_FSM_H
#define _BAP_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FSM32_FUNCTIONS_TABLE_IN_CODE
#include "psflbc01.h"
#endif
#include "qbl_types.h"
#include "csr_synergy.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
/* On CDA2 platforms the Synergy Profiles, QBL and the Platform code are compiled
 * for a single processor. We need to rename these symbols to avoid linker conflicts.
 */
#define fsm_16bit_run  fsm_16bit_run_synergy
#define fsm_32bit_run  fsm_32bit_run_synergy
#define fsm_sparse_run fsm_sparse_run_synergy
#define fsm_full_run   fsm_full_run_synergy

#ifdef USE_BLUESTACK_BITFIELDS
#define BITFIELD1(type, name, size)  unsigned int name:size
#else
#define BITFIELD1(type, name, size)  type name
#endif

/* Keep the magic numbers in one place */
#define TRANSITION32_NEW_STATE_SHIFT 0
#define TRANSITION32_NEW_STATE_BITS 9
#define TRANSITION32_ACTION_SHIFT \
    (TRANSITION32_NEW_STATE_SHIFT+TRANSITION32_NEW_STATE_BITS)
#define TRANSITION32_ACTION_BITS 9
#define TRANSITION32_EVENT_SHIFT (TRANSITION32_ACTION_SHIFT+TRANSITION32_ACTION_BITS)
#define TRANSITION32_EVENT_BITS (32 - TRANSITION32_EVENT_SHIFT)

/* Definitions for New hand bitfielded FSM */
/* The NULL event */
#define FSM32_EVENT_NULL ((fsm_event_t) ((1<<TRANSITION32_EVENT_BITS)-1))
/* The RE-POST event */
#define FSM32_EVENT_REPOST ((fsm_event_t) ((1<<TRANSITION32_EVENT_BITS)-2))
/* The ABORT event - for testing, cancel state-change */
#define FSM32_EVENT_ABORT ((fsm_event_t) ((1<<TRANSITION32_EVENT_BITS)-3))

/* The NULL event */
#define FSM32_EVENT_UNPAUSE ((fsm_event_t) 200)

/* The NULL action */
#define FSM32_ACTION_NULL ((fsm_action_t) ((1<<TRANSITION32_ACTION_BITS)-1))
#define FSM32_ACTION_UNDEF FSM32_ACTION_NULL

#define TRANSITION16_NEW_STATE_SHIFT 0
#define TRANSITION16_NEW_STATE_BITS 4
#define TRANSITION16_ACTION_SHIFT \
    (TRANSITION16_NEW_STATE_SHIFT+TRANSITION16_NEW_STATE_BITS)
#define TRANSITION16_ACTION_BITS 6
#define TRANSITION16_EVENT_SHIFT (TRANSITION16_ACTION_SHIFT+TRANSITION16_ACTION_BITS)
#define TRANSITION16_EVENT_BITS (16 - TRANSITION16_EVENT_SHIFT)

/* Definitions for New hand bitfielded FSM */

/* The NULL action */
#define FSM16_ACTION_NULL ((fsm_action_t) ((1<<TRANSITION16_ACTION_BITS)-1))
#define FSM16_ACTION_UNDEF FSM16_ACTION_NULL

/* Definitions for New hand bitfielded FSM */
/* The NULL event */
#define FSM_EVENT_NULL ((fsm_event_t) 0xFFFF)

/* The NULL action */
#define FSM_ACTION_NULL ((fsm_action_t) 255)
#define FSM_ACTION_UNDEF ((fsm_action_t) 0xFFFF)


/* Original FSM definitions */
/* Engine return values */
#define FSM_RESULT_FAIL ((fsm_result_t) 0)  /* Event was not processed */
#define FSM_RESULT_OK ((fsm_result_t) 1)    /* Event was processed */

/* Values specifying whether the engine should produce logging output, supplied
 * as an argument to the engine functions
 */
#define FSM_NO_LOG ((fsm_logging_t) 0)
#define FSM_LOG ((fsm_logging_t) 1)

/* Define FSM_LOGGING macro, which can be used in place of the logging argument
 * to the engine functions, providing control over FSM logging via the
 * "FSM_DEBUG" build option. 
 * Using the "FSM_DEBUG_NAMES" option will enable textual state/event name
 * logging.
 */
#if defined(FSM_DEBUG)
#define FSM_LOGGING FSM_LOG
#else
#define FSM_LOGGING FSM_NO_LOG
#endif


/* Macro used to declare action functions */
#define FSM_ACTION_FN(fn) fsm_event_t fn(void *pv_arg1, void *pv_arg2)

/* Common code to extract a field from a transition */
#define TRANSITION32_FIELD(field, transition) \
    (uint16_t) (((transition) >> TRANSITION32_ ## field ## _SHIFT & \
		 ((1<< TRANSITION32_ ## field ## _BITS)-1)))
#define BUILD_TRANSITION32(event, action, new_state) \
    ((uint32_t)(event)     << TRANSITION32_EVENT_SHIFT | \
     (uint32_t)(action)    << TRANSITION32_ACTION_SHIFT | \
     (uint32_t)(new_state) << TRANSITION32_NEW_STATE_SHIFT)

/* FSM basic data types */
typedef uint16_t fsm_event_t;
typedef uint8_t  fsm_state_t;
typedef uint8_t  fsm_action_t;
typedef uint16_t fsm32_state_t;
typedef uint16_t fsm32_action_t;
typedef uint8_t fsm16_event_t;
typedef uint8_t fsm16_state_t;
typedef uint8_t fsm16_action_t;

/* Type for returning an indication of whether an event was processed by
 * the engine */
typedef uint8_t fsm_result_t;

/* Type for specifying whether logging is required */
typedef uint8_t fsm_logging_t;

/* Action function pointer type */
typedef FSM_ACTION_FN((*fsm_action_fn_t));

typedef uint16_t fsm_action_fn_low_t;
typedef uint8_t fsm_action_fn_high_t;

/* 
 * Fully-specified state machine definitions
 */

/* Full transition definition */
typedef struct
{
    fsm_action_t    action_index;   /* Index into action table */
    fsm_state_t     next_state;     /* Next state */
} FSM_FULL_TRANSITION_T;

/* Full state machine definition - a pointer to this structure is passed
 * to the engine */
typedef struct
{
    const FSM_FULL_TRANSITION_T *p_table;   /* Pointer to transition table */
    const fsm_action_fn_t *p_actions;       /* Pointer to action fn table */
    uint8_t num_events;     /* Number of events per state */

    /* Logging information - can be NULL */
    const char *const fsm_name;     /* Name of state machine */
    const char * const *const state_names;  /* Array of state names */
    const char * const *const event_names;  /* Array of event names */
} FSM_FULL_DEFINITION_T;

/*
 * Sparse state machine definitions
 */

/* Sparse transition definition - an array of these is defined per state, and
 * must be terminated with FSM_EVENT_NULL.
 */
typedef struct
{
    fsm_event_t     event;              /* Event */
    BITFIELD1(uint8_t, action_index, 8); /* Index into action table */
    BITFIELD1(uint8_t, next_state, 8);   /* Next state */
} FSM_SPARSE_TRANSITION_T;

typedef struct
{
    BITFIELD1(fsm16_event_t, event, TRANSITION16_EVENT_BITS);
    BITFIELD1(fsm16_action_t, action_index, TRANSITION16_ACTION_BITS);
    BITFIELD1(fsm16_state_t, next_state, TRANSITION16_NEW_STATE_BITS);
} FSM_SPARSE16_TRANSITION_T;

/* Sparse state definition - this is built into an array of pointers to the
 * transition list for each state.
 */
typedef struct
{
    const FSM_SPARSE_TRANSITION_T *p_transitions;
} FSM_SPARSE_STATES_T;

#if defined(STATEWATCH)
#define FSM_NAME_LOGGING
#endif

#ifdef FSM_NAME_LOGGING
#define FSM_LOG_INFO(fn, sn, en) , fn, sn, en
#else
#define FSM_LOG_INFO(fn, sn, en)
#endif

/* Sparse transition definition - an array of these is defined per state, and
 * must be terminated with FSM_EVENT_NULL.
 */
typedef struct
{
    const FSM_SPARSE_STATES_T *p_table; /* Pointer to state table */
    const fsm_action_fn_t *p_actions;       /* Pointer to action fn table */

#ifdef FSM_NAME_LOGGING
    /* Logging information - can be NULL */
    const char *const fsm_name;     /* Name of state machine */
    const char * const *const state_names;  /* Array of state names */
    const char * const *const event_names;  /* Array of event names */
#endif
} FSM_SPARSE_DEFINITION_T;

/* Sparse transition definition - an array of these is defined per state, and
 * must be terminated with FSM_EVENT_NULL.
 * To save constant space this is now 3 fields inside a uint32_t, we can
 * not use bitfields anymore as they straddle a 16 bit boundary and the
 * complier puts it in a new 16 bit word.
 * So We now have a 32 bit word defined as
 *
 *  31:18 event (14 bits)
 *  17:9  action (9 bits)
 *  8:0   state (9 bits)
 */
typedef struct
{
    uint32_t        trans;
} FSM_SPARSE32_TRANSITION_T;

/* Sparse state definition - this is built into an array of pointers to the
 * transition list for each state.
 */
typedef struct
{
    const FSM_SPARSE32_TRANSITION_T *p_transitions;
} FSM_SPARSE32_STATES_T;

typedef void (*fsm_action_table_ptr)(void);

/* Sparse state machine definition - a pointer to this structure is passed to
 * the engine.
 */
typedef struct
{
    const FSM_SPARSE32_STATES_T *p_table; /* Pointer to state table */
#if defined (COMPRESS_FSM_ACTION_FUNCTIONS) && !defined (REALLY_ON_HOST)
#ifdef FSM32_FUNCTIONS_TABLE_IN_CODE
    /*
     * Addresses in the memory map used by psflbc01_read of the tables
     * holding the high and low parts of the action function addresses.
     */
    fsm_action_table_ptr p_actions_low_addr;
    fsm_action_table_ptr p_actions_high_addr;
#define FSM_SPARSE32_ACTION_TABLE_INIT(table) \
    (fsm_action_table_ptr) (PSFLBC01_PROG_START + (uint32) table ## _addr)
#else
    /*
     * Pointers to the tables holding the high and low parts of the action
     * function addresses.
     */
    const fsm_action_fn_low_t *p_actions_low;
    const fsm_action_fn_high_t *p_actions_high;
#define FSM_SPARSE32_ACTION_TABLE_INIT(table) table
#endif
#else
    const fsm_action_fn_t *p_actions;       /* Pointer to action fn table */
#endif

#ifdef FSM_NAME_LOGGING
    /* Logging information - can be NULL */
    const char *const fsm_name;     /* Name of state machine */
    const char * const *const state_names;  /* Array of state names */
    const char * const *const event_names;  /* Array of event names */
#endif
} FSM_SPARSE32_DEFINITION_T;


/* Sparse state definition - this is built into an array of pointers to the
 * transition list for each state.
 */
typedef struct
{
    const FSM_SPARSE16_TRANSITION_T *p_transitions;
} FSM_SPARSE16_STATES_T;


typedef struct
{
    const FSM_SPARSE16_STATES_T *p_table; /* Pointer to state table */

#if defined (COMPRESS_FSM_ACTION_FUNCTIONS) && !defined (REALLY_ON_HOST)
    /*
     * Pointers to the tables holding the high and low parts of the action
     * function addresses.
     */
    const fsm_action_fn_low_t *p_actions_low;
    const fsm_action_fn_high_t *p_actions_high;
#define FSM_SPARSE16_ACTION_TABLE_INIT(table) table
#else
    const fsm_action_fn_t *p_actions;       /* Pointer to action fn table */
#endif

#ifdef FSM_NAME_LOGGING
    /* Logging information - can be NULL */
    const char *const fsm_name;     /* Name of state machine */
    const char * const *const state_names;  /* Array of state names */
    const char * const *const event_names;  /* Array of event names */
#endif
} FSM_SPARSE16_DEFINITION_T;




typedef struct
{
    fsm_event_t event;          /* Event */
    uint16_t    action_index;   /* Index into action table */
    uint16_t    next_state;     /* Next state */
} FSM_TRANSITION_T;



/*! \brief fsm_full_run

    The engine function for a fully-specified state machine. Adheres to
    the "shake it until it stops rattling" principle, driving the state
    machine until there are no more events.

    \returns fsm_result_t - FSM_RESULT_OK if the event was processed
    FSM_RESULT_FAIL otherwise
*/
fsm_result_t fsm_full_run(
    const FSM_FULL_DEFINITION_T *p_fsm,     /* The FSM definition */
    fsm_state_t *p_state,                   /* State variable */
    fsm_event_t event,                      /* Event to process */
    void *pv_arg1,                          /* Two void pointer arguments */
    void *pv_arg2,
    fsm_logging_t log                       /* Logging control */
    );

/*! \brief fsm_sparse_run

    The engine function for a sparse state machine. Adheres to
    the "shake it until it stops rattling" principle, driving the state
    machine until there are no more events.

    \returns fsm_result_t - FSM_RESULT_OK if the event was processed
    FSM_RESULT_FAIL otherwise
*/
fsm_result_t fsm_sparse_run(
    const FSM_SPARSE_DEFINITION_T *p_fsm,   /* The FSM definition */
    fsm_state_t *p_state,                   /* State variable */
    fsm_event_t event,                      /* Event to process */
    void *pv_arg1,                          /* Two void pointer arguments */
    void *pv_arg2
    );

/*! \brief fsm_32bit_run

    The engine function for a 32 bit bitfiled sparse state machine. 
    Adheres to the "shake it until it stops rattling" principle, 
    driving the state machine until there are no more events.

    \returns fsm_result_t - FSM_RESULT_OK if the event was processed
    FSM_RESULT_FAIL otherwise
*/
extern fsm_result_t fsm_32bit_run(
    const FSM_SPARSE32_DEFINITION_T *p_fsm,   /* The FSM definition */
    fsm32_state_t *p_state,                   /* State variable */
    fsm_event_t event,                      /* Event to process */
    void *pv_arg1,                          /* Two void pointer arguments */
    void *pv_arg2,
    fsm_logging_t log                       /* Logging control */
    );

/*! \brief fsm_16bit_run_synergy

    The engine function for a 16 bit bitfiled sparse state machine. 
    Adheres to the "shake it until it stops rattling" principle, 
    driving the state machine until there are no more events.
    
    \returns fsm_result_t - FSM_RESULT_OK if the event was processed
    FSM_RESULT_FAIL otherwise
*/
extern fsm_result_t fsm_16bit_run_synergy(
    const FSM_SPARSE16_DEFINITION_T *p_fsm,   /* The FSM definition */
    fsm16_state_t *p_state,                   /* State variable */
    fsm_event_t event,                      /* Event to process */
    void *pv_arg1,                          /* Two void pointer arguments */
    void *pv_arg2
    );

#ifdef RFCOMM_TEST
extern void rfc_log_transition(const char *cur_state,
                               const char *event, 
                               const char *next_state, 
                               void *pv_arg1);
#endif 

#ifdef ON_CHIP_DEBUG
typedef struct {
    /* Used when the LM is being used as the Test engine */
    bool_t  stop_lm_state_machine;
    /* Items below are used to specify a trigger for when the FSMs will stop.
     * Basically, if *trigger_fsm == trigger_state, we halt all FSMs.
     */
    fsm32_state_t   *trigger_fsm;
    fsm32_state_t   trigger_state;
} FSM_STATE_DEBUG_T;

extern FSM_STATE_DEBUG_T fsm_state_debug;
#endif
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */

#ifdef __cplusplus
}
#endif 
#endif

