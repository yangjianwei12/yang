/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */


#ifndef GATT_BASS_SERVER_HANDOVER_MGR_H_
#define GATT_BASS_SERVER_HANDOVER_MGR_H_

#include "gatt_bass_server_private.h"
#include "hydra_types.h"
#include <marshal.h>

bool bassServerHandoverMgrMarshal(BassHandoverMgr* bassHandoverMgr,
                                  gatt_bass_server_data_t data,
                                  uint8 indexConn,
                                  uint8 handoverStep,
                                  uint8 *buf,
                                  uint16 length,
                                  uint16 *written);

bool bassServerHandoverMgrUnmarshal(BassHandoverMgr* bassHandoverMgr,
                                    uint8 handoverStep,
                                    connection_id_t cid,
                                    const uint8 *buf,
                                    uint16 length,
                                    uint16 *consumed);

void bassServerHandoverMgrCommit(GBASSSS* bassServer, connection_id_t cid);

void bassServerHandoverMgrComplete(BassHandoverMgr* bassHandoverMgr);

#endif /* GATT_BASS_SERVER_HANDOVER_MGR_H_ */
