/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_ml_private.h
\brief      Kymera Machine Learning Engine private type definitions
*/

#ifndef KYMERA_ML_PRIVATE_H
#define KYMERA_ML_PRIVATE_H

#include <chain.h>
#include "kymera_ml.h"

typedef enum {
    ml_engine_idle,
    ml_engine_started
} kymera_ml_engine_status_t;

typedef struct kymera_ml_engine_t {
    kymera_chain_handle_t chain;
    uint16 creation_counter;
} kymera_ml_engine_t;

typedef struct kymera_ml_user_config_t {
    Operator op;
    uint16 use_case_id;
    model_access_methods_t access_method;
    DataFileID model_file_id;
    struct kymera_ml_user_config_t* next;
} kymera_ml_user_config_t;

typedef struct kymera_ml_engine_state_t {
    kymera_ml_engine_t ml_engine;
    kymera_ml_user_config_t *user_configs;
} kymera_ml_engine_state_t;

#endif // KYMERA_ML_PRIVATE_H
