/* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd. */
/* %%version */
#define TRAPSET_MARSHAL 1
#include "gatt_vcs_server_private.h"
#include "gatt_vcs_server_handover_mgr.h"
#include "gatt_vcs_server_common.h"
#include "gatt_vcs_server.h"

#include <stdlib.h>
#include <panic.h>
#include <marshal.h>

typedef void* Sink;
typedef uint16 Task;

#include "../../../../../../marshal_common_desc/marshal_common_desc.h"
#include "csr_bt_core_stack_pmalloc.h"

/**************************** Start of Desc code ******************************/
#define VCS_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(gattVcsFirstHandoverData) \
    ENTRY(gatt_vcs_client_data) \
    ENTRY(GattVcsServerConfig) \
    ENTRY(connection_id_t)

/********************************************************************
 * Enum of 'type' identifiers used for marshalling, of the form MARSHAL_TYPE_xyz
 ********************************************************************/

/* Use xmacro to expand type table as enumeration of marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
      COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
      COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
      VCS_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
      VCS_MARSHAL_OBJ_TYPE_COUNT
};
#undef EXPAND_AS_ENUMERATION

/********************************************************************
 * Marshal Descriptors for GattVcsServerConfig
 ********************************************************************/
static const marshal_type_descriptor_t mtd_GattVcsServerConfig =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(GattVcsServerConfig);

/********************************************************************
 * Marshal Descriptors for connection_id_t
 ********************************************************************/
static const marshal_type_descriptor_t mtd_connection_id_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(connection_id_t);

/********************************************************************
 * Marshal Descriptors for gatt_vcs_client_data
 ********************************************************************/
static const marshal_member_descriptor_t mmd_gatt_vcs_client_data[] =
{
    MAKE_MARSHAL_MEMBER(gatt_vcs_client_data, connection_id_t, cid),
    MAKE_MARSHAL_MEMBER(gatt_vcs_client_data, GattVcsServerConfig, client_cfg),
};

static const marshal_type_descriptor_t mtd_gatt_vcs_client_data =
    MAKE_MARSHAL_TYPE_DEFINITION(gatt_vcs_client_data, mmd_gatt_vcs_client_data);

/********************************************************************
 * Marshal Descriptors for gattVcsFirstHandoverData
 ********************************************************************/

static const marshal_member_descriptor_t mmd_gattVcsFirstHandoverData[] =
{
    MAKE_MARSHAL_MEMBER(gattVcsFirstHandoverData, uint8, volume_setting),
    MAKE_MARSHAL_MEMBER(gattVcsFirstHandoverData, uint8, mute),
    MAKE_MARSHAL_MEMBER(gattVcsFirstHandoverData, uint8, change_counter),
    MAKE_MARSHAL_MEMBER(gattVcsFirstHandoverData, uint8, step_size),
    MAKE_MARSHAL_MEMBER(gattVcsFirstHandoverData, uint8, volume_flag),
    MAKE_MARSHAL_MEMBER(gattVcsFirstHandoverData, gatt_vcs_client_data, connectedClient),
};

static const marshal_type_descriptor_t mtd_gattVcsFirstHandoverData =
MAKE_MARSHAL_TYPE_DEFINITION(
    gattVcsFirstHandoverData,
    mmd_gattVcsFirstHandoverData);

/********************************************************************
 * Array of pointers to mtd_* structures
 ********************************************************************/

/* Use xmacro to expand type table as array of type descriptors */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *) &mtd_##type,
const marshal_type_descriptor_t * const  mtdVcsConnection[] =
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    VCS_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
#undef EXPAND_AS_TYPE_DEFINITION

/****************************** END of Desc code ******************************/

static gattVcsFirstHandoverData * gattFirstVcsdataInit(gatt_vcs_data vcsData, uint8 index)
{
    gattVcsFirstHandoverData *volumeControlServerData = zpnew(gattVcsFirstHandoverData);

    volumeControlServerData->change_counter = vcsData.change_counter;
    volumeControlServerData->mute = vcsData.mute;
    volumeControlServerData->step_size = vcsData.step_size;
    volumeControlServerData->volume_flag = vcsData.volume_flag;
    volumeControlServerData->volume_setting = vcsData.volume_setting;

    volumeControlServerData->connectedClient = vcsData.connected_clients[index];

    return volumeControlServerData;
}

static gatt_vcs_client_data * gattVcsClientDataInit(gatt_vcs_data vcsData, uint8 index)
{
    gatt_vcs_client_data *volumeControlServerData = NULL;

    volumeControlServerData = zpnew(gatt_vcs_client_data);
    (*volumeControlServerData) = vcsData.connected_clients[index];

    return volumeControlServerData;
}

static bool gattVcsServerAddHandoverConnection(GVCS *volumeControlServer,
                                               gatt_vcs_client_data vcsClientData)
{
    uint8 i = 0;

    for (i=0; i < GATT_VCS_MAX_CONNECTIONS; i++)
    {
        if (volumeControlServer->data.connected_clients[i].cid == 0)
        {
            volumeControlServer->data.connected_clients[i] = vcsClientData;
            return TRUE;
        }
    }

    return FALSE;
}

bool vcsServerHandoverMgrMarshal(VcsHandoverMgr* vcsHandoverMgr,
                                 gatt_vcs_data vcsData,
                                 uint8 indexConn,
                                 uint8 handoverStep,
                                 uint8 *buf,
                                 uint16 length,
                                 uint16 *written)
{
    bool marshalled;
    void * handoverConnectionInfo = NULL;
    marshal_type_t handoverConnectionInfoMarshalType = (handoverStep != 0) ?
                                                           MARSHAL_TYPE(gatt_vcs_client_data) :
                                                           MARSHAL_TYPE(gattVcsFirstHandoverData);

    if (!vcsHandoverMgr->marshallerInitialised)
    {
        vcsHandoverMgr->marshaller = MarshalInit(mtdVcsConnection, VCS_MARSHAL_OBJ_TYPE_COUNT);
        vcsHandoverMgr->marshallerInitialised = TRUE;

        if(handoverStep == 0)
            vcsHandoverMgr->handoverFirstConnectionInfo = gattFirstVcsdataInit(vcsData,
                                                                               indexConn);
        else
            vcsHandoverMgr->handoverSubsequentConnectionInfo[handoverStep-1] = gattVcsClientDataInit(vcsData,
                                                                                   indexConn);
    }

    handoverConnectionInfo = (handoverStep != 0) ? (void *) vcsHandoverMgr->handoverSubsequentConnectionInfo[handoverStep-1] :
                                                  (void *) vcsHandoverMgr->handoverFirstConnectionInfo;


    MarshalSetBuffer(vcsHandoverMgr->marshaller, (void *) buf, length);

    marshalled = Marshal(vcsHandoverMgr->marshaller,
                         handoverConnectionInfo,
                         handoverConnectionInfoMarshalType);

    *written = MarshalProduced(vcsHandoverMgr->marshaller);

    if (marshalled)
    {
        uint8 i;
        MarshalDestroy(vcsHandoverMgr->marshaller, /*free_all_objects*/FALSE);
        vcsHandoverMgr->marshallerInitialised = FALSE;

        if(vcsHandoverMgr->handoverFirstConnectionInfo)
        {
            free(vcsHandoverMgr->handoverFirstConnectionInfo);
            vcsHandoverMgr->handoverFirstConnectionInfo = NULL;
        }
        for (i = 0; i < GATT_VCS_MAX_CONNECTIONS-1; i++)
        {
            if(vcsHandoverMgr->handoverSubsequentConnectionInfo[i])
            {
                free(vcsHandoverMgr->handoverSubsequentConnectionInfo[i]);
                vcsHandoverMgr->handoverSubsequentConnectionInfo[i] = NULL;
            }
        }
    }
    return marshalled;
}

bool vcsServerHandoverMgrUnmarshal(VcsHandoverMgr* vcsHandoverMgr,
                                   uint8 handoverStep,
                                   connection_id_t cid,
                                   const uint8 *buf,
                                   uint16 length,
                                   uint16 *consumed)
{
    marshal_type_t unmarshalledType;
    bool unMarshalled;
    void ** handoverConnectionInfo;

    if (handoverStep != 0)
    {
        handoverConnectionInfo = (void **) &vcsHandoverMgr->handoverSubsequentConnectionInfo[handoverStep-1];
    }
    else
    {
        handoverConnectionInfo = (void **) &vcsHandoverMgr->handoverFirstConnectionInfo;
    }

    if (!vcsHandoverMgr->unMarshallerInitialised)
    {
        vcsHandoverMgr->unMarshaller = UnmarshalInit(mtdVcsConnection, VCS_MARSHAL_OBJ_TYPE_COUNT);
        vcsHandoverMgr->unMarshallerInitialised = TRUE;
    }

    UnmarshalSetBuffer(vcsHandoverMgr->unMarshaller, (void *) buf, length);

    unMarshalled = Unmarshal(vcsHandoverMgr->unMarshaller,
                             handoverConnectionInfo,
                             &unmarshalledType);

    *consumed = UnmarshalConsumed(vcsHandoverMgr->unMarshaller);

    if (unMarshalled)
    {
        PanicNull(handoverConnectionInfo);

        if(handoverStep != 0)
        {
            PanicFalse(unmarshalledType == MARSHAL_TYPE(gatt_vcs_client_data));
            vcsHandoverMgr->handoverSubsequentConnectionInfo[handoverStep-1]->cid = cid;
        }
        else
        {
            PanicFalse(unmarshalledType == MARSHAL_TYPE(gattVcsFirstHandoverData));
            vcsHandoverMgr->handoverFirstConnectionInfo->connectedClient.cid = cid;
        }
        UnmarshalDestroy(vcsHandoverMgr->unMarshaller, /*free_all_objects*/FALSE);
        vcsHandoverMgr->unMarshallerInitialised = FALSE;
    }
    return unMarshalled;
}

void vcsServerHandoverMgrCommit(GVCS* volumeControlServer, connection_id_t cid)
{
    gatt_vcs_client_data vcsServerClientData =
    {
        .cid = 0,
        .client_cfg.volumeStateClientCfg = 0xFFFF,
        .client_cfg.volumeFlagClientCfg = 0xFFFF,
    };

    uint8 index = vcsServerGetCidIndex(volumeControlServer, cid);
    uint8 i;

    if(volumeControlServer->vcsHandoverMgr == NULL)
    {
        GATT_VCS_SERVER_ERROR("Called vcsServerHandoverMgrCommit but no vcsHandoverMgr allocated - has (un)marshal been called and no previous call to Complete() or Abort()?\n");
        return;
    }

    if(volumeControlServer->vcsHandoverMgr->handoverFirstConnectionInfo &&
        cid == volumeControlServer->vcsHandoverMgr->handoverFirstConnectionInfo->connectedClient.cid)
    {
        volumeControlServer->data.change_counter =
                volumeControlServer->vcsHandoverMgr->handoverFirstConnectionInfo->change_counter;
        volumeControlServer->data.mute =
                volumeControlServer->vcsHandoverMgr->handoverFirstConnectionInfo->mute;
        volumeControlServer->data.step_size =
                volumeControlServer->vcsHandoverMgr->handoverFirstConnectionInfo->step_size;
        volumeControlServer->data.volume_flag =
                volumeControlServer->vcsHandoverMgr->handoverFirstConnectionInfo->volume_flag;
        volumeControlServer->data.volume_setting =
                volumeControlServer->vcsHandoverMgr->handoverFirstConnectionInfo->volume_setting;

        vcsServerClientData = volumeControlServer->vcsHandoverMgr->handoverFirstConnectionInfo->connectedClient;
    }
    else
    {
        for (i = 0; i < GATT_VCS_MAX_CONNECTIONS-1; i++)
        {
            if(volumeControlServer->vcsHandoverMgr->handoverSubsequentConnectionInfo[i] &&
               cid == volumeControlServer->vcsHandoverMgr->handoverSubsequentConnectionInfo[i]->cid)
                
            {
                vcsServerClientData  = (*volumeControlServer->vcsHandoverMgr->handoverSubsequentConnectionInfo[i]);
            }
        }
    }
    if (vcsServerClientData.cid == 0)
    {
        GATT_VCS_SERVER_DEBUG("vcsServerHandoverMgrCommit: Connection not found!\n");
    }
    else if(index == GATT_VCS_SERVER_INVALID_CID_INDEX)
    {
        if(!gattVcsServerAddHandoverConnection(volumeControlServer,
                                               vcsServerClientData))
            GATT_VCS_SERVER_ERROR("vcsServerHandoverMgrCommit: gattVcsServerAddHandoverConnection failed!\n");
    }
    else
    {
        GATT_VCS_SERVER_DEBUG("vcsServerHandoverMgrCommit: Connected client already present!\n");
    }
}

void vcsServerHandoverMgrComplete(VcsHandoverMgr* vcsHandoverMgr)
{
    uint8 i;
    if(vcsHandoverMgr)
    {
        if (vcsHandoverMgr->marshallerInitialised)
        {
            /* The Handover procedure has finished (failed to complete successfully
             * in this case), and the Marshaller has not been destroyed, it may
             * contain objects that have not been freed.
             */
            MarshalDestroy(vcsHandoverMgr->marshaller, /*free_all_objects*/FALSE);
            vcsHandoverMgr->marshallerInitialised = FALSE;
        }

        if (vcsHandoverMgr->unMarshallerInitialised)
        {
            /* The Handover procedure has finished (failed to complete successfully
             * in this case), and the UnMarshaller has not been destroyed, it still
             * contains objects that have not been freed.
             */
            UnmarshalDestroy(vcsHandoverMgr->unMarshaller, /*free_all_objects*/FALSE);
            vcsHandoverMgr->unMarshallerInitialised = FALSE;
        }

        if(vcsHandoverMgr->handoverFirstConnectionInfo)
        {
            free(vcsHandoverMgr->handoverFirstConnectionInfo);
            vcsHandoverMgr->handoverFirstConnectionInfo = NULL;
        }
        for (i = 0; i < GATT_VCS_MAX_CONNECTIONS-1; i++)
        {
            if(vcsHandoverMgr->handoverSubsequentConnectionInfo[i])
            {
                free(vcsHandoverMgr->handoverSubsequentConnectionInfo[i]);
                vcsHandoverMgr->handoverSubsequentConnectionInfo[i] = NULL;
            }
        }
    }
}
