/****************************************************************************
 * Copyright (c) 2019 - 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  aanc2_proc.h
 * \ingroup aanc2
 *
 * AANC2 algorithm processing library header file.
 *
 */

#ifndef _AANC2_PROC_H_
#define _AANC2_PROC_H_


#include "types.h"                      /* Kalimba type definitions */
#include "platform/pl_fractional.h"     /* Fractional number support */
#include "portability_macros.h"         /* Imports ADDR_PER_WORD definition */

#include "audio_log/audio_log.h"        /* Logging */

#include "pmalloc/pl_malloc.h"          /* Memory management functions */
#include "mem_utils/memory_table.h"     /* Memory table functions */
#include "buffer/cbuffer_c.h"           /* Cbuffer management functions */
#include "base_aud_cur_op.h"            /* Shared audio curation functions */

#include "aanc2_defs.h"                 /* AANC2 shared definitions */
#include "aanc2_gen_c.h"                /* AANC2 parameters/statistics/modes */
#include "aanc2_clipping.h"             /* AANC2 clipping types & functions */

/******************************************************************************
Public Constant Definitions
*/
#define AANC2_PROC_MEM_TABLE_SIZE            9 /* Memory table size (entries) */

#define AANC2_PROC_NUM_TAPS_BP               5 /* Number of bandpass taps */
#define AANC2_PROC_NUM_TAPS_PLANT            9 /* Number of plant model taps */
#define AANC2_PROC_NUM_TAPS_CONTROL          9 /* Number of control model taps */

/* Amount of memory required by the FxLMS */
#define AANC2_PROC_FXLMS_DM_BYTES            FXLMS100_DM_BYTES(\
                                                AANC2_PROC_NUM_TAPS_PLANT, \
                                                AANC2_PROC_NUM_TAPS_CONTROL, \
                                                AANC2_PROC_NUM_TAPS_BP)

#define AANC2_PROC_QUIET_MODE_RESET_FLAG     0x7FEFFFFF

/* Mask in the proc_flags word for any ED event */
#define AANC2_ED_FLAG_MASK                  (AANC2_FLAGS_ED_INT | \
                                             AANC2_FLAGS_ED_EXT | \
                                             AANC2_FLAGS_ED_PLAYBACK)

/* Mask in the proc_flags word for any saturation event */
#define AANC2_SATURATION_FLAG_MASK          (AANC2_FLAGS_SATURATION_INT | \
                                             AANC2_FLAGS_SATURATION_EXT | \
                                             AANC2_FLAGS_SATURATION_PLANT | \
                                             AANC2_FLAGS_SATURATION_CONTROL)

/* Clipping threshold */
#define AANC2_CLIPPING_THRESHOLD             0x3FFFFFFF

/* Reset masks in the proc_flags word for clipping */
#define AANC2_CLIPPING_RESET_INT_MIC_FLAG    0x7FFFFEFF
#define AANC2_CLIPPING_RESET_EXT_MIC_FLAG    0x7FFFFDFF
#define AANC2_CLIPPING_RESET_PLAYBACK_FLAG   0x7FFFFBFF

/* Mask in the proc_flags word for any clipping event */
#define AANC2_CLIPPING_FLAG_MASK            (AANC2_FLAGS_CLIPPING_INT | \
                                             AANC2_FLAGS_CLIPPING_EXT | \
                                             AANC2_FLAGS_CLIPPING_PLAYBACK)

/* Fine tune FxLMS: leakage term */
#define AANC2_FXLMS_GAMMA                    0
/* Maximum allowable gain step per FxLMS iteration */
#define AANC2_FXLMS_MAX_DELTA                0x00A40000u /* 1.28125, Q9.N */

/* ED attack and decay time (s) */
#define AANC2_ED_ATTACK                      0x07FEF9DBu /* 3.998, Q7.N */
#define AANC2_ED_DECAY                       0x07FEF9DBu /* 3.998, Q7.N */

/* ED initial frame time (s) */
#define AANC2_ED_INIT_FRAME                  0x00020C4Au /* 0.004, Q7.N */

/* ED count threshold time (s) */
#define AANC2_ED_COUNT_TH                    0x010A3D71u /* 0.52, Q7.N */


/******************************************************************************
Public Type Definitions
*/

/* AANC2 algorithm information */
typedef struct _AANC2_PROC
{
    unsigned proc_flags;            /* Algorithm processing flags */
    unsigned prev_flags;            /* Previous frame flags */

    malloc_t_entry *p_table;        /* Pointer to memory allocation table */
    tCbuffer *p_tmp_ed;             /* Pointer to temp buffer used by EDs */

    tCbuffer *p_tmp_int_ip;         /* Pointer to temp int mic ip (DM1) */
    tCbuffer *p_tmp_int_op;         /* Pointer to temp int mic op (DM2) */
    ED100_DMX *p_ed_int;            /* Pointer to int mic ED object */
    uint8 *p_ed_int_dm1;            /* Pointer to int mic ED DM1 memory */

    uint8 prev_gain;                /* Previously calculated gain value */
    bool self_speech_disabled:8;    /* Self speech disabled flag */
    bool pb_ratio_disabled:8;       /* Playback ratio disabled flag */
    unsigned self_speech_threshold; /* Self speech threshold */
    unsigned pb_ratio_threshold;    /* Playback ratio threshold */

    /* Note that temp int/ext mic input buffers are in different memory banks
     * to facilitate efficient clipping and peak detection. Output buffers
     * are in DM2 to facilitate efficient FXLMS processing.
     */
    tCbuffer *p_tmp_ext_ip;         /* Pointer to temp ext mic ip (DM2) */
    tCbuffer *p_tmp_ext_op;         /* Pointer to temp ext mic op (DM2) */
    ED100_DMX *p_ed_ext;            /* Pointer to ext mic ED object */
    uint8 *p_ed_ext_dm1;            /* Pointer to ext mic ED DM1 memory */

    tCbuffer *p_tmp_pb_ip;          /* Pointer to temp playback buffer */
    ED100_DMX *p_ed_pb;             /* Pointer to playback ED object */
    uint8 *p_ed_pb_dm1;             /* Pointer to playback ED DM1 memory */

    FXLMS100_DMX *p_fxlms;          /* Pointer to FxLMS data */
    uint8 *p_fxlms_dm1;             /* Pointer to FxLMS memory in DM1 */
    uint8 *p_fxlms_dm2;             /* Pointer to FxLMS memory in DM2 */

    AANC2_CLIP_DETECT clip_ext;     /* Clip detect struct for ext mic */
    AANC2_CLIP_DETECT clip_int;     /* Clip detect struct for int mic */
    AANC2_CLIP_DETECT clip_pb;      /* Clip detect struct for playback */

    unsigned quiet_hi_threshold;    /* Quiet mode high threshold */
    unsigned quiet_lo_threshold;    /* Quiet mode low threshold */

    /* Input/Output buffer pointers from terminals */
    tCbuffer *p_playback_ip;        /* Playback input terminal */
    tCbuffer *p_fbmon_ip;           /* FB mon input terminal */
    tCbuffer *p_mic_int_ip;         /* Internal mic input terminal */
    tCbuffer *p_mic_ext_ip;         /* External mic input terminal */

    tCbuffer *p_playback_op;        /* Playback output terminal */
    tCbuffer *p_fbmon_op;           /* FB mon output terminal */
    tCbuffer *p_mic_int_op;         /* Internal mic input terminal */
    tCbuffer *p_mic_ext_op;         /* External mic input terminal */

    void *f_handle;                 /* Pointer to AANC feature handle */
    EXT_OP_ID opid;                 /* Operator ID */

} AANC2_PROC;

/******************************************************************************
Public Function Definitions
*/

/**
 * \brief  Create the AANC2_PROC data object.
 *
 * \param  p_ag             Pointer to the AANC2_PROC data object.
 * \param  sample_rate      The sample rate of the operator. Used for the AANC
 *                          ED module.
 *
 * \return  boolean indicating success or failure.
 */
extern bool aanc2_proc_create(AANC2_PROC *p_ag, unsigned sample_rate);

/**
 * \brief  Destroy the AANC2_PROC data object.
 *
 * \param  p_ag             Pointer to the AANC2_PROC data object.
 *
 * \return  boolean indicating success or failure.
 */
extern bool aanc2_proc_destroy(AANC2_PROC *p_ag);

/**
 * \brief  Initialize the AANC2_PROC data object.
 *
 * \param  p_ag             Pointer to the AANC2_PROC data object.
 * \param  p_params         Pointer to the AANC2 parameters.
 *
 * \return  boolean indicating success or failure.
 */
extern bool aanc2_proc_initialize(AANC2_PROC *p_ag,
                                  AANC2_PARAMETERS *p_params);

/**
 * \brief  Initialize parameters involved in an AANC2 concurrency use-case
 *
 * \param  p_ag             Pointer to the AANC2_PROC data object.
 * \param  p_params         Pointer to the AANC2 parameters.
 * \param  concurrency      Boolean indicating whether in a concurrency or not
 *
 * \return  boolean indicating success or failure.
 */
extern bool aanc2_proc_initialize_concurrency(AANC2_PROC *p_ag,
                                              AANC2_PARAMETERS *p_params,
                                              bool concurrency);



/**
 * \brief  AANC2 algorithm process data
 *
 * \param  p_ag             Pointer to the AANC2_PROC data object.
 * \param  samples          Number of samples to process (frame size)
 * \param  calculate_gain   Boolean indicating whether the gain calculation step
 *                          should be performed.
 *
 * \return  boolean indicating success or failure.
 */
extern bool aanc2_proc_process_data(AANC2_PROC *p_ag,
                                    unsigned samples,
                                    bool calculate_gain);

/******************************************************************************
Inline Function Definitions
*/

/**
 * \brief  Reset the previous flag state in the AANC2_PROC data object.
 *
 * \param  p_ag             Pointer to the AANC2_PROC data object.
 *
 * \return  boolean indicating success or failure.
 */
inline void aanc2_proc_reset_prev_flags(AANC2_PROC *p_ag);

inline void aanc2_proc_reset_prev_flags(AANC2_PROC *p_ag)
{
    p_ag->prev_flags = 0;
    return;
}

/**
 * \brief  Update the previous flag detections
 *
 * \param  p_ag      Pointer to the AANC2_PROC data object
 *
 * \return  void.
 *
 * Because previous flags are used outside of the aanc2_proc processing loop
 * this allows the flags to be updated when that processing is done.
 */
inline void aanc2_proc_update_prev_flags(AANC2_PROC *p_ag);

inline void aanc2_proc_update_prev_flags(AANC2_PROC *p_ag)
{
    p_ag->prev_flags = p_ag->proc_flags;
    return;
}


#endif /* _AANC2_PROC_H_ */
