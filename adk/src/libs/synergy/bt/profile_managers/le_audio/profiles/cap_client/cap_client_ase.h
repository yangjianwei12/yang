/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef CAP_CLIENT_ASE_H
#define CAP_CLIENT_ASE_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void capClientHandleReadAseInfoCfm(CAP_INST *inst,
          BapUnicastClientReadAseInfoCfm* cfm,
          CapClientGroupInstance *gInst);

CapClientBool capClientGetbapAseFromAseId(CsrCmnListElm_t* elem, void *value);

CapClientBool capClientGetbapAseFromCisHandle(CsrCmnListElm_t* elem, void* value);

CapClientBool capClientGetbapAseFromCisId(CsrCmnListElm_t* elem, void* value);

uint8* capClientGetAseIdForGivenCidAndState(uint32 cid,
                                     uint8 direction,
                                     uint8 aseCount,
                                     uint8 state,
                                     BapInstElement* bap);

uint8 capClientGetCisCount(BapInstElement *bap);

uint8 capClientGetAseCountInState(uint8 state,
                          BapInstElement* bap,
                          uint8 direction);

uint8 capClientGetAseCountInUse(BapInstElement *bap);

uint16 *capClientGetCisHandlesForContext(CapClientContext useCase,
                                  CapClientGroupInstance *cap,
                                  uint8 cisCount);

uint8* capClientGetAseIdsInUse(BapInstElement* bap, uint8 aseCount, CapClientContext useCase);

void capClientFlushAseIdFromBapInstance(BapInstElement* bap, uint8 aseType);

uint8* capClientGetCodecConfigurableAses(uint8 direction,
                                   uint8 aseCount,
                                   uint8 state,
                                   BapInstElement* bap,
                                   CapClientContext useCase);

CapClientBool capClientIsCisHandleUnique(uint16* cisHandles,
                                        uint8 count,
                                        uint16 cisHandle);
#endif /* INSTALL_LEA_UNICAST_CLIENT */

#endif
