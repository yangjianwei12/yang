/*!
\copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   aghfp_profile AGHFP Profile
\ingroup    profiles
\brief      Application domain AGHFP component. Thi is the only public interface to AG to the application.

@{
*/

#ifndef AGHFP_PROFILE_H
#define AGHFP_PROFILE_H

#include "domain_message.h"
#include "aghfp_profile_typedef.h"
#include "aghfp_profile_abstraction.h"

/*! \brief Message IDs from HFG to main application task */
typedef enum
{
    APP_AGHFP_INIT_CFM = APP_AGHFP_MESSAGE_BASE,    /*!< Indicate HFP has been initialised */
    APP_AGHFP_CONNECTED_IND,                      /*!< SLC connected */
    APP_AGHFP_DISCONNECTED_IND,                   /*!< SLC disconnected */
    APP_AGHFP_SCO_CONNECTED_IND,                  /*!< Active SCO connected*/
    APP_AGHFP_SCO_DISCONNECTED_IND,               /*!< SCO channel disconnect */
    APP_AGHFP_SLC_STATUS_IND,                     /*!< SLC status updated */
    APP_AGHFP_AT_CMD_CFM,                         /*!< Result of an send AT command request */
    APP_AGHFP_AT_CMD_IND,                         /*!< AT command received not handled within HFP profile  */
    APP_AGHFP_SCO_INCOMING_RING_IND,              /*!< There is an incoming call (not connected) */
    APP_AGHFP_SCO_INCOMING_ENDED_IND,             /*!< Incoming call has gone away (unanswered) */
    APP_AGHFP_VOLUME_IND,                         /*!< New HFG volume level */
    APP_AGHFP_CONNECT_CFM,                        /*!< Connection confirmation */
    APP_AGHFP_DISCONNECT_CFM,                     /*!< Disconnect confirmation */
    APP_AGHFP_CALL_START_IND,                     /*!< Call has started indication */
    APP_AGHFP_CALL_END_IND,                       /*!< Call has ended indication */
} hfp_profile_messages;

typedef enum
{
    APP_AGHFP_CONNECT_FAILED,             /*!< Connect attempt failed */
    APP_AGHFP_DISCONNECT_LINKLOSS,        /*!< Disconnect due to link loss following supervision timeout */
    APP_AGHFP_DISCONNECT_NORMAL,          /*!< Disconnect initiated by local or remote device */
    APP_AGHFP_DISCONNECT_ERROR            /*!< Disconnect due to unknown reason */
} appAgHfpDisconnectReason;

/*! Message sent to status_notify_list clients indicating SLC state. */
typedef struct
{
    bool slc_connected;                 /*!< SLC Connected True/False */
    bdaddr bd_addr;                     /*!< Address of HF */
} APP_AGHFP_SLC_STATUS_IND_T;

/*! Message sent to status_notify_list clients indicating HFP profile has connected. */
typedef struct
{
    aghfpInstanceTaskData *instance;    /*!< The AGHFP instance this applies to */
    bdaddr bd_addr;                     /*!< Address of HF */
} APP_AGHFP_CONNECTED_IND_T;

/*! Message sent to status_notify_list clients indicating HFP profile has connected. */
typedef struct
{
    aghfpInstanceTaskData *instance;    /*!< The AGHFP instance this applies to */
    bdaddr bd_addr;                     /*!< Address of HF */
} APP_AGHFP_DISCONNECTED_IND_T;

/*! Message sent to status_notify_list clients indicating HFP profile has incoming a ringing incoming call. */
typedef struct
{
    bdaddr bd_addr;                     /*!< Address of HF */
} APP_AGHFP_SCO_INCOMING_RING_IND_T;


/*! \brief Initialise the HFP module. */
bool AghfpProfile_Init(Task init_task);

/*! \brief Is HFP connected for a particular HFP instance

    \param instance the HFP instance to query

    \return True if HFP is connected for the specified HFP instance
*/
bool AghfpProfile_IsConnectedForInstance(aghfpInstanceTaskData * instance);


bool AghfpProfile_IsCallActiveForInstance(aghfpInstanceTaskData * instance);


void AghfpProfile_HandleError(aghfpInstanceTaskData * instance, MessageId id, Message message);

/*! \brief Is HFP disconnected

    \param instance the HFP instance to query

    \return True if HFP is disconnected for specified HFP instance
*/
bool AghfpProfile_IsDisconnected(aghfpInstanceTaskData * instance);

/*! \brief Get application HFP instance task.

    \note currently this returns the voice_source_1 instance task.
*/
Task AghfpProfile_GetInstanceTask(aghfpInstanceTaskData * instance);

/*! \brief Start alerting the HF to an incoming call

  \param bd_addr Bluetooth address of the HF device

*/
aghfpInstanceTaskData * AghfpProfileInstance_GetInstanceForSource(voice_source_t source);

/*! \brief Connect to HFP HF device with a state address

    \param bd_addr Address for the remote HF device
*/
void AghfpProfile_Connect(const bdaddr *bd_addr);

/*! \brief Disconnect to HFP HF device with a state address

    \param bd_addr Address for the remote HF device
*/
void AghfpProfile_Disconnect(const bdaddr *bd_addr);

/*! \brief Indication of incoming call

    \param bd_addr Address for the remote HF device
*/
void AghfpProfile_CallIncomingInd(const bdaddr *bd_addr);

/*! \brief Hold the ongoing active call

    \param bd_addr Address for the remote HF device
    \return TRUE if successful
*/
bool AghfpProfile_HoldActiveCall(const bdaddr *bd_addr);

/*! \brief Release the ongoing held call

    \param bd_addr Address for the remote HF device
    \return TRUE if successful
*/
bool AghfpProfile_ReleaseHeldCall(const bdaddr *bd_addr);

/*! \brief Indication of outgoing call

    \param bd_addr Address for the remote HF device
*/
void AghfpProfile_CallOutgoingInd(const bdaddr *bd_addr);

/*! \brief Register with HFP to receive notifications of state changes.

    \param  task    The task being registered to receive notifications.
 */
void AghfpProfile_OutgoingCallAnswered(const bdaddr *bd_addr);

/*! \brief Turn inband ringing on

    \param bd_addr Address for the remote HF device

    \param enable True if inband ringing is enabled. False if disabled
*/
void AghfpProfile_EnableInBandRinging(const bdaddr *bd_addr, bool enable);

/*! \brief Set value to be sent for CLIP indication

    \param clip Telephone number and number type to be sent to the HF
*/
void AghfpProfile_SetClipInd(clip_data clip);

/*! \brief Clear value to be sent for CLIP indication
*/
void AghfpProfile_ClearClipInd(void);

/*! \brief Set the string to be sent for the network operator indication (AT+COPS)

    \param network_operator NULL terminated string for network operator name
*/
void AghfpProfile_SetNetworkOperatorInd(char *network_operator);

/*! \brief Clear the string to be sent for the network operator indication (AT+COPS)
*/
void AghfpProfile_ClearNetworkOperatorInd(void);

/*! \brief Send Battery charge indication to the Peer device

    \param The instance corresponding to the device and the level to send 
*/
void AghfpProfile_SendBatteryChgIndicator(aghfpInstanceTaskData * instance, uint16 level);


/*! \brief Register with HFP to receive notifications of SLC connect/disconnect.

    \param  task    The task being registered to receive notifications.
 */
void AghfpProfile_ClientRegister(Task task);

/*! \brief Register with HFP to receive notifications of state changes.

    \param  task    The task being registered to receive notifications.
 */
void AghfpProfile_RegisterStatusClient(Task task);

/*! \brief Is HFP SCO active for a particular AGHFP instance

    \param instance the HFP instance to query

    \return True if SCO is active for the specified AGHFP instance
*/
bool AghfpProfile_IsScoActiveForInstance(aghfpInstanceTaskData * instance);

/*! \brief Is SCO disconnected for a particular AGHFP instance

    \param instance the AGHFP instance to query

    \return True if SCO is disconnected for the specified AGHFP instance or if the instance is NULL
*/
bool AghfpProfile_IsScoDisconnectedForInstance(aghfpInstanceTaskData * instance);

/*! \brief Send service indicator the HFP HF device 

    \param instance Instance of the HFG device

    \param avail service availability
*/
void AghfpProfile_SendServiceIndicator(aghfpInstanceTaskData * instance, aghfp_service_availability avail);

/*! \brief Send Signal strength indication to the HFP HF device 

    \param instance Instance of the HFG device

    \param level signal strength

*/
void AghfpProfile_SendSignalStrengthIndicator(aghfpInstanceTaskData * instance, uint16 level);
/*! \brief Clear the call history of a HF device

    \param  bd_addr Address of HF
 */
void AghfpProfile_ClearCallHistory(void);

/*! \brief Update the last number dialled by the HF
*/
void AghfpProfile_SetLastDialledNumber(uint16 length, uint8* number);

const aghfp_audio_params* AghfpProfile_GetAudioParams(aghfpInstanceTaskData *instance);

/*! \brief To Enable or Disable automatic outgoing audio connection initiation during incoming call, for PTS testing

    \param instance the AGHFP instance to query

    \param enable TRUE to Enable FALSE to Disable
*/
void AghfpProfile_SetAutomaticOutgoingAudioConnection(aghfpInstanceTaskData * instance, bool enable);

/*! \brief To retain instance even after HF disconnection, for PTS testing

    \param instance the AGHFP instance to query

    \param enable TRUE to retain instance, otherwise FALSE
*/
void AghfpProfile_RetainInstanceAfterDisconnection(aghfpInstanceTaskData * instance, bool enable);

#endif // AGHFP_PROFILE_H


/*! @} !*/