/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of audio router observer functions.
*/

#include "audio_router_observer.h"
#include "logging.h"

typedef struct
{
    audio_router_observer_interface_t *observer_interface[MAX_AUDIO_ROUTER_OBSERVERS];
    uint8 number_of_registered_interfaces;
}audio_router_interface_list_t;

/*! \brief Supported interfaces structure */
typedef struct
{
    audio_router_interface_list_t audio_observer_interface;
    audio_router_interface_list_t voice_observer_interface;
}audio_router_interfaces_t;

static audio_router_interfaces_t audio_router_observer_registry;

bool AudioRouterObserver_Init(Task init_task)
{
    UNUSED(init_task);
    DEBUG_LOG("AudioRouterObserver_Init called.");

    memset(&audio_router_observer_registry.audio_observer_interface, 0, sizeof(audio_router_interface_list_t));
    memset(&audio_router_observer_registry.voice_observer_interface, 0, sizeof(audio_router_interface_list_t));

    return TRUE;
}

static bool audioRouterObserver_AdjustInterfaceList(audio_router_observer_interface_t **interface, uint8 deleted_index, uint8 number_of_interfaces)
{
    uint8 i = deleted_index;

    /* Delete index and reorder */
    while((i < number_of_interfaces - 1) && interface[i + 1])
    {
        interface[i] = interface[i + 1];
        i++;
    }

    /* Put the last entry as NULL */
    interface[i] = NULL;

    return TRUE;
}

/*Audio observer registry functions*/
void AudioRouterObserver_RegisterAudioObserver(const audio_router_observer_interface_t* interface)
{
    PanicNull((void *)interface);

    if(audio_router_observer_registry.audio_observer_interface.number_of_registered_interfaces >= MAX_AUDIO_ROUTER_OBSERVERS)
    {
        DEBUG_LOG("AudioRouterObserver_RegisterAudioObserver, audio interface not registered");
        Panic();
    }
    else
    {
        audio_router_observer_registry.audio_observer_interface.observer_interface[audio_router_observer_registry.audio_observer_interface.number_of_registered_interfaces] = (void *) interface;
        audio_router_observer_registry.audio_observer_interface.number_of_registered_interfaces ++;
        DEBUG_LOG("AudioRouterObserver_RegisterAudioObserver, audio interface registered successfully");
    }
}

static bool audioRouterObserver_RemoveAudioInterfaceEntry(const audio_router_observer_interface_t* interface)
{
    unsigned index = 0;

    while(index < audio_router_observer_registry.audio_observer_interface.number_of_registered_interfaces && audio_router_observer_registry.audio_observer_interface.observer_interface[index])
    {
        if(audio_router_observer_registry.audio_observer_interface.observer_interface[index] == interface)
        {
            return audioRouterObserver_AdjustInterfaceList(audio_router_observer_registry.audio_observer_interface.observer_interface, index, audio_router_observer_registry.audio_observer_interface.number_of_registered_interfaces);
        }
        index++;
    }
    return FALSE;
}

void AudioRouterObserver_UnregisterAudioObserver(const audio_router_observer_interface_t* interface)
{
    PanicNull((void *)interface);

    if(audio_router_observer_registry.audio_observer_interface.number_of_registered_interfaces > 0)
    {
        if(audioRouterObserver_RemoveAudioInterfaceEntry(interface))
        {
            audio_router_observer_registry.audio_observer_interface.number_of_registered_interfaces --;
            DEBUG_LOG("AudioRouterObserver_UnregisterAudioObserver, audio interface unregistered successfully");
        }
        else
        {
            DEBUG_LOG("AudioRouterObserver_UnregisterAudioObserver, audio interface didn't get unregistered");
            Panic();
        }
    }
    else
    {
        DEBUG_LOG("AudioRouterObserver_UnregisterAudioObserver, no audio interface is registered");
        Panic();
    }
}

void AudioRouterObserver_OnAudioSourceStateChange(generic_source_t source, source_state_t state)
{
    DEBUG_LOG("AudioRouterObserver_OnAudioSourceStateChange enum:source_type_t:%d enum:source_state_t:%d", source.type, state);

    uint8 registered_interfaces = audio_router_observer_registry.audio_observer_interface.number_of_registered_interfaces;

    for(uint8 i = 0; i < registered_interfaces; i++)
    {
        const audio_router_observer_interface_t* interface = audio_router_observer_registry.audio_observer_interface.observer_interface[i];

        if(interface)
        {
            if(interface->OnSourceStateChange)
            {
                interface->OnSourceStateChange(source, state);
            }
        }
    }
}

/*Voice observer registry functions*/
void AudioRouterObserver_RegisterVoiceObserver(const audio_router_observer_interface_t* interface)
{
    PanicNull((void *)interface);

    if(audio_router_observer_registry.voice_observer_interface.number_of_registered_interfaces >= MAX_AUDIO_ROUTER_OBSERVERS)
    {
        DEBUG_LOG("AudioRouterObserver_RegisterVoiceObserver, voice interface not registered");
        Panic();
    }
    else
    {
        audio_router_observer_registry.voice_observer_interface.observer_interface[audio_router_observer_registry.voice_observer_interface.number_of_registered_interfaces] = (void *) interface;
        audio_router_observer_registry.voice_observer_interface.number_of_registered_interfaces ++;
        DEBUG_LOG("AudioRouterObserver_RegisterVoiceObserver, voice interface registered successfully");
    }
}

static bool audioRouterObserver_RemoveVoiceInterfaceEntry(const audio_router_observer_interface_t* interface)
{
    unsigned index = 0;

    while(index < audio_router_observer_registry.voice_observer_interface.number_of_registered_interfaces && audio_router_observer_registry.voice_observer_interface.observer_interface[index])
    {
        if(audio_router_observer_registry.voice_observer_interface.observer_interface[index] == interface)
        {
            return audioRouterObserver_AdjustInterfaceList(audio_router_observer_registry.voice_observer_interface.observer_interface, index, audio_router_observer_registry.voice_observer_interface.number_of_registered_interfaces);
        }
        index++;
    }
    return FALSE;
}

void AudioRouterObserver_UnregisterVoiceObserver(const audio_router_observer_interface_t* interface)
{
    PanicNull((void *)interface);

    if(audio_router_observer_registry.voice_observer_interface.number_of_registered_interfaces > 0)
    {
        if(audioRouterObserver_RemoveVoiceInterfaceEntry(interface))
        {
            audio_router_observer_registry.voice_observer_interface.number_of_registered_interfaces --;
            DEBUG_LOG("AudioRouterObserver_UnregisterVoiceObserver, voice interface unregistered successfully");
        }
        else
        {
            DEBUG_LOG("AudioRouterObserver_UnregisterVoiceObserver, voice interface didn't get unregistered");
            Panic();
        }
    }
    else
    {
        DEBUG_LOG("AudioRouterObserver_UnregisterVoiceObserver, no voice interface is registered");
        Panic();
    }
}

void AudioRouterObserver_OnVoiceSourceStateChange(generic_source_t source, source_state_t state)
{
    DEBUG_LOG("AudioRouterObserver_OnVoiceSourceStateChange enum:source_type_t:%d enum:source_state_t:%d", source.type, state);

    uint8 registered_interfaces = audio_router_observer_registry.voice_observer_interface.number_of_registered_interfaces;

    for(uint8 i = 0; i < registered_interfaces; i++)
    {
        const audio_router_observer_interface_t* interface = audio_router_observer_registry.voice_observer_interface.observer_interface[i];

        if(interface)
        {
            if(interface->OnSourceStateChange)
            {
                interface->OnSourceStateChange(source, state);
            }
        }
    }
}
