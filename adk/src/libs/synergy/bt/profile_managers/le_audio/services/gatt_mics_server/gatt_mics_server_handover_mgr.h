/* Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd. */
/* %%version */


#ifndef GATT_MICS_SERVER_HANDOVER_MGR_H_
#define GATT_MICS_SERVER_HANDOVER_MGR_H_

#include "gatt_mics_server_private.h"
#include "hydra_types.h"

bool micsServerHandoverMgrMarshal(MicsHandoverMgr* micsHandoverMgr,
                                 micsData micsData,
                                 uint8 indexConn,
                                 uint8 handoverStep,
                                 uint8 *buf,
                                 uint16 length,
                                 uint16 *written);

bool micsServerHandoverMgrUnmarshal(MicsHandoverMgr* micsHandoverMgr,
                                   uint8 handoverstep,
                                   connection_id_t cid,
                                   const uint8 *buf,
                                   uint16 length,
                                   uint16 *consumed);

void micsServerHandoverMgrCommit(GMICS_T* micControlServer, connection_id_t cid);

void micsServerHandoverMgrComplete(MicsHandoverMgr* micsHandoverMgr);

#endif /* GATT_MICS_SERVER_HANDOVER_MGR_H_ */
