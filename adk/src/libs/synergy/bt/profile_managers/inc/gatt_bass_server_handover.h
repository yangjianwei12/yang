/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_BASS_SERVER_HANDOVER_H_
#define GATT_BASS_SERVER_HANDOVER_H_

#include "service_handle.h"
#include "csr_bt_profiles.h"
#include "gatt_bass_server.h"

bool gattBassServerHandoverMarshal(ServiceHandle serviceHandle,
                                   connection_id_t cid,
                                   uint8 *buf,
                                   uint16 length,
                                   uint16 *written);

bool gattBassServerHandoverUnmarshal(ServiceHandle serviceHandle,
                                     connection_id_t cid,
                                     const uint8 *buf,
                                     uint16 length,
                                     uint16 *consumed);

void gattBassServerHandoverCommit(ServiceHandle serviceHandle, connection_id_t cid, const bool newRole);

void gattBassServerHandoverAbort(ServiceHandle serviceHandle);

void gattBassServerHandoverComplete(ServiceHandle serviceHandle, const bool newRole );

bool gattBassServerHandoverVeto(ServiceHandle serviceHandle);

bool gattBassServerHasValidConnection(ServiceHandle serviceHandle, connection_id_t cid);

#endif /* GATT_BASS_SERVER_HANDOVER_H_ */
