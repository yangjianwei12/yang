/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_TBS_CLIENT_WRITE_H_
#define GATT_TBS_CLIENT_WRITE_H_


#include "gatt_telephone_bearer_client_private.h"


/***************************************************************************
NAME
    writeClientConfigValue

DESCRIPTION
    Write Client Configuration descriptor value by handle.
*/
void writeClientConfigValue(GTBSC *tbs_client, bool notificationsEnable, uint16 handle);


/***************************************************************************
NAME
    handleTbsWriteCharacteristicValueResp

DESCRIPTION
    Handle response as a result of writing a characteristic value.
*/
void handleTbsWriteCharacteristicValueResp(GTBSC *tbs_client, const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *write_cfm);

/***************************************************************************
NAME
    handleTbsWriteWithoutResponseResp

DESCRIPTION
    Handle response as a result of writing a characteristic value without response.
*/
void handleTbsWriteWithoutResponseResp(GTBSC *tbs_client,
                                                const GATT_MANAGER_WRITE_WITHOUT_RESPONSE_CFM_T *write_cfm);

/***************************************************************************
NAME
    sendTbsClientWriteCfm
DESCRIPTION
    Send a write confirmation message specified by the id parameter
*/
void sendTbsClientWriteCfm(GTBSC *const tbs_client,
                           const GattTelephoneBearerClientStatus status,
                           GattTelephoneBearerClientMessageId id);

/***************************************************************************
NAME
    tbsWriteSignalStrengthIntervalRequest
DESCRIPTION
    Handle internal signal strength interval write request
*/
void tbsWriteSignalStrengthIntervalRequest(GTBSC *tbs_client, const uint8 interval, bool writeworesponse);

/***************************************************************************
NAME
    tbsWriteCallControlPointRequest
DESCRIPTION
    Handle internal Control Point write request
*/
void tbsWriteCallControlPointRequest(GTBSC *tbs_client,  const uint8 opcode, const uint8 size, const uint8* param);

#endif
