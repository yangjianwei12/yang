/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file		wired_audio_private.h
    \addtogroup wired_source
    \brief      private utility interface declarations of wired audio
    @{
*/

#ifndef WIRED_AUDIO_PRIVATE_H_
#define WIRED_AUDIO_PRIVATE_H_

#include "audio_sources_list.h"
#include "source_param_types.h"
#include "wired_audio_source.h"
#include <csrtypes.h>
#include <task_list.h>

#include "feature_manager.h"

/*! Macro that defines wired audio line_in source type */
#define WIRED_AUDIO_SOURCE_LINE_IN            (0x1)

/*! wired audio detect task data */
typedef struct
{
    TaskData task;

    /*! List of tasks registered for notifications from wired audio detect */
    task_list_t *client_tasks;

    /*! Mask that tells which wired audio device type is connected currently */
    uint8 wired_devices_mask;
	source_state_t wired_source_state; /*stores the status of audio line in source*/
    /*! Line in PIO to monitor */
    uint8 line_in_pio;
    /*! Line in is detected when PIO level is high */
    bool line_in_detect_pio_on_high;
    uint32 rate; /*! input rate */
    uint32 min_latency; /*! in milli-seconds */
    uint32 max_latency; /*! in milli-seconds */
    uint32 target_latency; /*! in milli-seconds */
    bool allow_pio_monitor_events; /*! wired pio events are allowed */
} wiredAudioSourceTaskData;

extern wiredAudioSourceTaskData wiredaudio_source_taskdata;

#define WiredAudioSourceGetTaskData()     (&wiredaudio_source_taskdata)

#define WiredAudioSourceGetTask()         (&wiredaudio_source_taskdata.task)

void WiredAudioSource_UpdateClient(void);

void WiredAudioDetect_ConfigurePio(const wired_audio_pio_t * source_pio);
bool WiredAudioDetect_IsSet(uint8 mask);

/*! \brief Update wired audio device connect mask

    \param connected   Connection status, TRUE connected, FALSE disconnected
    \param mask   A bitmask, which represent wired audio sources
*/
void WiredAudioDetect_UpdateConnectMask(bool connected, uint8 mask);

/*! \brief Reset wired audio device connect mask.*/
void WiredAudioDetect_ResetConnectMask(void);

bool WiredAudioDetect_StartMonitoring(void);
bool WiredAudioDetect_StopMonitoring(void);

unsigned WiredAudioSource_GetContext(audio_source_t source);

/*! \brief PIO state change handler for Wired audio detect module.*/
void WiredAudioDetect_HandlePioChanged(const MessagePioChanged* message);

/*! \brief Send Wired audio status messages to clients. */
void WiredAudioSource_SendStatusMessage(bool connected, audio_source_t src);

/*! \brief Increment wired audio volume */
void WiredAudioSource_IncrementVolume(audio_source_t source);

/*! \brief Decrement wired audio volume */
void WiredAudioSource_DecrementVolume(audio_source_t source);

/*! \brief Utility functions to prioritize wired audio source over other features */
void WiredAudioSource_SetFeatureState(feature_state_t state);

/*! \brief Utility functions to start wired audio source feature */
bool WiredAudioSource_StartFeature(void);

/*! \brief Utility functions to stop wired audio source feature */
void WiredAudioSource_StopFeature(void);

#endif /* WIRED_AUDIO_PRIVATE_H_ */

/*! @} */
