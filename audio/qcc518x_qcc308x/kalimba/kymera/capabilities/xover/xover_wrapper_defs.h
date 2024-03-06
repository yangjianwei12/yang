/****************************************************************************
 * Copyright (c) 2015 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
#ifndef _XOVER_WRAPPER_DEFS_H_
#define _XOVER_WRAPPER_DEFS_H_

/****************************************************************************
Public Constant Definitions
*/
#define XOVER_VERSION_LENGTH                       2

/** default block size for this operator's terminals */
#define XOVER_CAP_DEFAULT_BLOCK_SIZE                   1
#define XOVER_CAP_MAX_IO_GROUPS                        4
#define XOVER_CAP_MAX_IN_CHANNELS                      XOVER_CAP_MAX_IO_GROUPS
#define XOVER_CAP_MAX_OUT_CHANNELS_2BAND               2 * XOVER_CAP_MAX_IO_GROUPS
#define XOVER_CAP_MAX_OUT_CHANNELS_3BAND               3 * XOVER_CAP_MAX_IO_GROUPS
#define XOVER_PEQ_PARAMETERS_SIZE                     (44*sizeof(unsigned)) /* PEQ has 44 parameters */
#define XOVER_MAX_NUM_INT_BUF                          4
#define XOVER_2BAND_NUM_INT_BUF                        2
#define XOVER_3BAND_NUM_INT_BUF                        4

/** channel mask values */
#define CHAN_MASK_0                                    1
#define CHAN_MASK_1                                    2
#define CHAN_MASK_2                                    4
#define CHAN_MASK_3                                    8

/** capability selection values */
#define XOVER_2BAND                                    0
#define XOVER_3BAND                                    1

#endif /* _XOVER_WRAPPER_DEFS_H_ */
