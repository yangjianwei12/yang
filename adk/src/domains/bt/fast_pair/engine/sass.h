/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       sass.h
\brief      Header file for the component handling GFP Smart Audio Source Switching.
*/

#ifndef SASS_H
#define SASS_H

#if (defined(INCLUDE_FAST_PAIR) && !defined(DISABLE_FP_SASS_SUPPORT))

#include "device.h"
#include <bdaddr.h>
#include <task_list.h>

typedef struct
{
    uint8 connection_state;
    uint8 custom_data;
    uint8 connected_devices_bitmap;
}sass_connection_status_field_t;

/* Message Code for SASS message group */
typedef enum
{
    SASS_GET_CAPABILITY_EVENT = 0x10,
    SASS_NOTIFY_CAPABILITY_EVENT = 0x11,
    SASS_SET_MP_STATE_EVENT = 0x12,
    SASS_SET_SWITCHING_PREFERENCE_EVENT = 0x20,
    SASS_GET_SWITCHING_PREFERENCE_EVENT = 0x21,
    SASS_NOTIFY_SWITCHING_PREFERENCE_EVENT = 0x22,
    SASS_SWITCH_AUDIO_SOURCE_EVENT = 0x30,
    SASS_SWITCH_BACK_EVENT = 0x31,
    SASS_NOTIFY_MP_SWITCH_EVENT = 0x32,
    SASS_GET_CONNECTION_STATUS_EVENT = 0x33,
    SASS_NOTIFY_CONNECTION_STATUS_EVENT = 0x34,
    SASS_NOTIFY_SASS_INITIATED_CONN_EVENT = 0x40,
    SASS_INUSE_ACC_KEY_IND_EVENT = 0x41,
    SASS_SEND_CUSTOM_DATA_EVENT = 0x42,
    SASS_SET_DROP_CONN_TARGET_EVENT = 0x43
} SASS_MESSAGE_CODE;

/*! \brief Audio status associated with a particular device or source */
typedef enum
{
    audio_status_disconnected,
    audio_status_connected,
    audio_status_playing,
    audio_status_paused,
    audio_status_interrupted
}sass_audio_status_t;

/*! \brief  Send connection status field updates to SASS advertising manager.

    \param is_conn_status_change  connection status update

*/
typedef void (*sass_connection_status_change_callback)(void);

/*! \brief Get bdaddr of switch to device
    \param None
    \return bdaddr* pointer to the address of switch to device
*/
bdaddr* Sass_GetSwitchToDeviceBdAddr(void);

/*! \brief Resume playing on switch to device after switching.
 */
bool Sass_ResumePlayingOnSwitchToDevice(void);

/*! \brief Reset the states related to Switch Active Audio Source event
    \param None
    \return void
*/
void Sass_ResetSwitchActiveAudioSourceStates(void);

/*! \brief Reset bdaddr of switch to device
    \param None
    \return void
*/
void Sass_ResetSwitchToDeviceBdAddr(void);

/*! \brief Reset bdaddr of switch away device
    \param None
    \return void
*/
void Sass_ResetSwitchAwayDeviceBdAddr(void);

/*! \brief Reset Resume playing on switch to device flag after switching.
 */
void Sass_ResetResumePlayingFlag(void);

 /*! \brief Notify multipoint switch to all the connected seekers when there is a change in active audio source
    \param switch_event_reason Reason for sending MP switch event
    \return None
*/
void Sass_NotifyMultipointSwitchEvent(uint8 switch_event_reason);

/*! \brief Set connection state data leaving on-head detection flag unchanged
 *  \param connection_state value to be set
 */
void Sass_SetConnectionStateExceptOnHeadFlag(uint8 connection_state);

/*! \brief Get current connection state.
 */
uint8 Sass_GetConnectionState(void);

/*! \brief Set custom data for current streaming.
 *  \param custom data value to be updated
 */
void Sass_SetCustomData(uint8 custom_data);

/*! \brief Get current custom data.
 */
uint8 Sass_GetCustomData(void);

/*! \brief Set connected devices bitmap data
 *  \param connected_devices_bitmap value to be set
 */
void Sass_SetConnectedDeviceBitMap(uint8 connected_devices_bitmap);

/*! \brief Get current connected devices bitmap. 
 */
uint8 Sass_GetConnectedDeviceBitMap(void);

/*! \brief SASS advertising manager shall register a callback with SASS plugin to get connection status updates.
 */
void Sass_RegisterForConnectionStatusChange(sass_connection_status_change_callback callBack);

/*! \brief Check if any account key associated with the given handset device
 */
bool Sass_IsAccountKeyAssociatedWithDevice(device_t device);

/*! \brief Checks if latest connected device was for SASS switching
    \param device Device instance
    \return TRUE if last connected device was for SASS switching, otherwise FALSE
*/
bool Sass_IsConnectedDeviceForSassSwitch(device_t device);

/*! \brief Checks if latest barge-in connection was for SASS switching
    \param None
    \return TRUE if last barge-in connection was for SASS switching, otherwise FALSE
*/
bool Sass_IsBargeInForSassSwitch(void);

/*! \brief Disable connection switch for SASS
 *  \param void
    \return void
*/
void Sass_DisableConnectionSwitch(void);

/*! \brief Enable connection switch for SASS
 *  \param void
    \return void
*/
void Sass_EnableConnectionSwitch(void);

/*! \brief Check if connection switching for SASS is disabled
 *  \param void
    \return bool TRUE if connection switch for SASS is disabled, FALSE otherwise
*/
bool Sass_IsConnectionSwitchDisabled(void);

/*! \brief Get the audio status for a handset device.
    \param device The requested handset device.
    \return The audio status, or audio_status_disconnected, if no audio status was set for
    the handset device.
*/
sass_audio_status_t Sass_GetAudioStatus(device_t device);

/*! \brief SASS Initialization.
 */
void Sass_Init(void);

#else
typedef void (*sass_connection_status_change_callback)(void);
typedef enum
{
    audio_status_disconnected,
    audio_status_connected,
    audio_status_playing,
    audio_status_paused,
    audio_status_interrupted
}sass_audio_status_t;
typedef struct
{
    uint8 connection_state;
    uint8 custom_data;
    uint8 connected_devices_bitmap;
}sass_connection_status_field_t;
#define Sass_GetSwitchToDeviceBdAddr()  NULL
#define Sass_ResumePlayingOnSwitchToDevice() FALSE
#define Sass_ResetSwitchActiveAudioSourceStates() 
#define Sass_ResetSwitchToDeviceBdAddr()
#define Sass_ResetSwitchAwayDeviceBdAddr()
#define Sass_ResetResumePlayingFlag()
#define Sass_NotifyMultipointSwitchEvent(x) UNUSED(x)
#define Sass_SetConnectionStateExceptOnHeadFlag(x) UNUSED(x)
#define Sass_GetConnectionState() 0
#define Sass_SetCustomData(x) UNUSED(x)
#define Sass_GetCustomData() 0
#define Sass_SetConnectedDeviceBitMap(x) UNUSED(x)
#define Sass_GetConnectedDeviceBitMap() 0
#define Sass_RegisterForConnectionStatusChange(x) UNUSED(x)
#define Sass_IsAccountKeyAssociatedWithDevice(x) (UNUSED(x),FALSE)
#define Sass_IsConnectedDeviceForSassSwitch(x) (UNUSED(x),FALSE)
#define Sass_IsBargeInForSassSwitch() FALSE
#define Sass_DisableConnectionSwitch()
#define Sass_EnableConnectionSwitch()
#define Sass_IsConnectionSwitchDisabled() TRUE
#define Sass_GetAudioStatus(x) (UNUSED(x),0)
#define Sass_Init() DEBUG_LOG("Sass_Init SASS is disabled")
#endif

#endif /* SASS_H */

