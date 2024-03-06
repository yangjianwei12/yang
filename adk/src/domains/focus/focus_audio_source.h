/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   focus_audio_source_domain Audio Source
\ingroup    focus_domain
\brief      Focus interface definition for instantiating a module which shall
            return the focussed Audio Source.
*/

#ifndef FOCUS_AUDIO_SOURCE_H
#define FOCUS_AUDIO_SOURCE_H

#include "focus_types.h"

#include <audio_sources.h>
#include <ui_inputs.h>

/*! @{ */

/*! \brief Focus interface callback used by Focus_GetAudioSourceForContext API */
typedef bool (*focus_audio_source_for_context_t)(audio_source_t* audio_source);

/*! \brief Focus interface callback used by Focus_GetAudioSourceForUiInput API */
typedef bool (*focus_audio_source_for_ui_input_t)(ui_input_t ui_input, audio_source_t* audio_source);

/*! \brief Structure used to configure the focus interface callbacks to be used
           to access the focussed audio source. */
typedef struct
{
    focus_audio_source_for_context_t for_context;
    focus_audio_source_for_ui_input_t for_ui_input;
} focus_get_audio_source_t;

/*! \brief Configure a set of function pointers to use for retrieving the focussed audio source

    \param a structure containing the functions implementing the focus interface for retrieving
           the focussed audio source.
*/
void Focus_ConfigureAudioSource(focus_get_audio_source_t const * focus_get_audio_source);

/*! \brief Get the focussed audio source to query the context of the specified UI Provider

    \param provider - a UI Provider
    \param audio_source - a pointer to the focussed audio_source_t handle
    \return a bool indicating whether or not a focussed audio source was returned in the
            audio_source parameter
*/
bool Focus_GetAudioSourceForContext(audio_source_t* audio_source);

/*! \brief Get the focussed audio source that should consume the specified UI Input

    \param ui_input - the UI Input that shall be consumed
    \param audio_source - a pointer to the focussed audio_source_t handle
    \return a bool indicating whether or not a focussed audio source was returned in the
            audio_source parameter
*/
bool Focus_GetAudioSourceForUiInput(ui_input_t ui_input, audio_source_t* audio_source);

/*! \brief Get the current focus status for the specified audio source

    \param audio_source - the audio_source_t handle
    \return the focus status of the specified audio source
*/
focus_t Focus_GetFocusForAudioSource(const audio_source_t audio_source);

/*! \brief Configure override function pointers to use for retrieving the focussed audio source

    Clients can register the override functions that can return the focussed audio source in some scenarios
    that client is interested in. Override functions to be executed first to check for focussed audio source, 
    if no active audio source is found, then logic to find focussed audio source would fall-back on functions 
    registered through Focus_ConfigureAudioSource().

    \param A structure containing the function pointers implementing the focus interface for retrieving
           the focussed audio source for the specified client configuration.
*/
void Focus_ConfigureAudioSourceOverride(focus_get_audio_source_t const * focus_get_audio_source);

/*! @} */

#endif /* FOCUS_AUDIO_SOURCE_H */
