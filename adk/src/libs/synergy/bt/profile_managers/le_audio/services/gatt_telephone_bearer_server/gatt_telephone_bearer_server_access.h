/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef GATT_TELEPHONE_BEARER_SERVER_ACCESS_H_
#define GATT_TELEPHONE_BEARER_SERVER_ACCESS_H_

#include "gatt_telephone_bearer_server.h"
#include "gatt_telephone_bearer_server_private.h"
#include "gatt_telephone_bearer_server_common.h"


void PanicNull(void* ptr);

/**************************************************************************
NAME
    tbsFindCall

DESCRIPTION
    Find the index in the call list of the given caller id
*/
uint8 tbsFindCall(GTBS_T *telephoneBearerServer, uint8 cId);

/**************************************************************************
NAME
    tbsGetCallList

DESCRIPTION
    gets the call list
*/
uint8* tbsGetCallList(const GTBS_T *telephoneBearerServer, uint16* len);

/**************************************************************************
NAME
    tbsGetCallStateList

DESCRIPTION
    gets the call state list
*/
uint8* tbsGetCallStateList(const GTBS_T *telephoneBearerServer, uint16* len);

/**************************************************************************
NAME
    tbsAddCall

DESCRIPTION
    
*/
uint8 tbsAddCall(GTBS_T *telephoneBearerServer, GattTbsCallStates state, GattTbsCallFlags flags, uint16 callerIdLen, char* callerId, bool joinAllowed);

/**************************************************************************
NAME
    tbsSetCallState

DESCRIPTION
    Sets an existing call to the specified state
*/
bool tbsSetCallState(GTBS_T *telephoneBearerServer, uint8 cId, GattTbsCallStates state);

/**************************************************************************
NAME
    tbsSetAllCallState

DESCRIPTION
    Sets all existing calls in oldState to newState.
    Returns TRUE if at least one call had its state changed.
*/
bool tbsSetAllCallState(GTBS_T *telephoneBearerServer,
                          const GattTbsCallStates oldState,
                          const GattTbsCallStates newState);

/**************************************************************************
NAME
    tbsDeleteCall

DESCRIPTION
    Deletes a call from the call array
*/
bool tbsDeleteCall(GTBS_T *telephoneBearerServer, uint8 cId);


/***************************************************************************
NAME
    tbsHandleAccessIndication

DESCRIPTION
    Handle the access indications that were sent
    to the Telephone Bearer Service library.
*/
void tbsHandleAccessIndication(
        GTBS_T *telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        );

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifyCurrentCalls

DESCRIPTION
    Notify connected clients of changes to the Current Calls Characteristic
*/
void gattTelephoneBearerServerNotifyCurrentCalls(GTBS_T *telephoneBearerServer, uint8* clientIndex);

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifyCallState

DESCRIPTION
    Notify connected clients of changes to the Call State Characteristic
*/
void gattTelephoneBearerServerNotifyCallState(GTBS_T *telephoneBearerServer, uint8* clientIndex);

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifyIncomingCallTargetUri

DESCRIPTION
    Notify connected clients of changes to the Incoming Call Target URI Characteristic
*/
void gattTelephoneBearerServerNotifyIncomingCallTargetUri(GTBS_T *telephoneBearerServer, uint8* clientIndex);

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifyIncomingCall

DESCRIPTION
    Notify connected clients of changes to the Incoming Call Characteristic
*/
void gattTelephoneBearerServerNotifyIncomingCall(GTBS_T *telephoneBearerServer, uint8* clientIndex);

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifyCallFriendlyName

DESCRIPTION
    Notify connected clients of changes to the Call Friendly Name Characteristic
*/
void gattTelephoneBearerServerNotifyCallFriendlyName(GTBS_T *telephoneBearerServer, uint8* clientIndex);

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifySignalStrength

DESCRIPTION
    Notify connected clients of changes to the Signal Strength Characteristic
*/
void gattTelephoneBearerServerNotifySignalStrength(GTBS_T *telephoneBearerServer, uint8* clientIndex);

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifyCallControlPoint

DESCRIPTION
    Notify connected clients of changes to the Call Control Point Characteristic
*/
void gattTelephoneBearerServerNotifyCallControlPoint(GTBS_T *telephoneBearerServer, TbsCallControlPointNotification *ccpn, uint8* clientIndex);

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifyBearerUriPrefixList

DESCRIPTION
    Notify connected clients of changes to the Bearer Uri List Characteristic
*/
void gattTelephoneBearerServerNotifyBearerUriPrefixList(GTBS_T *telephoneBearerServer, uint8* clientIndex);

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifyCallChange

DESCRIPTION
    Notify connected clients of changes to the Call state and
    current call list characteristics
*/
void gattTelephoneBearerServerNotifyCallChange(GTBS_T *telephoneBearerServer);

/***************************************************************************
NAME
    tbsPrevalidateCcpOpcode

DESCRIPTION
    Used to prevalidate a Call Control Point opcode before it is sent to the
    application for processing.
    Checks that the opcode is supported and all call IDs are valid.
    Sets opcode and callId to the values taken from the Call Control Point access

    Returns the outcome of the check, TBS_CCP_RESULT_SUCCESS if valid.

*/
GattTbsCcpNotificationResultCodes tbsPrevalidateCcpOpcode(const ServiceHandle srvcHndl,
                                                             TbsCallControlPointChracteristic *controlPoint,
                                                             uint16 size,
                                                             uint8* opcode,
                                                             uint8* callId);

/***************************************************************************
NAME
    validateCcpOpcodeCallId

DESCRIPTION
    Validate the opcode for the given call id, returns new state following
    opcode or TBS_CALL_STATE_INVALID if invalid transition assumed that the
    callIndex is valid
*/
GattTbsCallStates validateCcpOpcodeCallId(const ServiceHandle srvcHndl, uint8 callId, uint16 oc);



#endif /* GATT_TELEPHONE_BEARER_SERVER_ACCESS_H_ */
