/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  anc_compander_defs.h
 * \ingroup anc_compander
 *
 * ANC Compander operator constant defs
 *
 */
#ifndef _ANC_COMPANDER_DEFS_H_
#define _ANC_COMPANDER_DEFS_H_

/****************************************************************************
Public Constant Definitions
*/
#define COMPANDER_VERSION_LENGTH                  2

/* Number of statistics reported by the capability */
#define ANC_COMPANDER_N_STAT (sizeof(ANC_COMPANDER_STATISTICS)/sizeof(ParamType))

/* Terminal and channel defs */
/* Max number of sources for ANC Comapnder operator */
#define ANC_COMPANDER_CAP_MAX_SOURCES             3
/* Max number of sinks for ANC Comapnder operator */
#define ANC_COMPANDER_CAP_MAX_SINKS               3
#define ANC_COMPANDER_CAP_MAX_METADATA_TERMINALS  3
/* Max of sinks/sources */
#define ANC_COMPANDER_CAP_MAX_CHANNELS            3
/* Number of channels for companding */
#define ANC_COMPANDER_NUM_COMPANDING_CHANNELS     1
/* Input channel number for Playback data */
#define ANC_COMPANDER_PLAYBACK_TERMINAL_NUM       0
/* Input channel number for companding. (FF Mic data) */
#define ANC_COMPANDER_COMPANDING_TERMINAL_NUM     1
/* Input channel number for FB Mic data */
#define ANC_COMPANDER_FB_PATH_TERMINAL_NUM        2

#define ANC_COMPANDER_PLAYBACK_TERMINAL_POS \
            (1 << ANC_COMPANDER_PLAYBACK_TERMINAL_NUM)
#define ANC_COMPANDER_COMPANDING_TERMINAL_POS \
            (1 << ANC_COMPANDER_COMPANDING_TERMINAL_NUM)
#define ANC_COMPANDER_FB_PATH_TERMINAL_POS \
            (1 << ANC_COMPANDER_FB_PATH_TERMINAL_NUM)

/* Flags */
#define ANC_COMPANDER_SUPPORTS_METADATA           TRUE
#define ANC_COMPANDER_SUPPORTS_IN_PLACE           TRUE
#define ANC_COMPANDER_DYNAMIC_BUFFERS             TRUE

/* Masks */
#define ANC_COMPANDER_MIN_VALID_SOURCES           0
#define ANC_COMPANDER_MIN_VALID_SINKS (1 << ANC_COMPANDER_COMPANDING_TERMINAL_NUM)
#define ANC_COMPANDER_MAX_VALID_SINKS (ANC_COMPANDER_COMPANDING_TERMINAL_POS | \
                                       ANC_COMPANDER_FB_PATH_TERMINAL_POS)
#define ANC_COMPANDER_MAX_VALID_SOURCES (ANC_COMPANDER_COMPANDING_TERMINAL_POS | \
                                        ANC_COMPANDER_FB_PATH_TERMINAL_POS)


/* Misc defines */
/* Block size at 96kHz for 1ms (96 samples) */
#define ANC_COMPANDER_WRAPPER_BLOCK_SIZE          96
#define ANC_COMPANDER_INTERNAL_BUFFER_SIZE \
    (ANC_COMPANDER_WRAPPER_BLOCK_SIZE + 1)
#define ANC_COMPANDER_PERSISTENCE_UCID            0
#define ANC_COMPANDER_PERSIST_DATA_LEN            2
/* Lookahead duration shift (Shift value is in Q12.20 format) */
#define ANC_COMPANDER_LOOKAHEAD_DUR_SHIFT         20

#endif /* _ANC_COMPANDER_DEFS_H_ */