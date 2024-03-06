/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      This module resolves tie breaks between sources
*/

#include "focus_select.h"
#include "focus_select_audio.h"
#include "focus_select_config.h"
#include "focus_select_tie_break.h"

#include <audio_router.h>
#include <device_list.h>
#include <device_properties.h>
#include <logging.h>
#include <panic.h>
#include <voice_sources.h>


static const focus_select_audio_tie_break_t audio_tie_break_default[] = 
{
    FOCUS_SELECT_AUDIO_A2DP,
    FOCUS_SELECT_AUDIO_USB,
    FOCUS_SELECT_AUDIO_LINE_IN,
    FOCUS_SELECT_AUDIO_LEA_UNICAST,
    FOCUS_SELECT_AUDIO_LEA_BROADCAST
};

COMPILE_TIME_ASSERT(ARRAY_DIM(audio_tie_break_default) == FOCUS_SELECT_AUDIO_MAX_SOURCES,
                    FOCUS_SELECT_invalid_size_audio_tie_break_default);

static const focus_select_voice_tie_break_t voice_tie_break_default[] = 
{
    FOCUS_SELECT_VOICE_HFP,
    FOCUS_SELECT_VOICE_USB,
    FOCUS_SELECT_VOICE_LEA_UNICAST
};

COMPILE_TIME_ASSERT(ARRAY_DIM(voice_tie_break_default) == FOCUS_SELECT_VOICE_MAX_SOURCES,
                    FOCUS_SELECT_invalid_size_voice_tie_break_default);

static const focus_select_audio_tie_break_t * audio_source_tie_break_ordering = NULL;
static const focus_select_voice_tie_break_t * voice_source_tie_break_ordering = NULL;

void FocusSelect_TieBreakInit(void)
{
    audio_source_tie_break_ordering = audio_tie_break_default;
    voice_source_tie_break_ordering = voice_tie_break_default;
}

static audio_source_t focusSelect_ConvertAudioTieBreakToSource(focus_status_t * focus_status, focus_select_audio_tie_break_t prio)
{
    audio_source_t source = audio_source_none;
    switch(prio)
    {
    case FOCUS_SELECT_AUDIO_LINE_IN:
        source = audio_source_line_in;
        break;
    case FOCUS_SELECT_AUDIO_USB:
        source = audio_source_usb;
        break;
    case FOCUS_SELECT_AUDIO_A2DP:
        {
            generic_source_t generic_a2dp_1 = {.type = source_type_audio, .u = {.audio = audio_source_a2dp_1}};
            generic_source_t generic_a2dp_2 = {.type = source_type_audio, .u = {.audio = audio_source_a2dp_2}};

            bool highest_priority_is_a2dp_1 = FocusSelect_IsSourceContextHighestPriority(focus_status, generic_a2dp_1);
            bool highest_priority_is_a2dp_2 = FocusSelect_IsSourceContextHighestPriority(focus_status, generic_a2dp_2);

            if(highest_priority_is_a2dp_1 && highest_priority_is_a2dp_2)
            {
                device_list_mru_index_t mru_index_a2dp_1 = DeviceList_GetMruIndex(AudioSources_GetAudioSourceDevice(audio_source_a2dp_1));
                device_list_mru_index_t mru_index_a2dp_2 = DeviceList_GetMruIndex(AudioSources_GetAudioSourceDevice(audio_source_a2dp_2));

                DEBUG_LOG("focusSelect_ConvertAudioTieBreakToSource: Both A2DP sources has higher priority mru_index_a2dp_1:%d  mru_index_a2dp_2:%d",
                           mru_index_a2dp_1, mru_index_a2dp_2);
                if(focus_select_config.media_barge_in_enabled)
                {
                    /* Use MRU to decide voice_source, if we are tie breaking between two A2DP sources. */
                    source = ((mru_index_a2dp_1 <= mru_index_a2dp_2) ? audio_source_a2dp_1 :audio_source_a2dp_2);
                }
                else
                {
                    audio_source_t  last_routed_audio = AudioRouter_GetLastRoutedAudio();

                    if(last_routed_audio == audio_source_a2dp_1 || last_routed_audio == audio_source_a2dp_2)
                    {
                        source = last_routed_audio;
                    }
                    else
                    {
                        source = ((mru_index_a2dp_1 >= mru_index_a2dp_2) ? audio_source_a2dp_1 :audio_source_a2dp_2);
                    }
                }
            }
            else if(highest_priority_is_a2dp_1)
            {
                source = audio_source_a2dp_1;
            }
            else if(highest_priority_is_a2dp_2)
            {
                source = audio_source_a2dp_2;
            }
            else
            {
                /* A2DP is not available or not a tie break source, skip. */
            }
            DEBUG_LOG("focusSelect_ConvertAudioTieBreakToSource: FOCUS_SELECT_AUDIO_A2DP enum:audio_source_t:%d enum:audio_source_provider_context_t:%d",
                       source, focus_status->highest_priority_context);
        }
        break;
    case FOCUS_SELECT_AUDIO_LEA_UNICAST:
        source = audio_source_le_audio_unicast_1;
        break;
    case FOCUS_SELECT_AUDIO_LEA_BROADCAST:
        source = audio_source_le_audio_broadcast;
        break;
    default:
        break;
    }
    return source;
}

void FocusSelect_HandleTieBreak(focus_status_t * focus_status)
{
    audio_source_t  last_routed_audio = AudioRouter_GetLastRoutedAudio();
    generic_source_t curr_source = {.type=source_type_audio, .u.audio=last_routed_audio};

    /* Nothing to be done if all audio sources are disconnected or there is no need to tie break */
    if (focus_status->highest_priority_context == context_audio_disconnected ||
        focus_status->num_highest_priority_sources == 1)
    {
        return;
    }

    /* Firstly, use the last routed audio source, if it is in tie break and it's context is context_audio_connected */
    if (last_routed_audio != audio_source_none && last_routed_audio < max_audio_sources &&
            FocusSelect_IsAudioSourceContextConnected(focus_status, last_routed_audio) &&
            FocusSelect_IsSourceContextHighestPriority(focus_status, curr_source))
    {
        DEBUG_LOG_VERBOSE("FocusSelect_HandleTieBreak last routed audio enum:audio_source_t:%d enum:audio_source_provider_context_t:%d",
                          last_routed_audio, focus_status->highest_priority_context);

        focus_status->highest_priority_source.type = source_type_audio;
        focus_status->highest_priority_source.u.audio = last_routed_audio;
    }
    /* Otherwise, run through the prioritisation of audio sources and select the highest */
    else if(focus_select_config.transport_based_ordering_enabled)
    {
        PanicNull((void*)audio_source_tie_break_ordering);

        /* Tie break using the Application specified priority. */
        for (int i=0; i<FOCUS_SELECT_AUDIO_MAX_SOURCES; i++)
        {
            curr_source.u.audio = focusSelect_ConvertAudioTieBreakToSource(focus_status, audio_source_tie_break_ordering[i]);

            if (curr_source.u.audio != audio_source_none && FocusSelect_IsSourceContextHighestPriority(focus_status, curr_source))
            {
                focus_status->highest_priority_source = curr_source;
                break;
            }
        }
        DEBUG_LOG("FocusSelect_HandleTieBreak: Based on Transport Type, enum:audio_source_t:%d enum:audio_source_provider_context_t:%d",
                  focus_status->highest_priority_source.u.audio,
                  focus_status->highest_priority_context);
    }
    else
    {
        /* Tie break using MRU index of source device */
        audio_source_t audio_source = audio_source_none+1;
        device_t device = NULL;
        device_list_mru_index_t mru_index, highest_priority_mru_index = DEVICE_LIST_MRU_INDEX_NOT_SET;
        for (; audio_source<max_audio_sources; audio_source++)
        {
            curr_source.u.audio = audio_source;
            if (FocusSelect_IsSourceContextHighestPriority(focus_status, curr_source))
            {
                bool is_mru_source_update_required = FALSE;
                device = AudioSources_GetAudioSourceDevice(curr_source.u.audio);
                mru_index = DeviceList_GetMruIndex(device);
                DEBUG_LOG("FocusSelect_HandleTieBreak enum:audio_source_t:%d mru_index:%d", curr_source.u.audio, mru_index);
                if(mru_index != DEVICE_LIST_MRU_INDEX_NOT_SET)
                {
                    is_mru_source_update_required = (focus_select_config.media_barge_in_enabled ? mru_index < highest_priority_mru_index : mru_index > highest_priority_mru_index);

                    if(is_mru_source_update_required || (highest_priority_mru_index == DEVICE_LIST_MRU_INDEX_NOT_SET))
                    {
                        focus_status->highest_priority_source = curr_source;
                        highest_priority_mru_index = mru_index;
                    }
                }
            }
        }

        DEBUG_LOG("FocusSelect_HandleTieBreak: Based on MRU Index, enum:audio_source_t:%d enum:audio_source_provider_context_t:%d enum:device_list_mru_index_t:%d",
                  focus_status->highest_priority_source.u.audio, focus_status->highest_priority_context, highest_priority_mru_index);
        PanicFalse(highest_priority_mru_index != DEVICE_LIST_MRU_INDEX_NOT_SET);
    }

}

static voice_source_t focusSelect_ConvertVoiceTieBreakToSource(focus_status_t * focus_status, focus_select_voice_tie_break_t prio)
{
    voice_source_t source = voice_source_none;
    switch(prio)
    {
    case FOCUS_SELECT_VOICE_USB:
        source = voice_source_usb;
        break;
    case FOCUS_SELECT_VOICE_LEA_UNICAST:
        source = voice_source_le_audio_unicast_1;
        break;
    case FOCUS_SELECT_VOICE_HFP:
        {
            generic_source_t generic_hfp_1 = {.type = source_type_voice, .u = {.voice = voice_source_hfp_1}};
            generic_source_t generic_hfp_2 = {.type = source_type_voice, .u = {.voice = voice_source_hfp_2}};
            
            bool highest_priority_is_hfp_1 = FocusSelect_IsSourceContextHighestPriority(focus_status, generic_hfp_1);
            bool highest_priority_is_hfp_2 = FocusSelect_IsSourceContextHighestPriority(focus_status, generic_hfp_2);

            if(highest_priority_is_hfp_1 && highest_priority_is_hfp_2)
            {
                /* Use MRU to decide voice_source, if we are tie breaking between two HFP sources. */
                device_list_mru_index_t mru_index_hfp_1 = DeviceList_GetMruIndex(VoiceSources_GetDeviceForSource(voice_source_hfp_1));
                device_list_mru_index_t mru_index_hfp_2 = DeviceList_GetMruIndex(VoiceSources_GetDeviceForSource(voice_source_hfp_2));

                DEBUG_LOG("focusSelect_ConvertVoiceTieBreakToSource: Both HFP sources has higher priority mru_index_hfp_1:%d  mru_index_hfp_2:%d",
                           mru_index_hfp_1, mru_index_hfp_2);
                source = ((mru_index_hfp_1 < mru_index_hfp_2) ? voice_source_hfp_1 :voice_source_hfp_2);
            }
            else if(highest_priority_is_hfp_1)
            {
                source = voice_source_hfp_1;
            }
            else if(highest_priority_is_hfp_2)
            {
                source = voice_source_hfp_2;
            }
            else
            {
                /* HFP is not available or not a tie break source, skip. */
            }
            DEBUG_LOG("focusSelect_ConvertVoiceTieBreakToSource: FOCUS_SELECT_VOICE_HFP enum:voice_source_t:%d enum:voice_source_provider_context_t:%d",
                       source, focus_status->highest_priority_context);
        }
        break;
    default:
        break;
    }
    return source;
}

void FocusSelect_HandleVoiceTieBreak(focus_status_t * focus_status)
{
    generic_source_t curr_source = {.type = source_type_voice, .u.voice = voice_source_none};
    /* Nothing to be done if all voice sources are disconnected or there is no need to tie break */
    if (focus_status->highest_priority_context == context_voice_disconnected ||
        focus_status->num_highest_priority_sources == 1)
    {
        return;
    }

    /* Run through the prioritisation of voice sources and select the highest */
    if(focus_select_config.transport_based_ordering_enabled)
    {
        PanicNull((void*)voice_source_tie_break_ordering);
        /* Tie break using the Application specified priority. */
        for (int i=0; i<FOCUS_SELECT_VOICE_MAX_SOURCES; i++)
        {
            curr_source.u.voice = focusSelect_ConvertVoiceTieBreakToSource(focus_status, voice_source_tie_break_ordering[i]);
            if (curr_source.u.voice != voice_source_none && FocusSelect_IsSourceContextHighestPriority(focus_status, curr_source))
            {
                focus_status->highest_priority_source = curr_source;
                break;
            }
        }
        DEBUG_LOG_VERBOSE("FocusSelect_HandleVoiceTieBreak selected Based on Transport Type, enum:voice_source_t:%d  enum:voice_source_provider_context_t:%d",
                          focus_status->highest_priority_source.u.voice,
                          focus_status->highest_priority_context);
    }
    else
    {
        /* Tie break using MRU index of source device */
        voice_source_t voice_source = voice_source_none+1;
        device_t device = NULL;
        device_list_mru_index_t mru_index, highest_priority_mru_index = DEVICE_LIST_MRU_INDEX_NOT_SET;

        for (; voice_source<max_voice_sources; voice_source++)
        {
            curr_source.u.voice = voice_source;
            if (FocusSelect_IsSourceContextHighestPriority(focus_status, curr_source))
            {
                device = VoiceSources_GetDeviceForSource(curr_source.u.voice);
                mru_index = DeviceList_GetMruIndex(device);
                DEBUG_LOG("FocusSelect_HandleVoiceTieBreak enum:voice_source_t:%d mru_index:%d", curr_source.u.voice, mru_index);
                if(mru_index < highest_priority_mru_index)
                {
                    focus_status->highest_priority_source = curr_source;
                    highest_priority_mru_index = mru_index;
                }
            }
        }
        DEBUG_LOG_VERBOSE("FocusSelect_HandleVoiceTieBreak selected Based on MRU Index, enum:voice_source_t:%d  enum:voice_source_provider_context_t:%d",
                          focus_status->highest_priority_source.u.voice,
                          focus_status->highest_priority_context);
    }

}

void FocusSelect_ConfigureAudioSourceTieBreakOrder(const focus_select_audio_tie_break_t tie_break_prio[FOCUS_SELECT_AUDIO_MAX_SOURCES])
{
    audio_source_tie_break_ordering = tie_break_prio;
}

void FocusSelect_ConfigureVoiceSourceTieBreakOrder(const focus_select_voice_tie_break_t tie_break_prio[FOCUS_SELECT_VOICE_MAX_SOURCES])
{
    voice_source_tie_break_ordering = tie_break_prio;
}


