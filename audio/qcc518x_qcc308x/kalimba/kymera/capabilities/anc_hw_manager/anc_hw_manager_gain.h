/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  anc_hw_manager_gain.h
 * \ingroup anc_hw_manager
 *
 * ANC HW Manager (AHM) gain interface definitions.
 *
 */

#ifndef _ANC_HW_MANAGER_GAIN_H_
#define _ANC_HW_MANAGER_GAIN_H_

/******************************************************************************
Include Files
*/
#ifdef RUNNING_ON_KALSIM
#include "capabilities.h"
#else
#include "types.h"
#endif

#include "anc_hw_manager_common.h"

/* ANC HAL features */
#include "stream/stream_for_anc.h"

/****************************************************************************
Public Variable Definitions
*/

/* Invalid gains are used to initialize the previous gain structure. These are
 * deliberately outside of the bounds of valid gains so that any real gain will
 * be correctly written and stored on the next gain update.
 */
#define AHM_INVALID_FINE_GAIN   0xFFFF
#define AHM_INVALID_COARSE_GAIN 0x1000
#define AHM_COARSE_GAIN_MASK    0xF

#define AHM_DELTA_NOMINAL                0x01000000 /* 1.0 in Q8.24 */
#define AHM_DELTA_MAX                    0x7f000000 /* 127.0 in Q8.24 */
/* Value just greater than 1 allows gain ramps to 1 to succeed */
#define AHM_NOMINAL_TARGET               0x01001000
#define AHM_DELTA_GAIN_SHIFT             8
#define AHM_DELTA_PRECISION              24 - AHM_DELTA_GAIN_SHIFT
#define AHM_DELTA_ROUNDING               (1 << (AHM_DELTA_PRECISION - 1))

/* Represent ANC gain based on coarse and fine values */
typedef struct _AHM_GAIN
{
    int16 coarse;           /* Coarse gain */
    uint16 fine;            /* Fine gain */
} AHM_GAIN;

/* Represent all the gains that are controlled */
typedef struct _AHM_GAIN_BANK
{
    AHM_GAIN ff;            /* FF path gain */
    AHM_GAIN fb;            /* FB path gain */
    AHM_GAIN ec;            /* EC path gain */
    AHM_GAIN rx_ffa_mix;    /* Rx FFa mix path gain */
    AHM_GAIN rx_ffb_mix;    /* Rx FFb mix path gain */
} AHM_GAIN_BANK;

/* Shared type for describing coarse gains */
typedef struct _AHM_SHARED_COARSE_GAIN
{
    bool valid:8;               /* Validity of the associated gain */
    int8 gain;                  /* Coarse gain value */
    AHM_ANC_INSTANCE instance;  /* ANC instance to apply the gain to */
} AHM_SHARED_COARSE_GAIN;

/* Shared type for describing fine gains */
typedef struct _AHM_SHARED_FINE_GAIN
{
    /* Owned by the capability client */

    /* Delta gain fields */
    int gain_delta;                  /* Gain delta, Q8.24 */
    int tc_attack;                   /* Delta attack time constant, Q1.31 */
    int tc_release;                  /* Delta release time constant, Q1.31 */

    /* Absolute/nominal gain fields */
    uint8 gain;                      /* Fine gain value */

    /* Absolute gain fields */
    bool valid:8;                    /* Validity of the associated gain */

    /* Owned by AHM */
    AHM_GAIN_CONTROL_TYPE gain_type; /* Indicates gain type */
    AHM_ANC_INSTANCE instance;       /* ANC instance to apply the gain to */

    /* Delta gain field */
    int gain_current;                /* Current gain, Q8.24 */
    /* Nominal gain field */
    bool using_nominal:8;            /* For nominal indicate whether active */
} AHM_SHARED_FINE_GAIN;

/****************************************************************************
Public Function Definitions
*/

#ifdef RUNNING_ON_KALSIM
/**
 * \brief  Send an unsolicited message with the updated gain values.
 *
 * \param  p_config         Pointer to ANC path and channel configuration
 * \param  p_gains          Pointer to ANC gains
 * \param  p_prev_gains     Pointer to previous ANC gains
 * \param  op_data          Pointer to operator data
 *
 * \return - TRUE if successful
 *
 */
extern bool ahm_write_gain(AHM_ANC_CONFIG *p_config,
                           AHM_GAIN_BANK *p_gains,
                           AHM_GAIN_BANK *p_prev_gains,
                           OPERATOR_DATA *op_data);
#else
/**
 * \brief  Write changed gain values to the ANC HW.
 *
 * \param  p_config         Pointer to ANC path and channel configuration
 * \param  p_gains          Pointer to ANC gains
 * \param  p_prev_gains     Pointer to previous ANC gains
 *
 * \return - TRUE if successful
 *
 */
extern bool ahm_write_gain(AHM_ANC_CONFIG *p_config,
                           AHM_GAIN_BANK *p_gains,
                           AHM_GAIN_BANK *p_prev_gains);
#endif

/**
 * \brief  Initialize the previous gain bank to invalid values to ensure any
 *         subsequent gain update is written correctly.
 *
 * \param  p_prev_gains     Pointer to previous ANC gains
 *
 * \return - TRUE if successful
 *
 */
extern void ahm_initialize_prev_gain(AHM_GAIN_BANK *p_prev_gains);

/**
 * \brief  ASM function to calculate dB representation of gain
 *
 * \param  fine_gain    Fine gain value
 * \param  coarse_gain  Coarse gain value
 *
 * \return int value of calculated gain in dB in Q12.20
 */
extern int ahm_calc_gain_db(uint16 fine_gain, int16 coarse_gain);

/**
 * \brief  Function to calculate dB difference of given HW gains
 *
 * \param  fine_gain1    Fine gain value   (gain1)
 * \param  coarse_gain1  Coarse gain value (gain1)
 * \param  fine_gain2    Fine gain value   (gain2)
 * \param  coarse_gain2  Coarse gain value (gain2)
 *
 * \return int value of gain difference (gain1 - gain2) calculated in dB (Q12.20)
 */
extern int ahm_get_gain_difference_db(uint16 fine_gain1,
                                      int16 coarse_gain1,
                                      uint16 fine_gain2,
                                      int16 coarse_gain2);

/**
 * \brief  ASM function to convert gain from log10 to log2 dB representation.
 *
 * \param  gain_db    Fine gain value in dB
 *
 * \return int Fine gain value in log2
 */
extern int ahm_convert_db_to_log2(int gain_db);

#endif /* _ANC_HW_MANAGER_GAIN_H_ */
