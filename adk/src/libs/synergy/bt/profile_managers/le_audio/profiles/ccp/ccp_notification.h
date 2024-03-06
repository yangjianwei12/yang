/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CCP_NOTIFICATION_H_
#define CCP_NOTIFICATION_H_

#include <gatt_telephone_bearer_client.h>

#include "ccp_private.h"

/***************************************************************************
NAME
    ccpSendTbsSetNtfCfm

DESCRIPTION
    Send specified notification message
*/
void ccpSendTbsSetNtfCfm(CCP *ccp_inst,
                                status_t status,
                                CcpMessageId id);

#endif
