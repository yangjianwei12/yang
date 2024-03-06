/****************************************************************************
 * Copyright (c) 2022 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  atr_vad_event.h
 * \ingroup atr_vad
 *
 * Auto Transparency VAD (ATR_VAD) operator event definitions
 *
 */

#ifndef _ATR_VAD_EVENT_H_
#define _ATR_VAD_EVENT_H_

/******************************************************************************
Include Files
*/
#include "types.h"
#include "audio_log/audio_log.h"

#include "anc_hw_manager_event.h"
#include "atr_vad_defs.h"

/******************************************************************************
Public Definitions
*/

/* Event type encoding */
#define ATR_VAD_EVENT_TYPE_ATTACK   AHM_EVENT_TYPE_TRIGGER
#define ATR_VAD_EVENT_TYPE_RELEASE  AHM_EVENT_TYPE_CLEAR

/******************************************************************************
Public Type Declarations
*/

/* Represent the state of an ATR VAD event */
typedef enum
{
    ATR_VAD_EVENT_RELEASE,          /* Release message sent/initial condition */
    ATR_VAD_EVENT_ATTACK_COUNT,     /* Condition detected */
    ATR_VAD_EVENT_ATTACK_MESSAGE,   /* Attack counter expired */
    ATR_VAD_EVENT_ATTACK,           /* Attack message sent/initial condition */
    ATR_VAD_EVENT_RELEASE_COUNT,    /* Condition cleared */
    ATR_VAD_EVENT_RELEASE_MESSAGE   /* Release counter expired */
} ATR_VAD_EVENT_STATE;

/* Release configuration matches application API */
typedef enum
{
    ATR_VAD_RELEASE_NONE,               /* 0 */
    ATR_VAD_RELEASE_SHORT,              /* 1 */
    ATR_VAD_RELEASE_NORMAL,             /* 2 */
    ATR_VAD_RELEASE_LONG                /* 3 */
} ATR_VAD_RELEASE;

/* Attack and release time parameters for ATR event messaging */
typedef struct _ATR_VAD_EVENT_CONFIG
{
    int attack_time;
    int short_release_time;
    int normal_release_time;
    int long_release_time;
} ATR_VAD_EVENT_CONFIG;

/* Represent ATR VAD event */
typedef struct _ATR_VAD_EVENT
{
    ATR_VAD_EVENT_CONFIG config;        /* Event timer configuration */
    int counter;                        /* Event counter */
    int attack_reset_count;             /* Reset value for attack counting */
    int release_reset_count;            /* Reset value for release counting */
    ATR_VAD_EVENT_STATE state;          /* Event state */
    unsigned *p_detect;                 /* Pointer to detection flag */
    unsigned *p_mode;                   /* Used to setup the message payload */
    AHM_EVENT_MSG msg;                  /* Messaging payload */
    bool confirm:8;                     /* Whether to confirm event or not */
    bool notify_release:8;              /* Whether to send release message */
} ATR_VAD_EVENT;


/******************************************************************************
Public Function Definitions
*/

/**
 * \brief  Process a detection event
 *
 * \param  p_event          Pointer to the event to process
 *
 * \return - TRUE if there is a message to send
 */
extern bool atr_vad_process_event(ATR_VAD_EVENT *p_event);

/**
 * \brief  Setup the detection event structure
 *
 * \param  p_event          Pointer to the event to configure
 * \param  release_select   Release time selection
 *
 * \return - NONE
 */
extern void atr_vad_setup_event(ATR_VAD_EVENT *p_event,
                                ATR_VAD_RELEASE release_select);

#endif /* _ATR_VAD_H_ */