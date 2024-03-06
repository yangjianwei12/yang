/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  aanc2_events.h
 * \ingroup aanc2
 *
 * AANC Controller (AANC2) events public header file.
 *
 */

#ifndef _AANC2_EVENTS_H_
#define _AANC2_EVENTS_H_

/******************************************************************************
Include Files
*/

#include "capabilities.h"
#include "aanc2_defs.h"
#include "opmgr/opmgr_operator_data.h"

/******************************************************************************
Public Constant Definitions
*/

/* Event IDs */
#define AANC2_EVENT_ID_GAIN         0
#define AANC2_EVENT_ID_ED           1
#define AANC2_EVENT_ID_QUIET        2
#define AANC2_EVENT_ID_CLIP         3
#define AANC2_EVENT_ID_SAT          4
#define AANC2_EVENT_ID_SELF_TALK    5
#define AANC2_EVENT_ID_SPL          6
#define AANC2_EVENT_ID_GENTLE_MUTE  7

/******************************************************************************
Public Type Declarations
*/

/* Represent an event message type */
typedef enum
{
    AANC2_EVENT_MSG_NEGATIVE_TRIGGER,
    AANC2_EVENT_MSG_POSITIVE_TRIGGER
} AANC2_EVENT_MSG_TYPE;

/* Represent an event mesasge */
typedef struct _AANC2_EVENT_MSG
{
    OPERATOR_DATA *op_data;
    AANC2_EVENT_MSG_TYPE type;
    uint16 id;
    uint16 payload;
    EXT_OP_ID opid;
} AANC2_EVENT_MSG;

/* Represent the state of an AANC event */
typedef enum
{
    AANC2_EVENT_CLEAR,
    AANC2_EVENT_DETECTED,
    AANC2_EVENT_SENT
} AANC2_EVENT_STATE;

/* Represent ANC event messaging states */
typedef struct _AANC2_EVENT
{
    int frame_counter;
    unsigned set_frames;
    AANC2_EVENT_STATE running;
    AANC2_EVENT_MSG msg;
} AANC2_EVENT;

/******************************************************************************
Public Function Definitions
*/

/**
 * \brief  Initialize events for messaging.
 *
 * \param  p_event          Pointer to the AANC2 event
 * \param  op_data          Pointer to the operator data
 * \param  timer_duration   Event tTimer duration parameter
 * \param  id               Event ID
 *
 * \return  void.
 */
extern void aanc2_initialize_event(AANC2_EVENT *p_event,
                                   OPERATOR_DATA *op_data,
                                   unsigned timer_duration,
                                   uint16 id);

/**
 * \brief  Process an event based on changes in the current state
 *
 * \param  p_event      Pointer to the event to process
 * \param  cur_state    Current state of the event
 * \param  prev_state   Previous state of the event
 *
 * \return  boolean indicating success or failure.
 */
extern void aanc2_process_event(AANC2_EVENT *p_event,
                                unsigned cur_state,
                                unsigned prev_state);

/**
 * \brief  Sent an event trigger message.
 *
 * \param  op_data      Pointer to the operator data
 * \param  p_msg        Pointer to the message to send
 *
 * \return  bool indicating success
 */
extern bool aanc2_send_event_trigger(AANC2_EVENT_MSG *p_msg);

/**
 * \brief  Process an event detection condition.
 *
 * \param  p_event      Pointer to the event to process
 *
 * \return  void.
 */
extern void aanc2_process_event_detect_condition(AANC2_EVENT *p_event);

/**
 * \brief  Process an event clear condition.
 *
 * \param  p_event      Pointer to the event to process
 *
 * \return  void.
 */
extern void aanc2_process_event_clear_condition(AANC2_EVENT *p_event);

/******************************************************************************
Inline Function Definitions
*/
/**
 * \brief  Clear an AANC2 event
 *
 * \param  p_event      Pointer to the event to clear
 *
 * \return  void.
 */
inline void aanc2_clear_event(AANC2_EVENT *p_event);

inline void aanc2_clear_event(AANC2_EVENT *p_event)
{
       p_event->frame_counter =p_event->set_frames;
       p_event->running = AANC2_EVENT_CLEAR;
}


#endif /* _AANC2_EVENTS_H_*/
