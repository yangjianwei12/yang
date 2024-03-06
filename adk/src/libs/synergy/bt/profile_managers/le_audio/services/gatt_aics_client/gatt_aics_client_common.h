/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_AICS_CLIENT_COMMON_H_
#define GATT_AICS_CLIENT_COMMON_H_


#include "gatt_aics_client.h"
#include "gatt_aics_client_private.h"

/***************************************************************************
NAME
    aicsClientSendAicsClientWriteCfm

DESCRIPTION
    Send a write confirmation message specified by the id parameter
*/
void aicsClientSendAicsClientWriteCfm(GAICS *const aics_client,
                                      const status_t status,
                                      GattAicsClientMessageId id);

/***************************************************************************
NAME
    aicsSendReadCccCfm

DESCRIPTION
    Send a read ccc confirmation message specified by the id parameter
*/
void aicsSendReadCccCfm(GAICS *vcs_client,
                        status_t status,
                        uint16 size_value,
                        const uint8 *value,
                        GattAicsClientMessageId id);

#endif
