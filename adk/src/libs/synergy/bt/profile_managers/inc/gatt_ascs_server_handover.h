/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_ASCS_SERVER_HANDOVER_H_
#define GATT_ASCS_SERVER_HANDOVER_H_

#include "service_handle.h"
#include "csr_bt_profiles.h"
#include "gatt_ascs_server.h"

bool gattAscsServerHandoverMarshal(ServiceHandle serviceHandle,
                           ConnectionId cid,
                           uint8 *buf,
                           uint16 length,
                           uint16 *written);

bool gattAscsServerHandoverUnmarshal(ServiceHandle serviceHandle,
                             ConnectionId cid,
                             const uint8 *buf,
                             uint16 length,
                             uint16 *consumed);

void gattAscsServerHandoverCommit(ServiceHandle serviceHandle, ConnectionId cid, const bool newRole);

void gattAscsServerHandoverAbort(ServiceHandle serviceHandle);

void gattAscsServerHandoverComplete(ServiceHandle serviceHandle, const bool newRole );

bool gattAscsServerHandoverVeto(ServiceHandle serviceHandle);

bool gattAscsServerHasValidConnection(ServiceHandle serviceHandle, ConnectionId cid);
#endif /* GATT_ASCS_SERVER_HANDOVER_H_ */
