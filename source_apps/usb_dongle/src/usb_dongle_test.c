/*!
\copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of specific application testing functions
*/

#include "usb_dongle_logging.h"
#include "usb_dongle_config.h"

#include "usb_dongle_test.h"
#include <av.h>
#include "usb_application.h"
#include "usb_source_hid.h"
#include "usb_dongle_sm.h"
#include "device_list.h"
#include "device_db_serialiser.h"
#include "device_properties.h"
#include "profile_manager.h"
#include "panic.h"
#include "usb_dongle_sm_private.h"
#include "usb_dongle_volume_observer.h"
#include "connection_manager.h"
#include "av.h"
#include "kymera.h"
#include "usb_dongle_audio.h"
#include "power_manager.h"
#include "system_reboot.h"
#include "system_state.h"
#include "pairing.h"
#include "rssi_pairing.h"
#include "sink_service.h"
#include <ui.h>
#include <telephony_messages.h>
#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
#include "usb_dongle_a2dp.h"
#include "usb_dongle_hfp.h"
#include <aghfp_profile_abstraction.h>
#include <aghfp_profile_typedef.h>
#include <aghfp_profile_call_list.h>
#include <aghfp.h>
#include <avrcp_profile_metadata.h>
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
#include "usb_dongle_lea.h"
#include "le_audio_client.h"
#include "csip_client.h"
#include "gatt.h"
#include "usb_dongle_lea_config.h"
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

#include "common_test.h"

#include <stdio.h>
#include <ps.h>
#include <ps_key_map.h>
#include <av.h>
#include <avrcp_profile.h>

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#ifdef INCLUDE_WIRED_ANALOG_AUDIO
#include "wired_audio_private.h"
#endif

#define TEST_NOT_IMPLEMENTED()  DEBUG_LOG_ALWAYS("Test \"%s\" not implemented.", __func__ )
#define TEST_DEPRECATED()       DEBUG_LOG_ALWAYS("Test \"%s\" is deprecated and has no effect.", __func__ )

#define MAX_CLIP_SIZE 50

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
#define APP_TEST_UNICAST_VOLUME_STEP_SIZE       10
#define APP_TEST_UNICAST_VOLUME_MAX_LEVEL       255
#define APP_TEST_UNICAST_VOLUME_MIN_LEVEL       0
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

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

/*! \brief Reset USB dongle to factory defaults.
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

    DEBUG_LOG_ALWAYS("appTestResetHappened, %d", result);

    if(reset_happened) {
        reset_happened = FALSE;
    }

    return result;
}

bool appTestApplicationIsState(usb_dongle_state_t checked_state)
{
    usb_dongle_state_t state = UsbDongleSm_GetState();

    DEBUG_LOG_ALWAYS("appTestApplicationIsState, current state"
                     " enum:usb_dongle_state_t:%d", state);

    return (state == checked_state);
}

usb_dongle_state_t appTestApplicationGetState(void)
{
    return UsbDongleSm_GetState();
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
#if defined(INCLUDE_A2DP_ANALOG_SOURCE) || defined(INCLUDE_LE_AUDIO_ANALOG_SOURCE)

void appTestWiredAnalogAudioPlugged(void)
{
    MessageSend(UsbDongleSmGetTask(), WIRED_AUDIO_DEVICE_CONNECT_IND, NULL);
}

void appTestWiredAnalogAudioUnplugged(void)
{
    MessageSend(UsbDongleSmGetTask(), WIRED_AUDIO_DEVICE_DISCONNECT_IND, NULL);
}

bool appTestWiredAnalogIsInputConnected(void)
{
    return UsbDongle_AudioInputIsConnected(usb_dongle_audio_input_analogue);
}

#endif /* INCLUDE_A2DP_ANALOG_SOURCE || INCLUDE_LE_AUDIO_ANALOG_SOURCE */

/*
** Earbud Pairing
*/

void appTestDeletePairing(void)
{
    DEBUG_LOG_ALWAYS("appTestDeletePairing");
    Ui_InjectUiInput(ui_input_sm_delete_handsets);
}

static device_t usbDongleTest_GetEarbud(void)
{
    return UsbDongleSm_GetCurrentSink();
}

/*
** Earbud to Dongle Pairing
*/

bool appTestEarbudPair(uint16 nap, uint8 uap, uint32 lap)
{
    if (PairingGetTaskData()->state >= PAIRING_STATE_PENDING_AUTHENTICATION )
    {
        DEBUG_LOG_ALWAYS("appTestEarbudPair, USB dongle already pairing");
        return FALSE;
    }

    bdaddr earbud_address;
    BdaddrSetZero(&earbud_address);

    earbud_address.nap = nap;
    earbud_address.uap = uap;
    earbud_address.lap = lap;

    DEBUG_LOG_ALWAYS("appTestEarbudPair, bdaddr 0x%04x 0x%02x 0x%06lx",
                     earbud_address.nap,
                     earbud_address.uap,
                     earbud_address.lap);

    return UsbDongleSm_PairSink(&earbud_address);
}

bool appTestEarbudIsPaired(void)
{
    bool paired = BtDevice_IsPairedWithSink();

    DEBUG_LOG_ALWAYS("appTestEarbudIsPaired, %d",paired);

    return paired;
}

void appTestEarbudPairingRssiStart(void)
{
    DEBUG_LOG_ALWAYS("appTestEarbudPairingRssiStart");
    Ui_InjectUiInput(ui_input_pair_sink);
}

void appTestEarbudPairingRssiStop(void)
{
    DEBUG_LOG_ALWAYS("appTestEarbudPairingRssiStop");
    RssiPairing_Stop();
}

void appTestEarbudPairingRssiDisable(void)
{
    DEBUG_LOG_ALWAYS("appTestEarbudPairingRssiDisable");
    SinkService_EnablePairing(FALSE);
}

void appTestEarbudPairingRssiEnable(void)
{
    DEBUG_LOG_ALWAYS("appTestEarbudPairingRssiEnable");
    SinkService_EnablePairing(TRUE);
}

bool appTestDongleBecomeBondable(void)
{
    DEBUG_LOG_ALWAYS("appTestDongleBecomeBondable");

    if (UsbDongleSm_GetState() != APP_STATE_IDLE)
    {
        DEBUG_LOG_ALWAYS("appTestDongleBecomeBondable, not IDLE");
        return FALSE;
    }

    if (PairingGetTaskData()->state >= PAIRING_STATE_DISCOVERABLE)
    {
        DEBUG_LOG_ALWAYS("appTestDongleBecomeBondable, already pairing");
        return FALSE;
    }

    Pairing_Pair(UsbDongleSmGetTask(), TRUE);
    return TRUE;
}

bool appTestDongleStopBondable(void)
{
    DEBUG_LOG_ALWAYS("appTestDongleStopBondable");

    if (PairingGetTaskData()->state >= PAIRING_STATE_PENDING_AUTHENTICATION)
    {
        DEBUG_LOG_ALWAYS("appTestDongleStopBondable, pairing can't be stopped");
        return FALSE;
    }

    Pairing_PairStop(NULL);
    return TRUE;
}

/*
** Earbud Bluetooth connections
*/

bool appTestTransmitEnable(bool val)
{
    return VmTransmitEnable(val);
}

void appTestEarbudDisconnect(void)
{
    DEBUG_LOG_ALWAYS("appTestEarbudDisconnect");
    Ui_InjectUiInput(ui_input_disconnect_sink);
}

void appTestEarbudConnect(void)
{
    DEBUG_LOG_ALWAYS("appTestEarbudConnect");
    Ui_InjectUiInput(ui_input_connect_sink);
}

bool appTestEarbudIsConnected(void)
{
    bool connected = SinkService_IsConnected();

    DEBUG_LOG_ALWAYS("appTestEarbudIsConnected, %d", connected);

    return connected;
}

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
bool appTestEarbudIsA2dpConnected(void)
{
    bool connected = FALSE;
    device_t device  = usbDongleTest_GetEarbud();
    if (!device)
    {
        return FALSE;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);
    /* Find earbud AV instance */
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
    connected = theInst && appA2dpIsConnected(theInst);

    DEBUG_LOG_ALWAYS("appTestEarbudIsA2dpConnected:%d", connected);

    return connected;
}

bool appTestEarbudIsAvrcpConnected(void)
{
    bool connected = FALSE;
    device_t device  = usbDongleTest_GetEarbud();
    if (!device)
    {
        return FALSE;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);

    /* Find earbud AV instance */
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
    connected = theInst && appAvrcpIsConnected(theInst);

    DEBUG_LOG_ALWAYS("appTestEarbudIsAvrcpConnected = %d", connected);

    return connected;
}

bool appTestEarbudIsHfpConnected(void)
{
    return UsbDongle_HfpInConnectedState();
}

bool appTestEarbudIsScoConnectionActive(void)
{
    aghfpInstanceTaskData *aghfp_instance;

    aghfp_instance = AghfpProfileInstance_GetInstanceForSource(voice_source_hfp_1);

    return AghfpProfile_IsScoActiveForInstance(aghfp_instance);
}

bool appTestEarbudIsA2dpPlaying(void)
{
    TEST_NOT_IMPLEMENTED();
    return FALSE;
}

bool appTestEarbudIsA2dpMediaConnected(void)
{
    bool connected = FALSE;
    device_t device  = usbDongleTest_GetEarbud();
    if (!device)
    {
        return FALSE;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);

        /* Find earbud AV instance */
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
    connected = theInst && appA2dpIsConnectedMedia(theInst);


    DEBUG_LOG_ALWAYS("appTestEarbudIsA2dpMediaConnected, %d", connected);

    return connected;
}

bool appTestEarbudIsA2dpStreaming(void)
{
    bool streaming = FALSE;
    device_t device  = usbDongleTest_GetEarbud();
    if (!device)
    {
        return FALSE;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);

        /* Find earbud AV instance */
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
    streaming = theInst && appA2dpIsStreaming(theInst);


    DEBUG_LOG_ALWAYS("appTestEarbudIsA2dpStreaming, %d", streaming);

    return streaming;
}

bool appTestEarbudChangeAvVolume(int8 step)
{
    UNUSED(step);
    TEST_NOT_IMPLEMENTED();
    return FALSE;
}

bool appTestEarbudSetAvVolume(uint8 volume)
{
    device_t device  = usbDongleTest_GetEarbud();
    if (!device)
    {
        return FALSE;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);

    /* Find earbud AV instance */
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
    
    if (theInst && appAvrcpIsConnected(theInst))
    {
        UsbDongle_VolumeObserverSetAbsoluteSinkVolume(volume);
        return TRUE;

    } else
    {
        return FALSE;
    }
}

bool appTestEarbudNotifyVolumeChange(uint8 volume)
{
    device_t device  = usbDongleTest_GetEarbud();
    if (!device)
    {
        return FALSE;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);

    /* Find earbud AV instance */
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);

    if (theInst && appAvrcpIsConnected(theInst))
    {
        appAvAvrcpVolumeNotification(theInst, volume);
        return TRUE;
    } else
    {
        return FALSE;
    }
}
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

bool appTestEarbudIsQhsConnected(void)
{
    bool qhs_connected_status = FALSE;
    device_t device  = usbDongleTest_GetEarbud();

    if (!device)
    {
        return FALSE;
    } else
    {
        bdaddr bd_addr = DeviceProperties_GetBdAddr(device);
        qhs_connected_status = ConManagerGetQhsConnectStatus(&bd_addr);
        DEBUG_LOG_ALWAYS("appTestIsEarbudQhsConnected, %d", qhs_connected_status);
        return qhs_connected_status;
    }
}


/*
** CODEC tests
*/

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
app_test_a2dp_codec_t appTestCodecGetType(void)
{
    app_test_a2dp_codec_t codec_type = a2dp_codec_unknown;
    device_t device  = usbDongleTest_GetEarbud();

    if (!device)
    {
        return a2dp_codec_unknown;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);

    /*Find Earbud AV instance*/
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);

    if(theInst)
    {
        switch(theInst->a2dp.current_seid)
        {
            case AV_SEID_SBC_SRC:
                codec_type = a2dp_codec_sbc;
                break;

            case AV_SEID_APTX_CLASSIC_SRC:
                codec_type = a2dp_codec_aptx;
                break;

            case AV_SEID_APTXHD_SRC:
                codec_type = a2dp_codec_aptx_hd;
                break;

            case AV_SEID_APTX_ADAPTIVE_SRC:
                codec_type = a2dp_codec_aptx_adaptive;
                break;

            default:
                break;
        }
    }

    DEBUG_LOG_ALWAYS("appTestCodecGetType, enum:app_test_a2dp_codec_t:%d", codec_type);
    return codec_type;
}

uint32 appTestCodecSampleRateGetCurrent(void)
{
    uint32 rate = UsbDongle_A2dpGetCurrentSampleRate();

    if (rate)
    {
        DEBUG_LOG_ALWAYS("appTestCodecSampleRateGetCurrent, A2DP sample rate %lu Hz", rate);
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestCodecSampleRateGetCurrent, no A2DP media channel");
    }
    return rate;
}

bool appTestCodecAptxAdModeSetHQ(void)
{
    if (Kymera_AptxAdEncoderGetDesiredQualityMode() == aptxad_mode_high_quality)
    {
        DEBUG_LOG_ALWAYS("appTestCodecAptxAdModeSetHQ, aptx mode is HQ already");
        /* Update preference anyway (might be LL running overridden as HQ) */
        Kymera_AptxAdEncoderSetDesiredQualityMode(aptxad_mode_high_quality);
    }

    /* Switch to HQ mode if current audio mode is not HQ. It could be LEA (such as broadcast)
       running at present */
    if (UsbDongleConfig_SetNewMode(usb_dongle_audio_mode_high_quality, usb_dongle_transport_mode_invalid))
    {
        Ui_InjectUiInput(ui_input_set_dongle_mode);
    }

    return TRUE;
}

bool appTestCodecAptxAdModeSetLL(void)
{
    if (Kymera_AptxAdEncoderGetDesiredQualityMode() == aptxad_mode_low_latency)
    {
        DEBUG_LOG_ALWAYS("appTestCodecAptxAdModeSetLL, aptx mode is LL already");
    }

    /* Switch to gaming mode if current audio mode is not gaming. It could be LEA (such as broadcast)
       running at present */
    if (UsbDongleConfig_SetNewMode(usb_dongle_audio_mode_gaming, usb_dongle_transport_mode_invalid))
    {
        Ui_InjectUiInput(ui_input_set_dongle_mode);
    }

    return TRUE;
}

aptxad_quality_mode_t appTestCodecAptxAdModeGetCurrent(void)
{
    aptxad_quality_mode_t mode = appTestEarbudIsA2dpStreaming() ? Kymera_AptxAdEncoderGetActiveQualityMode() :
                                                                  Kymera_AptxAdEncoderGetDesiredQualityMode();
    DEBUG_LOG_ALWAYS("appTestCodecAptxAdModeGetCurrent, enum:aptxad_quality_mode_t:%d", mode);
    return mode;
}

void appTestCodecAptxAdHqSampleRateSet(uint32 rate)
{
    DEBUG_LOG_ALWAYS("appTestCodecAptxAdHqSampleRateSet, set aptX Adaptive HQ rate %lu Hz", rate);
    UsbDongle_A2dpAptxAdHqSampleRateSet(rate);
}

uint32 appTestCodecAptxAdHqSampleRateGetSaved(void)
{
    uint32 rate = UsbDongle_A2dpAptxAdHqSampleRateGet();

    if (rate)
    {
        DEBUG_LOG_ALWAYS("appTestCodecAptxAdHqSampleRateGetSaved, saved aptX Adaptive HQ rate %lu Hz", rate);
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestCodecAptxAdHqSampleRateGetSaved, no saved aptX Adaptive HQ rate.");
    }
    return rate;
}

void appTestCodecAptxAdHqSampleRateDefault(void)
{
    DEBUG_LOG_ALWAYS("appTestCodecAptxAdHqSampleRateDefault, clear saved aptX Adaptive HQ rate");
    UsbDongle_A2dpAptxAdHqSampleRateDefault();
}

void appTestCodecAptxAdRfSignalUpdatesDisable(void)
{
    Kymera_AptxAdEncoderEnableRfSignalUpdates(FALSE);
    DEBUG_LOG_ALWAYS("appTestCodecAptxAdRfSignalUpdatesDisable, automatic updates DISABLED");
}

void appTestCodecAptxAdRfSignalUpdatesEnable(void)
{
    Kymera_AptxAdEncoderEnableRfSignalUpdates(TRUE);
    DEBUG_LOG_ALWAYS("appTestCodecAptxAdRfSignalUpdatesEnable, automatic updates ENABLED");
}

bool appTestCodecAptxAdRfSignalParamsSet(int16 rssi, uint16 cqddr, uint16 quality)
{
    aptxad_96K_encoder_rf_signal_params_t signal_params;

    signal_params.rssi = rssi;
    signal_params.cqddr = cqddr;
    signal_params.quality = quality;

    DEBUG_LOG_ALWAYS("appTestCodecAptxAdRfSignalParamsSet, RSSI %d, CQDDR %u, Quality %u",
                     signal_params.rssi, signal_params.cqddr, signal_params.quality);

    if (!Kymera_AptxAdEncoderSendRfSignalParams(&signal_params))
    {
        DEBUG_LOG_ALWAYS("appTestCodecAptxAdRfSignalParamsSet, params stored, but"
                         " unable to send to encoder now, likely not running");
        return FALSE;
    }
    return TRUE;
}

bool appTestCodecAptxAdRfSignalParamsGet(aptxad_96K_encoder_rf_signal_params_t *params)
{
    aptxad_96K_encoder_rf_signal_params_t signal_params;

    Kymera_AptxAdEncoderGetRfSignalParams(&signal_params);

    DEBUG_LOG_ALWAYS("appTestCodecAptxAdRfSignalParamsGet, RSSI %d, CQDDR %u, Quality %u",
                     signal_params.rssi, signal_params.cqddr, signal_params.quality);
    if (params)
    {
        params->rssi = signal_params.rssi;
        params->cqddr = signal_params.cqddr;
        params->quality = signal_params.quality;
        return TRUE;
    }
    return FALSE;
}

#ifdef TEST_AV_CODEC_PSKEY
static void appTestCodecsAdvertisedA2dpPrint(uint16 set_codecs)
{
    if (set_codecs & AV_CODEC_PS_BIT_SBC)
    {
        DEBUG_LOG_ALWAYS("  advertised SBC");
    }
    if (set_codecs & AV_CODEC_PS_BIT_AAC)
    {
        DEBUG_LOG_ALWAYS("  advertised AAC");
    }
    if (set_codecs & AV_CODEC_PS_BIT_APTX)
    {
        DEBUG_LOG_ALWAYS("  advertised aptX Classic");
    }
    if (set_codecs & AV_CODEC_PS_BIT_APTX_ADAPTIVE)
    {
        DEBUG_LOG_ALWAYS("  advertised aptX Adaptive");
    }
    if (set_codecs & AV_CODEC_PS_BIT_APTX_HD)
    {
        DEBUG_LOG_ALWAYS("  advertised aptX HD");
    }
}
#endif

bool appTestCodecsAdvertisedA2dpSet(uint16 codecs)
{
    #ifdef TEST_AV_CODEC_PSKEY
    uint16 key = PS_KEY_TEST_AV_CODEC;
    uint16 words_written = PsStore(key, &codecs, sizeof(codecs));
    if (words_written)
    {
        DEBUG_LOG_ALWAYS("appTestCodecsAdvertisedA2dpSet, PS_KEY_TEST_AV_CODEC 0x%x", codecs);
        appTestCodecsAdvertisedA2dpPrint(codecs);
        return TRUE;
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestCodecsAdvertisedA2dpSet, Failed to write PS_KEY_TEST_AV_CODEC");
        return FALSE;
    }
    #else
    DEBUG_LOG_ALWAYS("appTestCodecsAdvertisedA2dpSet, TEST_AV_CODEC_PSKEY not defined");
    UNUSED(codecs);
    return FALSE;
    #endif
}

bool appTestCodecsAdvertisedA2dpGet(uint16* set_codecs)
{
    #ifdef TEST_AV_CODEC_PSKEY
    uint16 key = PS_KEY_TEST_AV_CODEC;
    uint16 set_codecs_read;
    uint16 words_read = PsRetrieve(key, &set_codecs_read, sizeof(set_codecs_read));
    if (words_read)
    {
        DEBUG_LOG_ALWAYS("appTestCodecsAdvertisedA2dpGet, PS_KEY_TEST_AV_CODEC 0x%x", set_codecs_read);
        appTestCodecsAdvertisedA2dpPrint(set_codecs_read);
        if (set_codecs)
        {
            *set_codecs = set_codecs_read;
            return TRUE;
        }
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestCodecsAdvertisedA2dpGet, failed to read PS_KEY_TEST_AV_CODEC");
    }
    return FALSE;
    #else
    DEBUG_LOG_ALWAYS("appTestCodecsAdvertisedA2dpGetc TEST_AV_CODEC_PSKEY not defined");
    UNUSED(set_codecs);
    return FALSE;
    #endif
}


#ifdef TEST_HFP_CODEC_PSKEY
static void appTestCodecsAdvertisedHFPPrint(uint16 set_codecs)
{
    if (set_codecs & HFP_CODEC_PS_BIT_NB)
    {
        DEBUG_LOG_ALWAYS("  advertised narrowband speech (NBS)");
    }
    if (set_codecs & HFP_CODEC_PS_BIT_WB)
    {
        DEBUG_LOG_ALWAYS("  advertised wideband speech (WBS)");
    }
    if (set_codecs & HFP_CODEC_PS_BIT_SWB)
    {
        DEBUG_LOG_ALWAYS("  advertised superwideband speech (SWBS)");
    }
}
#endif

bool appTestCodecsAdvertisedHFPSet(uint16 codecs)
{
#ifdef TEST_HFP_CODEC_PSKEY
    uint16 key = PS_KEY_TEST_HFP_CODEC;
    uint16 words_written = PsStore(key, &codecs, sizeof(codecs));
    if (words_written)
    {
        DEBUG_LOG_ALWAYS("appTestCodecsAdvertisedHFPSet, PS_KEY_TEST_HFP_CODEC 0x%x", codecs);
        appTestCodecsAdvertisedHFPPrint(codecs);
        return TRUE;
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestCodecsAdvertisedHFPSet, failed to write PS_KEY_TEST_HFP_CODEC");
        return FALSE;
    }
#else
    DEBUG_LOG_ALWAYS("appTestCodecsAdvertisedHFPSet, TEST_HFP_CODEC_PSKEY not defined");
    UNUSED(codecs);
    return FALSE;
#endif
}

bool appTestCodecsAdvertisedHFPGet(uint16* set_codecs)
{
#ifdef TEST_HFP_CODEC_PSKEY
    uint16 key = PS_KEY_TEST_HFP_CODEC;
    uint16 set_codecs_read;
    uint16 words_read = PsRetrieve(key, &set_codecs_read, sizeof(set_codecs_read));
    if (words_read)
    {
        DEBUG_LOG_ALWAYS("appTestCodecsAdvertisedHFPGet, PS_KEY_TEST_HFP_CODEC 0x%x", set_codecs_read);
        appTestCodecsAdvertisedHFPPrint(set_codecs_read);
        if (set_codecs)
        {
            *set_codecs = set_codecs_read;
            return TRUE;
        }
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestCodecsAdvertisedHFPGet, failed to read PS_KEY_TEST_HFP_CODEC");
    }
    return FALSE;
#else
    DEBUG_LOG_ALWAYS("appTestCodecsAdvertisedHFPGetm TEST_HFP_CODEC_PSKEY not defined");
    UNUSED(set_codecs);
    return FALSE;
#endif
}




void appTestLatencyAdaptiveEnable(void)
{
    TEST_DEPRECATED();
}

void appTestLatencyAdaptiveDisable(void)
{
    TEST_DEPRECATED();
}

void appTestLatencyTargetSet(uint16 target_latency)
{
    UsbDongle_A2dpLatencyTargetSet(target_latency);
}

uint16 appTestLatencyTargetGet(void)
{
    return UsbDongle_A2dpLatencyTargetGet();
}

void appTestLatencyTargetDefault(void)
{
    UsbDongle_A2dpLatencyTargetDefault();
}
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */


/*
** Power and Battery
*/
void appTestDonglePowerReboot(void)
{
    DEBUG_LOG_ALWAYS("appTestDonglePowerReboot");
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
    device_t earbud_device = usbDongleTest_GetEarbud();

    if (earbud_device)
    {
        bdaddr addr = DeviceProperties_GetBdAddr(earbud_device);
        connected = ConManagerIsConnected(&addr);
        DEBUG_LOG_ALWAYS("appTestEarbudIsAclConnected, %d 0x%06x", connected, addr.lap);
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestEarbudIsAclConnected, 0 no earbud device/addr");
    }
    return connected;
}

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
    return UsbDongle_AudioInputIsConnected(usb_dongle_audio_input_usb);
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

bool appTestUsbHidTransportPhoneMute(void)
{
    return UsbSource_SendEvent(USB_SOURCE_PHONE_MUTE);
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

bool appTestUsbHidTransportButtonOne(void)
{
    return UsbSource_SendEvent(USB_SOURCE_BUTTON_ONE);
}

bool appTestUsbHidTransportFlash(void)
{
    return UsbSource_SendEvent(USB_SOURCE_FLASH);
}

bool appTestTelephonyUsbRingingStart(void)
{
    Telephony_NotifyCallIncomingOutOfBandRingtone(voice_source_usb);
    return TRUE;
}

bool appTestTelephonyUsbRingingStop(void)
{
    Telephony_NotifyCallIncomingEnded(voice_source_usb);
    return TRUE;
}

bool appTestTelephonyUsbOnHook(void)
{
    Telephony_NotifyCallEnded(voice_source_usb);
    return TRUE;
}

bool appTestTelephonyUsbOffHook(void)
{
    Telephony_NotifyCallActive(voice_source_usb);
    return TRUE;
}

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

bool appTestHfpConnect(void)
{
    device_t device = usbDongleTest_GetEarbud();
    if (device)
    {
        bdaddr bd_addr = DeviceProperties_GetBdAddr(device);
        UsbDongle_HfpConnect(&bd_addr);
        return TRUE;
    }
    return FALSE;
}

void appTestHfpDisconnect(void)
{
    DEBUG_LOG_ALWAYS("appTestHfpDisconnect");

    UsbDongle_HfpDisconnect();
}

bool appTestHfpIncomingCall(void)
{
    UsbDongle_HfpIncomingVoiceCall();
    return TRUE;
}

void appTestHfpIncomingCallWithAddr(uint16 nap, uint8 uap, uint32 lap)
{
    DEBUG_LOG_ALWAYS("appTestHfpIncomingCallWithAddr");

    bdaddr address;
    BdaddrSetZero(&address);

    address.nap = nap;
    address.uap = uap;
    address.lap = lap;

    AghfpProfile_CallIncomingInd(&address);
}

bool appTestHfpAnswerIncoming(void)
{
    VoiceSources_AcceptIncomingCall(voice_source_hfp_1);
    return TRUE;
}

bool appTestHfpCallReject(void)
{
    VoiceSources_RejectIncomingCall(voice_source_hfp_1);
    return UsbSource_SendEvent(USB_SOURCE_BUTTON_ONE);
}

void appTestHfpTerminateOngoingCall(void)
{
    VoiceSources_TerminateOngoingCall(voice_source_hfp_1);
}

bool appTestHfpSendRegistrationStatus(bool availability)
{
    aghfpInstanceTaskData *aghfp_instance;

    aghfp_instance = AghfpProfileInstance_GetInstanceForSource(voice_source_hfp_1);

    if ( aghfp_instance )
    {
        AghfpProfile_SendServiceIndicator(aghfp_instance, availability?aghfp_service_present:aghfp_service_none);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

bool appTestHfpSendSignalStrengthIndication(uint16 level)
{
    aghfpInstanceTaskData *aghfp_instance;

    aghfp_instance = AghfpProfileInstance_GetInstanceForSource(voice_source_hfp_1);
    if ( aghfp_instance )
    {
        AghfpProfile_SendSignalStrengthIndicator(aghfp_instance, level);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

bool appTestHfpDisconnectSCO(void)
{
    DEBUG_LOG_ALWAYS("appTestHfpDisconnectSCO");

    aghfpInstanceTaskData *aghfp_instance;

    aghfp_instance = AghfpProfileInstance_GetInstanceForSource(voice_source_hfp_1);

    /* These functions should make direct API calls only. Used for PTS test cases */
    if (AghfpProfile_IsScoActiveForInstance(aghfp_instance))
    {
        AghfpProfileAbstract_AudioDisconnect(aghfp_instance);
        return TRUE;
    }

    return FALSE;
}

bool appTestHfpConnectSCO(void)
{
    DEBUG_LOG_ALWAYS("appTestHfpConnectSCO");

    aghfpInstanceTaskData *aghfp_instance;

    aghfp_instance = AghfpProfileInstance_GetInstanceForSource(voice_source_hfp_1);

    /* These functions should make direct API calls only. Used for PTS test cases */
    if (!AghfpProfile_IsScoActiveForInstance(aghfp_instance))
    {
        AghfpProfileAbstract_ConfigureAudioSettings(aghfp_instance);
        AghfpProfileAbstract_AudioConnect(aghfp_instance, AghfpProfile_GetAudioParams(aghfp_instance), aghfp_instance->qce_codec_mode_id);
        return TRUE;
    }

    return FALSE;
}

bool appTestHfpSetAutomaticOutgoingAudioConnection(bool enable)
{
    DEBUG_LOG_ALWAYS("appTestHfpSetAutomaticOutgoingAudioConnection");

    aghfpInstanceTaskData *aghfp_instance;

    aghfp_instance = AghfpProfileInstance_GetInstanceForSource(voice_source_hfp_1);

    if (aghfp_instance)
    {
        /* Enable/Disable Automatic Outgoing Audio connection for PTS test cases */
        AghfpProfile_SetAutomaticOutgoingAudioConnection(aghfp_instance, enable);
        return TRUE;
    }

    return FALSE;
}

bool appTestHfpRetainInstanceAfterDisconnection(bool enable)
{
    DEBUG_LOG_ALWAYS("appTestHfpRetainInstanceAfterDisconnection");

    aghfpInstanceTaskData *aghfp_instance;

    aghfp_instance = AghfpProfileInstance_GetInstanceForSource(voice_source_hfp_1);

    if (aghfp_instance)
    {
        /* Enable/Disable retain instance after HF disconnection, for PTS test cases */
        AghfpProfile_RetainInstanceAfterDisconnection(aghfp_instance, enable);
        return TRUE;
    }

    return FALSE;
}

void appTestHfpSetClip(void)
{
    clip_data caller_id_data;
    uint8 phone_number[11] = {"\"1234567\""};
    caller_id_data.clip_number = phone_number;
    caller_id_data.size_clip_number = 9;
    caller_id_data.clip_type = 129;

    AghfpProfile_SetClipInd(caller_id_data);
}

bool appTestHfpSetClipWithNumber(char *number)
{
    DEBUG_LOG_ALWAYS("appTestHfpSetClipWithNumber");

    if ((MAX_CLIP_SIZE - 3) < strlen(number))
    {
        DEBUG_LOG_ERROR("appTestHfpSetClipWithNumber, number size %d is greater than max allowed size of %d", strlen(number), MAX_CLIP_SIZE);
        return FALSE;
    }

    char number_with_quotes[MAX_CLIP_SIZE];
    clip_data caller_id_data;
    uint8 *phone_number = (uint8*)number_with_quotes;

    sprintf(number_with_quotes, "\"%s\"\0", number);

    caller_id_data.clip_number = phone_number;
    caller_id_data.size_clip_number = strlen(number_with_quotes);
    caller_id_data.clip_type = 129;

    AghfpProfile_SetClipInd(caller_id_data);

    return TRUE;
}

void appTestHfpClearClip(void)
{
    AghfpProfile_ClearClipInd();
}

void appTestHfpSetNetworkOperator(void)
{
    AghfpProfile_SetNetworkOperatorInd("test");
}

void appTestHfpClearNetworkOperator(void)
{
    AghfpProfile_ClearNetworkOperatorInd();
}

bool appTestHfpSendBatteryLevelIndication(uint16 level)
{
    DEBUG_LOG_ALWAYS("appTestHfpSendBatteryLevelIndication");

    aghfpInstanceTaskData *aghfp_instance;

    aghfp_instance = AghfpProfileInstance_GetInstanceForSource(voice_source_hfp_1);

    if (aghfp_instance)
    {
        AghfpProfile_SendBatteryChgIndicator(aghfp_instance, level);
        return TRUE;
    }

    return FALSE;
}

bool appTestHfpAnswerOutgoing(void)
{
    DEBUG_LOG_ALWAYS("appTestHfpAnswerOutgoing");

    aghfpInstanceTaskData *aghfp_instance;

    aghfp_instance = AghfpProfileInstance_GetInstanceForSource(voice_source_hfp_1);

    if (aghfp_instance)
    {
        AghfpProfile_OutgoingCallAnswered(&aghfp_instance->hf_bd_addr);
        return TRUE;
    }

    return FALSE;
}

bool appTestHfpSetCallOnHold(void)
{
    DEBUG_LOG_ALWAYS("appTestHfpSetCallOnHold");

    aghfpInstanceTaskData *aghfp_instance = AghfpProfileInstance_GetInstanceForSource(voice_source_hfp_1);

    if (!aghfp_instance)
    {
        return FALSE;
    }

    DEBUG_LOG_ALWAYS("appTestHfpSetCallOnHold, holding call for bd_addr lap %#x nap %#x uap %#x", aghfp_instance->hf_bd_addr.lap, aghfp_instance->hf_bd_addr.nap, aghfp_instance->hf_bd_addr.uap);

    return AghfpProfile_HoldActiveCall(&aghfp_instance->hf_bd_addr);
}

bool appTestHfpReleaseHeldCall(void)
{
    DEBUG_LOG_ALWAYS("appTestHfpReleaseHeldCall");

    aghfpInstanceTaskData *aghfp_instance = AghfpProfileInstance_GetInstanceForSource(voice_source_hfp_1);

    if (!aghfp_instance)
    {
        return FALSE;
    }

    DEBUG_LOG_ALWAYS("appTestHfpReleaseHeldCall, rReleasing held call for bd_addr lap %#x nap %#x uap %#x", aghfp_instance->hf_bd_addr.lap, aghfp_instance->hf_bd_addr.nap, aghfp_instance->hf_bd_addr.uap);

    return AghfpProfile_ReleaseHeldCall(&aghfp_instance->hf_bd_addr);
}

void appTestHfpPrintCallList(void)
{
    DEBUG_LOG_ALWAYS("appTestHfpPrintCallList");

    aghfpInstanceTaskData *aghfp_instance = AghfpProfileInstance_GetInstanceForSource(voice_source_hfp_1);

    if (!aghfp_instance)
    {
        return;
    }

    call_list_element_t *call;

    DEBUG_LOG_ALWAYS("appTestHfpPrintCallList, listing all existing calls:\n");

    for_each_call(aghfp_instance->call_list, call)
    {
        DEBUG_LOG_ALWAYS("  call index %d", call->call.idx);
        DEBUG_LOG_ALWAYS("  call status enum:aghfp_call_state:%d", call->call.status);
        DEBUG_LOG_ALWAYS("");
    }
}

void appTestHfpClearCallHistory(void)
{
    DEBUG_LOG_ALWAYS("appTestHfpClearCallHistory");

    AghfpProfile_ClearCallHistory();
}

void appTestHfpSetLastDialledNumber(void)
{
    DEBUG_LOG_ALWAYS("appTestHfpSetLastDialledNumber");

    const char* dialled_number = "12345678912";

    AghfpProfile_SetLastDialledNumber(strlen(dialled_number), (uint8*)dialled_number);
}

bool appTestHfpSetVolume(uint8 volume)
{
    aghfpInstanceTaskData *aghfp_instance;

    aghfp_instance = AghfpProfileInstance_GetInstanceForSource(voice_source_hfp_1);

    if (aghfp_instance)
    {
        DEBUG_LOG_ALWAYS("appTestHfpSetVolume");
        AghfpProfileAbstract_SetRemoteSpeakerVolume(aghfp_instance, volume);
        return TRUE;
    }

    return FALSE;
}

bool appTestHfpSetMicGain(uint8 gain)
{
    aghfpInstanceTaskData *aghfp_instance;

    aghfp_instance = AghfpProfileInstance_GetInstanceForSource(voice_source_hfp_1);

    if (aghfp_instance)
    {
        DEBUG_LOG_ALWAYS("appTestHfpSetMicGain");
        AghfpProfileAbstract_SetRemoteMicrophoneGain(aghfp_instance, gain);
        return TRUE;
    }

    return FALSE;
}


bool appTestEarbudSendTrackChange(void)
{
    avInstanceTaskData* av_instance = Av_GetInstanceForHandsetSource(audio_source_a2dp_1);
    if (av_instance == NULL)
    {
        return FALSE;
    }
    DEBUG_LOG_ALWAYS("appTestEarbudSendTrackChange, send track change for 0x%04x, 0x%02x, 0x%06lx",
                     av_instance->bd_addr.nap,
                     av_instance->bd_addr.uap,
                     av_instance->bd_addr.lap);


    return AvrcpMetadata_SendTrackChange(&av_instance->bd_addr, 0, 0);
}

void appTestEarbudSetPlayStatusStopped(void)
{
    avInstanceTaskData* av_instance = Av_GetInstanceForHandsetSource(audio_source_a2dp_1);
    if (av_instance == NULL)
    {
        return;
    }

    AvrcpMetadata_SetPlayStatus(av_instance, avrcp_play_status_stopped);
}

void appTestEarbudSetPlayStatusPlaying(void)
{
    avInstanceTaskData* av_instance = Av_GetInstanceForHandsetSource(audio_source_a2dp_1);
    if (av_instance == NULL)
    {
        return;
    }

    AvrcpMetadata_SetPlayStatus(av_instance, avrcp_play_status_playing);
}

void appTestEarbudSetPlayStatusPaused(void)
{
    avInstanceTaskData* av_instance = Av_GetInstanceForHandsetSource(audio_source_a2dp_1);
    if (av_instance == NULL)
    {
        return;
    }

    AvrcpMetadata_SetPlayStatus(av_instance, avrcp_play_status_paused);
}

bool appTestEarbudSendPassthroughPlay(void)
{
    if (!UsbDongle_A2dpSourceInConnectedState())
    {
        return FALSE;
    }
    AudioSources_Play(audio_source_a2dp_1);
    return TRUE;
}

bool appTestEarbudSendPassthroughPause(void)
{
    if (!UsbDongle_A2dpSourceInConnectedState())
    {
        return FALSE;
    }
    AudioSources_Pause(audio_source_a2dp_1);
    return TRUE;
}

bool appTestEarbudSendPassthroughForward(void)
{
    if (!UsbDongle_A2dpSourceInConnectedState())
    {
        return FALSE;
    }
    AudioSources_Forward(audio_source_a2dp_1);
    return TRUE;
}

bool appTestEarbudSendPassthroughBack(void)
{
    if (!UsbDongle_A2dpSourceInConnectedState())
    {
        return FALSE;
    }
    AudioSources_Back(audio_source_a2dp_1);
    return TRUE;
}

bool appTestEarbudSendPassthroughFastRewind(bool state)
{
    if (!UsbDongle_A2dpSourceInConnectedState())
    {
        return FALSE;
    }
    AudioSources_FastRewind(audio_source_a2dp_1, state);
    return TRUE;
}

bool appTestEarbudSendPassthroughFastForward(bool state)
{
    if (!UsbDongle_A2dpSourceInConnectedState())
    {
        return FALSE;
    }
    AudioSources_FastForward(audio_source_a2dp_1, state);
    return TRUE;
}

void appTestEarbudSetTrackSelected(bool selected)
{
    UNUSED(selected);
    AvrcpMetadata_SetTrackSelected(selected);
}

bool appTestEarbudA2dpMediaSuspend(void)
{
    DEBUG_LOG_ALWAYS("appTestEarbudA2dpMediaSuspend");
    return UsbDongle_A2dpSourceSuspend();
}

bool appTestEarbudA2dpMediaResume(void)
{
    DEBUG_LOG_ALWAYS("appTestEarbudA2dpMediaResume");
    return UsbDongle_A2dpSourceResume();
}

void appTestEarbudUseLargeMetadata(bool use_large_metadata)
{
    UNUSED(use_large_metadata);
    AvrcpMetadata_SetLargeMetadata(use_large_metadata);
}

bool appTestEarbudSendPassthroughVolumeUp(void)
{
    if (!UsbDongle_A2dpSourceInConnectedState())
    {
        return FALSE;
    }
    avInstanceTaskData* av_instance = Av_GetInstanceForHandsetSource(audio_source_a2dp_1);
    if (av_instance == NULL)
    {
        return FALSE;
    }
    appAvrcpRemoteControl(av_instance, opid_volume_up, 0, FALSE, 0);
    appAvrcpRemoteControl(av_instance, opid_volume_up, 1, FALSE, 0);

    return TRUE;
}

bool appTestEarbudSendPassthroughVolumeDown(void)
{
    if (!UsbDongle_A2dpSourceInConnectedState())
    {
        return FALSE;
    }
    avInstanceTaskData* av_instance = Av_GetInstanceForHandsetSource(audio_source_a2dp_1);
    if (av_instance == NULL)
    {
        return FALSE;
    }
    appAvrcpRemoteControl(av_instance, opid_volume_down, 0, FALSE, 0);
    appAvrcpRemoteControl(av_instance, opid_volume_down, 1, FALSE, 0);

    return TRUE;
}

#ifdef USE_SYNERGY
bool appTestEarbudAvrcpNotiRegister(CsrBtAvrcpNotiId event_id)
{
    DEBUG_LOG_ALWAYS("appTestEarbudAvrcpNotiRegister");
    device_t device  = usbDongleTest_GetEarbud();
    if (!device)
    {
        return FALSE;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
    if (appAvrcpGetState(theInst) == AVRCP_STATE_CONNECTED)
    {
        AvrcpCtNotiRegisterReqSend(&(AvGetTaskData()->task),theInst->avrcp.connectionId,(CsrBtAvrcpNotiMask)(1 << (event_id - 1)), 0, CSR_BT_AVRCP_NOTI_REG_STANDARD);
        theInst->avrcp.notification_lock = event_id;

        /* Set registered event bit */
        appAvrcpSetEventRegistered(theInst, event_id);
        return TRUE;
    }
    return FALSE;
}

bool appTestEarbudA2dpMediaClose(void)
{
	DEBUG_LOG_ALWAYS("appTestEarbudA2dpMediaClose");
	device_t device  = usbDongleTest_GetEarbud();
    if (!device)
    {
        return FALSE;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);
	avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
	AvCloseReqSend(theInst->a2dp.stream_id, theInst->a2dp.tLabel);
	return TRUE;
}

bool appTestEarbudA2dpMediaAbort(void)
{
	DEBUG_LOG_ALWAYS("appTestEarbudA2dpMediaAbort");
	device_t device  = usbDongleTest_GetEarbud();
    if (!device)
    {
        return FALSE;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);
	avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
	AvAbortReqSend(theInst->a2dp.stream_id, A2DP_ASSIGN_TLABEL(theInst));
	return TRUE;
}

/*! \brief Send A2dp Reconfigure stream request to remote
    Added to support PTS qualification TCs.
*/
bool appTestEarbudA2dpMediaReconfigure(void)
{
	DEBUG_LOG_ALWAYS("appTestEarbudA2dpMediaReconfigure");
	device_t device  = usbDongleTest_GetEarbud();
    if (!device)
    {
        return FALSE;
    }
    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);
	avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
	if (theInst)
	{
		 uint8 *caps = (uint8 *)PanicUnlessMalloc(sizeof(uint8) * 8);

        /* Just hardcode the values */
        caps[0] = 7;
        caps[1] = 6;
        caps[2] = 0;
        caps[3] = 0;
        caps[4] = 17;
        caps[5] = 21;
        caps[6] = 30;
        caps[7] = 50;
        /* Find handset AV instance */
        AvReconfigReqSend(theInst->a2dp.stream_id,
                          A2DP_ASSIGN_TLABEL(theInst),
                          8,
                          caps);
        return TRUE;
	}
	return FALSE;
}

bool appTestReconfigureAvrcpWithBrowsing(void)
{
    CsrBtAvrcpRoleDetails ctFeatures;
    CsrBtAvrcpRoleDetails tgFeatures;

    DEBUG_LOG("appTestReconfigureAvrcpWithBrowsing");

    if (UsbDongleSm_GetState() != APP_STATE_IDLE)
    {
        DEBUG_LOG_ALWAYS("appTestReconfigureAvrcpWithBrowsing, App not in IDLE State");
        return FALSE;
    }

    if (AvGetTaskData()->bitfields.state != AV_STATE_IDLE)
    {
        DEBUG_LOG_ALWAYS("appTestReconfigureAvrcpWithBrowsing, AV not in IDLE State");
        return FALSE;
    }

    AvrcpConfigRoleSupport(&tgFeatures,                                             /* Pointer to details */
                           CSR_BT_AVRCP_CONFIG_ROLE_STANDARD,                       /* Role config */
                           CSR_BT_AVRCP_CONFIG_SR_VERSION_16,                       /* AVRCP version */
                           CSR_BT_AVRCP_CONFIG_SR_FEAT_CAT1_PLAY_REC |
#ifdef INCLUDE_AVRCP_BROWSING
                           CSR_BT_AVRCP_CONFIG_SR_FEAT_BROWSING |
#endif
                           CSR_BT_AVRCP_CONFIG_SR_FEAT_CAT2_MON_AMP,                /* Features */
                           (CsrCharString*)CsrStrDup(AVRCP_CONFIG_PROVIDER_NAME),   /* Provider name */
                           (CsrCharString*)CsrStrDup("AVRCP TG"));                  /* Service name */
    AvrcpConfigRoleSupport(&ctFeatures,
                           CSR_BT_AVRCP_CONFIG_ROLE_STANDARD,
                           CSR_BT_AVRCP_CONFIG_SR_VERSION_16,
#ifdef INCLUDE_AVRCP_BROWSING
                           CSR_BT_AVRCP_CONFIG_SR_FEAT_BROWSING |
#endif
                           CSR_BT_AVRCP_CONFIG_SR_FEAT_CAT2_MON_AMP,
                           (CsrCharString*)CsrStrDup(AVRCP_CONFIG_PROVIDER_NAME),
                           (CsrCharString*)CsrStrDup("AVRCP CT"));

    AvrcpConfigReqSend(&(AvGetTaskData()->task),
                       CSR_BT_AVRCP_CONFIG_GLOBAL_STANDARD,
                       AVRCP_CONFIG_DEFAULT_MTU,
                       tgFeatures,
                       ctFeatures);

    return TRUE;
}

#endif /* USE_SYNERGY */
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

void appTestSetUnicastVolume(uint32 cid, uint8 volume)
{
    if (LeAudioClient_GetGroupHandle(cid))
    {
        UsbDongle_VolumeObserverSetAbsoluteSinkVolume(volume);
    }
}

uint8 appTestGetUnicastVolume(uint32 cid)
{
    return LeAudioClient_GetAbsoluteVolume(LeAudioClient_GetGroupHandle(cid));
}

static void appTestUnicastSourceVolumeCtrl(uint32 cid, bool vol_up)
{
    int16 volume;
    ServiceHandle handle = LeAudioClient_GetGroupHandle(cid);

    volume = (int16) LeAudioClient_GetAbsoluteVolume(handle);

    DEBUG_LOG_ALWAYS("appTestUnicastSourceVolumeCtrl current-volume: %d, step: %d, up/down: %d", volume, APP_TEST_UNICAST_VOLUME_STEP_SIZE, vol_up);

    if (vol_up)
    {
        volume += APP_TEST_UNICAST_VOLUME_STEP_SIZE;
    }
    else
    {
        volume -= APP_TEST_UNICAST_VOLUME_STEP_SIZE;
    }

    if (volume > APP_TEST_UNICAST_VOLUME_MAX_LEVEL)
    {
        volume = APP_TEST_UNICAST_VOLUME_MAX_LEVEL;
    }
    else if (volume < APP_TEST_UNICAST_VOLUME_MIN_LEVEL)
    {
        volume = APP_TEST_UNICAST_VOLUME_MIN_LEVEL;
    }

    UsbDongle_VolumeObserverSetAbsoluteSinkVolume((uint8) volume);
}

void appTestUnicastSourceVolumeUp(uint32 cid)
{
    appTestUnicastSourceVolumeCtrl(cid, TRUE);
}

void appTestUnicastSourceVolumeDown(uint32 cid)
{
    appTestUnicastSourceVolumeCtrl(cid, FALSE);
}

void appTestMuteUnicastAudio(uint32 cid, bool mute)
{
    LeAudioClient_SetMute(LeAudioClient_GetGroupHandle(cid), mute);
}

bool appTestIsUnicastStreaming(uint32 cid)
{
    bool is_streaming = LeAudioClient_IsUnicastStreamingActive(LeAudioClient_GetGroupHandle(cid));

    DEBUG_LOG_ALWAYS("appTestIsUnicastStreaming %d", is_streaming);
    return is_streaming;
}

uint16 appTestUnicastGetAudioContext(void)
{
    uint16 audio_context = UsbDongle_LeaIsSourceActive() ? LeAudioClient_GetUnicastSessionCapAudioContext() : 0;

    DEBUG_LOG_ALWAYS("appTestUnicastGetAudioContext, audio_context :0x%x", audio_context);
    return audio_context;
}

uint32 appTestUnicastGetCodecId(void)
{
    uint32 codec_id = UsbDongle_LeaIsSourceActive() ? LeAudioClient_GetConfiguredCodecId() : 0x00;

    DEBUG_LOG_ALWAYS("appTestUnicastGetCodecId, codecId 0x%08x", codec_id);
    return codec_id;
}

void appTestUnicastGamingModeEnable(void)
{
    uint8 mode_change = UsbDongleConfig_SetNewMode(usb_dongle_audio_mode_gaming, usb_dongle_transport_mode_invalid);

    if (mode_change)
    {
        Ui_InjectUiInput(ui_input_set_dongle_mode);
    }

    DEBUG_LOG_ALWAYS("appTestUnicastGamingModeEnable, Enabling Gaming mode mode_change: %d", mode_change);
}

void appTestUnicastGamingModeDisable(void)
{
    uint8 mode_change = UsbDongleConfig_SetNewMode(usb_dongle_audio_mode_high_quality, usb_dongle_transport_mode_invalid);

    if (mode_change)
    {
        DEBUG_LOG_ALWAYS("appTestUnicastGamingModeDisable, Disabling Gaming mode");
        Ui_InjectUiInput(ui_input_set_dongle_mode);
    }

    DEBUG_LOG_ALWAYS("appTestUnicastGamingModeDisable, Disabling Gaming mode mode_change: %d", mode_change);
}

bool appTestIsUnicastGamingModeEnabled(void)
{
    bool is_gaming_mode_enabled = usbDongleConfig_IsInGamingAudioMode();

    DEBUG_LOG_ALWAYS("appTestIsUnicastGamingModeEnabled, Gaming mode %d", is_gaming_mode_enabled);

    return is_gaming_mode_enabled;
}

bool appTestIsCcpConnected(void)
{
    return LeAudioClient_IsUnicastConnected(LeAudioClient_GetGroupHandle(INVALID_CID));
}

bool appTestIsMcpConnected(void)
{
    return LeAudioClient_IsUnicastConnected(LeAudioClient_GetGroupHandle(INVALID_CID));
}

uint8 appTestGetCurrentMcpMediaState(void)
{
    return LeAudioClient_GetCurrentMediaState();
}

bool appTestCsipSetLock(uint32 cid, bool lock_enabled)
{
    return CsipClient_SetLockForMember(cid, lock_enabled);
}

bool appTestCsipSetLockForAll(bool lock_enabled)
{
    return CsipClient_SetLockForAllMembers(lock_enabled);
}

bool appTestCsipReadSetSize(void)
{
    return CsipClient_ReadSetSize();
}

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */


#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
void appTestBroadcastModeEnable(void)
{
    bool mode_changed = FALSE;

    if (!usbDongleConfig_IsInBroadcastAudioMode() && UsbDongle_LeaIsBroadcastModeSwitchingPossible() &&
        UsbDongleConfig_SetNewMode(usb_dongle_audio_mode_broadcast, usb_dongle_transport_mode_invalid))
    {
        Ui_InjectUiInput(ui_input_set_dongle_mode);
        mode_changed = TRUE;
    }

    DEBUG_LOG_ALWAYS("appTestBroadcastModeEnable broadcast_mode:%d, mode_changed: %d", usbDongleConfig_IsInBroadcastAudioMode(), mode_changed);
}

void appTestBroadcastModeDisable(void)
{
    bool mode_changed = FALSE;

    if (usbDongleConfig_IsInBroadcastAudioMode() && UsbDongle_LeaIsBroadcastModeSwitchingPossible() &&
        UsbDongleConfig_SetNewMode(usb_dongle_audio_mode_gaming, usb_dongle_transport_mode_invalid))
    {
        Ui_InjectUiInput(ui_input_set_dongle_mode);
        mode_changed = TRUE;
    }

    DEBUG_LOG_ALWAYS("appTestBroadcastModeDisable broadcast_mode:%d, mode_changed: %d", usbDongleConfig_IsInBroadcastAudioMode(), mode_changed);
}

bool appTestIsInBroadcastMode(void)
{
    return usbDongleConfig_IsInBroadcastAudioMode();
}

void appTestSetPbpBroadcastMode(bool pbp_enable)
{
    DEBUG_LOG_ALWAYS("appTestSetPbpBroadcastMode %d", pbp_enable);

    UsbDongle_LeaConfigSetPbpBroadcastmode(pbp_enable);
}

bool appTestIsInPbpBroadcastMode(void)
{
    return LeAudioClientBroadcast_IsInPbpMode();
}

void appTestSetBroadcastCode(const uint8 *code, uint8 len)
{
    DEBUG_LOG_ALWAYS("appTestSetBroadcastCode");

    UsbDongle_LeaConfigSetLeaBroadcastCode(code, len);
}

bool appTestSetBroadcastSourceName(const char *name, uint8 len)
{
    bool status = UsbDongle_LeaConfigSetLeaBroadcastSourceName(name, len);

    DEBUG_LOG_ALWAYS("appTestSetBroadcastSourceName, status: %d", status);

    return status;
}

bool appTestSetBroadcastConfiguration(uint32 stream_capability, bool pbp_enable)
{
    bool status = UsbDongle_LeaConfigSetBroadcastStreamCapability(stream_capability, pbp_enable);

    DEBUG_LOG_ALWAYS("appTestSetBroadcastConfiguration, stream_capability:0x%x, public: %d", stream_capability, pbp_enable);

    return status;
}

bool appTestSetBroadcastBisConfiguration(uint32 number_of_bis, uint32 no_of_channels_per_bis)
{
    bool status = UsbDongle_LeaConfigSetBroadcastBisConfig(number_of_bis, no_of_channels_per_bis);

    DEBUG_LOG_ALWAYS("appTestSetBroadcastBisConfiguration, number_of_bis:%d, no_of_channels_per_bis: %d", number_of_bis, no_of_channels_per_bis);

    return status;
}

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

uint32 appTestGetLeaActiveSpkrPathSampleRate(void)
{
    return LeAudioClient_GetActiveSpkrPathSampleRate();
}

uint32 appTestGetLeaActiveMicPathSampleRate(void)
{
    return LeAudioClient_GetActiveMicPathSampleRate();
}

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

usb_dongle_transport_mode_t appTestGetTransportMode(void)
{
    return usbDongleConfig_GetTransportMode();
}

void appTestSetTransportMode(usb_dongle_transport_mode_t transport_mode)
{
    if (UsbDongleConfig_SetNewMode(usb_dongle_audio_mode_invalid, transport_mode))
    {
        DEBUG_LOG_ALWAYS("appTestSetTransportMode, transport_mode: enum:usb_dongle_transport_mode_t:%d",
                          transport_mode);

        Ui_InjectUiInput(ui_input_set_dongle_mode);
    }
}

usb_dongle_audio_mode_t appTestGetAudioMode(void)
{
    return usbDongleConfig_GetAudioMode();
}

sink_service_transport_t appTestGetConnectedTransport(void)
{
    return usbDongleConfig_GetConnectedTransport();
}

#ifdef USE_SYNERGY

int appTestGetKeys(void)
{
    uint8 index = 0;

    if (CsrBtTdDbCountDevices() == 0)
    {
        DEBUG_LOG_ALWAYS(" No device in Database\n");
        return index;
    }

    for (index = 0; index < APP_CONFIG_MAX_PAIRED_DEVICES; index++)
    {
        CsrBtTypedAddr addr;
        CsrBtTdDbBredrKey bredrKey;
        CsrBtTdDbLeKeys leKeys;

        if ( (CsrBtTdDbGetEntryByIndex(index,
                                     CSR_BT_TD_DB_SOURCE_SC,
                                     CSR_BT_TD_DB_SC_KEY_BREDR_KEY,
                                     sizeof(CsrBtTdDbBredrKey),
                                     &bredrKey,
                                     &addr) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS) ||
             (CsrBtTdDbGetEntryByIndex(index,
                                      CSR_BT_TD_DB_SOURCE_SC,
                                      CSR_BT_TD_DB_SC_KEY_LE_KEYS,
                                      sizeof(CsrBtTdDbLeKeys),
                                      &leKeys,
                                      &addr) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS))
        {
            DEBUG_LOG_ALWAYS("appTestGetKeys: %1X:%04X:%02X:%06X",
                      addr.type,
                      addr.addr.nap,
                      addr.addr.uap,
                      addr.addr.lap);

            /* Displaying BREDR link key in reverse order as required by Sniffer tools */
            DEBUG_LOG_ALWAYS("Link Key: 0x%04X, 0x%04X, 0x%04X, 0x%04X",
                      *(uint16 *)(&bredrKey.linkkey[14]),
                      *(uint16 *)(&bredrKey.linkkey[12]),
                      *(uint16 *)(&bredrKey.linkkey[10]),
                      *(uint16 *)(&bredrKey.linkkey[8]));

            DEBUG_LOG_ALWAYS("0x%04X, 0x%04X, 0x%04X, 0x%04X",
                      *(uint16 *)(&bredrKey.linkkey[6]),
                      *(uint16 *)(&bredrKey.linkkey[4]),
                      *(uint16 *)(&bredrKey.linkkey[2]),
                      *(uint16 *)(&bredrKey.linkkey[0]));

            CsrBtTdDbGetEntryByIndex(index,
                                     CSR_BT_TD_DB_SOURCE_SC,
                                     CSR_BT_TD_DB_SC_KEY_LE_KEYS,
                                     sizeof(CsrBtTdDbLeKeys),
                                     &leKeys,
                                     &addr);

            /* Displaying LE link key in reverse order as required by Sniffer tools */
            DEBUG_LOG_ALWAYS("LTK: 0x%04X, 0x%04X, 0x%04X, 0x%04X",
                      leKeys.encCentral.ltk[7],
                      leKeys.encCentral.ltk[6],
                      leKeys.encCentral.ltk[5],
                      leKeys.encCentral.ltk[4]);

            DEBUG_LOG_ALWAYS("0x%04X, 0x%04X, 0x%04X, 0x%04X",
                      leKeys.encCentral.ltk[3],
                      leKeys.encCentral.ltk[2],
                      leKeys.encCentral.ltk[1],
                      leKeys.encCentral.ltk[0]);
        }
        else
        {
            break;
        }
    }

    return index;
}

#endif /* USE_SYNERGY */
