/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_basic_passthrough.c
\brief      Implementation of Basic Passthrough connected to Mic Framework in noise cancellation usecases.
*/

#include "kymera_basic_passthrough.h"
#include "kymera_setup.h"

#if defined(ENABLE_ADAPTIVE_ANC)

static kymera_chain_handle_t basic_passthrough_chain;

#define kymeraBasicPassthrough_IsActive() (kymeraBasicPassthrough_GetChain() != NULL)
#define kymeraBasicPassthrough_PanicIfNotActive()  if(!kymeraBasicPassthrough_IsActive()) \
                                                                Panic()
#define kymeraBasicPassthrough_PanicIfActive()  if(kymeraBasicPassthrough_IsActive()) \
                                                                Panic()
                                                                
#define BPT_BUFFER_SIZE 1200U

static kymera_chain_handle_t kymeraBasicPassthrough_GetChain(void)
{
    return basic_passthrough_chain;
}

static void kymeraBasicPassthrough_SetChain(kymera_chain_handle_t chain)
{
    basic_passthrough_chain = chain;
}

static Operator kymeraBasicPassthrough_GetOperator(void)
{
    return ChainGetOperatorByRole(kymeraBasicPassthrough_GetChain(), OPR_BASIC_PASSTHROUGH);
}

void KymeraBasicPassthrough_Create(void)
{
    kymeraBasicPassthrough_PanicIfActive();
    kymeraBasicPassthrough_SetChain(PanicNull(ChainCreate(Kymera_GetChainConfigs()->chain_anc_client_basic_passthrough_config)));
}

void KymeraBasicPassthrough_Configure(const KYMERA_INTERNAL_AANC_ENABLE_T* param)
{
    UNUSED(param);
    kymeraBasicPassthrough_PanicIfNotActive();
    Operator op = kymeraBasicPassthrough_GetOperator();
    OperatorsStandardSetBufferSize(op, BPT_BUFFER_SIZE);
}

void KymeraBasicPassthrough_Connect(void)
{
    kymeraBasicPassthrough_PanicIfNotActive();
    ChainConnect(kymeraBasicPassthrough_GetChain());
}

void KymeraBasicPassthrough_Start(void)
{
    kymeraBasicPassthrough_PanicIfNotActive();
    ChainStart(kymeraBasicPassthrough_GetChain());
}

void KymeraBasicPassthrough_Stop(void)
{
    kymeraBasicPassthrough_PanicIfNotActive();
    ChainStop(kymeraBasicPassthrough_GetChain());
}

void KymeraBasicPassthrough_Destroy(void)
{
    kymeraBasicPassthrough_PanicIfNotActive();
    ChainDestroy(kymeraBasicPassthrough_GetChain());
    kymeraBasicPassthrough_SetChain(NULL);
}

bool KymeraBasicPassthrough_IsActive(void)
{
    return kymeraBasicPassthrough_IsActive();
}

Sink KymeraBasicPassthrough_GetVoiceMicPathSink(void)
{
    kymeraBasicPassthrough_PanicIfNotActive();
    return ChainGetInput(kymeraBasicPassthrough_GetChain(), EPR_BPT_VOICE_IN);
}

Sink KymeraBasicPassthrough_GetBCMPathSink(void)
{
    kymeraBasicPassthrough_PanicIfNotActive();
    return ChainGetInput(kymeraBasicPassthrough_GetChain(), EPR_BPT_BCM_IN);
}

Sink KymeraBasicPassthrough_GetRefPathSink(void)
{
#ifndef ENABLE_UNIFIED_ANC_GRAPH
    kymeraBasicPassthrough_PanicIfNotActive();
#endif
    return ChainGetInput(kymeraBasicPassthrough_GetChain(), EPR_BPT_REF_IN);
}

Sink KymeraBasicPassthrough_GetFFMicPathSink(void)
{
#ifndef ENABLE_UNIFIED_ANC_GRAPH
    kymeraBasicPassthrough_PanicIfNotActive();
#endif
    return ChainGetInput(kymeraBasicPassthrough_GetChain(), EPR_BPT_FF_IN);
}

Sink KymeraBasicPassthrough_GetFBMicPathSink(void)
{
#ifndef ENABLE_UNIFIED_ANC_GRAPH
    kymeraBasicPassthrough_PanicIfNotActive();
#endif
    return ChainGetInput(kymeraBasicPassthrough_GetChain(), EPR_BPT_FB_IN);
}

Source KymeraBasicPassthrough_GetRefPathSource(void)
{
    kymeraBasicPassthrough_PanicIfNotActive();
    return ChainGetOutput(kymeraBasicPassthrough_GetChain(), EPR_BPT_REF_OUT);
}

Source KymeraBasicPassthrough_GetFFMicPathSource(void)
{
    kymeraBasicPassthrough_PanicIfNotActive();
    return ChainGetOutput(kymeraBasicPassthrough_GetChain(), EPR_BPT_FF_OUT);
}

Source KymeraBasicPassthrough_GetFBMicPathSource(void)
{
    kymeraBasicPassthrough_PanicIfNotActive();
    return ChainGetOutput(kymeraBasicPassthrough_GetChain(), EPR_BPT_FB_OUT);
}

Source KymeraBasicPassthrough_GetVoiceMicPathSource(void)
{
    kymeraBasicPassthrough_PanicIfNotActive();
    return ChainGetOutput(kymeraBasicPassthrough_GetChain(), EPR_BPT_VOICE_OUT);
}

Source KymeraBasicPassthrough_GetBCMPathSource(void)
{
    kymeraBasicPassthrough_PanicIfNotActive();
    return ChainGetOutput(kymeraBasicPassthrough_GetChain(), EPR_BPT_BCM_OUT);
}

#endif /* #if defined(ENABLE_ADAPTIVE_ANC)*/

