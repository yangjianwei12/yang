/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       usb_dongle_config.h
\brief      Application configuration file
*/
#include "rssi_pairing.h"
#include "sink_service.h"
#include "profile_manager.h"
#include "usb_dongle_sm.h"

#ifndef USB_DONGLE_CONFIG_H_
#define USB_DONGLE_CONFIG_H_

/*! The maximum number of paired devices supported by the application.
    Used to determine the Trusted Device List (TDL) size.
    \note The connection library places an upper limit of 8 devices on this.
*/
#define APP_CONFIG_MAX_PAIRED_DEVICES 8

/*! \name Initial Application Upgrade Version
    Factory-default upgrade version of this app. After a successful upgrade,
    the upgrade version from the upgrade file header will be used instead.
    @{
*/
#define APP_CONFIG_INITIAL_UPGRADE_VERSION_MAJOR 1
#define APP_CONFIG_INITIAL_UPGRADE_VERSION_MINOR 0
/*! @} */

/*! Initial Persistant Store (PS) data config version.
    Factory-default PS data config version of this app. After a successful upgrade,
    the PS config version from the upgrade file header will be used instead.
*/
#define APP_CONFIG_INITIAL_PS_DATA_CONFIG_VERSION 1

/*! \name Sink Service Configuration Items
    @{
*/

/*! Maximum number of seconds to attempt reconnection to a previously paired sink.
	The page timeout ranges from 1 to 40 seconds.
	Pairing may fail to complete if the value is below 5.
*/
#define APP_CONFIG_PAGE_TIMEOUT_S 10
/*! @} */

/*! \name Inquiry Parameters
    @{
*/

/*! Maximum number of devices to consider per inquiry window.
	The inquiry responses ranges from 1 to 255 devices.
*/
#define APP_CONFIG_INQUIRY_MAX_RESPONSES 20
/*! Duration of each inquiry window, in units of 1.28s.
	The inquiry timeout ranges from 1 to 48 (in units of 1.28s).	
*/
#define APP_CONFIG_INQUIRY_TIMEOUT 10
#define APP_CONFIG_INQUIRY_COD_FILTER (AUDIO_MAJOR_SERV_CLASS) /*!< Limit results to a certain Class of Device (COD). */
/*! @} */

/*! \name RSSI Pairing Parameters
    @{
*/

/*! Devices below this RSSI value will not be considered for pairing.
	The RSSI threshold ranges from -127 to +20dBm.
*/
#define APP_CONFIG_RSSI_PAIRING_RSSI_THRESHOLD -60
/*! Minimum RSSI gap between 1st and 2nd devices in the scan list, to avoid ambiguity.
	The RSSI gap can range from 0 to 147dBm.
*/
#define APP_CONFIG_RSSI_PAIRING_MIN_RSSI_GAP 5
#define APP_CONFIG_RSSI_PAIRING_INQUIRY_PARAM_SET 0 /*!< Set of inquiry parameters to use for RSSI pairing (index into inquiry_params_set). */
/*! Maximum number of inquiry window iterations to perform during RSSI pairing.
	The inquiry count ranges from 0 to 65535.
*/
#define APP_CONFIG_RSSI_PAIRING_INQUIRY_COUNT 10
/*! @} */

/*! \name AptX Adaptive Latency Configuration
    @{
*/
#define APP_CONFIG_LATENCY_TARGET_APTX_AD_LL_USB_TO_HS      45  /*!< Latency for aptX Adaptive LL mode, USB input, Headset output */
#define APP_CONFIG_LATENCY_TARGET_APTX_AD_LL_USB_TO_TWM     65  /*!< Latency for aptX Adaptive LL mode, USB input, TWM output */
#define APP_CONFIG_LATENCY_TARGET_APTX_AD_LL_ANALOG_TO_HS   45  /*!< Latency for aptX Adaptive LL mode, Analog input, Headset output */
#define APP_CONFIG_LATENCY_TARGET_APTX_AD_LL_ANALOG_TO_TWM  65  /*!< Latency for aptX Adaptive LL mode, Analog input, TWM output */
#define APP_CONFIG_LATENCY_TARGET_APTX_AD_HQ_MODE           250 /*!< Latency for aptX Adaptive HQ mode (all inputs, all sink types) */
/*! @} */

/*! \name Optional Application Configuration Items
    @{
*/

/*! \def DISCONNECT_ON_USB_SUSPEND
    Disconnect all devices, and prevent connections, when USB goes into suspend mode.
*/
#ifndef APP_CONFIG_DISCONNECT_ON_USB_SUSPEND
#define APP_CONFIG_DISCONNECT_ON_USB_SUSPEND FALSE
#endif

#ifdef HAVE_EXTERNAL_PA
/* External PA drive PIO to configure */
#define EXTERNAL_PA_CONFIG_PIO     17
#endif

/*! @} */

/*! \name Dynamic Application Configuration Items
    @{
*/

/*! \def Masks to indicate mode status change indication */
#define USB_DONGLE_MODE_CHANGE_STATUS_NO_CHANGE                 (0u)
#define USB_DONGLE_MODE_CHANGE_STATUS_AUDIO_MODE_UPDATED        (1u << 0u)
#define USB_DONGLE_MODE_CHANGE_STATUS_TRANSPORT_MODE_UPDATED    (1u << 1u)

/*! \brief USB dongle mode of operation */
typedef enum
{
    usb_dongle_audio_mode_high_quality,
    usb_dongle_audio_mode_gaming,

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
    usb_dongle_audio_mode_broadcast,
#endif

    usb_dongle_audio_mode_max,
    usb_dongle_audio_mode_invalid = 0xFF
} usb_dongle_audio_mode_t;

/*! \brief USB dongle mode of operation */
typedef enum
{
    usb_dongle_transport_mode_unknown = 0x00,

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
    usb_dongle_transport_mode_bredr = 0x01,
#endif

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
    usb_dongle_transport_mode_le_audio = 0x02,
#endif

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) && defined(INCLUDE_SOURCE_APP_LE_AUDIO)
    usb_dongle_transport_mode_dual = 0x03,
#endif

    usb_dongle_transport_mode_max,
    usb_dongle_transport_mode_invalid = 0xFF
} usb_dongle_transport_mode_t;

/*! \brief USB dongle mode configuration */
typedef struct
{
    usb_dongle_audio_mode_t         audio_mode;             /*!< Audio mode */
    usb_dongle_transport_mode_t     transport_mode;         /*!< Transport mode */
    usb_dongle_audio_mode_t         new_audio_mode;         /*!< New Audio mode to apply */
    usb_dongle_transport_mode_t     new_transport_mode;     /*!< New Transport mode to apply */
    sink_service_transport_t        connected_transport;    /*!< Connected transport. ie, LE/BREDR */
} usb_dongle_config_t;

extern const rssi_pairing_parameters_t usb_dongle_config_rssi_params;
extern const profile_t usb_dongle_config_profile_list[];
extern usb_dongle_config_t usb_dongle_config;

extern const sink_service_transport_t usb_dongle_config_mode_vs_transport_table[];

#define usbDongleConfig_GetTransportForAudioMode(mode)      (usb_dongle_config_mode_vs_transport_table[mode])

/*! Get the USB dongle audio mode preference */
#define usbDongleConfig_GetAudioMode()                      (usb_dongle_config.audio_mode)
#define usbDongleConfig_GetNewAudioMode()                   (usb_dongle_config.new_audio_mode)
#define usbDongleConfig_IsInGamingAudioMode()               (usb_dongle_config.audio_mode == usb_dongle_audio_mode_gaming)
#define usbDongleConfig_IsInHighQualityAudioMode()          (usb_dongle_config.audio_mode == usb_dongle_audio_mode_high_quality)
#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
#define usbDongleConfig_IsInBroadcastAudioMode()            (usb_dongle_config.audio_mode == usb_dongle_audio_mode_broadcast)
#else
#define usbDongleConfig_IsInBroadcastAudioMode()            (FALSE)
#endif

/*! Get the USB dongle transport mode preference */
#define usbDongleConfig_GetTransportMode()                  (usb_dongle_config.transport_mode)
#define usbDongleConfig_GetNewTransportMode()               (usb_dongle_config.new_transport_mode)

/*! Get the sink service connected transport */
#define usbDongleConfig_SetConnectedTransport(transport)    (usb_dongle_config.connected_transport = transport)
#define usbDongleConfig_GetConnectedTransport()             (usb_dongle_config.connected_transport)
#define usbDongleConfig_IsConnectedToBredrSink()            (usb_dongle_config.connected_transport == SINK_SERVICE_TRANSPORT_BREDR)
#define usbDongleConfig_IsConnectedToLeAudioSink()          (usb_dongle_config.connected_transport == SINK_SERVICE_TRANSPORT_LE)

#ifdef DISABLE_COLOCATED_BROADCAST
#define usbDongleConfig_IsPairingDisallowed()               (usbDongleConfig_IsInBroadcastAudioMode())
#else
#define usbDongleConfig_IsPairingDisallowed()               (FALSE)
#endif

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) && defined(INCLUDE_SOURCE_APP_LE_AUDIO)

/*! Check if USB dongle is in LE mode or not */
#define usbDongleConfig_IsInLeAudioMode()                   (usbDongleConfig_GetTransportMode() == usb_dongle_transport_mode_le_audio)

/*! Check if USB dongle is in BREDR mode or not */
#define usbDongleConfig_IsInBredrMode()                     (usbDongleConfig_GetTransportMode() == usb_dongle_transport_mode_bredr)

/*! Check if USB dongle is in dual mode or not */
#define usbDongleConfig_IsInDualMode()                      (usbDongleConfig_GetTransportMode() == usb_dongle_transport_mode_dual)

/*! Check if USB dongle is in dual mode and connected with a BREDR sink */
#define usbDongleConfig_IsInDualModeWithBredrConnected()    (usbDongleConfig_IsInDualMode() && usbDongleConfig_IsConnectedToBredrSink())

/*! Check if USB dongle is in dual mode and connected with an LE audio sink */
#define usbDongleConfig_IsInDualModeWithLeConnected()       (usbDongleConfig_IsInDualMode() && usbDongleConfig_IsConnectedToLeAudioSink())

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
/*! Check if given audio mode and transport mode combination is valid or not.
    Broadcast audio mode and BREDR transport mode is not allowed. */
#define usbDongleConfig_IsModeCombinationValid(audio_mode, transport_mode) \
                                                                      !(audio_mode == usb_dongle_audio_mode_broadcast && \
                                                                       transport_mode == usb_dongle_transport_mode_bredr)
#else
/*! Check if given audio mode and transport mode combination is valid or not */
#define usbDongleConfig_IsModeCombinationValid(audio_mode, transport_mode) \
                                                                      (TRUE)
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#else /* defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) && defined(INCLUDE_SOURCE_APP_LE_AUDIO) */

/*! Check if given audio mode and transport mode combination is valid or not */
#define usbDongleConfig_IsModeCombinationValid(audio_mode, transport_mode) \
                                                                      (TRUE)

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO)

/*! Check if USB dongle is in LE mode or not */
#define usbDongleConfig_IsInLeAudioMode()                   (FALSE)

/*! Check if USB dongle is in BREDR mode or not */
#define usbDongleConfig_IsInBredrMode()                     (TRUE)

/*! Check if USB dongle is in dual mode or not */
#define usbDongleConfig_IsInDualMode()                      (FALSE)

/*! Check if USB dongle is in dual mode and connected with a BREDR sink */
#define usbDongleConfig_IsInDualModeWithBredrConnected()    (FALSE)

/*! Check if USB dongle is in dual mode and connected with an LE audio sink */
#define usbDongleConfig_IsInDualModeWithLeConnected()       (FALSE)

#elif defined(INCLUDE_SOURCE_APP_LE_AUDIO)

/*! Check if USB dongle is in LE mode or not */
#define usbDongleConfig_IsInLeAudioMode()                   (TRUE)

/*! Check if USB dongle is in BREDR mode or not */
#define usbDongleConfig_IsInBredrMode()                     (FALSE)

/*! Check if USB dongle is in dual mode or not */
#define usbDongleConfig_IsInDualMode()                      (FALSE)

/*! Check if USB dongle is in dual mode and connected with a BREDR sink */
#define usbDongleConfig_IsInDualModeWithBredrConnected()    (FALSE)

/*! Check if USB dongle is in dual mode and connected with an LE audio sink */
#define usbDongleConfig_IsInDualModeWithLeConnected()       (FALSE)

#endif
#endif /* defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) && defined(INCLUDE_SOURCE_APP_LE_AUDIO) */

/*! Check if USB dongle is in BREDR mode or dual mode with BREDR sink connected */
#define usbDongleConfig_IsInModeBredrOrDualWithBredrConnected()       (usbDongleConfig_IsInBredrMode() || \
                                                                       usbDongleConfig_IsInDualModeWithBredrConnected())

/*! Check if USB dongle is in LE audio mode or dual mode with LE audio sink connected */
#define usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected()        (usbDongleConfig_IsInLeAudioMode() || \
                                                                       usbDongleConfig_IsInDualModeWithLeConnected())

/*! Initialises USB dongle config data */
void UsbDongleConfigInit(void);

/*! \brief Saves mode into PsStore if modified.

    \param audio_mode - Audio mode (usb_dongle_audio_mode_invalid for not update audio mode).
    \param transport_mode - Transport mode (usb_dongle_transport_mode_invalid for not update transport mode).

    \return 0 means no change, any other value audio/transport mode changed (see USB_DONGLE_MODE_CHANGE_STATUS_XXX).
*/
uint8 UsbDongleConfig_SaveMode(usb_dongle_audio_mode_t audio_mode, usb_dongle_transport_mode_t transport_mode);

/*! \brief New mode requested for later applying.

    \param audio_mode - Audio mode (usb_dongle_audio_mode_invalid for not update audio mode).
    \param transport_mode - Transport mode (usb_dongle_transport_mode_invalid for not update transport mode).

    \return 0 means no change, any other value audio/transport mode changed (see USB_DONGLE_MODE_CHANGE_STATUS_XXX).
*/
uint8 UsbDongleConfig_SetNewMode(usb_dongle_audio_mode_t audio_mode, usb_dongle_transport_mode_t transport_mode);

/*! @} */

#endif /* USB_DONGLE_CONFIG_H_ */
