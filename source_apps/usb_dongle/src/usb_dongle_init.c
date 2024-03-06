/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Initialisation module
*/

#ifndef USB_DONGLE_INIT_C
#define USB_DONGLE_INIT_C

#include "usb_dongle_logging.h"

#include "usb_dongle_init.h"
#include "usb_dongle_init_bt.h"
#include "usb_dongle_advertising.h"
#include "usb_dongle_audio.h"
#include "usb_dongle_config.h"
#include "usb_dongle_dfu.h"
#include "usb_dongle_setup_audio.h"
#include "usb_dongle_sm.h"
#include "usb_dongle_ui_config.h"
#include "usb_dongle_ui.h"
#include "usb_dongle_buttons.h"
#include "usb_dongle_volume_observer.h"

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
#include "usb_dongle_a2dp.h"
#include "usb_dongle_hfp.h"
#include "usb_dongle_inquiry.h"
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
#include "usb_dongle_lea.h"
#include "usb_dongle_hid.h"
#include "usb_dongle_le_voice.h"
#include "gatt_handler.h"
#include "le_scan_manager.h"
#include "gatt_connect.h"
#include "le_audio_client.h"
#include "gatt_service_discovery.h"
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */
#include "bt_device_class.h"

#include <app_task.h>
#include <authentication.h>
#include <unexpected_message.h>
#include <temperature.h>
#include <pairing.h>
#include <power_manager.h>
#include <power_manager_action.h>
#include <connection_manager_config.h>
#include <device_db_serialiser.h>
#include <pio_monitor.h>
#include <local_addr.h>
#include <local_name.h>
#include <connection_message_dispatcher.h>
#include <ui.h>
#include <volume_messages.h>
#include <audio_sources.h>
#include <volume_service.h>
#include <le_scan_manager.h>
#include <focus_select.h>
#include <usb_device.h>
#include <usb_application.h>
#include <usb_app_source_audio.h>

#include <bredr_scan_manager.h>
#include <connection_manager.h>
#include <device_list.h>
#include <dfu.h>
#include <charger_monitor.h>
#include <message_broker.h>
#include <profile_manager.h>
#include <power_manager_sm.h>
#include <ui_indicator_prompts.h>
#include <led_manager.h>
#include <av.h>
#include <ui.h>
#include <ui_indicator_leds.h>
#include <system_state.h>
#include <panic.h>
#include <pio.h>
#include <stdio.h>
#include <feature.h>
#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
#include <aghfp_profile.h>
#include <inquiry_manager.h>
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */
#include <telephony_service.h>
#include <telephony_messages.h>
#include <rssi_pairing.h>
#include <bandwidth_manager.h>
#include <sink_service.h>
#include <transport_manager.h>

#include "input_event_manager.h"
#include <app/bluestack/dm_prim.h>

#ifdef INCLUDE_QCOM_CON_MANAGER
#include <qualcomm_connection_manager.h>
#endif

/* Definition of application log level */
DEBUG_LOG_DEFINE_LEVEL_VAR

/*! The maximum number of non-BT devices supported by the application.
    Used to determine the Device List size.
    \note This include USB & Analogue Audio devices.
*/
#define APP_CONFIG_MAX_NON_BT_DEVICES (USB_DEVICE_MAX_USB_HOSTS_SUPPORTED + WIRED_AUDIO_MAX_ANALOG_SOURCE_SUPPORTED)

/*! \brief Transport manager does not have proper init function, so add a fix for it   */
static bool usb_dongleInitTransportManagerInitFixup(Task init_task)
{
    UNUSED(init_task);

    TransportMgrInit();

    return TRUE;
}

/*! \brief Utility function to Init device DB serialiser   */
static bool appInitDeviceDbSerialiser(Task init_task)
{
    UNUSED(init_task);

    DeviceDbSerialiser_Init();
    /* Register persistent device data users */
    BtDevice_RegisterPddu();

    /* Allow space in device list to store all paired devices + non BT devices*/
    DeviceList_Init(APP_CONFIG_MAX_PAIRED_DEVICES + APP_CONFIG_MAX_NON_BT_DEVICES);
    /* Set maximum supported BT devices */
    BtDevice_SetMaxTrustedDevices(APP_CONFIG_MAX_PAIRED_DEVICES);
    DeviceDbSerialiser_Deserialise();

    return TRUE;
}

/*! \brief Utility function to get input actions  */
static const InputActionMessage_t* usb_dongleInitGetInputActions(uint16* input_actions_dim)
{
    const InputActionMessage_t* input_actions = NULL;

    *input_actions_dim = ARRAY_DIM(default_message_group);
    input_actions = default_message_group;
    return input_actions;
}
/*! \brief Utility function to init event manager   */
static bool appInputEventMangerInit(Task init_task)
{
    const InputActionMessage_t* input_actions = NULL;
    uint16 input_actions_dim = 0;
    UNUSED(init_task);

    input_actions = usb_dongleInitGetInputActions(&input_actions_dim);
    PanicNull((void*)input_actions);
    InputEventManagerInit(Ui_GetUiTask(), input_actions,
                            input_actions_dim, &input_event_config);
    return TRUE;
}

#ifdef UNMAP_AFH_CH78
/*! Unmap AFH channel 78

    It is need to meet regulatory requirements when QHS is used.
*/
static bool usbDongle_RemapAfh78(Task init_task)
{
    static const uint8_t afh_map[10] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f};

    UNUSED(init_task);

    MESSAGE_MAKE(prim, DM_HCI_SET_AFH_CHANNEL_CLASS_REQ_T);
    prim->common.op_code = DM_HCI_SET_AFH_CHANNEL_CLASS_REQ;
    prim->common.length = sizeof(DM_HCI_SET_AFH_CHANNEL_CLASS_REQ_T);
    memcpy(prim->map, afh_map, sizeof(afh_map));

    VmSendDmPrim(prim);

    return TRUE;
}
#endif

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
/*! \brief Function to initialise application AV interface.

    Used to initialise A2DP & AVRCP related functionality in the application.
*/
static bool usb_dongleAvSourceInitCfmHandler(Message message)
{
    UNUSED(message);
    UsbDongle_A2dpSourceInit();
    UsbDongle_HfpInit();
    return TRUE;
}
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INIT_DEBUG
/*! Debug function blocks execution until appInitDebugWait is cleared:
    apps1.fw.env.vars['usb_dongleInitDebugWait'].set_value(0) */
static bool usb_dongleInitDebug(Task init_task)
{
    volatile static bool usb_dongleInitDebugWait = TRUE;
    while(usb_dongleInitDebugWait);

    UNUSED(init_task);
    return TRUE;
}
#endif

static bool usb_dongleUsbApplication(Task init_task)
{
   UNUSED(init_task);

#if defined(INCLUDE_USB_AUDIO)
   UsbApplication_Open(&usb_app_source_audio);
#endif /* INCLUDE_USB_AUDIO */

   return TRUE;
}

#ifdef INCLUDE_DFU
static bool usb_dongleInit_DfuSetAppVersionInfo(Task init_task)
{
    UNUSED(init_task);
    DEBUG_LOG_FN_ENTRY("chargerCaseInit_DfuAppRegister");

    Dfu_SetVersionInfo(APP_CONFIG_INITIAL_UPGRADE_VERSION_MAJOR,
                       APP_CONFIG_INITIAL_UPGRADE_VERSION_MINOR,
                       APP_CONFIG_INITIAL_PS_DATA_CONFIG_VERSION);

    return TRUE;
}
#endif /* INCLUDE_DFU */

/*! \brief Table of initialisation functions */
static const system_state_step_t usb_dongleInitTable[] =
{
#ifdef INIT_DEBUG
    {usb_dongleInitDebug,      0, NULL},
#endif
    {PioMonitorInit,        0, NULL},
    {Ui_Init,               0, NULL},
#ifdef INCLUDE_CHARGER
    {Charger_Init,          0, NULL},
#endif
    {LedManager_Init,       0, NULL},
    {appPowerInit,          APP_POWER_INIT_CFM, NULL},
#ifdef INCLUDE_DFU
    {Dfu_EarlyInit,         0, NULL},
#endif /* INCLUDE_DFU */
    {AppConnectionInit, INIT_CL_CFM, NULL},
#ifdef UNMAP_AFH_CH78
    {usbDongle_RemapAfh78,       0, NULL},
#endif
    {AppMessageDispatcherRegister, 0, NULL},
    {appInputEventMangerInit, 0, NULL},
    {LocalAddr_Init,        0, NULL},
    {ConManagerInit,        0, NULL},
    {appInitDeviceDbSerialiser, 0, NULL},
    {LocalName_Init,        LOCAL_NAME_INIT_CFM, NULL},
    {appDeviceInit,         INIT_READ_LOCAL_BD_ADDR_CFM, appDeviceHandleClDmLocalBdAddrCfm},
    {BandwidthManager_Init, 0, NULL},
    {BredrScanManager_Init, BREDR_SCAN_MANAGER_INIT_CFM, NULL},
#if defined(USE_SYNERGY) && defined(INCLUDE_SOURCE_APP_LE_AUDIO)
    {GattConnect_RegisterDB, GATT_CONNECT_SERVER_INIT_COMPLETE_CFM , NULL},
#endif
    {Volume_InitMessages,   0, NULL},
    {AudioSources_Init,     0, NULL},
    {Pairing_Init,          PAIRING_INIT_CFM, NULL},
    {FocusSelect_Init,      0, NULL},
    {UsbDevice_Init,        0, NULL},
    {usb_dongleUsbApplication, 0, NULL},
    {appKymeraInit,         0, NULL},
    {UsbDongle_AudioInit, 0, NULL},
#ifdef INCLUDE_QCOM_CON_MANAGER
    {QcomConManagerInit, QCOM_CON_MANAGER_INIT_CFM,NULL},
#endif
#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
    {GattConnect_Init,     0, NULL},
    {GattHandlerInit,       0, NULL},
    /* This needs to be initialised before any other modules, which registers
     * services for discovery on initialisation.
     */
    {GattServiceDiscovery_Init,  0, NULL},
    {LeScanManager_Init,  0, NULL},
    {LeAudioClient_Init,  LE_AUDIO_CLIENT_INIT_CFM,  NULL},
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */
    {usb_dongleInitTransportManagerInitFixup, 0, NULL},        /*! \todo TransportManager does not meet expected init interface */

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
    {ProfileManager_Init,   0, NULL},
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */
#ifdef INCLUDE_DFU
    {usb_dongleInit_DfuSetAppVersionInfo, 0, NULL}, /* Must be called before Dfu_Init */
    {Dfu_Init,              UPGRADE_INIT_CFM, NULL},
    {UsbDongleDfu_Init,   0, NULL},
#endif /* INCLUDE_DFU */
    {VolumeService_Init,    0, NULL},
    {UiLeds_Init,           0, NULL},
    {UsbDongleUi_Init,    0, NULL},
    {Telephony_InitMessages, 0, NULL},

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
    {AghfpProfile_Init, APP_AGHFP_INIT_CFM, NULL},
    {appAvInit,             AV_INIT_CFM, usb_dongleAvSourceInitCfmHandler},
    {UsbDongleAdvertising_Init, 0, NULL},
    {InquiryManager_Init, 0, NULL},
    {UsbDongleInquiry_Init, 0, NULL},
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

    {RssiPairing_Init, 0, NULL},
    {SinkService_Init, 0, NULL},
#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
    {UsbDongle_LeaInit, 0, NULL},
    {UsbDongleHid_Init, 0, NULL},
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) || defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)
    {UsbDongle_VolumeObserverInit, 0, NULL},
#endif /* defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) || defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) */

    /* Must be after Ui_Init, Dfu_EarlyInit, ConManagerInit (registers as a client)
     * Also after appAvInit, UsbDevice_Init, UsbDongle_AudioInit */
    {UsbDongleSmInit,     0, NULL},
};

static bool finalPowerOffStep(Task task)
{
    UNUSED(task);
    appPowerDoPowerOff();

    return TRUE;
}

static const system_state_step_t shutdown_table[] =
{
    {finalPowerOffStep, 0, NULL}
};

static void usbDongleInit_SetMessageBrokerRegistrations(void)
{
    unsigned registrations_array_dim = (unsigned)message_broker_group_registrations_end -
                              (unsigned)message_broker_group_registrations_begin;
    PanicFalse((registrations_array_dim % sizeof(message_broker_group_registration_t)) == 0);
    registrations_array_dim /= sizeof(message_broker_group_registration_t);
    MessageBroker_Init(message_broker_group_registrations_begin,
                       registrations_array_dim);
}

void UsbDongleInit_StartInitialisation(void)
{
    UsbDongle_StartBtInit();

    LedManager_SetHwConfig(&usb_dongle_led_config);

    usbDongleInit_SetMessageBrokerRegistrations();

    usb_dongleSetBundlesConfig();

    SystemState_Init();
    SystemState_RemoveLimboState();

    SystemState_RegisterForStateChanges(appGetAppTask());
    SystemState_RegisterTableForInitialise(usb_dongleInitTable, ARRAY_DIM(usb_dongleInitTable));
    SystemState_RegisterTableForShutdown(shutdown_table, ARRAY_DIM(shutdown_table));
    SystemState_RegisterTableForEmergencyShutdown(shutdown_table, ARRAY_DIM(shutdown_table));

    SystemState_Initialise();
}

/*! \brief Initialize usb_dongle UI */
static void usb_dongleInit_CompleteUiInitialisation(void)
{
    const ui_config_table_content_t* config_table;
    unsigned config_table_size;


    config_table = UsbDongleUi_GetConfigTable(&config_table_size);

    Ui_SetConfigurationTable(config_table, config_table_size);

    /* UI and App is completely initialized, system is ready for inputs */
    PioMonitorEnable();
}
static void usb_donglehandleUnexpectedMessage(MessageId id)
{
    DEBUG_LOG_VERBOSE("usb_donglehandleUnexpectedMessage, 0x%x", (id));
}

static void usb_donglehandleUnexpectedSysMessage(MessageId id)
{
    DEBUG_LOG_VERBOSE("usb_donglehandleUnexpectedSysMessage, 0x%x", (id));
}

void UsbDongleInit_CompleteInitialisation(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongleInit_CompleteInitialisation");

    usb_dongleInit_CompleteUiInitialisation();

    /* complete power manager initialisation*/
    appPowerInitComplete();

    UnexpectedMessage_RegisterHandler(usb_donglehandleUnexpectedMessage);
    UnexpectedMessage_RegisterSysHandler(usb_donglehandleUnexpectedSysMessage);

    usb_dongleSetupAudio();

    SystemState_StartUp();

    UsbDongleUi_ConfigureFocusSelection();

    DEBUG_LOG_INFO("UsbDongleInit_CompleteInitialisation, completed initialisation");
}


#endif /* USB_DONGLE_INIT_C */
