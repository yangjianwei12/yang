/*!
\copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for test module
*/
#ifndef USB_DONGLE_TEST_H
#define USB_DONGLE_TEST_H

#include "usb_dongle_sm.h"
#include "usb_dongle_config.h"
#include <audio_aptx_adaptive_encoder_params.h>
#include <av.h>

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
#include "le_audio_client.h"
#endif

/*
** Test Infrastructure
*/

uint16 appTestSetTestNumber(uint16 test_number);

/*! Set a test iteration to be displayed through appTestWriteMarker

    This is intended for use in test output. Set test_iteration to
    zero to not display.

    \param  test_iteration The test iteration number to use in output

    \return The test iteration. This can give output on the pydbg console
        as well as the output.
*/
uint16 appTestSetTestIteration(uint16 test_iteration);


/*! Write a marker to output

    This is intended for use in test output

    \param  marker The value to include in the marker. A marker of 0 will
        write details of the testcase and iteration set through
        appTestSetTestNumber() and appTestSetTestIteration().

    \return The marker value. This can give output on the pydbg console
        as well as the output.
*/
uint16 appTestWriteMarker(uint16 marker);


/* 
** System and State
*/

/*! \brief Reset to factory defaults.
    Will drop any connections, delete all pairing and reboot.
*/
void appTestFactoryReset(void);

/*! \brief Determine if a reset has happened

    \return TRUE if a reset has happened since the last call to the function
*/
bool appTestResetHappened(void);


/*! Check if the application state machine is in the specified role

    The states are defined in usb_dongle_sm.h, and can be accessed from
    python by name - example
    \code
      apps1.fw.env.enums["usbDongleState"]["USB_DONGLE_STATE_IDLE"])
    \endcode

    \return TRUE The USB dongle state machine is in the specified state
*/
bool appTestApplicationIsState(usb_dongle_state_t checked_state);

/*! \brief Get the application state

    \return Returns the enum for the application state
*/
usb_dongle_state_t appTestApplicationGetState (void);

/*! \brief Test if the application initialisation is complete.

    \return Returns TRUE if the initialisation is complete.
*/
bool appTestApplicationIsInitialisationCompleted(void);

/*! Report the contents of the Device Database. */
void appTestDeviceDatabaseReport(void);

/*! \brief Simple test function to get the DSP clock speed in MegaHertz during the Active mode.
    \note Make sure DSP is up and running before using this test function on pydbg window to avoid panic.
    \returns DSP clock speed in MegaHertz during the Active mode
*/
uint32 appTestDspClockSpeedInActiveMode(void);


/*
** Analog Input
*/

#if defined(INCLUDE_A2DP_ANALOG_SOURCE) || defined(INCLUDE_LE_AUDIO_ANALOG_SOURCE)

/*! \brief To simulate plugin in of 3.5mm jack to the board.
NOTE: NOT to be used if boards support PIO detection. The toggling of PIO from high to low indicates 3.5mm jack is plugged in */
void appTestWiredAnalogAudioPlugged(void);

/*! \brief To simulate plugging out of 3.5mm jack from the board.
NOTE: NOT to be used if boards support PIO detection. The toggling of PIO from low to high indicates 3.5mm jack is plugged out */
void appTestWiredAnalogAudioUnplugged(void);

/*! \brief Query the connected inputs for wired analog input

    \return True if the connected input bitmask contains the wired analog input
*/
bool appTestWiredAnalogIsInputConnected(void);

#endif /* INCLUDE_WIRED_ANALOG_AUDIO */

/*
** Earbud to Dongle Pairing
*/

/*! \brief Delete all earbud pairing
           \note This will disconnect from the active earbud first before
                 deleting the device database and clearing the paired device list
*/
void appTestDeletePairing(void);

/*! \brief Pair to an earbud with a specific address.
 
    \param nap The NAP of the earbud address to attempt pairing with
    \param uap The UAP of the earbud address to attempt pairing with
    \param lap The LAP of the earbud address to attempt pairing with

    \return TRUE if the pairing module was called to start pairing
    \return FALSE if the pairing module or the USB dongle SM are
            in pairing mode, an earbud is already paired, or the
            USB dongle is not idle
*/
bool appTestEarbudPair(uint16 nap, uint8 uap, uint32 lap);

/*! \brief Return True if there is an Earbud in the paired device list */
bool appTestEarbudIsPaired(void);

/*! \brief Disable RSSI pairing */
void appTestEarbudPairingRssiDisable(void);

/*! \brief Enable RSSI pairing */
void appTestEarbudPairingRssiEnable(void);

/*! \brief Allow devices to pair to the dongle
    \return TRUE if successful
*/
bool appTestDongleBecomeBondable(void);

/*! \brief Prevent devices from pairing to the dongle
    \return TRUE if successful
*/
bool appTestDongleStopBondable(void);

/*! \brief Start an RSSI scan */
void appTestEarbudPairingRssiStart(void);

/*! \brief Stop an ongoing RSSI pairing scan */
void appTestEarbudPairingRssiStop(void);

/*
** Earbud Bluetooth connections
*/

/*! \brief Enable/Disable RF transmission
 */
bool appTestTransmitEnable(bool);

/*! \brief Connect to paired earbuds */
void appTestEarbudConnect(void);

/*! \brief Disconnect from the earbuds */
void appTestEarbudDisconnect(void);

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
/*! \brief Send A2dp Media Suspend request to remote
    Added to support PTS qualification TCs.
*/
bool appTestEarbudA2dpMediaSuspend(void);

/*! \brief Send A2dp Media Resume request to remote
    Added to support PTS qualification TCs.
*/
bool appTestEarbudA2dpMediaResume(void);
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

/*! \brief Return if there is connection to the earbuds

    \return bool TRUE if connection
                 FALSE if no connection
*/
bool appTestEarbudIsConnected(void);

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
/*! \brief Return if USB dongle has an earbud A2DP connection

    \return bool TRUE USB dongle has A2DP earbud connection
                 FALSE USB dongle does not have A2DP earbud connection
*/
bool appTestEarbudIsA2dpConnected(void);

/*! \brief Return if USB dongle has an earbud AVRCP connection

    \return bool TRUE USB dongle has AVRCP earbud connection
                 FALSE USB dongle does not have AVRCP earbud connection
*/
bool appTestEarbudIsAvrcpConnected(void);

/*! \brief Return if USB dongle has an earbud HFP connection

    \return bool TRUE USB dongle has HFP earbud connection
                 FALSE USB dongle does not have HFP earbud connection
*/
bool appTestEarbudIsHfpConnected(void);
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

/*! \brief Return if application  has an ACL connection to the earbud

    It does not indicate if the earbud is usable, with profiles
    connected. Use appTestEarbudIsConnected.

    \return bool TRUE headset has an ACL connection
                 FALSE application does not have an ACL connection to the Handset
*/
bool appTestEarbudIsAclConnected(void);

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
/*! \brief Return the A2DP play status

    \return bool TRUE if A2DP play status is playing
                 FALSE if A2DP play status is not playing
*/
bool appTestEarbudIsA2dpPlaying(void);

/*! \brief Return Earbud has an A2DP media connection or not

    \return bool TRUE Earbud has A2DP media connection
                 FALSE Earbud does not have A2DP media connection
*/
bool appTestEarbudIsA2dpMediaConnected(void);

/*! \brief Return Earbud is in A2DP streaming mode or not

    \return bool TRUE Earbud is in A2DP streaming mode
                 FALSE Earbud is not in A2DP streaming mode
*/
bool appTestEarbudIsA2dpStreaming(void);


/*! \brief Return if earbud is connected via HFP

    \return bool TRUE Earbud has an HFP connection
                 FALSE Earbud does not have an HFP connection
*/
bool appTestEarbudIsHfpConnected(void);

/*! \brief Return if the earbud has an active SCO connection

    \return bool TRUE Earbud has a SCO connection
                 FALSE Earbud does not have a SCO connection
*/
bool appTestEarbudIsScoConnectionActive(void);

/*! \brief Send relative AVRCP absolute volume change to the Earbud

    \param step : relative Step change to apply (+127 to -127)

    \return bool TRUE absolute AVRCP volume step change sent
                 FALSE absolute AVRCP volume step change was not sent
*/
bool appTestEarbudChangeAvVolume(int8 step);

/*! \brief Send the Avrcp absolute set volume command to the Earbud

    \param volume   New volume level to set (0-127).
*/
bool appTestEarbudSetAvVolume(uint8 volume);

/*! \brief Send the Avrcp Notify Volume Change command to the Earbud

    \param volume   Volume level to notify (0-127).
*/
bool appTestEarbudNotifyVolumeChange(uint8 volume);


/*! \brief Disable the PTS test mode */
bool appTestEarbudSendTrackChange(void);

void appTestEarbudSetTrackSelected(bool selected);

void appTestEarbudUseLargeMetadata(bool use_large_metadata);

void appTestEarbudSetPlayStatusStopped(void);

void appTestEarbudSetPlayStatusPlaying(void);

void appTestEarbudSetPlayStatusPaused(void);

bool appTestEarbudSendPassthroughVolumeUp(void);

bool appTestEarbudSendPassthroughVolumeDown(void);

bool appTestEarbudSendPassthroughPlay(void);

bool appTestEarbudSendPassthroughPause(void);

bool appTestEarbudSendPassthroughForward(void);

bool appTestEarbudSendPassthroughBack(void);

bool appTestEarbudSendPassthroughFastRewind(bool state);

bool appTestEarbudSendPassthroughFastForward(bool state);

#ifdef USE_SYNERGY
bool appTestEarbudAvrcpNotiRegister(CsrBtAvrcpNotiId event_id);

bool appTestEarbudA2dpMediaClose(void);

bool appTestEarbudA2dpMediaAbort(void);

bool appTestEarbudA2dpMediaReconfigure(void);

/*! \brief Reconfigure AVRCP to initiate Browsing connection after outgoing Control channel is established.
    Added to support PTS qualification TCs.

    \return TRUE if able to reconfigure, FALSE otherwise
*/
bool appTestReconfigureAvrcpWithBrowsing(void);
#endif /* USE_SYNERGY */

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */
/*!
 * \brief This returns true if earbud is connected over qhs and false if 
          earbud is not connected over qhs
 * \return true or false as per description.
 */
bool appTestEarbudIsQhsConnected(void);


/*
** CODEC tests
*/

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
/*! \brief To get current A2DP codec
    The return value follows this enum
*/
typedef enum
{
    a2dp_codec_unknown,
    a2dp_codec_sbc,
    a2dp_codec_aac,
    a2dp_codec_aptx,
    a2dp_codec_aptx_hd,
    a2dp_codec_aptx_adaptive,
} app_test_a2dp_codec_t;
app_test_a2dp_codec_t appTestCodecGetType(void);

/*! \brief Get the current sample rate of the configured A2DP codec.

    \returns Sample rate configured for the current A2DP codec, in Hz (or 0 if
             no media channel established).
*/
uint32 appTestCodecSampleRateGetCurrent(void);

/*! \brief Set the aptX adaptive encoder to HQ mode.

    This API will automatically restart the audio chain if required, to make the
    change take immediate effect if the encoder is already running. Otherwise,
    the preference is saved for next time the encoder is started.

    \returns TRUE if mode changed, FALSE if not i.e. already in HQ mode.
*/
bool appTestCodecAptxAdModeSetHQ(void);

/*! \brief Set aptX adaptive encoder to Low Latency mode.

    This API will automatically restart the audio chain if required, to make the
    change take immediate effect if the encoder is already running. Otherwise,
    the preference is saved for next time the encoder is started. It will also
    automatically renegotiate the A2DP sample rate to 48kHz, if not already.

    \returns TRUE if mode changed, FALSE if not i.e. already in LL mode.
*/
bool appTestCodecAptxAdModeSetLL(void);

/*! \brief Get current preferred aptX adaptive encoder quality mode.

    \returns Currently active mode (live value) if the encoder is running,
             otherwise the desired mode to be used next time the encoder starts.
*/
aptxad_quality_mode_t appTestCodecAptxAdModeGetCurrent(void);

/*! \brief Set the sample rate to be used by aptX Adaptive in HQ mode.

    Overrides the default aptX Adaptive behaviour of selecting the HQ sample
    rate based on the current audio input rate, and instead uses the supplied
    pre-set value (if valid and supported by both sink & source).

    This may trigger an A2DP reconfigure, if an aptX Adaptive media channel is
    already established, and we are in HQ mode. Otherwise, the rate will be
    applied the next time HQ mode is activated / aptX Adaptive connected.

    If a non-supported (invalid) rate is supplied, then the codec will instead
    select the highest possible rate supported by both sides.

    \param rate The sample rate to use for aptX Adaptive HQ, if possible.
*/
void appTestCodecAptxAdHqSampleRateSet(uint32 rate);

/*! \brief Get the saved sample rate to be used by aptX Adaptive in HQ mode.

    \returns Saved sample rate previously set for aptX Adaptive HQ mode using
             appTestCodecAptxAdHqSampleRateSet(), or 0 if none set.
*/
uint32 appTestCodecAptxAdHqSampleRateGetSaved(void);

/*! \brief Reset the sample rate to be used for aptX Adaptive HQ to the default.

    Clears any saved aptX Adaptive HQ mode sample rate previously set via
    appTestCodecAptxAdHqSampleRateSet(). This will return the codec to its
    default behaviour of selecting an appropriate sample rate based on the
    current audio input rate.
*/
void appTestCodecAptxAdHqSampleRateDefault(void);

/*! \brief Disable automatic RF signal param updates to the aptX adaptive encoder. */
void appTestCodecAptxAdRfSignalUpdatesDisable(void);

/*! \brief Re-enable automatic RF signal param updates to the aptX adaptive encoder. */
void appTestCodecAptxAdRfSignalUpdatesEnable(void);

/*! \brief Send an updated set of RF signal params to the aptX adaptive encoder.

    If manual control of the RF signal parameters is desired, it is recommended
    to first disable automatic updates, by calling the test function
    appTestCodecAptxAdRfSignalUpdatesDisable(). Otherwise any params sent via
    this function will be quickly overwritten by automatic updates, if the
    encoder is running. If the encoder isn't running, then the params will be
    stored and sent as the "initial set" next time the encoder starts.

    \param[in] rssi The new raw signal strength, in dBm (default -40).
    \param[in] cqddr The new recommended link transmission rate, in Mbps
                     (default 3, range 2-6). Higher recommended rates allow use
                     of larger packet types, and thus higher data throughput.
    \param[in] quality The new link quality value (default 3, range 1-5).
                       Metric indicating how reliable the channel is (5 = best).

    \return TRUE if params sent to encoder, FALSE if encoder not running (in
            which case they are stored & sent next time the encoder is started).
*/
bool appTestCodecAptxAdRfSignalParamsSet(int16 rssi, uint16 cqddr, uint16 quality);

/*! \brief Get the current most recent set of RF signal parameters.

    This will either be the latest live values last sent to the encoder, if the
    encoder is running. Or the set of values to be sent next time the encoder
    starts, if it's not currently running.

    \param[out] params Pointer to structure to write RF signal values to.
    params can be set to NULL, in which case the function will only print the parameters to the log.

    \return TRUE if values written, FALSE if not (e.g. NULL pointer provided).
            Function will print current values to apps1 log regardless.
*/
bool appTestCodecAptxAdRfSignalParamsGet(aptxad_96K_encoder_rf_signal_params_t *params);

/*! \brief Set the PS_KEY_TEST_AV_CODEC.
 *
 *  \param[in] codecs Bitmap of codecs PS_KEY_TEST_AV_CODEC to be set.
 *
 *  \note Needs TEST_AV_CODEC_PSKEY defined within the application.
 *        Device needs to be reset before changes take effect.
 *
 *  \return TRUE if value is written to PS_KEY_TEST_AV_CODEC. FALSE if not.
 *          Function will print message to apps1 log.
*/
bool appTestCodecsAdvertisedA2dpSet(uint16 codecs);

/*! \brief Get the PS_KEY_TEST_AV_CODEC.
 *
 *  \param[out] set_codecs Pointer to variable to write PS_KEY_TEST_AV_CODEC
 *                         bitmap of codecs to.
 *
 *  \note Needs TEST_AV_CODEC_PSKEY defined within the application.
 *        If NULL pointer provided, and value is read, the value will be printed
 *        to the log but not stored in pointer.
 *
 *  \return TRUE if value is read and value stored in pointer.
 *          FALSE if value is not read and/or NULL pointer provided.
 *          Function will print message to apps1 log.
*/
bool appTestCodecsAdvertisedA2dpGet(uint16* set_codecs);



/*! \brief Set the PS_KEY_TEST_HFP_CODEC_PSKEY.
 *
 *  \param[in] codecs Bitmap of codecs PS_KEY_TEST_HFP_CODEC_PSKEY to be set.
 *
 *  \note Needs TEST_HFP_CODEC_PSKEY defined within the application.
 *        Device needs to be reset before changes take effect.
 *
 *  \return TRUE if value is written to PS_KEY_TEST_HFP_CODEC_PSKEY. FALSE if not.
 *          Function will print message to apps1 log.
*/
bool appTestCodecsAdvertisedHFPSet(uint16 codecs);

/*! \brief Get the PS_KEY_TEST_HFP_CODEC_PSKEY.
 *
 *  \param[out] set_codecs Pointer to variable to write PS_KEY_TEST_HFP_CODEC_PSKEY
 *                         bitmap of codecs to.
 *
 *  \note Needs TEST_HFP_CODEC_PSKEY defined within the application.
 *        If NULL pointer provided, and value is read, the value will be printed
 *        to the log but not stored in pointer.
 *
 *  \return TRUE if value is read and value stored in pointer.
 *          FALSE if value is not read and/or NULL pointer provided.
 *          Function will print message to apps1 log.
*/
bool appTestCodecsAdvertisedHFPGet(uint16* set_codecs);


/*! \brief Enable Adaptive Latency automatic adjustments.
    \note This API is deprecated, and no longer has any effect.
          AptX Adaptive latency is now controlled directly by the encoder. */
void appTestLatencyAdaptiveEnable(void);

/*! \brief Disable Adaptive Latency automatic adjustments.
    \note This API is deprecated, and no longer has any effect.
          AptX Adaptive latency is now controlled directly by the encoder. */
void appTestLatencyAdaptiveDisable(void);

/*! \brief Set the Target Latency

    \param[in] target_latency The target latency
*/
void appTestLatencyTargetSet(uint16 target_latency);

/*! \brief Get the Target Latency

    \return[out] The target latency
*/
uint16 appTestLatencyTargetGet(void);


/*! \brief Set the system back to the default target latency */
void appTestLatencyTargetDefault(void);
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */


/* 
** Power and Battery
*/

/*! \brief Reboot the USB dongle */
void appTestDonglePowerReboot(void);


/*! \brief Returns the current battery voltage */
uint16 appTestBatteryGetVoltage(void);

/*! \brief Sets the battery voltage to be returned by
    appBatteryGetVoltage().

    This test function also causes the battery module to read
    this new voltage level - which may be subjected to hysteresis.
 */
void appTestBatterySetVoltage(uint16 new_level);

/*! \brief Return TRUE if the current battery state is battery_level_ok. */
bool appTestBatteryStateIsOk(void);

/*! \brief Return TRUE if the current battery state is battery_level_low. */
bool appTestBatteryStateIsLow(void);

/*! \brief Return TRUE if the current battery state is battery_level_critical. */
bool appTestBatteryStateIsCritical(void);

/*! \brief Return TRUE if the current battery state is battery_level_too_low. */
bool appTestBatteryStateIsTooLow(void);

/*! \brief Return the number of milliseconds taken for the battery measurement
    filter to fully respond to a change in input battery voltage */
uint32 appTestBatteryFilterResponseTimeMs(void);

#ifdef HAVE_THERMISTOR
/*! \brief Returns the expected thermistor voltage at a specified temperature.
    \param temperature The specified temperature in degrees Celsius.
    \return The equivalent milli-voltage.
*/
uint16 appTestBatteryThermistorDegreesCelsiusToMillivolts(int8 temperature);
#endif


/*
** Lid
*/

/*! \brief Return TRUE if the lid is open */
bool appTestLidIsOpen(void);

/*! \brief Return TRUE if the lid is closed */
bool appTestLidIsClosed(void);

/*! \brief Send a lid open event */
void appTestLidSetOpen(void);

/*! \brief Send a lid closed event */
void appTestLidSetClosed(void);


/* 
** Handset/DFU
*/

#ifdef INCLUDE_DFU
/*! \brief Allow handsets to pair for DFU for a limited time */
void appTestDfuHandsetPairingStart(void);

/*! \brief Disallow handset pairing/connections */
void appTestDfuHandsetPairingCancel(void);
#endif /* INCLUDE_DFU */


/* 
** USB
*/

/*! \brief To Simulate USB Host disconnection even when device connected to host via USB
     This is done by disconnecting USB application device with internal hub

    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbDetach(void);

/*! \brief To Simulate USB Host re-connection by re-connecting USB application device with internal hub

    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbAttach(void);

/*! \brief Query the connected inputs for usb input

    \return True if the connected input bitmask contains the usb input
*/
bool appTestUsbIsInputConnected(void);

/*! \brief Send a HID consumer Play/Pause event over USB
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportPlayPause(void);

/*! \brief Send a HID consumer Play event over USB
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportPlay(void);

/*! \brief Send a HID consumer Pause event over USB
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportPause(void);

/*! \brief Send a HID consumer Volume Up event over USB
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportVolumeUp(void);

/*! \brief Send a HID consumer Volume Down event over USB
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportVolumeDown(void);

/*! \brief Send a HID consumer Mute event over USB
 *  \note This will cause the USB Host to mute its speaker volume
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportMute(void);

/*! \brief Send a HID Telephony Mute event over USB
 *  \note This will tell the USB Host that the Sink device has muted its microphone
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportPhoneMute(void);

/*! \brief Send a HID consumer Fast Forward ON event over USB
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportFfwdOn(void);

/*! \brief Send a HID consumer Fast Forward OFF event over USB
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportFfwdOff(void);

/*! \brief Send a HID consumer Rewind ON event over USB
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportRewOn(void);

/*! \brief Send a HID consumer Rewind OFF event over USB
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportRewOff(void);

/*! \brief Send a HID consumer Next Track event over USB
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportNextTrack(void);

/*! \brief Send a HID consumer Previous Track event over USB
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportPreviousTrack(void);

/*! \brief Send a HID consumer Stop event over USB
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportStop(void);

/*! \brief Send a HID Telephony Report Hook Switch On event over USB
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportHookSwitchAnswer(void);

/*! \brief Send a HID Telephony Report Hook Switch Off event over USB
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportHookSwitchTerminate(void);

/*! \brief Send a HID Telephony Report Programmable button 1 event over USB
    \note This will normally reject incomming calls on the PC
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportButtonOne(void);

/*! \brief Send a HID Telephony Report Flash event over USB
    \note This will normally resume calls on hold
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportFlash(void);

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
/*! \brief Connect HFP
    \return TRUE if operation is successful otherwise False
*/
bool appTestHfpConnect(void);

/*! \brief Disconnect HFP
*/
void appTestHfpDisconnect(void);

/*! \brief Start an incoming HFP call
    \return TRUE on success
*/
bool appTestHfpIncomingCall(void);

/*! \brief Start an incoming call
    \param nap Nap portion of BT address
    \param uap Uap portion of BT address
    \param lap Lap portion of BT address
*/
void appTestHfpIncomingCallWithAddr(uint16 nap, uint8 uap, uint32 lap);

/*! \brief Answer Incoming HFP
    \return Answer incoming HFP call
*/
bool appTestHfpAnswerIncoming(void);

/*! \brief Reject incoming HFP
    \return Reject incoming HFP call
*/
bool appTestHfpCallReject(void);
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

/*! \brief Telephony rining message
    \return mimic an incoming ringing message from USB
*/
bool appTestTelephonyUsbRingingStart(void);

/*! \brief Telephony rining message
    \return mimic an incoming stop ringing message from USB
*/
bool appTestTelephonyUsbRingingStop(void);

/*! \brief Telephony rining message
    \return mimic an on hook call
*/
bool appTestTelephonyUsbOnHook(void);

/*! \brief Telephony rining message
    \return mimic an off hook call
*/
bool appTestTelephonyUsbOffHook(void);

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
/*! \brief Reject ongoing call
    \return Hangup an ongoing call
*/
void appTestHfpTerminateOngoingCall(void);

/*! \brief Send cellular network registration status/availabllity to the HF
    \return TRUE if successful
*/
bool appTestHfpSendRegistrationStatus(bool availability);

/*! \brief Send cellular network signal strength to the HF
    \return TRUE if successful
*/
bool appTestHfpSendSignalStrengthIndication(uint16 level);

/*! \brief Connect HFP SCO link
    \return TRUE if successful
*/
bool appTestHfpConnectSCO(void);

/*! \brief Disconnect HFP SCO link
    \return TRUE if successful
*/
bool appTestHfpDisconnectSCO(void);


/*! \brief To Enable or Disable automatic outgoing audio connection initiation during incoming call, for PTS testing
    \return TRUE if successful
*/
bool appTestHfpSetAutomaticOutgoingAudioConnection(bool enable);

/*! \brief To Enable or Disable retain instance after HF disconnection, for PTS testing
    \return TRUE if successful
*/
bool appTestHfpRetainInstanceAfterDisconnection(bool enable);

/*! \brief Set the default phone number to be sent to the HF device
*/
void appTestHfpSetClip(void);

/*! \brief Set the supplied phone number to be sent to the HF device
 * \param number Phone number to be sent in the CLIP message
*/
bool appTestHfpSetClipWithNumber(char *number);

/*! \brief Clear the phone number that is being sent to the HF device
*/
void appTestHfpClearClip(void);

/*! \brief Set the network operator string to be sent to the HF
*/
void appTestHfpSetNetworkOperator(void);

/*! \brief Clear the network operator string to be sent to the HF
*/
void appTestHfpClearNetworkOperator(void);

/*! \brief Hold an ongoing active call
 *  \return TRUE if successful
*/
bool appTestHfpSetCallOnHold(void);

/*! \brief Release an ongoing held call
 *  \return TRUE if successful
*/
bool appTestHfpReleaseHeldCall(void);

/*! \brief Send a battery level indication to the HF
 * \param level The level to be sent to the HF
 * \return TRUE if successful
*/
bool appTestHfpSendBatteryLevelIndication(uint16 level);

/*! \brief Answer an outgoing call
 * \return TRUE if successful
*/
bool appTestHfpAnswerOutgoing(void);

/*! \brief Print out current call list
*/
void appTestHfpPrintCallList(void);

/*! \brief Clear call history
*/
void appTestHfpClearCallHistory(void);

/*! \brief Set last dialled number
*/
void appTestHfpSetLastDialledNumber(void);

/*! \brief Set HFP Speaker Volume
*/
bool appTestHfpSetVolume(uint8 volume);

/*! \brief Set HFP Mic Gain
*/
bool appTestHfpSetMicGain(uint8 gain);
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
/*! \brief Send the Unicast absolute set volume command to the Remote device.
*/
void appTestSetUnicastVolume(uint32 cid, uint8 volume);

/*! \brief Get the unicast volume in given cid

    \param cid The GATT connection on which the get volume needs to be performed

    \return Return the unicast volume
*/
uint8 appTestGetUnicastVolume(uint32 cid);

/*! \brief Increase the unicast volume for given cid by step of 10.
*/
void appTestUnicastSourceVolumeUp(uint32 cid);

/*! \brief Decrement the unicast volume for given cid by step of 10.
*/
void appTestUnicastSourceVolumeDown(uint32 cid);

/*! \brief Mute/Unmute the Unicast audio.
*/
void appTestMuteUnicastAudio(uint32 cid, bool mute);

/*! \brief Check if unicast is streaming
    \return True if streaming False Otherwise.
*/
bool appTestIsUnicastStreaming(uint32 cid);

/*! \brief Get Lea unicast audio context
    \return Lea Audio context
*/
uint16 appTestUnicastGetAudioContext(void);

/*! \brief Get the negotiated Codec ID if streaming, else unknown codec id 0x00.

    \return It is 32 bit value based on BAP codec ID as coded below:
             codecId            -  8 bit (LSB)
             codecVersionNum    -  8 bit
             vendorCodecId      - 16 bit (MSB)
*/
uint32 appTestUnicastGetCodecId(void);

/*! \brief Enable LEA Gaming mode
*/
void appTestUnicastGamingModeEnable(void);

/*! \brief Disable LEA Gaming mode
*/
void appTestUnicastGamingModeDisable(void);

/*! \brief Check if LEA Gaming mode is enabled
    \return True if enabled False Otherwise.
*/
bool appTestIsUnicastGamingModeEnabled(void);

/*! \brief Check if CCP profile is connected or not

    \return True if connected, False otherwise.
*/
bool appTestIsCcpConnected(void);

/*! \brief Check if MCP profile is connected or not

    \return True if connected, False otherwise.
*/
bool appTestIsMcpConnected(void);

/*! \brief Get the current MCP media state

    \return Media state
                0 - Media is inactive
                1 - Media is playing
                2 - Media is paused
*/
uint8 appTestGetCurrentMcpMediaState(void);

/*! \brief Set the lock in given device

    \param lock_enabled TRUE to set the lock, FALSE to release

    \return TRUE if able to place request, FALSE otherwise
*/
bool appTestCsipSetLock(uint32 cid, bool lock_enabled);

/*! \brief Set the lock in all the discovered set members

    \param lock_enabled TRUE to set the lock, FALSE to release

    \return TRUE if able to place request, FALSE otherwise
*/
bool appTestCsipSetLockForAll(bool lock_enabled);

/*! \brief Read the set size parameter of the coordinated set

    \return TRUE if able to read, FALSE otherwise
*/
bool appTestCsipReadSetSize(void);

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
/*! \brief Enable broadcast mode
*/
void appTestBroadcastModeEnable(void);

/*! \brief Disable broadcast mode
*/
void appTestBroadcastModeDisable(void);

/*! \brief Check if dongle is in broadcast mode

    \return True if in broadcast mode, False if in unicast mode.
*/
bool appTestIsInBroadcastMode(void);

/*! \brief Used to set broadcast mode as PBP or TMAP

    \param pbp_enable TRUE to set to PBP broadcast mode
                      FALSE to set to default TMAP broadcast mode
*/
void appTestSetPbpBroadcastMode(bool pbp_enable);

/*! \brief Check if broadcast is configured in PBP mode or not

    \return TRUE if in broadcast mode, FALSE otherwise
*/
bool appTestIsInPbpBroadcastMode(void);

/*! \brief Sets the broadcast code for the streaming.

     \param code Pointer to the 16 byte broadcast code. If not encrypted, NULL can be passed here.
     \param len Length of broadcast code. Maximum length upto 16 characters can be given.
*/
void appTestSetBroadcastCode(const uint8 *code, uint8 len);

/*! \brief  Sets the broadcast source name.

     \param name Pointer to the broadcast source name.
     \param len Length of broadcast source name. Maximum length upto 30 characters can be given.

     \return TRUE if able to set the name, returns FALSE otherwise.
*/
bool appTestSetBroadcastSourceName(const char *name, uint8 len);

/*! \brief  Sets the broadcast source configuration

     \param broadcast_stream_capability BAP configuration TmapClientStreamCapability.
     \param is_public_broadcast Is this for public broadcast source

     \return TRUE if able to set, returns FALSE otherwise.
*/
bool appTestSetBroadcastConfiguration(uint32 stream_capability, bool pbp_enable);

/*! \brief  Sets the broadcast BIS configuration.

     \param number_of_bis Number of BISes to configure.
     \param audio_channels_per_bis Audio channels per BIS (to determine BAP broadcast configs)

     \return TRUE if able to set, returns FALSE otherwise.
*/
bool appTestSetBroadcastBisConfiguration(uint32 number_of_bis, uint32 no_of_channels_per_bis);

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

/*! \brief Get the currently active speaker path sample rate used by the LEA profiles for streaming
           (either unicast/ broadcast)

    \return sample rate in Hz. Returns 0 if no streaming is ongoing
*/
uint32 appTestGetLeaActiveSpkrPathSampleRate(void);

/*! \brief Get the currently active mic path sample rate used by the LEA profiles for streaming

    \return sample rate in Hz. Returns 0 if there is no usecase for mic running.
            ie, if no gaming + VBC or conversational streaming
*/
uint32 appTestGetLeaActiveMicPathSampleRate(void);

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

/*! \brief  Get the current transport mode of the dongle

    \return transport mode ( 1-BREDR, 2-LE, 3-Dual)
*/
usb_dongle_transport_mode_t appTestGetTransportMode(void);

/*! \brief  Set the transport mode of the dongle

    \param transport_mode
*/
void appTestSetTransportMode(usb_dongle_transport_mode_t transport_mode);

/*! \brief  Get the current audio mode of the dongle

    \return audio mode ( 0-High quality, 1-Gaming, 2-Broadcast)
*/
usb_dongle_audio_mode_t appTestGetAudioMode(void);

/*! \brief  Get the transport type of the connected sink

    \return connected transport (0 - Not connected, 1-BREDR, 2-LE)
*/
sink_service_transport_t appTestGetConnectedTransport(void);

#ifdef USE_SYNERGY
/*! \brief Prints security keys of all bonded devices
*/
int appTestGetKeys(void);
#endif

#endif /* USB_DONGLE_TEST_H */
