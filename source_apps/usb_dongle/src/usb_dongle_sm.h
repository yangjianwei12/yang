/*!
\copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file
\brief      USB Dongle application state machine.
*/

#ifndef USB_DONGLE_SM_H_
#define USB_DONGLE_SM_H_

#include <bdaddr.h>
#include <device.h>
#include <message.h>

/*!
@startuml

    note "USB Dongle Application States" as N1
   
    [*] -down-> CHARGERCASE_STATE_NULL: HS State Init
    CHARGERCASE_STATE_NULL -down->CHARGERCASE_STATE_LIMBO :SSM transition to system_state_limbo
    CHARGERCASE_STATE_NULL: Initial state
    state UsbDongleisactive {
        CHARGERCASE_STATE_LIMBO->CHARGERCASE_STATE_LIMBO: Start Limbo Timer
        CHARGERCASE_STATE_LIMBO: Stable state after Startup
        CHARGERCASE_STATE_LIMBO->CHARGERCASE_STATE_POWERING_ON:User Initiated Power On/SSM transition to system_state_powering_on
        CHARGERCASE_STATE_POWERING_ON-down->CHARGERCASE_STATE_IDLE: If already a device in PDL
        CHARGERCASE_STATE_IDLE:Can have BT connection or not
        CHARGERCASE_STATE_POWERING_ON -down-> CHARGERCASE_STATE_PAIRING: If no device in PDL
        CHARGERCASE_STATE_POWERING_ON : A transition state for powering On
        CHARGERCASE_STATE_PAIRING -> CHARGERCASE_STATE_IDLE: If pairing successful or failed
        CHARGERCASE_STATE_IDLE-down->CHARGERCASE_STATE_BUSY:Start Music Streaming/Call(BT)
        CHARGERCASE_STATE_IDLE-left->CHARGERCASE_STATE_IDLE:Start Idle timer in case of no activity(eg:No BT)
        CHARGERCASE_STATE_IDLE->CHARGERCASE_STATE_PAIRING:User Initiated Pairing
    }
    CHARGERCASE_STATE_IDLE-up->CHARGERCASE_STATE_POWERING_OFF:Idle Timer Expires and Ok to power off(eg:No BT)
    CHARGERCASE_STATE_BUSY->CHARGERCASE_STATE_IDLE:End Music Streaming/Call
    UsbDongleisactive -up->CHARGERCASE_STATE_POWERING_OFF: User Initiated Power OFF
    UsbDongleisactive -up->CHARGERCASE_STATE_TERMINATING : Emergency Shutdown
    CHARGERCASE_STATE_TERMINATING: State to handle Emergency Shutdown
    CHARGERCASE_STATE_TERMINATING->PowerdOff
    CHARGERCASE_STATE_POWERING_OFF ->CHARGERCASE_STATE_POWERING_OFF: Disconnect Link if needed
    CHARGERCASE_STATE_POWERING_OFF: State to handle normal PowerOFF
    CHARGERCASE_STATE_POWERING_OFF ->CHARGERCASE_STATE_LIMBO
    CHARGERCASE_STATE_LIMBO -left->PowerdOff:No ChargerConnected/Limbo Timer Expires
    state PowerdOff #LightBlue
    PowerdOff: This state is realized in hardware, either Dormant or Off
    note left of CHARGERCASE_STATE_LIMBO
        A Limbo timer and Charger 
        determines the transition
        to PowerdOff
        If Charger is connected
        USB Dongle will be in Limbo
        Else after Limbo timer 
        expires state transition
        to PowerdOff
    end note
    state CHARGERCASE_FACTORY_RESET:Reset the system, Can be entered after Power On
    CHARGERCASE_FACTORY_RESET-left->[*]
@enduml
*/

/*! \brief USB Dongle application states. */
typedef enum sm_usb_dongle_states
{
    /*! Initial state before state machine is running. */
    APP_STATE_INIT = 0,
    APP_STATE_IDLE,
    APP_STATE_PAIRING,
    APP_STATE_CONNECTING,
    APP_STATE_CONNECTED,
    APP_STATE_AUDIO_STARTING,
    APP_STATE_AUDIO_STREAMING,
    APP_STATE_AUDIO_STOPPING,
    APP_STATE_DISCONNECTING,
    APP_STATE_VOICE_STARTING,
    APP_STATE_VOICE_STREAMING,
    APP_STATE_VOICE_STOPPING,
} usb_dongle_state_t;

/*! \brief SM UI Provider contexts */
typedef enum
{
    context_app_sm_inactive,
    context_app_sm_active,
    context_app_sm_idle,
    context_app_sm_pairing,
    context_app_sm_connecting,
    context_app_sm_connected,
    context_app_sm_streaming,

} sm_provider_context_t;

/*! \brief Initialise the main application state machine.
 */
bool UsbDongleSmInit(Task init_task);

/*! \brief Provides state manager (SM) task to other components

    \return Task - usb_dongle SM task.
*/
Task UsbDongleSmGetTask(void);

/*! \brief Allows other components to access the current SM state.

    \return usb_dongle_state_t - usb_dongle current SM state.
*/
usb_dongle_state_t UsbDongleSm_GetState(void);

/*! \brief Allows other components to know which is the active sink device.

    If connected, this will be the currently connected sink, otherwise, it will
    be the sink device we were previously connected / intend to connect to.

    \return device_t - The current sink device, if any.
*/
device_t UsbDongleSm_GetCurrentSink(void);

/*! \brief Pair to a specific sink device with a known address.

    \param sink_addr - The address of the sink to pair to.
    \return bool - Whether pairing was successfully initiated or not.
*/
bool UsbDongleSm_PairSink(const bdaddr *sink_addr);

#endif /* USB_DONGLE_SM_H_ */

