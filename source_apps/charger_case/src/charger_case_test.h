/*!
\copyright  Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for test module
*/
#ifndef CHARGER_CASE_TEST_H
#define CHARGER_CASE_TEST_H

#include "charger_case_sm.h"

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

/*! \brief Are we running in PTS test mode */
bool appTestPtsModeIsEnabled(void);

/*! \brief Enable the PTS test mode */
void appTestPtsModeEnable(void);

/*! \brief Disable the PTS test mode */
void appTestPtsModeDisable(void);

/*! Check if the application state machine is in the specified role

    The states are defined in charger_case_sm.h, and can be accessed from
    python by name - example
    \code
      apps1.fw.env.enums["chargerCaseState"]["CHARGERCASE_STATE_POWERING_ON"])
    \endcode

    \return TRUE The charger case state machine is in the specified state
*/
bool appTestApplicationIsState(charger_case_state_t checked_state);

/*! \brief Get the application state

    \return Returns the enum for the application state
*/
charger_case_state_t appTestApplicationGetState (void);

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

#ifdef INCLUDE_A2DP_ANALOG_SOURCE

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
** Earbud Physical
*/

/*! \brief Return TRUE if the Left earbud is in the case. */
bool appTestEarbudLeftIsInCase(void);

/*! \brief Return TRUE if the Right earbud is in the case. */
bool appTestEarbudRightIsInCase(void);

/*! \brief Send a Left earbud "in case" event */
void appTestEarbudLeftInCaseEvent(void);

/*! \brief Send a Right earbud "in case" event */
void appTestEarbudRightInCaseEvent(void);

/*! \brief Send a Left earbud "out of case" event */
void appTestEarbudLeftOutOfCaseEvent(void);

/*! \brief Send a Right earbud "out of case" event */
void appTestEarbudRightOutOfCaseEvent(void);


/*
** Earbud to Case Pairing
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
    \return FALSE if the pairing module or the charger case SM are
            in pairing mode, an earbud is already paired, or the
            charger case is not idle
*/
bool appTestEarbudPair(uint16 nap, uint8 uap, uint32 lap);

/*! \brief Return True if there is an Earbud in the paired device list */
bool appTestEarbudIsPaired(void);

/*! \brief Disable RSSI pairing */
void appTestEarbudPairingRssiDisable(void);

/*! \brief Enable RSSI pairing */
void appTestEarbudPairingRssiEnable(void);

/*! \brief Start an RSSI scan */
void appTestEarbudPairingRssiStart(void);

/*! \brief Stop an ongoing RSSI pairing scan */
void appTestEarbudPairingRssiStop(void);

/*
** Earbud Bluetooth connections
*/

/*! \brief Connect to paired earbuds */
void appTestEarbudConnect(void);

/*! \brief Disconnect from the earbuds */
void appTestEarbudDisconnect(void);

/*! \brief Return if there is connection to the earbuds

    \return bool TRUE if connection
                 FALSE if no connection
*/
bool appTestEarbudIsConnected(void);

/*! \brief Return if charger case has an earbud A2DP connection

    \return bool TRUE charger case has A2DP earbud connection
                 FALSE charger case does not have A2DP earbud connection
*/
bool appTestEarbudIsA2dpConnected(void);

/*! \brief Return if charger case has an earbud AVRCP connection

    \return bool TRUE charger case has AVRCP earbud connection
                 FALSE charger case does not have AVRCP earbud connection
*/
bool appTestEarbudIsAvrcpConnected(void);

/*! \brief Return if charger case has an earbud HFP connection

    \return bool TRUE charger case has HFP earbud connection
                 FALSE charger case does not have HFP earbud connection
*/
bool appTestEarbudIsHfpConnected(void);

/*! \brief Return if application  has an ACL connection to the earbud

    It does not indicate if the earbud is usable, with profiles
    connected. Use appTestEarbudIsConnected.

    \return bool TRUE headset has an ACL connection
                 FALSE application does not have an ACL connection to the Handset
*/
bool appTestEarbudIsAclConnected(void);

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

/*! \brief Send relative AVRCP absolute volume change to the Earbud

    \param step : relative Step change to apply (+127 to -127)

    \return bool TRUE absolute AVRCP volume step change sent
                 FALSE absolute AVRCP volume step change was not sent
*/
bool appTestEarbudChangeAvVolume(int8 step);

/*! \brief Send the Avrcp absolute set volume command to the Earbud

    \param volume   New volume level to set (0-127).
*/
void appTestEarbudSetAvVolume(uint8 volume);


/*
** CODEC tests
*/

/*! \brief To get current A2DP codec
    The return value follows this enum
*/
typedef enum
{
    a2dp_codec_sbc,
    a2dp_codec_aac,
    a2dp_codec_aptx,
    a2dp_codec_aptx_adaptive,
    a2dp_codec_unknown,
} app_test_a2dp_codec_t;
app_test_a2dp_codec_t appTestCodecGetType(void);

/*! \brief Set aptX to HQ mode */
void appTestCodecAptxSetHq(void);

/*! \brief Set aptX to LL mode */
void appTestCodecAptxSetLl(void);

/*! \brief Enable aptX latency adjustments */
void appTestAptxLatencyAdjustEnable(void);

/*! \brief Disable aptX latency adjustments */
void appTestAptxLatencyAdjustDisable(void);

/*! \brief Adjust the max source latency value */
void appTestLatencySetMax(uint16);

/*! \brief Adjust the min source latency value */
void appTestLatencySetMin(uint16);

/*! \brief Adjust the target source latency value */
void appTestLatencySetTarget(uint16);

/*! \brief Get the max source latency */
uint16 appTestLatencyGetMax(void);

/*! \brief Get the min source latency */
uint16 appTestLatencyGetMin(void);

/*! \brief Get the target source latency */
uint16 appTestLatencyGetTarget(void);
/* 
** Power and Battery
*/

/*! \brief Reboot the charger case */
void appTestCasePowerReboot(void);


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
    \return TRUE if operation is successful otherwise False
*/
bool appTestUsbHidTransportMute(void);

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

#ifdef INCLUDE_CHARGER

/*! Simulate charger connected/disconnected events
 *
 * \param is_connected TRUE to simulate attach or FALSE for detach */
void appTestChargerConnected(bool is_connected);

/*! Simulate detection of particular charger
 *
 * \param attached_status type of charger detected
 * \param charger_dp_millivolts voltage measured on USB_DP line
 * \param charger_dm_millivolts voltage measured on USB_DM line
 * \param cc_status USB_CC line state
 * */
void appTestChargerDetected(usb_attached_status attached_status,
                                       uint16 charger_dp_millivolts,
                                       uint16 charger_dm_millivolts,
                                       usb_type_c_advertisement );

/*! Simulate charger status events
 *
 * \param chg_status charger status to simulate */
void appTestChargerStatus(charger_status chg_status);

/*! Return back to normal operation after appsTestCharger* calls. */
void appTestChargerRestore(void);

#endif /* INCLUDE_CHARGER */


#endif /* CHARGER_CASE_TEST_H */
