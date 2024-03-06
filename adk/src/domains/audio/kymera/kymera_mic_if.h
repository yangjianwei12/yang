/*!
\copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      MIC interface for kymera component
*/

#ifndef KYMERA_MIC_IF_H_
#define KYMERA_MIC_IF_H_

#include <microphones.h>

/*! Default microphone rate */
#define DEFAULT_MIC_RATE (16000)

/*!
\brief  List of each possible user.
        Since multiple users can coexist, each user has a separate bit.
*/
typedef enum
{
    mic_user_none = 0,
    mic_user_sco = (1 << 0),
    mic_user_custom = (1 << 1), // reserved for additional customer client
    mic_user_aanc = (1 << 2),   // Adaptive ANC
    mic_user_anc = (1 << 3),    // Classic ANC can optionally be registered if Adaptive ANC is not used
    mic_user_va  = (1 << 4),
    mic_user_leakthrough = (1 << 5),
    mic_user_usb_voice = (1 << 6),
    mic_user_le_voice = (1 << 7),
    mic_user_fit_test = (1 << 8),
    mic_user_le_mic = (1 << 9),
    mic_user_gaming_background = (1 << 10),
    mic_user_all_mask = ((1 << 11) - 1),
} mic_users_t;

/*!
\brief  Each user specifies if it is allowed or not to be interrupted.
        - mic_user_state_non_interruptible: No disconnection / reconnection is allowed while a non-interruptible user is active (e.g. SCO)
          If a new user wants to connect with a microphone that is not active, the connection will be rejected.
          If a user disconnects and a non-interruptible user is continuing to run, the microphone configuration will
          not be changed until the non-interruptible user is stopping.
        - mic_user_state_interruptible: The user allows to be disconnected and reconnected.
          If multiple users are active, a disconnection and reconnected is allowed if all active users are interruptible.
        - mic_user_state_always_interrupt: The user will be disconnected and reconnected each time another user connects or disconnects.
          This allows a user to adjust according to the current mic concurrency state.
*/
typedef enum
{
    mic_user_state_interruptible = 0,
    mic_user_state_always_interrupt,
    mic_user_state_non_interruptible,
} mic_user_state_t;

typedef struct
{
    uint8 num_of_mics;
    /* List in the order you want the mics in (first will be primary etc) */
    const uint16 *mic_ids;
    /* List in the same order as types */
    const Sink *mic_sinks;
} mic_connections_t;

typedef struct
{
    uint32 sample_rate;
    mic_connections_t connections;
} mic_connect_params_t;

/*!
\brief  List of possible events that are sent to the users to inform about
        the reason for their disconnection.
*/
typedef enum
{
    /*! Additional info when a new user connects:
     *  an extra mic is required by the new user. */
    mic_event_extra_mic = (1 << 0),
    /*! Additional info when a new user connects:
     *  a higher sample rate is required by the new user. */
    mic_event_higher_sample_rate = (1 << 1),
    /*! Additional info when a new user connects:
     *  a change in UCID is needed when switching Leakthrough on and off. */
    mic_event_ucid_change_required = (1 << 2),
    /*! Additional info when a new user connects:
     *  a chain rebuild is needed when switching the task period of aec_ref */
    mic_event_task_period_change_required = (1 << 3),
    /*! Additional info when a new user connects:
     *  a chain rebuild is needed when the client aec_ref requirement changes */
    mic_event_aec_ref_change_required = (1 << 4),
    /*! Additional info when a new user connects:
     *  a chain rebuild is needed when switching the ttp delay of aec_ref */
    mic_event_ttp_delay_change_required = (1 << 5),
    /*! mic_event_connecting will be sent when a new user connects. */
    mic_event_connecting = (1 << 6),
    /*! mic_event_disconnecting will be sent whe an existing user disconnects. */
    mic_event_disconnecting = (1 << 7)
} mic_event_t;

/*!
\brief  Complete disconnection info sent to all users
*/
typedef struct
{
    mic_users_t user;
    mic_event_t event;
} mic_change_info_t;

/*!
\brief  Callbacks to inform each active user about microphone related events or to read out dynamic values from the user.
*/
typedef struct
{
    /*! Mandatory: Mic interface requests all information about the microphones from a user */
    bool (*MicGetConnectionParameters)(uint16 *mic_ids, Sink *mic_sinks, uint8 *num_of_mics, uint32 *sample_rate, Sink *aec_ref_sink);

    /*! Optional: Mics might be disconnected in case of a concurrency. The reason is delivered in *info.
     *  All affected users are informed in advance about the disconnection.
     *  The disconnection is executed when the user is defined as interruptible or always_interrupt.
     *  Return TRUE if user wants to reconnect or FALSE if user wants to stop. */
    bool (*MicDisconnectIndication)(const mic_change_info_t *info);
    /*! Optional: When all users are disconnected, the users are informed with the ReadyForReconnection Indication */
    void (*MicReadyForReconnectionIndication)(const mic_change_info_t *info);
    /*! Optional: Indication for a successfully reconnection after a DisconnectIndication. */
    void (*MicReconnectedIndication)(void);
    /*! Optional: If a user has updated its state, all other registered users are informed. */
    void (*MicUserUpdatedState)(void);
    /*! Optional: Notifies users in advance about changes inside the framework.
     *  The difference between this notification and the MicDisconnectIndication is:
     *  When the Mic DisconnectIndication is received,
     *  the framework has already decided to disconnect a client based on the state of the user.
     *  This indication is sent out before each disconnection or connection of another user.
     *  Its parameter contains the same info as the MicDisconnectIndication.
     *  This indication can be used to e.g. change the user state in certain use cases to prevent being disconnected. */
    void (*MicUserChangePendingNotification)(const mic_change_info_t *info);
    /*! Optional: Defines if the user requires aec_ref output to be connected or not.
     *  If no callback is available, no aec_ref output usage is expected.
     *  This information can be changed by the users at runtime. */
    bool (*MicGetAecRefUsage)(void);
    /*! Optional: Reads the state of a user (interruptible, always_interrupt or non-interruptible).
     *  Interruptible allows the framework to disconnect and reconnect a user at any point in time in necessary.
     *  Non-interruptible forbits any change in microphone configuration during the lifetime of a user.
     *  Always_interrupt will disconnect and reconnect a user each time a new user is connecting. The difference between
     *  interruptible and always_interrupt is: An interruptible user is not reconnected when the connecting user has the same
     *  microphone requirements.
     *  If no callback is available, interruptible is expected.
     *  The state can be changed at runtime.
     *  To inform other users about a changed state, the function MicUserUpdatedState needs to be called. */
    mic_user_state_t (*MicGetUserState)(void);
    /*! Optional: defines the required TTP latency in microseconds.
     *  Each time a new user connects the required value of the new user is checked against the already programmed ttp delay.
     *  Changing this value prior to connecting the user is OK.
     *  The user shouldn't change the ttp delay while being connected. Since aec_ref is already running,
     *  the new value cannot be taken over.
     *  If multiple users request a TTP delay, the value must be the same. Otherwise the system will panic. */
    uint32 (*MicGetMandatoryTtpDelay)(void);
    /*! Clients can register this function to return a UCID offset to mic framework for AEC_REF operator. */
    uint8 (*MicGetUcidOffset)(void);
} mic_callbacks_t;

/*!
\brief  These registration values are read in at registration time.
*/
typedef struct
{
    /*! Mic interface will connect to all mandatory microphones of all registered users
    *   in case a non-interruptible client is connnecting. With that a transition between different
    *   use case can be achieved without discontinuities. */
    uint8 num_of_mandatory_mics;
    const uint16 *mandatory_mic_ids;

    /*! mandatory_task_period_us defines the required task period for each user in microseconds.
     *  If the value is not written, the default 2000 us task period will be used.
     *  The user shouldn't change the task period after registration. */
    const uint32 *mandatory_task_period_us;

    /*! If activate_timestamps is set, a user needs timestamp information propagated through the chain.
     *  active_timestamps flag is intended for users that do not need a specific ttp_delay.
     *  A default ttp_delay of 40ms is set if no other user requests a ttp_delay value.
     *  If another user requests a different ttp_delay, this ttp_delay will be set.
     *  The ttp_delay value can change dynamically. */
    const unsigned activate_timestamps:1;

} mic_permanent_registry_per_user_t;

/*!
\brief  User registration structure
*/
typedef struct
{
    /*! registering user */
    mic_users_t user;
    /*! Users will be informed about events via callbacks, see mic_callbacks_t */
    const mic_callbacks_t *callbacks;

    /*! These registration values are read in at registration time. */
    mic_permanent_registry_per_user_t permanent;

} mic_registry_per_user_t;

/*! \brief  Connects requested microphones to a user chain.
            The connection from and to AEC Reference is handled by this function.
            Using the registered callback functions Kymera_MicConnect requests the parameters for the connection from the user.
            The function will take care about concurrency chains.
            If a conflicting situation occurs (e.g. another user is active but uses different microphones or
            the requested sample rate is higher thant the current sample rate), a reconnection of all users is necessary.
            Depending on the state of all active users, the connection request can be rejected and needs to be repeated.
    \param  user name of requesting user
    \return TRUE: Connection was possible FALSE: Connection was not allowed due to the state of other active users
 */
bool Kymera_MicConnect(mic_users_t user);

/*! \brief Disconnects requested microphones from a user chain.
           The disconnection of AEC Reference is handled by this function.
           Using the registered callback functions Kymera_MicDisconnect requests the parameters for the disconnection from the user.
           The microphone(s) might be in use by a different user. The function takes care about disconnecting the
           microphones, AEC Reference or only the requesting user chain.
    \param user name of disconnecting user
 */
void Kymera_MicDisconnect(mic_users_t user);

/*! \brief Register possible microphone users at initialzation time.
    \param info->user: user that register
    \param info->callbacks: callback functions
    \param info->mandatory_mics: Mandatory mics are an option to register additional mics.
                 If a user connects its mics, all mandatory mics from all registered users are connected in addition.
                 This allows to register mics for a concurrency use case that might happen later.
                 When the concurrency use case happens, the sinks from the additional user can be connected to the splitter.
                 No change in mic configuration is necessary (which could lead to a reconnection).
    \param info->state: user is interruptible or non-interruptible
 */
void Kymera_MicRegisterUser(const mic_registry_per_user_t * const info);

/*! \brief If a user has updated its state it needs to inform mic interface about the changed state.
    \param user that has updated its state.
*/
void Kymera_MicUserUpdatedState(mic_users_t user);

/*! \brief Returns the active microphone users
    \param active microphone users
*/
mic_users_t Kymera_MicGetActiveUsers(void);

/*! \brief Facilitate transition to low power mode for MIC chain
    \param user name of requesting user
*/
void Kymera_MicSleep(mic_users_t user);

/*! \brief Facilitate transition to exit low power mode for MIC chain
    \param user name of requesting user
*/
void Kymera_MicWake(mic_users_t user);

#ifdef ENABLE_AEC_LEAKTHROUGH
/*! \brief Attach a microphone to a speaker via the internal leakthrough path of AEC Ref.
           If the mic isn't presently in use, the path will be connected when the mic becomes available.
    \param user name of requesting user
    \return TRUE = connection possible, FALSE = connection refused
 */
bool Kymera_MicAttachLeakthrough(mic_users_t user);

/*! \brief Detach a microphone from a speaker via the internal leakthrough path of AEC Ref.
    \param user name of requesting user
 */
void Kymera_MicDetachLeakthrough(mic_users_t user);
#endif    // ENABLE_AEC_LEAKTHROUGH

#ifdef HOSTED_TEST_ENVIRONMENT
/*! \brief Clear state: Clears registry and other entries */
void Kymera_MicClearState(void);

/*! \brief Access from the test environment: Read out sink connected to aec_ref */
Sink Kymera_MicGetAecSplitterConnection(uint8 stream_index);

/*! \brief Access from the test environment: Read out sink connected to microphone */
Sink Kymera_MicGetMicSplitterConnection(uint8 stream_index, uint8 channel);
#endif    //HOSTED_TEST_ENVIRONMENT

#endif // KYMERA_MIC_IF_H_
