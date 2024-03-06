/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_VOCS_CLIENT_COMMON_H_
#define GATT_VOCS_CLIENT_COMMON_H_


#include "gatt_vocs_client.h"
#include "gatt_vocs_client_private.h"

/***************************************************************************
NAME
    vcsClientSendVocsClientWriteCfm

DESCRIPTION
    Send a write confirmation message specified by the id parameter
*/
void vocsClientSendVocsClientWriteCfm(GVOCS *const vocs_client,
                                      const status_t status,
                                      GattVocsClientMessageId id);

/***************************************************************************
NAME
    vocsSendReadCccCfm

DESCRIPTION
    Send a read confirmation message specified by the id parameter.
*/
void vocsSendReadCccCfm(GVOCS *vcs_client,
                        status_t status,
                        uint16 size_value,
                        const uint8 *value,
                        GattVocsClientMessageId id);

#endif
