/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP interface implementation.
 */

/**
 * \addtogroup BAP_PRIVATE
 * @{
 */

#include <stdio.h>
#include <string.h>
#include "bap_client_list_container_cast.h"
#include "bap_utils.h"
#include "dm_prim.h"
#include "dmlib.h"
#include "bap_client_lib.h"
#include "bap_profile.h"
#include "bap_ase.h"
#include "bap_connection.h"
#include "bap_stream_group.h"
#include "bap_client_connection.h"
#include "bap_client_list_util_private.h"
#include "csr_bt_gatt_lib.h"
#include "bap_gatt_msg_handler.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "gatt_pacs_client.h"
#include "gatt_pacs_client_private.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "csr_bt_cm_prim.h"
#include "csr_bt_cm_lib.h"

#include "gatt_service_discovery_lib.h"

#ifdef INSTALL_LEA_BROADCAST_SOURCE
#include "bap_broadcast_src.h"
#endif

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
#include "bap_broadcast_assistant_msg_handler.h"
#include "bap_broadcast_assistant.h"
#include "bap_broadcast_gatt_client_bass.h"
#endif

static struct BAP *inst;

/*! \brief Make the BapStreamGroup structure RTTI information visible.
 */
type_name_enable_verify_of_external_type(BapStreamGroup)

/*! \brief Handle upstream CM primitive.
 *
 *  \param [in] bap      A pointer to a BAP structure.
 *  \param [in] primitive A pointer to a CsrBtCmPrim structure.
 *
 *  \return Nothing.
 */
static void handleUpstreamCmPrimitive(BAP * const bap,
                                      CsrBtCmPrim * const primitive);

/*! \brief Handle downstream primitive.
 *
 *  \param [in] bap      A pointer to a BAP structure.
 *  \param [in] primitive A pointer to a BapUPrim structure.
 *
 *  \return Nothing.
 */
static void handleDownstreamPrimitive(BAP * const bap,
                                      BapUPrim * const primitive);

/*! \brief Free downstream BAP primitive.
 *
 *  \param [in] primitive A pointer to an BapUPrim structure.
 *
 *  \return Nothing.
 */
static void freeDownstreamPrimitive(BapUPrim *const p_uprim);


static void handleBapInitReq(BAP* const bap,
                             BapInternalInitReq* const primitive);

static void handleBapDeinitReq(BAP* const bap, BapInternalDeinitReq* const primitive);

#ifdef INSTALL_LEA_UNICAST_CLIENT

/*! \brief Free upstream BAP primitive.
 *
 *  \param [in] primitive A pointer to an BapUPrim structure.
 *
 *  \return Nothing.
 */
static void freeUpstreamPrimitive(BapUPrim * const prim);


static void handleBapDiscoverAudioRoleReq(BAP* const bap,
                                          BapInternalDiscoverAudioRoleReq* const primitive);

static void handleBapDiscoverRemoteAudioCapabilityReq(BAP* const bap,
                                                      BapInternalDiscoverRemoteAudioCapabilityReq* const primitive);

static void handleBapRegisterPacsNotificationReq(BAP* const bap,
                                                 BapInternalRegisterPacsNotificationReq* const primitive);

static void handleBapGetRemoteAudioLocationReq(BAP* const bap,
                                               BapInternalGetRemoteAudioLocationReq* const primitive);

static void handleBapSetRemoteAudioLocationReq(BAP* const bap,
                                               BapInternalSetRemoteAudioLocationReq* const primitive);

static void handleBapDiscoverAudioContextReq(BAP* const bap,
                                             BapInternalDiscoverAudioContextReq* const primitive);

static void handleStreamGroupCodecConfigureReq(BAP * const bap,
                                               BapInternalUnicastClientCodecConfigureReq * const primitive);

static void handleBapUnicastClientCigConfigureReq(BAP* const bap,
                                                  BapInternalUnicastClientCigConfigureReq* const primitive);

static void handleBapUnicastClientCigTestConfigureReq(BAP* const bap,
                                                      BapInternalUnicastClientCigTestConfigureReq* const primitive);

static void handleStreamGroupQosConfigureReq(BAP * const bap,
                                             BapInternalUnicastClientQosConfigureReq * const primitive);

static void handleStreamGroupEnableReq(BAP * const bap,
                                       BapInternalUnicastClientEnableReq * const primitive);

static void handleStreamGroupDisableReq(BAP * const bap,
                                        BapInternalUnicastClientDisableReq * const primitive);

static void handleStreamGroupReleaseReq(BAP * const bap,
                                        BapInternalUnicastClientReleaseReq * const primitive);

static void handleStreamGroupUpdateMetadataReq(BAP * const bap,
                                               BapInternalUnicastClientUpdateMetadataReq * const primitive);

static void handleStreamGroupReceiverReadyReq(BAP * const bap,
                                              BapInternalUnicastClientReceiverReadyReq * const primitive);

static void handleBapUnicastClientRemoveCigReq(BAP* const bap,
                                               BapInternalUnicastClientCigRemoveReq* const primitive);

static void handleStreamGroupCisConnectReq(BAP * const bap,
                                           BapInternalUnicastClientCisConnectReq * const primitive);

static void handleStreamGroupCisDisconnectReq(BAP * const bap,
                                              BapInternalUnicastClientCisDisconnectReq * const primitive);
#endif

static void handleBapClientSetupDatapathReq(BAP* const bap,
                                            BapInternalSetupDataPathReq* const primitive);

static void handleBapClientRemoveDatapathReq(BAP* const bap,
                                             BapInternalRemoveDataPathReq* const primitive);
#ifdef INSTALL_LEA_UNICAST_CLIENT

static BapAse* bapClientFindAseByCisHandle(BAP * const bap,
                                           uint16 cisHandle);

static Bool bapClientRemoveConnectionByCid(BAP * const bap,
                                           BapProfileHandle cid);

static Bool bapClientRemoveStreamGroupById(BAP * const bap,
                                           BapProfileHandle streamGroupId);

static void handleBapSetControlPointOpReq(BAP * const bap,
                                          BapInternalSetControlPointOpReq * const req);

static void bapAscsClientSrvcInit(BapConnection *const connection,
                                  CsrBtConnId cid,
                                  uint16 startHndl,
                                  uint16 endHndl,
                                  BapAscsClientDeviceData *data);

static void bapPacsClientSrvcInit(BapConnection *const connection,
                                  CsrBtConnId cid,
                                  uint16 startHndl,
                                  uint16 endHndl,
                                  BapPacsClientDeviceData* data);

static void bapClientAseResetCisHandle(BAP * const bap, uint16 cisHandle);

static void bapConnectionAseResetCisHandle(BapConnection * const connection,
                                         uint16 cisHandle);

/*! \brief RTTI information for the BAP structure.
 */
type_name_declare_and_initialise_const_rtti_variable(BAP, 'A','s','C','p')

/*! \brief Make the QBL_TASK structure RTTI information visible.
 */
type_name_enable_verify_of_external_type(QBL_TASK)

/*! \brief Make the BapProfile structure RTTI information visible.
 */
type_name_enable_verify_of_external_type(BapProfile)

/*! \brief Make the BapConnection structure RTTI information visible.
 */
type_name_enable_verify_of_external_type(BapConnection)

/*! \brief Make the BAP_INITIATOR_CONNECTION structure RTTI information visible.
 */
type_name_enable_verify_of_external_type(BAP_INITIATOR_CONNECTION)


/*! \brief Make the BAP_ACCEPTOR_CONNECTION structure RTTI information visible.
 */
type_name_enable_verify_of_external_type(BAP_ACCEPTOR_CONNECTION)
#endif 

void bapClientInitialiseList(BAP * const bap)
{
    memset(bap, 0, sizeof(BAP));
#ifdef INSTALL_LEA_UNICAST_CLIENT
    bapClientListInitialise(&bap->profileList);
    bapClientListInitialise(&bap->connectionList);
    bapClientListInitialise(&bap->streamGroupList);

    bap->controller.state = BAP_CONTROLLER_STATE_IDLE;
#endif

#ifdef INSTALL_LEA_BROADCAST_SOURCE
    bapClientListInitialise(&bap->bigStreamList);
#endif

    type_name_initialise_rtti_member_variable(BAP, bap);
}


void bapClientStart(void)
{
#ifdef INSTALL_GATT_SUPPORT
    bapServerGattInit();
#endif
}

/**
 * \ingroup BAP
 */
void bapClientRcvMessage(BapUPrim * const prim)
{
    putMessageSynergy(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, prim);
}

/**
 * \ingroup BAP
 */
void bapClientRcvIntercontextMessage(BapUPrim * const prim)
{
    putMessageSynergy(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, prim);
}

static void handleBapInitReq(BAP* const bap,
                             BapInternalInitReq* const primitive)
{
#ifdef INSTALL_LEA_UNICAST_CLIENT
    BapConnection* connection = NULL;
    CsrBtTypedAddr addr;
    GattSdSrvcId srvcIds = GATT_SD_INVALID_SRVC;

    if (CsrBtGattClientUtilFindAddrByConnId(primitive->initData.cid, &addr))
    {
        connection = bapClientConnectionNew(bap, primitive->phandle, &addr);
        /* Update connection ID */
        connection->cid = primitive->initData.cid;
        connection->role = 0;
        connection->bapInitStatus = BAP_RESULT_SUCCESS;

        if( primitive->initData.role & BAP_ROLE_UNICAST_CLIENT)
        {
            connection->role = BAP_ROLE_UNICAST_CLIENT;
            /* Initialize service handles */
            connection->ascs.srvcHndl = INVALID_SERVICE_HANDLE;
            connection->pacs.srvcHndl = INVALID_SERVICE_HANDLE;
            srvcIds |= GATT_SD_PACS_SRVC | GATT_SD_ASCS_SRVC;
        }

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
        /* Broadcast Assistant Role handling */
        if( primitive->initData.role & BAP_ROLE_BROADCAST_ASSISTANT)
        {
            bap->appPhandle = primitive->phandle;

            /* Initialise Assistant */
            bapBroadcastAssistantInit(connection);
            srvcIds |= GATT_SD_BASS_SRVC;
        }
#endif

        /* Update BAP state */
        bap->controller.state = BAP_CONTROLLER_STATE_CONNECTED;
        bap->controller.connection = connection;
        /* Push this connection to the BAP Connection list */
        bapClientListPush(&bap->connectionList, &connection->listElement);

        if(primitive->initData.handles != NULL)
        {
            if(((srvcIds & GATT_SD_ASCS_SRVC) == GATT_SD_ASCS_SRVC) &&
                            primitive->initData.handles->ascsHandles)
            {
                bapAscsClientSrvcInit(connection,
                                      connection->cid,
                                      primitive->initData.handles->ascsHandles->startHandle,
                                      primitive->initData.handles->ascsHandles->endHandle,
                                      primitive->initData.handles->ascsHandles);

                srvcIds = srvcIds & ~GATT_SD_ASCS_SRVC;
            }

            if(((srvcIds & GATT_SD_PACS_SRVC) == GATT_SD_PACS_SRVC) &&
                           primitive->initData.handles->pacsHandles)
            {
                bapPacsClientSrvcInit(connection,
                                      connection->cid,
                                      primitive->initData.handles->pacsHandles->startHandle,
                                      primitive->initData.handles->pacsHandles->endHandle,
                                      primitive->initData.handles->pacsHandles);

                srvcIds = srvcIds & ~GATT_SD_PACS_SRVC;
            }
#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
            if(((srvcIds & GATT_SD_BASS_SRVC) == GATT_SD_BASS_SRVC) &&
                             primitive->initData.handles->bassHandles)
            {
                bapBassClientSrvcInit(connection,
                                      connection->cid,
                                      0,
                                      0,
                                      primitive->initData.handles->bassHandles);

                srvcIds = srvcIds & ~GATT_SD_BASS_SRVC;
            }
#endif
        }
       /* Do Service Discovery for handles which are not cached*/

        if(srvcIds)
        {
            GattServiceDiscoveryFindServiceRange(CSR_BT_BAP_IFACEQUEUE, primitive->initData.cid, srvcIds);
        }
    }
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */

#ifdef INSTALL_LEA_BROADCAST_SOURCE
    /* Broadcast Source Role handling */
    if( primitive->initData.role & BAP_ROLE_BROADCAST_SOURCE)
    {
        /* initialise Broadcast Source instance data structure */
        BapBigStream *bigStream;
        BapProfileHandle handle = primitive->initData.cid;
        bap->appPhandle = primitive->phandle;

        /* Broadcast Server only role */
        if( primitive->initData.cid == INVALID_CONNID )
        {
            handle = bapBroadcastSrcGetHandle();

            if( !handle )
            {
                bapUtilsSendBapInitCfm(primitive->phandle,
                                       handle,
                                       BAP_RESULT_INSUFFICIENT_RESOURCES,
                                       primitive->initData.role);
                return;
            }
        }

        bigStream = bapBigStreamNew(bap, primitive->phandle, handle);

        if (bigStream != NULL)
        {
            bapClientListPush(&bap->bigStreamList, &bigStream->listElement);
        }

        if( primitive->initData.cid == INVALID_CONNID )
        {
            bapUtilsSendBapInitCfm(primitive->phandle,
                                   handle,
                                   BAP_RESULT_SUCCESS,
                                   primitive->initData.role);
        }
    }
#endif /* INSTALL_LEA_BROADCAST_SOURCE */

    if( !(primitive->initData.role & (BAP_ROLE_UNICAST_CLIENT |
                    BAP_ROLE_BROADCAST_SOURCE | BAP_ROLE_BROADCAST_ASSISTANT)))
    {
        bapUtilsSendBapInitCfm(primitive->phandle,
                               primitive->initData.cid,
                               BAP_RESULT_NOT_SUPPORTED,
                               primitive->initData.role);
    }
}

static void handleBapDeinitReq(BAP* const bap,
                               BapInternalDeinitReq* const primitive)
{
#ifdef INSTALL_LEA_BROADCAST_SOURCE
    CsrSchedQid rspPhandle = CSR_SCHED_QID_INVALID;
#endif

#ifdef INSTALL_LEA_UNICAST_CLIENT
    BapConnection* connection = NULL;

    if (bapClientFindConnectionByCid(bap, primitive->handle, &connection))
    {
        if( primitive->role & BAP_ROLE_UNICAST_CLIENT)
        {
            if(connection->ascs.srvcHndl)
            {
                /* Remove the ASCS Client instance */
                connection->numService++;
                GattAscsClientTerminate(connection->ascs.srvcHndl);
            }

            if(connection->pacs.srvcHndl)
            {
                /* Remove the PACS Client instance */
                connection->numService++;
                GattPacsClientTerminateReq(connection->pacs.srvcHndl);
            }
        }

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
        if( primitive->role & BAP_ROLE_BROADCAST_ASSISTANT)
        {
            bapBroadcastAssistantDeinit(connection);
        }
#endif

    }
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */

#ifdef INSTALL_LEA_BROADCAST_SOURCE
    if( primitive->role & BAP_ROLE_BROADCAST_SOURCE)
    {
        rspPhandle = bap->appPhandle;
        if(( primitive->role & BAP_ROLE_BROADCAST_SOURCE) == BAP_ROLE_BROADCAST_SOURCE)
            bapBroadcastSrcResetHandle(primitive->handle);
        bapBigStreamDelete(bap, primitive->handle);
        bapUtilsSendBapDeinitCfm(rspPhandle,
                                 primitive->handle,
                                 primitive->role,
                                 BAP_RESULT_SUCCESS, NULL);
    }
#endif

}

#ifdef INSTALL_LEA_UNICAST_CLIENT
static void handleBapAddPacRecordReq(BAP* const bap,
                                     BapInternalAddPacRecordReq* const primitive)
{
    Status status;
    BapAddPacRecordCfm* cfmPrim;
    uint16 pacRecordId = INVALID_PAC_RECORD_ID;

    if (primitive && primitive->pacRecord)
    {
        status = bapClientAddPacRecord(bap,
                                       GLOBAL_BAP_PROFILE_ID,
                                       primitive->pacRecord->recordType,
                                       &primitive->pacRecord->codecSubrecord,
                                       &pacRecordId);

        cfmPrim = CsrPmemZalloc(sizeof(BapAddPacRecordCfm));
        cfmPrim->type = BAP_ADD_PAC_RECORD_CFM;
        cfmPrim->result = (status == STATUS_SUCCESS) ? BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;
        cfmPrim->pacRecordId = pacRecordId;
        cfmPrim->recordType = primitive->pacRecord->recordType;

        putMessageSynergy(PHANDLE_TO_QUEUEID(primitive->phandle), BAP_PRIM, cfmPrim);

        CsrPmemFree(primitive->pacRecord);
    }
}

static void handleBapRemovePacRecordReq(BAP* const bap,
                                        BapInternalRemovePacRecordReq* const primitive)
{
    Status status;
    BapRemovePacRecordCfm* cfmPrim;

    /* Remove the pac record based on pacRecordId*/

    status = bapClientRemovePacRecord(bap,
                                      GLOBAL_BAP_PROFILE_ID,
                                      primitive->pacRecordId);

    cfmPrim = CsrPmemZalloc(sizeof(BapRemovePacRecordCfm));
    cfmPrim->type = BAP_REMOVE_PAC_RECORD_CFM;
    cfmPrim->result = (status == STATUS_SUCCESS) ? BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;
    cfmPrim->pacRecordId = primitive->pacRecordId;

    putMessageSynergy(PHANDLE_TO_QUEUEID(primitive->phandle), BAP_PRIM, cfmPrim);
}


static void handleBapDiscoverAudioRoleReq(BAP* const bap,
                                          BapInternalDiscoverAudioRoleReq* const primitive)
{
    bool status = FALSE;
    BapConnection* connection = NULL;
    BapDiscoverAudioRoleCfm* cfmPrim;
    GattPacsClientType direction = (primitive->recordType == BAP_AUDIO_SINK_RECORD) ?
        GATT_PACS_CLIENT_SINK : GATT_PACS_CLIENT_SOURCE;

    if (bapClientFindConnectionByCid(bap, primitive->handle, &connection))
    {
        status = GattPacsClientFindAudioRoleReq(connection->pacs.srvcHndl, direction);

        cfmPrim = CsrPmemZalloc(sizeof(BapDiscoverAudioRoleCfm));
        cfmPrim->type = BAP_DISCOVER_AUDIO_ROLE_CFM;
        cfmPrim->result = status ? BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;
        cfmPrim->handle = primitive->handle;
        cfmPrim->recordType = primitive->recordType;

        putMessageSynergy(connection->rspPhandle, BAP_PRIM, cfmPrim);
    }
}


Status bapClientAddCodecSubrecordRecord(BAP * const bap,
                                        uint16 pacRecordId,
                                        BapServerDirection serverDirection,
                                        BapCodecSubRecord * const codecSubrecord)
{
    (void)bap;
    (void)pacRecordId;
    (void)serverDirection;
    (void)codecSubrecord;

    return STATUS_ERROR; /* return ERROR until this function is filled in */
}

void bapClientSetAdvData(BAP * const bap,
                         uint8 advDataLen,
                         uint8 * const advData)
{
    (void)bap;

    dm_hci_ulp_set_advertising_data_req(advDataLen, advData, NULL);
}

Bool bapClientFindProfileById(BAP * const bap,
                              uint16 profileId,
                              struct BapProfile ** const profile)
{
    BapClientListElement *listElement;

    for (listElement = bapClientListPeekFront(&bap->profileList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
         *profile = CONTAINER_CAST(listElement, BapProfile, listElement);

         if ((*profile)->id == profileId)
         {
             return TRUE;
         }
    }
    return FALSE;
}

Bool bapClientFindConnectionByCid(BAP * const bap,
                                  BapProfileHandle cid,
                                  struct BapConnection ** const connection)
{
    BapClientListElement *listElement;

    for (listElement = bapClientListPeekFront(&bap->connectionList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
         *connection = CONTAINER_CAST(listElement, BapConnection, listElement);
         if ((*connection)->cid == cid)
         {
             return TRUE;
         }
    }

    return FALSE;
}

static Bool bapClientRemoveConnectionByCid(BAP * const bap,
                                           BapProfileHandle cid)
{
    BapClientListElement *listElement;
    BapConnection * connection = NULL;

    for (listElement = bapClientListPeekFront(&bap->connectionList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
         connection = CONTAINER_CAST(listElement, BapConnection, listElement);
         if (connection && connection->cid == cid)
         {
             bapClientListRemoveIf(&connection->cisList, connListElement,
                                   BapCis, cis, ((cis->serverIsSinkAse ) ||
                                   (cis->serverIsSourceAse )),
                                   bapCisDelete(cis));
             bapClientListRemove(&bap->connectionList, listElement);
             bapConnectionDelete(connection);
             return TRUE;
         }
    }

    return FALSE;
}

Bool bapClientFindConnectionByPacsSrvcHndl(BAP* const bap,
                                           ServiceHandle pacsSrvcHndl,
                                           struct BapConnection** const connection)
{
    BapClientListElement* listElement;

    for (listElement = bapClientListPeekFront(&bap->connectionList);
        listElement != NULL;
        listElement = bapClientListElementGetNext(listElement))
    {
        *connection = CONTAINER_CAST(listElement, BapConnection, listElement);

        if ((*connection)->pacs.srvcHndl == pacsSrvcHndl)
        {
            return TRUE;
        }
    }
    return FALSE;
}

Bool bapClientFindConnectionByAscsSrvcHndl(BAP* const bap,
                                           ServiceHandle ascsSrvcHndl,
                                           struct BapConnection** const connection)
{
    BapClientListElement* listElement;

    for (listElement = bapClientListPeekFront(&bap->connectionList);
        listElement != NULL;
        listElement = bapClientListElementGetNext(listElement))
    {
        *connection = CONTAINER_CAST(listElement, BapConnection, listElement);

        if ((*connection)->ascs.srvcHndl == ascsSrvcHndl)
        {
            return TRUE;
        }
    }
    return FALSE;
}


Bool bapClientFindConnectionByTypedBdAddr(BAP * const bap,
                                          TYPED_BD_ADDR_T * const peerAddrt,
                                          struct BapConnection ** const connection)
{
    BapClientListElement *listElement;

    for (listElement = bapClientListPeekFront(&bap->connectionList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
         *connection = CONTAINER_CAST(listElement, BapConnection, listElement);

         if (tbdaddr_eq(&((*connection)->peerAddrt), peerAddrt))
         {
             return TRUE;
         }
    }
    return FALSE;
}

void bapClientAddStreamGroup(BAP * const bap, struct BapStreamGroup * const streamGroup)
{
    bapClientListPush(&bap->streamGroupList, &streamGroup->listElement);
}

Bool bapClientFindStreamGroupById(BAP * const bap,
                                  BapProfileHandle streamGroupId,
                                  struct BapStreamGroup ** const streamGroup)
{
    BapClientListElement *listElement;

    for (listElement = bapClientListPeekFront(&bap->streamGroupList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
         *streamGroup = CONTAINER_CAST(listElement, BapStreamGroup, listElement);

         if ((*streamGroup)->id == streamGroupId)
         {
             return TRUE;
         }
    }
    return FALSE;
}

Bool bapClientFindStreamGroupByCigId(BAP * const bap,
                                     uint8 cigId,
                                     struct BapStreamGroup ** const streamGroup)
{
    bapClientListFindIf(&bap->streamGroupList,
                        listElement,
                        BapStreamGroup,
                        *streamGroup,
                        (*streamGroup)->cigId == cigId);

    return (*streamGroup)? TRUE : FALSE;
}

static Bool bapClientRemoveStreamGroupById(BAP * const bap,
                                           BapProfileHandle streamGroupId)
{
    BapClientListElement *listElement;
    BapStreamGroup *streamGroup = NULL;

    for (listElement = bapClientListPeekFront(&bap->streamGroupList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
         streamGroup = CONTAINER_CAST(listElement, BapStreamGroup, listElement);

         if (streamGroup && streamGroup->id == streamGroupId)
         {
             bapClientListRemove(&bap->streamGroupList, listElement);
             bapStreamGroupDelete(streamGroup);
             return TRUE;
         }
    }
    return FALSE;
}

static Bool bapClientFindStreamGroupByCisHandle(BAP * const bap,
                                                uint16 cisHandle,
                                                struct BapStreamGroup ** const streamGroup)
{
    BapCis* cis = NULL;

    /*
     * Find a stream group that contains a CIS with the specified cis_handle.
     * Note: This function assumes that cis_handles are unique across all
     *       stream_groups (CIGs). If more than one stream_group has a CIS
     *       with a cis handle equal to cis_handle, then the first matching
     *       stream_group is returned.
     * Note: bapStreamGroupFindCisByCisHandle() returns either:
     *           * NULL (if the stream group doesn't have a CIS with the specified cis_handle)
     *           * A pointer to a CIS that _does_ have a cis handle equal to cis_handle
     */
    bapClientListFindIf(&bap->streamGroupList,
                        listElement,
                        BapStreamGroup,
                        *streamGroup,
                        (cis = bapStreamGroupFindCisByCisHandle(*streamGroup, cisHandle)) != NULL); /* Predicate: find_if exits if this is TRUE */

    /*
     * If we've found a cis with a cis handle equal to cis_handle, then we've also found the stream_group
     * that the cis is in.
     */
    return (cis)? TRUE : FALSE;
}

static BapAse* bapClientFindAseByCisHandle(BAP * const bap,
                                           uint16 cisHandle)
{
    BapStreamGroup *streamGroup = NULL;

    if (bapClientFindStreamGroupByCisHandle(bap,
                                            cisHandle,
                                            &streamGroup))
    {
        BapConnection *connection = NULL;

        if (bapClientFindConnectionByCid(bap, streamGroup->id, &connection))
        {
            BapAse *ase = NULL;
            ase = bapConnectionFindAseByCisHandle(connection, cisHandle);
            return ase;
        }
    }
    return NULL;
}

static void bapClientAseResetCisHandle(BAP * const bap, uint16 cisHandle)
{
    BapStreamGroup *streamGroup = NULL;

    if (bapClientFindStreamGroupByCisHandle(bap, cisHandle, &streamGroup))
    {
        BapConnection *connection = NULL;

        if (bapClientFindConnectionByCid(bap, streamGroup->id, &connection))
        {
            bapConnectionAseResetCisHandle(connection, cisHandle);
        }
    }
}

/* Reset all Sink and Source ASES associated with the CIS Handle */
static void bapConnectionAseResetCisHandle(BapConnection * const connection,
                                         uint16 cisHandle)
{
    BapClientListElement* listElement;

    for (listElement = bapClientListPeekFront(&connection->cisList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
        BapCis* cis = CONTAINER_CAST(listElement, BapCis, connListElement);
        if(cis != NULL)
        {
            if (cis->serverIsSinkAse && 
                (bapAseGetCisHandle(cis->serverIsSinkAse) == cisHandle))
                cis->serverIsSinkAse->cis->cisHandle = INVALID_CIS_HANDLE;
            if (cis->serverIsSourceAse && 
                (bapAseGetCisHandle(cis->serverIsSourceAse) == cisHandle))
                cis->serverIsSourceAse->cis->cisHandle = INVALID_CIS_HANDLE;
        }
    }
}

void bapClientDestroyList(BAP * const bap)
{
    /* Call 'bapConnectionDestroy()' on each element in the 'connection_list' */
    bapClientListForeach(&bap->connectionList,
                         listElement,
                         BapConnection,
                         connection,
                         bapConnectionDelete(connection));

    /* Call 'bapProfileDelete()' on each element in the 'profile_list' */
    bapClientListForeach(&bap->profileList,
                         listElement,
                         BapProfile,
                         profile,
                         bapProfileDelete(profile));

    bapClientListForeach(&bap->streamGroupList,
                         listElement,
                         BapStreamGroup,
                         streamGroup,
                         bapStreamGroupDelete(streamGroup));
}

void bapClientSendDeinitCfmSuccess(BAP * const bap,
                                   struct BapConnection *const connection)
{
    BapHandles handle;
    CsrSchedQid rspHandle = connection->rspPhandle;
    BapProfileHandle cid = connection->cid;
    BapRole role = connection->role;
    BapResult bapInitStatus = connection->bapInitStatus;

    CsrMemCpy(&handle, &(connection->handles), sizeof(handle));

    /* Destroy BAP connection instance */
    bapClientRemoveConnectionByCid(bap, cid);
    /* Destroy BAP Stream group */
    bapClientRemoveStreamGroupById(bap, cid);

    /* Send Deinit Cfm only if initiated as part of deinit req */
    if (bapInitStatus == BAP_RESULT_SUCCESS)
        bapUtilsSendBapDeinitCfm(rspHandle,
                                 cid,
                                 role,
                                 BAP_RESULT_SUCCESS,
                                 &handle);
}

static void handleAscsPrimitive(BAP * const bap, uint16 primitiveId, void *primitive)
{

    GattAscsServiceMessageId *prim = (GattAscsServiceMessageId *)primitive;

    switch (*prim)
    {
        case GATT_ASCS_CLIENT_INIT_CFM:
        {
            GattAscsClientInitCfm *cfm = (GattAscsClientInitCfm *) primitive;
            bapClientGattInitCfm(bap, cfm);
        }
        break;

        case GATT_ASCS_CLIENT_READ_ASE_INFO_CFM:
        {
            GattAscsClientReadAseInfoCfm *cfm = (GattAscsClientReadAseInfoCfm *) primitive;
            bapClientGattHandleAsesReadCfm(bap, cfm);
        }
        break;

        case GATT_ASCS_CLIENT_READ_ASE_STATE_CFM:
        {
            GattAscsClientReadAseStateCfm *cfm = (GattAscsClientReadAseStateCfm *) primitive;
            bapClientGattHandleAsesReadStateCfm(bap, cfm);
        }
        break;

        case GATT_ASCS_CLIENT_INDICATION_IND:
        {
            GattAscsClientIndicationInd *cfm = (GattAscsClientIndicationInd *) primitive;
            bapClientGattHandleNotification(bap, cfm);
        }
        break;

        case GATT_ASCS_CLIENT_WRITE_ASE_CFM:
        {
            GattAscsClientWriteAseCfm *cfm = (GattAscsClientWriteAseCfm *) primitive;
            bapClientGattHandleAseWriteCfm(bap, cfm);
        }
        break;

        case GATT_ASCS_CLIENT_TERMINATE_CFM:
        {
            GattAscsClientTerminateCfm *ind = (GattAscsClientTerminateCfm*)primitive;
            BapConnection *connection = NULL;

            if(ind->status == GATT_ASCS_CLIENT_STATUS_SUCCESS)
            {
                if (bapClientFindConnectionByAscsSrvcHndl(bap, ind->clientHandle, &connection))
                {
                    connection->handles.ascsHandles = (BapAscsClientDeviceData*)
                                                CsrPmemZalloc(sizeof(BapAscsClientDeviceData));
                    CsrMemCpy(connection->handles.ascsHandles, &ind->deviceData, sizeof(BapAscsClientDeviceData));
                    connection->numService--;

                    if(connection->numService == 0)
                    {
                        bapClientSendDeinitCfmSuccess(bap, connection);
                    }
                }

            }
        }
        break;

        default:
        break;
    }

    CSR_UNUSED(primitiveId);
}


static void handlePacsPrimitive(BAP* const bap, uint16 primitiveId, void* primitive)
{

    GattPacsServiceMessageId* prim = (GattPacsServiceMessageId*)primitive;

    switch (*prim)
    {
        case GATT_PACS_CLIENT_INIT_CFM:
        {
            GattPacsClientInitCfm* cfm = (GattPacsClientInitCfm*)primitive;
            bapClientGattPacsClientInitCfm(bap, cfm);
        }
        break;

        case GATT_PACS_CLIENT_NOTIFICATION_CFM:
        {
            GattPacsClientNotificationCfm* cfm = (GattPacsClientNotificationCfm*)primitive;
            bapClientGattHandleRegisterPacsNotificationCfm(bap, cfm);
        }
        break;

        case GATT_PACS_CLIENT_READ_PAC_RECORD_CFM:
        {
            GattPacsClientReadPacRecordCfm* cfm = (GattPacsClientReadPacRecordCfm*)primitive;
            bapClientGattHandleReadPacRecordCfm(bap, cfm);
            if(cfm->record.value != NULL)
            {
                CsrPmemFree(cfm->record.value);
            }
        }
        break;

        case GATT_PACS_CLIENT_READ_AUDIO_LOCATION_CFM:
        {
            GattPacsClientReadAudioLocationCfm* cfm = (GattPacsClientReadAudioLocationCfm*)primitive;
            bapClientGattHandleReadAudioLocationCfm(bap, cfm);
        }
        break;

        case GATT_PACS_CLIENT_WRITE_AUDIO_LOCATION_CFM:
        {
            GattPacsClientWriteAudioLocationCfm* cfm = (GattPacsClientWriteAudioLocationCfm*)primitive;
            bapClientGattHandleWriteAudioLocationCfm(bap, cfm);
        }
        break;

        case GATT_PACS_CLIENT_READ_AUDIO_CONTEXT_CFM:
        {
            GattPacsClientReadAudioContextCfm* cfm = (GattPacsClientReadAudioContextCfm*)primitive;
            bapClientGattHandleReadAudioContextCfm(bap, cfm);
        }
        break;

        case GATT_PACS_CLIENT_PAC_RECORD_NOTIFICATION_IND:
        {
            GattPacsClientPacRecordNotificationInd* ind = (GattPacsClientPacRecordNotificationInd*)primitive;
            bapClientGattHandlePacRecordNotificationInd(bap, ind);
            if(ind ->record.value != NULL)
            {
                CsrPmemFree(ind ->record.value);
            }

        }
        break;

        case GATT_PACS_CLIENT_AUDIO_LOCATION_NOTIFICATION_IND:
        {
            GattPacsClientAudioLocationNotificationInd* ind = (GattPacsClientAudioLocationNotificationInd*)primitive;
            bapClientGattHandleAudioLocationNotificationInd(bap, ind);
        }
        break;

        case GATT_PACS_CLIENT_AUDIO_CONTEXT_NOTIFICATION_IND:
        {
            GattPacsClientAudioContextNotificationInd* ind = (GattPacsClientAudioContextNotificationInd*)primitive;
            bapClientGattHandleAudioContextNotificationInd(bap, ind);
        }
        break;

        case GATT_PACS_CLIENT_TERMINATE_CFM:
        {
            GattPacsClientTerminateCfm *ind = (GattPacsClientTerminateCfm*)primitive;
            BapConnection *connection = NULL;

            if(ind->status == GATT_PACS_CLIENT_STATUS_SUCCESS)
            {
                if (bapClientFindConnectionByCid(bap, ind->cid, &connection))
                {
                    connection->handles.pacsHandles = (BapPacsClientDeviceData*)
                                                     CsrPmemZalloc(sizeof(BapPacsClientDeviceData));
                    CsrMemCpy(connection->handles.pacsHandles, &ind->deviceData, sizeof(BapPacsClientDeviceData));
                    connection->numService--;

                    if(connection->numService == 0)
                    {
                        bapClientSendDeinitCfmSuccess(bap, connection);
                    }
                }

            }
        }
        break;

        default:
            break;
    }

    CSR_UNUSED(primitiveId);
}

static void bapPacsClientSrvcInit(BapConnection *const connection,
                                  CsrBtConnId cid,
                                  uint16 startHndl,
                                  uint16 endHndl,
                                  BapPacsClientDeviceData *data)
{
    GattPacsClientInitData params;

    if(connection == NULL)
        return;

    connection->pacs.pacsStartHandle = startHndl;
    connection->pacs.pacsEndHandle = endHndl;
    connection->pacs.srvcHndl = INVALID_SERVICE_HANDLE;
    connection->numService++;

    params.cid = cid;
    params.startHandle = startHndl;
    params.endHandle = endHndl;
    GattPacsClientInitReq(CSR_BT_BAP_IFACEQUEUE, &params, data);
}

static void bapAscsClientSrvcInit(BapConnection *const connection,
                                  CsrBtConnId cid,
                                  uint16 startHndl,
                                  uint16 endHndl,
                                  BapAscsClientDeviceData  *data)
{
    GattAscsClientInitParams params;

    if(connection == NULL)
        return;

    connection->ascs.ascsStartHandle = startHndl;
    connection->ascs.ascsStartHandle = endHndl;
    connection->ascs.srvcHndl = INVALID_SERVICE_HANDLE;
    connection->numService++;

    params.cid = cid;
    params.startHandle = startHndl;
    params.endHandle = endHndl;

    GattAscsClientInit(CSR_BT_BAP_IFACEQUEUE, &params, data);
}

static void gattBapClientSrvcInit(BAP * const bap,
                                  CsrBtConnId cid,
                                  GattSdSrvcId srvcId,
                                  uint16 startHndl,
                                  uint16 endHndl)
{
    BapConnection *connection = NULL;

    BAP_CLIENT_DEBUG("(BAP) : gattBapClientSrvcInit cid %d, Service UUID 0x%x, startHndl 0x%x, endHndl 0x%x\n",
            cid, srvcId, startHndl, endHndl);

    if (bapClientFindConnectionByCid(bap, cid, &connection))
    {
        switch (srvcId)
        {
            case GATT_SD_ASCS_SRVC:
            {
                bapAscsClientSrvcInit(connection, cid, startHndl, endHndl,NULL);
            }
            break;
            case GATT_SD_PACS_SRVC:
            {
                bapPacsClientSrvcInit(connection, cid, startHndl, endHndl, NULL);
            }
            break;
#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
            case GATT_SD_BASS_SRVC:
            {
                bapBassClientSrvcInit(connection, cid, startHndl, endHndl, NULL);
            }
            break;
#endif
            default:
                 BAP_CLIENT_DEBUG("(BAP) gattBapClientSrvcInit default srvcId 0x%x \n\n", srvcId);
            break;
        }
    }
}

static void handleGattSrvcDiscPrimitive(BAP * const bap, uint16 primitiveId, void *primitive)
{
    GattSdPrim *prim = (GattSdPrim *) primitive;

    switch (*prim)
    {
        case GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM:
        {
            GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *cfm =
                (GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *) primitive;
            if (cfm->result == GATT_SD_RESULT_SUCCESS)
            {
                uint8 i = 0;
                for (i = 0; i < cfm->srvcInfoCount; i++)
                {
                    BAP_CLIENT_DEBUG("(BAP) : Start Hndl = 0x%x, End Hndl = 0x%x, Id = 0x%x\n",
                        cfm->srvcInfo[i].startHandle, cfm->srvcInfo[i].endHandle, cfm->srvcInfo[i].srvcId);
                    gattBapClientSrvcInit(bap, cfm->cid,
                                    cfm->srvcInfo[i].srvcId,
                                    cfm->srvcInfo[i].startHandle,
                                    cfm->srvcInfo[i].endHandle);
                }
            }
            else
            {
                /* NOTE:- This is a temporary change to handle application
                 * which does not use GATT SD lib for primary service
                 * discovery */
                CsrBtGattDiscoverAllPrimaryServicesReqSend(bap->gattId,
                                                            cfm->cid);
            }

            if (cfm->srvcInfoCount && cfm->srvcInfo)
                CsrPmemFree(cfm->srvcInfo);

            break;
        }
    }

    CSR_UNUSED(primitiveId);
}
#endif

void bapClientFreePrimitive(BapUPrim * const prim)
{
    if (prim == NULL)
    {
        return;
    }

    switch (prim->type)
    {
        case BAP_INTERNAL_UNICAST_CLIENT_CODEC_CONFIGURE_REQ:
        case BAP_INTERNAL_UNICAST_CLIENT_QOS_CONFIGURE_REQ:
        case BAP_INTERNAL_UNICAST_CLIENT_ENABLE_REQ:
        case BAP_INTERNAL_UNICAST_CLIENT_DISABLE_REQ:
        case BAP_INTERNAL_UNICAST_CLIENT_RELEASE_REQ:
        case BAP_INTERNAL_UNICAST_CLIENT_CIG_CONFIGURE_REQ:
        case BAP_INTERNAL_UNICAST_CLIENT_CIG_TEST_CONFIGURE_REQ:
        case BAP_INTERNAL_SETUP_DATA_PATH_REQ:
        case BAP_INTERNAL_UNICAST_CLIENT_CIS_CONNECT_REQ:
        case BAP_INTERNAL_UNICAST_CLIENT_RECEIVER_READY_REQ:
        case BAP_INTERNAL_UNICAST_CLIENT_UPDATE_METADATA_REQ:
        case BAP_INTERNAL_BROADCAST_SRC_ENABLE_STREAM_REQ:
        case BAP_INTERNAL_BROADCAST_SRC_UPDATE_METADATA_REQ:
        {
            freeDownstreamPrimitive(prim);
        }
        break;

#ifdef INSTALL_LEA_UNICAST_CLIENT
        case BAP_UNICAST_CLIENT_READ_ASE_INFO_CFM:
        {
            freeUpstreamPrimitive(prim);
        }
        break;
#endif
        default:
            break;
    }
}

void bapClientHandler(void **gash)
{
    BAP *bap = *((BAP **)gash);
    void *primitive;
    uint16 primitiveId;
    if (CsrSchedMessageGet(&primitiveId, &primitive))
    {
        if( primitiveId == CSR_BT_CM_PRIM )
        {
            handleUpstreamCmPrimitive(bap, (CsrBtCmPrim *)primitive);
        }
#ifdef INSTALL_LEA_UNICAST_CLIENT
        else if( primitiveId == GATT_SRVC_DISC_PRIM)
        {
            handleGattSrvcDiscPrimitive(bap, primitiveId, primitive);
        }
        else if( primitiveId == ASCS_CLIENT_PRIM)
        {
            handleAscsPrimitive(bap, primitiveId, primitive);
        }
        else if (primitiveId == PACS_CLIENT_PRIM)
        {
            handlePacsPrimitive(bap, primitiveId, primitive);
        }
        else if(primitiveId == CSR_BT_GATT_PRIM)
        {
            bapGattMessageHandler(bap, primitiveId, primitive);
        }
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
        else if( primitiveId == BAP_PRIM )
        {
            handleDownstreamPrimitive(bap, (BapUPrim *)primitive);
        }
#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
        else if (primitiveId == BASS_CLIENT_PRIM)
        {
            handleBassPrimitive(bap, primitiveId, primitive);
        }
#endif
        SynergyMessageFree(primitiveId, primitive);
    }
    primitive = NULL;
}

void bapClientInit(void **gash)
{

    CsrUint16 isocType = DM_ISOC_TYPE_UNICAST;

    inst  = CsrPmemZalloc(sizeof(*inst));
    *gash = inst;

    bapClientInitialiseList((BAP *)(*gash));
    CsrBtGattRegisterReqSend(CSR_BT_BAP_IFACEQUEUE, 0);

#ifdef INSTALL_LEA_BROADCAST_SOURCE
    isocType |= DM_ISOC_TYPE_BROADCAST;
#endif
    CmIsocRegisterReqSend(CSR_BT_BAP_IFACEQUEUE, isocType);
}

#ifdef ENABLE_SHUTDOWN
void bapClientDeinit(void **gash)
{
    BAP *bap;

    bap = *((BAP **)gash);

    bapClientDestroyList(bap);

    CsrPmemFree(bap);
}
#endif

static void handleUpstreamCmPrimitive(BAP * const bap, CsrBtCmPrim * const primitive)
{
    (void)bap;
    BAP_CLIENT_INFO("BAP Client: handleUpstreamCmPrimitive primitive 0x%x \n", *primitive);


    switch (*primitive)
    {
#ifdef INSTALL_LEA_UNICAST_CLIENT
        case CSR_BT_CM_ISOC_REGISTER_CFM:
        {
            CmIsocRegisterCfm *prim = (CmIsocRegisterCfm *) primitive;
            if(prim->resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                BAP_CLIENT_DEBUG(" CSR_BT_CM_ISOC_REGISTER_CFM FAILED \n");
            }
        }
        break;

        case CSR_BT_CM_ISOC_CONFIGURE_CIG_CFM:
        {
            CmIsocConfigureCigCfm *prim = (CmIsocConfigureCigCfm *) primitive;
            /* send to application */
            bapUtilsSendCigConfigureCfm(bap->appPhandle, prim);
        }
        break;

        case CSR_BT_CM_ISOC_CONFIGURE_CIG_TEST_CFM:
        {
            CmIsocConfigureCigTestCfm *prim = (CmIsocConfigureCigTestCfm *) primitive;
            /* send to application */
            bapUtilsSendCigTestConfigureCfm(bap->appPhandle, prim);
        }
        break;

        case CSR_BT_CM_ISOC_CIS_CONNECT_CFM:
        {
            CmIsocCisConnectCfm *prim = (CmIsocCisConnectCfm *) primitive;
            BapConnection *connection = NULL;

            if(bapClientFindConnectionByTypedBdAddr(bap,
                                                    (TYPED_BD_ADDR_T *)&prim->tp_addr.addrt,
                                                    &connection))
            {
                BapAse* ase = NULL;
                BapClientAse* clientAse = NULL;

                /* Find the ase with the right cis_handle */
                ase = bapConnectionFindAseByCisHandle(connection, prim->cis_handle);
                if(ase)
                {
                    uint8 numAses = 0;
                    BapAse *aseIds[2];
                    uint8 i = 0;

                    /* check for Bidirectional cis and get the ASEs */
                    numAses = bapConnectionFindAseByCigAndCisId(connection,
                                                                ase->cis->cigId,
                                                                ase->cis->cisId,
                                                                ase->cis->cisHandle,
                                                                aseIds);

                    for( i = 0; i< numAses; i++)
                    {
                        clientAse = CONTAINER_CAST(aseIds[i], BapClientAse, ase);

                        if(clientAse)
                        {
                            bapStreamGroupAseStateCisConnectCfmReceived(clientAse->ase.streamGroup,
								                                        clientAse,
								                                        prim,
								                                        i+1,
								                                        numAses);
                        }
                    }
                }
            }
        }
        break;

        case CSR_BT_CM_ISOC_CIS_DISCONNECT_CFM:
        {
            CmIsocCisDisconnectCfm *prim = (CmIsocCisDisconnectCfm *) primitive;
            BapResult  result = BAP_RESULT_SUCCESS;
            BapAse *ase = NULL;

            /* send to application */
            if(prim->resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                result = BAP_RESULT_ERROR;
            }

            ase = bapClientFindAseByCisHandle(bap, prim->cis_handle);
            if (ase != NULL )
            {
                ase->cis->cisHandle = INVALID_CIS_HANDLE;
                bapClientAseResetCisHandle(bap, prim->cis_handle);

                bapUtilsSendCisDisconnectCfm(ase->streamGroup->rspPhandle,
                                             result,
                                             ase->streamGroup->id,
                                             prim->cis_handle);
            }
        }
        break;

        case CSR_BT_CM_ISOC_CIS_DISCONNECT_IND:
        {
            CmIsocCisDisconnectInd *prim = (CmIsocCisDisconnectInd *) primitive;
            BapAse *ase = NULL;

            ase = bapClientFindAseByCisHandle(bap, prim->cis_handle);
            if (ase != NULL )
            {
                ase->cis->cisHandle = INVALID_CIS_HANDLE;
                bapClientAseResetCisHandle(bap, prim->cis_handle);
                
                bapUtilsSendCisDisconnectInd(ase->streamGroup->rspPhandle,
                                             (uint16)prim->reason,
                                             ase->streamGroup->id,
                                             prim->cis_handle);
            }
        }
        break;

        case CSR_BT_CM_ISOC_REMOVE_CIG_CFM:
        {
            CmIsocRemoveCigCfm *prim = (CmIsocRemoveCigCfm *) primitive;
                /* send to application */
            bapUtilsSendRemoveCigCfm(bap->appPhandle, prim);
        }
        break;
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */

        case CSR_BT_CM_ISOC_SETUP_ISO_DATA_PATH_CFM:
        {
            CmIsocSetupIsoDataPathCfm *prim = (CmIsocSetupIsoDataPathCfm *) primitive;
            BapResult result = BAP_RESULT_SUCCESS;
#ifdef INSTALL_LEA_UNICAST_CLIENT
            BapAse* ase = NULL;
#endif
            BapProfileHandle streamGroupId = 0;
#ifdef INSTALL_LEA_BROADCAST_SOURCE
            BapBigStream* big = NULL;
#endif 

#ifdef INSTALL_LEA_UNICAST_CLIENT
            ase = bapClientFindAseByCisHandle(bap, prim->handle);

            if (ase)
            {
                streamGroupId = ase->streamGroup->id;
            }
            else
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#ifdef INSTALL_LEA_BROADCAST_SOURCE            
            {
                if (bapBroadcastSrcFindStreamByBisHandle(bap, prim->handle, &big) && big)
                    streamGroupId = big->profilePhandle;
            }
#endif

            if(prim->resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS)
                result = BAP_RESULT_ERROR;

            /* send to application */
            bapUtilsSendSetupDatapathCfm(bap->appPhandle,
                                         prim->handle,
                                         result,
                                         streamGroupId);
        }
        break;

        case CSR_BT_CM_ISOC_REMOVE_ISO_DATA_PATH_CFM:
        {
            CmIsocRemoveIsoDataPathCfm *prim = (CmIsocRemoveIsoDataPathCfm *) primitive;
            BapResult result = BAP_RESULT_SUCCESS;

            if(prim->resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS)
                result = BAP_RESULT_ERROR;

            /* send to application */
            bapUtilsSendRemoveDatapathCfm(bap->appPhandle,
                                          prim->handle,
                                          result);
        }
        break;

#ifdef INSTALL_LEA_BROADCAST_SOURCE
        case CSR_BT_CM_EXT_ADV_REGISTER_APP_ADV_SET_CFM:
        case CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM:
        case CSR_BT_CM_EXT_ADV_SET_PARAMS_CFM:
        case CM_DM_EXT_ADV_SET_PARAMS_V2_CFM:
        case CSR_BT_CM_EXT_ADV_SET_DATA_CFM:
        case CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_CFM:
        case CSR_BT_CM_EXT_ADV_ENABLE_CFM:
        case CSR_BT_CM_PERIODIC_ADV_SET_PARAMS_CFM:
        case CSR_BT_CM_PERIODIC_ADV_SET_DATA_CFM:
        case CSR_BT_CM_PERIODIC_ADV_READ_MAX_ADV_DATA_LEN_CFM:
        case CSR_BT_CM_PERIODIC_ADV_START_CFM:
        case CSR_BT_CM_PERIODIC_ADV_STOP_CFM:
        case CSR_BT_CM_ISOC_CREATE_BIG_CFM:
        case CSR_BT_CM_ISOC_TERMINATE_BIG_CFM:
        case CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_CFM:
        case CSR_BT_CM_ISOC_CREATE_BIG_TEST_CFM:
        case CSR_BT_CM_EXT_ADV_SETS_INFO_CFM:
        {
            bapBroadcastSrcStreamCmPrimitive(bap, primitive);
        }
        break;
#endif /* INSTALL_LEA_BROADCAST_SOURCE */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
        case CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_CFM:
        case CSR_BT_CM_PERIODIC_ADV_SET_TRANSFER_CFM:
        case CSR_BT_CM_PERIODIC_SCAN_START_FIND_TRAINS_CFM:
        case CSR_BT_CM_PERIODIC_SCAN_STOP_FIND_TRAINS_CFM:
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM:
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL_CFM:
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_ENABLE_CFM:
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TERMINATE_CFM:
        case CSR_BT_CM_EXT_SCAN_FILTERED_ADV_REPORT_IND:
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_IND:
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_LOST_IND:
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_CFM:
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_IND:
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_CFM:
        case CSR_BT_CM_EXT_SCAN_CTRL_SCAN_INFO_IND:
        {
            bapBroadcastAssistantCmPrimitiveHandler(bap, primitive);
        }
        break;
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

        default:
            BAP_CLIENT_DEBUG("Default CM handling prim %x\n", *primitive);

            break;
    }

    /*cm_free_primitive(primitive);*/
}

static void handleDownstreamPrimitive(BAP * const bap,
                                      BapUPrim * const primitive)
{
#ifdef INSTALL_LEA_UNICAST_CLIENT
    BapConnection *connection = NULL;
    TYPED_BD_ADDR_T *addrt = NULL;
    bool cidFound = FALSE;
    uint32 cid = INVALID_CONNID;
#endif

    switch (primitive->type)
    {
        case BAP_INTERNAL_INIT_REQ:
            handleBapInitReq(bap, &primitive->bapInternalInitReq);
            break;

        case BAP_INTERNAL_DEINIT_REQ:
            handleBapDeinitReq(bap, &primitive->bapInternalDeinitReq);
            break;

#ifdef INSTALL_LEA_UNICAST_CLIENT
        case BAP_INTERNAL_ADD_PAC_RECORD_REQ:
            handleBapAddPacRecordReq(bap, &primitive->bapInternalAddPacRecordReq);
            break;

        case BAP_INTERNAL_REMOVE_PAC_RECORD_REQ:
            handleBapRemovePacRecordReq(bap, &primitive->bapInternalRemovePacRecordReq);
            break;

        case BAP_INTERNAL_DISCOVER_AUDIO_ROLE_REQ:
            handleBapDiscoverAudioRoleReq(bap, &primitive->bapInternalDiscoverAudioRoleReq);
            break;

        case BAP_INTERNAL_DISCOVER_REMOTE_AUDIO_CAPABILITY_REQ:
            handleBapDiscoverRemoteAudioCapabilityReq(bap, &primitive->bapInternalDiscoverRemoteAudioCapabilityReq);
            break;

        case BAP_INTERNAL_REGISTER_PACS_NOTIFICATION_REQ:
            handleBapRegisterPacsNotificationReq(bap, &primitive->bapInternalRegisterPacsNotificationReq);
            break;

        case BAP_INTERNAL_GET_REMOTE_AUDIO_LOCATION_REQ:
            handleBapGetRemoteAudioLocationReq(bap, &primitive->bapInternalGetRemoteAudioLocationReq);
            break;

        case BAP_INTERNAL_SET_REMOTE_AUDIO_LOCATION_REQ:
            handleBapSetRemoteAudioLocationReq(bap, &primitive->bapInternalSetRemoteAudioLocationReq);
            break;

        case BAP_INTERNAL_DISCOVER_AUDIO_CONTEXT_REQ:
            handleBapDiscoverAudioContextReq(bap, &primitive->bapInternalDiscoverAudioContextReq);
            break;

        case BAP_INTERNAL_UNICAST_CLIENT_REGISTER_ASE_NOTIFICATION_REQ:
            cidFound = TRUE;
            cid = primitive->bapInternalUnicastClientRegisterAseNotificationReq.handle;
            break;

        case BAP_INTERNAL_UNICAST_CLIENT_READ_ASE_INFO_REQ:
            cidFound = TRUE;
            cid = primitive->bapInternalUnicastClientReadAseInfoReq.handle;
            break;

       case BAP_INTERNAL_UNICAST_CLIENT_CODEC_CONFIGURE_REQ:
            handleStreamGroupCodecConfigureReq(bap, &primitive->bapInternalUnicastClientCodecConfigureReq);
            break;

        case BAP_INTERNAL_UNICAST_CLIENT_CIG_CONFIGURE_REQ:
            handleBapUnicastClientCigConfigureReq(bap, &primitive->bapInternalUnicastClientCigConfigureReq);
            break;

        case BAP_INTERNAL_UNICAST_CLIENT_CIG_TEST_CONFIGURE_REQ:
            handleBapUnicastClientCigTestConfigureReq(bap, &primitive->bapInternalUnicastClientCigTestConfigureReq);
            break;

        case BAP_INTERNAL_UNICAST_CLIENT_CIG_REMOVE_REQ:
            handleBapUnicastClientRemoveCigReq(bap, &primitive->bapInternalUnicastClientCigRemoveReq);
            break;

        case BAP_INTERNAL_UNICAST_CLIENT_QOS_CONFIGURE_REQ:
            handleStreamGroupQosConfigureReq(bap, &primitive->bapInternalUnicastClientQosConfigureReq);
            break;

        case BAP_INTERNAL_UNICAST_CLIENT_ENABLE_REQ:
            handleStreamGroupEnableReq(bap, &primitive->bapInternalUnicastClientEnableReq);
            break;

        case BAP_INTERNAL_UNICAST_CLIENT_CIS_CONNECT_REQ:
            handleStreamGroupCisConnectReq(bap, &primitive->bapInternalUnicastClientCisConnectReq);
            break;

        case BAP_INTERNAL_UNICAST_CLIENT_CIS_DISCONNECT_REQ:
            handleStreamGroupCisDisconnectReq(bap, &primitive->bapInternalUnicastClientCisDisconnectReq);
            break;
#endif
        case BAP_INTERNAL_SETUP_DATA_PATH_REQ:
            handleBapClientSetupDatapathReq(bap, &primitive->bapInternalSetupDataPathReq);
            break;

        case BAP_INTERNAL_REMOVE_DATA_PATH_REQ:
            handleBapClientRemoveDatapathReq(bap, &primitive->bapInternalRemoveDataPathReq);
            break;

#ifdef INSTALL_LEA_UNICAST_CLIENT
        case BAP_INTERNAL_UNICAST_CLIENT_DISABLE_REQ:
            handleStreamGroupDisableReq(bap, &primitive->bapInternalUnicastClientDisableReq);
            break;

        case BAP_INTERNAL_UNICAST_CLIENT_RELEASE_REQ:
            handleStreamGroupReleaseReq(bap, &primitive->bapInternalUnicastClientReleaseReq);
            break;

        case BAP_INTERNAL_UNICAST_CLIENT_UPDATE_METADATA_REQ:
            handleStreamGroupUpdateMetadataReq(bap, &primitive->bapInternalUnicastClientUpdateMetadataReq);
            break;

        case BAP_INTERNAL_UNICAST_CLIENT_RECEIVER_READY_REQ:
            handleStreamGroupReceiverReadyReq(bap, &primitive->bapInternalUnicastClientReceiverReadyReq);
            break;

        case BAP_INTERNAL_SET_CONTROL_POINT_OP_REQ:
             handleBapSetControlPointOpReq(bap, &primitive->bapInternalSetControlPointOpReq);
             break;
#endif

#ifdef INSTALL_LEA_BROADCAST_SOURCE
        case BAP_INTERNAL_BROADCAST_SRC_CONFIGURE_STREAM_REQ:
            bapBroadcastSrcStreamConfigureReqHandler(bap,&primitive->bapInternalBroadcastSrcConfigureStreamReq);
            break;
        case BAP_INTERNAL_BROADCAST_SRC_ENABLE_STREAM_REQ:
            bapBroadcastSrcStreamEnableReqHandler(bap,&primitive->bapInternalBroadcastSrcEnableStreamReq);
            break;
        case BAP_INTERNAL_BROADCAST_SRC_ENABLE_STREAM_TEST_REQ:
            bapBroadcastSrcStreamEnableTestReqHandler(bap,&primitive->bapInternalBroadcastSrcEnableStreamTestReq);
            break;
        case BAP_INTERNAL_BROADCAST_SRC_DISABLE_STREAM_REQ:
            bapBroadcastSrcStreamDisableReqHandler(bap,&primitive->bapInternalBroadcastSrcDisableStreamReq);
            break;
        case BAP_INTERNAL_BROADCAST_SRC_RELEASE_STREAM_REQ:
            bapBroadcastSrcStreamReleaseReqHandler(bap,&primitive->bapInternalBroadcastSrcReleaseStreamReq);
            break;
        case BAP_INTERNAL_BROADCAST_SRC_UPDATE_METADATA_REQ:
            bapBroadcastSrcStreamMetadataReqHandler(bap,&primitive->bapInternalBroadcastSrcUpdateMetadataStreamReq);
            break;
        case BAP_INTERNAL_BROADCAST_SRC_SET_BROADCAST_ID:
        {
            BapInternalBroadcastSrcSetBroadcastId *msg = (BapInternalBroadcastSrcSetBroadcastId *)primitive;
            bapBroadcastSrcSetBroadcastId(bap, msg->handle, msg->broadcastId);
        }
            break;

#endif /* INSTALL_LEA_BROADCAST_SOURCE */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
        case BAP_INTERNAL_BROADCAST_ASSISTANT_START_SCAN_REQ:
        case BAP_INTERNAL_BROADCAST_ASSISTANT_STOP_SCAN_REQ:
        case BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_START_REQ:
        case BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_REQ:
        case BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_TERMINATE_REQ:
        case BAP_INTERNAL_BROADCAST_ASSISTANT_BRS_REGISTER_FOR_NOTIFICATION_REQ:
        case BAP_INTERNAL_BROADCAST_ASSISTANT_READ_BRS_CCC_REQ:
        case BAP_INTERNAL_BROADCAST_ASSISTANT_READ_BRS_REQ:
        case BAP_INTERNAL_BROADCAST_ASSISTANT_ADD_SRC_REQ:
        case BAP_INTERNAL_BROADCAST_ASSISTANT_MODIFY_SRC_REQ:
        case BAP_INTERNAL_BROADCAST_ASSISTANT_REMOVE_SRC_REQ:
        case BAP_INTERNAL_BROADCAST_ASSISTANT_SET_CODE_RSP:
            bapBroadcastAssistantInternalMsgHandler(bap, (BapUPrim *)primitive);
            break;
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */
        default:
            break;
    }

#ifdef INSTALL_LEA_UNICAST_CLIENT
    if ((addrt != NULL) && bapClientFindConnectionByTypedBdAddr(bap, addrt, &connection))
    {
        bapConnectionRcvBapPrimitive(connection, primitive);
    }
    else if (cidFound && bapClientFindConnectionByCid(bap, cid, &connection))
    {
        bapConnectionRcvBapPrimitive(connection, primitive);
    }
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
    bapClientFreePrimitive(primitive);
}

static void freeDownstreamPrimitive(BapUPrim * const prim)
{
    uint8 i;

    if (prim == NULL)
    {
        return;
    }

    /* Action taken depends on the primitive type */
    switch (prim->type)
    {
        case BAP_INTERNAL_UNICAST_CLIENT_CODEC_CONFIGURE_REQ:
        {
            for (i = 0; i < prim->bapInternalUnicastClientCodecConfigureReq.numAseCodecConfigurations; ++i)
            {
                CsrPmemFree(prim->bapInternalUnicastClientCodecConfigureReq.aseCodecConfigurations[i]);
                prim->bapInternalUnicastClientCodecConfigureReq.aseCodecConfigurations[i] = NULL;
            }
        }
        break;

        case BAP_INTERNAL_UNICAST_CLIENT_QOS_CONFIGURE_REQ:
        {
            for (i = 0; i < prim->bapInternalUnicastClientQosConfigureReq.numAseQosConfigurations; ++i)
            {
                CsrPmemFree(prim->bapInternalUnicastClientQosConfigureReq.aseQosConfigurations[i]);
                prim->bapInternalUnicastClientQosConfigureReq.aseQosConfigurations[i] = NULL;
            }
        }
        break;
        case BAP_INTERNAL_UNICAST_CLIENT_ENABLE_REQ:
        {
            for (i = 0; i < prim->bapInternalUnicastClientEnableReq.numAseEnableParameters; ++i)
            {
                if ((prim->bapInternalUnicastClientEnableReq.aseEnableParameters[i]->metadata) && 
                    (prim->bapInternalUnicastClientEnableReq.aseEnableParameters[i]->metadataLen))
                {
                    CsrPmemFree(prim->bapInternalUnicastClientEnableReq.aseEnableParameters[i]->metadata);
                    prim->bapInternalUnicastClientEnableReq.aseEnableParameters[i]->metadata = NULL;
                }
                CsrPmemFree(prim->bapInternalUnicastClientEnableReq.aseEnableParameters[i]);
                prim->bapInternalUnicastClientEnableReq.aseEnableParameters[i] = NULL;
            }
        }
        break;
        case BAP_INTERNAL_UNICAST_CLIENT_UPDATE_METADATA_REQ:
        {
            for (i = 0; i < prim->bapInternalUnicastClientUpdateMetadataReq.numAseMetadataParameters; ++i)
            {
                if ((prim->bapInternalUnicastClientUpdateMetadataReq.aseMetadataParameters[i]->metadata) && 
                    (prim->bapInternalUnicastClientUpdateMetadataReq.aseMetadataParameters[i]->metadataLen))
                {
                    CsrPmemFree(prim->bapInternalUnicastClientUpdateMetadataReq.aseMetadataParameters[i]->metadata);
                    prim->bapInternalUnicastClientUpdateMetadataReq.aseMetadataParameters[i]->metadata = NULL;
                }
                CsrPmemFree(prim->bapInternalUnicastClientUpdateMetadataReq.aseMetadataParameters[i]);
                prim->bapInternalUnicastClientUpdateMetadataReq.aseMetadataParameters[i] = NULL;
            }
        }
        break;
        case BAP_INTERNAL_UNICAST_CLIENT_DISABLE_REQ:
        {
            for (i = 0; i < prim->bapInternalUnicastClientDisableReq.numAseDisableParameters; ++i)
            {
                CsrPmemFree(prim->bapInternalUnicastClientDisableReq.aseDisableParameters[i]);
                prim->bapInternalUnicastClientDisableReq.aseDisableParameters[i] = NULL;
            }
        }
        break;
        case BAP_INTERNAL_UNICAST_CLIENT_RELEASE_REQ:
        {
            for (i = 0; i < prim->bapInternalUnicastClientReleaseReq.numAseReleaseParameters; ++i)
            {
                CsrPmemFree(prim->bapInternalUnicastClientReleaseReq.aseReleaseParameters[i]);
                prim->bapInternalUnicastClientReleaseReq.aseReleaseParameters[i] = NULL;
            }
        }
        break;
        case BAP_INTERNAL_UNICAST_CLIENT_CIG_CONFIGURE_REQ:
        {
            if(prim->bapInternalUnicastClientCigConfigureReq.cigParameters.cisCount)
            {
                CsrPmemFree(prim->bapInternalUnicastClientCigConfigureReq.cigParameters.cisConfig);
            }
        }
        break;
        case BAP_INTERNAL_UNICAST_CLIENT_CIG_TEST_CONFIGURE_REQ:
        {
            if(prim->bapInternalUnicastClientCigTestConfigureReq.cigTestParameters.cisCount)
            {
                CsrPmemFree(prim->bapInternalUnicastClientCigTestConfigureReq.cigTestParameters.cisTestConfig);
            }
        }
        break;
        case BAP_INTERNAL_SETUP_DATA_PATH_REQ:
        {
            if(prim->bapInternalSetupDataPathReq.dataPathParameter.vendorDataLen)
            {
                CsrPmemFree(prim->bapInternalSetupDataPathReq.dataPathParameter.vendorData);
                prim->bapInternalSetupDataPathReq.dataPathParameter.vendorData = NULL;
            }
        }
        break;
        case BAP_INTERNAL_UNICAST_CLIENT_CIS_CONNECT_REQ:
        {
            for (i = 0; i < prim->bapInternalUnicastClientCisConnectReq.cisCount; ++i)
            {
                CsrPmemFree(prim->bapInternalUnicastClientCisConnectReq.cisConnParameters[i]);
            }
        }
        break;
        case BAP_INTERNAL_UNICAST_CLIENT_RECEIVER_READY_REQ:
        {
            if(prim->bapInternalUnicastClientReceiverReadyReq.aseIds)
            {
                CsrPmemFree(prim->bapInternalUnicastClientReceiverReadyReq.aseIds);
            }
        }
        break;
        case BAP_INTERNAL_BROADCAST_SRC_ENABLE_STREAM_REQ:
        {
            if (prim->bapInternalBroadcastSrcEnableStreamReq.bigConfigParameters)
            {
                CsrPmemFree(prim->bapInternalBroadcastSrcEnableStreamReq.bigConfigParameters);
            }
            if (prim->bapInternalBroadcastSrcEnableStreamReq.broadcastCode)
            {
                CsrPmemFree(prim->bapInternalBroadcastSrcEnableStreamReq.broadcastCode);
            }
        }
        break;
        case BAP_INTERNAL_BROADCAST_SRC_UPDATE_METADATA_REQ:
        {
            if (prim->bapInternalBroadcastSrcUpdateMetadataStreamReq.numSubgroup)
            {
                CsrPmemFree(prim->bapInternalBroadcastSrcUpdateMetadataStreamReq.subgroupMetadata);
            }
        }
        break;
        default:
            break;
    }

}

#ifdef INSTALL_LEA_UNICAST_CLIENT
static void freeUpstreamPrimitive(BapUPrim * const prim)
{
    if (prim == NULL)
    {
        return;
    }

    /* Action taken depends on the primitive type */
    switch (prim->type)
    {
        case BAP_UNICAST_CLIENT_READ_ASE_INFO_CFM:
        {
            if(prim->bapUnicastClientReadAseInfoCfm.numAses)
            {
                CsrPmemFree(prim->bapUnicastClientReadAseInfoCfm.aseIds);
                prim->bapUnicastClientReadAseInfoCfm.aseIds = NULL;
                if(prim->bapUnicastClientReadAseInfoCfm.aseInfo &&
                    prim->bapUnicastClientReadAseInfoCfm.aseInfoLen)
                {
                    CsrPmemFree(prim->bapUnicastClientReadAseInfoCfm.aseInfo);
                }
            }
            break;
        }

        default:
            break;
    }

}


static void handleBapDiscoverRemoteAudioCapabilityReq(BAP* const bap,
                                                      BapInternalDiscoverRemoteAudioCapabilityReq* const primitive)
{
    BapConnection* connection = NULL;
    GattPacsClientType direction = (primitive->recordType == BAP_AUDIO_SINK_RECORD) ?
                                    GATT_PACS_CLIENT_SINK : GATT_PACS_CLIENT_SOURCE;

    if (bapClientFindConnectionByCid(bap, primitive->handle, &connection))
    {
        GattPacsClientReadPacRecordReq(connection->pacs.srvcHndl, direction);
    }
}

static void handleBapRegisterPacsNotificationReq(BAP* const bap,
                                                 BapInternalRegisterPacsNotificationReq* const primitive)
{
    BapConnection* connection = NULL;

    if (bapClientFindConnectionByCid(bap, primitive->handle, &connection))
    {
        GattPacsClientRegisterForNotification(connection->pacs.srvcHndl, primitive->notifyType, primitive->notifyEnable);
    }
}

static void handleBapGetRemoteAudioLocationReq(BAP* const bap,
                                               BapInternalGetRemoteAudioLocationReq* const primitive)
{
    BapConnection* connection = NULL;
    GattPacsClientType direction = (primitive->recordType == BAP_AUDIO_SINK_RECORD) ?
        GATT_PACS_CLIENT_SINK : GATT_PACS_CLIENT_SOURCE;

    if (bapClientFindConnectionByCid(bap, primitive->handle, &connection))
    {
        GattPacsClientReadAudioLocationReq(connection->pacs.srvcHndl, direction);
    }
}

static void handleBapSetRemoteAudioLocationReq(BAP* const bap,
                                               BapInternalSetRemoteAudioLocationReq* const primitive)
{
    BapConnection* connection = NULL;
    GattPacsClientType direction = (primitive->recordType == BAP_AUDIO_SINK_RECORD) ?
        GATT_PACS_CLIENT_SINK : GATT_PACS_CLIENT_SOURCE;

    if (bapClientFindConnectionByCid(bap, primitive->handle, &connection))
    {
        GattPacsClientWriteAudioLocationReq(connection->pacs.srvcHndl, direction, primitive->location);
    }
}

static void handleBapDiscoverAudioContextReq(BAP* const bap,
                                             BapInternalDiscoverAudioContextReq* const primitive)
{
    BapConnection* connection = NULL;
    GattPacsClientAudioContext context = (primitive->context == BAP_PAC_AVAILABLE_AUDIO_CONTEXT) ?
        GATT_PACS_CLIENT_AVAILABLE : GATT_PACS_CLIENT_SUPPORTED;

    if (bapClientFindConnectionByCid(bap, primitive->handle, &connection))
    {
        GattPacsClientReadAudioContextReq(connection->pacs.srvcHndl, context);
    }
}


static void handleStreamGroupCodecConfigureReq(BAP * const bap,
                                               BapInternalUnicastClientCodecConfigureReq * const primitive)
{
    BapResult result = BAP_RESULT_ERROR;
    BapStreamGroup *streamGroup = NULL;

    if (!bapClientFindStreamGroupById(bap,
                                      primitive->handle,
                                      &streamGroup))
    {
        BapConnection *connection = NULL;

        if (bapClientFindConnectionByCid(bap, primitive->handle, &connection))
        {
            streamGroup = bapStreamGroupNew(bap,
                                             connection->rspPhandle,
                                             primitive->handle,
                                             primitive->numAseCodecConfigurations,
                                             primitive->aseCodecConfigurations);
            if (streamGroup != NULL)
            {
                bapClientAddStreamGroup(bap, streamGroup);
            }
        }
    }
    else
    {
        result = bapStreamGroupUpdate(bap,
                                      streamGroup,
                                      primitive->numAseCodecConfigurations,
                                      primitive->aseCodecConfigurations);

    }

    if (streamGroup != NULL)
    {
        result = bapStreamGroupConfigure(streamGroup,
                                         primitive);
    }

    if (result != BAP_RESULT_SUCCESS)
    {
        if (streamGroup)
        {
            bapUtilsSendStreamGroupCodecConfigureCfm(streamGroup->rspPhandle,
                                                     result,
                                                     primitive->handle);
        }
    }
}

static void handleSetQosConfigureReq(BAP * const bap, BapStreamGroup *streamGroup,
                                     uint8 numAses,
                                     BapAseQosConfiguration ** const aseQosConfigurations)
{
    uint8 i = 0;

    for (i = 0; i < numAses; ++i)
    {
        BapConnection *connection = NULL;

        if (bapClientFindConnectionByCid(bap, streamGroup->id, &connection))
        {
            BapAse *ase = NULL;

            ase = bapConnectionFindAseByAseId(connection,
                                              aseQosConfigurations[i]->aseId);

            if(ase)
            {
                memset(&ase->isocConfig, 0, sizeof(BapIsocConfig));
                /* update CIG ID */
                ase->cis->cigId = aseQosConfigurations[i]->cigId;
                /* update QOS parameter from the Upper layer */
                ase->isocConfig.sduIntervalMtoS = aseQosConfigurations[i]->qosConfiguration.sduInterval;
                ase->isocConfig.framing = aseQosConfigurations[i]->qosConfiguration.framing;
                ase->isocConfig.phyMtoS = aseQosConfigurations[i]->qosConfiguration.phy;
                ase->isocConfig.sduSizeMtoS = aseQosConfigurations[i]->qosConfiguration.sduSize;
                ase->isocConfig.rtnMtoS = aseQosConfigurations[i]->qosConfiguration.rtn;
                ase->isocConfig.transportLatencyMtoS = aseQosConfigurations[i]->qosConfiguration.transportLatency;
                ase->presentationDelayMin = aseQosConfigurations[i]->qosConfiguration.presentationDelay;

                bapAseQosSerialise(&ase->qos,
                                   aseQosConfigurations[i]->qosConfiguration.sduInterval,
                                   aseQosConfigurations[i]->qosConfiguration.framing,
                                   aseQosConfigurations[i]->qosConfiguration.phy,
                                   aseQosConfigurations[i]->qosConfiguration.sduSize,
                                   aseQosConfigurations[i]->qosConfiguration.rtn,
                                   aseQosConfigurations[i]->qosConfiguration.transportLatency);
            }

        }
    }
}

static void handleStreamGroupQosConfigureReq(BAP * const bap,
                                             BapInternalUnicastClientQosConfigureReq * const primitive)
{
    BapResult result = BAP_RESULT_ERROR;
    BapStreamGroup *streamGroup = NULL;

    if (bapClientFindStreamGroupById(bap,
                                     primitive->handle,
                                     &streamGroup))
    {
        handleSetQosConfigureReq(bap,
                                 streamGroup,
                                 primitive->numAseQosConfigurations,
                                 primitive->aseQosConfigurations);

        result = bapStreamGroupQosConfigure(streamGroup,
                                            primitive);
        if (result != BAP_RESULT_SUCCESS)
        {
            bapUtilsSendStreamGroupQosConfigureCfm(streamGroup->rspPhandle,
                                                   result,
                                                   primitive->handle);
        }
    }

}

static void handleStreamGroupEnableReq(BAP * const bap,
                                       BapInternalUnicastClientEnableReq * const primitive)
{
    BapStreamGroup *streamGroup = NULL;
    BapResult result = BAP_RESULT_ARG_ERROR;

    if (bapClientFindStreamGroupById(bap,
                                     primitive->handle,
                                     &streamGroup))
    {
        result = bapStreamGroupEnable(streamGroup, primitive);
        if (result != BAP_RESULT_SUCCESS)
        {
            bapUtilsSendStreamGroupEnableCfm(streamGroup->rspPhandle,
                                             result,
                                             primitive->handle);
        }

    }
}

static void handleStreamGroupDisableReq(BAP * const bap,
                                        BapInternalUnicastClientDisableReq * const primitive)
{
    BapStreamGroup *streamGroup = NULL;
    BapResult result = BAP_RESULT_ARG_ERROR;

    if (bapClientFindStreamGroupById(bap,
                                     primitive->handle,
                                     &streamGroup))
    {
        result = bapStreamGroupDisable(streamGroup, primitive);
        if (result != BAP_RESULT_SUCCESS)
        {
            bapUtilsSendStreamGroupDisableCfm(streamGroup->rspPhandle,
                                              result,
                                              primitive->handle);
        }
    }
}

static void handleStreamGroupReleaseReq(BAP * const bap,
                                        BapInternalUnicastClientReleaseReq * const primitive)
{
    BapStreamGroup *streamGroup = NULL;
    BapResult result = BAP_RESULT_ARG_ERROR;

    if (bapClientFindStreamGroupById(bap,
                                     primitive->handle,
                                     &streamGroup))
    {
        result = bapStreamGroupRelease(streamGroup, primitive);
    }
    if (result != BAP_RESULT_SUCCESS)
    {
        bapUtilsSendStreamGroupReleaseCfm(streamGroup->rspPhandle,
                                          result,
                                          primitive->handle);
    }
}

static void handleStreamGroupUpdateMetadataReq(BAP * const bap,
                                               BapInternalUnicastClientUpdateMetadataReq * const primitive)
{
    BapStreamGroup *streamGroup = NULL;
    BapResult result = BAP_RESULT_ARG_ERROR;

    if (bapClientFindStreamGroupById(bap,
                                     primitive->handle,
                                     &streamGroup))
    {
        result = bapStreamGroupHandleBapPrim(streamGroup,
                                             (BapUPrim *)primitive);
        if (result != BAP_RESULT_SUCCESS)
        {
            bapUtilsSendStreamGroupMetadataCfm(streamGroup->rspPhandle,
                                               result,
                                               primitive->handle);
        }
    }
}

static void handleStreamGroupReceiverReadyReq(BAP * const bap,
                                              BapInternalUnicastClientReceiverReadyReq * const primitive)
{
    BapStreamGroup *streamGroup = NULL;
    BapResult result = BAP_RESULT_ARG_ERROR;

    if (bapClientFindStreamGroupById(bap,
                                     primitive->handle,
                                     &streamGroup))
    {
        result = bapStreamGroupHandleBapPrim(streamGroup,
                                             (BapUPrim *)primitive);
    }
    if (result != BAP_RESULT_SUCCESS)
    {
        bapUtilsSendStreamGroupReceiverReadyCfm(streamGroup->rspPhandle,
                                                result,
                                                primitive->handle,
                                                primitive->readyType);
    }
}

Status bapClientAddPacRecord(BAP * const bap,
                             uint16 profileId,
                             BapServerDirection serverDirection,
                             BapCodecSubRecord* codecSubrecord,
                             uint16 * const pacRecordId)
{
    BapClientPacRecord *pacRecord;
    BapProfile *profile;
    Status status = STATUS_ERROR;

    if (!bapClientFindProfileById(bap, profileId, &profile))
    {
        profile = bapProfileNew(profileId);

        if (profile == NULL)
        {
            return STATUS_ERROR;
        }
        else
        {
            bapClientListPush(&bap->profileList, &profile->listElement);
        }
    }

    pacRecord = bapPacRecordNew(profile,
                                serverDirection,
                                codecSubrecord,
                                profile->lastPacRecordId);

    if (pacRecord != NULL)
    {
        bapProfileAddPacRecord(profile, pacRecord);

        *pacRecordId = pacRecord->id;

        status = STATUS_SUCCESS;
    }

    return status;
}

Status bapClientRemovePacRecord(BAP* const bap,
                                uint16 profileId,
                                uint16 pacRecordId)
{
    BapProfile* profile;
    Status status = STATUS_ERROR;

    /* Search for BAP Profile instance based on Profile ID */
    if (bapClientFindProfileById(bap, profileId, &profile))
    {
        /* Remove the pac record specified by pac_record_id */
        if (bapProfileRemovePacRecord(profile, pacRecordId))
            status = STATUS_SUCCESS;
    }

    return status;
}

/* ISOC related function calls */

static void handleBapUnicastClientCigConfigureReq(BAP* const bap,
                                                  BapInternalUnicastClientCigConfigureReq* const primitive)
{
    if((primitive->cigParameters.cigId == 0)|| /* New CIG configuration */
        (primitive->cigParameters.cigId < 0xFF)) /* Reconfiguration of CIG */
    {
        uint8 i;
        CmCisConfig *cisConfigParams[MAX_SUPPORTED_CIS];
        for (i=0; i < primitive->cigParameters.cisCount; i++)
        {
            cisConfigParams[i] = CsrPmemZalloc(sizeof(CmCisConfig));
            if(cisConfigParams[i])
            {
                memcpy(cisConfigParams[i], (primitive->cigParameters.cisConfig+i),
                            sizeof(CmCisConfig));
            }
        }

        /* store the app handle for CIG config */
        bap->appPhandle = primitive->handle;

        CmIsocConfigureCigReqSend(CSR_BT_BAP_IFACEQUEUE,
                                  primitive->cigParameters.sduIntervalMtoS,
                                  primitive->cigParameters.sduIntervalStoM,
                                  primitive->cigParameters.maxTransportLatencyMtoS,
                                  primitive->cigParameters.maxTransportLatencyStoM,
                                  primitive->cigParameters.cigId,
                                  primitive->cigParameters.sca,
                                  primitive->cigParameters.packing,
                                  primitive->cigParameters.framing,
                                  primitive->cigParameters.cisCount,
                                  cisConfigParams);
    }
    else  /* Invalid CIG ID */
    {
        /* TODO */
    }
}

static void handleBapUnicastClientCigTestConfigureReq(BAP* const bap,
                                                      BapInternalUnicastClientCigTestConfigureReq* const primitive)
{
    if((primitive->cigTestParameters.cigId == 0)|| /* New CIG Test configuration */
        (primitive->cigTestParameters.cigId < 0xFF)) /* Reconfiguration of CIG */
    {
        uint8 i;
        CmCisTestConfig *cisTestConfigParams[MAX_SUPPORTED_CIS];
        for (i=0; i < primitive->cigTestParameters.cisCount; i++)
        {
            cisTestConfigParams[i] = CsrPmemZalloc(sizeof(CmCisTestConfig));
            if(cisTestConfigParams[i])
            {
                memcpy(cisTestConfigParams[i], (primitive->cigTestParameters.cisTestConfig+i),
                            sizeof(CmCisTestConfig));
            }
        }

        /* store the app handle for CIG Test config */
        bap->appPhandle = primitive->handle;

        CmIsocConfigureCigTestReqSend(CSR_BT_BAP_IFACEQUEUE,
                                      primitive->cigTestParameters.sduIntervalMtoS,
                                      primitive->cigTestParameters.sduIntervalStoM,
                                      primitive->cigTestParameters.isoInterval,
                                      primitive->cigTestParameters.cigId,
                                      primitive->cigTestParameters.ftMtoS,
                                      primitive->cigTestParameters.ftStoM,                                     
                                      primitive->cigTestParameters.sca,
                                      primitive->cigTestParameters.packing,
                                      primitive->cigTestParameters.framing,
                                      primitive->cigTestParameters.cisCount,
                                      cisTestConfigParams);
    }
    else  /* Invalid CIG ID */
    {
        /* TODO */
    }
}

static void handleBapUnicastClientRemoveCigReq(BAP* const bap,
                                               BapInternalUnicastClientCigRemoveReq* const primitive)
{
    if(primitive->cigId <= 0xEF)
    {
        /* Verify the existing CIG_ID exist in the list */
        CmIsocRemoveCigReqSend(CSR_BT_BAP_IFACEQUEUE, primitive->cigId);
    }
    CSR_UNUSED(bap);
}
#endif

static void handleBapClientSetupDatapathReq(BAP* const bap,
                                            BapInternalSetupDataPathReq* const primitive)
{
    uint8 *codecConfig = NULL;
    uint8 size = 0;
    bool vSCodec = FALSE;
    uint8 codecConfigLength = SAMPLING_FREQUENCY_LENGTH + FRAME_DURATION_LENGTH +
                AUDIO_CHANNEL_ALLOCATION_LENGTH + OCTETS_PER_CODEC_FRAME_LENGTH +
                primitive->dataPathParameter.vendorDataLen + 4; /* Type field of 4 */
    uint8 codecId[CM_CODEC_ID_SIZE];

    if( primitive->dataPathParameter.isoHandle > 0x0EFF)
    {
        bapUtilsSendSetupDatapathCfm(bap->appPhandle,
                                     primitive->dataPathParameter.isoHandle,
                                     BAP_RESULT_INVALID_PARAMETER,
                                     primitive->handle);
        return;
    }

    if((primitive->dataPathParameter.codecId.codecId == BAP_CODEC_ID_VENDOR_DEFINED) &&
       (primitive->dataPathParameter.codecId.companyId == BAP_COMPANY_ID_QUALCOMM) &&
       ((primitive->dataPathParameter.codecId.vendorCodecId == BAP_VS_CODEC_ID_APTX) ||
       (primitive->dataPathParameter.codecId.vendorCodecId == BAP_VS_CODEC_ID_APTX_LITE) ||
       (primitive->dataPathParameter.codecId.vendorCodecId == BAP_VS_CODEC_ID_APTX_ADAPTIVE)))
    {
        vSCodec = TRUE;
        codecConfigLength = primitive->dataPathParameter.vendorDataLen;
    }

    /* Fill codec id and config data */
    codecId[0] = primitive->dataPathParameter.codecId.codecId;
    codecId[1] = (uint8)(primitive->dataPathParameter.codecId.companyId & 0x00FF);
    codecId[2] = (uint8)((primitive->dataPathParameter.codecId.companyId & 0xFF00) >> 8);
    codecId[3] = (uint8)(primitive->dataPathParameter.codecId.vendorCodecId & 0x00FF);
    codecId[4] = (uint8)((primitive->dataPathParameter.codecId.vendorCodecId & 0xFF00) >> 8);

    codecConfig = CsrPmemZalloc(codecConfigLength);
    if(codecConfig)
    {
        if (vSCodec == FALSE)
        {
            uint16 sampFreq = primitive->dataPathParameter.codecConfiguration.samplingFrequency;

            /*Codec_Specific_Configuration */
            codecConfig[size++] = SAMPLING_FREQUENCY_LENGTH;
            codecConfig[size++] = SAMPLING_FREQUENCY_TYPE;
            codecConfig[size++] = bapMapPacsSamplingFreqToAscsValue(sampFreq);

            codecConfig[size++] = FRAME_DURATION_LENGTH;
            codecConfig[size++] = FRAME_DURATION_TYPE;
            if(primitive->dataPathParameter.codecConfiguration.frameDuaration ==
                BAP_SUPPORTED_FRAME_DURATION_10MS)
            {
                codecConfig[size++] = 0x01;
            }
            else
            {
                codecConfig[size++] = 0x00;
            }

            if(primitive->dataPathParameter.codecConfiguration.audioChannelAllocation == BAP_AUDIO_LOCATION_MONO)
            {
                codecConfigLength = codecConfigLength - (AUDIO_CHANNEL_ALLOCATION_LENGTH + 1);
            }
            else
            {
                codecConfig[size++] = AUDIO_CHANNEL_ALLOCATION_LENGTH;
                codecConfig[size++] = AUDIO_CHANNEL_ALLOCATION_TYPE;
                codecConfig[size++] = (uint8)(primitive->dataPathParameter.codecConfiguration.audioChannelAllocation & 0x000000FF);
                codecConfig[size++] = (uint8)((primitive->dataPathParameter.codecConfiguration.audioChannelAllocation & 0x0000ff00) >> 8);
                codecConfig[size++] = (uint8)((primitive->dataPathParameter.codecConfiguration.audioChannelAllocation & 0x00ff0000) >> 16);
                codecConfig[size++] = (uint8)((primitive->dataPathParameter.codecConfiguration.audioChannelAllocation & 0xff000000u) >> 24);
            }

            codecConfig[size++] = OCTETS_PER_CODEC_FRAME_LENGTH;
            codecConfig[size++] = OCTETS_PER_CODEC_FRAME_TYPE;
            codecConfig[size++] = (uint8)(primitive->dataPathParameter.codecConfiguration.octetsPerFrame& 0x00FF);
            codecConfig[size++] = (uint8)((primitive->dataPathParameter.codecConfiguration.octetsPerFrame & 0xff00) >> 8);
        }
        /* Copy vendorData which is already in  LTV format*/
        if (primitive->dataPathParameter.vendorData && primitive->dataPathParameter.vendorDataLen)
        {
            memcpy(&codecConfig[size], primitive->dataPathParameter.vendorData, primitive->dataPathParameter.vendorDataLen);
        }
    }

    CmIsocSetupIsoDataPathReqSend(CSR_BT_BAP_IFACEQUEUE,
                                  primitive->dataPathParameter.isoHandle,
                                  primitive->dataPathParameter.dataPathDirection,
                                  primitive->dataPathParameter.dataPathId,
                                  codecId,
                                  primitive->dataPathParameter.controllerDelay,
                                  codecConfigLength,
                                  codecConfig);

     if(codecConfig)
        CsrPmemFree(codecConfig);

}

static void handleBapClientRemoveDatapathReq(BAP* const bap,
                                             BapInternalRemoveDataPathReq* const primitive)
{
    if( primitive->isoHandle > 0x0EFF)
    {
        bapUtilsSendSetupDatapathCfm(bap->appPhandle,
                                     primitive->isoHandle,
                                     BAP_RESULT_INVALID_PARAMETER,
                                     primitive->handle);
        return;
    }

    /* Verify the for valid iso_handle */
    CmIsocRemoveIsoDataPathReqSend(CSR_BT_BAP_IFACEQUEUE,
                                   primitive->isoHandle,
                                   primitive->dataPathDirection);
}

#ifdef INSTALL_LEA_UNICAST_CLIENT

static void handleStreamGroupCisConnectReq(BAP * const bap,
                                           BapInternalUnicastClientCisConnectReq * const primitive)
{
    BapStreamGroup *streamGroup = NULL;
    BapResult result = BAP_RESULT_ARG_ERROR;

    if (bapClientFindStreamGroupById(bap,
                                     primitive->handle,
                                     &streamGroup))
    {
        result = bapStreamGroupHandleBapPrim(streamGroup,
                                             (BapUPrim *)primitive);
        if (result != BAP_RESULT_SUCCESS)
        {
            bapUtilsSendStreamGroupCisConnectCfm(streamGroup->rspPhandle,
                                                 result,
                                                 primitive->handle);
        }
    }
}

static void handleStreamGroupCisDisconnectReq(BAP * const bap,
                                              BapInternalUnicastClientCisDisconnectReq * const primitive)
{
    BapStreamGroup *streamGroup = NULL;
    BapResult result = BAP_RESULT_ARG_ERROR;

    if (bapClientFindStreamGroupById(bap,
                                     primitive->handle,
                                     &streamGroup))
    {
        result = bapStreamGroupHandleBapPrim(streamGroup,
                                             (BapUPrim *)primitive);
        if (result != BAP_RESULT_SUCCESS)
        {
            bapUtilsSendCisDisconnectCfm(streamGroup->rspPhandle,
                                         result,
                                         primitive->handle,
                                         primitive->cisHandle);
        }
    }
}

static void handleBapSetControlPointOpReq(BAP * const bap,
                                          BapInternalSetControlPointOpReq * const req)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_INVALID_OPERATION;

    if (bapClientFindConnectionByCid(bap, req->handle, &connection))
    {
        if(connection->ascs.srvcHndl != INVALID_SERVICE_HANDLE)
        {
            /* update ASCS control point operation */
            GattAscsSetAsesControlPointReq(connection->ascs.srvcHndl,
                                                    req->controlOpResponse);
        }

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
        /* Update the assistant for control point op */
        bapBroadcastAssistantUpdateControlPointOp(connection, req->controlOpResponse, req->longWrite);
#endif
        result = BAP_RESULT_SUCCESS;
    }

    bapSetUtilsSendControlPointOpCfm(bap->appPhandle,
                                     req->handle,
                                     result);
}
#endif

struct BAP* bapGetInstance(void)
{
    return inst;
}

/**@}*/
