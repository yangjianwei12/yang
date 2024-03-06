/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_VCS_SERVER_HANDOVER_H_
#define GATT_VCS_SERVER_HANDOVER_H_

#include "service_handle.h"
#include "csr_bt_profiles.h"
#include "gatt_vcs_server.h"

bool gattVcsServerHandoverMarshal(ServiceHandle serviceHandle,
                                  connection_id_t cid,
                                  uint8 *buf,
                                  uint16 length,
                                  uint16 *written);

bool gattVcsServerHandoverUnmarshal(ServiceHandle serviceHandle,
                                    connection_id_t cid,
                                    const uint8 *buf,
                                    uint16 length,
                                    uint16 *consumed);

void gattVcsServerHandoverCommit(ServiceHandle serviceHandle,
                                 connection_id_t cid,
                                 const bool newRole);

void gattVcsServerHandoverAbort(ServiceHandle serviceHandle);

void gattVcsServerHandoverComplete(ServiceHandle serviceHandle, const bool newRole );

bool gattVcsServerHandoverVeto(ServiceHandle serviceHandle);

#endif /* GATT_VCS_SERVER_HANDOVER_H_ */
