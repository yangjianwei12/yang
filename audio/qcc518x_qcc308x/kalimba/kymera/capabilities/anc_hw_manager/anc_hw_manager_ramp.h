/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  anc_hw_manager_ramp.h
 * \ingroup anc_hw_manager
 *
 * ANC HW Manager (AHM) ramp definitions.
 *
 */

#ifndef _ANC_HW_MANAGER_RAMP_H_
#define _ANC_HW_MANAGER_RAMP_H_

/******************************************************************************
Include Files
*/
#include "types.h"
#include "pl_intrinsics.h"

//#define DEBUG_AHM_RAMP

#ifdef DEBUG_AHM_RAMP
#include "pl_timers/pl_timers.h"
#endif

#include "anc_hw_manager_common.h"
#include "anc_hw_manager_event.h"
#include "anc_hw_manager_gain.h"

/******************************************************************************
Public Definitions
*/
/* Precision for the ramp values */
#define AHM_RAMP_PRECISION  23
#define AHM_RAMP_ROUNDING   (1 << (AHM_RAMP_PRECISION - 1))
#define AHM_DIV_PRECISION   24
#define AHM_DELAY_PRECISION 20
#define AHM_DELAY_ROUNDING  (1 << (AHM_DELAY_PRECISION - 1))
#define AHM_RAMP_END_THRESH 0x00010000
/******************************************************************************
Public Type Declarations
*/


/* Represent the state of an FF or FB gain ramp */
typedef enum
{
    AHM_RAMP_INITIALIZED,
    AHM_RAMP_WAITING,
    AHM_RAMP_RUNNING,
    AHM_RAMP_FINISHED,
    AHM_RAMP_IDLE
} AHM_RAMP_STATE;

/* Represent an FF or FB gain ramp.
 *
 * A ramp can last for up to 10s. Supposing a maximum sample rate of 96kHz
 * then the counter needs to be able to count to 960,000 or 20 bits.
 *
 * Ramping 0-1 in 10s at 96kHz means a resolution of 1/960,000 gain values
 * per step, or 20 bits.
 *
 */
typedef struct _AHM_RAMP
{
    unsigned duration;              /* Duration of the ramp (samples) */
    int rate;                       /* Ramp rate (gain steps/sample) Q8.24 */
    int value;                      /* Gain value Q8.24 */
    int counter;                    /* sample counter value */
    AHM_RAMP_STATE state;           /* Ramp state */
    AHM_SHARED_FINE_GAIN *p_gain;   /* Pointer to fine gain value to control */
    uint16 target;                  /* Target value (fine gain) */
    uint16 *p_static;               /* Pointer to fine gain static value */
    uint16 *p_cur;                  /* Pointer to current fine gain value */
    uint16 path;                    /* Path that the ramp is on */
} AHM_RAMP;

/* Represent a fine gain ramp */
typedef struct _AHM_RAMP_CONFIG
{
    uint16 target;                  /* Target gain */
    uint16 nominal_gain;            /* Nominal gain */
    unsigned delay;                 /* Ramp delay (s, Q12.20) */
    unsigned duration;              /* Ramp duration (s, Q12.20) */
    unsigned slow_rate;             /* Ramp monitor rate */
    unsigned fast_rate;             /* Ramp update rate */

} AHM_RAMP_CONFIG;

/* Represent a fine gain delta ramp
 *
 * Current gain value of Q8.24 allows 8 bits of gain and 24 bits of precision.
 *
 */
typedef struct _AHM_DELTA_RAMP
{
    int delay;                      /* Ramp delay (samples) */
    int counter;                    /* Delay counter */
    int tc;                         /* Ramp time constant */
    AHM_RAMP_STATE state;           /* Ramp state */
    AHM_SHARED_FINE_GAIN *p_gain;   /* Pointer to fine gain value to control */
    int target;                     /* Target value (delta, Q8.24) */
    uint16 *p_cur;                  /* Pointer to current fine gain value */
    uint16 path;                    /* Path that the ramp is on */
    uint16 prev_nominal_gain;       /* Prev nominal gain */
} AHM_DELTA_RAMP;

/******************************************************************************
Public Function Definitions
*/

/**
 * \brief  Initialize a ramp on FF or FB fine gains
 *
 * \param p_ramp  Pointer to AHM_RAMP struct
 * \param p_cfg  Pointer to ramp configuration data
 * \param p_msg  Pointer to message data
 * \param sample_rate  Sample rate
 *
 * \return  TRUE if a message needs to be sent otherwise FALSE
 */
extern bool ahm_initialize_ramp(AHM_RAMP *p_ramp,
                                AHM_RAMP_CONFIG *p_cfg,
                                AHM_EVENT_MSG *p_msg,
                                unsigned sample_rate);

/**
 * \brief  Initialize a ramp on FF or FB fine gains using a delta gain method
 *
 * \param p_ramp  Pointer to AHM_RAMP struct
 * \param p_cfg  Pointer to ramp configuration data
 * \param p_msg  Pointer to message data
 *
 * \return  TRUE if a message needs to be sent otherwise FALSE
 */
extern bool ahm_initialize_delta_ramp(AHM_DELTA_RAMP *p_ramp,
                                      AHM_RAMP_CONFIG *p_cfg,
                                      AHM_EVENT_MSG *p_msg);

/**
 * \brief  FF/FB fine gain ramp state machine
 * \param p_ramp  Pointer to AHM_RAMP struct
 * \param p_msg  Pointer to message data
 * \param samples  Number of samples to process
 *
 *  The INITIALIZED state is reserved for future use.
 *  If there is a delay the frame counter will initially count down to the
 *  ramp duration during AANC_RAMP_WAITING.
 *  During AHM_RAMP_RUNNING the ramp is implemented and the gain updated
 *  When the ramp is finished the state is moved on to AHM_RAMP_FINISHED
 *
 * \return  TRUE if a message needs to be sent otherwise FALSE
 */
extern bool ahm_process_ramp(AHM_RAMP *p_ramp,
                             AHM_EVENT_MSG *p_msg,
                             unsigned samples);

/**
 * \brief  FF/FB fine delta gain ramp state machine
 *
 * \param p_ramp  Pointer to AHM_RAMP struct
 * \param p_msg  Pointer to message data
 *
 *  The INITIALIZED state is reserved for future use.
 *  If there is a delay the frame counter will initially count down to the
 *  ramp duration during AANC_RAMP_WAITING.
 *  During AHM_RAMP_RUNNING the ramp is implemented and the gain updated
 *  When the ramp is finished the state is moved on to AHM_RAMP_FINISHED
 *
 * \return  TRUE if a message needs to be sent otherwise FALSE
 */
extern bool ahm_process_delta_ramp(AHM_DELTA_RAMP *p_ramp,
                                   AHM_EVENT_MSG *p_msg, uint16 nominal_gain);

/**
 * \brief  Calculate the duration of the ramp in samples
 *
 * \param  duration_sec  Duration of the ramp in seconds, Q12.20
 * \param  sample_rate  Sample rate
 *
 * \return  Duration of the ramp in samples
 *
 */
extern int ahm_calc_duration_samples(unsigned duration_sec,
                                     unsigned sample_rate);

/**
 * \brief  Calculate a time constant for the ramp
 *
 * \param  duration_sec  Duration of the ramp in seconds, Q12.20
 * \param  fast_rate  Calculation rate of the ramp
 *
 * \return  Duration of the ramp in samples
 *
 */
extern int ahm_calc_ramp_tc(unsigned duration_sec,
                            int fast_rate);

#endif /* _ANC_HW_MANAGER_RAMP_H_ */
