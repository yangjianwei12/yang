/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP connection interface.
 */

/**
 * \defgroup BapConnection BAP
 * @{
 */

#ifndef BAP_CONNECTION_H_
#define BAP_CONNECTION_H_

#include "bap_client_type_name.h"
#include "bap_client_list.h"
#include "bap_client_list_element.h"
#include "bap_ascs.h"
#include "bluetooth.h"
#include "tbdaddr.h"
#include "bap_client_lib.h"
#include "dm_prim.h"
#include "bap_ase.h"
#include "bap_cis.h"
#include "gatt_ascs_client.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

#ifdef INSTALL_GATT_SUPPORT
#include "message.h"
#endif

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
#include "bap_broadcast_assistant_private.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * forward decl
 */
struct BapConnectionVtable;
struct BAP_ASE_REGISTRY;
struct BapStreamGroup;

typedef struct
{
    BapClientListElement       listElement;
    uint8*                   msg;
    uint8                    msgSize;
    struct BapStreamGroup*     streamGroup;
    type_name_declare_rtti_member_variable
} BapPendingAscsMsg;


typedef struct
{
    uint16                 pacsStartHandle;
    uint16                 pacsEndHandle;
    ServiceHandle          srvcHndl;
}BapPacs;

typedef struct
{
    uint16                 ascsStartHandle;
    uint16                 ascsEndHandle;
    ServiceHandle          srvcHndl;
}BapAscs;

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
typedef struct
{
    uint16                 bassStartHandle;
    uint16                 bassEndHandle;
    ServiceHandle          srvcHndl;
}BapBass;
#endif

#define bap_pending_ascs_msg_initialise(pendingMsg,                \
                                        msg,                        \
                                        msgSize,                    \
                                        streamGroup)                  \
{                                                                      \
    bapClientListElementInitialise(&(pendingMsg)->list_element);        \
    (pendingMsg)->msg = (msg);                                \
    (pendingMsg)->msg_size = (msgSize);                    \
    (pendingMsg)->stream_group = (streamGroup);                         \
    type_name_initialise_rtti_member_variable(BapPendingAscsMsg, pendingMsg); \
}

/*! \brief BapConnection struct.
 */
typedef struct BapConnection
{
    BapClientListElement listElement;
    BapClientList        cisList;
    phandle_t            rspPhandle;
    TYPED_BD_ADDR_T      peerAddrt;
    BapProfileHandle     cid;
    BapRole              role;
#if defined(DEBUG)
    uint8              counter;
#endif
    BapPacs              pacs;  /* PACS Client instance */
    BapAscs              ascs;  /* ASES Client instance */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
    BapBass              bass;  /* BASS Client instance */
    BroadcastAssistant   broadcastAssistant;
#endif

    uint8              numService; /* Number of Discovered primary services */
    BapHandles         handles;
    BapResult          bapInitStatus;

    struct BAP           *bap;
    struct BapConnectionVtable const * vtable;
    type_name_declare_rtti_member_variable
} BapConnection;

typedef struct BapConnectionVtable
{
    BapResult (*handleBapPrim)(BapConnection * const, BapUPrim* );
    void (*handleAscsMsg)(BapConnection * const, AscsMsg* );
    void (*handleCmPrim)(BapConnection * const, CsrBtCmPrim* );
    BapAse* (*createAse)(BapConnection * const, struct BapStreamGroup * const, BapAseCodecConfiguration * const);
    BapResult (*codecConfigureAse)(BapConnection * const, BapInternalUnicastClientCodecConfigureReq * const);
    BapResult (*qosConfigureAse)(BapConnection * const,  BapInternalUnicastClientQosConfigureReq * const );
    BapResult (*enableAse)(BapConnection * const,BapInternalUnicastClientEnableReq * const);
    BapResult (*disableAse)(BapConnection * const, BapInternalUnicastClientDisableReq * const);
    BapResult (*releaseAse)(BapConnection * const, BapInternalUnicastClientReleaseReq * const);
    void (*deleteInstance)(BapConnection * const);
} BapConnectionVtable;

void bapConnectionInitialise(BapConnection * const connection,
                             phandle_t phandle,
                             TYPED_BD_ADDR_T const * const peerAddrt,
                             struct BAP * const bap,
                             const BapConnectionVtable * const vtable);

uint8 bapConnectionGetAseIds(BapConnection * const connection,
                               uint8* aseIds);

uint8 bapConnectionGetAseInfo(BapConnection * const connection,
                                BapAseInfo* aseInfo);

BapResult bapConnectionRcvBapPrimitive(BapConnection * const connection,
                                       BapUPrim * const primitive);

void bapConnectionVHandleCmPrim(BapConnection * const connection, CsrBtCmPrim* prim);

BapAse* bapConnectionVCreateAse(BapConnection * const connection,
                                 struct BapStreamGroup * const streamGroup,
                                 BapAseCodecConfiguration * const aseConfiguration);

BapResult bapConnectionVCodecConfigureAse(BapConnection * const connection,
                                          BapInternalUnicastClientCodecConfigureReq * const primitive);

BapResult bapConnectionVQosConfigureAse(BapConnection * const connection,
                                        BapInternalUnicastClientQosConfigureReq * const primitive);

BapResult bapConnectionVEnableAse(BapConnection * const connection,
                                  BapInternalUnicastClientEnableReq *const primitive);

BapResult bapConnectionVDisableAse(BapConnection * const connection,
                                   BapInternalUnicastClientDisableReq *const primitive);

BapResult bapConnectionVReleaseAse(BapConnection * const connection,
                                   BapInternalUnicastClientReleaseReq *const primitive);

void bapConnectionHandleRegisterAseNotificationReq(BapConnection * const connection,
                                                   BapInternalUnicastClientRegisterAseNotificationReq * const primitive);

void bapConnectionHandleBapGetRemoteAseInfoReq(BapConnection * const connection,
                                               BapInternalUnicastClientReadAseInfoReq * const primitive);

void bapConnectionRcvCmPrimitive(BapConnection * const connection,
                                 CsrBtCmPrim * const primitive);

bool bapConnectionSendAscsCmd(BapConnection * const connection,
                              uint8 * const msg,
                              uint8 msg_size);

/*
 * This is following a pattern that appears in previous code in the company:
 * the 'destroy' function implements all the code needed to cleanup a
 * connection, it can include freeing memory of internally allocated
 * components and notifying users that this instance is being destroyed (so
 * they don't try to use it any more), but this function doesn't free the
 * memory of the connection itself; that would be done by calling the
 * (virtual) delete function instead. The delete is virtual so that the
 * memory for the right instance can be freed (the memory for the derived
 * class, e.g. the bap_client_connection etc.)
 */
void bapConnectionDestroy(BapConnection * const connection);

BapCis* bapConnectionFindCis(BapConnection * const this,
                             uint8 cisId,
                             uint8 cigId);

BapCis* bapConnectionCreateCis(BapConnection * const this,
                               uint8 cisId,
                               uint8 cigId);

BapCis* bapConnectionFindCreateCis(BapConnection * const this,
                                   uint8 cisId,
                                   uint8 cigId);

BapAse * bapConnectionFindAseByAseId(BapConnection * const connection,
                                     uint8 aseId);

BapAse * bapConnectionFindAseByCisId(BapConnection * const connection,
                                     uint8 cisId);

uint8 bapConnectionFindAseByCigAndCisId(BapConnection * const connection,
                                          uint8 cigId,
                                          uint8 cisId,
                                          uint16 cisHandle,
                                          BapAse ** const ases);

BapAse* bapConnectionFindAseByCisHandle(BapConnection * const connection,
                                         uint16 cisHandle);

void bapConnectionRemoveAse(BapConnection * const connection,
                            uint8 aseId);


/*
 * Virtual functions
 */
#define bapConnectionHandleCmPrim(connection, prim) \
    ((connection)->vtable->handleCmPrim((connection), (prim)))

#define bapConnectionHandleBapPrim(connection, prim) \
    ((connection)->vtable->handleBapPrim((connection), (prim)))

#define bapConnectionHandleAscsMsg(connection, msg) \
    ((connection)->vtable->handleAscsMsg((connection), (msg)))

#define bapConnectionCreateAse(connection, streamGroup, aseConfiguration) \
    ((connection)->vtable->createAse((connection), (streamGroup), (aseConfiguration)))

#define bapConnectionConfigureAse(connection, codecConfigurations) \
    ((connection)->vtable->codecConfigureAse((connection), (codecConfigurations)))

#define bapConnectionQosConfigureAse(connection, qosConfigurations) \
    ((connection)->vtable->qosConfigureAse((connection), (qosConfigurations)))

#define bapConnectionEnableAse(connection, enableConfigurations) \
    ((connection)->vtable->enableAse((connection), (enableConfigurations)))

#define bapConnectionDisableAse(connection, disableConfigurations) \
    ((connection)->vtable->disableAse((connection), (disableConfigurations)))

#define bapConnectionReleaseAse(connection, releaseConfigurations) \
    ((connection)->vtable->releaseAse((connection), (releaseConfigurations)))


#define bapConnectionDelete(connection) \
    ((connection)->vtable->deleteInstance((connection))) /* called delete_instance to avoid clash with 'delete' */

#define bapConnectionSetCid(connection, cid) do \
    { \
        ((connection)->cid = (cid)); \
    } while (0)

#define bapConnectionSetAddrt(connection, addrt) do \
    { \
        tbdaddr_copy(&(connection)->peer_addrt, addrt); \
    } while (0)

#define bapConnectionRegisterPrimitiveHandler(connection, handler) \
    (bapClientListPush(&(connection)->bap_prim_handler_list, &(handler)->list_element))

#ifdef __cplusplus
}
#endif

#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#endif /* BAP_CONNECTION_H_ */

/**@}*/
