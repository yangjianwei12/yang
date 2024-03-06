/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP STREAM GROUP interface implementation.
 */

/**
 * \addtogroup BAP_STREAM_GROUP_PRIVATE
 * @{
 */

#include "bap_stream_group.h"
#include "bap_client_list_container_cast.h"
#include "tbdaddr.h"
#include "bap_utils.h"
#include "bap_client_ase.h"
#include "bap_client_debug.h"
#include "bap_ase.h"
#include "bap_gatt_msg_handler.h"
#include <stdio.h>

#ifdef INSTALL_LEA_UNICAST_CLIENT

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

/*! \brief RTTI information for the BapStreamGroup structure.
 */
type_name_declare_and_initialise_const_rtti_variable(BapStreamGroup,  'A','s','S','g')

/*! \brief Make the BapConnection structure RTTI information visible.
 */
type_name_enable_verify_of_external_type(BapConnection)

/*! \brief Make the BapClientConnection structure RTTI information visible.
 */
type_name_enable_verify_of_external_type(BapClientConnection)

/*! \brief Make the BapAse structure RTTI information visible.
 */
type_name_enable_verify_of_external_type(BapAse)

/*! \brief Make the BapCis structure RTTI information visible.
 */
type_name_enable_verify_of_external_type(BapCis)

static uint8 cisCount = 0;

BapStreamGroup *bapStreamGroupNew(struct BAP * const bap,
                                  phandle_t phandle,
                                  BapProfileHandle id,
                                  uint8 numAses,
                                  BapAseCodecConfiguration ** const aseCodecConfigurations)
{
    uint8 i;
    BapStreamGroup *streamGroup;

    streamGroup = CsrPmemZalloc(sizeof(BapStreamGroup));

    if (streamGroup != NULL)
    {
        bapClientListElementInitialise(&streamGroup->listElement);

        bapClientListInitialise(&streamGroup->cisList);
        bapClientListInitialise(&streamGroup->clientConnectionList);

        streamGroup->id = id;
        streamGroup->cigId = 0;
        streamGroup->rspPhandle = phandle;
        streamGroup->numAses = numAses;
        streamGroup->numCis = 0;
        streamGroup->presentationDelayMinMicroseconds = BAP_ASE_PRESENTATION_DELAY_MIN_MICROSECONDS;
        streamGroup->presentationDelayMaxMicroseconds = BAP_ASE_PRESENTATION_DELAY_MAX_MICROSECONDS;
        streamGroup->bap = bap;

        type_name_initialise_rtti_member_variable(BapStreamGroup, streamGroup);
        for (i = 0; i < streamGroup->numAses; ++i)
        {
            BapConnection *connection = NULL;
            BapClientConnection *clientConn;

            if (bapClientFindConnectionByCid(bap, id, &connection))
            {
                bapConnectionCreateAse(connection,
                                       streamGroup,
                                       aseCodecConfigurations[i]);
                /*
                 * Have we already got this connection in our list?
                 */
                bapClientListFindIf(&streamGroup->clientConnectionList,
                                    listElement,
                                    BapClientConnection,
                                    clientConn,
                                    &clientConn->connection == connection);

                /* If client_conn is NULL, then we do not already have this connection in our list */
                if (clientConn == NULL)
                {
                    /*
                     * We do not already have this connection in our list (so add it to the end of the list)
                     *
                     * NOTE: As we are a Stream Group we only have Client connections (so the
                     *       cast (below) is safe and the rtti in debug builds will not complain).
                     *       If BAP ever handles Client and Server connections simultaneously, then it
                     *       will need to provide separate qbl_bap_find_[client|server]_connection_by_cid()
                     *       functions (or the equivalent thereof).
                     */
                    clientConn = CONTAINER_CAST(connection, BapClientConnection, connection);

                    bapClientListPush(&streamGroup->clientConnectionList, &clientConn->listElement);
                }

                if (clientConn)
                {
                    bapClientConnectionSetStreamGroup(clientConn, streamGroup);
                }
            }
        }

/* TODO
 * Check for failures; clear up the stream_group and any created CIS and ASE instances
        update stream_group->num_cis;
*/
    }

    return streamGroup;
}

BapResult bapStreamGroupUpdate(struct BAP * const bap,
                               BapStreamGroup *streamGroup,
                               uint8 numAses,
                               BapAseCodecConfiguration ** const aseCodecConfigurations)
{
    uint8 i;

    /* Set a default ISO config parameter based on different chipset */

    for (i = 0; i < numAses ; ++i)
    {
        BapConnection *connection = NULL;
        BapClientConnection *clientConn;

        if (bapClientFindConnectionByCid(bap, streamGroup->id, &connection))
        {
            bapConnectionCreateAse(connection,
                                   streamGroup,
                                   aseCodecConfigurations[i]);
            /*
             * Have we already got this connection in our list?
             */
            bapClientListFindIf(&streamGroup->clientConnectionList,
                                listElement,
                                BapClientConnection,
                                clientConn,
                                &clientConn->connection == connection);
            /* If client_conn is NULL, then we do not already have this connection in our list */
            if (clientConn == NULL)
            {
                /*
                 * We do not already have this connection in our list (so add it to the end of the list)
                 *
                 * NOTE: As we are a Stream Group we only have Client connections (so the
                 *       cast (below) is safe and the rtti in debug builds will not complain).
                 *       If BAP ever handles Client and Server connections simultaneously, then it
                 *       will need to provide separate qbl_bap_find_[client|server]_connection_by_cid()
                 *       functions (or the equivalent thereof).
                 */

                clientConn = CONTAINER_CAST(connection, BapClientConnection, connection);

                bapClientListPush(&streamGroup->clientConnectionList, &clientConn->listElement);
            }
            if (clientConn)
            {
                bapClientConnectionSetStreamGroup(clientConn, streamGroup);
            }
        }
    }

    return BAP_RESULT_SUCCESS;
}
BapCis* bapStreamGroupFindCisByCisHandle(BapStreamGroup * const streamGroup,
                                         uint16 cisHandle)
{
    BapCis* cis;

    bapClientListFindIf(&streamGroup->cisList,
                        strmGrpListElement,
                        BapCis,
                        cis,
                        bapCisGetCisHandle(cis) == cisHandle); /* Predicate: find_if exits if this is true */
    return cis;
}

BapResult bapStreamGroupConfigure(BapStreamGroup * const streamGroup,
                                  BapInternalUnicastClientCodecConfigureReq * const primitive)
{
    BapConnection *connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                primitive->numAseCodecConfigurations,
                                primitive->numAseCodecConfigurations);

    /* find the connection instance */
    if (bapClientFindConnectionByCid(streamGroup->bap, streamGroup->id, &connection))
    {
        if ((result = bapConnectionConfigureAse(connection,primitive)) == BAP_RESULT_SUCCESS)
        {
            result = BAP_RESULT_SUCCESS;
        }
    }

    return result;
}

BapResult bapStreamGroupQosConfigure(BapStreamGroup * const streamGroup,
                                     BapInternalUnicastClientQosConfigureReq * const primitive)
{
    BapConnection *connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                primitive->numAseQosConfigurations,
                                primitive->numAseQosConfigurations);

    /* find the connection instance */
    if (bapClientFindConnectionByCid(streamGroup->bap, streamGroup->id, &connection))
    {
        if ((result = bapConnectionQosConfigureAse(connection,primitive)) == BAP_RESULT_SUCCESS)
        {
            result = BAP_RESULT_SUCCESS;
        }
    }
        
    return result;
}


BapResult bapStreamGroupEnable(BapStreamGroup * const streamGroup, 
                               BapInternalUnicastClientEnableReq * const primitive)
{
    BapConnection *connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                primitive->numAseEnableParameters,
                                primitive->numAseEnableParameters);

    /* find the connection instance */
    if (bapClientFindConnectionByCid(streamGroup->bap, streamGroup->id, &connection))
    {
        if ((result= bapConnectionEnableAse(connection,primitive)) == BAP_RESULT_SUCCESS)
        {
            result = BAP_RESULT_SUCCESS;
        }
    }

    return result;
}

BapResult bapStreamGroupDisable(BapStreamGroup * const streamGroup,
                                BapInternalUnicastClientDisableReq * const primitive)
{
    BapConnection *connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                primitive->numAseDisableParameters,
                                primitive->numAseDisableParameters);

    /* find the connection instance */
    if (bapClientFindConnectionByCid(streamGroup->bap, streamGroup->id, &connection))
    {
        if ((result = bapConnectionDisableAse(connection,primitive)) == BAP_RESULT_SUCCESS)
        {
            result = BAP_RESULT_SUCCESS;
        }
    }
        
    return result;
}

BapResult bapStreamGroupRelease(BapStreamGroup * const streamGroup,
                                BapInternalUnicastClientReleaseReq * const primitive)
{
    BapConnection *connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                primitive->numAseReleaseParameters,
                                primitive->numAseReleaseParameters);
    /* find the connection instance */
    if (bapClientFindConnectionByCid(streamGroup->bap, streamGroup->id, &connection))
    {
        if ((result = bapConnectionReleaseAse(connection,primitive)) == BAP_RESULT_SUCCESS)
        {
            result = BAP_RESULT_SUCCESS;
        }
    }
        
    return result;
}

BapResult bapStreamGroupHandleBapPrim(BapStreamGroup * const streamGroup, 
                                      BapUPrim *const primitive)
{
    BapConnection *connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    switch(primitive->type)
    {
        case BAP_INTERNAL_UNICAST_CLIENT_CIS_CONNECT_REQ:
        {
            BapInternalUnicastClientCisConnectReq * const prim =
                (BapInternalUnicastClientCisConnectReq *)primitive;

            bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                        prim->cisCount,
                                        prim->cisCount);
            cisCount = prim->cisCount;
        }
        break;
        case BAP_INTERNAL_UNICAST_CLIENT_CIS_DISCONNECT_REQ:
        {
            BapInternalUnicastClientCisDisconnectReq * const prim =
                (BapInternalUnicastClientCisDisconnectReq *)primitive;

            if(prim->cisHandle)
                bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                            1,
                                            1);

        }
        break;
        case BAP_INTERNAL_UNICAST_CLIENT_UPDATE_METADATA_REQ:
        {
            BapInternalUnicastClientUpdateMetadataReq * const prim =
                (BapInternalUnicastClientUpdateMetadataReq *)primitive;

            bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                        prim->numAseMetadataParameters,
                                        prim->numAseMetadataParameters);
        }
        break;
        case BAP_INTERNAL_UNICAST_CLIENT_RECEIVER_READY_REQ:
        {
            BapInternalUnicastClientReceiverReadyReq * const prim =
                (BapInternalUnicastClientReceiverReadyReq *)primitive;

            bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                        prim->numAses,
                                        prim->numAses);

        }
        break;
        default:
            return result;

    }

    /* find the connection instance */
    if (bapClientFindConnectionByCid(streamGroup->bap, streamGroup->id, &connection))
    {
        result = bapConnectionHandleBapPrim(connection, (BapUPrim *)primitive);
    }
        
    return result;
}


void bapStreamGroupAseDestroyedInd(BapStreamGroup * const streamGroup,
                                   BapAse * const ase)
{
    (void) streamGroup;
    (void) ase;
    /*
     * Remove any ase pointers from this stream group
     */
}

void bapStreamGroupAseStateIdleNotifyReceived(BapStreamGroup * const streamGroup,
                                              BapClientAse * const clientAse,
                                              AscsMsg * const cfm)
{
    BapStreamGroupAseArg arg;

    arg.clientAse = clientAse;
    arg.cfm = cfm;

    if( clientAse->state == CLIENT_ASE_STATE_RELEASING)
    {
        /* Server initiated Released Indication with no Cache */
        (void)fsm_16bit_run(&bapClientAseFsm,
                        &clientAse->state,
                        CLIENT_ASE_EVENT_RELEASED_RECEIVED,
                        (void *)streamGroup,
                        (void *)&arg);
    }
}

void bapStreamGroupAseStateCodecConfiguredNotifyReceived(BapStreamGroup * const streamGroup,
                                                         BapClientAse * const clientAse,
                                                         AscsMsg * const cfm)
{
    BapStreamGroupAseArg arg;

    arg.clientAse = clientAse;
    arg.cfm = cfm;

    if( clientAse->state == CLIENT_ASE_STATE_RELEASING)
    {
        /* Server initiated Released Indication with Cache enable */
        (void)fsm_16bit_run(&bapClientAseFsm,
                            &clientAse->state,
                            CLIENT_ASE_EVENT_RELEASED_IND_RECEIVED,
                            (void *)streamGroup,
                            (void *)&arg);

    }
    else if(( clientAse->state == CLIENT_ASE_STATE_WAIT_CODEC_CONFIGURE_CFM)||
        ( clientAse->state == CLIENT_ASE_STATE_RECONFIGURE_WAIT_CODEC_CONFIGURE_CFM))
    {
        (void)fsm_16bit_run(&bapClientAseFsm,
                        &clientAse->state,
                        CLIENT_ASE_EVENT_CODEC_CONFIGURE_CFM_RECEIVED,
                        (void *)streamGroup,
                        (void *)&arg);
    }
    else if((clientAse->state == CLIENT_ASE_STATE_CODEC_CONFIGURED) &&
        (streamGroup->cfmCounter.nExpectedCfm))
    {
        (void)fsm_16bit_run(&bapClientAseFsm,
                         &clientAse->state,
                         CLIENT_ASE_EVENT_CODEC_CONFIGURE_CFM_RECEIVED,
                         (void*)streamGroup,
                         (void*)&arg);
    }
    else
    {
        /*  Client is about to send Config codec and also received config codec
            Indication from the server */
        (void)fsm_16bit_run(&bapClientAseFsm,
                        &clientAse->state,
                        CLIENT_ASE_EVENT_CONFIGURE_IND,
                        (void *)streamGroup,
                        (void *)&arg);
    }

}

void bapStreamGroupAseStateQosConfiguredNotifyReceived(BapStreamGroup * const streamGroup,
                                                      BapClientAse * const clientAse,
                                                      AscsMsg * const cfm)
{
    BapStreamGroupAseArg arg;

    arg.clientAse = clientAse;
    arg.cfm = cfm;

    (void)fsm_16bit_run(&bapClientAseFsm,
                        &clientAse->state,
                        CLIENT_ASE_EVENT_QOS_CONFIGURE_CFM_RECEIVED,
                        (void *)streamGroup,
                        (void *)&arg);
}


void bapStreamGroupAseStateEnablingNotifyReceived(BapStreamGroup * const streamGroup,
                                                  BapClientAse * const clientAse,
                                                  AscsMsg * const cfm)
{
    BapStreamGroupAseArg arg;

    arg.clientAse = clientAse;
    arg.cfm = cfm;

    (void)fsm_16bit_run(&bapClientAseFsm,
                        &clientAse->state,
                        CLIENT_ASE_EVENT_ENABLE_CFM_RECEIVED,
                        (void *)streamGroup,
                        (void *)&arg);
}

void bapStreamGroupAseStateCisConnectCfmReceived(BapStreamGroup * const streamGroup,
                                                 BapClientAse * const clientAse,
                                                 CmIsocCisConnectCfm * const cfm,
                                                 uint8 index,
                                                 uint8 numAses)
{
    BapStreamGroupAseArg arg;
    BapResult  errorCode = (cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)?
                    BAP_RESULT_SUCCESS : BAP_RESULT_ARG_ERROR;

    arg.clientAse = clientAse;
    arg.cfm = cfm;

    if(cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        (void)fsm_16bit_run(&bapClientAseFsm,
                            &clientAse->state,
                            CLIENT_ASE_EVENT_CIS_ESTABLISHED,
                            (void *)streamGroup,
                            (void *)&arg);
    }
    else
    {
        (void)fsm_16bit_run(&bapClientAseFsm,
                            &clientAse->state,
                            CLIENT_ASE_EVENT_CIS_FAILED,
                            (void *)streamGroup,
                            (void *)&arg);
        clientAse->ase.cis->cisHandle = INVALID_CIS_HANDLE;
    }

    /* Keep track of how many cfms we've received */
    if (index == numAses)
    {
        bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                                   TRUE);

        bapUtilsSendStreamGroupCisConnectInd(streamGroup->rspPhandle,
                                             errorCode,
                                             streamGroup->id,
                                             cfm->cis_handle,
                                             (BapUnicastClientCisParam*)&cfm->cis_params,
                                             TRUE);
        if(cisCount)
        {
            cisCount--;
        }

        if (bapConfirmationCounterAllConfirmationsHaveBeenReceived(&streamGroup->cfmCounter))
        {
            if (bapConfirmationCounterAggregateConfirmationResultIsSuccess(&streamGroup->cfmCounter))
            {
                bapConfirmationCounterReset(&streamGroup->cfmCounter, 0, 0);
                bapUtilsSendStreamGroupCisConnectCfm(streamGroup->rspPhandle,
                                                     BAP_RESULT_SUCCESS,
                                                     streamGroup->id);
                cisCount = 0;
            }
        }
        else if(cisCount == 0)
        {
                bapConfirmationCounterReset(&streamGroup->cfmCounter, 0, 0);
                bapUtilsSendStreamGroupCisConnectCfm(streamGroup->rspPhandle,
                                                     BAP_RESULT_SUCCESS,
                                                     streamGroup->id);
        }
    }
}

void bapStreamGroupAseStateStartReadyNotifyReceived(BapStreamGroup * const streamGroup,
                                                    BapClientAse * const clientAse,
                                                    AscsMsg * const cfm)
{
    BapStreamGroupAseArg arg;

    arg.clientAse = clientAse;
    arg.cfm = cfm;

    (void)fsm_16bit_run(&bapClientAseFsm,
                        &clientAse->state,
                        CLIENT_ASE_EVENT_START_READY_RECEIVED,
                        (void *)streamGroup,
                        (void *)&arg);
}

void bapStreamGroupAseStateUpdateMetadataNotifyReceived(BapStreamGroup * const streamGroup,
                                                        BapClientAse * const clientAse,
                                                        AscsMsg * const cfm)
{
    BapStreamGroupAseArg arg;

    arg.clientAse = clientAse;
    arg.cfm = cfm;

    (void)fsm_16bit_run(&bapClientAseFsm,
                        &clientAse->state,
                        CLIENT_ASE_EVENT_METADATA_CFM_RECEIVED,
                        (void *)streamGroup,
                        (void *)&arg);
}

void bapStreamGroupAseStateStopReadyNotifyReceived(BapStreamGroup * const streamGroup,
                                                   BapClientAse * const clientAse,
                                                   AscsMsg * const cfm)
{
    BapStreamGroupAseArg arg;

    arg.clientAse = clientAse;
    arg.cfm = cfm;

    if( clientAse->state == CLIENT_ASE_STATE_WAIT_DISABLE_CFM)
    {
        (void)fsm_16bit_run(&bapClientAseFsm,
                        &clientAse->state,
                        CLIENT_ASE_EVENT_DISABLE_CFM_RECEIVED,
                        (void *)streamGroup,
                        (void *)&arg);
    }
    (void)fsm_16bit_run(&bapClientAseFsm,
                        &clientAse->state,
                        CLIENT_ASE_EVENT_STOP_READY_RECEIVED,
                        (void *)streamGroup,
                        (void *)&arg);
}

void bapStreamGroupAseStateDisablingNotifyReceived(BapStreamGroup * const streamGroup,
                                                   BapClientAse * const clientAse,
                                                   AscsMsg * const cfm)
{
    BapStreamGroupAseArg arg;

    arg.clientAse = clientAse;
    arg.cfm = cfm;

    if( clientAse->state != CLIENT_ASE_STATE_WAIT_DISABLE_CFM)
    {
        (void)fsm_16bit_run(&bapClientAseFsm,
                            &clientAse->state,
                            CLIENT_ASE_EVENT_DISABLE_IND,
                            (void *)streamGroup,
                            (void *)&arg);

    }
    else
    {
        (void)fsm_16bit_run(&bapClientAseFsm,
                        &clientAse->state,
                        CLIENT_ASE_EVENT_DISABLE_CFM_RECEIVED,
                        (void *)streamGroup,
                        (void *)&arg);
    }
}

void bapStreamGroupAseStateReleasingNotifyReceived(BapStreamGroup * const streamGroup,
                                                   BapClientAse * const clientAse,
                                                   AscsMsg * const cfm)
{
    BapStreamGroupAseArg arg;

    arg.clientAse = clientAse;
    arg.cfm = cfm;

    if( clientAse->state != CLIENT_ASE_STATE_WAIT_RELEASE_CFM)
    {
        (void)fsm_16bit_run(&bapClientAseFsm,
                            &clientAse->state,
                            CLIENT_ASE_EVENT_RELEASE_IND,
                            (void *)streamGroup,
                            (void *)&arg);

    }
    else
    {
        (void)fsm_16bit_run(&bapClientAseFsm,
                            &clientAse->state,
                            CLIENT_ASE_EVENT_RELEASE_CFM_RECEIVED,
                            (void *)streamGroup,
                            (void *)&arg);
    }
}

void bapStreamGroupAseStateErrorNotifyReceived(BapStreamGroup * const streamGroup,
                                               BapClientAse * const clientAse,
                                               AscsMsg * const cfm)
{
    BapStreamGroupAseArg arg;

    arg.clientAse = clientAse;
    arg.cfm = cfm;

    (void)fsm_16bit_run(&bapClientAseFsm,
                        &clientAse->state,
                        CLIENT_ASE_EVENT_FAILED_CFM_RECEIVED,
                        (void *)streamGroup,
                        (void *)&arg);
}

void bapStreamGroupDelete(BapStreamGroup * const streamGroup)
{
    CsrPmemFree(streamGroup);
}


uint8 bapStreamGroupGetAseInfo(BapStreamGroup * const streamGroup,
                                 BapAseInfo* aseInfo)
{
    uint8 numAses = 0;

    bapClientListForeach(&streamGroup->clientConnectionList,
                         listElement,
                         BapClientConnection,
                         client_connection,
                         numAses += bapConnectionGetAseInfo(&client_connection->connection, &aseInfo[numAses]));

    return numAses;
}


/*! \brief Make the BapClientAse structure RTTI information visible.
 */
type_name_enable_verify_of_external_type(BapClientAse)
                                 
void bapStreamGroupHandleAscsMsg(BapStreamGroup* const streamGroup,
                                 AscsMsg* const msg)
{
    uint8 serverAseState = msg[ASE_STATE_OFFSET];
    uint8 aseId = msg[ASE_ID_OFFSET];
    BapClientConnection* clientConnection = NULL;
    BapAse* ase = NULL;
    BapClientAse* clientAse = NULL;

    /* Find the ase with the right ase_id */
    bapClientListFindIf(&streamGroup->clientConnectionList,
                        listElement,
                        BapClientConnection,
                        clientConnection,
                        (ase = bapConnectionFindAseByAseId(&clientConnection->connection, aseId)) != NULL);

    /* client connections only have client ases, so the cast is safe  */
    if(ase)
    {
        clientAse = CONTAINER_CAST(ase, BapClientAse, ase);
    }
    else
    {
        if(serverAseState == ASE_STATE_IDLE)
        {
            /* Server Initiated Config codec,before Client created an ASE */
            /* TODO there is a possibility of collission with Client
               initiating Config codec */
        }
        clientAse = CONTAINER_CAST(ase, BapClientAse, ase);
    }

    if(clientAse)
    {
        BAP_CLIENT_INFO(" Received ASE Notification with ase Id %d ASE State %d\n",ase->id, serverAseState);
        
        /* Check the state of the notification */
        switch(serverAseState)
        {
            case ASE_STATE_IDLE:
            {
                clientAse->ase.aseStateOnServer = serverAseState;
                bapStreamGroupAseStateIdleNotifyReceived(streamGroup,
                                                         clientAse,
                                                         msg);
                /* Remove the ASE instance from the connection */
                #ifdef ASE_CLEANUP
                bapClientListRemoveIf(&streamGroup->cis_list, strm_grp_list_element,
                                      BapCis, cis,
                                      ((cis->server_is_sink_ase   && (cis->server_is_sink_ase->id   == clientAse->ase.id)) ||
                                      (cis->server_is_source_ase && (cis->server_is_source_ase->id == clientAse->ase.id))),
                                      NULL);
                bapConnectionRemoveAse(&client_connection->connection,
                                       clientAse->ase.id);
                #endif
            }
            break;

            case ASE_STATE_CODEC_CONFIGURED:
                clientAse->ase.aseStateOnServer = serverAseState;
                bapStreamGroupAseStateCodecConfiguredNotifyReceived(streamGroup,
                                                                    clientAse,
                                                                    msg);
            break;

            case ASE_STATE_QOS_CONFIGURED:
            {
                if(((clientAse->ase.aseStateOnServer == ASE_STATE_ENABLING) || 
                    (clientAse->ase.aseStateOnServer == ASE_STATE_STREAMING)) &&
                    (clientAse->ase.serverDirection == BAP_SERVER_DIRECTION_SINK))
                {
                    clientAse->ase.aseStateOnServer = serverAseState;
                    bapStreamGroupAseStateStopReadyNotifyReceived(streamGroup,
                                                                  clientAse,
                                                                  msg);
                }
                else if (clientAse->ase.aseStateOnServer == ASE_STATE_DISABLING)
                {
                    clientAse->ase.aseStateOnServer = serverAseState;
                    bapStreamGroupAseStateStopReadyNotifyReceived(streamGroup,
                                                                  clientAse,
                                                                  msg);
                }
                else
                {
                    clientAse->ase.aseStateOnServer = serverAseState;
                    bapStreamGroupAseStateQosConfiguredNotifyReceived(streamGroup,
                                                                      clientAse,
                                                                      msg);
                }
            }
            break;

            case ASE_STATE_ENABLING:
            {
                if(clientAse->ase.aseStateOnServer == ASE_STATE_ENABLING)
                {
                    bapStreamGroupAseStateUpdateMetadataNotifyReceived(streamGroup,
                                                                       clientAse,
                                                                       msg);
                }
                else
                {
                    clientAse->ase.aseStateOnServer = serverAseState;
                    bapStreamGroupAseStateEnablingNotifyReceived(streamGroup,
                                                                 clientAse,
                                                                 msg);
                }
            }
            break;

            case ASE_STATE_DISABLING:
                clientAse->ase.aseStateOnServer = serverAseState;
                bapStreamGroupAseStateDisablingNotifyReceived(streamGroup,
                                                              clientAse,
                                                              msg);
            break;

            case ASE_STATE_RELEASING:
                clientAse->ase.aseStateOnServer = serverAseState;
                bapStreamGroupAseStateReleasingNotifyReceived(streamGroup,
                                                              clientAse,
                                                              msg);
            break;
            case ASE_STATE_STREAMING:
            {
                if(clientAse->ase.aseStateOnServer == ASE_STATE_STREAMING)
                {
                    clientAse->ase.aseStateOnServer = serverAseState;
                    bapStreamGroupAseStateUpdateMetadataNotifyReceived(streamGroup,
                                                                       clientAse,
                                                                       msg);
                }
                else
                {
                    clientAse->ase.aseStateOnServer = serverAseState;
                    bapStreamGroupAseStateStartReadyNotifyReceived(streamGroup,
                                                                   clientAse,
                                                                   msg);
                }
             }
             break;

            default:
                break;
        }

    }
}

void bapStreamGroupHandleAscsCpMsg(BapStreamGroup* const streamGroup,
                                   AscsMsg* const msg)
{
    uint8 opcode = msg[ASE_CP_NOTIFY_OPCODE_OFFSET];
    uint8 numAses = msg[ASE_CP_NOTIFY_NUM_ASE_OFFSET];
    BapClientConnection* clientConnection = NULL;
    BapAse* ase = NULL;
    BapClientAse* clientAse = NULL;
    uint8 i = 0;
    AseErrorNotify* errorNotify = (AseErrorNotify*)&msg[ASE_CP_NOTIFY_ASE_ID_OFFSET];

    for( i =0; i<numAses; i++)
    {
        BAP_CLIENT_INFO(" Received ASE CP Notification ase Id %d Opcode %d Response Code %d Reason %d\n",
                         errorNotify->aseId, opcode, errorNotify->responseCode, errorNotify->reason);
        /* Check the Response code and Reason */
        if((errorNotify->responseCode != 0) || (errorNotify->reason != 0))
        {
            bapClientListFindIf(&streamGroup->clientConnectionList,
                                listElement,
                                BapClientConnection,
                                clientConnection,
                                (ase = bapConnectionFindAseByAseId(&clientConnection->connection, errorNotify->aseId)) != NULL);
    
            /*
             * client connections only have client ases, so the cast is safe
             */
            clientAse = CONTAINER_CAST(ase, BapClientAse, ase);
    
            if(clientAse)
            {
                /* Check the state of the notification */
                if((opcode >= ASE_OPCODE_CONFIG_CODEC) || (opcode <= ASE_OPCODE_RELEASE))
                {
                    bapStreamGroupAseStateErrorNotifyReceived(streamGroup,
                                                              clientAse,
                                                              (AscsMsg *)errorNotify);
                }
            }
        }
        ++errorNotify;
    }
}
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */

/**@}*/
