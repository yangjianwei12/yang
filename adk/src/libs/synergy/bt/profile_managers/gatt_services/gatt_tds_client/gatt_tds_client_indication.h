/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_TDS_CLIENT_NOTIFICATION_H_
#define GATT_TDS_CLIENT_NOTIFICATION_H_

#include "gatt_tds_client_private.h"

/* TDS Inidcation value */
#define TDS_INDICATION_VALUE   (0x02)

/***************************************************************************
NAME
    tdsClientHandleInternalRegisterForNotification

DESCRIPTION
    Handle a TDS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ message.
*/
void tdsClientHandleInternalRegisterForIndication(GTDSC *gatt_tds_client,
                                     uint32 indic_value);

void handleTdsClientIndication(GTDSC *tdsClient, uint16 handle, uint16 valueLength, uint8 *value);

void tdsClientIndicationCfm(GTDSC *const tdsClient,
                       GattTdsClientStatus status,
                       GattTdsClientMessageId id);

#endif
