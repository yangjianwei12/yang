/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       ama_eq.c
    \ingroup    ama
    \brief      Implementation of the EQ handling for Amazon AVS
*/

#ifdef INCLUDE_AMA

#include <logging.h>
#include "ama_eq.h"
#include "ama_state.h"
#include "voice_ui_eq.h"
#include "ama_send_command.h"

#define INVALID_GAIN    0xFFFF

static uint16 ama_eq_bass_gain      = INVALID_GAIN;
static uint16 ama_eq_mid_gain       = INVALID_GAIN;
static uint16 ama_eq_treble_gain    = INVALID_GAIN;

bool Ama_EqInit(void)
{
    DEBUG_LOG("Ama_EqInit");
    return TRUE;
}

void Ama_EqUpdate(void)
{
    uint16 bass_gain = (uint16) VoiceUi_GetLowEqGain();
    uint16 mid_gain = (uint16) VoiceUi_GetMidEqGain();
    uint16 treble_gain = (uint16) VoiceUi_GetHighEqGain();
    DEBUG_LOG_VERBOSE("Ama_EqUpdate: bass:%u, mid:%u, treble:%u", bass_gain, mid_gain, treble_gain);
    /* Only send COMMAND__SYNCHRONIZE_STATE for the features that have changed */
    if (bass_gain != ama_eq_bass_gain)
    {
        AmaSendCommand_SyncState(AMA_FEATURE_EQUALIZER_BASS, STATE__VALUE_INTEGER, bass_gain);
        ama_eq_bass_gain = bass_gain;
    }
    if (mid_gain != ama_eq_mid_gain)
    {
        AmaSendCommand_SyncState(AMA_FEATURE_EQUALIZER_MID, STATE__VALUE_INTEGER, mid_gain);
        ama_eq_mid_gain = mid_gain;
    }
    if (treble_gain != ama_eq_treble_gain)
    {
        AmaSendCommand_SyncState(AMA_FEATURE_EQUALIZER_TREBLE, STATE__VALUE_INTEGER, treble_gain);
        ama_eq_treble_gain = treble_gain;
    }
}

uint32 Ama_EqGetEqualizerBass(void)
{
    uint32 bass_gain = (uint32) VoiceUi_GetLowEqGain();
    DEBUG_LOG_VERBOSE("Ama_EqGetEqualizerBass: %lu", bass_gain);
    return bass_gain;
}

uint32 Ama_EqGetEqualizerMid(void)
{
    uint32 mid_gain = (uint32) VoiceUi_GetMidEqGain();
    DEBUG_LOG_VERBOSE("Ama_EqGetEqualizerMid: %lu", mid_gain);
    return mid_gain;
}

uint32 Ama_EqGetEqualizerTreble(void)
{
    uint32 treble_gain = (uint32) VoiceUi_GetHighEqGain();
    DEBUG_LOG_VERBOSE("Ama_EqGetEqualizerTreble: %lu", treble_gain);
    return treble_gain;
}

void Ama_EqSetEqualizerBass(uint32 bass_gain)
{
    UNUSED(bass_gain);    /* In case the following are macro-ed out */
    DEBUG_LOG_VERBOSE("Ama_EqSetEqualizerBass: %lu", bass_gain);
    VoiceUi_SetLowEqGain(bass_gain);
}

void Ama_EqSetEqualizerMid(uint32 mid_gain)
{
    UNUSED(mid_gain);    /* In case the following are macro-ed out */
    DEBUG_LOG_VERBOSE("Ama_EqSetEqualizerMid: %lu", mid_gain);
    VoiceUi_SetMidEqGain(mid_gain);
}

void Ama_EqSetEqualizerTreble(uint32 treble_gain)
{
    UNUSED(treble_gain);    /* In case the following are macro-ed out */
    DEBUG_LOG_VERBOSE("Ama_EqSetEqualizerTreble: %lu", treble_gain);
    VoiceUi_SetHighEqGain(treble_gain);
}

#endif /* INCLUDE_AMA */

