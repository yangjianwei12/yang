/*!
\copyright  Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle VA Wake-Up-Word chain

*/

#ifdef INCLUDE_VOICE_UI
#include "kymera_va_wuw_chain.h"
#include "kymera_va_mic_chain.h"
#include "kymera_va_common.h"
#include "kymera_chain_roles.h"
#include "kymera_data.h"
#include "kymera_setup.h"
#include "kymera_chain_utils.h"
#include <logging.h>
#include <opmsg_prim.h>
#include <custom_operator.h>
#include <vmal.h>

#define GET_WUW_VERSION_NUMBER_MSG_ID (0x0A)

static void kymera_ConfigureGraphManager(Operator graph_manager, const void *params);
static void kymera_ConfigureWuwEngine(Operator wuw, const void *params);
static const operator_config_map_t graph_manager_operator_config_map[] = {
    { OPR_VA_GRAPH_MANAGER, kymera_ConfigureGraphManager }
};
static const operator_config_map_t wuw_engine_operator_config_map[] = {
    { OPR_WUW, kymera_ConfigureWuwEngine }
};

static struct
{
    const appKymeraVaWuwChainTable *chain_config_map;
    kymera_chain_handle_t va_wuw_chain;
    kymera_chain_handle_t graph_manager_chain;
    DataFileID wuw_model_handle;
    va_wuw_engine_t largest_wuw_engine;
    unsigned gm_delegation_started:1;
} state =
{
    .chain_config_map = NULL,
    .va_wuw_chain = NULL,
    .graph_manager_chain = NULL,
    .wuw_model_handle = DATA_FILE_ID_INVALID,
    .largest_wuw_engine = va_wuw_engine_none,
    .gm_delegation_started = FALSE
};

static void kymera_ConfigureGraphManager(Operator graph_manager, const void *params)
{
    const va_wuw_chain_create_params_t *chain_params = params;
    Task task = chain_params->operators_params.wuw_detection_handler;
    PanicNull(task);
    MessageOperatorTask(graph_manager, task);
    const OPMSG_VA_GM_SET_SPLITTER_OFFSET splitter_offset = {
        OPMSG_VA_GM_SET_SPLITTER_OFFSET_CREATE(OPMSG_VA_GM_ID_SET_SPLITTER_OFFSET,
                                               chain_params->operators_params.engine_init_preroll_ms)
    };
    PanicFalse(OperatorMessage(graph_manager, splitter_offset._data,
                               OPMSG_VA_GM_SET_SPLITTER_OFFSET_WORD_SIZE, NULL, 0));
}

static void kymera_ConfigureWuwEngine(Operator wuw, const void *params)
{
    const va_wuw_chain_create_params_t *chain_params = params;
    const va_wuw_chain_op_params_t *op_params = &chain_params->operators_params;
    if (state.wuw_model_handle == DATA_FILE_ID_INVALID)
    {
        PanicFalse(op_params->LoadWakeUpWordModel != NULL);
        state.wuw_model_handle = op_params->LoadWakeUpWordModel(op_params->wuw_model);
        PanicFalse(state.wuw_model_handle != DATA_FILE_ID_INVALID);
    }
    OperatorsStandardSetSampleRate(wuw, Kymera_GetVaSampleRate());
    OperatorsWuwEngineLoadModel(wuw, state.wuw_model_handle);
}

static const chain_config_t * kymera_GetWuwEngineChainConfig(va_wuw_engine_t wuw_engine)
{
    PanicNull((void *)state.chain_config_map);

    for(unsigned i = 0; i < state.chain_config_map->table_length; i++)
    {
        if (state.chain_config_map->chain_table[i].chain_params.wuw_engine == wuw_engine)
        {
            return state.chain_config_map->chain_table[i].chain_config;
        }
    }

    DEBUG_LOG_PANIC("kymera_GetWuwEngineChainConfig: Wake-Up-Word engine not supported!");
    return NULL;
}

static const chain_config_t * kymera_GetGraphManagerChainConfig(void)
{
    return Kymera_GetChainConfigs()->chain_va_graph_manager_config;
}

static Operator kymera_GetOperatorFromChain(unsigned operator_role, kymera_chain_handle_t chain)
{
    PanicNull(chain);
    return ChainGetOperatorByRole(chain, operator_role);
}

static Sink kymera_GetChainInput(unsigned input_role)
{
    PanicNull(state.va_wuw_chain);
    return ChainGetInput(state.va_wuw_chain, input_role);
}

static void kymera_CreateChains(const kymera_va_wuw_chain_params_t *params)
{
    PanicNotNull(state.va_wuw_chain);
    PanicNotNull(state.graph_manager_chain);

    bool creating_largest_wuw_engine = state.largest_wuw_engine == params->wuw_engine;

    if(creating_largest_wuw_engine || (state.largest_wuw_engine == va_wuw_engine_none))
    {
        state.va_wuw_chain = PanicNull(ChainCreate(kymera_GetWuwEngineChainConfig(params->wuw_engine)));
        state.graph_manager_chain = PanicNull(ChainCreate(kymera_GetGraphManagerChainConfig()));
    }
    else
    {
        /* Create largest WuW engine first to prevent fragmentation */
        state.va_wuw_chain = PanicNull(ChainCreate(kymera_GetWuwEngineChainConfig(state.largest_wuw_engine)));
        state.graph_manager_chain = PanicNull(ChainCreate(kymera_GetGraphManagerChainConfig()));

        /* Remove the largest WuW engine and replace with requested engine now that VA graph manager is place-holding */
        ChainDestroy(state.va_wuw_chain);
        state.va_wuw_chain = NULL;
        state.va_wuw_chain = PanicNull(ChainCreate(kymera_GetWuwEngineChainConfig(params->wuw_engine)));
    }
}

static void kymera_ConfigureChains(const va_wuw_chain_create_params_t *params)
{
    Kymera_ConfigureChain(state.va_wuw_chain, wuw_engine_operator_config_map, ARRAY_DIM(wuw_engine_operator_config_map), params);

    if(KymeraGetTaskData()->chain_config_callbacks && KymeraGetTaskData()->chain_config_callbacks->ConfigureWuwChain)
    {
        KymeraGetTaskData()->chain_config_callbacks->ConfigureWuwChain(state.va_wuw_chain);
    }

    Kymera_ConfigureChain(state.graph_manager_chain, graph_manager_operator_config_map, ARRAY_DIM(graph_manager_operator_config_map), params);

    if(KymeraGetTaskData()->chain_config_callbacks && KymeraGetTaskData()->chain_config_callbacks->ConfigureGraphManagerChain)
    {
        KymeraGetTaskData()->chain_config_callbacks->ConfigureGraphManagerChain(state.graph_manager_chain);
    }
}

static void kymera_ConnectChains(void)
{
    ChainConnect(state.va_wuw_chain);
    ChainConnect(state.graph_manager_chain);
}

static void kymera_DisconnectChain(void)
{
    StreamDisconnect(NULL, kymera_GetChainInput(EPR_VA_WUW_IN));
}

static void kymera_RunUsingOperatorsNotToPreserve(OperatorFunction function)
{
    Operator ops[] = {kymera_GetOperatorFromChain(OPR_VA_GRAPH_MANAGER, state.graph_manager_chain), kymera_GetOperatorFromChain(OPR_WUW, state.va_wuw_chain)};
    function(ops, ARRAY_DIM(ops));
}

static void kymera_ChainSleep(Operator *array, unsigned length_of_array)
{
    operator_list_t operators_to_exclude = {array, length_of_array};
    ChainSleep(state.va_wuw_chain, &operators_to_exclude);
    ChainSleep(state.graph_manager_chain, &operators_to_exclude);
}

static void kymera_ChainWake(Operator *array, unsigned length_of_array)
{
    operator_list_t operators_to_exclude = {array, length_of_array};
    ChainWake(state.va_wuw_chain, &operators_to_exclude);
    ChainWake(state.graph_manager_chain, &operators_to_exclude);
}

#ifdef ENABLE_GRAPH_MANAGER_CLOCK_CONTROL

static void kymera_VaGmSetTriggerModeClock(Operator gm, audio_dsp_clock_type active_mode, audio_dsp_clock_type trigger_mode)
{
    const OPMSG_VA_GM_SET_TRIGGER_MODE_CLOCK msg =
    {
        OPMSG_VA_GM_SET_TRIGGER_MODE_CLOCK_CREATE(OPMSG_VA_GM_ID_SET_TRIGGER_MODE_CLOCK, active_mode, trigger_mode)
    };

    if(!OperatorMessage(gm, msg._data, OPMSG_VA_GM_SET_TRIGGER_MODE_CLOCK_WORD_SIZE, NULL, 0))
    {
        DEBUG_LOG_ERROR("kymera_VaGmSetTriggerModeClock cmd failed active_mode=enum:audio_dsp_clock_type:%d trigger_mode=enum:audio_dsp_clock_type:%d",
                         active_mode, trigger_mode);
    }
}

static OPMSG_VA_GM_LOAD kymera_GetGmLoadFromClockType(audio_dsp_clock_type clock_type)
{
    switch(clock_type)
    {
        case AUDIO_DSP_EXT_LP_CLOCK:
        case AUDIO_DSP_VERY_LP_CLOCK:
        case AUDIO_DSP_LP_CLOCK:
        case AUDIO_DSP_VERY_SLOW_CLOCK:
            DEBUG_LOG_WARN("kymera_GetGmLoadFromClockType: enum:audio_dsp_clock_type:%d Not Supported, using enum:audio_dsp_clock_type:%d instead", clock_type, AUDIO_DSP_SLOW_CLOCK);
        case AUDIO_DSP_SLOW_CLOCK:
            return OPMSG_VA_GM_LOAD_LOW;
        case AUDIO_DSP_BASE_CLOCK:
            return OPMSG_VA_GM_LOAD_MEDIUM;
        case AUDIO_DSP_TURBO_CLOCK:
            return OPMSG_VA_GM_LOAD_HIGH;
        case AUDIO_DSP_TURBO_PLUS_CLOCK:
            return OPMSG_VA_GM_LOAD_MAX;
        default:
            DEBUG_LOG_PANIC("kymera_GetGmLoadFromClockType: Unknown value enum:audio_dsp_clock_type:%d", clock_type);
            return 0;
    }
}

static void kymera_VaGmSetGraphLoad(Operator gm, audio_dsp_clock_type full_load_clock)
{
    if(full_load_clock != AUDIO_DSP_CLOCK_NO_CHANGE)
    {
        const OPMSG_VA_GM_SET_GRAPH_LOAD msg =
        {
            OPMSG_VA_GM_SET_GRAPH_LOAD_CREATE(OPMSG_VA_GM_ID_SET_GRAPH_LOAD, kymera_GetGmLoadFromClockType(full_load_clock))
        };
        PanicFalse(OperatorMessage(gm, msg._data, OPMSG_VA_GM_SET_GRAPH_LOAD_WORD_SIZE, NULL, 0));
    }
}

static void kymera_SetGmClockSpeed(audio_dsp_clock_type active_mode, audio_dsp_clock_type trigger_mode)
{
    if(state.graph_manager_chain)
    {
        Operator gm = kymera_GetOperatorFromChain(OPR_VA_GRAPH_MANAGER, state.graph_manager_chain);
        if (gm != INVALID_OPERATOR)
        {
            kymera_VaGmSetTriggerModeClock(gm, active_mode, trigger_mode);
            kymera_VaGmSetGraphLoad(gm, trigger_mode);
        }
    }
}
#endif /* #ifdef ENABLE_GRAPH_MANAGER_CLOCK_CONTROL */

void Kymera_CreateVaWuwChain(const va_wuw_chain_create_params_t *params)
{
    PanicFalse(params != NULL);
    kymera_CreateChains(&params->chain_params);
    kymera_ConfigureChains(params);
    kymera_ConnectChains();
}

void Kymera_DestroyVaWuwChain(void)
{
    kymera_DisconnectChain();
    if (state.wuw_model_handle != DATA_FILE_ID_INVALID)
    {
        PanicFalse(OperatorDataUnloadEx(state.wuw_model_handle));
        state.wuw_model_handle = DATA_FILE_ID_INVALID;
    }
    ChainDestroy(state.va_wuw_chain);
    ChainDestroy(state.graph_manager_chain);
    state.va_wuw_chain = NULL;
    state.graph_manager_chain = NULL;
}

void Kymera_ConnectVaWuwChainToMicChain(void)
{
    PanicNull(StreamConnect(Kymera_GetVaMicChainWuwOutput(), kymera_GetChainInput(EPR_VA_WUW_IN)));
}

void Kymera_StartVaWuwChain(void)
{
    ChainStart(state.va_wuw_chain);
    ChainStart(state.graph_manager_chain);
}

void Kymera_StopVaWuwChain(void)
{
    ChainStop(state.va_wuw_chain);
    ChainStop(state.graph_manager_chain);
}

void Kymera_VaWuwChainSleep(void)
{
    PanicFalse(OperatorFrameworkTriggerNotificationStart(TRIGGER_ON_GM, kymera_GetOperatorFromChain(OPR_VA_GRAPH_MANAGER, state.graph_manager_chain)));
    kymera_RunUsingOperatorsNotToPreserve(kymera_ChainSleep);
}

void Kymera_VaWuwChainWake(void)
{
    kymera_RunUsingOperatorsNotToPreserve(kymera_ChainWake);
    PanicFalse(OperatorFrameworkTriggerNotificationStop());
}

void Kymera_VaWuwChainStartGraphManagerDelegation(void)
{
    Kymera_VaMicChainStartGraphManagerDelegation(kymera_GetOperatorFromChain(OPR_VA_GRAPH_MANAGER, state.graph_manager_chain), kymera_GetOperatorFromChain(OPR_WUW, state.va_wuw_chain));
    state.gm_delegation_started = TRUE;
}

void Kymera_VaWuwChainStopGraphManagerDelegation(void)
{
    Kymera_VaMicChainStopGraphManagerDelegation(kymera_GetOperatorFromChain(OPR_VA_GRAPH_MANAGER, state.graph_manager_chain), kymera_GetOperatorFromChain(OPR_WUW, state.va_wuw_chain));
    state.gm_delegation_started = FALSE;
}

bool Kymera_VaWuwChainSetDspClock(audio_dsp_clock_type active_mode, audio_dsp_clock_type trigger_mode)
{
#ifdef ENABLE_GRAPH_MANAGER_CLOCK_CONTROL
    kymera_SetGmClockSpeed(active_mode, trigger_mode);
    // After delegation to GM it is in control of the DSP clock and it shouldn't be set via the DSP framework
    return state.gm_delegation_started;
#else
    UNUSED(active_mode);
    UNUSED(trigger_mode);
    // Set DSP clock via the DSP framework
    return FALSE;
#endif
}

void * Kymera_VaWuwChainGetMetadata(size_t size)
{
    return OperatorsGetApvaMetadata(kymera_GetOperatorFromChain(OPR_WUW, state.va_wuw_chain), size);
}

void Kymera_SetVaWuwChainTable(const appKymeraVaWuwChainTable *chain_table)
{
    state.chain_config_map = chain_table;
}

void Kymera_GetWakeUpWordEngineVersion(va_wuw_engine_t wuw_engine, va_audio_wuw_engine_version_t *version)
{
    uint16 id = GET_WUW_VERSION_NUMBER_MSG_ID;
    uint16 recv_msg[3];
    bool chain_already_created = (state.va_wuw_chain != NULL);

    if(!chain_already_created)
    {
        state.va_wuw_chain = PanicNull(ChainCreate(kymera_GetWuwEngineChainConfig(wuw_engine)));
    }

    Operator op = kymera_GetOperatorFromChain(OPR_WUW, state.va_wuw_chain);
    PanicFalse(VmalOperatorMessage(op, &id, SIZEOF_OPERATOR_MESSAGE(id), recv_msg, SIZEOF_OPERATOR_MESSAGE(recv_msg)));
    PanicFalse(recv_msg[0] == GET_WUW_VERSION_NUMBER_MSG_ID);
    version->msw = recv_msg[1];
    version->lsw = recv_msg[2];

    DEBUG_LOG("Kymera_GetWuwEngineVersion id 0x%X, msw 0x%X, lsw 0x%X", recv_msg[0], version->msw, version->lsw);

    if(!chain_already_created)
    {
        ChainDestroy(state.va_wuw_chain);
        state.va_wuw_chain = NULL;
    }
}

void Kymera_StoreLargestWuwEngine(void)
{
    PanicNull((void *)state.chain_config_map);

    uint32 largest_size = 0;

    for(unsigned i = 0; i < state.chain_config_map->table_length; i++)
    {
        uint32 size = 0;

        if(state.chain_config_map->chain_table[i].chain_config->operator_config->role == OPR_WUW)
        {
            size = CustomOperatorGetProgramSize(state.chain_config_map->chain_table[i].chain_config->operator_config->capability_id);
        }

        if(size > largest_size)
        {
            largest_size = size;
            state.largest_wuw_engine = state.chain_config_map->chain_table[i].chain_params.wuw_engine;
        }
    }

    DEBUG_LOG_ALWAYS("Kymera_StoreLargestWuwEngine: Largest engine is enum:va_wuw_engine_t:%d with size %u", state.largest_wuw_engine, largest_size);
}

#endif /*#ifdef INCLUDE_VOICE_UI */
