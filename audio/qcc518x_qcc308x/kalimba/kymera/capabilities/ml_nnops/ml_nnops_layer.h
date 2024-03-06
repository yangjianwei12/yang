/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  ml_nnops_layer.h
 * \ingroup capabilities
 *
 * Source File for constructing NN Operator layer. <br>
 *
 */

#ifndef ML_NNOPS_LAYER_H
#define ML_NNOPS_LAYER_H

/****************************************************************************
Include Files
*/
#include "layer_lite_kalimba.h"
#include "ml_defines.h"
#include "pmalloc/pl_malloc.h"
#include "ml_nnops_tensor_def.h"
#include "ml_op_interface.h"

/* ML_NNOPS_LAYER link list. Please note that this is an example
 * showing how to connect different ML layers.
 */
typedef struct ML_NNOPS_LAYER
{
    ml_oph_t h_ml_op;
    LAYER_TYPE op_type;
    struct ML_NNOPS_LAYER *next;   
}ML_NNOPS_LAYER;

/**
 * \brief Creates machine learning layer
 *
 * \param ml_nnops_layer Pointer to the 'ML_NNOPS_LAYER' structure.
 *
 * \return TRUE is success, FALSE otherwise
 */
bool create_layer(ML_NNOPS_LAYER* ml_nnops_layer);

/**
 * \brief Destroys machine learning layer
 *
 * \param ml_nnops_layer Pointer to the 'ML_NNOPS_LAYER' structure.
 *
 * \return TRUE is success, FALSE otherwise
 */
bool destroy_layer(ML_NNOPS_LAYER* ml_nnops_layer);

#endif /* ML_NNOPS_LAYER_H */
