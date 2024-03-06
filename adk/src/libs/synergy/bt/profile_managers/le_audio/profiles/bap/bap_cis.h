/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef BAP_CIS_H
#define BAP_CIS_H

#include "bap_client_type_name.h"
#include "bap_client_list_element.h"
#include "bap_client_list.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifdef INSTALL_LEA_UNICAST_CLIENT

/*
 * Forward decl
 */
struct BapClientAse;
struct BapConnection;

typedef enum
{
    BAP_CIS_STATE_IDLE,
    BAP_CIS_STATE_ACCEPTED,
    BAP_CIS_STATE_CONNECTED
} BapCisState;

/*! \brief BapCis struct.
 */
typedef struct BapCis
{
    BapClientListElement              connListElement;
    BapClientListElement              strmGrpListElement;
    BapAse*                           serverIsSinkAse;
    BapAse*                           serverIsSourceAse;
    uint8                           cigId;
    uint8                           cisId;
    uint16                          cisHandle;
    BapCisState                       state;
    struct BapConnection*             connection;
    type_name_declare_rtti_member_variable
} BapCis;

void bapCisInitialise(BapCis * const cis,
                      uint8 cisId,
                      uint8 cigId,
                      struct BapConnection* const connection);

void bapCisAddAse(BapCis * const cis, struct BapAse* const ase);

BapAse* bapCisFindAse(BapCis * const cis, uint8 aseId);

BapAse* bapCisFindAseByCisId(BapCis * const cis, uint8 cisId);

void bapCisDelete(BapCis * const cis);

uint8 bapCisGetNumAses(BapCis * const cis);

uint8 bapCisGetAseIds(BapCis * const cis, uint8* aseIds);

/* uint8 bapCisGetAses(BapCis * const cis, BapAse* ase); */

uint8 bapCisGetAseInfo(BapCis * const cis, BapAseInfo* aseInfo);

bool bapCisIsAnAseConfigured(BapCis * const cis);

#define bapCisSetCisHandleIndex(cis, cisH) ((cis)->cis_handle_index = (cisH))

#define bapCisGetCisHandle(cis) ((cis)->cisHandle)

#define bapCisSetStateAccepted(cis)   ((cis)->state = BAP_CIS_STATE_ACCEPTED)

#define bapCisIsAccepted(cis) ((cis)->state == BAP_CIS_STATE_ACCEPTED)

#define bapCisIsConnected(cis) ((cis)->state == BAP_CIS_STATE_CONNECTED)

#define bapCisSetStateConnected(cis)    ((cis)->state = BAP_CIS_STATE_CONNECTED)

#define bapCisSetStateDisconnected(cis) ((cis)->state = BAP_CIS_STATE_IDLE)

#define bapCisSetCigId(cis, cigid)  ((cis)->cigId = (cigid))

#define bapCisSetCisId(cis, cisid)  ((cis)->cisId = (cisid))

#define bapCisGetServerIsSinkAse(cis)    ((cis)->serverIsSinkAse)

#define bapCisGetServerIsSourceAse(cis)  ((cis)->serverIsSoureAse)
#endif /* INSTALL_LEA_UNICAST_CLIENT */

#ifdef __cplusplus
}
#endif

#endif /* BAP_CIS_H */

