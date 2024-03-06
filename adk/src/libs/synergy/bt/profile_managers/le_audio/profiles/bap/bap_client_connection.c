/*******************************************************************************

Copyright (C) 2018 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP CONNECTION initiator interface implementation.
 */

/**
 * \addtogroup BAP_CLIENT_CONNECTION_PRIVATE
 * @{
 */

#include <string.h>
#include "bap_client_list_container_cast.h"
#include "bap_client_list_util_private.h"
#include "bap_utils.h"
#include "bap_profile.h"
#include "bap_codec_subrecord.h"
#include "bap_client_ase.h"
#include "bap_client_connection.h"
#include "bap_stream_group.h"
#include "bap_gatt_msg_handler.h"
#include "csr_bt_cm_lib.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

#define STREAMING_AUDIO_CONTEXT_LENGTH      0x03
#define STREAMING_AUDIO_CONTEXT_TYPE        0x02


static BapResult bapClientConnectionVHandleBapPrim(BapConnection * const connection, BapUPrim* prim);

static void bapClientConnectionVHandleAscsMsg(BapConnection * const connection, AscsMsg* msg);

static BapResult bapClientConnectionVCodecConfigureAse(BapConnection * const connection,
                                                       BapInternalUnicastClientCodecConfigureReq * const primitive);

static BapResult bapClientConnectionVQosConfigureAse(BapConnection * const connection,
                                                     BapInternalUnicastClientQosConfigureReq * const primitive);

static BapResult bapClientConnectionVEnableAse(BapConnection * const connection,
                                               BapInternalUnicastClientEnableReq *const primitive);

static BapResult bapClientConnectionCisConnect(BapConnection * const connection,
                                               BapInternalUnicastClientCisConnectReq *const primitive);

static BapResult bapClientConnectionCisDisconnect(BapConnection * const connection,
                                                  BapInternalUnicastClientCisDisconnectReq *const primitive);

static BapResult bapClientConnectionVDisableAse(BapConnection * const connection,
                                                BapInternalUnicastClientDisableReq *const primitive);

static BapResult bapClientConnectionVReleaseAse(BapConnection * const connection,
                                                BapInternalUnicastClientReleaseReq *const primitive);

static BapResult bapClientConnectionUpdateMetadata(BapConnection * const connection,
                                                   BapInternalUnicastClientUpdateMetadataReq *const primitive);

static BapResult bapClientConnectionReceiverReady(BapConnection * const connection,
                                                  BapInternalUnicastClientReceiverReadyReq *const primitive);

#ifdef INSTALL_GATT_SUPPORT
static uint8* bapClientConnectionGenericOpNew(uint8 opcode,
                                                uint8 numAses,
                                                uint8* aseIds,
                                                uint8* size);
#endif
static BapAse* bapClientConnectionVCreateAse(BapConnection * const connection,
                                             struct BapStreamGroup * const streamGroup,
                                             BapAseCodecConfiguration * const aseConfiguration);

static void bapClientConnectionVDelete(BapConnection * const connection);

static void bapClientConnectionUpdateStatusForEachAse(BapClientConnection * const client,
                                                      BapResult status,
                                                      uint8 numAses,
                                                      uint8* aseIds);

static const BapConnectionVtable connectionVTable =
{
    bapClientConnectionVHandleBapPrim,
    bapClientConnectionVHandleAscsMsg,
    bapConnectionVHandleCmPrim,
    bapClientConnectionVCreateAse,
    bapClientConnectionVCodecConfigureAse,
    bapClientConnectionVQosConfigureAse,
    bapClientConnectionVEnableAse,
    bapClientConnectionVDisableAse,
    bapClientConnectionVReleaseAse,
    bapClientConnectionVDelete
};

/*! \brief RTTI information for the BapClientConnection structure.
 */
type_name_declare_and_initialise_const_rtti_variable(BapClientConnection,  'A','s','I','c')

/*! \brief Make the BapClientAse structure RTTI information visible.
 */
type_name_enable_verify_of_external_type(BapClientAse)

BapConnection *bapClientConnectionNew(BAP * const bap,
                                      phandle_t phandle,
                                      TYPED_BD_ADDR_T * const peerAddrt)
{
    BapConnection *connection = NULL;
    BapClientConnection * const client = CsrPmemZalloc(sizeof(BapClientConnection));

    bapClientListElementInitialise(&client->listElement);

    client->streamGroup = NULL;

    if (client != NULL)
    {
        bapConnectionInitialise(&client->connection,
                                phandle,
                                peerAddrt,
                                bap,
                                &connectionVTable);

        type_name_initialise_rtti_member_variable(BapClientConnection, client);

        connection = &client->connection;
    }

    return connection;
}

static BapResult bapClientConnectionVHandleBapPrim(BapConnection * const connection, BapUPrim* prim)
{
    BapResult result = BAP_RESULT_SUCCESS;
    switch (prim->type)
    {
        case BAP_INTERNAL_UNICAST_CLIENT_CIS_CONNECT_REQ:
            result = bapClientConnectionCisConnect(connection,
                                                   &prim->bapInternalUnicastClientCisConnectReq);
            break;
        case BAP_INTERNAL_UNICAST_CLIENT_CIS_DISCONNECT_REQ:
            result = bapClientConnectionCisDisconnect(connection,
                                                      &prim->bapInternalUnicastClientCisDisconnectReq);
            break;
        case BAP_INTERNAL_UNICAST_CLIENT_UPDATE_METADATA_REQ:
            result = bapClientConnectionUpdateMetadata(connection,
                                                       &prim->bapInternalUnicastClientUpdateMetadataReq);
            break;
        case BAP_INTERNAL_UNICAST_CLIENT_RECEIVER_READY_REQ:
            result = bapClientConnectionReceiverReady(connection,
                                                      &prim->bapInternalUnicastClientReceiverReadyReq);
            break;
        default:
            break;
    }
    return result;
}

static void bapClientConnectionVHandleAscsMsg(BapConnection * const connection, AscsMsg* msg)
{
    (void)connection;
    (void)msg;
    /* TODO */
}

static BapResult bapClientConnectionVCodecConfigureAse(BapConnection * const connection,
                                                       BapInternalUnicastClientCodecConfigureReq * const primitive)
{
    BapAse *ase;
    BapResult result = BAP_RESULT_ARG_ERROR;
    BapStreamGroup *streamGroup = NULL;
    BapClientAse *clientAse = NULL;
    BapClientListElement* listElement;
    uint16 i;

    BapClientConnection *client = CONTAINER_CAST(connection, BapClientConnection, connection);

    for( i=0; i< primitive->numAseCodecConfigurations; i++)
    {
        /* ASE ID verification */
        ase = bapConnectionFindAseByAseId(&client->connection,
                                          primitive->aseCodecConfigurations[i]->aseId);
        if( ase == NULL)
        {
            return BAP_RESULT_INVALID_ASE_ID;
        }
    }

    /* Check ASE State from the ASE FSM */
    for( i=0; i< primitive->numAseCodecConfigurations; i++)
    {
        ase = bapConnectionFindAseByAseId(&client->connection,
                                          primitive->aseCodecConfigurations[i]->aseId);
        if(ase)
        {
            clientAse = CONTAINER_CAST(ase, BapClientAse, ase);
            streamGroup = clientAse->ase.streamGroup;
            result = bapClientAseCodecConfigure(clientAse, primitive);
            clientAse->ase.errorCode = result;
            if( result != BAP_RESULT_SUCCESS)
            {
                return result;
            }
        }
    }

    if(streamGroup)
    {
        /* Send a single Config codec */
        for (listElement = bapClientListPeekFront(&streamGroup->clientConnectionList);
             listElement != NULL;
             listElement = bapClientListElementGetNext(listElement))
        {
            BapClientConnection* tmpClient = CONTAINER_CAST(listElement, BapClientConnection, listElement);
            if(tmpClient->connection.cid == streamGroup->id)
            {
                if (bapClientConnectionSendConfigCodecOp(tmpClient,
                                                         primitive->numAseCodecConfigurations,
                                                         primitive->aseCodecConfigurations) != BAP_RESULT_SUCCESS)
                {
                    result = BAP_RESULT_INVALID_OPERATION;
                }
            }
        }
    }
    return result;
}

static BapResult bapClientConnectionVQosConfigureAse(BapConnection * const connection,
                                                     BapInternalUnicastClientQosConfigureReq * const primitive)
{
    BapAse *ase =  NULL;
    BapClientAse *clientAse =  NULL;
    BapResult result = BAP_RESULT_ARG_ERROR;
    BapStreamGroup *streamGroup = NULL;
    uint8 i;

    BapClientConnection *client = CONTAINER_CAST(connection, BapClientConnection, connection);

    for( i=0; i< primitive->numAseQosConfigurations; i++)
    {
        ase = bapConnectionFindAseByAseId(&client->connection,
                                          primitive->aseQosConfigurations[i]->aseId);
        if( ase == NULL)
        {
            return BAP_RESULT_INVALID_ASE_ID;
        }
    }

    /* sending change state in  ASE FSM */
    for( i=0; i< primitive->numAseQosConfigurations; i++)
    {
        ase = bapConnectionFindAseByAseId(&client->connection,
                                          primitive->aseQosConfigurations[i]->aseId);

        if(ase)
        {
            /* update from dummy cigId and cisId to actual cigId and cisId */
            ase->cis->cigId = primitive->aseQosConfigurations[i]->cigId;
            ase->cis->cisId = primitive->aseQosConfigurations[i]->cisId;
            ase->cis->cisHandle = primitive->aseQosConfigurations[i]->cisHandle;

            clientAse = CONTAINER_CAST(ase, BapClientAse, ase);
            streamGroup = clientAse->ase.streamGroup;
            result = bapClientAseQosConfigure(clientAse, primitive);
            clientAse->ase.errorCode = result;
            if( result != BAP_RESULT_SUCCESS)
            {
                return result;
            }
        }
    }

    if(streamGroup)
    {
        bapClientListForeach(&streamGroup->clientConnectionList,
                             listElement,
                             BapClientConnection,
                             tmpClient,
                             bapClientConnectionSetAsePdSendConfigQosOp(client,
                                                                        primitive->numAseQosConfigurations,
                                                                        primitive->aseQosConfigurations)
                             );
    }
    return result;
}

static BapResult bapClientConnectionVEnableAse(BapConnection * const connection,
                                               BapInternalUnicastClientEnableReq *const primitive)
{
    BapAse *ase = NULL;
    BapClientAse *clientAse =  NULL;
    BapStreamGroup *streamGroup = NULL;
    BapResult result = BAP_RESULT_ARG_ERROR;
    uint16 i;
    BapClientConnection *client = CONTAINER_CAST(connection, BapClientConnection, connection);

    for( i=0; i< primitive->numAseEnableParameters; i++)
    {
        ase = bapConnectionFindAseByAseId(&client->connection,
                                          primitive->aseEnableParameters[i]->aseId);
        if( ase == NULL)
        {
            return BAP_RESULT_INVALID_ASE_ID;
        }
    }

    /* sending Enable in from one ASE FSM */
    for( i=0; i< primitive->numAseEnableParameters; i++)
    {
        ase = bapConnectionFindAseByAseId(&client->connection,
                                          primitive->aseEnableParameters[i]->aseId);

        if (ase != NULL)
        {
            clientAse = CONTAINER_CAST(ase, BapClientAse, ase);
            streamGroup = clientAse->ase.streamGroup;
            result = bapClientAseEnable(clientAse, primitive);
            clientAse->ase.errorCode = result;
            if( result != BAP_RESULT_SUCCESS)
            {
                return result;
            }
        }
    }

    if(streamGroup)
    {
        bapClientListForeach(&streamGroup->clientConnectionList,
                             listElement,
                             BapClientConnection,
                             tmpClient,
                             bapClientConnectionSendEnableOp(client,
                                                             primitive->numAseEnableParameters,
                                                             primitive->aseEnableParameters)
                             );
    }
    return result;
}

static BapResult bapClientConnectionVDisableAse(BapConnection * const connection,
                                                BapInternalUnicastClientDisableReq *const primitive)
{
    BapAse *ase = NULL;
    BapClientAse *clientAse =  NULL;
    BapStreamGroup *streamGroup = NULL;
    BapResult result = BAP_RESULT_ARG_ERROR;
    uint16 i;
    BapClientConnection *client = CONTAINER_CAST(connection, BapClientConnection, connection);

    for( i=0; i< primitive->numAseDisableParameters; i++)
    {
        ase = bapConnectionFindAseByAseId(&client->connection,
                                          primitive->aseDisableParameters[i]->aseId);
        if( ase == NULL)
        {
            return BAP_RESULT_INVALID_ASE_ID;
        }
    }

    /* TODO ASE ID verification for all */
    /* sending Codec Config in from one ASE FSM */
    for( i=0; i< primitive->numAseDisableParameters; i++)
    {
        ase = bapConnectionFindAseByAseId(&client->connection,
                                          primitive->aseDisableParameters[i]->aseId);

        if (ase != NULL)
        {
            clientAse = CONTAINER_CAST(ase, BapClientAse, ase);
            streamGroup = clientAse->ase.streamGroup;
            result = bapClientAseDisable(clientAse, primitive);
            clientAse->ase.errorCode = result;
            if( result != BAP_RESULT_SUCCESS)
            {
                return result;
            }
        }
    }

    if(streamGroup)
    {
        bapClientListForeach(&streamGroup->clientConnectionList,
                             listElement,
                             BapClientConnection,
                             tmpClient,
                             bapClientConnectionSendDisableOp(client,
                                                              primitive->numAseDisableParameters,
                                                              primitive->aseDisableParameters)
                             );
    }
    return result;
}

static BapResult bapClientConnectionVReleaseAse(BapConnection * const connection,
                                                BapInternalUnicastClientReleaseReq *const primitive)
{
    BapAse *ase = NULL;
    BapClientAse *clientAse =  NULL;
    BapStreamGroup *streamGroup = NULL;
    BapResult result = BAP_RESULT_ARG_ERROR;
    uint16 i;

    BapClientConnection *client = CONTAINER_CAST(connection, BapClientConnection, connection);

    for( i=0; i< primitive->numAseReleaseParameters; i++)
    {
        ase = bapConnectionFindAseByAseId(&client->connection,
                                          primitive->aseReleaseParameters[i]->aseId);
        if( ase == NULL)
        {
            return BAP_RESULT_INVALID_ASE_ID;
        }
    }

    /* sending Codec Config in from one ASE FSM */
    for( i=0; i< primitive->numAseReleaseParameters; i++)
    {
        ase = bapConnectionFindAseByAseId(&client->connection,
                                          primitive->aseReleaseParameters[i]->aseId);

        if (ase != NULL)
        {
            clientAse = CONTAINER_CAST(ase, BapClientAse, ase);
            streamGroup = clientAse->ase.streamGroup;
            result = bapClientAseRelease(clientAse, primitive);
            clientAse->ase.errorCode = result;
            if( result != BAP_RESULT_SUCCESS)
            {
                return result;
            }
        }
    }

    if(streamGroup)
    {
        bapClientListForeach(&streamGroup->clientConnectionList,
                             listElement,
                             BapClientConnection,
                             tmpClient,
                             bapClientConnectionSendReleaseOp(client,
                                                              primitive->numAseReleaseParameters,
                                                              primitive->aseReleaseParameters)
                             );
    }
    return result;
}

static BapResult bapClientConnectionUpdateMetadata(BapConnection * const connection,
                                                   BapInternalUnicastClientUpdateMetadataReq *const primitive)
{
    BapAse *ase = NULL;
    BapClientAse *clientAse =  NULL;
    BapStreamGroup *streamGroup = NULL;
    BapResult result = BAP_RESULT_SUCCESS;
    uint16 i;

    BapClientConnection *client = CONTAINER_CAST(connection, BapClientConnection, connection);

    for( i=0; i< primitive->numAseMetadataParameters; i++)
    {
        ase = bapConnectionFindAseByAseId(&client->connection,
                                          primitive->aseMetadataParameters[i]->aseId);
        if( ase == NULL)
        {
            return BAP_RESULT_INVALID_ASE_ID;
        }
    }

    /* TODO ASE ID verification for all */
    for( i=0; i< primitive->numAseMetadataParameters; i++)
    {
        ase = bapConnectionFindAseByAseId(&client->connection,
                                          primitive->aseMetadataParameters[i]->aseId);

       if (ase != NULL)
        {
            clientAse = CONTAINER_CAST(ase, BapClientAse, ase);
            streamGroup = clientAse->ase.streamGroup;
            result = bapClientAseUpdateMetadata(clientAse, primitive);
            clientAse->ase.errorCode = result;
            if( result != BAP_RESULT_SUCCESS)
            {
                return result;
            }
        }
    }

    if(streamGroup)
    {
        bapClientListForeach(&streamGroup->clientConnectionList,
                             listElement,
                             BapClientConnection,
                             tmpClient,
                             bapClientConnectionSendMetadataOp(client,
                                                               primitive->numAseMetadataParameters,
                                                               primitive->aseMetadataParameters)
                             );
    }
    return result;
}

static BapResult bapClientConnectionReceiverReady(BapConnection * const connection,
                                                  BapInternalUnicastClientReceiverReadyReq *const primitive)
{
    BapAse *ase = NULL;
    BapClientAse *clientAse =  NULL;
    BapStreamGroup *streamGroup = NULL;
    BapResult result = BAP_RESULT_SUCCESS;
    uint16 i;

    BapClientConnection *client = CONTAINER_CAST(connection, BapClientConnection, connection);

    for( i=0; i< primitive->numAses; i++)
    {
        ase = bapConnectionFindAseByAseId(&client->connection,
                                          primitive->aseIds[i]);
        if(( ase == NULL) || (ase->serverDirection != BAP_SERVER_DIRECTION_SOURCE))
        {
            return BAP_RESULT_INVALID_ASE_ID;
        }
    }

    for( i=0; i< primitive->numAses; i++)
    {
        ase = bapConnectionFindAseByAseId(&client->connection,
                                          primitive->aseIds[i]);

        if (ase != NULL)
        {
            clientAse = CONTAINER_CAST(ase, BapClientAse, ase);
            streamGroup = clientAse->ase.streamGroup;

            result = bapClientAseReceiverReady(clientAse, primitive);
            clientAse->ase.errorCode = result;
            if( result != BAP_RESULT_SUCCESS)
            {
                return result;
            }
        }
    }

    if(streamGroup)
    {
        bapClientListForeach(&streamGroup->clientConnectionList,
                             listElement,
                             BapClientConnection,
                             tmpClient,
                             bapClientConnectionSendHandshakeOp(client,
                                                                primitive->readyType,
                                                                primitive->numAses,
                                                                primitive->aseIds)
                             );
    }
    return result;
}

static BapResult bapClientConnectionCisConnect(BapConnection * const connection,
                                               BapInternalUnicastClientCisConnectReq *const primitive)
{
    BapAse *ase = NULL;
    BapClientAse *clientAse =  NULL;
    BapStreamGroup *streamGroup = NULL;
    BapResult result = BAP_RESULT_SUCCESS;
    uint16 i;
    BapClientConnection *client = NULL;
    BapConnection *connections = NULL;

    for( i=0; i< primitive->cisCount; i++)
    {
        if(bapClientFindConnectionByTypedBdAddr(connection->bap,
                     (TYPED_BD_ADDR_T *)&primitive->cisConnParameters[i]->tpAddrt.addrt,
                                                    &connections))
        {
            client = CONTAINER_CAST(connections, BapClientConnection, connection);

            ase = bapConnectionFindAseByCisId(&client->connection,
                                              primitive->cisConnParameters[i]->cisId);

            /* update CIS handle of the ASES */
            if( ase == NULL)
            {
                return BAP_RESULT_INVALID_ASE_ID;
            }
            ase->cis->cisHandle = primitive->cisConnParameters[i]->cisHandle;
        }
        else
        {
            return BAP_RESULT_INVALID_PARAMETER;
        }

    }

    for( i=0; i< primitive->cisCount; i++)
    {
        uint8 num_ases = 0;
        BapAse *ase_ids[2];
        uint8 j = 0;

        if(bapClientFindConnectionByTypedBdAddr(connection->bap,
                     (TYPED_BD_ADDR_T *)&primitive->cisConnParameters[i]->tpAddrt.addrt,
                                                    &connections))
        {
            client = CONTAINER_CAST(connections, BapClientConnection, connection);

            ase = bapConnectionFindAseByCisId(&client->connection,
                                              primitive->cisConnParameters[i]->cisId);

            if( ase == NULL)
            {
                return BAP_RESULT_INVALID_ASE_ID;
            }
            /* check for Bidirectional cis and get the ASEs */
            num_ases = bapConnectionFindAseByCigAndCisId(&client->connection,
                                                         ase->cis->cigId,
                                                         primitive->cisConnParameters[i]->cisId,
                                                         primitive->cisConnParameters[i]->cisHandle,
                                                         ase_ids);

            if((num_ases == 0) || (num_ases > 2))
            {
                /* No matching CIG_ID and CIS_ID pair */
                return BAP_RESULT_INVALID_PARAMETER;
            }

            for( j = 0; j< num_ases; j++)
            {
                clientAse = CONTAINER_CAST(ase_ids[j], BapClientAse, ase);

                if(clientAse)
                {
                    result = bapClientAseCisConnect(clientAse, primitive);
                    streamGroup = clientAse->ase.streamGroup;
                    clientAse->ase.errorCode = result;
                }

                if( result != BAP_RESULT_SUCCESS)
                {
                    return result;
                }
            }
        }
    }

    if(streamGroup)
    {
        bapClientListForeach(&streamGroup->clientConnectionList,
                             listElement,
                             BapClientConnection,
                             tmpClient,
                             bapClientConnectionSendCisConnect(client,
                                                               primitive->cisCount,
                                                               primitive->cisConnParameters)
                             );
    }
    return result;
}

static BapResult bapClientConnectionCisDisconnect(BapConnection * const connection,
                                                  BapInternalUnicastClientCisDisconnectReq *const primitive)
{
    BapAse *ase = NULL;
    BapResult result = BAP_RESULT_INVALID_STATE;

    BapClientConnection *client = CONTAINER_CAST(connection, BapClientConnection, connection);

    ase = bapConnectionFindAseByCisHandle(&client->connection,
                                          primitive->cisHandle);

    if( ase == NULL)
    {
        return BAP_RESULT_ARG_ERROR;
    }

#ifdef CSR_BT_ISOC_ENABLE
#ifndef CSR_BT_INSTALL_QUAL_TESTER_SUPPORT
    if( ase->aseStateOnServer != BAP_ASE_STATE_STREAMING)
#endif
    {
        CmIsocCisDisconnectReqSend(CSR_BT_BAP_IFACEQUEUE, primitive->cisHandle,
                        primitive->reason);
        result = BAP_RESULT_SUCCESS;
    }
#endif

    return result;
}


void bapClientConnectionGetAllCisesInCig(BapClientConnection * const this, BapClientList* cisList, uint8 cigId)
{
    bapClientListForeach(&this->connection.cisList,
                         connListElement,
                         BapCis,
                         cis,
                         if (cis->cigId == cigId)
                             bapClientListPush(cisList, &cis->strmGrpListElement)
                         );
}

static BapAse* bapClientConnectionVCreateAse(BapConnection * const connection,
                                             struct BapStreamGroup * const streamGroup,
                                             BapAseCodecConfiguration * const aseCodecConfiguration)
{
    BapClientAse *clientAse = NULL;
    BapResult result = BAP_RESULT_ARG_ERROR;
    /* Dummy unique cis_id is set, actual to be set in QOS confg */
    uint8 cisId = aseCodecConfiguration->aseId;
    BapClientPacRecord *pacRecord = NULL;

    BapClientConnection *client = CONTAINER_CAST(connection, BapClientConnection, connection);

    if (client)
    {
    #ifdef PAC_VALIDATE
        BAP_PROFILE *profile;
        uint16   profileId = 0x8FD1;

        if (bapClientFindProfileById(client->connection.bap,
                                     profileId,
                                     &profile))
        {
            pacRecord = bapProfileFindPacRecordByCodecId(profile,
                                                         aseCodecConfiguration->server_direction,
                                                         &aseCodecConfiguration->codec_id);

            if ((pacRecord != NULL))
            {
    #endif
                if (!bapConnectionFindAseByAseId(&client->connection,
                                                 aseCodecConfiguration->aseId))
                {
                    #ifdef PAC_VALIDATE
                    if (bapPacRecordCheckCodecConfiguration(pacRecord,
                                                            &aseCodecConfiguration->codec_configuration))
                    #endif
                    {
                        BapCis* cis = bapConnectionFindCis(&client->connection,
                                                           cisId,
                                                           streamGroup->cigId);
                        if (!cis)
                        {
                            cis = bapConnectionCreateCis(&client->connection,
                                                         cisId,
                                                         streamGroup->cigId);
                            /*
                             * A CIS created by a client connection needs to be added to the stream group CIS list.
                             */
                            if (cis)
                                bapClientListPush(&streamGroup->cisList, &cis->strmGrpListElement);
                        }

                        if (cis)
                        {
                            clientAse = bapClientAseNew(aseCodecConfiguration,
                                                         pacRecord,
                                                         &client->connection,
                                                         cis,
                                                         streamGroup);
                            if (clientAse)
                            {
                                result = BAP_RESULT_SUCCESS;
                            }
                            else
                            {
                                /*
                                 * remove the CIS we've just created
                                 */
                                bapClientListRemoveIf(&client->connection.cisList,
                                                      connListElement,
                                                      BapCis,   /* Type of each element on the list */
                                                      tmpCis,        /* local variable to use in the predicate and the 'action' function (see below) */
                                                      (cis->cisId == cisId) &&  /* predicate */
                                                      (cis->cigId == streamGroup->cigId) &&             /* predicate */
                                                      (cis->serverIsSinkAse == NULL) &&                 /* predicate */
                                                      (cis->serverIsSourceAse == NULL),                 /* predicate */
                                                      bapCisDelete(cis)); /* 'action' to perform on removed elements */
                            }
                        }
                    }
                    #ifdef PAC_VALIDATE
                    else
                    {
                        result = BAP_RESULT_ARG_ERROR;
                    }
                    #endif
                }

                if (result != BAP_RESULT_SUCCESS)
                {
                    if (clientAse != NULL)
                    {
                        bapAseDelete(&clientAse->ase);
                        clientAse = NULL;
                    }
                }
        #ifdef PAC_VALIDATE
            }
        }
        #endif
    }
    return (clientAse)? &clientAse->ase : NULL;
}


BapResult bapClientConnectionSendConfigCodecOp(BapClientConnection * const client,
                                               uint8 numAses,
                                               BapAseCodecConfiguration ** const aseCodecConfiguration)
{
    BapAse *ase;
    uint16 aseIndex;
    BapResult result = BAP_RESULT_ERROR;
    uint8 configCodecOpSize = 0;
    uint8 *configCodecOp = NULL;
    uint8 size = 0;
    uint8 codecConfigSize = SAMPLING_FREQUENCY_LENGTH + FRAME_DURATION_LENGTH +
                              AUDIO_CHANNEL_ALLOCATION_LENGTH + OCTETS_PER_CODEC_FRAME_LENGTH +
                              LC3_BLOCKS_PER_SDU_LENGTH + 5 +1; /* Type field of 5 and Length field of 1 */

    configCodecOpSize = sizeof(uint8) + /* opcode */
                        sizeof(uint8);   /* num ases field */

    for (aseIndex = 0; aseIndex < numAses; aseIndex++)
    {
        configCodecOpSize += sizeof(AscsCodecInfo) + 2 - 1; /* +2 of Target latency and PHY -1 direction*/
        configCodecOpSize += codecConfigSize;
    }

    configCodecOp =  CsrPmemAlloc(configCodecOpSize * sizeof(uint8));

    if(configCodecOp)
    {
        configCodecOp[size++] = ASE_OPCODE_CONFIG_CODEC;
        configCodecOp[size++] = numAses;
        for(aseIndex = 0; aseIndex < numAses; aseIndex++)
        {
            uint16 sampFreq = aseCodecConfiguration[aseIndex]->codecConfiguration.samplingFrequency;
            bool vSCodec = FALSE;
            uint8 codecSpecificLen = codecConfigSize;

            if((aseCodecConfiguration[aseIndex]->codecId.codecId == BAP_CODEC_ID_VENDOR_DEFINED) &&
                (aseCodecConfiguration[aseIndex]->codecId.companyId == BAP_COMPANY_ID_QUALCOMM) &&
                ((aseCodecConfiguration[aseIndex]->codecId.vendorCodecId == BAP_VS_CODEC_ID_APTX_LITE) ||
                (aseCodecConfiguration[aseIndex]->codecId.vendorCodecId == BAP_VS_CODEC_ID_APTX) ||
                (aseCodecConfiguration[aseIndex]->codecId.vendorCodecId == BAP_VS_CODEC_ID_APTX_ADAPTIVE)))
            {
                vSCodec = TRUE;
            }
            
            /* verify that ASE Codec config cid matched for client */
            configCodecOp[size++] = aseCodecConfiguration[aseIndex]->aseId;

            /* Target latency */
            configCodecOp[size++] = aseCodecConfiguration[aseIndex]->targetLatency;
            /* Target Phy */
            configCodecOp[size++] = aseCodecConfiguration[aseIndex]->targetPhy;

            /* Codec Id */
            configCodecOp[size++] = aseCodecConfiguration[aseIndex]->codecId.codecId;
            configCodecOp[size++] = (uint8)(aseCodecConfiguration[aseIndex]->codecId.companyId & 0x00FF);
            configCodecOp[size++] = (uint8)((aseCodecConfiguration[aseIndex]->codecId.companyId & 0xff00) >> 8);
            configCodecOp[size++] = (uint8)(aseCodecConfiguration[aseIndex]->codecId.vendorCodecId & 0x00FF);
            configCodecOp[size++] = (uint8)((aseCodecConfiguration[aseIndex]->codecId.vendorCodecId & 0xff00) >> 8);

            /* Skip audioChannelAllocation LTV in the case of Mono */
            if(aseCodecConfiguration[aseIndex]->codecConfiguration.audioChannelAllocation == BAP_AUDIO_LOCATION_MONO)
            {
                codecSpecificLen = codecSpecificLen - (AUDIO_CHANNEL_ALLOCATION_LENGTH + 1);
            }
            /* Skip frameDuaration LTV if value is zero*/
            if(aseCodecConfiguration[aseIndex]->codecConfiguration.frameDuaration == BAP_SUPPORTED_FRAME_DURATION_NONE)
            {
                codecSpecificLen = codecSpecificLen - (FRAME_DURATION_LENGTH + 1);
            }
            /* Skip lc3BlocksPerSdu LTV  if value is less than equal to 1 default value*/
            if(aseCodecConfiguration[aseIndex]->codecConfiguration.lc3BlocksPerSdu < 2)
            {
                codecSpecificLen = codecSpecificLen - (LC3_BLOCKS_PER_SDU_LENGTH + 1);
            }

            /* Codec_Specific_Configuration_Length */
            configCodecOp[size++] = codecSpecificLen - 1;

            /*Codec_Specific_Configuration */
            configCodecOp[size++] = SAMPLING_FREQUENCY_LENGTH;
            if(vSCodec)
                configCodecOp[size++] = SAMPLING_FREQUENCY_TYPE_VS;
            else
                configCodecOp[size++] = SAMPLING_FREQUENCY_TYPE;

            configCodecOp[size++] = bapMapPacsSamplingFreqToAscsValue(sampFreq);

            if(aseCodecConfiguration[aseIndex]->codecConfiguration.frameDuaration)
            {
                configCodecOp[size++] = FRAME_DURATION_LENGTH;
                configCodecOp[size++] = FRAME_DURATION_TYPE;
                if(aseCodecConfiguration[aseIndex]->codecConfiguration.frameDuaration ==
                    BAP_SUPPORTED_FRAME_DURATION_10MS)
                {
                    configCodecOp[size++] = 0x01;
                }
                else
                {
                    configCodecOp[size++] = 0x00;
                }
            }
            
            if(aseCodecConfiguration[aseIndex]->codecConfiguration.audioChannelAllocation)
            {
                configCodecOp[size++] = AUDIO_CHANNEL_ALLOCATION_LENGTH;
                if(vSCodec)
                    configCodecOp[size++] = AUDIO_CHANNEL_ALLOCATION_TYPE_VS;
                else
                    configCodecOp[size++] = AUDIO_CHANNEL_ALLOCATION_TYPE;
                configCodecOp[size++] = (uint8)(aseCodecConfiguration[aseIndex]->codecConfiguration.audioChannelAllocation & 0x000000FF);
                configCodecOp[size++] = (uint8)((aseCodecConfiguration[aseIndex]->codecConfiguration.audioChannelAllocation & 0x0000ff00) >> 8);
                configCodecOp[size++] = (uint8)((aseCodecConfiguration[aseIndex]->codecConfiguration.audioChannelAllocation & 0x00ff0000) >> 16);
                configCodecOp[size++] = (uint8)((aseCodecConfiguration[aseIndex]->codecConfiguration.audioChannelAllocation & 0xff000000u) >> 24);
            }

            configCodecOp[size++] = OCTETS_PER_CODEC_FRAME_LENGTH;
            configCodecOp[size++] = OCTETS_PER_CODEC_FRAME_TYPE;
            configCodecOp[size++] = (uint8)(aseCodecConfiguration[aseIndex]->codecConfiguration.octetsPerFrame& 0x00FF);
            configCodecOp[size++] = (uint8)((aseCodecConfiguration[aseIndex]->codecConfiguration.octetsPerFrame & 0xff00) >> 8);

            if(aseCodecConfiguration[aseIndex]->codecConfiguration.lc3BlocksPerSdu > 1)
            {
                /* TLV for B.4.2.5	LC3_Blocks_Per_SDU */
                configCodecOp[size++] = LC3_BLOCKS_PER_SDU_LENGTH;
                configCodecOp[size++] = LC3_BLOCKS_PER_SDU_TYPE;
                configCodecOp[size++] = aseCodecConfiguration[aseIndex]->codecConfiguration.lc3BlocksPerSdu; /* Default: 1 LC3 frame per SDU */
            }
        }

        if (bapConnectionSendAscsCmd(&client->connection,
                                     configCodecOp,
                                     size))
        {
            result = BAP_RESULT_SUCCESS;
        }

        for (aseIndex = 0; aseIndex < numAses; aseIndex++)
        {
            ase = bapConnectionFindAseByAseId(&client->connection, aseCodecConfiguration[aseIndex]->aseId);
            if (ase != NULL)
            {
                ase->errorCode = result;
            }
        }
    }

    return result;
}

static void bapClientConnectionUpdateStatusForEachAse(BapClientConnection * const client,
                                                      BapResult status,
                                                      uint8 numAses,
                                                      uint8* aseIds)
{
    int i;
    for (i = 0; i < numAses; i++)
{
        BapAse *ase = bapConnectionFindAseByAseId(&client->connection, aseIds[i]);
        if (ase != NULL)
        {
            ase->errorCode = status;
        }
    }
}

BapResult bapClientConnectionSetAsePdSendConfigQosOp(BapClientConnection * const client,
                                                     uint8 numAses,
                                                     BapAseQosConfiguration** const aseQosConfiguration)
{
    BapResult result = BAP_RESULT_ERROR;
    uint16 aseIndex;
    uint8 aseIds[BAP_MAX_SUPPORTED_ASES] = {0};

    if (numAses > 0)
    {
        uint8 size = 0;
        uint8 *configQosOp = CsrPmemAlloc(sizeof(uint8) + /* opcode */
                                            sizeof(uint8) + /* num ase ids field */
                                            sizeof(AscsConfigQosInfo) * numAses);
        if(configQosOp)
        {
            configQosOp[size++] = ASE_OPCODE_CONFIG_QOS;
            configQosOp[size++] = numAses;

            for (aseIndex = 0; aseIndex < numAses; ++aseIndex)
            {
                /* verify that ASE QOS config cid matched for client */
                 BapAse *ase = bapConnectionFindAseByAseId(&client->connection, aseQosConfiguration[aseIndex]->aseId);
                /*
                 * Because the ase_ids in ase_ids[] were supplied by bapConnectionGetAseIds()
                 * it should not be possible for bapConnectionFindAseByAseId() to return NULL
                 */
                if (ase)
                {
                    uint32 presentationDelay = aseQosConfiguration[aseIndex]->qosConfiguration.presentationDelay;
                    aseIds[aseIndex] = ase->id;

                    bapAseSetPresentationDelay(ase, presentationDelay);
                    configQosOp[size++] = ase->id;
                    configQosOp[size++] = bapAseGetCigId(ase);
                    configQosOp[size++] = bapAseGetCisId(ase);
                    memmove(&configQosOp[size], &ase->qos.data[0], MAX_QOS_LEN);
                    size += MAX_QOS_LEN;
                    configQosOp[size++] = (uint8) ((presentationDelay & 0x000000ff) >>  0); /* Presentation Delay */
                    configQosOp[size++] = (uint8) ((presentationDelay & 0x0000ff00) >>  8);
                    configQosOp[size++] = (uint8) ((presentationDelay & 0x00ff0000) >> 16);
                }
            }

            if (bapConnectionSendAscsCmd(&client->connection,
                                         configQosOp,
                                         size))
            {
                result = BAP_RESULT_SUCCESS;
            }

            bapClientConnectionUpdateStatusForEachAse(client,
                                                      result,
                                                      numAses,
                                                      aseIds);
        }
    }

    return result;
}

static void bapClientConnectionVDelete(BapConnection * const connection)
{
    BapClientConnection *client = CONTAINER_CAST(connection, BapClientConnection, connection);

    /* Make sure the base class destructor gets called */
    bapConnectionDestroy(&client->connection);

    CsrPmemFree(client);
}

BapResult bapClientConnectionSendEnableOp(BapClientConnection * const client,
                                          uint8 numAses,
                                          BapAseEnableParameters **const aseEnableParams)
{
    BapResult result = BAP_RESULT_ERROR;
    uint16 aseIndex;
    uint8 aseIds[BAP_MAX_SUPPORTED_ASES] = {0};
    uint16 totalMetaDataLen = 0;

    if(numAses)
    {
        BapAse *ase= NULL;
        uint8 *enableOp = NULL;
        uint8 size = 0;
        uint8 j;

        for(j = 0; j < numAses; j++)
        {
            totalMetaDataLen += aseEnableParams[j]->metadataLen;
        }

        if((enableOp = CsrPmemAlloc(sizeof(uint8) + /* size of opcode field */
                                    sizeof(uint8) + /* num ases */
                                    numAses *
                                    (sizeof(uint8) + /* size of ases_ids field */
                                    sizeof(uint8) /* size of metadata length field */
                                    + sizeof(uint8) * (1 + STREAMING_AUDIO_CONTEXT_LENGTH))
                                    + sizeof(uint8) * totalMetaDataLen
                                    /* size Metadata field ( L + TV)*/))!= NULL)
        {
            enableOp[size++] = ASE_OPCODE_ENABLE;
            enableOp[size++] = numAses;
            for (aseIndex = 0; aseIndex < numAses; aseIndex++)
            {
                ase = bapConnectionFindAseByAseId(&client->connection,
                                                  aseEnableParams[aseIndex]->aseId);

                if(ase != NULL)
                {
                    uint8 metadataLen = (uint8)(aseEnableParams[aseIndex]->metadataLen);
                    /* update context type */
                    ase->contextType = aseEnableParams[aseIndex]->streamingAudioContexts;
                    aseIds[aseIndex] = ase->id;

                    enableOp[size++] = ase->id;
                    /* Metadata length */
                    enableOp[size++] = 1 + STREAMING_AUDIO_CONTEXT_LENGTH + metadataLen;/* L + TV */
                    /* Metadata in LTV format */
                    enableOp[size++] = STREAMING_AUDIO_CONTEXT_LENGTH;
                    enableOp[size++] = STREAMING_AUDIO_CONTEXT_TYPE;
                    enableOp[size++] = (uint8)(aseEnableParams[aseIndex]->streamingAudioContexts & 0x00ff);
                    enableOp[size++] = (uint8)((aseEnableParams[aseIndex]->streamingAudioContexts & 0xff00) >> 8);

                    /* Copy Metadata which is already in  LTV format*/

                    if (aseEnableParams[aseIndex]->metadata && metadataLen)
                    {
                        memcpy(&enableOp[size], aseEnableParams[aseIndex]->metadata, metadataLen);
                        size += metadataLen;
                    }
                }
            }
        }

        if(ase)
        {
            if( bapConnectionSendAscsCmd(ase->connection,
                                         enableOp,
                                         size))
            {
                result = BAP_RESULT_SUCCESS;
            }
        }
        bapClientConnectionUpdateStatusForEachAse(client,
                                                  result,
                                                  numAses,
                                                  aseIds);
    }

    return result;
}

BapResult bapClientConnectionSendMetadataOp(BapClientConnection * const client,
                                            uint8 numAses,
                                            BapAseMetadataParameters **const aseMetadataParams)
{
    BapResult result = BAP_RESULT_ERROR;
    uint16 aseIndex ;
    uint8 aseIds[BAP_MAX_SUPPORTED_ASES] = {0};

    if(numAses)
    {
        BapAse *ase= NULL;
        uint8 *metadataParams = NULL;
        uint8 size = 0;
        uint8 j;
        uint16 totalMetaDataLen = 0;

        for(j = 0; j < numAses; j++)
         {
             totalMetaDataLen += aseMetadataParams[j]->metadataLen;
         }

        if((metadataParams = CsrPmemAlloc(sizeof(uint8) + /* size of opcode field */
                                          sizeof(uint8) + /* num ases */
                                          numAses *
                                          (sizeof(uint8) + /* size of ases_ids field */
                                          sizeof(uint8) /* size of metadata length field */ +
                                          sizeof(uint8) *(1+ STREAMING_AUDIO_CONTEXT_LENGTH))
                                          + totalMetaDataLen/* size Metadata field ( L + TV)*/))!= NULL)
        {

            metadataParams[size++] = ASE_OPCODE_METADATA;
            metadataParams[size++] = numAses;
            for (aseIndex = 0; aseIndex < numAses; aseIndex++)
            {
                uint8 metadataLen = aseMetadataParams[aseIndex]->metadataLen;
                ase = bapConnectionFindAseByAseId(&client->connection,
                                                  aseMetadataParams[aseIndex]->aseId);


                if(ase != NULL)
                {
                    /* update context type */
                    ase->contextType = aseMetadataParams[aseIndex]->streamingAudioContexts;
                    aseIds[aseIndex] = ase->id;

                    metadataParams[size++] = ase->id;
                    /* Metadata length */
                    metadataParams[size++] = 1 + STREAMING_AUDIO_CONTEXT_LENGTH + metadataLen; /* L + TV */
                    /* Metadata in LTV format */
                    metadataParams[size++] = STREAMING_AUDIO_CONTEXT_LENGTH;
                    metadataParams[size++] = STREAMING_AUDIO_CONTEXT_TYPE;
                    metadataParams[size++] = (uint8)(aseMetadataParams[aseIndex]->streamingAudioContexts & 0x00ff);
                    metadataParams[size++] = (uint8)((aseMetadataParams[aseIndex]->streamingAudioContexts & 0xff00) >> 8);

                    /* Copy Metadata which is already in  LTV format*/
                    if (aseMetadataParams[aseIndex]->metadata && metadataLen)
                    {
                        memcpy(&metadataParams[size], aseMetadataParams[aseIndex]->metadata, metadataLen);
                        size += metadataLen;
                    }


                }
            }
        }

        if(ase)
        {
            if( bapConnectionSendAscsCmd(ase->connection,
                                         metadataParams,
                                         size))
            {
                result = BAP_RESULT_SUCCESS;
            }
        }
        bapClientConnectionUpdateStatusForEachAse(client,
                                                  result,
                                                  numAses,
                                                  aseIds);
    }

    return result;
}

BapResult bapClientConnectionSendCisConnect(BapClientConnection * const client,
                                            uint8 cisCount,
                                            BapUnicastClientCisConnection **const cisConnParams)
{
    BapResult result = BAP_RESULT_ERROR;
    uint8 i;
    CmCisConnection *cisConnections[MAX_SUPPORTED_CIS];

    for (i = 0; i < MAX_SUPPORTED_CIS; i++)
    {
        cisConnections[i] = NULL;
    }

    for(i = 0; i<cisCount; i++)
    {
        cisConnections[i] = (CmCisConnection *)CsrPmemAlloc(sizeof(CmCisConnection));
        cisConnections[i]->cis_handle = cisConnParams[i]->cisHandle;
        tbdaddr_copy(&cisConnections[i]->tp_addrt.addrt, &cisConnParams[i]->tpAddrt.addrt);
        cisConnections[i]->tp_addrt.tp_type = LE_ACL;
    }

#ifdef CSR_BT_ISOC_ENABLE
    CmIsocCisConnectReqSend(CSR_BT_BAP_IFACEQUEUE,
                            cisCount,
                            &cisConnections[0]);
#endif

    CSR_UNUSED(client);
    return result;
}


BapResult bapClientConnectionSendHandshakeOp(BapClientConnection * const client,
                                             uint8 readyType,
                                             uint8 numAses,
                                             uint8 *aseIds)
{
    uint8 i;
    BapResult status = BAP_RESULT_SUCCESS;
    uint8 numServerIsSourceAses = 0;
    uint8 serverIsSourceAses[BAP_MAX_SUPPORTED_ASES];

    for (i = 0; i < numAses; i++)
    {
        BapAse* ase = bapConnectionFindAseByAseId(&client->connection, aseIds[i]);
        if (ase != NULL)
        {
            if (ase->serverDirection == BAP_SERVER_DIRECTION_SOURCE)
            {
                serverIsSourceAses[numServerIsSourceAses++] = ase->id;
            }
        }
    }

    if (numServerIsSourceAses > 0)
    {
        uint8 *handshakeOp = CsrPmemAlloc(sizeof(uint8) +          /* size of opcode field */
                                            sizeof(uint8) +          /* size of num_ase_ids field */
                                            sizeof(uint8) * numServerIsSourceAses);   /* the number of ase ids in this message */

        if (handshakeOp)
        {
            uint8 size = 0;

            if(readyType == BAP_RECEIVER_START_READY)
                handshakeOp[size++] = ASE_OPCODE_START_READY;
            else
                handshakeOp[size++] = ASE_OPCODE_STOP_READY;
            handshakeOp[size++] = numServerIsSourceAses;

            for (i = 0; i < numServerIsSourceAses; i++)
            {
                handshakeOp[size++] = serverIsSourceAses[i];
            }
            if (bapConnectionSendAscsCmd(&client->connection,
                                         handshakeOp,
                                         size))
            {
                status = BAP_RESULT_SUCCESS;
            }
        }
        else
        {
            status = BAP_RESULT_ERROR;
        }
    }
    return status;
}


BapResult bapClientConnectionSendDisableOp(BapClientConnection * const client,
                                           uint8 numAses,
                                           BapAseParameters **const aseDisableParams)
{
    BapResult status  = BAP_RESULT_ERROR;
    uint16 aseIndex;
    uint8 aseIds[BAP_MAX_SUPPORTED_ASES] = {0};

    if(numAses)
    {
        BapAse *ase= NULL;
        uint8 size = 0;
        uint8 *disableOp = NULL;

        if((disableOp = CsrPmemAlloc(sizeof(uint8) + /* size of opcode field */
                                     sizeof(uint8) + /* size num ases */
                                     numAses *
                                     sizeof(uint8) /* size of ases_ids field */))!= NULL)
        {
            disableOp[size++] = ASE_OPCODE_DISABLE;
            disableOp[size++] = numAses;
            for (aseIndex = 0; aseIndex < numAses; aseIndex++)
            {
                ase = bapConnectionFindAseByAseId(&client->connection,
                                                  aseDisableParams[aseIndex]->aseId);

                if(ase != NULL)
                {
                    aseIds[aseIndex] = ase->id;
                    disableOp[size++] = aseIds[aseIndex];
                }
            }
        }

        if(ase)
        {
            if(bapConnectionSendAscsCmd(ase->connection,
                                        disableOp,
                                        size))
            {
                status = BAP_RESULT_SUCCESS;
            }
        }
        bapClientConnectionUpdateStatusForEachAse(client,
                                                  status,
                                                  numAses,
                                                  aseIds);
    }

    return status;
}

/* TODO combine Desable and Release procedure */
BapResult bapClientConnectionSendReleaseOp(BapClientConnection * const client,
                                           uint8 numAses,
                                           BapAseParameters **const aseReleaseParams)
{
    BapResult status  = BAP_RESULT_ERROR;
    uint16 aseIndex;
    uint8 aseIds[BAP_MAX_SUPPORTED_ASES] = {0};

    if(numAses)
    {
        BapAse *ase= NULL;
        uint8 size = 0;
        uint8 *releaseOp = NULL;

        if((releaseOp = CsrPmemAlloc(sizeof(uint8) + /* size of opcode field */
                                     sizeof(uint8) + /* size of length for ase id + content type +ccid */
                                     numAses *
                                     sizeof(uint8)  /* size of ases_ids field */))!= NULL)
        {
            releaseOp[size++] = ASE_OPCODE_RELEASE;
            releaseOp[size++] = numAses;
            for (aseIndex = 0; aseIndex < numAses; aseIndex++)
            {
                ase = bapConnectionFindAseByAseId(&client->connection,
                                                  aseReleaseParams[aseIndex]->aseId);

                if(ase != NULL)
                {
                    aseIds[aseIndex] = ase->id;

                    releaseOp[size++] = aseIds[aseIndex];
                }
            }
        }

        if(ase)
        {
            if(bapConnectionSendAscsCmd(ase->connection,
                                        releaseOp,
                                        size))
            {
                status = BAP_RESULT_SUCCESS;
            }
        }
        bapClientConnectionUpdateStatusForEachAse(client,
                                                  status,
                                                  numAses,
                                                  aseIds);
    }


    return status;
}


void bapClientConnectionHandleReadAseInfoCfm(BapConnection * const connection,
                                             uint8 numAses,
                                             uint8 *aseInfo,
                                             uint8 aseState,
                                             BapAseType type)
{
    bapUtilsSendGetRemoteAseInfoCfm(connection->rspPhandle,
                                    BAP_RESULT_SUCCESS,
                                    connection->cid,
                                    numAses,
                                    aseInfo,
                                    aseState,
                                    type,
                                    0,
                                    NULL);
}


/****************************************************************************
 * Handle notifications received by client connections here
 ****************************************************************************/

void bapClientConnectionRcvAscsMsg(BapClientConnection * const client,
                                   AscsMsg * const msg)
{

    /*
     * Messages are sent one-at-a-time to ASCS via a tx output queue.
     * A msgs is only removed from the end of the queue after its associated response
     * is received.
     * Check to see if this received prim is a response to the prim at the head of the output queue.
     *
     * Response prims don't include the ase_id, so the 'sender_ase' (which is
     * where the response must be sent) is determined from the request prim
     * at the head of the output queue.
     */
#if defined(DEBUG)
    if (client->connection.counter != 0)
    {
        client->connection.counter--;
        TRACE("---\nCurrent client counter decremented to: %d  received a message!. \n---\n", client->connection.counter);
    }
    else
    {
        TRACE("Client counter is 0! Not decrementing but received message! \n");
    }
#endif
    bapStreamGroupHandleAscsMsg(client->streamGroup,
                                msg);

}

#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
/**@}*/
