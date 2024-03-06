/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle functions common to LE Audio
*/

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)

#include "kymera_le_common.h"
#include "kymera_state.h"
#include "kymera_le_mic_chain.h"
#include "kymera_le_voice.h"
#include "kymera_config.h"
#include "kymera_aec.h"
#include "kymera_mic_if.h"

bool kymeraLeAudio_IsMaxDspClockRequired(void)
{
    bool max_clock_reqd = FALSE;
    appKymeraState state = appKymeraGetState();
    bool is_stereo_device = Multidevice_IsDeviceStereo();

#ifdef INCLUDE_CIS_MIRRORING
    /* For stereo earbud config the clock speed should be set to max */
    is_stereo_device = TRUE;
#endif

    /* Boost audio clock to 240MHz when required */
    if (appKymeraInConcurrency())
    {
        max_clock_reqd = TRUE;
    }
    else if (state == KYMERA_STATE_LE_VOICE_ACTIVE)
    {
        if (is_stereo_device ||
            appConfigVoiceGetNumberOfMics() > 1 ||
            Kymera_IsLeVoiceSplitStereoChain() || Kymera_AecGetTaskPeriod() == AEC_REF_TASK_PERIOD_1MS)
        {
            max_clock_reqd = TRUE;
        }
    }
    else if (state == KYMERA_STATE_LE_AUDIO_ACTIVE && Kymera_IsVoiceBackChannelEnabled())
    {
        if (is_stereo_device || Kymera_LeMicSampleRate() > 16000 || Kymera_LeMicIsCodecTypeAptxLite() 
            || Kymera_AecGetTaskPeriod() == AEC_REF_TASK_PERIOD_1MS)
        {
            max_clock_reqd = TRUE;
        }
    }

    return max_clock_reqd;
}

void KymeraLeAudioVoice_SetMicMuteState(bool mute)
{
    appKymeraState state = appKymeraGetState();
    Operator op = INVALID_OPERATOR;
    mic_users_t mic_users = Kymera_MicGetActiveUsers();

    if (mic_users & (mic_user_le_mic | mic_user_le_voice))
    {
        if (state == KYMERA_STATE_LE_AUDIO_ACTIVE && (op = Kymera_GetLeMicCvcSend()) != INVALID_OPERATOR)
        {
            /* Mute via CVC SEND operator */
            OperatorsStandardSetControl(op, cvc_send_mute_control, mute);
            op = INVALID_OPERATOR;
        }
        else if (state == KYMERA_STATE_LE_AUDIO_ACTIVE || state == KYMERA_STATE_LE_VOICE_ACTIVE)
        {
            op = Kymera_GetAecOperator();
        }

        if (op != INVALID_OPERATOR)
        {
            OperatorsAecMuteMicOutput(op, mute);
        }
    }
}
#endif
