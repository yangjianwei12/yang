/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP BapCis interface implementation.
 */

/**
 * \addtogroup BAP
 * @{
 */
#include <stdio.h>
#include "bap_ase.h"
#include "bap_connection.h"
#include "bap_client_list.h"
#include "bap_client_ase.h"
#include "bap_client_debug.h"
#include "bap_cis.h"
#include "bap_client_list_util_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

/*! \brief RTTI information for the BapCis structure.
 */
type_name_declare_and_initialise_const_rtti_variable(BapCis,  'C','i','s','_')


void bapCisInitialise(BapCis * const cis,
                      uint8 cisId,
                      uint8 cigId,
                      BapConnection* const connection)
{
    bapClientListElementInitialise(&cis->connListElement);
    bapClientListElementInitialise(&cis->strmGrpListElement);

    cis->serverIsSinkAse = NULL;
    cis->serverIsSourceAse = NULL;

    cis->cigId = cigId;
    cis->cisId = cisId;
    cis->cisHandle = INVALID_CIS_HANDLE;
    cis->state = BAP_CIS_STATE_IDLE;
    cis->connection = connection;

    type_name_initialise_rtti_member_variable(BapCis, cis);
}

void bapCisAddAse(BapCis * const cis, BapAse* const ase)
{
    if (ase->serverDirection == BAP_SERVER_DIRECTION_SINK)
    {
        if (cis->serverIsSinkAse)
        {
            BAP_CLIENT_ERROR("Error: Setting cis->serverIsSinkAse, but it is already storing a valid pointer\n");
        }
        cis->serverIsSinkAse = ase;
    }
    else if (ase->serverDirection == BAP_SERVER_DIRECTION_SOURCE)
    {
        if (cis->serverIsSourceAse)
        {
            BAP_CLIENT_ERROR("Error: Setting cis->serverIsSourceAse, but it is already storing a valid pointer\n");
        }
        cis->serverIsSourceAse = ase;
    }
}

uint8 bapCisGetNumAses(BapCis * const cis)
{
    if ((cis->serverIsSinkAse) && (cis->serverIsSourceAse))
        return 2; /* This CIS has two ASEs */
    else if ((cis->serverIsSinkAse) || (cis->serverIsSourceAse))
        return 1; /* This CIS has one ASE */
    else
        return 0; /* This CIS has no ASEs */
}

uint8 bapCisGetAseIds(BapCis * const cis, uint8* aseIds)
{
    uint8 numAses = 0;

    if (cis->serverIsSinkAse)
        aseIds[numAses++] = cis->serverIsSinkAse->id;
    if (cis->serverIsSourceAse)
        aseIds[numAses++] = cis->serverIsSourceAse->id;

    return numAses;
}

uint8 bapCisGetAseInfo(BapCis * const cis, BapAseInfo* aseInfo)
{
    uint8 numAses = 0;

    if (cis->serverIsSinkAse)
        bapAseGetAseInfo(cis->serverIsSinkAse, &aseInfo[numAses++]);
    if (cis->serverIsSourceAse)
        bapAseGetAseInfo(cis->serverIsSourceAse, &aseInfo[numAses++]);

    return numAses;
}

BapAse* bapCisFindAse(BapCis * const cis, uint8 aseId)
{
    if (cis->serverIsSinkAse && (cis->serverIsSinkAse->id == aseId))
    {
        return cis->serverIsSinkAse;
    }
    else if (cis->serverIsSourceAse && (cis->serverIsSourceAse->id == aseId))
    {
        return cis->serverIsSourceAse;
    }
    return NULL;
}

BapAse* bapCisFindAseByCisId(BapCis * const cis, uint8 cisId)
{
    if (cis->serverIsSinkAse && (cis->serverIsSinkAse->cis->cisId == cisId))
    {
        return cis->serverIsSinkAse;
    }
    else if (cis->serverIsSourceAse && (cis->serverIsSourceAse->cis->cisId == cisId))
    {
        return cis->serverIsSourceAse;
    }
    return NULL;
}

void bapCisDelete(BapCis * const cis)
{
    if (cis->serverIsSinkAse)
        bapAseDelete(cis->serverIsSinkAse);

    if (cis->serverIsSourceAse)
        bapAseDelete(cis->serverIsSourceAse);

    CsrPmemFree(cis);
}

bool bapCisIsAnAseConfigured(BapCis * const cis)
{
    if ( cis->serverIsSourceAse &&
       ((cis->serverIsSourceAse->aseStateOnServer == BAP_ASE_STATE_CODEC_CONFIGURED) ||
        (cis->serverIsSourceAse->aseStateOnServer == BAP_ASE_STATE_QOS_CONFIGURED  )))
    {
        return TRUE;
    }
    else if ( cis->serverIsSinkAse &&
            ((cis->serverIsSinkAse->aseStateOnServer == BAP_ASE_STATE_CODEC_CONFIGURED) ||
             (cis->serverIsSinkAse->aseStateOnServer == BAP_ASE_STATE_QOS_CONFIGURED  )))
    {
        return TRUE;
    }
    return FALSE;
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */
/**@}*/
