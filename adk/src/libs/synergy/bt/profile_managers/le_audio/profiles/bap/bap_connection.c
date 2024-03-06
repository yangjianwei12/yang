/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP CONNECTION interface implementation.
 */

/**
 * \addtogroup BAP_CONNECTION_PRIVATE
 * @{
 */

#include "bap_client_list_container_cast.h"
#include "tbdaddr.h"
#include "bap_connection.h"
#include "bap_client_list_util_private.h"
#include "bap_utils.h"
#include "bap_cis.h"
#include "l2cap_prim.h"
#include "bap_gatt_msg_handler.h"
#include "gatt_ascs_client_private.h"
#include "csr_bt_gatt_lib.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

static BapResult bapConnectionVHandleBapPrim(BapConnection * const connection,
                                             BapUPrim* prim);

static void bapConnectionVHandleAscsMsg(BapConnection * const connection,
                                        AscsMsg* msg);

static void bapConnectionVDelete(BapConnection * const connection);


static const BapConnectionVtable default_connection_vtable =
{
    bapConnectionVHandleBapPrim,
    bapConnectionVHandleAscsMsg,
    bapConnectionVHandleCmPrim,
    bapConnectionVCreateAse,
    bapConnectionVCodecConfigureAse,
    bapConnectionVQosConfigureAse,
    bapConnectionVEnableAse,
    bapConnectionVDisableAse,
    bapConnectionVReleaseAse,
    bapConnectionVDelete
};

/*! \brief RTTI information for the BapConnection structure.
 */
type_name_declare_and_initialise_const_rtti_variable(BapConnection,  'A','s','C','o')

/*! \brief RTTI information for the BapPendingAscsMsg structure.
 */
type_name_declare_and_initialise_const_rtti_variable(BapPendingAscsMsg,  'A','p','A','p')

/*! \brief Make the BapCis structure RTTI information visible.
 */
type_name_enable_verify_of_external_type(BapCis)


void bapConnectionInitialise(BapConnection * const connection,
                             phandle_t phandle,
                             TYPED_BD_ADDR_T const * const peerAddrt,
                             struct BAP * const bap,
                             const BapConnectionVtable * const vtable)
{
    connection->cid = L2CA_CID_INVALID;

    connection->bap = bap;
    connection->rspPhandle = phandle;
#if defined(DEBUG)
    connection->counter = 0;
#endif
    if (peerAddrt != NULL)
    {
        tbdaddr_copy(&connection->peerAddrt, peerAddrt);
    }

    bapClientListElementInitialise(&connection->listElement);

    bapClientListInitialise(&connection->cisList);

    if (vtable == NULL)
    {
        connection->vtable = &default_connection_vtable;
    }
    else
    {
        connection->vtable = vtable;
    }

    type_name_initialise_rtti_member_variable(BapConnection, connection);
}

uint8 bapConnectionGetAseIds(BapConnection * const connection,
                               uint8* aseIds)
{

    uint8 numAses = 0;

    /*
     * Iterate through the list of CISes and:
     *     1. store all their ASE ids in the ase_id array
     *     2. count the number of ASEs found
     */
    bapClientListForeach(&connection->cisList,
                         connListElement,
                         BapCis,
                         cis,
                         numAses += bapCisGetAseIds(cis, &aseIds[numAses]));

    return numAses;
}

uint8 bapConnectionGetAseInfo(BapConnection * const connection,
                                BapAseInfo* aseInfo)
{

    uint8 numAses = 0;

    /*
     * Iterate through the list of CISes and:
     *     1. store all their ASE ids in the ase_id array
     *     2. count the number of ASEs found
     */
    bapClientListForeach(&connection->cisList,
                         connListElement,
                         BapCis,
                         cis,
                         numAses += bapCisGetAseInfo(cis, &aseInfo[numAses]));

    return numAses;
}

/****************************************************************************
 * Handle prims that are common to all connection types here.
 * Prims that are handled differently based on connection type are passed to
 * the virtual bapConnectionHandleCmPrim() function
 ****************************************************************************/
void bapConnectionRcvCmPrimitive(BapConnection * const connection,
                                 CsrBtCmPrim * const prim)
{
    /* Pass to derived class for handling */
    bapConnectionHandleCmPrim(connection, prim);
}

/****************************************************************************
 * Handle prims that are common to all connection types here
 * Prims that are handled differently based on connection type
 * are handled by the default path of the switch statement and
 * are handled by the derived classes themselves (via a virtual function call)
 ****************************************************************************/
BapResult bapConnectionRcvBapPrimitive(BapConnection * const connection,
                                       BapUPrim * const prim)
{
    BapResult result =  BAP_RESULT_SUCCESS;
    switch (prim->type)
    {

        case BAP_INTERNAL_UNICAST_CLIENT_REGISTER_ASE_NOTIFICATION_REQ:
            bapConnectionHandleRegisterAseNotificationReq(connection, 
                                                          &prim->bapInternalUnicastClientRegisterAseNotificationReq);
            break;

        case BAP_INTERNAL_UNICAST_CLIENT_READ_ASE_INFO_REQ:
            bapConnectionHandleBapGetRemoteAseInfoReq(connection,
                                                      &prim->bapInternalUnicastClientReadAseInfoReq);
            break;

        default:
            /* Pass to derived class for handling */
            result = bapConnectionHandleBapPrim(connection, prim);
            break;
    }
    return result;
}

bool bapConnectionSendAscsCmd(BapConnection * const connection,
                              uint8 * const msg,
                              uint8 msg_size)
{
    (void) msg;
    (void) msg_size;
    (void) connection;

#if defined(DEBUG)
        connection->counter++;
        TRACE("---\nCurrent client counter incremented to: %d  about to send. \n---\n", connection->counter);

#endif
#ifdef INSTALL_GATT_SUPPORT
        GattAscsWriteAsesControlPointReq((ASCSC_T *)(connection->ascs->client),
                                                       (uint16)msg_size, msg);
#endif
    {
            GattAscsWriteAsesControlPointReq(connection->ascs.srvcHndl, (CsrUint16) msg_size, msg);
        /*
         * In the case where the msg is actually sent above
         * there is no need to store the msg pointer any more, it is only
         * stored because we have generic code (below) that:
         * 1. queues up pending messages until they can be sent
         * 2. stores the response handler (and expected response type) for the messages we send.
         */
    }
    return TRUE;
}

BapAse* bapConnectionVCreateAse(BapConnection * const connection,
                                 struct BapStreamGroup * const streamGroup,
                                 BapAseCodecConfiguration * const aseConfiguration)
{
    (void)connection;
    (void)streamGroup;
    (void)aseConfiguration;
    return NULL;
}

BapResult bapConnectionVCodecConfigureAse(BapConnection * const connection,
                                          BapInternalUnicastClientCodecConfigureReq * const primitive)
{
    (void)connection;
    (void)primitive;

    return BAP_RESULT_INVALID_OPERATION;
}

BapResult bapConnectionVQosConfigureAse(BapConnection * const connection,
                                        BapInternalUnicastClientQosConfigureReq * const primitive)
{
    (void)connection;
    (void)primitive;

    return BAP_RESULT_INVALID_OPERATION;
}

BapResult bapConnectionVEnableAse(BapConnection * const connection, 
                                  BapInternalUnicastClientEnableReq *const primitive)
{
    (void)connection;
    (void)primitive;

    return BAP_RESULT_INVALID_OPERATION;
}

BapResult bapConnectionVDisableAse(BapConnection * const connection,
                                   BapInternalUnicastClientDisableReq *const primitive)
{
    (void)connection;
    (void)primitive;

    return BAP_RESULT_INVALID_OPERATION;
}
BapResult bapConnectionVReleaseAse(BapConnection * const connection,
                                   BapInternalUnicastClientReleaseReq *const primitive)
{
    (void)connection;
    (void)primitive;

    return BAP_RESULT_INVALID_OPERATION;
}

void bapConnectionDestroy(BapConnection * const connection)
{
    /* call bapCisDelete on all cises in on this connection*/
    bapClientListForeach(&connection->cisList,
                         connListElement,
                         BapCis,
                         cis,                       /* Local variable to use in the 'action to be performed' when iterating over the list. */
                         bapCisDelete(cis));     /* 'action to be performed': this code is executed on each element in the list */

}

static void bapConnectionVDelete(BapConnection * const connection)
{
    (void)connection;
    /*
     * This is pure virtual - the derived class needs to free its own memory
     * (because the base class doesn't know how much memory to free).
     */
}

BapCis* bapConnectionFindCis(BapConnection * const this,
                              uint8 cisId,
                              uint8 cigId)
{
    BapCis* cis;

    bapClientListFindIf(&this->cisList,
                        connListElement,
                        BapCis,
                        cis,
                       (cis->cisId == cisId) && (cis->cigId == cigId));
    return cis;
}

BapCis* bapConnectionCreateCis(BapConnection * const this,
                                uint8 cisId,
                                uint8 cigId)
{
    BapCis* cis = (BapCis*) CsrPmemAlloc(sizeof(BapCis));

    if (cis)
    {
        bapCisInitialise(cis, cisId, cigId, this);
        bapClientListPush(&this->cisList, &cis->connListElement);
    }
    return cis;
}

BapCis* bapConnectionFindCreateCis(BapConnection * const this,
                                    uint8 cisId,
                                    uint8 cigId)
{
    BapCis* cis = bapConnectionFindCis(this, cisId, cigId);

    if (!cis)
    {
        cis = bapConnectionCreateCis(this, cisId, cigId);
    }
    return cis;
}

BapAse * bapConnectionFindAseByAseId(BapConnection * const connection,
                                      uint8 aseId)
{
    BapCis* cis;
    BapAse* ase = NULL;

    bapClientListFindIf(&connection->cisList,
                        connListElement,
                        BapCis,
                        cis,
                        (ase = bapCisFindAse(cis, aseId)) != NULL)
    return ase;
}

BapAse * bapConnectionFindAseByCisId(BapConnection * const connection,
                                      uint8 cisId)
{
    BapCis* cis;
    BapAse* ase = NULL;

    bapClientListFindIf(&connection->cisList,
                        connListElement,
                        BapCis,
                        cis,
                        (ase = bapCisFindAseByCisId(cis, cisId)) != NULL)
    return ase;
}

uint8 bapConnectionFindAseByCigAndCisId(BapConnection * const connection,
                                          uint8 cigId,
                                          uint8 cisId,
                                          uint16 cisHandle,
                                          BapAse ** const ases)
{
    uint8 numAsesFound = 0;
    BapClientListElement* listElement;

    for (listElement = bapClientListPeekFront(&connection->cisList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
        BapCis* cis = CONTAINER_CAST(listElement, BapCis, connListElement);
        if ((cis->cigId == cigId) && (cis->cisId == cisId) && (cis->cisHandle == cisHandle))
        {
            if (cis->serverIsSinkAse)
                ases[numAsesFound++] = cis->serverIsSinkAse;
            if (cis->serverIsSourceAse)
                ases[numAsesFound++] = cis->serverIsSourceAse;
        }
    }
    return numAsesFound;
}

BapAse* bapConnectionFindAseByCisHandle(BapConnection * const connection,
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
                return cis->serverIsSinkAse;
            if (cis->serverIsSourceAse && 
                (bapAseGetCisHandle(cis->serverIsSourceAse) == cisHandle))
                return cis->serverIsSourceAse;
        }
    }
    return NULL;
}

void bapConnectionRemoveAse(BapConnection * const connection, uint8 aseId)
{
    bapClientListRemoveIf(&connection->cisList,
                          connListElement,
                          BapCis,
                          cis,
                          ((cis->serverIsSinkAse   && (cis->serverIsSinkAse->id   == aseId)) ||
                          (cis->serverIsSourceAse && (cis->serverIsSourceAse->id == aseId))),
                          bapCisDelete(cis));
}

/*
 * Must be overridden to handle prims not handled directly by bap_connection_rcv_asmp_primitive
 */
static void bapConnectionVHandleAscsMsg(BapConnection * const connection,
                                        AscsMsg* prim)
{
    (void)connection;
    (void)prim;
}
/*
 * Must be overridden to handle prims not handled directly by bapConnectionRcvBapPrimitive
 */
static BapResult bapConnectionVHandleBapPrim(BapConnection * const connection,
                                             BapUPrim* prim)
{
    (void)connection;
    (void)prim;
    return BAP_RESULT_INVALID_OPERATION;
}
/*
 * Must be overridden to handle prims not handled directly by bap_connection_rcv_dm_primitive
 */
void bapConnectionVHandleCmPrim(BapConnection * const connection, CsrBtCmPrim* prim)
{
    (void)connection;
    (void)prim;
}


void bapConnectionHandleRegisterAseNotificationReq(BapConnection * const connection,
                                                   BapInternalUnicastClientRegisterAseNotificationReq * const primitive)
{
    bapClientGattRegisterAseNotification(connection,
                                         primitive->aseId,
                                         primitive->notifyEnable);
}

void bapConnectionHandleBapGetRemoteAseInfoReq(BapConnection * const connection,
                                               BapInternalUnicastClientReadAseInfoReq * const primitive)
{
    bapClientGattAseDiscovery(connection, primitive->aseId, primitive->aseType);
}
#endif


/**@}*/
