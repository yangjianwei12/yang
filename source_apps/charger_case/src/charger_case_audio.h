/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Start/stop audio from various inputs, keep track of active source.
*/

#ifndef CHARGER_CASE_AUDIO_H
#define CHARGER_CASE_AUDIO_H

#include <audio_sources.h>
#include <message.h>
#include <wired_audio_source.h>

/*! \brief Charger Case application audio inputs */
typedef enum charger_case_audio_inputs
{
    charger_case_audio_input_none       = 0,
    charger_case_audio_input_usb        = (1<<0),
    charger_case_audio_input_analogue   = (1<<1),

} charger_case_audio_input_t;

/*! \brief Callback to indicate that ChargerCase_AudioStop has completed */
typedef void (*ChargerCaseAudioStopped)(void);

/*! \brief Determine whether any audio source is currently streaming.

    Checks to see if any audio chain has been started.

    \returns True if there is an active audio source, False otherwise.
 */
bool ChargerCase_AudioIsActive(void);

/*! \brief Add an audio input to be considered for source selection.

    Usually called by a client when a new audio input becomes available.

    \param[in] input The audio input that was plugged in / connected.
 */
void ChargerCase_AudioInputAdd(charger_case_audio_input_t input);

/*! \brief Remove an audio input from consideration for source selection.

    Usually called by a client when an audio input is no longer available.

    \param[in] input The audio input that was unplugged / disconnected.
 */
void ChargerCase_AudioInputRemove(charger_case_audio_input_t input);

/*! \brief Get the highest priority audio source from connected inputs.

    Determine the audio source that has the highest priority from all currently
    available connected inputs.

    \returns The audio source with highest priority.
 */
audio_source_t ChargerCase_AudioDetermineNewSource(void);

/*! \brief Start the highest priority audio source from connected inputs.

    Determine the audio source that has the highest priority from all currently
    available connected inputs, and start its audio chain or switch if required.

    \param[in] config Parameters used to connect wired audio chain.

    \returns True if a source is now active (or was already), False otherwise.
 */
bool ChargerCase_AudioStart(const wired_audio_config_t *config);

/*! \brief Stop the currently active audio source.

    Calling this function will always result in any active audio chain stopping.
    This operation may be asynchronous, so a callback can optionally be supplied
    by the caller which will be invoked upon completion.

    \param[in] callback Invoked once chain has successfully stopped.
 */
void ChargerCase_AudioStop(ChargerCaseAudioStopped callback);

/*! \brief Switch to a higher priority audio source if one is available.

    Usually called by a client after ChargerCase_AudioInputAdd or
    ChargerCase_AudioInputRemove changes the avaiable audio inputs.
 */
void ChargerCase_AudioSwitchSourceIfRequired(void);

/*! \brief Initialise charger case audio.
 */
bool ChargerCase_AudioInit(Task init_task);

/*! \brief Returns whether an audio_input is currently connected
 *
    \param[in] audio_input The audio input that should be checked.

    \returns Returns TRUE if the audio input is currently connected.
 */
bool ChargerCase_AudioInputIsConnected(charger_case_audio_input_t audio_input);

#endif // CHARGER_CASE_AUDIO_H
