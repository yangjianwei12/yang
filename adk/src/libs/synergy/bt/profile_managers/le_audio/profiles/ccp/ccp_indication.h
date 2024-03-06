/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CCP_INDICATION_H_
#define CCP_INDICATION_H_

#include <gatt_telephone_bearer_client.h>

#include "ccp_private.h"


/***************************************************************************
NAME
    ccpHandleTbsProviderNameInd

DESCRIPTION
    Handle a GattTelephoneBearerClientProviderNameInd message.
*/
void ccpHandleTbsProviderNameInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientProviderNameInd *ind);


/***************************************************************************
NAME
    ccpHandleTbsBearerTechnologyInd

DESCRIPTION
    Handle a GattTelephoneBearerClientBearerTechnologyInd message.
*/
void ccpHandleTbsBearerTechnologyInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientBearerTechnologyInd *ind);

/***************************************************************************
NAME
    ccpHandleTbsSignalStrengthInd

DESCRIPTION
    Handle a GattTelephoneBearerClientSignalStrengthInd message.
*/
void ccpHandleTbsSignalStrengthInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientSignalStrengthInd *ind);

/***************************************************************************
NAME
    ccpHandleTbsCurrentCallsInd

DESCRIPTION
    Handle a GattTelephoneBearerClientCurrentCallsInd message.
*/
void ccpHandleTbsCurrentCallsInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientCurrentCallsInd *ind);

/***************************************************************************
NAME
    ccpHandleTbsFlagsInd

DESCRIPTION
    Handle a GattTelephoneBearerClientFlagsInd message.
*/
void ccpHandleTbsFlagsInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientFlagsInd *ind);

/***************************************************************************
NAME
    ccpHandleTbsIncomingCallTargetBearerUriInd

DESCRIPTION
    Handle a GattTelephoneBearerClientIncomingCallTargetBearerUriInd message.
*/
void ccpHandleTbsIncomingCallTargetBearerUriInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientIncomingCallTargetBearerUriInd *ind);

/***************************************************************************
NAME
    ccpHandleTbsCallStateInd

DESCRIPTION
    Handle a GattTelephoneBearerClientCallStateInd message.
*/
void ccpHandleTbsCallStateInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientCallStateInd *ind);

/***************************************************************************
NAME
    ccpHandleTbsCallControlPointInd

DESCRIPTION
    Handle a GattTelephoneBearerClientCallControlPointInd message.
*/
void ccpHandleTbsCallControlPointInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientCallControlPointInd *ind);

/***************************************************************************
NAME
    ccpHandleTbsTerminationReasonInd

DESCRIPTION
    Handle a GattTelephoneBearerClientTerminationReasonInd message.
*/
void ccpHandleTbsTerminationReasonInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientTerminationReasonInd *ind);

/***************************************************************************
NAME
    ccpHandleTbsIncomingCallInd

DESCRIPTION
    Handle a GattTelephoneBearerClientIncomingCallInd message.
*/
void ccpHandleTbsIncomingCallInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientIncomingCallInd *ind);

/***************************************************************************
NAME
    ccpHandleTbsCallFriendlyNameInd

DESCRIPTION
    Handle a GattTelephoneBearerClientCallFriendlyNameInd message.
*/
void ccpHandleTbsCallFriendlyNameInd(CCP *ccp_inst,
                               const GattTelephoneBearerClientCallFriendlyNameInd *ind);
#endif
