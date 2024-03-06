/*!
\copyright  Copyright (c) 2019 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       headset_sm.h
\brief      headset application State machine.
*/

#ifndef HEADSET_SM_H_
#define HEADSET_SM_H_

#include "headset_sm_config.h"
#include "headset_rules.h"
#ifdef ENABLE_TWM_SPEAKER
#include "stereo_topology.h"
#endif

/*!
@startuml

    note "Headset Application States" as N1
   
    [*] -down-> HEADSET_STATE_NULL: HS State Init
    HEADSET_STATE_NULL -down->HEADSET_STATE_LIMBO :SSM transition to system_state_limbo
    HEADSET_STATE_NULL: Initial state
    state Headsetisactive {
        HEADSET_STATE_LIMBO->HEADSET_STATE_LIMBO: Start Limbo Timer
        HEADSET_STATE_LIMBO: Stable state after Startup
        HEADSET_STATE_LIMBO->HEADSET_STATE_POWERING_ON:User Initiated Power On/SSM transition to system_state_powering_on
        HEADSET_STATE_POWERING_ON-down->HEADSET_STATE_IDLE: If already a device in PDL
        HEADSET_STATE_IDLE:Can have BT connection or not
        HEADSET_STATE_POWERING_ON -down-> HEADSET_STATE_PAIRING: If no device in PDL
        HEADSET_STATE_POWERING_ON : A transition state for powering On
        HEADSET_STATE_PAIRING -> HEADSET_STATE_IDLE: If pairing successful or failed
        HEADSET_STATE_IDLE-down->HEADSET_STATE_BUSY:Start Music Streaming/Call(BT)
        HEADSET_STATE_IDLE-left->HEADSET_STATE_IDLE:Start Idle timer in case of no activity(eg:No BT)
        HEADSET_STATE_IDLE->HEADSET_STATE_PAIRING:User Initiated Pairing
    }
    HEADSET_STATE_IDLE-up->HEADSET_STATE_POWERING_OFF:Idle Timer Expires and Ok to power off(eg:No BT)
    HEADSET_STATE_BUSY->HEADSET_STATE_IDLE:End Music Streaming/Call
    Headsetisactive -up->HEADSET_STATE_POWERING_OFF: User Initiated Power OFF
    Headsetisactive -up->HEADSET_STATE_TERMINATING : Emergency Shutdown
    HEADSET_STATE_TERMINATING: State to handle Emergency Shutdown
    HEADSET_STATE_TERMINATING->PowerdOff
    HEADSET_STATE_POWERING_OFF ->HEADSET_STATE_POWERING_OFF: Disconnect Link if needed
    HEADSET_STATE_POWERING_OFF: State to handle normal PowerOFF
    HEADSET_STATE_POWERING_OFF ->HEADSET_STATE_LIMBO 
    HEADSET_STATE_LIMBO -left->PowerdOff:No ChargerConnected/Limbo Timer Expires 
    state PowerdOff #LightBlue
    PowerdOff: This state is realized in hardware, either Dormant or Off
    note left of HEADSET_STATE_LIMBO
        A Limbo timer and Charger 
        determines the transition
        to PowerdOff
        If Charger is connected
        Headset will be in Limbo
        Else after Limbo timer 
        expires state transition
        to PowerdOff
    end note
    state HEADSET_FACTORY_RESET:Reset the system, Can be entered after Power On
    HEADSET_FACTORY_RESET-left->[*]
@enduml
*/

/*! \brief Headset Application states.
 */
typedef enum sm_headset_states
{
    /*!< Initial state before state machine is running. */
    HEADSET_STATE_NULL                = 0x0000,
    HEADSET_STATE_FACTORY_RESET,
    HEADSET_STATE_LIMBO,
    HEADSET_STATE_POWERING_ON,
    HEADSET_STATE_PAIRING,
    HEADSET_STATE_IDLE,
    HEADSET_STATE_BUSY,
    HEADSET_STATE_TERMINATING,
    HEADSET_STATE_POWERING_OFF,
#ifdef ENABLE_TWM_SPEAKER
    HEADSET_STATE_PEER_PAIRING
#endif
} headsetState;

/*! \brief SM UI Provider contexts */
typedef enum
{
    context_app_sm_inactive,
    context_app_sm_active,
    context_app_sm_powered_off,
    context_app_sm_powered_on,
    context_app_sm_idle,
    context_app_sm_idle_connected,
    context_app_sm_exit_idle,

} sm_provider_context_t;

/*! \brief Main application state machine task data. */
typedef struct
{
    TaskData task;                          /*!< SM task */
    headsetState state;                     /*!< Application state */
    uint16 disconnect_lock;                 /*!< Disconnect message lock */
    bool user_pairing:1;                    /*!< User initiated pairing */
    bool auto_poweron:1;                    /*!< Auto power on flag */
    bool allow_rules_to_run:1;              /*! Can be used to check whether headset rules are allowed it run or not */
#ifdef ENABLE_TWM_SPEAKER
    bool spk_type_is_standalone:1;
    bool spk_party_mode:1;
    stereo_topology_find_role_t twm_role:2;
#endif /* ENABLE_TWM_SPEAKER */
#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
    bool spk_lea_broadcasting_media:1;
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
} smTaskData;

/*!< Application state machine. */
extern smTaskData headset_sm;

/*! Get pointer to application state machine task data struture. */
#define SmGetTaskData()          (&headset_sm)

/*! \brief Initialise the main application state machine.
 */
bool headsetSmInit(Task init_task);
/*! \brief provides state manager(sm) task to other components

    \return     Task - headset sm task.
*/
Task headsetSmGetTask(void);

/*! \brief Method to check if handset connection is prohibited or not.
     If it returns true, then connection is prohibited, else vice versa.
*/
bool headsetSmIsHandsetConnectionProhibited(void);

/*! \brief Method to check if headset rules are allowed to run or not.*/
bool headsetSmIsAllowedToRunHeadsetRules(void);

/*! \brief Application state machine message handler.
    \param task The SM task.
    \param id The message ID to handle.
    \param message The message content (if any).
*/
void headsetSmHandleMessage(Task task, MessageId id, Message message);

#ifdef ALLOW_WA_BT_COEXISTENCE
#define headsetSmWiredAudioConnected()
#else
/*! \brief Method to handle wired audio device arrival.*/
void headsetSmWiredAudioConnected(void);
#endif /* ALLOW_WA_BT_COEXISTENCE */

/*! \brief Method to handle wired audio device removal.*/

#ifdef ALLOW_WA_BT_COEXISTENCE
#define headsetSmWiredAudioDisconnected()
#else
void headsetSmWiredAudioDisconnected(void);
#endif /* ALLOW_WA_BT_COEXISTENCE */

/*! \brief Initiate disconnect of handset link */
bool headsetSmDisconnectLink(void);

/*! \brief Method to handle topology stop confirmation. */
bool headetSmHandleTopologyStopCfm(Message message);

/*! \brief Request to connect the most recently used handset.
*/
void headsetSmSetEventConnectMruHandset(void);

/*! \brief Request to disconnect the least recently used handset.
*/
void headsetSmSetEventDisconnectLruHandset(void);

/*! \brief Request to disconnect all handsets.
    \note This currently only disconnects one connected handset.
    \todo This functionality will need to be updated when multiple
          handsets connections are supported (multipoint)
*/
void headsetSmSetEventDisconnectAllHandsets(void);

/*! \brief function to allow headset rules to run

    Note: Headset Rules are meant to run in STEREO_TOPOLOGY_STATE_STARTED and
          disallowed after transitioning to STEREO_TOPOLOGY_STATE_STOPPING

    \param allow TRUE to allow headset connections to run rules, FALSE to disallow.
*/
void headsetSmSetAllowHeadsetRulesToRun(bool allow);

#endif /* HEADSET_SM_H_ */

