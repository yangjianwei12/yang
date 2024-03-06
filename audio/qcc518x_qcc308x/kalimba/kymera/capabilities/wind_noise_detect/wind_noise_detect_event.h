/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  wind_noise_detect_event.h
 * \ingroup wind_noise_detect
 *
 * Wind noise detection (WND) operator event definitions
 *
 */

#ifndef _WIND_NOISE_DETECT_EVENT_H_
#define _WIND_NOISE_DETECT_EVENT_H_

/******************************************************************************
Include Files
*/
#include "types.h"
#include "audio_log/audio_log.h"

#include "anc_hw_manager_event.h"
#include "wind_noise_detect_defs.h"

/******************************************************************************
Public Definitions
*/

/* Event type encoding */
#define WND_EVENT_TYPE_ATTACK   AHM_EVENT_TYPE_TRIGGER
#define WND_EVENT_TYPE_RELEASE  AHM_EVENT_TYPE_CLEAR

/******************************************************************************
Public Type Declarations
*/

/* Represent the state of a WND event */
typedef enum
{
    WND_EVENT_RELEASE,        /* Release message sent OR initial condition */
    WND_EVENT_ATTACK_COUNT,   /* Windy condition detected */
    WND_EVENT_ATTACK_MESSAGE, /* Attack counter expired */
    WND_EVENT_ATTACK,         /* Attack message sent OR initial condition */
    WND_EVENT_RELEASE_COUNT,  /* Windy condition cleared */
    WND_EVENT_RELEASE_MESSAGE /* Release counter expired */
} WND_EVENT_STATE;

/* Represent WND event */
typedef struct _WND_EVENT
{
    int counter;
    int attack_reset_count;
    int release_reset_count;
    unsigned *p_detect;
    int *p_pwr_level;
    unsigned *p_intensity;
    unsigned *p_mode;
    bool confirm;
    WND_EVENT_STATE state;
    AHM_EVENT_MSG msg;
} WND_EVENT;

/******************************************************************************
Public Function Definitions
 */

/**
 * \brief  Setup the wind detection payload
 *
 * \param  p_event          Pointer to the wind event to process
 * \return - TRUE if successful
 */
extern void wnd_setup_event_payload(WND_EVENT *p_event);

/**
 * \brief  Process a wind detection event
 *
 * \param  p_event          Pointer to the wind event to process
 * \param  ext_op_id        Opertor ID
 * \return - TRUE if there is a message to send
 */
extern bool wnd_process_event(WND_EVENT *p_event, EXT_OP_ID ext_op_id);

/**
 * \brief  Setup the wind detection event structure
 *
 * \param  p_event          Pointer to the wind event to configure
 * \param  attack_duration  Attack timer duration (s, Q12.N)
 * \param  release_duration Release timer duration (s, Q12.N)
 *
 * \return - NONE
 */
extern void wnd_setup_event(WND_EVENT *p_event,
                            unsigned attack_duration,
                            unsigned release_duration);

#endif /* _WIND_NOISE_DETECT_EVENT_H_ */
