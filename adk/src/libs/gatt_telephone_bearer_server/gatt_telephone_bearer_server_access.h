/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_TELEPHONE_BEARER_SERVER_ACCESS_H_
#define GATT_TELEPHONE_BEARER_SERVER_ACCESS_H_

#include "gatt_telephone_bearer_server.h"
#include "gatt_telephone_bearer_server_common.h"
#include "gatt_telephone_bearer_server_private.h"

/**************************************************************************
NAME
    tbsFindCall

DESCRIPTION
    Find the index in the call list of the given caller id
*/
uint8 tbsFindCall(GTBS_T *telephone_bearer_server, uint8 cId);

/**************************************************************************
NAME
    tbsGetCallList

DESCRIPTION
    gets the call list
*/
uint8* tbsGetCallList(const GTBS_T *telephone_bearer_server, uint16* len);

/**************************************************************************
NAME
    tbsGetCallStateList

DESCRIPTION
    gets the call state list
*/
uint8* tbsGetCallStateList(const GTBS_T *telephone_bearer_server, uint16* len);

/**************************************************************************
NAME
    tbsAddCall

DESCRIPTION
    
*/
uint16 tbsAddCall(GTBS_T *telephone_bearer_server, GattTbsCallStates state, GattTbsCallFlags flags, uint16 callerIdLen, char* callerId);

/**************************************************************************
NAME
    tbsSetCallState

DESCRIPTION
    Sets an existing call to the specified state
*/
bool tbsSetCallState(GTBS_T *telephone_bearer_server, uint8 cId, GattTbsCallStates state);

/**************************************************************************
NAME
    tbsSetAllCallState

DESCRIPTION
    Sets all existing calls in oldState to newState.
    Returns TRUE if at least one call had its state changed.
*/
bool tbsSetAllCallState(GTBS_T *telephone_bearer_server,
                          const GattTbsCallStates oldState,
                          const GattTbsCallStates newState);

/**************************************************************************
NAME
    tbsDeleteCall

DESCRIPTION
    Deletes a call from the call array
*/
bool tbsDeleteCall(GTBS_T *telephone_bearer_server, uint8 cId);


/***************************************************************************
NAME
    tbsHandleAccessIndication

DESCRIPTION
    Handle the access indications that were sent
    to the Telephone Bearer Service library.
*/
void tbsHandleAccessIndication(
        GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        );

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifyCurrentCalls

DESCRIPTION
    Notify connected clients of changes to the Current Calls Characteristic
*/
void gattTelephoneBearerServerNotifyCurrentCalls(GTBS_T *telephone_bearer_server, uint8* clientIndex);

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifyCallState

DESCRIPTION
    Notify connected clients of changes to the Call State Characteristic
*/
void gattTelephoneBearerServerNotifyCallState(GTBS_T *telephone_bearer_server, uint8* clientIndex);

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifyIncomingCallTargetUri

DESCRIPTION
    Notify connected clients of changes to the Incoming Call Target URI Characteristic
*/
void gattTelephoneBearerServerNotifyIncomingCallTargetUri(GTBS_T *telephone_bearer_server, uint8* clientIndex);

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifyIncomingCall

DESCRIPTION
    Notify connected clients of changes to the Incoming Call Characteristic
*/
void gattTelephoneBearerServerNotifyIncomingCall(GTBS_T *telephone_bearer_server, uint8* clientIndex);

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifyCallFriendlyName

DESCRIPTION
    Notify connected clients of changes to the Call Friendly Name Characteristic
*/
void gattTelephoneBearerServerNotifyCallFriendlyName(GTBS_T *telephone_bearer_server, uint8* clientIndex);

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifySignalStrength

DESCRIPTION
    Notify connected clients of changes to the Signal Strength Characteristic
*/
void gattTelephoneBearerServerNotifySignalStrength(GTBS_T *telephone_bearer_server, uint8* clientIndex);

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifyCallControlPoint

DESCRIPTION
    Notify connected clients of changes to the Call Control Point Characteristic
*/
void gattTelephoneBearerServerNotifyCallControlPoint(GTBS_T *telephone_bearer_server, tbsCallControlPointNotification *ccpn, uint8* clientIndex);

/***************************************************************************
NAME
    gattTelephoneBearerServerNotifyCallChange

DESCRIPTION
    Notify connected clients of changes to the Call state and
    current call list characteristics
*/
void gattTelephoneBearerServerNotifyCallChange(GTBS_T *telephone_bearer_server);

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
                                                             tbsCallControlPointChracteristic *controlPoint,
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
