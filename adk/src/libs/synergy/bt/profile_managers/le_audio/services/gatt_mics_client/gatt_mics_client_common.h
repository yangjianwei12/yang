/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#ifndef GATT_MICS_CLIENT_COMMON_H_
#define GATT_MICS_CLIENT_COMMON_H_

#include "gatt_mics_client.h"

/***************************************************************************
NAME
    micsClientSendMicsClientWriteCfm

DESCRIPTION
    Send a write confirmation message specified by the id parameter
*/
void micsClientSendMicsClientWriteCfm(GMICSC *const mics_client,
                                    const status_t status,
                                    GattMicsClientMessageId id);

/***************************************************************************
NAME
    micsSendReadCccCfm

DESCRIPTION
    Send a read confirmation message specified by the id parameter
*/
void micsSendReadCccCfm(GMICSC *mics_client,
                       status_t status,
                       uint16 size_value,
                       const uint8 *value,
                       GattMicsClientMessageId id);
#endif
