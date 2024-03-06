/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       wired_audio_source_interface.c
    \ingroup    wired_source
    \brief      wired audio source interface definitions
*/

#if defined(INCLUDE_WIRED_ANALOG_AUDIO) || defined(INCLUDE_A2DP_ANALOG_SOURCE) || defined(INCLUDE_LE_AUDIO_ANALOG_SOURCE)

#include "wired_audio_source_interface.h"
#include "kymera_adaptation_audio_protected.h"
#include "wired_audio_private.h"
#include "audio_sources.h"
#include "volume_system.h"
#include "wired_audio_source.h"
#include "logging.h"

#include <stdlib.h>
#include <panic.h>

static bool wiredAudio_GetWiredAudioConnectParameters(audio_source_t source, source_defined_params_t * source_params);
static void wiredAudio_FreeWiredAudioConnectParameters(audio_source_t source, source_defined_params_t * source_params);
static bool wiredAudio_GetWiredAudioDisconnectParameters(audio_source_t source, source_defined_params_t * source_params);
static void wiredAudio_FreeWiredAudioDisconnectParameters(audio_source_t source, source_defined_params_t * source_params);
static bool wiredAudio_IsAudioRouted(audio_source_t source);
static bool wiredAudio_IsAudioAvailable(audio_source_t sources);
static source_status_t wiredAudio_SetState(audio_source_t source, source_state_t state);
static source_state_t wiredAudio_GetState(audio_source_t source);

static const audio_source_audio_interface_t wired_source_audio_interface =
{
    .GetConnectParameters = wiredAudio_GetWiredAudioConnectParameters,
    .ReleaseConnectParameters = wiredAudio_FreeWiredAudioConnectParameters,
    .GetDisconnectParameters = wiredAudio_GetWiredAudioDisconnectParameters,
    .ReleaseDisconnectParameters = wiredAudio_FreeWiredAudioDisconnectParameters,
    .IsAudioAvailable = wiredAudio_IsAudioAvailable,
    .IsAudioRouted = wiredAudio_IsAudioRouted,
    .SetState = wiredAudio_SetState,
    .GetState = wiredAudio_GetState,
    .Device = WiredAudioSource_GetDevice,
};


static bool wiredAudio_GetWiredAudioConnectParameters(audio_source_t source, source_defined_params_t * source_params)
{
    switch(source)
    {
        case audio_source_line_in:
            {
                wired_analog_connect_parameters_t *conn_param = (wired_analog_connect_parameters_t*)malloc(sizeof(wired_analog_connect_parameters_t));
                
                PanicNull(conn_param);
                
                conn_param->rate = WiredAudioSourceGetTaskData()->rate;
                conn_param->max_latency = WiredAudioSourceGetTaskData()->max_latency;
                conn_param->min_latency = WiredAudioSourceGetTaskData()->min_latency;
                conn_param->target_latency = WiredAudioSourceGetTaskData()->target_latency;
                conn_param->volume = AudioSources_CalculateOutputVolume(audio_source_line_in);

                source_params->data = (wired_analog_connect_parameters_t*)conn_param;
                source_params->data_length = sizeof(wired_analog_connect_parameters_t);
                
            }
            break;
        
         default:
            return FALSE;
    }
   return TRUE;
}

static void wiredAudio_FreeWiredAudioConnectParameters(audio_source_t source, source_defined_params_t * source_params)
{
    switch(source)
    {
        case audio_source_line_in:
            {
                PanicNull(source_params);
                PanicFalse(source_params->data_length == sizeof(wired_analog_connect_parameters_t));
                if(source_params->data_length)
                {
                    free(source_params->data);
                    source_params->data = (void *)NULL;
                    source_params->data_length = 0;
                }
            }
            break;
        
        default:
            break;
    }
}

static bool wiredAudio_GetWiredAudioDisconnectParameters(audio_source_t source, source_defined_params_t * source_params)
{
   UNUSED(source);
   UNUSED(source_params);

   return TRUE;
}

static void wiredAudio_FreeWiredAudioDisconnectParameters(audio_source_t source, source_defined_params_t * source_params)
{
   UNUSED(source);
   UNUSED(source_params);

   return;
}

static bool wiredAudio_IsAudioRouted(audio_source_t source)
{
    return WiredAudioSource_IsAudioRouted(source);
}

static bool wiredAudio_IsAudioAvailable(audio_source_t source)
{
    return WiredAudioSource_IsAudioAvailable(source);
}

const audio_source_audio_interface_t * WiredAudioSource_GetWiredAudioInterface(void)
{
    return &wired_source_audio_interface;
}

/***************************************************************************
DESCRIPTION
    Set the wired audio state
*/
static source_status_t wiredAudio_SetState(audio_source_t source, source_state_t state)
{
    DEBUG_LOG("wiredAudio_SetState enum:audio_source_t:source=%d enum:source_state_t:state=%d", source, state);
    source_status_t status = source_status_ready;
	wiredAudioSourceTaskData *sp = WiredAudioSourceGetTaskData();
	sp->wired_source_state = state;
    switch(state)
    {
        case source_state_connecting:
            {
                if(!WiredAudioSource_StartFeature())
                {
                    WiredAudioSource_SetFeatureState(feature_state_idle);
                    status = source_status_preparing;
                }
                else
                {
                    WiredAudioSource_SetFeatureState(feature_state_running);
                }
            }
            break;
        case source_state_disconnected:
            {
                WiredAudioSource_SetFeatureState(feature_state_idle);
                WiredAudioSource_StopFeature();
            }
            break;
        default:
            break;
    }
    DEBUG_LOG("wiredAudio_SetState is enum:source_status_t:%d", status);
    return status;
}

/*    Set the wired audio state    */
static source_state_t wiredAudio_GetState(audio_source_t source)
{
    source_state_t state = source_state_invalid;
    wiredAudioSourceTaskData *wiredTaskData = WiredAudioSourceGetTaskData();

    if(wiredTaskData != NULL)
    {
        state = wiredTaskData->wired_source_state;
    }

    DEBUG_LOG("wiredAudio_GetState enum:audio_source_t:source=%d enum:source_state_t:state=%d", source, state);
    return state;
}

#endif /* INCLUDE_WIRED_ANALOG_AUDIO || INCLUDE_A2DP_ANALOG_SOURCE || INCLUDE_LE_AUDIO_ANALOG_SOURCE */

