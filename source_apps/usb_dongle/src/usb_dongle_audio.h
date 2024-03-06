/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Start/stop audio from various inputs, keep track of active source.
*/

#ifndef USB_DONGLE_AUDIO_H
#define USB_DONGLE_AUDIO_H

#include <audio_sources.h>
#include <message.h>
#include <wired_audio_source.h>
#include "usb_dongle_config.h"

/*! \brief USB Dongle application audio inputs */
typedef enum usb_dongle_audio_inputs
{
    usb_dongle_audio_input_none       = 0,
    usb_dongle_audio_input_usb        = (1<<0),
    usb_dongle_audio_input_analogue   = (1<<1),
    usb_dongle_audio_input_usb_voice  = (1<<2),
    usb_dongle_audio_input_sco_voice  = (1<<3),

} usb_dongle_audio_input_t;

/*! \brief Callback to indicate that UsbDongle_AudioStop has completed */
typedef void (*UsbDongleAudioStopped)(void);

/*! \brief Determine whether any audio source is currently streaming.

    Checks to see if any audio chain has been started.

    \returns True if there is an active audio source, False otherwise.
 */
bool UsbDongle_AudioIsActive(void);

/*! \brief Add an audio input to be considered for source selection.

    Usually called by a client when a new audio input becomes available.

    \param[in] input The audio input that was plugged in / connected.
 */
void UsbDongle_AudioInputAdd(usb_dongle_audio_input_t input);

/*! \brief Remove an audio input from consideration for source selection.

    Usually called by a client when an audio input is no longer available.

    \param[in] input The audio input that was unplugged / disconnected.
 */
void UsbDongle_AudioInputRemove(usb_dongle_audio_input_t input);

/*! \brief Get the highest priority audio source from connected inputs.

    Determine the audio source that has the highest priority from all currently
    available connected inputs.

    \returns The audio source with highest priority.
 */
audio_source_t UsbDongle_AudioDetermineNewSource(void);

/*! \brief Get the highest priority voice source from connected inputs.

    Determine the voice source that has the highest priority from all currently
    available connected inputs.

    \returns The voice source with highest priority.
 */
voice_source_t UsbDongle_VoiceDetermineNewSource(void);

/*! \brief Check if the currently streaming audio source needs to be changed.

    If currently streaming, checks that the audio source being streamed is the
    highest priority out of those available. Usually called by a client after
    UsbDongle_AudioInputAdd or UsbDongle_AudioInputRemove changes the avaiable
    audio inputs during streaming. Returns FALSE if not currently streaming.

    \returns TRUE if the source should be changed to another, FALSE otherwise.
 */
bool UsbDongle_AudioSourceSwitchIsRequired(void);

/*! \brief Check if audio graph restart is required due to a USB audio configuration
           change.

    \param[in] is_sampling_rate_changed TRUE if sample rate have changed

    \returns TRUE if audio graph restart is needed, FALSE otherwise
 */
bool UsbDongle_IsGraphRestartNeededForUsbAudioConfigChange(bool is_sampling_rate_changed);

/*! \brief Restart USB audio graph. Usually to apply updated sample rates
 */
void UsbDongle_AudioRestartUsbGraph(void);

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO)

/*! \brief Start the highest priority audio source from connected inputs.

    Determine the audio source that has the highest priority from all currently
    available connected inputs, and start its audio chain.

    \note Fails (and returns FALSE) if any source is already active.

    \param[in] config Parameters used to connect wired audio chain.

    \returns TRUE if a source was started, FALSE otherwise.
 */
bool UsbDongle_AudioStart(const wired_audio_config_t *config);

/*! \brief Stop the currently active audio source.

    Calling this function will always result in any active audio chain stopping.
    This operation may be asynchronous, so a callback can optionally be supplied
    by the caller which will be invoked upon completion.

    \param[in] callback Invoked once chain has successfully stopped.
 */
void UsbDongle_AudioStop(UsbDongleAudioStopped callback);

#elif defined(INCLUDE_SOURCE_APP_LE_AUDIO)

#define UsbDongle_AudioStart(config)        UNUSED(config)
#define UsbDongle_AudioStop(callback)       UNUSED(callback)

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

/*! \brief Initialise USB dongle audio.
 */
bool UsbDongle_AudioInit(Task init_task);

/*! \brief Returns whether an audio_input is currently connected

    \param[in] audio_input The audio input that should be checked.

    \returns Returns TRUE if the audio input is currently connected.
 */
bool UsbDongle_AudioInputIsConnected(usb_dongle_audio_input_t audio_input);

/*! \brief Returns the latencies for the currently active audio stream

    \param[in] min_latency_ms The minimum latency
    \param[in] max_latency_ms The maximum latency
    \param[in] target_latency_ms The target latency

    \returns Returns TRUE if the latencies are valid
 */
bool UsbDongle_AudioGetLatencies(uint32 *min_latency_ms, uint32 *max_latency_ms, uint32 *target_latency_ms );

/*! \brief Get preferred sample rate of current highest priority audio input.

    Some audio inputs can have their sample rate configured after being
    connected (e.g. analog line in), and so don't have any particular preference
    on sample rate. They can be configured to match the rest of the chain.
    Other inputs however (such as USB) are entirely host controlled - their
    sample rates cannot be influenced by the USB dongle at all. So the preferred
    sample rate for the rest of the chain is to match the input's sample rate,
    if possible. USB sample rates in particular are also dynamic - they can
    change after enumeration, and are also application specific, depending on
    which driver interface the applications running on the host use e.g. ASIO,
    MME, WASAPI, etc.

    \returns Preferred sample rate of current audio input, or 0 if no preference.
 */
uint32 UsbDongle_AudioGetCurrentPreferredSampleRate(void);

/*! \brief Connect audio source.
*/
bool UsbDongle_AudioSourceConnect(void);

/*! \brief Connect audio stream.
*/
void UsbDongle_AudioStreamConnect(void);

/*! \brief Disconnect audio stream.
*/
void UsbDongle_AudioStreamDisconnect(void);

/*! \brief To check if audio source is active.

     \return TRUE if audio source is active.
*/
bool UsbDongle_AudioIsSourceAvailable(void);

/*! \brief To check if audio source with VBC is available.

     \return TRUE if audio source with VBC is available and can
             start streaming if needed.
*/
bool UsbDongle_AudioIsVbcAvailable(void);

/*! \brief To check if audio with VBC is active currently

     \return TRUE if VBC is currently streaming, FALSE otherwise
*/
bool UsbDongle_AudioIsVbcActive(void);

#endif /* USB_DONGLE_AUDIO_H */
