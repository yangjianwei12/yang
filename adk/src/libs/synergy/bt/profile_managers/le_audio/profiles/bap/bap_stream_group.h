/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP stream group interface.
 */

/**
 * \defgroup BapStreamGroup BAP
 * @{
 */

#ifndef BAP_STREAM_GROUP_H_
#define BAP_STREAM_GROUP_H_

#include "bap_confirmation_counter.h"
#include "bap_client_type_name.h"
#include "bap_client_list_element.h"
#include "bap_ase.h"
#include "dm_prim.h"
#include "bap_client_ase.h"
#include "fsm.h"
#include "bap_client_connection.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

#ifdef __cplusplus
extern "C" {
#endif

#define BAP_STREAM_GROUP_INVALID_ID (0xff)

/*! \brief BapStreamGroup struct.
 */
typedef struct BapStreamGroup
{
    BapClientListElement         listElement;
    phandle_t                    rspPhandle;
    BapProfileHandle             id;  /* cid of the connection */
    uint8                      cigId;
    uint8                      numAses;
    uint8                      numCis;
    BapClientList                cisList;
    BapClientList                clientConnectionList;
    uint32                     presentationDelayMinMicroseconds;
    uint32                     presentationDelayMaxMicroseconds;
    struct BAP                   *bap;
    BapConfirmationCounter       cfmCounter;
    type_name_declare_rtti_member_variable
} BapStreamGroup;

typedef struct
{
    BapClientAse    *clientAse;
    void             *cfm;
} BapStreamGroupAseArg;


BapStreamGroup *bapStreamGroupNew(struct BAP * const bap,
                                  phandle_t phandle,
                                  BapProfileHandle id,
                                  uint8 numAses,
                                  BapAseCodecConfiguration ** const aseCodecConfigurations);

BapResult bapStreamGroupUpdate(struct BAP * const bap,
                               BapStreamGroup *streamGroup,
                               uint8 numAses,
                               BapAseCodecConfiguration ** const aseCodecConfigurations);

BapResult bapStreamGroupConfigure(BapStreamGroup * const streamGroup,
                                  BapInternalUnicastClientCodecConfigureReq * const primitive);

BapResult bapStreamGroupQosConfigure(BapStreamGroup * const streamGroup,
                                     BapInternalUnicastClientQosConfigureReq * const primitive);

BapResult bapStreamGroupEnable(BapStreamGroup * const streamGroup,
                               BapInternalUnicastClientEnableReq * const primitive);

BapResult bapStreamGroupDisable(BapStreamGroup * const streamGroup,
                                BapInternalUnicastClientDisableReq * const primitive);

BapResult bapStreamGroupRelease(BapStreamGroup * const streamGroup,
                                BapInternalUnicastClientReleaseReq * const primitive);

void bapStreamGroupAseStateIdleNotifyReceived(BapStreamGroup * const streamGroup,
                                              BapClientAse * const clientAse,
                                              AscsMsg * const cfm);

void bapStreamGroupAseStateCodecConfiguredNotifyReceived(BapStreamGroup * const streamGroup,
                                                         BapClientAse * const clientAse,
                                                         AscsMsg * const cfm);

void bapStreamGroupAseStateQosConfiguredNotifyReceived(BapStreamGroup * const streamGroup,
                                                       BapClientAse * const clientAse,
                                                       AscsMsg * const cfm);

void bapStreamGroupAseStateEnablingNotifyReceived(BapStreamGroup * const streamGroup,
                                                  BapClientAse * const clientAse,
                                                  AscsMsg * const cfm);

void bapStreamGroupAseStateCisConnectCfmReceived(BapStreamGroup * const streamGroup,
                                                 BapClientAse * const clientAse,
                                                 CmIsocCisConnectCfm * const cfm,
                                                 uint8 index, uint8 num_ases);

void bapStreamGroupAseStateStartReadyNotifyReceived(BapStreamGroup * const streamGroup,
                                                    BapClientAse * const clientAse,
                                                    AscsMsg * const cfm);

void bapStreamGroupAseStateUpdateMetadataNotifyReceived(BapStreamGroup * const streamGroup,
                                                        BapClientAse * const clientAse,
                                                        AscsMsg * const cfm);

void bapStreamGroupAseStateStopReadyNotifyReceived(BapStreamGroup * const streamGroup,
                                                   BapClientAse * const clientAse,
                                                   AscsMsg * const cfm);

void bapStreamGroupAseStateDisablingNotifyReceived(BapStreamGroup * const streamGroup,
                                                   BapClientAse * const clientAse,
                                                   AscsMsg * const cfm);

void bapStreamGroupAseStateReleasingNotifyReceived(BapStreamGroup * const streamGroup,
                                                   BapClientAse * const clientAse,
                                                   AscsMsg * const cfm);

void bapStreamGroupAseStateErrorNotifyReceived(BapStreamGroup * const streamGroup,
                                               BapClientAse * const clientAse,
                                               AscsMsg * const cfm);

BapResult bapStreamGroupHandleBapPrim(BapStreamGroup * const streamGroup, 
                                      BapUPrim * const primitive);


void bapStreamGroupAseDestroyedInd(BapStreamGroup * const streamGroup,
                                   BapAse * const ase);

uint8 bapStreamGroupGetAseInfo(BapStreamGroup * const streamGroup,
                                 BapAseInfo* aseInfo);

void bapStreamGroupHandleAscsMsg(BapStreamGroup* const streamGroup,
                                 AscsMsg* const msg);

void bapStreamGroupHandleAscsCpMsg(BapStreamGroup* const streamGroup,
                                   AscsMsg* const msg);

struct BapCis* bapStreamGroupFindCisByCisHandle(BapStreamGroup * const streamGroup,
                                                uint16 cisHandle);

void bapStreamGroupDelete(BapStreamGroup * const connection);

#ifdef __cplusplus
}
#endif
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#endif /* BAP_STREAM_GROUP_H_ */

/**@}*/
