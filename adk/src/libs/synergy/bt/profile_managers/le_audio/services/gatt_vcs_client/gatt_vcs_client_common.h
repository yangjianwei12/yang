/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_VCS_CLIENT_COMMON_H_
#define GATT_VCS_CLIENT_COMMON_H_

#include "gatt_vcs_client.h"

/***************************************************************************
NAME
    vcsClientSendVcsClientWriteCfm

DESCRIPTION
    Send a write confirmation message specified by the id parameter
*/
void vcsClientSendVcsClientWriteCfm(GVCSC *const vcs_client,
                                    const status_t status,
                                    GattVcsClientMessageId id);

/***************************************************************************
NAME
    vcsSendReadCccCfm

DESCRIPTION
    Send a read confirmation message specified by the id parameter
*/
void vcsSendReadCccCfm(GVCSC *vcs_client,
                       status_t status,
                       uint16 size_value,
                       const uint8 *value,
                       GattVcsClientMessageId id);
#endif
