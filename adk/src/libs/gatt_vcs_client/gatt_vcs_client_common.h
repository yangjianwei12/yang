/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_VCS_CLIENT_COMMON_H_
#define GATT_VCS_CLIENT_COMMON_H_

#include <gatt.h>

#include "gatt_vcs_client.h"

/***************************************************************************
NAME
    vcsClientSendVcsClientWriteCfm

DESCRIPTION
    Send a write confirmation message specified by the id parameter
*/
void vcsClientSendVcsClientWriteCfm(GVCSC *const vcs_client,
                                    const gatt_status_t status,
                                    GattVcsClientMessageId id);

/***************************************************************************
NAME
    vcsSendReadCccCfm

DESCRIPTION
    Send a read confirmation message specified by the id parameter
*/
void vcsSendReadCccCfm(GVCSC *vcs_client,
                       gatt_status_t status,
                       uint16 size_value,
                       const uint8 *value,
                       GattVcsClientMessageId id);
#endif