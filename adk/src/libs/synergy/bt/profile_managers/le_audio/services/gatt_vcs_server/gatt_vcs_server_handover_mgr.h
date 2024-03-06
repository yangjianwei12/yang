/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */


#ifndef GATT_VCS_SERVER_HANDOVER_MGR_H_
#define GATT_VCS_SERVER_HANDOVER_MGR_H_

#include "gatt_vcs_server_private.h"
#include "hydra_types.h"

bool vcsServerHandoverMgrMarshal(VcsHandoverMgr* vcsHandoverMgr,
                                 gatt_vcs_data vcsData,
                                 uint8 indexConn,
                                 uint8 handoverStep,
                                 uint8 *buf,
                                 uint16 length,
                                 uint16 *written);

bool vcsServerHandoverMgrUnmarshal(VcsHandoverMgr* vcsHandoverMgr,
                                   uint8 handoverstep,
								   connection_id_t cid,
                                   const uint8 *buf,
                                   uint16 length,
                                   uint16 *consumed);

void vcsServerHandoverMgrCommit(GVCS* volumeControlServer, connection_id_t cid);

void vcsServerHandoverMgrComplete(VcsHandoverMgr* vcsHandoverMgr);

#endif /* GATT_VCS_SERVER_HANDOVER_MGR_H_ */
