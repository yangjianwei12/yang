/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file
\brief      Charger Case application state machine.
*/

#ifndef CHARGERCASE_SM_H_
#define CHARGERCASE_SM_H_

#include <message.h>

/*!
@startuml

    note "Charger Case Application States" as N1
   
    [*] -down-> CHARGERCASE_STATE_NULL: HS State Init
    CHARGERCASE_STATE_NULL -down->CHARGERCASE_STATE_LIMBO :SSM transition to system_state_limbo
    CHARGERCASE_STATE_NULL: Initial state
    state ChargerCaseisactive {
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
    ChargerCaseisactive -up->CHARGERCASE_STATE_POWERING_OFF: User Initiated Power OFF
    ChargerCaseisactive -up->CHARGERCASE_STATE_TERMINATING : Emergency Shutdown
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
        Charger Case will be in Limbo
        Else after Limbo timer 
        expires state transition
        to PowerdOff
    end note
    state CHARGERCASE_FACTORY_RESET:Reset the system, Can be entered after Power On
    CHARGERCASE_FACTORY_RESET-left->[*]
@enduml
*/

/*! \brief Charger Case application states. */
typedef enum sm_charger_case_states
{
    /*! Initial state before state machine is running. */
    CHARGER_CASE_STATE_INIT = 0,
    CHARGER_CASE_STATE_FACTORY_RESET,
    CHARGER_CASE_STATE_IDLE,
    CHARGER_CASE_STATE_PAIRING,
    CHARGER_CASE_STATE_CONNECTING,
    CHARGER_CASE_STATE_CONNECTED,
    CHARGER_CASE_STATE_AUDIO_STARTING,
    CHARGER_CASE_STATE_AUDIO_STREAMING,
    CHARGER_CASE_STATE_AUDIO_STOPPING,
    CHARGER_CASE_STATE_DISCONNECTING,

} charger_case_state_t;

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

/*! \brief Application state machine task data structure. */
typedef struct
{
    TaskData task;                      /*!< SM task */
    charger_case_state_t state;         /*!< Application state */
    bool pairing_delete_requested;

} charger_case_sm_data_t;


/*! Application state machine task data instance. */
extern charger_case_sm_data_t charger_case_sm;

/*! Get pointer to application state machine task data. */
#define SmGetTaskData() (&charger_case_sm)

/*! \brief Initialise the main application state machine.
 */
bool ChargerCaseSmInit(Task init_task);

/*! \brief Provides state manager (SM) task to other components

    \return Task - charger_case SM task.
*/
Task ChargerCaseSmGetTask(void);

charger_case_state_t ChargerCaseSm_GetState(void);

#endif /* CHARGERCASE_SM_H_ */

