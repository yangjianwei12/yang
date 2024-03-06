/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      This module is an implementation of the focus interface for audio routing
*/

#include "focus_select.h"
#include "focus_select_audio.h"
#include "focus_select_status.h"
#include "focus_select_tie_break.h"
#include "focus_voice_source.h"

#include <audio_sources.h>
#include <bt_device.h>
#include <connection_manager.h>
#include <device_list.h>
#include <device_properties.h>
#include <focus_generic_source.h>
#include <logging.h>
#include <panic.h>
#include <voice_sources.h>
#include <ui.h>

/* Note: the lower bits in these masks are to accomodate the integer priority of the source */
#define SOURCE_TYPE_HAS_PRIORITY    0x01
#define VOICE_SOURCE_HAS_AUDIO            0x40

/* Note: Audio source which has connected context but has audio should get priority
 * between priority of context_audio_is_paused and context_audio_is_broadcast */
#define AUDIO_SOURCE_PRIORITY_WHEN_CONNECTED_AND_HAS_AUDIO 0x03

static const bool voice_sources_have_priority = TRUE;

/* Look-up table mapping the audio_context into a priority suitable for comparison
   with voice sources priorities for determining which source should have focus
   for audio routing. 0 is the lowest priority. */
static int8 audio_context_to_audio_prio_mapping[] = {
    [context_audio_disconnected] = 0,
    [context_audio_connected] = 1,
    [context_audio_is_paused] = 2,
    [context_audio_is_broadcast] = 4,
    [context_audio_is_streaming] = 5,
    [context_audio_is_playing] = 6,
    [context_audio_is_high_priority] = 7,
    [context_audio_is_va_response] = 8,
};
COMPILE_TIME_ASSERT(ARRAY_DIM(audio_context_to_audio_prio_mapping) == max_audio_contexts,
                    FOCUS_SELECT_invalid_size_audio_context_to_audio_prio_mapping_table);

/* Look-up table mapping the voice_context symbol to the relative priority of
   that context in determining focus. This table considers priorities for audio
   routing purposes. 0 is the lowest priority. */
static int8 voice_context_to_audio_prio_mapping[] = {
    [context_voice_disconnected] = 0, 
    [context_voice_connected] = 1,
    [context_voice_ringing_incoming] = 8,
    [context_voice_call_held] = 8,
    [context_voice_ringing_outgoing] = 9,
    [context_voice_in_call] = 9,
    [context_voice_in_call_with_incoming] = 9,
    [context_voice_in_call_with_outgoing] = 9,
    [context_voice_in_call_with_held] = 9,
    [context_voice_in_multiparty_call] = 9,
};
COMPILE_TIME_ASSERT(ARRAY_DIM(voice_context_to_audio_prio_mapping) == max_voice_contexts,
                    FOCUS_SELECT_invalid_size_audio_prio_mapping_table);

static source_cache_data_t * focusSelect_CalculatePriorityForAudio(focus_status_t * focus_status, generic_source_t curr_source)
{
    uint8 source_priority = 0;
    bool source_has_audio = FALSE;
    unsigned source_context = BAD_CONTEXT;

    if (GenericSource_IsVoice(curr_source))
    {
        source_has_audio = VoiceSources_IsVoiceChannelAvailable(curr_source.u.voice);
        source_context = VoiceSources_GetSourceContext(curr_source.u.voice);

        source_priority = voice_context_to_audio_prio_mapping[source_context];
        source_priority |= (source_has_audio ? VOICE_SOURCE_HAS_AUDIO : 0x0);
        source_priority <<= 1;
        source_priority |= (voice_sources_have_priority ? SOURCE_TYPE_HAS_PRIORITY : 0x0);
    }
    else if (GenericSource_IsAudio(curr_source))
    {
        source_context = AudioSources_GetSourceContext(curr_source.u.audio);
		source_has_audio = AudioSources_IsAudioAvailable(curr_source.u.audio);

        if(source_context == context_audio_connected && source_has_audio)
        {
            source_priority = AUDIO_SOURCE_PRIORITY_WHEN_CONNECTED_AND_HAS_AUDIO;
        }
        else
        {
            source_priority = audio_context_to_audio_prio_mapping[source_context];
        }

        source_priority <<= 1;
        source_priority |= (!voice_sources_have_priority ? SOURCE_TYPE_HAS_PRIORITY : 0x0);
    }

    PanicFalse(source_context != BAD_CONTEXT);

    return FocusSelect_SetCacheDataForSource(focus_status, curr_source, source_context, source_has_audio, source_priority);
}

static bool focusSelect_VoiceSourceIsActive(focus_status_t * focus_status)
{
    bool is_active = FALSE;
    if (GenericSource_IsVoice(focus_status->highest_priority_source))
    {
        switch (focus_status->highest_priority_context)
        {
            case context_voice_disconnected:
            break;
            
            case context_voice_connected:
                is_active = VoiceSources_IsVoiceChannelAvailable(focus_status->highest_priority_source.u.voice);
            break;

            case max_voice_contexts:
            Panic();
            break;

            default:
            is_active = TRUE;
            break;

        }
    }
    return is_active;
}

static bool focusSelect_AudioSourceIsActive(focus_status_t * focus_status)
{
    bool is_active = FALSE;
    if (GenericSource_IsAudio(focus_status->highest_priority_source))
    {
        switch (focus_status->highest_priority_context)
        {
            case context_audio_disconnected:
			break;
            case context_audio_connected:
				is_active = AudioSources_IsAudioAvailable(focus_status->highest_priority_source.u.audio);
            break;
            case max_audio_contexts:
            Panic();
            break;

            default:
            is_active = TRUE;
            break;

        }
    }
    return is_active;
}

generic_source_t FocusSelect_GetFocusedSourceForAudioRouting(void)
{
    focus_status_t focus_status_struct = {0};
    focus_status_t * focus_status = &focus_status_struct;

    sources_iterator_t iter = SourcesIterator_Create(source_type_max);
    FocusSelect_CompileFocusStatus(iter, focus_status, focusSelect_CalculatePriorityForAudio);
    SourcesIterator_Destroy(iter);

    if (focus_status->num_highest_priority_sources != 1)
    {
        if (GenericSource_IsVoice(focus_status->highest_priority_source))
        {
            FocusSelect_HandleVoiceTieBreak(focus_status);
        }
        else if (GenericSource_IsAudio(focus_status->highest_priority_source))
        {
            FocusSelect_HandleTieBreak(focus_status);
        }
    }

    bool is_focused_source_active_for_audio_activity = focusSelect_VoiceSourceIsActive(focus_status) ||
                                                       focusSelect_AudioSourceIsActive(focus_status) ||
                                                       focus_status->highest_priority_source_has_audio;
    if (!is_focused_source_active_for_audio_activity)
    {
        focus_status->highest_priority_source.type = source_type_invalid;
        focus_status->highest_priority_source.u.voice = 0;
    }

    DEBUG_LOG_DEBUG("FocusSelect_GetFocusedSourceForAudioRouting src=(enum:source_type_t:%d,%d)",
                    focus_status->highest_priority_source.type,
                    focus_status->highest_priority_source.u.audio);

    return focus_status->highest_priority_source;
}

bool FocusSelect_DeviceHasVoiceOrAudioForegroundFocus(device_t device)
{
    bool device_has_voice_or_audio_focus = FALSE;

    voice_source_t voice_source = DeviceProperties_GetVoiceSource(device);
    audio_source_t audio_source = DeviceProperties_GetAudioSource(device);
    
    if (Focus_GetFocusForVoiceSource(voice_source) == focus_foreground)
    {
        device_has_voice_or_audio_focus = TRUE;
    }
    else if (Focus_GetFocusForAudioSource(audio_source) == focus_foreground)
    {
        device_has_voice_or_audio_focus = TRUE;
    }

    DEBUG_LOG("FocusSelect_DeviceHasVoiceOrAudioForegroundFocus "
              "device=%p focus=%d enum:voice_source_t:%d enum:audio_source_t:%d",
              device, device_has_voice_or_audio_focus, voice_source, audio_source );

    return device_has_voice_or_audio_focus;
}

focus_t FocusSelect_GetFocusForDevice(const device_t device)
{
    focus_t focus = focus_none;

    /* If we are a sink, then a remote (source) device is only in focus if our
       currently focused audio/voice source is the remote device. However, if we
       are a source, then the focus of the remote (sink) device is completely
       unrelated to our currently focused audio/voice source (which may for
       example be USB, or line-in). We only connect to one remote sink device at
       a time though, so we can instead determine focus by simply checking if
       the device in question is currently paired and connected. */
    if ((BtDevice_GetDeviceType(device) == DEVICE_TYPE_SINK))
    {
        bdaddr device_address = DeviceProperties_GetBdAddr(device);

        /* Getting here implies we are a source device. In which case there can
           only be one paired & connected sink, and it is always in focus whilst
           connected (regardless of audio/voice streaming state). */
        if (ConManagerIsConnected(&device_address) && DeviceList_IsDeviceOnList(device))
        {
            focus = focus_foreground;
        }

        DEBUG_LOG("FocusSelect_GetFocusForDevice device=0x%p is DEVICE_TYPE_SINK"
                  " -> enum:focus_t:%d", device, focus);
    }
    else if (FocusSelect_DeviceHasVoiceOrAudioForegroundFocus(device))
    {
        focus = focus_foreground;
        DEBUG_LOG("FocusSelect_GetFocusForDevice device=0x%p enum:focus_t:%d", device, focus);
    }
    else
    {
        DEBUG_LOG("FocusSelect_GetFocusForDevice device=0x%p focus_none because no focused source", device);
    }

    return focus;
}
