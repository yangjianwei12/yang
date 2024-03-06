/* Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd. */
/* %%version */
#define TRAPSET_MARSHAL 1
#include "gatt_mics_server_private.h"
#include "gatt_mics_server_handover_mgr.h"
#include "gatt_mics_server_common.h"
#include "gatt_mics_server.h"

#include <stdlib.h>
#include <panic.h>
#include <marshal.h>

typedef void* Sink;
typedef uint16 Task;

#include "../../../../../../marshal_common_desc/marshal_common_desc.h"
#include "csr_bt_core_stack_pmalloc.h"

/**************************** Start of Desc code ******************************/
#define MICS_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(gattMicsFirstHandoverData) \
    ENTRY(GattMicsClientData) \
    ENTRY(GattMicsClientConfigDataType) \
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
      MICS_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
      MICS_MARSHAL_OBJ_TYPE_COUNT
};
#undef EXPAND_AS_ENUMERATION

/********************************************************************
 * Marshal Descriptors for GattMicsServerConfig
 ********************************************************************/
static const marshal_type_descriptor_t mtd_GattMicsClientConfigDataType =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(GattMicsClientConfigDataType);

/********************************************************************
 * Marshal Descriptors for connection_id_t
 ********************************************************************/
static const marshal_type_descriptor_t mtd_connection_id_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(connection_id_t);

/********************************************************************
 * Marshal Descriptors for GattMicsClientData
 ********************************************************************/
static const marshal_member_descriptor_t mmd_gatt_mics_client_data[] =
{
    MAKE_MARSHAL_MEMBER(GattMicsClientData, connection_id_t, cid),
    MAKE_MARSHAL_MEMBER(GattMicsClientData, GattMicsClientConfigDataType, clientCfg),
};

static const marshal_type_descriptor_t mtd_GattMicsClientData =
    MAKE_MARSHAL_TYPE_DEFINITION(GattMicsClientData, mmd_gatt_mics_client_data);

/********************************************************************
 * Marshal Descriptors for gattMicsFirstHandoverData
 ********************************************************************/

static const marshal_member_descriptor_t mmd_gattMicsFirstHandoverData[] =
{
    MAKE_MARSHAL_MEMBER(gattMicsFirstHandoverData, uint8, micsServerMute),
    MAKE_MARSHAL_MEMBER(gattMicsFirstHandoverData, uint16, numClients),
    MAKE_MARSHAL_MEMBER(gattMicsFirstHandoverData, GattMicsClientData, connectedClient),
};

static const marshal_type_descriptor_t mtd_gattMicsFirstHandoverData =
MAKE_MARSHAL_TYPE_DEFINITION(
    gattMicsFirstHandoverData,
    mmd_gattMicsFirstHandoverData);

/********************************************************************
 * Array of pointers to mtd_* structures
 ********************************************************************/

/* Use xmacro to expand type table as array of type descriptors */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *) &mtd_##type,
const marshal_type_descriptor_t * const  mtdMicsConnection[] =
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    MICS_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
#undef EXPAND_AS_TYPE_DEFINITION

/****************************** END of Desc code ******************************/

static gattMicsFirstHandoverData * gattFirstMicsdataInit(micsData micsData, uint8 index)
{
    gattMicsFirstHandoverData *micControlServerData = zpnew(gattMicsFirstHandoverData);

    micControlServerData->micsServerMute = micsData.micsServerMute;
    micControlServerData->numClients = micsData.numClients;

    micControlServerData->connectedClient = micsData.connectedClients[index];

    return micControlServerData;
}

static GattMicsClientData * gattMicsClientDataInit(micsData micsData, uint8 index)
{
    GattMicsClientData *micControlServerData = NULL;

    micControlServerData = zpnew(GattMicsClientData);
    (*micControlServerData) = micsData.connectedClients[index];

    return micControlServerData;
}

static bool gattMicsServerAddHandoverConnection(GMICS_T *micControlServer,
                                               GattMicsClientData micsClientData)
{
    uint8 i = 0;

    for (i=0; i < MICS_MAX_CONNECTIONS; i++)
    {
        if (micControlServer->data.connectedClients[i].cid == 0)
        {
            micControlServer->data.connectedClients[i] = micsClientData;
            return TRUE;
        }
    }

    return FALSE;
}

bool micsServerHandoverMgrMarshal(MicsHandoverMgr* micsHandoverMgr,
                                 micsData micsData,
                                 uint8 indexConn,
                                 uint8 handoverStep,
                                 uint8 *buf,
                                 uint16 length,
                                 uint16 *written)
{
    bool marshalled;
    void * handoverConnectionInfo = NULL;
    marshal_type_t handoverConnectionInfoMarshalType = (handoverStep != 0) ?
                                                           MARSHAL_TYPE(GattMicsClientData) :
                                                           MARSHAL_TYPE(gattMicsFirstHandoverData);

    if (!micsHandoverMgr->marshallerInitialised)
    {
        micsHandoverMgr->marshaller = MarshalInit(mtdMicsConnection, MICS_MARSHAL_OBJ_TYPE_COUNT);
        micsHandoverMgr->marshallerInitialised = TRUE;

        if(handoverStep == 0)
            micsHandoverMgr->handoverFirstConnectionInfo = gattFirstMicsdataInit(micsData,
                                                                               indexConn);
        else
            micsHandoverMgr->handoverSubsequentConnectionInfo[handoverStep-1] = gattMicsClientDataInit(micsData,
                                                                                   indexConn);
    }

    handoverConnectionInfo = (handoverStep != 0) ? (void *) micsHandoverMgr->handoverSubsequentConnectionInfo[handoverStep-1] :
                                                  (void *) micsHandoverMgr->handoverFirstConnectionInfo;


    MarshalSetBuffer(micsHandoverMgr->marshaller, (void *) buf, length);

    marshalled = Marshal(micsHandoverMgr->marshaller,
                         handoverConnectionInfo,
                         handoverConnectionInfoMarshalType);

    *written = MarshalProduced(micsHandoverMgr->marshaller);

    if (marshalled)
    {
        uint8 i;
        MarshalDestroy(micsHandoverMgr->marshaller, /*free_all_objects*/FALSE);
        micsHandoverMgr->marshallerInitialised = FALSE;

        if(micsHandoverMgr->handoverFirstConnectionInfo)
        {
            free(micsHandoverMgr->handoverFirstConnectionInfo);
            micsHandoverMgr->handoverFirstConnectionInfo = NULL;
        }
        for (i = 0; i < MICS_MAX_CONNECTIONS-1; i++)
        {
            if(micsHandoverMgr->handoverSubsequentConnectionInfo[i])
            {
                free(micsHandoverMgr->handoverSubsequentConnectionInfo[i]);
                micsHandoverMgr->handoverSubsequentConnectionInfo[i] = NULL;
            }
        }
    }
    return marshalled;
}

bool micsServerHandoverMgrUnmarshal(MicsHandoverMgr* micsHandoverMgr,
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
        handoverConnectionInfo = (void **) &micsHandoverMgr->handoverSubsequentConnectionInfo[handoverStep-1];
    }
    else
    {
        handoverConnectionInfo = (void **) &micsHandoverMgr->handoverFirstConnectionInfo;
    }

    if (!micsHandoverMgr->unMarshallerInitialised)
    {
        micsHandoverMgr->unMarshaller = UnmarshalInit(mtdMicsConnection, MICS_MARSHAL_OBJ_TYPE_COUNT);
        micsHandoverMgr->unMarshallerInitialised = TRUE;
    }

    UnmarshalSetBuffer(micsHandoverMgr->unMarshaller, (void *) buf, length);

    unMarshalled = Unmarshal(micsHandoverMgr->unMarshaller,
                             handoverConnectionInfo,
                             &unmarshalledType);

    *consumed = UnmarshalConsumed(micsHandoverMgr->unMarshaller);

    if (unMarshalled)
    {
        PanicNull(handoverConnectionInfo);

        if(handoverStep != 0)
        {
            PanicFalse(unmarshalledType == MARSHAL_TYPE(GattMicsClientData));
            micsHandoverMgr->handoverSubsequentConnectionInfo[handoverStep-1]->cid = cid;
        }
        else
        {
            PanicFalse(unmarshalledType == MARSHAL_TYPE(gattMicsFirstHandoverData));
            micsHandoverMgr->handoverFirstConnectionInfo->connectedClient.cid = cid;
        }
        UnmarshalDestroy(micsHandoverMgr->unMarshaller, /*free_all_objects*/FALSE);
        micsHandoverMgr->unMarshallerInitialised = FALSE;
    }
    return unMarshalled;
}

void micsServerHandoverMgrCommit(GMICS_T* micControlServer, connection_id_t cid)
{
    GattMicsClientData micsServerClientData =
    {
        .cid = 0,
        .clientCfg.micsMuteClientCfg = 0,
    };

    uint8 index = micsServerGetCidIndex(micControlServer, cid);
    uint8 i;


    if(micControlServer->micsHandoverMgr == NULL)
    {
        GATT_MICS_SERVER_ERROR("Called micsServerHandoverMgrCommit but no micsHandoverMgr allocated - has (un)marshal been called and no previous call to Complete() or Abort()?\n");
        return;
    }

    if(micControlServer->micsHandoverMgr->handoverFirstConnectionInfo &&
        cid == micControlServer->micsHandoverMgr->handoverFirstConnectionInfo->connectedClient.cid)
    {
        micControlServer->data.micsServerMute =
                micControlServer->micsHandoverMgr->handoverFirstConnectionInfo->micsServerMute;
        micControlServer->data.numClients =
                micControlServer->micsHandoverMgr->handoverFirstConnectionInfo->numClients;

        micsServerClientData = micControlServer->micsHandoverMgr->handoverFirstConnectionInfo->connectedClient;
    }
    else
    {
        for (i = 0; i < MICS_MAX_CONNECTIONS-1; i++)
        {
            if(micControlServer->micsHandoverMgr->handoverSubsequentConnectionInfo[i] &&
               cid == micControlServer->micsHandoverMgr->handoverSubsequentConnectionInfo[i]->cid)
                
            {
                micsServerClientData  = (*micControlServer->micsHandoverMgr->handoverSubsequentConnectionInfo[i]);
            }
        }
    }
    if (micsServerClientData.cid == 0)
    {
        GATT_MICS_SERVER_DEBUG("micsServerHandoverMgrCommit: Connection not found!\n");
    }
    else if(index == GATT_MICS_SERVER_INVALID_CID_INDEX)
    {
        if(!gattMicsServerAddHandoverConnection(micControlServer,
                                               micsServerClientData))
            GATT_MICS_SERVER_ERROR("micsServerHandoverMgrCommit: gattMicsServerAddHandoverConnection failed!\n");
    }
    else
    {
        GATT_MICS_SERVER_DEBUG("micsServerHandoverMgrCommit: Connected client already present!\n");
    }
}

void micsServerHandoverMgrComplete(MicsHandoverMgr* micsHandoverMgr)
{
    uint8 i;
    if(micsHandoverMgr)
    {
        if (micsHandoverMgr->marshallerInitialised)
        {
            /* The Handover procedure has finished (failed to complete successfully
             * in this case), and the Marshaller has not been destroyed, it may
             * contain objects that have not been freed.
             */
            MarshalDestroy(micsHandoverMgr->marshaller, /*free_all_objects*/FALSE);
            micsHandoverMgr->marshallerInitialised = FALSE;
        }

        if (micsHandoverMgr->unMarshallerInitialised)
        {
            /* The Handover procedure has finished (failed to complete successfully
             * in this case), and the UnMarshaller has not been destroyed, it still
             * contains objects that have not been freed.
             */
            UnmarshalDestroy(micsHandoverMgr->unMarshaller, /*free_all_objects*/FALSE);
            micsHandoverMgr->unMarshallerInitialised = FALSE;
        }

        if(micsHandoverMgr->handoverFirstConnectionInfo)
        {
            free(micsHandoverMgr->handoverFirstConnectionInfo);
            micsHandoverMgr->handoverFirstConnectionInfo = NULL;
        }
        for (i = 0; i < MICS_MAX_CONNECTIONS-1; i++)
        {
            if(micsHandoverMgr->handoverSubsequentConnectionInfo[i])
            {
                free(micsHandoverMgr->handoverSubsequentConnectionInfo[i]);
                micsHandoverMgr->handoverSubsequentConnectionInfo[i] = NULL;
            }
        }
    }
}
