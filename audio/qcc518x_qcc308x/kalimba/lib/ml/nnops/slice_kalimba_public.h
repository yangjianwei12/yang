/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  slice_kalimba_public.h
 * \ingroup  ml operators
 *
 * Header file for slice operator
 *
 */

#ifndef SLICE_KALIMBA_PUBLIC_H
#define SLICE_KALIMBA_PUBLIC_H

#include "layer_lite_kalimba.h"

/* Max dimensions supported for slicing */
#define SLICE_MAX_NDIM (4)
/* Compatible version information */
#define SLICE_COMPATIBLE_VERSION 0
/* Operator version information*/
#define SLICE_OPERATOR_VERSION 0

typedef struct Slice {

    uint16 starts_w; /*!< start index for width */
    uint16 ends_w;   /*!< end index for width */

    uint16 starts_h; /*!< start index for height */
    uint16 ends_h;   /*!< end index for height */

    uint16 starts_c; /*!< start index for channel */
    uint16 ends_c;   /*!< end index for channel */

    uint16 starts_n; /*!< start index for batch */
    uint16 ends_n;   /*!< end index for batch */

    int8 scale;  /*!< scale factor for input tensor */
    int8 pad1;   /*!< unused field to make it word aligned */
    uint16 pad2; /*!< unused field to make it word aligned */

} Slice;

/**
 * \brief slice processing function
 *
 * param kalimba layer_lite structure with input, output and slice params
 *
 * \return ML_SUCCESS if successfull, else ML_FAIL
 */
int slice_forward(layer_lite *layer);

#endif /*SLICE_KALIMBA_PUBLIC_H*/
