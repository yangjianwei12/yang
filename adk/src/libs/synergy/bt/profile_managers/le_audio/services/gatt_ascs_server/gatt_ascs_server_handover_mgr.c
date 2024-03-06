/* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd. */
/* %%version */
#define TRAPSET_MARSHAL 1
#include "service_handle.h"
#include "gatt_ascs_server_private.h"
#include "gatt_ascs_server_handover_mgr.h"
#include "gatt_ascs_server_debug.h"


#include <stdlib.h>
#include <panic.h>
#include <marshal.h>

/*! TODO: move the following definitions and Desc related code to desc file */
typedef void* Sink;
typedef uint16 Task;
#include "../../../../../../marshal_common_desc/marshal_common_desc.h"

/**************************** Start of Desc code ******************************/
#define ASCS_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(GattAscsServerConfigureCodecInfo) \
    ENTRY(GattAscsServerConfigureCodecServerInfo) \
    ENTRY(GattAscsCodecId) \
    ENTRY(GattAscsServerConfigureQosInfo) \
    ENTRY(AscsAseDynamicData) \
    ENTRY(GattAscsServerAse) \
    ENTRY(AscsConnectionInfo)

/********************************************************************
 * Enum of 'type' identifiers used for marshalling, of the form MARSHAL_TYPE_xyz
 ********************************************************************/

/* Use xmacro to expand type table as enumeration of marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
      COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
      COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
      ASCS_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
      ASCS_MARSHAL_OBJ_TYPE_COUNT
};
#undef EXPAND_AS_ENUMERATION
/*
enum
{
    MARSHAL_TYPE_int, \
    MARSHAL_TYPE_int16, \
    MARSHAL_TYPE_uint8, \
    MARSHAL_TYPE_uint16, \
    MARSHAL_TYPE_uint32, \
    MARSHAL_TYPE_bdaddr, \
    MARSHAL_TYPE_typed_bdaddr, \
    MARSHAL_TYPE_TRANSPORT_T, \
    MARSHAL_TYPE_tp_bdaddr, \
    MARSHAL_TYPE_L2capSink,
    MARSHAL_TYPE_uint8_dyn_arr_t,
    MARSHAL_TYPE_GattAscsServerConfigureCodecInfo, \
    MARSHAL_TYPE_GattAscsServerConfigureCodecServerInfo, \
    MARSHAL_TYPE_GattAscsCodecId, \
    MARSHAL_TYPE_GattAscsServerConfigureQosInfo, \
    MARSHAL_TYPE_GattAscsServerAse, \
    MARSHAL_TYPE_AscsConnectionInfo,
};
*/

/********************************************************************
 * Marshal Descriptors for GattAscsCodecId
 ********************************************************************/
static const marshal_member_descriptor_t mmd_GattAscsCodecId[] =
{
    MAKE_MARSHAL_MEMBER(GattAscsCodecId, uint8,  codingFormat),
    MAKE_MARSHAL_MEMBER(GattAscsCodecId, uint16, companyId),
    MAKE_MARSHAL_MEMBER(GattAscsCodecId, uint16, vendorSpecificCodecId),
};

static const marshal_type_descriptor_t mtd_GattAscsCodecId =
    MAKE_MARSHAL_TYPE_DEFINITION(GattAscsCodecId, mmd_GattAscsCodecId);

/********************************************************************
 * Marshal Descriptors for GattAscsServerConfigureCodecServerInfo
 ********************************************************************/
static uint32 getCodecConfigurationLength(const void *parent,
                                          const marshal_member_descriptor_t *memberDescriptor,
                                          uint32 arrayElement)
{
    const GattAscsServerConfigureCodecServerInfo *obj = parent;

    PanicFalse(obj && memberDescriptor);
    PanicFalse(arrayElement == 0);
    PanicFalse(memberDescriptor->offset == offsetof(GattAscsServerConfigureCodecServerInfo, codecConfiguration));

    return obj->codecConfigurationLength;
}

static const marshal_member_descriptor_t mmd_GattAscsServerConfigureCodecServerInfo[] =
{
    MAKE_MARSHAL_MEMBER(GattAscsServerConfigureCodecServerInfo, uint32, presentationDelayMin),
    MAKE_MARSHAL_MEMBER(GattAscsServerConfigureCodecServerInfo, uint8, codecConfigurationLength),
    MAKE_MARSHAL_MEMBER_POINTER(GattAscsServerConfigureCodecServerInfo, uint8_dyn_arr_t, codecConfiguration),
};

static const marshal_type_descriptor_dynamic_t mtd_GattAscsServerConfigureCodecServerInfo =
    MAKE_MARSHAL_TYPE_DEFINITION_HAS_PTR_TO_DYNAMIC_ARRAY(
        GattAscsServerConfigureCodecServerInfo,
        mmd_GattAscsServerConfigureCodecServerInfo,
        getCodecConfigurationLength);


/********************************************************************
 * Marshal Descriptors for GattAscsServerConfigureCodecInfo
 ********************************************************************/

static const marshal_member_descriptor_t mmd_GattAscsServerConfigureCodecInfo[] =
{
    MAKE_MARSHAL_MEMBER(GattAscsServerConfigureCodecInfo, GattAscsCodecId, codecId),
    MAKE_MARSHAL_MEMBER(GattAscsServerConfigureCodecInfo, uint8, targetLatency),
    MAKE_MARSHAL_MEMBER(GattAscsServerConfigureCodecInfo, uint8, targetPhy),
    MAKE_MARSHAL_MEMBER(GattAscsServerConfigureCodecInfo, GattAscsServerConfigureCodecServerInfo, infoFromServer)
};

static const marshal_type_descriptor_t mtd_GattAscsServerConfigureCodecInfo =
MAKE_MARSHAL_TYPE_DEFINITION(GattAscsServerConfigureCodecInfo, mmd_GattAscsServerConfigureCodecInfo);

/********************************************************************
 * Marshal Descriptors for GattAscsServerConfigureQosInfo
 ********************************************************************/

static const marshal_member_descriptor_t mmd_GattAscsServerConfigureQosInfo[] =
{
    MAKE_MARSHAL_MEMBER(GattAscsServerConfigureQosInfo, uint8, cisId),
    MAKE_MARSHAL_MEMBER(GattAscsServerConfigureQosInfo, uint8, cigId),
    MAKE_MARSHAL_MEMBER(GattAscsServerConfigureQosInfo, uint8, retransmissionNumber),
    MAKE_MARSHAL_MEMBER(GattAscsServerConfigureQosInfo, uint8, framing),  /* GattAscsFraming is a uint8 */
    MAKE_MARSHAL_MEMBER(GattAscsServerConfigureQosInfo, uint8, phy),   /* GattAscsPhy is a uint8 */
    MAKE_MARSHAL_MEMBER(GattAscsServerConfigureQosInfo, uint16, maximumSduSize),
    MAKE_MARSHAL_MEMBER(GattAscsServerConfigureQosInfo, uint16, maxTransportLatency),
    MAKE_MARSHAL_MEMBER(GattAscsServerConfigureQosInfo, uint32, sduInterval),
    MAKE_MARSHAL_MEMBER(GattAscsServerConfigureQosInfo, uint32, presentationDelay),
};

static const marshal_type_descriptor_t mtd_GattAscsServerConfigureQosInfo =
    MAKE_MARSHAL_TYPE_DEFINITION(GattAscsServerConfigureQosInfo, mmd_GattAscsServerConfigureQosInfo);

/********************************************************************
 * Marshal Descriptors for AscsAseDynamicData
 ********************************************************************/

static const marshal_member_descriptor_t mmd_AscsAseDynamicData[] =
{
    MAKE_MARSHAL_MEMBER(        AscsAseDynamicData, uint8,                            metadataLength),
    MAKE_MARSHAL_MEMBER_POINTER(AscsAseDynamicData, uint8_dyn_arr_t,                  metadata),
    MAKE_MARSHAL_MEMBER(        AscsAseDynamicData, GattAscsServerConfigureCodecInfo, cachedConfigureCodecInfo),
    MAKE_MARSHAL_MEMBER_POINTER(AscsAseDynamicData, GattAscsServerConfigureQosInfo,   cachedConfigureQosInfoFromServer),
};

static uint32 getMetadataLength(const void *parent,
                                const marshal_member_descriptor_t *memberDescriptor,
                                uint32 arrayElement)
{
    const AscsAseDynamicData *obj = parent;

    PanicFalse(obj && memberDescriptor);
    PanicFalse(arrayElement == 0);
    PanicFalse(memberDescriptor->offset == offsetof(AscsAseDynamicData, metadata));

    return obj->metadataLength;
}

static const marshal_type_descriptor_dynamic_t mtd_AscsAseDynamicData =
    MAKE_MARSHAL_TYPE_DEFINITION_HAS_PTR_TO_DYNAMIC_ARRAY(
        AscsAseDynamicData,
        mmd_AscsAseDynamicData,
        getMetadataLength);

/********************************************************************
 * Marshal Descriptors for GattAscsServerAse
 ********************************************************************/

static const marshal_member_descriptor_t mmd_GattAscsServerAse[] =
{
    MAKE_MARSHAL_MEMBER(GattAscsServerAse, uint8, aseId),
    MAKE_MARSHAL_MEMBER(GattAscsServerAse, uint8, state),        /* GattAscsServerState is a uint8 */
    MAKE_MARSHAL_MEMBER(GattAscsServerAse, uint16, clientCfg),   /* ClientCfg is a uint16 */
    MAKE_MARSHAL_MEMBER_POINTER(GattAscsServerAse, AscsAseDynamicData, dynamicData)
};

static const marshal_type_descriptor_t mtd_GattAscsServerAse =
    MAKE_MARSHAL_TYPE_DEFINITION(GattAscsServerAse, mmd_GattAscsServerAse);

/********************************************************************
 * Marshal Descriptors for AscsConnectionInfo
 ********************************************************************/

static const marshal_member_descriptor_t mmd_AscsConnectionInfo[] =
{
    MAKE_MARSHAL_MEMBER(AscsConnectionInfo, uint16, clientCfg),
    MAKE_MARSHAL_MEMBER(AscsConnectionInfo, uint8, numAses),
    MAKE_MARSHAL_MEMBER_ARRAY_OF_POINTERS(AscsConnectionInfo, GattAscsServerAse, ase, GATT_ASCS_NUM_ASES_MAX),
};

static const marshal_type_descriptor_t mtd_AscsConnectionInfo =
MAKE_MARSHAL_TYPE_DEFINITION(
    AscsConnectionInfo,
    mmd_AscsConnectionInfo);

/********************************************************************
 * Array of pointers to mtd_* structures
 ********************************************************************/

/* Use xmacro to expand type table as array of type descriptors */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *) &mtd_##type,
const marshal_type_descriptor_t * const  mtdAscsConnection[] =
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    ASCS_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
#undef EXPAND_AS_TYPE_DEFINITION

/****************************** END of Desc code ******************************/


static AscsConnectionInfo* gattAscsConnectionInfoInit(GattAscsConnection const * const connection)
{
    AscsConnectionInfo* connectionInfo = zpnew(AscsConnectionInfo);

    connectionInfo->clientCfg = connection->aseControlPointNotify.clientCfg;
    connectionInfo->numAses   = GATT_ASCS_NUM_ASES_MAX;

    for (int aseIdx = 0; aseIdx < GATT_ASCS_NUM_ASES_MAX; ++aseIdx)
    {
        /* Shallow copy - ensure the marshal code doesn't free memory,
         * i.e. set the free_all_objects parameter to FALSE in Un/MarshalDestroy() */
        connectionInfo->ase[aseIdx] = &connection->ase[aseIdx];
    }

    return connectionInfo;
}

static bool ascsServerHandoverMgrMarshal(AscsHandoverMgr* ascsHandoverMgr,
                                   GattAscsConnection* connection,
                                   uint8 *buf,
                                   uint16 length,
                                   uint16 *written)
{
    bool marshalled;

    if (!ascsHandoverMgr->marshallerInitialised)
    {
        ascsHandoverMgr->marshaller = MarshalInit(mtdAscsConnection, ASCS_MARSHAL_OBJ_TYPE_COUNT);
        ascsHandoverMgr->marshallerInitialised = TRUE;
        ascsHandoverMgr->handoverConnectionInfo = gattAscsConnectionInfoInit(connection);
    }

    MarshalSetBuffer(ascsHandoverMgr->marshaller, (void *) buf, length);

    marshalled = Marshal(ascsHandoverMgr->marshaller, ascsHandoverMgr->handoverConnectionInfo, MARSHAL_TYPE(AscsConnectionInfo));

    *written = MarshalProduced(ascsHandoverMgr->marshaller);

    if (marshalled)
    {
        MarshalDestroy(ascsHandoverMgr->marshaller, /*free_all_objects*/FALSE);
        ascsHandoverMgr->marshallerInitialised = FALSE;

        free(ascsHandoverMgr->handoverConnectionInfo);
        ascsHandoverMgr->handoverConnectionInfo = NULL;
    }
    return marshalled;
}

/****************************************************************************
NAME
    gattAscsServerHandoverMarshal

DESCRIPTION
    Marshal the data associated with the ASCS Server

RETURNS
    bool TRUE if ASCS module marshaling completed successfully, or if it cannot complete and must be abandoned
         (e.g. if the cid is unrecognised).
         FALSE if marshaling is ongoing and we need more buffers (provided in subsequent calls to gattAscsMarshal() )
         before marshaling can be completed.
*/
bool gattAscsServerHandoverMarshal(ServiceHandle serviceHandle,
                           ConnectionId cid,
                           uint8 *buf,
                           uint16 length,
                           uint16 *written)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);

    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }

    GattAscsConnection* connection = ascsFindConnection(ascs, cid);

    if (connection)
    {
        if (ascs->ascsHandoverMgr == NULL)
        {
            ascs->ascsHandoverMgr = zpnew(AscsHandoverMgr); /* counters, bools etc. are initialised to zero */
        }
        return ascsServerHandoverMgrMarshal(ascs->ascsHandoverMgr, connection, buf, length, written);
    }
    else
    {
        /* Connection not found, nothing to marshal */
        *written = 0;
        return TRUE;
    }
}

static GattAscsConnection* ascsConnectionConstructFromConnInfo(AscsConnectionInfo const * const connectionInfo, ConnectionId cid)
{
    GattAscsConnection* connection = zpnew(GattAscsConnection);

    if (!connection)
        return NULL;

    connection->aseControlPointNotify.clientCfg = connectionInfo->clientCfg;
    /* NOTE: Other fields of the AseControlPointNotify are set during ascsAseControlPointCharacteristicReset()
     * which is called whenever an operation is received from the client */

    /* If we are handing over from an earbud that has a different Maximum ASE Capacity
       then there will be stability issues.
       Can a possible (future) replacement earbud have a larger Maximum ASE capacity?
    */
    if (connectionInfo->numAses != GATT_ASCS_NUM_ASES_MAX)
    {
        GATT_ASCS_SERVER_PANIC("Cannot handover if ASE capacity of one earbud is different to the other\n");
    }

    connection->cid = cid;

    for (int aseIdx = 0;  aseIdx < GATT_ASCS_NUM_ASES_MAX; ++aseIdx)
    {
        /* Shallow copy - need to make sure the marshal code doesn't free memory (set
         * the free_all_objects parameter to FALSE in Un/MarshalDestroy() */
        connection->ase[aseIdx] = *(connectionInfo->ase[aseIdx]);
        free(connectionInfo->ase[aseIdx]);
    }

    return connection;
}

static bool ascsServerHandoverMgrUnmarshal(AscsHandoverMgr* ascsHandoverMgr,
                                     ConnectionId cid,
                                     const uint8 *buf,
                                     uint16 length,
                                     uint16 *consumed)
{
    marshal_type_t unmarshalledType;
    bool unMarshalled;

    if (!ascsHandoverMgr->unMarshallerInitialised)
    {
        ascsHandoverMgr->unMarshaller = UnmarshalInit(mtdAscsConnection, ASCS_MARSHAL_OBJ_TYPE_COUNT);
        ascsHandoverMgr->unMarshallerInitialised = TRUE;
    }
    UnmarshalSetBuffer(ascsHandoverMgr->unMarshaller, (void *) buf, length);

    unMarshalled = Unmarshal(ascsHandoverMgr->unMarshaller, (void**)&ascsHandoverMgr->handoverConnectionInfo, &unmarshalledType);

    *consumed = UnmarshalConsumed(ascsHandoverMgr->unMarshaller);

    if (unMarshalled)
    {
        GattAscsConnection* newAscsConnection;
        PanicFalse(unmarshalledType == MARSHAL_TYPE(AscsConnectionInfo));
        PanicNull(ascsHandoverMgr->handoverConnectionInfo);
        newAscsConnection = ascsConnectionConstructFromConnInfo(ascsHandoverMgr->handoverConnectionInfo, cid);

        if (newAscsConnection)
        {
            ascsHandoverMgr->handoverConnection[ascsHandoverMgr->numHandoverConnections++] = newAscsConnection;
        }

        free(ascsHandoverMgr->handoverConnectionInfo);
        ascsHandoverMgr->handoverConnectionInfo = NULL;
        UnmarshalDestroy(ascsHandoverMgr->unMarshaller, /*free_all_objects*/FALSE);
        ascsHandoverMgr->unMarshallerInitialised = FALSE;
    }
    return unMarshalled;
}

bool gattAscsServerHandoverUnmarshal(ServiceHandle serviceHandle,
                             ConnectionId cid,
                             const uint8 *buf,
                             uint16 length,
                             uint16 *consumed)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);

    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    if (ascs->ascsHandoverMgr == NULL)
    {
        ascs->ascsHandoverMgr = zpnew(AscsHandoverMgr); /* counters, bools etc. are initialised to zero */
    }
    return ascsServerHandoverMgrUnmarshal(ascs->ascsHandoverMgr, cid, buf, length, consumed);
}

static void ascsHandoverCommitConnection(AscsHandoverMgr* ascsHandoverMgr, GattAscsServer* ascs, ConnectionId cid)
{
    bool moveConnectionToLowerIndex = FALSE;
    int connIdx;
    const uint8 numHandoverConnections = MIN(ascsHandoverMgr->numHandoverConnections, GATT_ASCS_NUM_CONNECTIONS_MAX);

    for (connIdx = 0; connIdx < numHandoverConnections; ++connIdx)
    {
        if (moveConnectionToLowerIndex)
        {
            ascsHandoverMgr->handoverConnection[connIdx - 1] = ascsHandoverMgr->handoverConnection[connIdx];
        }
        else
        {
            if (ascsHandoverMgr->handoverConnection[connIdx]->cid == cid)
            {
                if (ascs->numConnections < GATT_ASCS_NUM_CONNECTIONS_MAX)
                {
                    /* We have capacity in ASCS to store this handed over connection */
                    ascs->connection[ ascs->numConnections++ ] = ascsHandoverMgr->handoverConnection[connIdx];
                }
                else
                {
                    /* Handover is going to go very wrong at this point; we cannot commit this connection
                     * because we cannot store any more connections in ASCS and we cannot communicate this
                     * problem upwards (the top level handover API call has a void return)*/
                    GATT_ASCS_SERVER_DEBUG("ascsServerHandoverMgrCommit: handed over connection cannot be committed - ASCS numConnections is at max value (%d)\n", ascs->numConnections);
                    ascsConnectionDestruct(ascsHandoverMgr->handoverConnection[connIdx]);
                }
                moveConnectionToLowerIndex = TRUE;
                ascsHandoverMgr->numHandoverConnections--;
            }
        }
    }
}

static void ascsServerHandoverMgrCommit(AscsHandoverMgr* ascsHandoverMgr, GattAscsServer* ascs, ConnectionId cid, const bool newPrimary)
{
    if (newPrimary)
    {
        /* This is the new primary EB */
        ascsHandoverCommitConnection(ascsHandoverMgr, ascs, cid);
    }
    else
    {
        /* This is the new secondary EB */
         GattAscsConnection* connection = ascsFindConnection(ascs, cid);
         if (connection)
         {
             ascsRemoveConnection(ascs, connection);
         }
    }
}

void gattAscsServerHandoverCommit(ServiceHandle serviceHandle, ConnectionId cid, const bool newPrimary)
{

    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);

    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }

    if (ascs->ascsHandoverMgr != NULL)
    {
        ascsServerHandoverMgrCommit(ascs->ascsHandoverMgr, ascs, cid, newPrimary);
    }
    else
    {
        GATT_ASCS_SERVER_ERROR("Called gattAscsServerHandoverCommit but no ascsHandoverMgr allocated - has (un)marshal been called and no previous call to Complete() or Abort()?\n");
    }
}

static void ascsServerHandoverMgrComplete(AscsHandoverMgr* ascsHandoverMgr)
{
    if (ascsHandoverMgr->marshallerInitialised)
    {
        /* The Handover procedure has finished (failed to complete successfully
         * in this case), and the Marshaller has not been destroyed, it may
         * contain objects that have not been freed.
         */
        MarshalDestroy(ascsHandoverMgr->marshaller, /*free_all_objects*/FALSE);
        ascsHandoverMgr->marshallerInitialised = FALSE;
    }

    if (ascsHandoverMgr->unMarshallerInitialised)
    {
        /* The Handover procedure has finished (failed to complete successfully
         * in this case), and the UnMarshaller has not been destroyed, it still
         * contains objects that have not been freed.
         *
         * Fortunately, the unmarshaller will not give any data to ASCS unless it
         * has successfully unmarshalled a complete connection, at which point
         * the unmarshaller is destroyed. Since the unnmarshaller has *not* been
         * destroyed (ascsHandoverMgr->unMarshallerInitialised == TRUE) , it has
         * not given a connection to ASCS, so passing 'TRUE' as the free_all_object
         * argument should prevent any memory leaks and should not cause any
         * 'double frees'.
         */
        UnmarshalDestroy(ascsHandoverMgr->unMarshaller, /*free_all_objects*/TRUE);
        ascsHandoverMgr->unMarshallerInitialised = FALSE;
    }

    const uint8 numHandoverConnections = MIN(ascsHandoverMgr->numHandoverConnections, GATT_ASCS_NUM_CONNECTIONS_MAX);
    for (int connIdx = 0; connIdx < numHandoverConnections; ++connIdx)
    {
        ascsConnectionDestruct(ascsHandoverMgr->handoverConnection[connIdx]);
    }
    ascsHandoverMgr->numHandoverConnections = 0;
}

static void ascsServerHandoverMgrCleanup(ServiceHandle serviceHandle)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);

    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    /*
     * At the moment there is no difference between handling a hand over 'abort' or a hand over 'complete':
     * they both ensure any 'leftover' hand over connections are freed.
     */
    if (ascs->ascsHandoverMgr != NULL)
    {
        ascsServerHandoverMgrComplete(ascs->ascsHandoverMgr);
        free(ascs->ascsHandoverMgr);
        ascs->ascsHandoverMgr = NULL;
    }
    else
    {
        GATT_ASCS_SERVER_ERROR("Called ascsServerHandoverMgrCleanup but no ascsHandoverMgr allocated - has (un)marshal been called and no previous call to Complete() or Abort()?\n");
    }
}

void gattAscsServerHandoverAbort(ServiceHandle serviceHandle)
{
    /*
     * At the moment there is no difference between handling a hand over 'abort' or a hand over 'complete':
     * they both ensure any 'leftover' hand over connections are freed.
     */
    ascsServerHandoverMgrCleanup(serviceHandle);
}

void gattAscsServerHandoverComplete(ServiceHandle serviceHandle, const bool newPrimary )
{
    (void)newPrimary;
    /*
     * At the moment there is no difference between handling a hand over 'abort' or a hand over 'complete':
     * they both ensure any 'leftover' hand over connections are freed.
     */

    ascsServerHandoverMgrCleanup(serviceHandle);
}

static bool ascsAseVeto(GattAscsServerAse const * const ase)
{
    bool veto = FALSE;

    switch (ase->state)
    {
    case GATT_ASCS_SERVER_ASE_STATE_ENABLING:
        /* fall through */
    case GATT_ASCS_SERVER_ASE_STATE_DISABLING:
        /* fall through */
    case GATT_ASCS_SERVER_ASE_STATE_RELEASING:
        veto = TRUE; /* veto transient states */
        break;
    /*
    case GATT_ASCS_SERVER_ASE_STATE_IDLE:
    case GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED:
    case GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED:
    case GATT_ASCS_SERVER_ASE_STATE_STREAMING:
        veto = FALSE; * do not veto in these 'steady' states *
        break;
    */
    default:
        /* no action */
        break;
    };

    return veto;
}

static bool ascsConnectionVeto(GattAscsConnection const * const connection)
{
    int aseIdx;
    for (aseIdx = 0; aseIdx < GATT_ASCS_NUM_ASES_MAX; ++aseIdx)
    {
        if (ascsAseVeto(&connection->ase[aseIdx]))
            return TRUE;
    }
    return FALSE;
}

bool gattAscsServerHandoverVeto(ServiceHandle serviceHandle)
{
    int connIdx;
    (void) serviceHandle;
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);

    if (ascs == NULL)
    {
        /* Handover cannot proceed if we do not have an ASCS instance */
        gattAscsServerPanic();
    }

    const uint8 numConnections = MIN(ascs->numConnections, GATT_ASCS_NUM_CONNECTIONS_MAX);
    for (connIdx = 0; connIdx < numConnections; ++connIdx)
    {
        if (ascsConnectionVeto(ascs->connection[connIdx]))
            return TRUE;
    }
    return FALSE;
}

bool gattAscsServerHasValidConnection(ServiceHandle serviceHandle, ConnectionId cid)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);

    if (ascs && ascsFindConnection(ascs, cid))
    {
        return TRUE;
    }

    return FALSE;
}