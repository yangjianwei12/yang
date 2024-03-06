/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */
#define TRAPSET_MARSHAL 1
#include "gatt_bass_server_private.h"
#include "gatt_bass_server_handover_mgr.h"
#include "gatt_bass_server_debug.h"
#include "gatt_bass_server_common.h"
#include "csr_bt_core_stack_pmalloc.h"

#include <stdlib.h>
#include <panic.h>
#include <marshal.h>

typedef void* Sink;
typedef uint16 Task;
#include "../../../../../../marshal_common_desc/marshal_common_desc.h"

/**************************** Start of Desc code ******************************/
#define BASS_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(gattBassServerFirstHandoverData) \
    ENTRY(gattBassBroadcastHandoverSourceInfo) \
    ENTRY(CsrBtTypedAddr) \
    ENTRY(GattBassServerPaSyncState) \
    ENTRY(GattBassServerBroadcastBigEncryption) \
    ENTRY(BD_ADDR_T) \
    ENTRY(GattBassServerSubGroupsData) \
    ENTRY(gatt_bass_server_ccc_data_t) \
    ENTRY(connection_id_t) \
    ENTRY(GattBassServerConfig) \
    ENTRY(uint24) \
    ENTRY(GattBassServerSubGroupsData_dyn_arr_t)

/********************************************************************
 * Enum of 'type' identifiers used for marshalling, of the form MARSHAL_TYPE_xyz
 ********************************************************************/

/* Use xmacro to expand type table as enumeration of marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
      COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
      COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
      BASS_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
      BASS_MARSHAL_OBJ_TYPE_COUNT
};
#undef EXPAND_AS_ENUMERATION

/********************************************************************
 * Marshal Descriptors for connection_id_t
 ********************************************************************/
static const marshal_type_descriptor_t mtd_connection_id_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(connection_id_t);

/********************************************************************
 * Marshal Descriptors for GattBassServerConfig
 ********************************************************************/
static const marshal_member_descriptor_t mmd_GattBassServerConfig[] =
{
    MAKE_MARSHAL_MEMBER(GattBassServerConfig, uint8,  receiveStateCccSize),
    MAKE_MARSHAL_MEMBER_POINTER(GattBassServerConfig, uint16, receiveStateCcc),
};

static uint32 getCccSize(const void *parent,
                             const marshal_member_descriptor_t *memberDescriptor,
                             uint32 arrayElement)
{
    const GattBassServerConfig *obj = parent;

    PanicFalse(obj && memberDescriptor);
    PanicFalse(arrayElement == 0);
    PanicFalse(memberDescriptor->offset == offsetof(GattBassServerConfig, receiveStateCcc));

    return obj->receiveStateCccSize;
}

static const marshal_type_descriptor_dynamic_t mtd_GattBassServerConfig =
    MAKE_MARSHAL_TYPE_DEFINITION_HAS_PTR_TO_DYNAMIC_ARRAY(
        GattBassServerConfig,
        mmd_GattBassServerConfig,
        getCccSize);

/********************************************************************
 * Marshal Descriptors for gatt_bass_server_ccc_data_t
 ********************************************************************/
static const marshal_member_descriptor_t mmd_gatt_bass_server_ccc_data_t[] =
{
    MAKE_MARSHAL_MEMBER(gatt_bass_server_ccc_data_t, connection_id_t,  cid),
    MAKE_MARSHAL_MEMBER(gatt_bass_server_ccc_data_t, GattBassServerConfig, client_cfg),
};

static const marshal_type_descriptor_t mtd_gatt_bass_server_ccc_data_t =
    MAKE_MARSHAL_TYPE_DEFINITION(gatt_bass_server_ccc_data_t, mmd_gatt_bass_server_ccc_data_t);

/********************************************************************
 * Marshal Descriptors for GattBassServerSubGroupsData
 ********************************************************************/
static uint32 getMetadataLen(const void *parent,
                             const marshal_member_descriptor_t *memberDescriptor,
                             uint32 arrayElement)
{
    const GattBassServerSubGroupsData *obj = parent;

    PanicFalse(obj && memberDescriptor);
    PanicFalse(arrayElement == 0);
    PanicFalse(memberDescriptor->offset == offsetof(GattBassServerSubGroupsData, metadata));

    return obj->metadataLen;
}

static const marshal_member_descriptor_t mmd_GattBassServerSubGroupsData[] =
{
    MAKE_MARSHAL_MEMBER(GattBassServerSubGroupsData, uint32, bisSync),
    MAKE_MARSHAL_MEMBER(GattBassServerSubGroupsData, uint8, metadataLen),
    MAKE_MARSHAL_MEMBER_POINTER(GattBassServerSubGroupsData, uint8_dyn_arr_t, metadata),
};

static const marshal_type_descriptor_dynamic_t mtd_GattBassServerSubGroupsData =
    MAKE_MARSHAL_TYPE_DEFINITION_HAS_PTR_TO_DYNAMIC_ARRAY(
        GattBassServerSubGroupsData,
        mmd_GattBassServerSubGroupsData,
        getMetadataLen);

/********************************************************************
 * Marshal Descriptors for uint24
 ********************************************************************/
static const marshal_type_descriptor_t mtd_uint24 =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(uint24);

/********************************************************************
 * Marshal Descriptors for BD_ADDR_T
 ********************************************************************/

static const marshal_member_descriptor_t mmd_BD_ADDR_T[] =
{
    MAKE_MARSHAL_MEMBER(BD_ADDR_T, uint24, lap),
    MAKE_MARSHAL_MEMBER(BD_ADDR_T, uint8, uap),
    MAKE_MARSHAL_MEMBER(BD_ADDR_T, uint8, nap),
};

static const marshal_type_descriptor_t mtd_BD_ADDR_T =
MAKE_MARSHAL_TYPE_DEFINITION(BD_ADDR_T, mmd_BD_ADDR_T);

/********************************************************************
 * Marshal Descriptors for CsrBtTypedAddr
 ********************************************************************/

static const marshal_member_descriptor_t mmd_CsrBtTypedAddr[] =
{
    MAKE_MARSHAL_MEMBER(CsrBtTypedAddr, uint8, type),
    MAKE_MARSHAL_MEMBER(CsrBtTypedAddr, BD_ADDR_T, addr),
};

static const marshal_type_descriptor_t mtd_CsrBtTypedAddr =
    MAKE_MARSHAL_TYPE_DEFINITION(CsrBtTypedAddr, mmd_CsrBtTypedAddr);

/********************************************************************
 * Marshal Descriptors for GattBassServerPaSyncState
 ********************************************************************/
static const marshal_type_descriptor_t mtd_GattBassServerPaSyncState =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(GattBassServerPaSyncState);

/********************************************************************
 * Marshal Descriptors for GattBassServerBroadcastBigEncryption
 ********************************************************************/
static const marshal_type_descriptor_t mtd_GattBassServerBroadcastBigEncryption =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(GattBassServerBroadcastBigEncryption);

/********************************************************************
 * Marshal Descriptors for GattBassServerSubGroupsData_dyn_arr_t
 ********************************************************************/

/* Dummy type to describe the dynamic array of GattBassServerSubGroupsData */
typedef struct GattBassServerSubGroupsData_dyn_arr
{
    GattBassServerSubGroupsData array[1];
} GattBassServerSubGroupsData_dyn_arr_t;

/* Member descriptors for the dynamic array */
static const marshal_member_descriptor_t mmd_GattBassServerSubGroupsData_dyn_arr_t[] =
{
    MAKE_MARSHAL_MEMBER_ARRAY(GattBassServerSubGroupsData_dyn_arr_t, GattBassServerSubGroupsData, array, 1),
};

/* Since GattBassServerSubGroupsData_dyn_arr_t doesn't know the length of its array (only its parent
   knows) a callback is not set. The parent's array_elements callback
   will be called instead. */
const marshal_type_descriptor_dynamic_t mtd_GattBassServerSubGroupsData_dyn_arr_t =
        MAKE_MARSHAL_TYPE_DEFINITION_HAS_DYNAMIC_ARRAY(GattBassServerSubGroupsData_dyn_arr_t,
                                                       mmd_GattBassServerSubGroupsData_dyn_arr_t,
                                                       NULL);

/********************************************************************
 * Marshal Descriptors for gattBassBroadcastHandoverSourceInfo
 ********************************************************************/

static const marshal_member_descriptor_t mmd_gattBassBroadcastHandoverSourceInfo[] =
{
    MAKE_MARSHAL_MEMBER(gattBassBroadcastHandoverSourceInfo, uint8, source_id),
    MAKE_MARSHAL_MEMBER_ARRAY(gattBassBroadcastHandoverSourceInfo,
                              uint8,
                              broadcast_code,
                              GATT_BASS_SERVER_BROADCAST_CODE_SIZE),
    MAKE_MARSHAL_MEMBER(gattBassBroadcastHandoverSourceInfo, GattBassServerPaSyncState, paSyncState),
    MAKE_MARSHAL_MEMBER(gattBassBroadcastHandoverSourceInfo,
                        GattBassServerBroadcastBigEncryption,
                        bigEncryption),
    MAKE_MARSHAL_MEMBER(gattBassBroadcastHandoverSourceInfo, CsrBtTypedAddr, sourceAddress),
    MAKE_MARSHAL_MEMBER(gattBassBroadcastHandoverSourceInfo, uint32, broadcastId),
    MAKE_MARSHAL_MEMBER(gattBassBroadcastHandoverSourceInfo, uint8, sourceAdvSid),
    MAKE_MARSHAL_MEMBER(gattBassBroadcastHandoverSourceInfo, uint8, numSubGroups),
    MAKE_MARSHAL_MEMBER_ARRAY(gattBassBroadcastHandoverSourceInfo,
                              uint8,
                              badCode,
                              GATT_BASS_SERVER_BROADCAST_CODE_SIZE),
    MAKE_MARSHAL_MEMBER_POINTER(gattBassBroadcastHandoverSourceInfo, GattBassServerSubGroupsData_dyn_arr_t, subGroupsData),
};

static uint32 getDynamicDataLen(const void *parent,
                               const marshal_member_descriptor_t *memberDescriptor,
                               uint32 arrayElement)
{
    const gattBassBroadcastHandoverSourceInfo *obj = parent;
    uint32 len = 0;

    PanicFalse(obj && memberDescriptor);
    PanicFalse(arrayElement == 0);
    PanicFalse(memberDescriptor->offset == offsetof(gattBassBroadcastHandoverSourceInfo, subGroupsData));

    len = obj->numSubGroups;

    return len;
}

static const marshal_type_descriptor_dynamic_t mtd_gattBassBroadcastHandoverSourceInfo =
    MAKE_MARSHAL_TYPE_DEFINITION_HAS_PTR_TO_DYNAMIC_ARRAY(
        gattBassBroadcastHandoverSourceInfo,
        mmd_gattBassBroadcastHandoverSourceInfo,
        getDynamicDataLen
);

/********************************************************************
 * Marshal Descriptors for gattBassServerFirstHandoverData
 ********************************************************************/

static const marshal_member_descriptor_t mmd_gattBassServerFirstHandoverData[] =
{
    MAKE_MARSHAL_MEMBER(gattBassServerFirstHandoverData,
                        gatt_bass_server_ccc_data_t,
                        connectedClient),
    MAKE_MARSHAL_MEMBER_ARRAY_OF_POINTERS(gattBassServerFirstHandoverData,
                                          gattBassBroadcastHandoverSourceInfo,
                                          broadcast_source,
                                          BASS_SERVER_BROADCAST_RECEIVE_STATE_NUM)
};

static const marshal_type_descriptor_t mtd_gattBassServerFirstHandoverData =
        MAKE_MARSHAL_TYPE_DEFINITION(gattBassServerFirstHandoverData,
                                     mmd_gattBassServerFirstHandoverData);

/********************************************************************
 * Array of pointers to mtd_* structures
 ********************************************************************/

/* Use xmacro to expand type table as array of type descriptors */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *) &mtd_##type,
const marshal_type_descriptor_t * const  mtdBassConnection[] =
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    BASS_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
#undef EXPAND_AS_TYPE_DEFINITION

/****************************** END of Desc code ******************************/

static void gattBassClientDataInit(gatt_bass_server_ccc_data_t bassData,
                                   gatt_bass_server_ccc_data_t * bassServerConnectionData)
{
    bassServerConnectionData->cid = bassData.cid;
    bassServerConnectionData->client_cfg.receiveStateCccSize = bassData.client_cfg.receiveStateCccSize;

    bassServerConnectionData->client_cfg.receiveStateCcc =
           CsrPmemZalloc(sizeof(uint16) * bassData.client_cfg.receiveStateCccSize);

    SynMemMoveS(bassServerConnectionData->client_cfg.receiveStateCcc,
             (sizeof(uint16) * bassServerConnectionData->client_cfg.receiveStateCccSize),
             bassData.client_cfg.receiveStateCcc,
             sizeof(uint16) * bassData.client_cfg.receiveStateCccSize);
}

static gattBassServerFirstHandoverData* gattFirstBassConnectionInfoInit(gatt_bass_server_data_t *data,
                                                                        uint8 index)
{
    gattBassServerFirstHandoverData* connectionInfo = NULL;
    size_t size = data->broadcast_receive_state_num ?
                (sizeof(gatt_bass_broadcast_source_info_t *) * (data->broadcast_receive_state_num - 1)) : 0;
    uint8 i = 0;

    connectionInfo = (gattBassServerFirstHandoverData *)
                         CsrPmemZalloc(sizeof(gattBassServerFirstHandoverData) + size);

    gattBassClientDataInit(data->connected_clients[index],
                           &connectionInfo->connectedClient);

    if(data->broadcast_source)
    {
        for(i=0; i<data->broadcast_receive_state_num; i++)
        {
            if(data->broadcast_source[i])
            {
                connectionInfo->broadcast_source[i] = zpnew(gattBassBroadcastHandoverSourceInfo);

                connectionInfo->broadcast_source[i]->source_id = data->broadcast_source[i]->source_id;
                connectionInfo->broadcast_source[i]->bigEncryption =
                        data->broadcast_source[i]->broadcast_source_state.bigEncryption;
                connectionInfo->broadcast_source[i]->broadcastId =
                        data->broadcast_source[i]->broadcast_source_state.broadcastId;
                connectionInfo->broadcast_source[i]->numSubGroups =
                        data->broadcast_source[i]->broadcast_source_state.numSubGroups;
                connectionInfo->broadcast_source[i]->paSyncState =
                        data->broadcast_source[i]->broadcast_source_state.paSyncState;
                connectionInfo->broadcast_source[i]->sourceAddress =
                        data->broadcast_source[i]->broadcast_source_state.sourceAddress;
                connectionInfo->broadcast_source[i]->sourceAdvSid =
                        data->broadcast_source[i]->broadcast_source_state.sourceAdvSid;

                if(data->broadcast_source[i]->broadcast_source_state.badCode)
                {
                    SynMemMoveS(connectionInfo->broadcast_source[i]->badCode,
                             GATT_BASS_SERVER_BROADCAST_CODE_SIZE,
                             data->broadcast_source[i]->broadcast_source_state.badCode,
                             GATT_BASS_SERVER_BROADCAST_CODE_SIZE);
                }
                else
                {
                    memset(connectionInfo->broadcast_source[i]->badCode, 0, GATT_BASS_SERVER_BROADCAST_CODE_SIZE);
                }

                if(data->broadcast_source[i]->broadcast_source_state.subGroupsData)
                {
                    uint8 j = 0;

                    connectionInfo->broadcast_source[i]->subGroupsData =
                            (GattBassServerSubGroupsData *) CsrPmemZalloc(
                                (sizeof(GattBassServerSubGroupsData)) * data->broadcast_source[i]->broadcast_source_state.numSubGroups);

                    for(j=0; j<data->broadcast_source[i]->broadcast_source_state.numSubGroups; j++)
                    {
                        connectionInfo->broadcast_source[i]->subGroupsData[j].bisSync =
                                data->broadcast_source[i]->broadcast_source_state.subGroupsData[j].bisSync;
                        connectionInfo->broadcast_source[i]->subGroupsData[j].metadataLen =
                                data->broadcast_source[i]->broadcast_source_state.subGroupsData[j].metadataLen;

                        if(data->broadcast_source[i]->broadcast_source_state.subGroupsData[j].metadataLen)
                        {
                            connectionInfo->broadcast_source[i]->subGroupsData[j].metadata =
                                    (uint8 *) CsrPmemZalloc(data->broadcast_source[i]->broadcast_source_state.subGroupsData[j].metadataLen * sizeof(uint8));

                            SynMemMoveS(connectionInfo->broadcast_source[i]->subGroupsData[j].metadata,
                                     data->broadcast_source[i]->broadcast_source_state.subGroupsData[j].metadataLen,
                                     data->broadcast_source[i]->broadcast_source_state.subGroupsData[j].metadata,
                                     data->broadcast_source[i]->broadcast_source_state.subGroupsData[j].metadataLen);
                        }
                        else
                        {
                            connectionInfo->broadcast_source[i]->subGroupsData[j].metadata = NULL;
                        }
                    }
                }
                else
                {
                    connectionInfo->broadcast_source[i]->subGroupsData = NULL;
                }
            }
            else
            {
                connectionInfo->broadcast_source[i] = NULL;
            }
        }
    }
    else
    {
        connectionInfo->broadcast_source[0] = NULL;
    }

    return connectionInfo;
}

static void gattBassConnectionInfoDestroy(BassHandoverMgr* bassHandoverMgr)
{
    uint8 i = 0;
    if(bassHandoverMgr->handoverFirstConnectionInfo)
    {

        if(bassHandoverMgr->handoverFirstConnectionInfo->connectedClient.client_cfg.receiveStateCcc)
            CsrPmemFree(bassHandoverMgr->handoverFirstConnectionInfo->connectedClient.client_cfg.receiveStateCcc);

        for(i=0; i<BASS_SERVER_BROADCAST_RECEIVE_STATE_NUM; i++)
        {
            if(bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[i])
            {
                if(bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[i]->subGroupsData)
                {
                    uint8 j = 0;

                    for(j=0; j<bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[i]->numSubGroups; j++)
                    {
                        if(bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[i]->subGroupsData[j].metadata)
                            CsrPmemFree(bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[i]->subGroupsData[j].metadata);
                    }

                    CsrPmemFree(bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[i]->subGroupsData);
                }

                CsrPmemFree(bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[i]);
            }
        }

        CsrPmemFree(bassHandoverMgr->handoverFirstConnectionInfo);
        bassHandoverMgr->handoverFirstConnectionInfo = NULL;
    }
    for (i=0; i<BASS_SERVER_MAX_CONNECTIONS-1; i++)
    {
        if (bassHandoverMgr->handoverSubsequentConnectionInfo[i])
        {
            if(bassHandoverMgr->handoverSubsequentConnectionInfo[i]->client_cfg.receiveStateCcc)
            {
                CsrPmemFree(bassHandoverMgr->handoverSubsequentConnectionInfo[i]->client_cfg.receiveStateCcc);
            }
            
            CsrPmemFree(bassHandoverMgr->handoverSubsequentConnectionInfo[i]);
            bassHandoverMgr->handoverSubsequentConnectionInfo[i] = NULL;
        }
    }
}

bool bassServerHandoverMgrMarshal(BassHandoverMgr* bassHandoverMgr,
                                  gatt_bass_server_data_t data,
                                  uint8 indexConn,
                                  uint8 handoverStep,
                                  uint8 *buf,
                                  uint16 length,
                                  uint16 *written)
{
    bool marshalled;
    void * handoverConnectionInfo = NULL;
    marshal_type_t handoverConnectionInfoMarshalType = handoverStep != 0 ?
                                                           MARSHAL_TYPE(gatt_bass_server_ccc_data_t) :
                                                           MARSHAL_TYPE(gattBassServerFirstHandoverData);

    if(data.broadcast_receive_state_num != BASS_SERVER_BROADCAST_RECEIVE_STATE_NUM)
        GATT_BASS_SERVER_PANIC("The number of Broadcast Receive State characteristics doesn't correspond"
                               "to the number BASS Handover API is expecting (BASS_SERVER_BROADCAST_RECEIVE_STATE_NUM)\n");

    if (!bassHandoverMgr->marshallerInitialised)
    {
        bassHandoverMgr->marshaller = MarshalInit(mtdBassConnection, BASS_MARSHAL_OBJ_TYPE_COUNT);
        bassHandoverMgr->marshallerInitialised = TRUE;

        if(handoverStep == 0)
        {
            bassHandoverMgr->handoverFirstConnectionInfo = gattFirstBassConnectionInfoInit(&data,
                                                                                           indexConn);
        }
        else
        {
            bassHandoverMgr->handoverSubsequentConnectionInfo[handoverStep-1] = zpnew(gatt_bass_server_ccc_data_t);
            gattBassClientDataInit(data.connected_clients[indexConn],
                                   bassHandoverMgr->handoverSubsequentConnectionInfo[handoverStep-1]);
        }
    }

    handoverConnectionInfo = handoverStep != 0 ? (void *) bassHandoverMgr->handoverSubsequentConnectionInfo[handoverStep-1] :
                                                  (void *) bassHandoverMgr->handoverFirstConnectionInfo;

    MarshalSetBuffer(bassHandoverMgr->marshaller, (void *) buf, length);

    marshalled = Marshal(bassHandoverMgr->marshaller,
                         handoverConnectionInfo,
                         handoverConnectionInfoMarshalType);

    *written = MarshalProduced(bassHandoverMgr->marshaller);

    if (marshalled)
    {
        MarshalDestroy(bassHandoverMgr->marshaller, /*free_all_objects*/ FALSE);
        bassHandoverMgr->marshallerInitialised = FALSE;

        gattBassConnectionInfoDestroy(bassHandoverMgr);
    }
    return marshalled;
}

bool bassServerHandoverMgrUnmarshal(BassHandoverMgr* bassHandoverMgr,
                                    uint8 handoverStep,
                                    connection_id_t cid,
                                    const uint8 *buf,
                                    uint16 length,
                                    uint16 *consumed)
{
    marshal_type_t unmarshalledType;
    bool unMarshalled;
    void ** handoverConnectionInfo = (handoverStep != 0 ?
                                                 (void **) &bassHandoverMgr->handoverSubsequentConnectionInfo[handoverStep-1] :
                                                 (void **) &bassHandoverMgr->handoverFirstConnectionInfo);

    if (!bassHandoverMgr->unMarshallerInitialised)
    {
        bassHandoverMgr->unMarshaller = UnmarshalInit(mtdBassConnection, BASS_MARSHAL_OBJ_TYPE_COUNT);
        bassHandoverMgr->unMarshallerInitialised = TRUE;
    }
    UnmarshalSetBuffer(bassHandoverMgr->unMarshaller, (void *) buf, length);

    unMarshalled = Unmarshal(bassHandoverMgr->unMarshaller,
                             handoverConnectionInfo,
                             &unmarshalledType);

    *consumed = UnmarshalConsumed(bassHandoverMgr->unMarshaller);

    if (unMarshalled)
    {
        PanicNull(handoverConnectionInfo);

        if(handoverStep != 0)
        {
            PanicFalse(unmarshalledType == MARSHAL_TYPE(gatt_bass_server_ccc_data_t));
            bassHandoverMgr->handoverSubsequentConnectionInfo[handoverStep-1]->cid = cid;
        }
        else
        {
            PanicFalse(unmarshalledType == MARSHAL_TYPE(gattBassServerFirstHandoverData));
            bassHandoverMgr->handoverFirstConnectionInfo->connectedClient.cid = cid;
        }
        UnmarshalDestroy(bassHandoverMgr->unMarshaller, /*free_all_objects*/FALSE);
        bassHandoverMgr->unMarshallerInitialised = FALSE;
    }
    return unMarshalled;
}

static bool bassServerHandoverBadCodeIsChanged(GBASSSS* bassServer, uint8 index)
{
    uint8 *badCode = bassServer->data.broadcast_source[index]->broadcast_source_state.badCode;
    bool res = FALSE;

    if(!badCode)
        badCode = zpmalloc(GATT_BASS_SERVER_BROADCAST_CODE_SIZE);

    res = memcmp(badCode,
                  bassServer->bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[index]->badCode,
                  GATT_BASS_SERVER_BROADCAST_CODE_SIZE);

    if(!bassServer->data.broadcast_source[index]->broadcast_source_state.badCode)
        free(badCode);

    return res;
}

static bool bassServerIsAnyChangeToNotify(GBASSSS* bassServer,
                                          uint8 index)
{  
    if((!bassServer->bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[index] &&
        bassServer->data.broadcast_source[index]) ||
       (bassServer->bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[index] &&
        !bassServer->data.broadcast_source[index]))
    {
        return TRUE;
    }

    if(!bassServer->bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[index] &&
       !bassServer->data.broadcast_source[index])
    {
        return FALSE;
    }

    if((bassServer->bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[index]->broadcastId !=
            bassServer->data.broadcast_source[index]->broadcast_source_state.broadcastId) ||
       (bassServer->bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[index]->sourceAdvSid !=
            bassServer->data.broadcast_source[index]->broadcast_source_state.sourceAdvSid) ||
       (bassServer->bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[index]->bigEncryption !=
            bassServer->data.broadcast_source[index]->broadcast_source_state.bigEncryption) ||
       (bassServer->bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[index]->paSyncState  !=
            bassServer->data.broadcast_source[index]->broadcast_source_state.paSyncState)  ||
       (bassServer->bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[index]->sourceAddress.type !=
        bassServer->data.broadcast_source[index]->broadcast_source_state.sourceAddress.type) ||
       (!CsrBtBdAddrEq(&(bassServer->bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[index]->sourceAddress.addr),
                       &(bassServer->data.broadcast_source[index]->broadcast_source_state.sourceAddress.addr))) ||
       bassServerHandoverBadCodeIsChanged(bassServer,
                                          index) ||
       bassServer->data.broadcast_source[index]->broadcast_source_state.numSubGroups !=
            bassServer->bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[index]->numSubGroups ||
       bassServerIsBisSyncChanged(bassServer,
                                  bassServer->bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[index]->subGroupsData,
                                  index) ||
       bassServerIsMetadataChanged(bassServer,
                                   bassServer->bassHandoverMgr->handoverFirstConnectionInfo->broadcast_source[index]->subGroupsData,
                                   index))
    {
        return TRUE;
    }

    return FALSE;
}

static uint8 bassServerAddHandoverConnection(GBASSSS* bassServer,
                                            gatt_bass_server_ccc_data_t bassServerClientData)
{
    uint8 i = 0;

    for (i=0; i < BASS_SERVER_MAX_CONNECTIONS; i++)
    {
        if (bassServer->data.connected_clients[i].cid == 0)
        {
            bassServer->data.connected_clients[i].cid = bassServerClientData.cid;
            bassServer->data.connected_clients[i].client_cfg.receiveStateCccSize =
                    bassServerClientData.client_cfg.receiveStateCccSize;

            if(bassServer->data.connected_clients[i].client_cfg.receiveStateCcc)
                free(bassServer->data.connected_clients[i].client_cfg.receiveStateCcc);

            if(bassServerClientData.client_cfg.receiveStateCcc)
            {
                bassServer->data.connected_clients[i].client_cfg.receiveStateCcc =
                        (uint16 *) CsrPmemAlloc(sizeof(uint16) * bassServerClientData.client_cfg.receiveStateCccSize);

                SynMemMoveS(bassServer->data.connected_clients[i].client_cfg.receiveStateCcc,
                         sizeof(uint16) * bassServerClientData.client_cfg.receiveStateCccSize,
                         bassServerClientData.client_cfg.receiveStateCcc,
                         sizeof(uint16) * bassServerClientData.client_cfg.receiveStateCccSize);
            }
            else
            {
                bassServer->data.connected_clients[i].client_cfg.receiveStateCcc = NULL;
            }

            return i;
        }
    }

    return GATT_BASS_SERVER_INVALID_CID_INDEX;
}

void bassServerHandoverMgrCommit(GBASSSS* bassServer, connection_id_t cid)
{
    bool clientIsFound = FALSE;
    uint8 index = 0;
    uint8 clientIndex = GATT_BASS_SERVER_INVALID_CID_INDEX;
    uint8 i = 0;
    uint8 j;

    if(bassServer->data.broadcast_receive_state_num != BASS_SERVER_BROADCAST_RECEIVE_STATE_NUM)
        GATT_BASS_SERVER_PANIC("The number of Broadcast Receive State characteristics doesn't correspond"
                               "to the number BASS Handover API is expecting (BASS_SERVER_BROADCAST_RECEIVE_STATE_NUM)\n");

    clientIsFound = bassServerFindCid(bassServer, cid, &index);

    if(bassServer->bassHandoverMgr->handoverFirstConnectionInfo &&
        cid == bassServer->bassHandoverMgr->handoverFirstConnectionInfo->connectedClient.cid)
    {
        if(!clientIsFound)
        {
            clientIndex = bassServerAddHandoverConnection(bassServer,
                                                          bassServer->bassHandoverMgr->handoverFirstConnectionInfo->connectedClient);

            if(clientIndex == GATT_BASS_SERVER_INVALID_CID_INDEX)
                GATT_BASS_SERVER_ERROR("bassServerHandoverMgrCommit: bassServerAddHandoverConnection failed!\n");
        }

        if(bassServer->data.broadcast_source)
        {
            for(i=0; i<bassServer->data.broadcast_receive_state_num; i++)
            {
                if(bassServerIsAnyChangeToNotify(bassServer, i) &&
                   clientIndex != GATT_BASS_SERVER_INVALID_CID_INDEX)
                {
                    /* The Broadcast Receive State characteristic on the primary and its correspondant one on the
                     * secondary ara different: we need to notify the client if it has registered for notification*/
                    bassServerNotifyBroadcastReceiveStateCharacteristicToSingleClient(bassServer, i, clientIndex);
                    bassServer->isToBeNotified[i] = TRUE;
                }
            }
        }
    }
    
    else
    {
        for (j=0; j < BASS_SERVER_MAX_CONNECTIONS-1; j++)
        {
            if (bassServer->bassHandoverMgr->handoverSubsequentConnectionInfo[j] &&
                cid == bassServer->bassHandoverMgr->handoverSubsequentConnectionInfo[j]->cid)
            {
                if(!clientIsFound)
                {
                    clientIndex = bassServerAddHandoverConnection(bassServer,
                                                                  (*bassServer->bassHandoverMgr->handoverSubsequentConnectionInfo[j]));

                    if(clientIndex == GATT_BASS_SERVER_INVALID_CID_INDEX)
                        GATT_BASS_SERVER_ERROR("bassServerHandoverMgrCommit: bassServerAddHandoverConnection failed!\n");
                    else
                    {
                        for(i=0; i<BASS_SERVER_BROADCAST_RECEIVE_STATE_NUM; i++)
                        {
                            if(bassServer->isToBeNotified[i])
                                bassServerNotifyBroadcastReceiveStateCharacteristicToSingleClient(bassServer, i, clientIndex);
                        }
                    }
                }
            }
        }
    }

    if(clientIsFound)
        GATT_BASS_SERVER_DEBUG("bassServerHandoverMgrCommit: Connected client already present!\n");
}


void bassServerHandoverMgrComplete(BassHandoverMgr* bassHandoverMgr)
{
    if(bassHandoverMgr)
    {
        if (bassHandoverMgr->marshallerInitialised)
        {
            /* The Handover procedure has finished (failed to complete successfully
             * in this case), and the Marshaller has not been destroyed, it may
             * contain objects that have not been freed.
             */
            MarshalDestroy(bassHandoverMgr->marshaller, /*free_all_objects*/FALSE);
            bassHandoverMgr->marshallerInitialised = FALSE;
        }

        if (bassHandoverMgr->unMarshallerInitialised)
        {
            /* The Handover procedure has finished (failed to complete successfully
             * in this case), and the UnMarshaller has not been destroyed, it still
             * contains objects that have not been freed.
             */
            UnmarshalDestroy(bassHandoverMgr->unMarshaller, /*free_all_objects*/FALSE);
            bassHandoverMgr->unMarshallerInitialised = FALSE;
        }

        gattBassConnectionInfoDestroy(bassHandoverMgr);
    }
}
