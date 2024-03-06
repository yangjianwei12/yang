/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CCP_READ_H_
#define CCP_READ_H_

#include <gatt_telephone_bearer_client.h>
#include "ccp_private.h"

/***************************************************************************
NAME
    ccpHandleTbsReadProviderNameCfm

DESCRIPTION
    Handles the GattTelephoneBearerClientReadProviderNameCfm message.
*/
void ccpHandleTbsReadProviderNameCfm(CCP *ccp_inst,
                                    const GattTelephoneBearerClientReadProviderNameCfm *msg);

/***************************************************************************
NAME
    ccpHandleTbsReadBearerUciCfm

DESCRIPTION
    Handles the GattTelephoneBearerClientReadBearerUciCfm message.
*/
void ccpHandleTbsReadBearerUciCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadBearerUciCfm *msg);

/***************************************************************************
NAME
    ccpHandleTbsReadBearerTechnologyCfm

DESCRIPTION
    Handles the GattTelephoneBearerClientReadBearerTechnologyCfm message.
*/
void ccpHandleTbsReadBearerTechnologyCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadBearerTechnologyCfm *msg);

/***************************************************************************
NAME
    ccpHandleTbsReadBearerUriSchemesCfm

DESCRIPTION
    Handles the GattTelephoneBearerClientReadBearerUriSchemesSupportedListCfm message.
*/
void ccpHandleTbsReadBearerUriSchemesCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadBearerUriSchemesSupportedListCfm *msg);

/***************************************************************************
NAME
    ccpHandleTbsReadSignalStrengthCfm

DESCRIPTION
    Handles the GattTelephoneBearerClientReadSignalStrengthCfm message.
*/
void ccpHandleTbsReadSignalStrengthCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadSignalStrengthCfm *msg);

/***************************************************************************
NAME
    ccpHandleTbsReadSignalStrengthIntervalCfm

DESCRIPTION
    Handles the GattTelephoneBearerClientReadSignalStrengthIntervalCfm message.
*/
void ccpHandleTbsReadSignalStrengthIntervalCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadSignalStrengthIntervalCfm *msg);

/***************************************************************************
NAME
    ccpHandleTbsReadCurrentCallsListCfm

DESCRIPTION
    Handles the GattTelephoneBearerClientReadCurrentCallsListCfm message.
*/
void ccpHandleTbsReadCurrentCallsListCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadCurrentCallsListCfm *msg);

/***************************************************************************
NAME
    ccpHandleTbsReadContentControlIdCfm

DESCRIPTION
    Handles the GattTelephoneBearerClientReadContentControlIdCfm message.
*/
void ccpHandleTbsReadContentControlIdCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadContentControlIdCfm *msg);

/***************************************************************************
NAME
    ccpHandleTbsReadFlagsCfm

DESCRIPTION
    Handles the GattTelephoneBearerClientReadFeatureAndStatusFlagsCfm message.
*/
void ccpHandleTbsReadFlagsCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadFeatureAndStatusFlagsCfm *msg);

/***************************************************************************
NAME
    ccpHandleTbsReadIncomingCallTargetBearerUriCfm

DESCRIPTION
    Handles the GattTelephoneBearerClientReadIncomingCallTargetBearerUriCfm message.
*/
void ccpHandleTbsReadIncomingCallTargetBearerUriCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadIncomingCallTargetBearerUriCfm *msg);


/***************************************************************************
NAME
    ccpHandleTbsReadCallStateCfm

DESCRIPTION
    Handles the GattTelephoneBearerClientMsgReadCallStateCfm message.
*/
void ccpHandleTbsReadCallStateCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientMsgReadCallStateCfm *msg);

/***************************************************************************
NAME
    ccpHandleTbsReadIncomingCallCfm

DESCRIPTION
    Handles the GattTelephoneBearerClientMsgReadIncomingCallCfm message.
*/
void ccpHandleTbsReadIncomingCallCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientMsgReadIncomingCallCfm *msg);

/***************************************************************************
NAME
    ccpHandleTbsReadCallFriendlynameCfm

DESCRIPTION
    Handles the GattTelephoneBearerClientMsgReadCallFriendlyNameCfm message.
*/
void ccpHandleTbsReadCallFriendlyNameCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientMsgReadCallFriendlyNameCfm *msg);

/***************************************************************************
NAME
    ccpHandleTbsReadOptionalOpcodesCfm

DESCRIPTION
    Handles the GattTelephoneBearerClientReadOptionalOpcodesCfm message.
*/
void ccpHandleTbsReadOptionalOpcodesCfm(CCP *ccp_inst,
                                  const GattTelephoneBearerClientReadOptionalOpcodesCfm *msg);

#endif
