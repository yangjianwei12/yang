/*!
\copyright  Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface to audio sources.

            Implementation of this interface is mandatory for an audio source
*/

#ifndef AUDIO_SOURCES_AUDIO_INTERFACE_H_
#define AUDIO_SOURCES_AUDIO_INTERFACE_H_

#include "audio_sources_list.h"
#include "source_param_types.h"

#include <device.h>

#define MAX_AUDIO_INTERFACES (1)

typedef struct
{
    bool (*GetConnectParameters)(audio_source_t source, source_defined_params_t * source_params);
    void (*ReleaseConnectParameters)(audio_source_t source, source_defined_params_t * source_params);
    bool (*GetDisconnectParameters)(audio_source_t source, source_defined_params_t * source_params);
    void (*ReleaseDisconnectParameters)(audio_source_t source, source_defined_params_t * source_params);
    bool (*IsAudioRouted)(audio_source_t source);
    source_status_t (*SetState)(audio_source_t source, source_state_t state);
	bool (*IsAudioAvailable)(audio_source_t source); 					//If src ctx = "context_audio_is_paused"",Should return FALSE. 
    source_state_t (*GetState)(audio_source_t source);
    device_t (*Device)(audio_source_t source);
} audio_source_audio_interface_t;

#endif /* AUDIO_SOURCES_AUDIO_INTERFACE_H_ */
