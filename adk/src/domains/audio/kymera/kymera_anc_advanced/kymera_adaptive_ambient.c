/*!
\copyright  Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_adaptive_ambient.c
\brief      Implementation of Adaptive Ambient mode of ANC
*/
#include "kymera_adaptive_ambient.h"
#include "kymera_ahm.h"
#include "kymera.h"
#include "kymera_anc_common.h"
#include "kymera_mic_if.h"
#include "kymera_setup.h"
#include "kymera_anc.h"
#include "anc_state_manager.h"
#include "operators.h"

#ifdef ENABLE_ADAPTIVE_ANC

#define MAX_GAIN_STEP                          (6)
#define MIN_GAIN_STEP                          (-6)

#define GAIN_DB_TO_Q2N_SF (178344656) // 2^30/6.0205999132796239042747778944899 or 2^32-2/20*log 2
#define GAIN_DB(x)      ((int32)(GAIN_DB_TO_Q2N_SF * (x)))
#define MAX_GAIN_DB     ((int32)(GAIN_DB_TO_Q2N_SF * (6)))
#define MIN_GAIN_DB     ((int32)(GAIN_DB_TO_Q2N_SF * (-6)))
#define GAIN_DB_TO_Q5N_SF (22293082) // 2^27/6.0205999132796239042747778944899 or 2^32-5/20*log 2
#define GAIN_Q2N_SF_TO_DB(x) (x/GAIN_DB_TO_Q2N_SF)

#define  COMPANDER_SAMPLE_RATE (16000U)

#define BYPASS_PARAM_FLAG_POS                (0)

#define ENABLE_BYPASS_PARAM_FLAG             (TRUE)
#define DISABLE_BYPASS_PARAM_FLAG            (FALSE)

#define CONFIG_MASK(pos)                     (1U << (pos))
#define CONFIG_FLAG(flag, pos)               ((flag) << (pos))

static void kymeraAdaptiveAmbient_ActivateSplitterSecondOutput(void);
static void kymeraAdaptiveAmbient_DeactivateSplitterSecondOutput(void);
static void kymeraAdaptiveAmbient_SetUcids(anc_mode_t anc_mode);
static void kymeraAdaptiveAmbient_ConfigureSplitter(void);

int32 gain_step = 0;

static kymera_chain_handle_t adaptive_ambient_chain;


static uint32 kymeraAdaptiveAmbient_GetConfigFlags(void)
{
    Operator adrc_op;
    uint32 compander_config = 0;

    if (KymeraAdaptiveAmbient_GetChain() != NULL)
    {
        adrc_op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_COMPANDER);
        if(adrc_op)
        {
            compander_config = OperatorsAncCompanderGetConfig(adrc_op);
        }
    }

    return compander_config;
}

static bool kymeraAdaptiveAmbient_SetConfigFlags(bool config_flag_value, unsigned config_flag_pos)
{
    Operator adrc_op;
    uint32 compander_config;
    bool status = FALSE;

    if (KymeraAdaptiveAmbient_GetChain() != NULL)
    {
        adrc_op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_COMPANDER);

        /* Read current AHM configs */
        compander_config = kymeraAdaptiveAmbient_GetConfigFlags();
        /* Reset flag corresponding to config flag position */
        compander_config = (compander_config & ~(CONFIG_MASK(config_flag_pos)));
        /* Update flag to flag value */
        compander_config |= CONFIG_FLAG(config_flag_value, config_flag_pos);

        if(adrc_op)
        {
            OperatorsAncCompanderSetConfig(adrc_op, compander_config);
            status = TRUE;
        }
    }

    return status;
}

static void kymeraAdaptiveAmbient_SetChain(kymera_chain_handle_t chain)
{
    adaptive_ambient_chain = chain;
}

static void kymeraAdaptiveAmbient_SetUcids(anc_mode_t current_anc_mode)
{
    /* Set UCIDs */
    KymeraAdaptiveAmbient_SetAncCompanderUcid(current_anc_mode); /*Each ANC mode corresponds to unique UCID*/
    KymeraAdaptiveAmbient_SetHowlingControlUcid(current_anc_mode);
}

static void kymeraAdaptiveAmbient_SetCompanderSampleRate(unsigned sample_rate)
{
    Operator op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_COMPANDER);

    if(op)
    {
        OperatorsStandardSetSampleRate(op,sample_rate);
    }
}

kymera_chain_handle_t KymeraAdaptiveAmbient_GetChain(void)
{
    return adaptive_ambient_chain;
}

bool KymeraAdaptiveAmbient_IsActive(void)
{
    return (KymeraAdaptiveAmbient_GetChain() != NULL);
}

Sink KymeraAdaptiveAmbient_GetFFMicPathSink(void)
{
    PanicNull(KymeraAdaptiveAmbient_GetChain());
    return ChainGetInput(KymeraAdaptiveAmbient_GetChain(),EPR_ADV_ANC_COMPANDER_IN);
}

void KymeraAdaptiveAmbient_CreateChain(void)
{
    DEBUG_LOG("KymeraAdaptiveAmbient_CreateChain");
    kymeraAdaptiveAmbient_SetChain(PanicNull(ChainCreate(Kymera_GetChainConfigs()->chain_adaptive_ambient_config)));
}


void KymeraAdaptiveAmbient_ConnectChain(void)
{
    ChainConnect(KymeraAdaptiveAmbient_GetChain());
    kymeraAdaptiveAmbient_ActivateSplitterSecondOutput();
}

void KymeraAdaptiveAmbient_DisconnectChain(void)
{
    kymeraAdaptiveAmbient_DeactivateSplitterSecondOutput();
    StreamDisconnect(ChainGetOutput(KymeraAdaptiveAmbient_GetChain(), EPR_ADV_ANC_COMPANDER_OUT),
                            NULL);
}

void KymeraAdaptiveAmbient_StopChain(void)
{
    ChainStop(KymeraAdaptiveAmbient_GetChain());
}

void KymeraAdaptiveAmbient_DestroyChain(void)
{
    PanicNull(KymeraAdaptiveAmbient_GetChain());
    ChainDestroy(KymeraAdaptiveAmbient_GetChain());
    kymeraAdaptiveAmbient_SetChain(NULL);
}

/*Link Compander to ANC HW Manager*/
static void KymeraAdaptiveAmbient_LinkCompanderToHwMgr(bool link)
{
    Operator op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_COMPANDER);

    if (op)
    {    
        OperatorsAncLinkHwManager(op, link, KymeraAhm_GetOperator());
    }
}

/*Link HC to ANC HW Manager*/
static void KymeraAdaptiveAmbient_LinkHcgrToHwMgr(bool link)
{
    Operator op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_HC);

    if (op)
    {
        OperatorsAncLinkHwManager(op, link, KymeraAhm_GetOperator());
    }
}

/*Link HCGR to ANC Compander for FF Fine gain */
static void KymeraAdaptiveAmbient_LinkHcgrFFFineGain(void)
{
    Operator op_hcgr = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_HC);
    Operator op_compander = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_COMPANDER);

    if (op_hcgr && op_compander)
    {
        DEBUG_LOG("KymeraAdaptiveAmbient_LinkHcgrFFFineGain");
        OperatorsAncLinkTargetFFFineGain(op_hcgr, op_compander);
    }
}

/* Update makeup gain to ramp up directly to world volume leakthrough gain */
static void kymeraAdaptiveAmbient_ConfigureMakeupGain(void)
{
    Operator op_compander = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_COMPANDER);
    int8 world_volume_gain_dB;
    int8 balance_percentage;
    if (op_compander)
    {
        DEBUG_LOG("kymeraAdaptiveAmbient_ConfigureMakeupGain");
        AncStateManager_GetCurrentWorldVolumeGain(&world_volume_gain_dB);
        balance_percentage = AncStateManager_GetBalancePercentage();
        /* Applies balance percentage to world volume gain and updates makeup gain to ADRC in fixed point format */
        KymeraAnc_UpdateWorldVolumeGain(world_volume_gain_dB, balance_percentage);
    }
}

void KymeraAdaptiveAmbient_ConfigureChain(const KYMERA_INTERNAL_AANC_ENABLE_T* param)
{
    kymeraAdaptiveAmbient_SetUcids(param->current_mode);
    kymeraAdaptiveAmbient_SetCompanderSampleRate(COMPANDER_SAMPLE_RATE);
    KymeraAdaptiveAmbient_LinkCompanderToHwMgr(TRUE);
    KymeraAdaptiveAmbient_LinkHcgrToHwMgr(TRUE);
    KymeraAdaptiveAmbient_LinkHcgrFFFineGain();
    kymeraAdaptiveAmbient_ConfigureSplitter();
    kymeraAdaptiveAmbient_ConfigureMakeupGain();
}

void KymeraAdaptiveAmbient_StartChain(void)
{
    ChainStart(KymeraAdaptiveAmbient_GetChain());
    DEBUG_LOG_ALWAYS("Adaptive Ambient graph started");
}

void KymeraAdaptiveAmbient_ApplyModeChange(const KYMERA_INTERNAL_AANC_ENABLE_T* param)
{
    kymeraAdaptiveAmbient_SetUcids(param->current_mode);
    kymeraAdaptiveAmbient_ConfigureMakeupGain();
}

/********************************************Splitter Configuration ********************************************/
static void kymeraAdaptiveAmbient_ConfigureSplitter(void)
{
    Operator op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_SPLITTER);
    if(op)
    {
        OperatorsSplitterSetWorkingMode(op, splitter_mode_clone_input);
        OperatorsSplitterEnableSecondOutput(op, FALSE);
        OperatorsSplitterSetDataFormat(op, operator_data_format_pcm);
    }
}

static void kymeraAdaptiveAmbient_ActivateSplitterSecondOutput(void)
{
    DEBUG_LOG("kymeraAdaptiveAmbient_ActivateSplitterSecondOutput");
    Operator op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_SPLITTER);

    if(op)
    {
        OperatorsSplitterEnableSecondOutput(op, TRUE);
    }
}

static void kymeraAdaptiveAmbient_DeactivateSplitterSecondOutput(void)
{
    DEBUG_LOG("kymeraAdaptiveAmbient_DeactivateSplitterSecondOutput");
    Operator op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_SPLITTER);

    if(op)
    {
        OperatorsSplitterEnableSecondOutput(op, FALSE);
    }
}

/********************************************Splitter Configuration ends********************************************/

/********************************************Howling control configuration******************************************/
void KymeraAdaptiveAmbient_SetHowlingControlUcid(unsigned ucid)
{
    Operator op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_HC);

    if(op)
    {
        OperatorsStandardSetUCID(op, ucid);
    }
}

void KymeraAdaptiveAmbient_SetHowlingControlSysMode(hc_sysmode_t mode)
{
    Operator op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_HC);

    if(op)
    {
        DEBUG_LOG_INFO("KymeraAdaptiveAmbient_SetHowlingControlSysMode, AAMB HCGR sysmode, enum:hc_sysmode_t:%d", mode);
        OperatorsHowlingControlSetSysmodeCtrl(op, mode);
    }
}

/********************************************Howling control configuration ends*************************************/

/********************************************Compander Control configuraiton****************************************/

void KymeraAdaptiveAmbient_StartAncCompanderAdaptation(void)
{
    KymeraAdaptiveAmbient_SetAncCompanderSysMode(anc_compander_sysmode_full);
}

void KymeraAdaptiveAmbient_SetAncCompanderSysMode(anc_compander_sysmode_t mode)
{
    Operator op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_COMPANDER);

    if(op)
    {
        DEBUG_LOG_INFO("KymeraAdaptiveAmbient_SetAncCompanderSysMode, AAMB sysmode, enum:anc_compander_sysmode_t:%d", mode);
        OperatorsAncCompanderSetSysmodeCtrl(op, mode);
    }
}

void KymeraAdaptiveAmbient_SetAncCompanderUcid(unsigned ucid)
{
    Operator op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_COMPANDER);

    if(op)
    {
        OperatorsStandardSetUCID(op, ucid);
    }
}

bool KymeraAdaptiveAmbient_GetCompanderGain(int32* makeup_gain_fixed_point)
{
    bool status = FALSE;
    Operator op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_COMPANDER);

    if(op)
    {
        *makeup_gain_fixed_point = (int32)OperatorsAncCompanderGetMakeUpGain(op);
        status = TRUE;
    }

    return status;
}

uint16 KymeraAdaptiveAmbient_GetCompanderAdjustedGain(void)
{
    uint16 gain=0;
    
    Operator op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_COMPANDER);
    
    if(op)
    {
        gain = OperatorsCompanderGetAdjustedGain(op);
    }
    
    DEBUG_LOG("KymeraAdaptiveAmbient_GetCompanderAdjustedGain %d", gain);
    return gain;
}

void KymeraAdaptiveAmbient_AncCompanderMakeupGainVolumeUp(void)
{
    DEBUG_LOG("KymeraAncAdvanced_CompanderMakeupGainVolumeUp");
    if(AncConfig_IsAncModeLeakThrough(AncStateManager_GetCurrentMode()))
    {
        DEBUG_LOG("KymeraAncAdvanced_CompanderMakeupGainVolumeUp ");
        Operator op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_COMPANDER);

        if(op)
        {
            if (gain_step > MAX_GAIN_STEP-1)
            {
                DEBUG_LOG("KymeraAncAdvanced_CompanderMakeupGainVolumeUp ");
                OperatorsAncCompanderSetMakeUpGain(op, MAX_GAIN_DB);
                gain_step = MAX_GAIN_STEP;
            }
            else
            {
                gain_step = gain_step + 1;
                OperatorsAncCompanderSetMakeUpGain(op, GAIN_DB(gain_step));
                DEBUG_LOG("KymeraAncAdvanced_CompanderMakeupGainVolumeUp");
            }
        }
    }
}

void KymeraAdaptiveAmbient_AncCompanderMakeupGainVolumeDown(void)
{
    DEBUG_LOG("KymeraAncAdvanced_CompanderMakeupGainVolumeDown");
    if(AncConfig_IsAncModeLeakThrough(AncStateManager_GetCurrentMode()))
    {
        Operator op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_COMPANDER);

        if(op)
        {
            if (gain_step < MIN_GAIN_STEP+1)
            {
                OperatorsAncCompanderSetMakeUpGain(op, MIN_GAIN_DB);
                gain_step = MIN_GAIN_STEP;
            }
            else
            {
                gain_step = gain_step - 1;
                OperatorsAncCompanderSetMakeUpGain(op, GAIN_DB(gain_step));
            }
        }
    }
}

bool KymeraAdaptiveAmbient_UpdateAncCompanderMakeupGain(int32 makeup_gain_fixed_point)
{
    Operator adrc_op = ChainGetOperatorByRole(KymeraAdaptiveAmbient_GetChain(), OPR_ADV_AANC_COMPANDER);
    bool status = FALSE;

    if(adrc_op)
    {
        OperatorsAncCompanderSetMakeUpGain(adrc_op, makeup_gain_fixed_point);
        status = TRUE;
    }

    return status;
}

bool KymeraAdaptiveAmbient_EnableBypassAncCompanderParam(void)
{
    return kymeraAdaptiveAmbient_SetConfigFlags(ENABLE_BYPASS_PARAM_FLAG, BYPASS_PARAM_FLAG_POS);
}

bool KymeraAdaptiveAmbient_DisableBypassAncCompanderParam(void)
{
    return kymeraAdaptiveAmbient_SetConfigFlags(DISABLE_BYPASS_PARAM_FLAG, BYPASS_PARAM_FLAG_POS);
}

bool KymeraAdaptiveAmbient_GetBypassAncCompanderParam(void)
{
    uint32 compander_config;
    bool bypass_param_flag = FALSE;

    if (KymeraAdaptiveAmbient_GetChain() != NULL)
    {
        /* Read current AHM configs */
        compander_config = kymeraAdaptiveAmbient_GetConfigFlags();
        if (compander_config & (CONFIG_MASK(BYPASS_PARAM_FLAG_POS)))
        {
            bypass_param_flag = TRUE;
        }
    }

    return bypass_param_flag;
}

/********************************************Compander Control configuraiton ends***********************************/
#endif /*ENABLE_ADAPTIVE_ANC*/
