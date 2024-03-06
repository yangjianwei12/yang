/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_MICS_SERVER_HANDOVER_H_
#define GATT_MICS_SERVER_HANDOVER_H_

#include "service_handle.h"
#include "csr_bt_profiles.h"
#include "gatt_mics_server.h"

bool gattMicsServerHandoverMarshal(ServiceHandle serviceHandle,
                                  connection_id_t cid,
                                  uint8 *buf,
                                  uint16 length,
                                  uint16 *written);

bool gattMicsServerHandoverUnmarshal(ServiceHandle serviceHandle,
                                    connection_id_t cid,
                                    const uint8 *buf,
                                    uint16 length,
                                    uint16 *consumed);

void gattMicsServerHandoverCommit(ServiceHandle serviceHandle,
                                 connection_id_t cid,
                                 const bool newRole);

void gattMicsServerHandoverAbort(ServiceHandle serviceHandle);

void gattMicsServerHandoverComplete(ServiceHandle serviceHandle, const bool newRole );

bool gattMicsServerHandoverVeto(ServiceHandle serviceHandle);

#endif /* GATT_MICS_SERVER_HANDOVER_H_ */
