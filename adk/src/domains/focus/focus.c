/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   focus_domain Focus
\ingroup    domains
\brief      Module implementing the interface by which the application framework can
            call a concrete focus select implementation, either focus select or a
            customer module.
*/

#include "focus_audio_source.h"
#include "focus_device.h"
#include "focus_generic_source.h"
#include "focus_voice_source.h"
#include "focus_plugin.h"

#include <device_properties.h>

/*! @{ */

static const focus_device_t * select_focused_device_fns = NULL;
static const focus_device_t * select_override_device_fns = NULL;
static const focus_get_audio_source_t * select_focused_audio_source_fns = NULL;
static const focus_get_audio_source_t * select_override_audio_source_fns = NULL;
static const focus_get_voice_source_t * select_focused_voice_source_fns = NULL;
static const focus_get_generic_source_t * select_focused_generic_source_fns = NULL;
static const focus_get_generic_source_override_t * select_override_generic_source_fns = NULL;
static const focus_plugin_configure_t * plugin_configure_fns = NULL;

void Focus_ConfigureDevice(focus_device_t const * focus_device)
{
    select_focused_device_fns = focus_device;
}

bool Focus_GetDeviceForContext(ui_providers_t provider, device_t* device)
{
    if (select_focused_device_fns && select_focused_device_fns->for_context)
    {
        return select_focused_device_fns->for_context(provider, device);
    }
    return FALSE;
}

bool Focus_GetDeviceForUiInput(ui_input_t ui_input, device_t* device)
{
    bool device_found = FALSE;

    if (select_focused_device_fns && select_focused_device_fns->for_ui_input)
    {
        device_found = select_focused_device_fns->for_ui_input(ui_input, device);
    }

    if(select_override_device_fns && select_override_device_fns->for_ui_input)
    {
        if(select_override_device_fns->for_ui_input(ui_input, device))
        {
            device_found = TRUE;
        }
    }

    return device_found;
}

focus_t Focus_GetFocusForDevice(const device_t device)
{
    focus_t device_focus = focus_none;
    if (select_focused_device_fns && select_focused_device_fns->focus)
    {
        device_focus = select_focused_device_fns->focus(device);
    }
    return device_focus;
}

void Focus_ConfigureAudioSource(focus_get_audio_source_t const * focus_audio_source)
{
    select_focused_audio_source_fns = focus_audio_source;
}

bool Focus_GetAudioSourceForContext(audio_source_t* audio_source)
{
    bool source_found = FALSE;

    if (select_focused_audio_source_fns && select_focused_audio_source_fns->for_context)
    {
        source_found = select_focused_audio_source_fns->for_context(audio_source);
    }

    if(select_override_audio_source_fns && select_override_audio_source_fns->for_context)
    {
        if(select_override_audio_source_fns->for_context(audio_source))
        {
            source_found = TRUE;
        }
    }
    return source_found;
}

bool Focus_GetAudioSourceForUiInput(ui_input_t ui_input, audio_source_t* audio_source)
{
    if (select_focused_audio_source_fns && select_focused_audio_source_fns->for_ui_input)
    {
        return select_focused_audio_source_fns->for_ui_input(ui_input, audio_source);
    }
    return FALSE;
}

focus_t Focus_GetFocusForAudioSource(const audio_source_t audio_source)
{
    generic_source_t source_to_check = {.type=source_type_audio, .u.audio= audio_source};

    generic_source_t focused_source = Focus_GetFocusedGenericSourceForAudioRouting();

    if (GenericSource_IsSame(focused_source, source_to_check))
    {
        return focus_foreground;
    }
    else
    {
        voice_source_t matching_source = audio_source != audio_source_none ? DeviceProperties_GetVoiceSource(AudioSources_GetAudioSourceDevice(audio_source))
                                                                           : voice_source_none;
        /* If a matching voice source exists on the same device,
         * give background focus to the audio source, for as long as the voice source is ringing.
         * This is to account for silenced calls. */
        if (VoiceSource_IsValid(matching_source) && VoiceSources_IncomingCallRinging(matching_source))
        {
            return focus_background;
        }
        
        return focus_none;
    }
}

void Focus_ConfigureVoiceSource(focus_get_voice_source_t const * focus_voice_source)
{
    select_focused_voice_source_fns = focus_voice_source;
}

bool Focus_GetVoiceSourceForContext(ui_providers_t provider, voice_source_t* voice_source)
{
    if (select_focused_voice_source_fns && select_focused_voice_source_fns->for_context)
    {
        return select_focused_voice_source_fns->for_context(provider, voice_source);
    }
    return FALSE;
}

bool Focus_GetVoiceSourceInContextArray(ui_providers_t provider, voice_source_t* voice_source, const unsigned* contexts, const unsigned num_contexts)
{
    if (select_focused_voice_source_fns && select_focused_voice_source_fns->in_contexts)
    {
        return select_focused_voice_source_fns->in_contexts(provider, voice_source, contexts, num_contexts);
    }
    return FALSE;
}

bool Focus_GetVoiceSourceForUiInput(ui_input_t ui_input, voice_source_t* voice_source)
{
    if (select_focused_voice_source_fns && select_focused_voice_source_fns->for_ui_input)
    {
        return select_focused_voice_source_fns->for_ui_input(ui_input, voice_source);
    }
    return FALSE;
}

focus_t Focus_GetFocusForVoiceSource(const voice_source_t voice_source)
{
    generic_source_t source_to_check = {.type = source_type_voice, .u.voice = voice_source};

    generic_source_t focused_source = Focus_GetFocusedGenericSourceForAudioRouting();

    if (GenericSource_IsSame(focused_source, source_to_check))
    {
        return focus_foreground;
    }
    else
    {
        return focus_none;
    }
}

void Focus_ConfigureGenericSource(focus_get_generic_source_t const * focus_generic_source)
{
    select_focused_generic_source_fns = focus_generic_source;
}

generic_source_t Focus_GetFocusedGenericSourceForAudioRouting(void)
{
    generic_source_t source = {.type=source_type_invalid, .u.voice=voice_source_none};

    if (select_focused_generic_source_fns && select_focused_generic_source_fns->for_audio_routing)
    {
        source = select_focused_generic_source_fns->for_audio_routing();
    }

    if(select_override_generic_source_fns && select_override_generic_source_fns->for_audio_routing)
    {
        if(select_override_generic_source_fns->for_audio_routing(&source))
        {
            return source;
        }
    }
    return source;
}

void Focus_ConfigureDeviceOverride(focus_device_t const * focus_device)
{
    select_override_device_fns = focus_device;
}

void Focus_ConfigureAudioSourceOverride(focus_get_audio_source_t const * focus_audio_source)
{
    select_override_audio_source_fns = focus_audio_source;
}

void Focus_ConfigureGenericSourceOverride(focus_get_generic_source_override_t const * focus_generic_source)
{
    select_override_generic_source_fns = focus_generic_source;
}

void Focus_PluginConfigure(focus_plugin_configure_t const * plugin_configure)
{
    plugin_configure_fns = plugin_configure;
}

bool Focus_SetConfig(unsigned config_option, void* config)
{
    if(plugin_configure_fns && plugin_configure_fns->set_config)
    {
        return plugin_configure_fns->set_config(config_option, config);
    }
    
    return FALSE;
}

bool Focus_GetConfig(unsigned config_option, void* config)
{
    if(plugin_configure_fns && plugin_configure_fns->get_config)
    {
        return plugin_configure_fns->get_config(config_option, config);
    }
    
    return FALSE;
}

/*! @} */
