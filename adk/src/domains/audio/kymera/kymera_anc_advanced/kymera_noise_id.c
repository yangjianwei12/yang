
/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version
\file      kymera_noise_id.c
\brief     Implementation of Kymera Audio Chain related functionality for NoiseID
*/

#include "kymera_noise_id.h"
#include "kymera_anc_common.h"
#include "kymera_data.h"
#include "kymera_ucid.h"
#include "kymera_setup.h"
#include "kymera_internal_msg_ids.h"
#include "anc_state_manager.h"

#if defined(ENABLE_ADAPTIVE_ANC)

static kymera_chain_handle_t noise_id_chain;

#define kymeraNoiseID_IsActive() (KymeraNoiseID_GetChain() != NULL)
#define kymeraNoiseID_PanicIfNotActive()  if(!kymeraNoiseID_IsActive()) \
                                                                Panic()
#define kymeraNoiseID_PanicIfActive()  if(kymeraNoiseID_IsActive()) \
                                                                Panic()
#define NUM_STATUS_VAR 5
#define NOISE_CATEGORY_STATUS_OFFSET 2

kymera_chain_handle_t KymeraNoiseID_GetChain(void)
{
    return noise_id_chain;
}

static Operator KymeraNoiseID_GetNIDOperator(void)
{
    return ChainGetOperatorByRole(KymeraNoiseID_GetChain(), OPR_NOISE_ID);
}

static void kymeraNoiseID_SetChain(kymera_chain_handle_t chain)
{
    noise_id_chain = chain;
}

static void kymeraNoiseID_SetUcid(void)
{
    PanicFalse(Kymera_SetOperatorUcid(KymeraNoiseID_GetChain(), OPR_NOISE_ID, UCID_NOISE_ID));
}

void KymeraNoiseID_Create(void)
{
    DEBUG_LOG("KymeraNoiseID_Create");

    kymeraNoiseID_PanicIfActive();
    kymeraNoiseID_SetChain(PanicNull(ChainCreate(Kymera_GetChainConfigs()->chain_noise_id_config)));
}

void KymeraNoiseID_SetSysMode(noise_id_sysmode_t mode)
{
    Operator op = KymeraNoiseID_GetNIDOperator();

    if(op)
    {
        DEBUG_LOG("KymeraNoiseID_SetSysMode: enum:noise_id_sysmode_t:%d", mode);
        OperatorsNoiseIDSetSysmodeCtrl(op, mode);
    }
}

void KymeraNoiseID_SetCurrentCategory(noise_id_category_t category)
{
    Operator op = KymeraNoiseID_GetNIDOperator();

    if(op)
    {
        DEBUG_LOG("KymeraNoiseID_SetCurrentCategory");
        OperatorsNoiseIDSetCategory(op, category);
    }
}

void KymeraNoiseID_SetCategoryBasedOnCurrentMode(void)
{
    if (KymeraNoiseID_IsActive())
    {
        Operator op = KymeraNoiseID_GetNIDOperator();

        if (op)
        {
            anc_mode_t anc_mode = AncStateManager_GetCurrentMode();

            DEBUG_LOG("KymeraNoiseID_SetCategoryBasedOnCurrentMode anc mode enum:anc_mode_t:%d", anc_mode);

            if (AncConfig_IsNoiseIdSupportedForMode(anc_mode))
            {
                anc_noise_id_category_t category = AncConfig_GetNoiseIdCategoryForMode(anc_mode);
                DEBUG_LOG("KymeraNoiseID_SetCategoryBasedOnCurrentMode enum:anc_noise_id_category_t:%d", category);

                if (category != ANC_NOISE_ID_CATEGORY_NA)
                {
                    KymeraNoiseID_SetCurrentCategory((noise_id_category_t)category);
                }
            }
        }
    }
}

void KymeraNoiseID_Configure(void)
{
    DEBUG_LOG("KymeraNoiseID_Configure");
    if (KymeraNoiseID_IsActive())
    {
        kymeraTaskData *theKymera = KymeraGetTaskData();
        Operator op = KymeraNoiseID_GetNIDOperator();

        if (op)
        {
            /* UCID configuration */
            kymeraNoiseID_SetUcid();

            /* Set Noise ID at Standby until start */
            KymeraNoiseID_SetSysMode(noise_id_sysmode_standby);

            /*Set default category based on the current mode*/
            KymeraNoiseID_SetCategoryBasedOnCurrentMode();

            /* Associate kymera task to receive unsolicited messages */
            MessageOperatorTask(op, &theKymera->task);
        }
    }
}

void KymeraNoiseID_Connect(void)
{
    if (KymeraNoiseID_IsActive())
    {
        DEBUG_LOG("KymeraNoiseID_Connect");
        ChainConnect(KymeraNoiseID_GetChain());
    }
}

void KymeraNoiseID_Disconnect(void)
{
    if (KymeraNoiseID_IsActive())
    {
        DEBUG_LOG("KymeraNoiseID_Disconnect");
#ifdef ENABLE_UNIFIED_ANC_GRAPH
        StreamDisconnect(ChainGetOutput(KymeraNoiseID_GetChain(), EPR_NOISE_ID_OUT), NULL);
#else
        StreamDisconnect(NULL, ChainGetInput(KymeraNoiseID_GetChain(), EPR_NOISE_ID_IN));
#endif
    }
}

Sink KymeraNoiseID_GetFFMicPathSink(void)
{
    kymeraNoiseID_PanicIfNotActive();
    return ChainGetInput(KymeraNoiseID_GetChain(), EPR_NOISE_ID_IN);
}

Source KymeraNoiseID_GetFFMicPathSource(void)
{
    kymeraNoiseID_PanicIfNotActive();
    return ChainGetOutput(KymeraNoiseID_GetChain(), EPR_NOISE_ID_OUT);
}

get_status_data_t* KymeraNoiseID_GetStatusData(void)
{
    if (KymeraNoiseID_IsActive())
    {
        Operator op = ChainGetOperatorByRole(KymeraNoiseID_GetChain(), OPR_NOISE_ID);
        get_status_data_t* get_status = OperatorsCreateGetStatusData(NUM_STATUS_VAR);
        OperatorsGetStatus(op, get_status);
        return get_status;
    }

    return NULL;
}

void KymeraNoiseID_GetCurrentNoiseCategory(noise_id_category_t *category)
{
    get_status_data_t* get_status = KymeraNoiseID_GetStatusData();

    if(get_status)
    {
        *category = (noise_id_category_t)(get_status->value[NOISE_CATEGORY_STATUS_OFFSET]);
    }
    else
    {
        *category = (noise_id_category_t)ANC_NOISE_ID_CATEGORY_NA;
    }
    free(get_status);
}

void KymeraNoiseID_Start(void)
{
    if (KymeraNoiseID_IsActive())
    {
        DEBUG_LOG("KymeraNoiseID_Start");
        ChainStart(KymeraNoiseID_GetChain());
    }
}

void KymeraNoiseID_Stop(void)
{
    if (KymeraNoiseID_IsActive())
    {
        DEBUG_LOG("KymeraNoiseID_Stop");
        ChainStop(KymeraNoiseID_GetChain());
    }
}

void KymeraNoiseID_Enable(void)
{
    if (KymeraNoiseID_IsActive())
    {
        DEBUG_LOG("KymeraNoiseID_Enable");
        KymeraNoiseID_SetSysMode(noise_id_sysmode_full);
    }
}

void KymeraNoiseID_Disable(void)
{
    if (KymeraNoiseID_IsActive())
    {
        DEBUG_LOG("KymeraNoiseID_Disable");
        KymeraNoiseID_SetSysMode(noise_id_sysmode_standby);
    }
}

void KymeraNoiseID_Destroy(void)
{
    if (KymeraNoiseID_IsActive())
    {
        DEBUG_LOG("KymeraNoiseID_Destroy");
        ChainDestroy(KymeraNoiseID_GetChain());
        kymeraNoiseID_SetChain(NULL);
    }
}

bool KymeraNoiseID_IsActive(void)
{
    return kymeraNoiseID_IsActive();
}

#endif

