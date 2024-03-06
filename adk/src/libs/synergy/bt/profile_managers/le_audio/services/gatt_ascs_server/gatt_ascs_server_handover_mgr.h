/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */


#ifndef GATT_ASCS_SERVER_HANDOVER_MGR_H_
#define GATT_ASCS_SERVER_HANDOVER_MGR_H_

/*#include "hydra_macros.h"*/
/*#include "csr_bt_bluestack_types.h"*/
#include "gatt_ascs_server_private.h"
#include "hydra_types.h"
#include <marshal.h>

typedef struct
{
    uint16 clientCfg; /* ClientCfg is a uint16 */
    uint8 numAses;
    GattAscsServerAse* ase[GATT_ASCS_NUM_ASES_MAX];
} AscsConnectionInfo;

typedef struct AscsHandoverMgr
{
    bool                marshallerInitialised;
    marshaller_t        marshaller;
    bool                unMarshallerInitialised;
    unmarshaller_t      unMarshaller;
    uint8               numHandoverConnections;
    AscsConnectionInfo* handoverConnectionInfo;
    struct GattAscsConnection* handoverConnection[GATT_ASCS_NUM_CONNECTIONS_MAX];
} AscsHandoverMgr;

#endif /* GATT_ASCS_SERVER_HANDOVER_MGR_H_ */
