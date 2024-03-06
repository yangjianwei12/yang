/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_ml_engine.c
\brief      Kymera Machine Learning Engine
*/

#ifdef INCLUDE_AUDIO_ML_ENGINE

#include <stdlib.h>

#include <operator.h>
#include <operators.h>
#include <custom_operator.h>
#include <operators_ml.h>
#include <panic.h>
#include <logging.h>
#include <file.h>

#include <cap_id_prim.h>

#include "kymera_ml.h"

#include "kymera_ml_private.h"
#include "kymera_setup.h"

kymera_ml_engine_state_t state = {
    .ml_engine = {
        .chain = NULL,
        .creation_counter = 0,
    },
    .user_configs = NULL,
};

static void kymera_MlAddUserConfig(Operator op, uint16 use_case_id, model_access_methods_t access_method, DataFileID model_file_id)
{
    kymera_ml_user_config_t* this_user_config = PanicUnlessNew(kymera_ml_user_config_t);
    this_user_config->op = op;
    this_user_config->use_case_id = use_case_id;
    this_user_config->access_method = access_method;
    this_user_config->model_file_id = model_file_id;
    this_user_config->next = NULL;

    kymera_ml_user_config_t** current_config = &state.user_configs;
    while(*current_config)
    {
        current_config = &(*current_config)->next;
    }
    *current_config = this_user_config;
}

static bool kymera_MlPopUserConfig(Operator op, kymera_ml_user_config_t* output_config)
{
    for(kymera_ml_user_config_t** current_config = &state.user_configs;
        *current_config != NULL; current_config = &(*current_config)->next)
    {
        if ((*current_config)->op == op)
        {
            kymera_ml_user_config_t* popped_config = *current_config;
            *current_config = (*current_config)->next;

            memcpy(output_config, popped_config, sizeof(*output_config));
            output_config->next = NULL;

            free(popped_config);
            return TRUE;
        }
    }

    DEBUG_LOG("Operator not found, op=0x%x", op);
    return FALSE;
}

static bool kymera_MlUserConfigExists(Operator op)
{
    for(kymera_ml_user_config_t* current_config = state.user_configs; current_config != NULL; current_config = current_config->next)
    {
        if(current_config->op == op)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void Kymera_MlActivate(Operator op, const kymera_ml_config_t *ml_config)
{
    if (kymera_MlUserConfigExists(op))
    {
        DEBUG_LOG_PANIC("Can't activate same operator twice, op = 0x%x", op);
    }
    else
    {
        FILE_INDEX model_file_idx = FileFind(FILE_ROOT, ml_config->model_file, strlen(ml_config->model_file));
        PanicFalse(FILE_NONE != model_file_idx);

        DataFileID model_file_id = OperatorsMlLoadModel(op, ml_config->use_case_id, model_file_idx, ml_config->batch_reset_count, ml_config->access_method);

        kymera_MlAddUserConfig(op, ml_config->use_case_id, ml_config->access_method, model_file_id);

        OperatorsMlActivateModel(op, ml_config->use_case_id);
    }
}

void Kymera_MlDeactivate(Operator op)
{
    kymera_ml_user_config_t user_config = {0};
    PanicFalse(kymera_MlPopUserConfig(op, &user_config));

    if (model_copy_and_auto_unload != user_config.access_method)
    {
        OperatorsMlUnloadModel(op, user_config.use_case_id, user_config.model_file_id);
    }
}

void Kymera_MlEngineCreate(void)
{
    if ((state.ml_engine.chain == NULL) && (0 == state.ml_engine.creation_counter))
    {
        const chain_config_t * config = Kymera_GetChainConfigs()->chain_ml_engine_config;
        state.ml_engine.chain = PanicNull(ChainCreate(config));
        DEBUG_LOG("Kymera_MlEngineCreate");
        ChainStart(state.ml_engine.chain);
    }
    state.ml_engine.creation_counter++;
}

void Kymera_MlEngineDestroy(void)
{
    PanicFalse(state.ml_engine.creation_counter > 0);

    state.ml_engine.creation_counter--;
    if ((state.ml_engine.chain) && (0 == state.ml_engine.creation_counter))
    {
        DEBUG_LOG("Kymera_MlEngineDestroy");
        ChainStop(state.ml_engine.chain);
        ChainDestroy(state.ml_engine.chain);
        state.ml_engine.chain = NULL;
    }
}

#ifdef HOSTED_TEST_ENVIRONMENT
void Kymera_MlResetState()
{
    state.ml_engine.chain = NULL;
    state.ml_engine.creation_counter = 0;

    if(state.user_configs)
    {
        while (state.user_configs->next)
        {
            kymera_ml_user_config_t* next_config = state.user_configs->next;
            free(state.user_configs);
            state.user_configs = next_config;
        }
        free(state.user_configs);
        state.user_configs = NULL;
    }
}

kymera_ml_engine_state_t* Kymera_MlGetState()
{
    return &state;
}
#endif /* HOSTED_TEST_ENVIRONMENT */

#endif /* INCLUDE_KYMERA_ML */
