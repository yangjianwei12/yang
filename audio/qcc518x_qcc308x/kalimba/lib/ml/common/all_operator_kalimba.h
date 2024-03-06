/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  all_operator_kalimba.h
 * \ingroup  ml
 *
 * Master header file to include all individual NN operator header files
 *
 */

#ifndef ALL_OPERATOR_KALIMBA_H
#define ALL_OPERATOR_KALIMBA_H

#include "add_kalimba_public.h"
#include "argmax_kalimba_public.h"
#include "argmin_kalimba_public.h"
#include "averagepool_kalimba_public.h"
#include "batchnorm_kalimba_public.h"
#include "clip_kalimba_public.h"
#include "concat_kalimba_public.h"
#include "constantfill_kalimba_public.h"
#include "conv_kalimba_public.h"
#include "conv_transpose_kalimba_public.h"
#include "div_kalimba_public.h"
#include "exp_kalimba_public.h"
#include "expand_kalimba_public.h"
#ifdef INSTALL_INTERNAL_NN_OPERATOR_SET
#include "framer_kalimba_public.h"
#endif /* INSTALL_INTERNAL_NN_OPERATOR_SET */
#include "gather_kalimba_public.h"
#include "gemm_kalimba_public.h"
#include "global_averagepool_kalimba_public.h"
#include "gru_kalimba_public.h"
#include "identity_kalimba_public.h"
#include "log_kalimba_public.h"
#include "logsoftmax_kalimba_public.h"
#include "lstm_kalimba_public.h"
#include "max_kalimba_public.h"
#include "maxout_kalimba_public.h"
#include "maxpool_kalimba_public.h"
#include "min_kalimba_public.h"
#include "mul_kalimba_public.h"
#include "neg_kalimba_public.h"
#include "pad_kalimba_public.h"
#include "pow_kalimba_public.h"
#include "prelu_kalimba_public.h"
#include "reduce_kalimba_public.h"
#include "reducemean_kalimba_public.h"
#include "relu_kalimba_public.h"
#include "reshape_kalimba_public.h"
#include "rnn_kalimba_public.h"
#include "shape_kalimba_public.h"
#include "sigmoid_kalimba_public.h"
#include "slice_kalimba_public.h"
#include "softmax_kalimba_public.h"
#include "split_kalimba_public.h"
#include "sqrt_kalimba_public.h"
#include "squeeze_kalimba_public.h"
#include "subtract_kalimba_public.h"
#include "tanh_kalimba_public.h"
#include "transpose_kalimba_public.h"
#include "unsqueeze_kalimba_public.h"
#endif // ALL_OPERATOR_KALIMBA_H
