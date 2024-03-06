/*!
\copyright  Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       audio_router_observer.h
\addtogroup audio_router
\brief      Provides a standard API to register and unregister audio and voice observers for use in the audio router
*/

#ifndef AUDIO_ROUTER_OBSERVER_H
#define AUDIO_ROUTER_OBSERVER_H

#include "source_param_types.h"

#define MAX_AUDIO_ROUTER_OBSERVERS (1)

/*! \brief The audio router observer interface
*/
typedef struct
{
    void (*OnSourceStateChange)(generic_source_t source, source_state_t state);
} audio_router_observer_interface_t;

bool AudioRouterObserver_Init(Task init_task);

/*! \brief Registers an audio observer interface for the router.

    \param interface The audio router's audio observer interface to register
 */
void AudioRouterObserver_RegisterAudioObserver(const audio_router_observer_interface_t* interface);

/*! \brief Unregisters an audio observer interface for an audio source.

    \param interface The audio router's audio observer interface to unregister
 */
void AudioRouterObserver_UnregisterAudioObserver(const audio_router_observer_interface_t* interface);

/*! \brief Calls the OnSourceStateChange observer function of an audio sources registered observer interface.
    \param source The generic(audio) source
    \param state Indicates the state of the audio source
 */
void AudioRouterObserver_OnAudioSourceStateChange(generic_source_t source, source_state_t state);

/*! \brief Registers a voice observer interface for the router.

    \param interface The audio router's voice observer interface to register
 */
void AudioRouterObserver_RegisterVoiceObserver(const audio_router_observer_interface_t* interface);

/*! \brief Unregisters an voice observer interface for an audio source.

    \param interface The audio router's voice observer interface to unregister
 */
void AudioRouterObserver_UnregisterVoiceObserver(const audio_router_observer_interface_t* interface);

/*! \brief Calls the OnSourceStateChange observer function of an voice sources registered observer interface.
    \param source The generic(voice) source
    \param state Indicates the state of the voice source
 */
void AudioRouterObserver_OnVoiceSourceStateChange(generic_source_t source, source_state_t state);

#endif // AUDIO_ROUTER_OBSERVER_H
