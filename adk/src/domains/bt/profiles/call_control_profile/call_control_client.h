/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   call_control_client Call Control Client
    @{
    \ingroup    profiles
    \brief      Header file for Call control profile client
*/

#ifndef CALL_CONTROL_CLIENT_H_
#define CALL_CONTROL_CLIENT_H_

#ifdef USE_SYNERGY
#include "bt_types.h"
#include "ccp.h"
#include "voice_sources.h"
#include "device.h"

typedef enum
{
    CCP_NOTIFICATION_ID_PROVIDER_NAME,
    CCP_NOTIFICATION_ID_TECHNOLOGY,
    CCP_NOTIFICATION_ID_SIGNAL_STRENGTH,
    CCP_NOTIFICATION_ID_LIST_CURRENT_CALL,
    CCP_NOTIFICATION_ID_FLAGS,
    CCP_NOTIFICATION_ID_INCOMING_CALL_TARGET_BEARER_URI,
    CCP_NOTIFICATION_ID_CALL_STATE,
    CCP_NOTIFICATION_ID_CALL_CONTROL_POINT,
    CCP_NOTIFICATION_ID_TERMINATION_REASON,
    CCP_NOTIFICATION_ID_INCOMING_CALL,
    CCP_NOTIFICATION_ID_CALL_FRIENDLY_NAME,
} ccp_notification_id_t;

typedef enum
{
    CCP_READ_PROVIDER_NAME,
    CCP_READ_BEARER_UCI,
    CCP_READ_BEARER_TECHNOLOGY,
    CCP_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST,
    CCP_READ_SIGNAL_STRENGTH,
    CCP_READ_SIGNAL_STRENGTH_INTERVAL,
    CCP_READ_CURRENT_CALLS_LIST,
    CCP_READ_CONTENT_CONTROL_ID,
    CCP_READ_FEATURE_AND_STATUS_FLAGS,
    CCP_READ_INCOMING_CALL_TARGET_BEARER_URI,
    CCP_READ_CALL_STATE,
    CCP_READ_INCOMING_CALL,
    CCP_READ_CALL_FRIENDLY_NAME,
    CCP_READ_CCP_OPTIONAL_OPCODES,
} ccp_read_characteristics_id_t;

/*! Application CCP call state*/
typedef enum
{
    CCP_CALL_STATE_IDLE,
    CCP_CALL_STATE_INCOMING,
    CCP_CALL_STATE_OUTGOING_DIALING,
    CCP_CALL_STATE_OUTGOING_ALERTING,
    CCP_CALL_STATE_ACTIVE,
    CCP_CALL_STATE_LOCALLY_HELD,
    CCP_CALL_STATE_REMOTELY_HELD,
    CCP_CALL_STATE_LOCALLY_REMOTELY_HELD,
} ccp_call_state_t;

typedef struct
{
    /*! Call state at remote server */
    ccp_call_state_t state;

    /*! Index of call */
    uint8        tbs_call_id;
} ccp_call_info_t;

/*! CCP Call Termination type */
typedef enum
{
    CCP_TERMINATE_ALL_CALLS,
    CCP_HANGUP_ONGOING_CALLS,
    CCP_REJECT_INCOMING_CALL,
    CCP_TERMINATE_HELD_CALLS
} call_terminate_type_t;

/*! \brief Initialise the Call control client component
 */
void CallControlClient_Init(void);

/*! \brief Function to send the GTBS Opcode to the remote device.
    \param[in] cid    GATT Connection id to which the call control opcode to send.
    \param[in] op     GTBS client control opcode to be sent to remote GTBS server.
    \param[in] val    value to be associated with the opcode(not all opcodes will have value).
 */
void CallControlClient_SendCallControlOpcode(gatt_cid_t cid, uint8 op, int32 val);

/*! \brief Terminates the call for the given source
    \param[in] source Voice source
    \param[in] terminate_type \ref call_terminate_type_t
*/
void CallControlClient_TerminateCalls(voice_source_t source, call_terminate_type_t terminate_type);

/*! \brief Function returns voice context based on call state.
    \param[in] source Voice source

    \return Returns \ref voice_source_provider_context_t.
*/
unsigned CallClientControl_GetContext(voice_source_t source);

/*! \brief Function checks if call related activity is idle or not.
    \param[in] source Voice source

    \return Returns TRUE if call is idle.
*/
bool CallClientControl_IsCallContextIdle(voice_source_t source);

/*! \brief Set the call characteristics notifications
    \param[in] cid    GATT Connection id to which the call characteristics to be set.
    \param[in] type   The type of the notification as ccp_notification_id_t
    \param[in] notification_enable - True/False to enable/disable the notifications.

    \return The le voice context
 */
void CallControlClient_SetCallCharacteristicsNotification(gatt_cid_t cid, ccp_notification_id_t type, bool notification_enable);

/*! \brief Read the CCP call characteristics
    \param[in] cid    GATT Connection id to which the call characteristics to read.
    \param[in] type   The type as \ref ccp_read_characteristics_id_t
 */
void CallControlClient_ReadCallCharacteristics(gatt_cid_t cid, ccp_read_characteristics_id_t id);

/*! \brief Send unconditional call control point opcode to the remote 
    \param[in] cid    GATT Connection id to which the call characteristics to read.
    \param[in] Opcode   The Opcode to send
    \param[in] callid1   The call Id for which the opcode is to be sent.
    \param[in] callid2   The second call Id. This is required only when the opcode is to join the calls, otherwise can be zero.
 */
void CallControlClient_SendCallControlPointOpcode(gatt_cid_t cid, uint8 Opcode, uint8 callid1, uint8 callid2);

/*! \brief Prints all calls info 
 */
void CallControlClient_PrintAllCallsInfo(void);

/*! \brief Release any call which is held/waiting
    \param[in] source  voice source upon which the twc action is to be taken
 */
void CallControlClient_TwcReleaseHeldRejectWaiting(voice_source_t source);

/*! \brief Release the active call and accept incoming call
    \param[in] source  voice source upon which the twc action is to be taken
 */
void CallControlClient_TwcReleaseActiveAcceptOther(voice_source_t source);

/*! \brief Hold the active call and accept incoming call
    \param[in] source  voice source upon which the twc action is to be taken
 */
void CallControlClient_TwcHoldActiveAcceptOther(voice_source_t source);

/*! \brief Join all the calls
    \param[in] source  voice source upon which the twc action is to be taken
 */
void CallControlClient_TwcJoinCalls(voice_source_t source);

/*! \brief Check if CCP is connected or not

    \return TRUE if CCP connected else FALSE
 */
bool CallClientControl_IsCcpConnected(void);

/*! \brief Check if the Call control service is available in remote server and is discovered */
void CallControlClient_ConnectProfile(gatt_cid_t cid);

/*! \brief Get the content control id for call control client
    \param[in] cid    GATT Connection id to which the content control id is to read.

    \return Content control id of the call control client
 */
uint16 CallClientControl_GetContentControlId(gatt_cid_t cid);

/*! \brief Refreshes the Call state list for CCP 
    \param[in] source Voice source
*/
void CallControlClient_RefreshCallState(voice_source_t source);
/*! \brief Register the Call control client component with the Device Database Serialiser */
void CallControlClient_RegisterAsPersistentDeviceDataUser(void);


/*! \brief Get the device from voice source
    \param[in] source Voice source

    \return device_t device associated for the voice source
*/
device_t CallControlClient_FindDeviceFromVoiceSource(voice_source_t source);

/*! \brief Initiate a call through CCP using number
    \param[in] source  voice source to which the call needs to be originated.
    \param[in] digits  The number at which the call is to be place. If the number is NULL, Fixed number is used(123).
    \param[in] number_of_digits  The number of digits passed in the number to which the call is to be initiated
*/
void CallControlClient_InitiateCallUsingNumber(voice_source_t source, uint8 *digits, unsigned number_of_digits);

#else /* USE_SYNERGY */

#define CallControlClient_Init()
#define CallClientControl_GetContext(source) (context_voice_disconnected)
#define CallControlClient_SendCallControlOpcode(cid, op, val)
#define CallControlClient_SetCallCharacteristicsNotification(cid, type)
#define CallControlClient_PrintAllCallsInfo(cid)(FALSE)
#define CallControlClient_RegisterAsPersistentDeviceDataUser()

#endif /* USE_SYNERGY */

#endif /* CALL_CONTROL_CLIENT_H */

/*! @} */