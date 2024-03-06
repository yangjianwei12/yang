/*!
\copyright  Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of specific application testing functions
*/

#include "charger_case_test.h"
#include <av.h>
#include "logging.h"
#include "usb_application.h"
#include "usb_source_hid.h"
#include "charger_monitor.h"
#include "charger_case_sm.h"
#include "device_list.h"
#include "device_db_serialiser.h"
#include "logical_input_switch.h"
#include "device_properties.h"
#include "profile_manager.h"
#include "panic.h"
#include "charger_case_sm_private.h"
#include "connection_manager.h"
#include "av.h"
#include "charger_case_audio.h"
#include "charger_case_a2dp_source.h"
#include "charger_case_dfu.h"
#include "power_manager.h"
#include "system_reboot.h"
#include "system_state.h"
#include "pairing.h"
#include "common_test.h"

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#ifdef INCLUDE_WIRED_ANALOG_AUDIO
#include "wired_audio_private.h"
#endif

#define TEST_NOT_IMPLEMENTED()   DEBUG_LOG_ALWAYS("Test \"%s\" not implemented.", __func__ )

static struct
{
    uint16 testcase;
    uint16 iteration;
    uint16 last_marker;
} test_support = {0};

/*
** Test Infrastructure
*/

uint16 appTestSetTestNumber(uint16 test_number)
{
    test_support.testcase = test_number;

    return test_number;
}

uint16 appTestSetTestIteration(uint16 test_iteration)
{
    test_support.iteration = test_iteration;

    return test_iteration;
}

uint16 appTestWriteMarker(uint16 marker)
{
    static unsigned testcase = 0;
    test_support.last_marker = marker;

    if (   test_support.testcase
        && (  test_support.testcase != testcase
           || marker == 0))
    {
        testcase = test_support.testcase;

        if (test_support.iteration)
        {
            DEBUG_LOG_ALWAYS("@@@Testcase:%d  Iteration:%d -------------------------------", testcase, test_support.iteration);
        }
        else
        {
            DEBUG_LOG_ALWAYS("@@@Testcase:%d  ------------------------------------------", testcase);
        }
    }

    if (marker)
    {
        if (test_support.testcase && test_support.iteration)
        {
            DEBUG_LOG_ALWAYS("@@@Testcase marker: TC%d Iteration:%d Step:%d *************************",
                    testcase, test_support.iteration, marker);
        }
        else if (test_support.testcase)
        {
            DEBUG_LOG_ALWAYS("@@@Testcase marker: TC%d Step:%d *************************",
                    testcase, marker);
        }
        else
        {
            DEBUG_LOG_ALWAYS("@@@Testcase marker: Iteration:%d Step:%d *************************",
                    test_support.iteration, marker);
        }
    }

    return marker;
}

/*
** System and State
*/
static bool reset_happened = TRUE;

bool appTestApplicationIsInitialisationCompleted(void)
{
    bool completed = SystemState_GetState() > system_state_initialised;

    DEBUG_LOG_ALWAYS("appTestIsInitialisationCompleted:%d", completed);

    return completed;
}

/*! \brief Reset charger case to factory defaults.
    Will drop any connections, delete all pairing and reboot.
*/
void appTestFactoryReset(void)
{
    DEBUG_LOG_ALWAYS("appTestFactoryReset");
    reset_happened = FALSE;
    Ui_InjectUiInput(ui_input_factory_reset_request);
}

bool appTestResetHappened(void)
{
    bool result = reset_happened;

    DEBUG_LOG_ALWAYS("appTestResetHappened: %d", result);

    if(reset_happened) {
        reset_happened = FALSE;
    }

    return result;
}

bool appTestPtsModeIsEnabled(void)
{
    TEST_NOT_IMPLEMENTED();
    return FALSE;
}

void appTestPtsModeEnable(void)
{
    TEST_NOT_IMPLEMENTED();
}

void appTestPtsModeDisable(void)
{
    TEST_NOT_IMPLEMENTED();
}

bool appTestApplicationIsState(charger_case_state_t checked_state)
{
    charger_case_state_t state = ChargerCaseSm_GetState();

    DEBUG_LOG_ALWAYS("appTestApplicationIsState. Current State:"
                     " enum:charger_case_state_t:%d", state);

    return (state == checked_state);
}

charger_case_state_t appTestApplicationGetState(void)
{
    return ChargerCaseSm_GetState();
}

void appTestDeviceDatabaseReport(void)
{
    appTest_DeviceDatabaseReport();
}

uint32 appTestDspClockSpeedInActiveMode(void)
{
    TEST_NOT_IMPLEMENTED();
    return 0;
}


/*
** Analog Input
*/
#ifdef INCLUDE_A2DP_ANALOG_SOURCE

void appTestWiredAnalogAudioPlugged(void)
{
    TEST_NOT_IMPLEMENTED();
}

void appTestWiredAnalogAudioUnplugged(void)
{
    TEST_NOT_IMPLEMENTED();
}

bool appTestWiredAnalogIsInputConnected(void)
{
    return ChargerCase_AudioInputIsConnected(charger_case_audio_input_analogue);
}

#endif /* INCLUDE_WIRED_ANALOG_AUDIO */


/*
** Earbud Physical
*/
bool appTestEarbudLeftIsInCase(void)
{
    TEST_NOT_IMPLEMENTED();
    return FALSE;
}

bool appTestEarbudRightIsInCase(void)
{
    TEST_NOT_IMPLEMENTED();
    return FALSE;
}

void appTestEarbudLeftInCaseEvent(void)
{
    TEST_NOT_IMPLEMENTED();
}

void appTestEarbudRightInCaseEvent(void)
{
    TEST_NOT_IMPLEMENTED();
}

void appTestEarbudLeftOutOfCaseEvent(void)
{
    TEST_NOT_IMPLEMENTED();
}

void appTestEarbudRightOutOfCaseEvent(void)
{
    TEST_NOT_IMPLEMENTED();
}


/*
** Earbud Pairing
*/

void appTestDeletePairing(void)
{
    DEBUG_LOG_ALWAYS("appTestDeletePairing");
    if (appTestEarbudIsConnected())
    {
        MESSAGE_MAKE(msg,SM_INTERNAL_DISCONNECT_SINK_DEVICE_T );
        msg->request_pairing_delete = TRUE;
        MessageSend(ChargerCaseSmGetTask(), SM_INTERNAL_DISCONNECT_SINK_DEVICE, msg);
    }
    else
    {
        BtDevice_DeleteAllDevicesOfType(DEVICE_TYPE_SINK);
        ConnectionSmDeleteAllAuthDevices(0);
    }
}

static device_t chargerCaseTest_GetEarbud(void)
{
    uint8 type_sink = DEVICE_TYPE_SINK;

    device_t earbud_device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_type, &type_sink, sizeof(uint8));

    return earbud_device;
}

/*
** Earbud to Case Pairing
*/

bool appTestEarbudPair(uint16 nap, uint8 uap, uint32 lap)
{
    if (BtDevice_IsPairedWithSink())
    {
        DEBUG_LOG_ALWAYS("appTestEarbudPair: An earbud is already paired");
        return FALSE;
    }

    if (ChargerCaseSm_GetState() != CHARGER_CASE_STATE_IDLE)
    {
        DEBUG_LOG_ALWAYS("appTestEarbudPair: Charger Case not idle");
        return FALSE;
    }

    if (PairingGetTaskData()->state >= PAIRING_STATE_PENDING_AUTHENTICATION )
    {
        DEBUG_LOG_ALWAYS("appTestEarbudPair: Charger Case already pairing");
        return FALSE;
    }

    bdaddr earbud_address;
    BdaddrSetZero(&earbud_address);

    earbud_address.nap = nap;
    earbud_address.uap = uap;
    earbud_address.lap = lap;

    DEBUG_LOG_ALWAYS("appTestEarbudPair: bdaddr 0x%04x 0x%02x 0x%06lx",
                     earbud_address.nap,
                     earbud_address.uap,
                     earbud_address.lap);

    Pairing_PairAddress(&a2dp_source_data.task, &earbud_address);
    a2dp_source_data.state = CHARGER_CASE_A2DP_SOURCE_STATE_CONNECTING;
    return TRUE;
}

bool appTestEarbudIsPaired(void)
{
    bool paired = BtDevice_IsPairedWithSink();

    DEBUG_LOG_ALWAYS("appTestEarbudIsPaired:%d",paired);

    return paired;
}

void appTestEarbudPairingRssiStart(void)
{
    MessageSend(ChargerCaseSmGetTask(), SM_INTERNAL_START_PAIRING, NULL);
}

void appTestEarbudPairingRssiStop(void)
{
    DEBUG_LOG_ALWAYS("appTestEarbudCasePairingRssiStop");
    MessageSend(ChargerCaseSmGetTask(), SM_INTERNAL_STOP_PAIRING, NULL);
}

void appTestEarbudPairingRssiDisable(void)
{
    TEST_NOT_IMPLEMENTED();
}

void appTestEarbudPairingRssiEnable(void)
{
    TEST_NOT_IMPLEMENTED();
}

void appTestEarbudDisconnect(void)
{
    MessageSend(ChargerCaseSmGetTask(), SM_INTERNAL_DISCONNECT_SINK_DEVICE, NULL);
}

void appTestEarbudConnect(void)
{
    MessageSend(ChargerCaseSmGetTask(), SM_INTERNAL_CONNECT_SINK_DEVICE, NULL);
}

bool appTestEarbudIsConnected(void)
{
    bool connected = appTestEarbudIsA2dpConnected() ||
                     appTestEarbudIsAvrcpConnected() ||
                     appTestEarbudIsHfpConnected();

    DEBUG_LOG_ALWAYS("appTestEarbudIsConnected = %d", connected);

    return connected;
}

bool appTestEarbudIsA2dpConnected(void)
{
    bool connected = FALSE;
    device_t device  = chargerCaseTest_GetEarbud();
    if (!device)
    {
        return FALSE;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);
    /* Find earbud AV instance */
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
    connected = theInst && appA2dpIsConnected(theInst);

    DEBUG_LOG_ALWAYS("appTestEarbudIsA2dpConnected:%d", connected);

    /* If we get here then there's no A2DP connected for earbud */
    return connected;
}

bool appTestEarbudIsAvrcpConnected(void)
{

    bool connected = FALSE;
    device_t device  = chargerCaseTest_GetEarbud();
    if (!device)
    {
        return FALSE;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);

    /* Find earbud AV instance */
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
    connected = theInst && appAvrcpIsConnected(theInst);

    DEBUG_LOG_ALWAYS("appTestEarbudIsAvrcpConnected = %d", connected);

    /* If we get here then there's no AVRCP connected for earbud */
    return connected;

}


bool appTestEarbudIsHfpConnected(void)
{
    return FALSE;
}

bool appTestEarbudIsA2dpPlaying(void)
{
    TEST_NOT_IMPLEMENTED();
    return FALSE;
}

bool appTestEarbudIsA2dpMediaConnected(void)
{
    bool connected = FALSE;
    device_t device  = chargerCaseTest_GetEarbud();
    if (!device)
    {
        return FALSE;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);

        /* Find earbud AV instance */
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
    connected = theInst && appA2dpIsConnectedMedia(theInst);


    DEBUG_LOG_ALWAYS("appTestEarbudIsA2dpMediaConnected:%d", connected);

    return connected;
}

bool appTestEarbudIsA2dpStreaming(void)
{
    bool streaming = FALSE;
    device_t device  = chargerCaseTest_GetEarbud();
    if (!device)
    {
        return FALSE;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);

        /* Find earbud AV instance */
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
    streaming = theInst && appA2dpIsStreaming(theInst);


    DEBUG_LOG_ALWAYS("appTestEarbudIsA2dpStreaming:%d", streaming);

    return streaming;
}

bool appTestEarbudChangeAvVolume(int8 step)
{
    UNUSED(step);
    TEST_NOT_IMPLEMENTED();
    return FALSE;
}

void appTestEarbudSetAvVolume(uint8 volume)
{
    UNUSED(volume);
    TEST_NOT_IMPLEMENTED();
}


/*
** CODEC tests
*/

app_test_a2dp_codec_t appTestCodecGetType(void)
{
    device_t device  = chargerCaseTest_GetEarbud();
    if (!device)
    {
        return FALSE;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);

    /*Find Earbud AV instance*/
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);

    if(theInst)
    {
        switch(theInst->a2dp.current_seid)
        {
            case AV_SEID_SBC_SNK:
                return a2dp_codec_sbc;

            case AV_SEID_AAC_SNK:
                return a2dp_codec_aac;

            case AV_SEID_APTX_SNK:
                return a2dp_codec_aptx;

            case AV_SEID_APTX_ADAPTIVE_SNK:
                return a2dp_codec_aptx_adaptive;

            default:
                break;
        }
    }

    return a2dp_codec_unknown;
}

void appTestCodecAptxSetHq(void)
{
    TEST_NOT_IMPLEMENTED();
}

void appTestCodecAptxSetLl(void)
{
    TEST_NOT_IMPLEMENTED();
}

void appTestAptxLatencyAdjustEnable(void)
{
    TEST_NOT_IMPLEMENTED();
}

void appTestAptxLatencyAdjustDisable(void)
{
    TEST_NOT_IMPLEMENTED();
}

void appTestLatencySetTarget(uint16 latency)
{
    UNUSED(latency);
    TEST_NOT_IMPLEMENTED();
}

void appTestLatencySetMax(uint16 latency)
{
    UNUSED(latency);
    TEST_NOT_IMPLEMENTED();
}

void appTestLatencySetMin(uint16 latency)
{
    UNUSED(latency);
    TEST_NOT_IMPLEMENTED();
}

uint16 appTestLatencyGetMax(void)
{
    TEST_NOT_IMPLEMENTED();
    return 0;
}

uint16 appTestLatencyGetMin(void)
{
    TEST_NOT_IMPLEMENTED();
    return 0;
}

uint16 appTestLatencyGetTarget(void)
{
    TEST_NOT_IMPLEMENTED();
    return 0;
}


/*
** Power and Battery
*/
void appTestCasePowerReboot(void)
{
    DEBUG_LOG_ALWAYS("appTestCasePowerOff");
    SystemReboot_Reboot();
}

uint16 appTestBatteryGetVoltage(void)
{
    TEST_NOT_IMPLEMENTED();
    return 0;
}

void appTestBatterySetVoltage(uint16 new_level)
{
    TEST_NOT_IMPLEMENTED();
    UNUSED(new_level);
}

bool appTestBatteryStateIsOk(void)
{
    TEST_NOT_IMPLEMENTED();
    return FALSE;
}

bool appTestBatteryStateIsLow(void)
{
    TEST_NOT_IMPLEMENTED();
    return FALSE;
}

bool appTestBatteryStateIsCritical(void)
{
    TEST_NOT_IMPLEMENTED();
    return FALSE;
}

bool appTestBatteryStateIsTooLow(void)
{
    TEST_NOT_IMPLEMENTED();
    return FALSE;
}

uint32 appTestBatteryFilterResponseTimeMs(void)
{
    TEST_NOT_IMPLEMENTED();
    return 0;
}

#ifdef HAVE_THERMISTOR
/*! \brief Returns the expected thermistor voltage at a specified temperature.
    \param temperature The specified temperature in degrees Celsius.
    \return The equivalent milli-voltage.
*/
uint16 appTestBatteryThermistorDegreesCelsiusToMillivolts(int8 temperature)
{
    UNUSED(temperature);
    TEST_NOT_IMPLEMENTED();
    return 0;
}
#endif


/*
** Lid
*/

bool appTestLidIsOpen(void)
{
    TEST_NOT_IMPLEMENTED();
    return FALSE;
}

bool appTestLidIsClosed(void)
{
    TEST_NOT_IMPLEMENTED();
    return FALSE;
}

void appTestLidSetOpen(void)
{
    TEST_NOT_IMPLEMENTED();
}

void appTestLidSetClosed(void)
{
    TEST_NOT_IMPLEMENTED();
}


/*
** Earbud tests
*/

bool appTestEarbudIsAclConnected(void)
{
    bool connected = FALSE;
    device_t earbud_device = chargerCaseTest_GetEarbud();

    if (earbud_device)
    {
        bdaddr addr = DeviceProperties_GetBdAddr(earbud_device);
        connected = ConManagerIsConnected(&addr);
        DEBUG_LOG_ALWAYS("appTestEarbudIsAclConnected:%d 0x%06x", connected, addr.lap);
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestEarbudIsAclConnected:0 No earbud device/addr");
    }
    return connected;
}


/*
** Handset/DFU Tests
*/

#ifdef INCLUDE_DFU
void appTestDfuHandsetPairingStart(void)
{
    ChargerCaseDfu_HandsetPairingStart();
}

void appTestDfuHandsetPairingCancel(void)
{
    ChargerCaseDfu_HandsetPairingCancel();
}
#endif /* INCLUDE_DFU */


/*
** USB tests
*/
bool appTestUsbDetach(void)
{
    if(UsbApplication_IsAttachedToHub())
    {
        UsbDevice_HandleMessage(MESSAGE_USB_DETACHED, NULL);
        UsbApplication_Detach();
        return TRUE;
    }

    return FALSE;
}

bool appTestUsbAttach(void)
{
    if(!UsbApplication_IsAttachedToHub())
    {
        UsbApplication_Attach();
        return TRUE;
    }

    return FALSE;
}

bool appTestUsbIsInputConnected(void)
{
    return ChargerCase_AudioInputIsConnected(charger_case_audio_input_usb);
}

bool appTestUsbHidTransportPlayPause(void)
{
    return UsbSource_SendEvent(USB_SOURCE_PLAY_PAUSE);
}

bool appTestUsbHidTransportPlay(void)
{
    return UsbSource_SendEvent(USB_SOURCE_PLAY);
}

bool appTestUsbHidTransportPause(void)
{
    return UsbSource_SendEvent(USB_SOURCE_PAUSE);
}

bool appTestUsbHidTransportVolumeUp(void)
{
    return UsbSource_SendEvent(USB_SOURCE_VOL_UP);
}

bool appTestUsbHidTransportVolumeDown(void)
{
    return UsbSource_SendEvent(USB_SOURCE_VOL_DOWN);
}

bool appTestUsbHidTransportMute(void)
{
    return UsbSource_SendEvent(USB_SOURCE_MUTE);
}

bool appTestUsbHidTransportFfwdOn(void)
{
    return UsbSource_SendEvent(USB_SOURCE_FFWD_ON);
}

bool appTestUsbHidTransportFfwdOff(void)
{
    return UsbSource_SendEvent(USB_SOURCE_FFWD_OFF);
}

bool appTestUsbHidTransportRewOn(void)
{
    return UsbSource_SendEvent(USB_SOURCE_REW_ON);
}

bool appTestUsbHidTransportRewOff(void)
{
    return UsbSource_SendEvent(USB_SOURCE_REW_OFF);
}

bool appTestUsbHidTransportNextTrack(void)
{
    return UsbSource_SendEvent(USB_SOURCE_NEXT_TRACK);
}

bool appTestUsbHidTransportPreviousTrack(void)
{
    return UsbSource_SendEvent(USB_SOURCE_PREVIOUS_TRACK);
}

bool appTestUsbHidTransportStop(void)
{
    return UsbSource_SendEvent(USB_SOURCE_STOP);
}

bool appTestUsbHidTransportHookSwitchAnswer(void)
{
    return UsbSource_SendEvent(USB_SOURCE_HOOK_SWITCH_ANSWER);
}

bool appTestUsbHidTransportHookSwitchTerminate(void)
{
    return UsbSource_SendEvent(USB_SOURCE_HOOK_SWITCH_TERMINATE);
}

#ifdef INCLUDE_CHARGER

void appTestChargerStatus(charger_status chg_status)
{
    Charger_TestModeControl(TRUE);
    Charger_TestChargerStatus(chg_status);
}

void appTestChargerDetected(usb_attached_status attached_status,
                                       uint16 charger_dp_millivolts,
                                       uint16 charger_dm_millivolts,
                                       usb_type_c_advertisement cc_status)
{
    Charger_TestModeControl(TRUE);
    Charger_TestChargerDetected(attached_status,
                                charger_dp_millivolts,
                                charger_dm_millivolts,
                                cc_status);
}

void appTestChargerConnected(bool is_connected)
{
    Charger_TestModeControl(TRUE);
    Charger_TestChargerConnected(is_connected);
}

void appTestChargerRestore(void)
{
    Charger_TestModeControl(FALSE);
}

#endif /* INCLUDE_CHARGER */
